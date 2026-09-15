# SSB64 PC Port — Claude Session Context

PC port of Super Smash Bros. 64 built from the complete decompilation at github.com/VetriTheRetri/ssb-decomp-re. Target integration: libultraship (LUS) + Torch asset pipeline.

## Documentation

Detailed reference material lives under `docs/`. Read the file that matches the task before touching code. When looking for a topic not listed here, run `ls docs/` and `ls docs/bugs/` to see what's available.

| Topic | File |
|-------|------|
| Project status, ROM info, dependencies, source tree layout | `docs/architecture.md` |
| C type system, decomp naming prefixes, code style, macros | `docs/c_conventions.md` |
| RDRAM / RSP / RDP / GBI / audio / threading / controller / endianness | `docs/n64_reference.md` |
| CMake build, reloc stub regen, runtime logs, LP64 compat notes | `docs/build_and_tooling.md` |
| GBI trace capture (port + M64P plugin) and `gbi_diff.py` usage | `docs/debug_gbi_trace.md` |
| IDO BE bitfield layout audit (compile + rabbitizer disasm to verify port struct bit positions) | `docs/debug_ido_bitfield_layout.md` |
| Deblob system (typed fighter asset slices, manifests, synthesis, invariants, mod drop-ins) | `docs/deblob.md` |
| Resolved bugs (index + per-bug root cause / fix write-ups) | `docs/bugs/README.md` |

Ongoing investigations and handoff notes are loose `.md` files at the top level of `docs/` — check there before starting work on rendering, collision, or animation issues so you don't duplicate prior effort.

When you fix a new significant bug, add an entry under `docs/bugs/` using the slug pattern `<topic>_<YYYY-MM-DD>.md` and link it from `docs/bugs/README.md`.

---

## GitHub Issue Access

When asked to inspect GitHub issues, prefer the GitHub connector. If issue tools
are not visible yet, first run tool discovery for "GitHub issue fetch/view" so
the connector exposes `_fetch_issue` and `_fetch_issue_comments`, then fetch with
`repository_full_name: "JRickey/BattleShip"`.

The local GitHub CLI is also authenticated as `JRickey` and has admin access to
`JRickey/BattleShip`. If the human-formatted `gh issue view` output is blank or
unreliable, use the JSON/template path instead:

```bash
gh issue view <number> -R JRickey/BattleShip --json number,title,state,author,body,url,comments,labels
```

Known-good check from 2026-06-01: issue #209 and its comments were accessible
through both the connector and `gh --json`; #209 had no comments at that time.

---

## Parallel Sessions — Worktree Workflow

Multiple Claude windows working in the same checkout will clobber each other's source edits and build outputs. **Every parallel session works in its own git worktree.**

### Spinning up a new worktree

```bash
./scripts/new-worktree.sh <slug>           # configure only (fast)
./scripts/new-worktree.sh <slug> --build   # configure + full Debug compile
./scripts/new-worktree.sh <slug> --base some-branch --release
```

Output lands at `.claude/worktrees/<slug>` on branch `agent/<slug>`. The script:
1. Creates the worktree and branch.
2. Symlinks `baserom.us.z64` (gitignored, too large to duplicate).
3. **Independently clones `libultraship`, `torch`, and `decomp`** from the main tree's local submodule checkouts (picks up pinned SHAs that may not be pushed to the forks yet), then resets each submodule's `origin` to whatever URL the main tree's submodule uses — usually SSH so pushes work.
4. Regenerates gitignored codegen (`reloc_data.h`, `yamls/us/reloc_*.yml`, credits encodings).
5. Runs `cmake -B build` inside the worktree (and compiles if `--build` given).

### What this gives you

- **Full edit authority everywhere** — any file under `decomp/src/`, `decomp/include/`, `port/`, `libultraship/`, `torch/` is fair game. Submodule checkouts are real independent clones, not symlinks.
- **Zero collision** with other windows on source, build artifacts, or submodule state.
- **Normal git flow for submodule changes**:
  1. Edit and commit inside `<worktree>/decomp/`, `<worktree>/libultraship/`, or `<worktree>/torch/`.
  2. Push to the fork: `git -C <worktree>/<sm> push origin <branch>`.
     - `decomp` → `port-patches` on `JRickey/ssb-decomp-re`
     - `libultraship` → `ssb64` on `JRickey/libultraship`
     - `torch` → `ssb64` on `JRickey/Torch`
  3. In the outer worktree, bump the submodule pointer: `git add <sm> && git commit -m "Bump <sm>: <summary>"`.
  4. When the outer branch lands on main, the pointer update goes with it.

### Merging back to main

The outer worktree is a normal branch (`agent/<slug>`). Merge or PR it into `main` like any other branch. Submodule pointer bumps ride along in the commits.

### Cleanup

```bash
git worktree remove .claude/worktrees/<slug>
git branch -D agent/<slug>
```

Stale worktrees under `.claude/worktrees/` from past sessions are fine to remove — check `git worktree list` and prune anything you don't recognize.

### Gotchas

- **Never use relative `build` paths in Bash tool calls** — Claude Code resets cwd between `Bash` calls. `cmake --build build` from the project root builds the main tree, not the worktree. Always use absolute paths: `cmake --build <worktree>/build ...`.
- **Cap build parallelism at `-j 4` on the M1 16 GB machine.** Bare `-j` lets make spawn one clang per logical core; libultraship's Debug C++ TUs hold 1–2 GB resident each, so 8 in parallel push the laptop into swap and pin the fans for the whole compile. Always: `cmake --build <worktree>/build --target ssb64 -j 4`. Push to `-j 6` only when you know nothing else heavy is open. Worktree first-builds are full from-scratch (no shared cache with main tree), so this matters most the first time.
- The binary loads `BattleShip.o2r` (ROM-derived, user-extracted) and `f3d.o2r` (shaders) from its CWD at launch; without them it exits with `archive ... does not exist`. `new-worktree.sh` symlinks both from the main tree's `build/` into the worktree's `build/`. If the main tree has never been extracted, run `cmake --build <main-tree>/build --target ExtractAssets` there first so the symlinks resolve. (`ssb64.o2r` was an early-development port-asset archive; it is no longer produced or loaded — the build never contains ROM-derived data beyond the user's own first-run `BattleShip.o2r`.)

---

## Agent Directives

### Pre-Work

1. **THE "STEP 0" RULE**: Before any structural refactor on a file >300 LOC, first remove dead code, unused exports, unused imports, and debug logs. Commit cleanup separately.

2. **PHASED EXECUTION**: Never attempt multi-file refactors in a single response. Break work into phases. Complete Phase 1, run verification, wait for approval before Phase 2. Max 5 files per phase.

### Code Quality

3. **THE SENIOR DEV OVERRIDE**: If architecture is flawed, state is duplicated, or patterns are inconsistent — propose and implement structural fixes. Ask: "What would a senior, experienced, perfectionist dev reject in code review?" Fix all of it.

4. **FORCED VERIFICATION**: Do not report a task complete until you have run the build and fixed all errors. If no build is configured yet, state that explicitly.

5. **DECOMP PRESERVATION — preserve behavior, not byte-matching**: The decomp describes the *game*, not the build. Keep IDO idioms (goto, odd casts, temp variables) that encode original N64 semantics — those are load-bearing and must not be "modernized." But don't preserve **compiler compat shims** (warning suppressions, permissive flags, header shortcuts) that hurt port stability just to avoid touching decomp source. If a suppressed diagnostic is masking real bugs on modern LP64 toolchains (e.g., `-Wno-implicit-function-declaration` silently truncating 64-bit pointer returns to `int`), fix the root cause — add the missing include, wrap a port fix in `#ifdef PORT`, or adjust the decomp file itself — rather than keeping the suppression. **Accuracy to game behavior > accuracy to ROM bytes.** When choosing between stability and ROM-matching, choose stability and document the deviation in `docs/bugs/`.

### Context Management

6. **SUB-AGENT SWARMING**: For tasks touching >5 independent files, launch parallel sub-agents. Each agent gets its own context window.

7. **CONTEXT DECAY AWARENESS**: After 10+ messages, re-read any file before editing. Do not trust memory of file contents.

8. **FILE READ BUDGET**: For files over 500 LOC, use offset and limit parameters to read in chunks.

9. **EDIT INTEGRITY**: Before every edit, re-read the file. After editing, verify the change applied correctly. Never batch >3 edits to the same file without a verification read.

10. **NO SEMANTIC SEARCH**: When renaming or changing any function/type/variable, search separately for: direct calls, type references, string literals, dynamic references, re-exports, and tests.
