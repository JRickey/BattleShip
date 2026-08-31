#!/usr/bin/env bash
# new-worktree.sh — spin up an isolated git worktree for a parallel Claude session.
#
# Usage:
#   scripts/new-worktree.sh <slug> [--base <branch>] [--build] [--release]
#
# Example:
#   scripts/new-worktree.sh fighter-hitbox-review
#   scripts/new-worktree.sh ui-refactor --base main --build
#
# What it does:
#   1. Creates a worktree at .claude/worktrees/<slug> on new branch agent/<slug>
#   2. Symlinks every baserom.{us,jp}.{z64,n64,v64} that exists in the main
#      tree (gitignored, too big to duplicate). The compiled binary picks
#      its region's baserom at first-run; symlinking both means the same
#      worktree can build either flavor without re-linking.
#   3. Clones libultraship, torch, and decomp as independent repos inside
#      the worktree:
#        - Source = main tree's local submodule checkout (picks up pinned SHAs
#          that may only exist locally, never pushed to the fork)
#        - Origin URL reset to the real SSH upstream from .gitmodules, so
#          `git push` inside the worktree's submodule goes to GitHub.
#   4. Regenerates gitignored codegen (reloc stubs, yamls, reloc table, credits).
#   5. Runs `cmake -B build` (configure only; add --build to also compile).
#
# Resulting worktree is fully editable:
#   - Edit any file in decomp/src/, decomp/include/, port/, libultraship/,
#     torch/
#   - Commit normally inside libultraship/ / torch/, then push to the fork
#   - In the outer worktree: `git add libultraship && git commit` bumps the
#     submodule pointer; push that commit up when merging back to main.
#
# Parallel windows in separate worktrees never collide on source, build
# outputs, or submodule checkouts.

set -euo pipefail

# ── Parse args ──
SLUG=""
BASE="main"
DO_BUILD=0
CONFIG="Debug"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --base)    BASE="$2"; shift 2 ;;
        --build)   DO_BUILD=1; shift ;;
        --release) CONFIG="Release"; shift ;;
        --debug)   CONFIG="Debug"; shift ;;
        -h|--help) sed -n '2,30p' "$0"; exit 0 ;;
        -*)        echo "Unknown flag: $1" >&2; exit 1 ;;
        *)
            if [[ -n "$SLUG" ]]; then
                echo "ERROR: multiple slugs given ($SLUG, $1)" >&2; exit 1
            fi
            SLUG="$1"; shift ;;
    esac
done

if [[ -z "$SLUG" ]]; then
    echo "usage: $(basename "$0") <slug> [--base <branch>] [--build] [--release]" >&2
    exit 1
fi

# ── Paths ──
ROOT="$(git rev-parse --show-toplevel)"
WT_DIR="$ROOT/.claude/worktrees/$SLUG"
BRANCH="agent/$SLUG"

step()  { printf '\n\033[36m=== %s ===\033[0m\n' "$1"; }
fail()  { printf '\033[31mERROR: %s\033[0m\n' "$1" >&2; exit 1; }

[[ -e "$WT_DIR" ]] && fail "$WT_DIR already exists. Remove it or pick a different slug."

# Discover every baserom variant present in the main tree. The worktree
# inherits whatever the user has, so a US-only or JP-only setup still
# works — only a tree with neither US nor JP fails.
declare -a ROMS=()
for region in us jp; do
    for ext in z64 n64 v64; do
        if [[ -f "$ROOT/baserom.$region.$ext" ]]; then
            ROMS+=("baserom.$region.$ext")
            break  # first extension wins per region
        fi
    done
done
[[ ${#ROMS[@]} -gt 0 ]] || fail "no baserom.{us,jp}.{z64,n64,v64} in $ROOT"

# ── 1. Worktree + branch ──
step "Creating worktree $WT_DIR on branch $BRANCH (base: $BASE)"
git -C "$ROOT" worktree add "$WT_DIR" -b "$BRANCH" "$BASE"

# ── 2. ROM symlinks (gitignored, ~12 MB each) ──
step "Symlinking baseroms (${ROMS[*]})"
for rom in "${ROMS[@]}"; do
    ln -sf "$ROOT/$rom" "$WT_DIR/$rom"
done

# ── 3. Independent submodule clones ──
# Submodules pin SHAs to JRickey/libultraship and JRickey/Torch on `ssb64`.
# Those SHAs frequently exist only in the main checkout's local .git/modules
# (branch hasn't been pushed), so `git submodule update --init` inside a
# fresh worktree fails with "upload-pack: not our ref".
#
# Instead: clone from the main tree's working submodule (git follows the
# .git gitfile → .git/modules/<name>), then reset origin to the real SSH
# upstream so pushes from the worktree go to GitHub.
for sm in libultraship torch decomp; do
    # Some submodules may not be registered on older base branches
    # (e.g. `decomp` was added on agent/decomp-submodule). Skip silently
    # if the main tree doesn't track it yet.
    if [[ -z "$(git -C "$WT_DIR" config -f .gitmodules "submodule.$sm.path" 2>/dev/null)" ]]; then
        printf '\033[33m  Skipping submodule %s (not configured in main tree .gitmodules)\033[0m\n' "$sm"
        continue
    fi
    pinned_sha="$(git -C "$WT_DIR" rev-parse "HEAD:$sm")"
    # Prefer the main tree's configured origin (often SSH) over .gitmodules URL
    # (often HTTPS) so the worktree inherits whatever auth method the user has
    # set up for push.
    origin_url="$(git -C "$ROOT/$sm" remote get-url origin 2>/dev/null \
                  || git -C "$ROOT" config -f .gitmodules "submodule.$sm.url")"
    sm_branch="$(git -C "$ROOT" config -f .gitmodules "submodule.$sm.branch" || echo "")"

    step "Submodule $sm → $pinned_sha (origin: $origin_url)"
    rm -rf "$WT_DIR/$sm"
    git clone --no-local --quiet "$ROOT/$sm" "$WT_DIR/$sm"
    git -C "$WT_DIR/$sm" remote set-url origin "$origin_url"
    # The local source clone only carries refs/heads/* of the main checkout, so
    # SHAs reachable only via remote-tracking branches (e.g. an older tag's pin
    # that lives on a feature branch) won't be in the new clone. If the
    # checkout misses, fetch from the real fork and retry.
    if ! git -C "$WT_DIR/$sm" checkout --quiet "$pinned_sha" 2>/dev/null; then
        printf '  Pinned SHA not in local clone; fetching from %s\n' "$origin_url"
        git -C "$WT_DIR/$sm" fetch --quiet origin
        git -C "$WT_DIR/$sm" checkout --quiet "$pinned_sha"
    fi
    if [[ -n "$sm_branch" ]]; then
        # Re-create the tracking branch so `git push` from detached HEAD is obvious.
        git -C "$WT_DIR/$sm" checkout -B "$sm_branch" --quiet
    fi
done

# ── 4. Regenerate gitignored codegen ──
# reloc_data.h, yamls/us/reloc_*.yml, credits .encoded/.metadata are all
# gitignored and must be rebuilt on every fresh checkout before CMake runs.
step "Regenerating reloc codegen"
# Each tool resolves its root via __file__, so absolute-path invocation is safe.
python3 "$WT_DIR/tools/generate_reloc_stubs.py"
( cd "$WT_DIR" && python3 tools/generate_yamls.py )
( cd "$WT_DIR" && python3 tools/generate_fighter_slices.py )
( cd "$WT_DIR" && python3 tools/generate_reloc_table.py )

step "Encoding credits text"
(
    cd "$WT_DIR/decomp/src/credits"
    for f in staff.credits.us.txt titles.credits.us.txt; do
        python3 "$WT_DIR/tools/creditsTextConverter.py" "$f" > /dev/null
    done
    for f in info.credits.us.txt companies.credits.us.txt; do
        python3 "$WT_DIR/tools/creditsTextConverter.py" -paragraphFont "$f" > /dev/null
    done
)

# ── 5. CMake configure ──
if command -v ninja >/dev/null 2>&1; then GEN="Ninja"; else GEN="Unix Makefiles"; fi

step "Configuring CMake ($GEN, $CONFIG)"
cmake -S "$WT_DIR" -B "$WT_DIR/build" -G "$GEN" -DCMAKE_BUILD_TYPE="$CONFIG"

# ── 5b. Symlink extracted assets ──
# Torch extraction (BattleShip.o2r) is slow and produces bytewise-identical
# output for a given baserom. The binary loads BattleShip.o2r (ROM-derived)
# and f3d.o2r (shaders) from its CWD on launch — without them the game
# prints "archive ... does not exist" and exits. Reuse the main tree's
# extracted assets via symlink so parallel worktrees don't each re-extract.
step "Symlinking extracted assets (BattleShip.o2r / f3d.o2r)"
linked_any=0
for asset in BattleShip.o2r f3d.o2r; do
    src=""
    for cand in "$ROOT/build/$asset" "$ROOT/$asset"; do
        if [[ -f "$cand" ]]; then src="$cand"; break; fi
    done
    if [[ -n "$src" ]]; then
        ln -sf "$src" "$WT_DIR/build/$asset"
        linked_any=1
    else
        printf '  warn: %s not found in main tree (%s/build or %s) — extract it there first\n' \
            "$asset" "$ROOT" "$ROOT" >&2
    fi
done
if [[ $linked_any -eq 0 ]]; then
    printf '\033[33m  No assets symlinked. Build + extract in the main tree first (cmake --build %s/build --target ExtractAssets) before launching the worktree binary.\033[0m\n' "$ROOT" >&2
fi

# ── 6. Optional: compile ──
if [[ $DO_BUILD -eq 1 ]]; then
    if command -v sysctl >/dev/null 2>&1; then
        JOBS="$(sysctl -n hw.ncpu)"
    elif command -v nproc >/dev/null 2>&1; then
        JOBS="$(nproc)"
    else
        JOBS=4
    fi
    step "Building ssb64 ($JOBS jobs)"
    cmake --build "$WT_DIR/build" --target ssb64 --config "$CONFIG" -j "$JOBS"
fi

# ── Done ──
step "Worktree ready"
cat <<EOF
  Path:     $WT_DIR
  Branch:   $BRANCH  (base: $BASE)
  Build:    $WT_DIR/build       ($GEN, $CONFIG)
  ROM:      symlinked
  Submods:  libultraship, torch, decomp — independent clones,
            origin set to fork

  Point a new Claude window at: $WT_DIR
  Build:    cmake --build $WT_DIR/build --target ssb64 -j
  Remove:   git worktree remove $WT_DIR && git branch -D $BRANCH
EOF
