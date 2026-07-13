#!/usr/bin/env bash
# Builds BattleShip as a self-contained macOS .app bundle.
#
# Output: <repo-root>/dist/BattleShip.app
#
# Layout produced:
#   BattleShip.app/
#     Contents/
#       Info.plist
#       MacOS/
#         BattleShip                 — main executable
#         torch                      — sidecar for first-run extraction
#       Resources/
#         f3d.o2r                    — Fast3D shader archive (ROM-independent)
#         config.yml                 — Torch extraction config
#         yamls/us/*.yml             — Torch extraction recipes
#         gamecontrollerdb.txt       — SDL controller mappings
#
# The bundle is built with NON_PORTABLE=ON so the runtime resolves saves,
# BattleShip.cfg.json, and the user's extracted BattleShip.o2r out of the
# OS app-data dir (~/Library/Application Support/BattleShip/) instead of cwd.
#
# Notes:
# - The .app does NOT include BattleShip.o2r — that's ROM-derived and gets
#   extracted on first launch via the ImGui wizard.
# - Local builds ad-hoc sign the bundle with the mod-loader entitlements.
#   CI sets MACOS_CODESIGN_IDENTITY + MACOS_NOTARY_* to Developer ID-sign
#   the app, sign the DMG, submit it to Apple's notary service, and staple
#   the returned ticket before upload.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# ROM version: us (default) or jp. The JP build is a SEPARATE
# application — its own binary name, .app, .dmg, app-data dir and
# bundle id — so a user can keep both installed and they never touch
# each other's ROM/o2r/saves. APP_NAME mirrors CMake SSB64_APP_NAME /
# OUTPUT_NAME (BattleShip vs BattleShip-JP); the built binary is named
# accordingly. US keeps the historical "BattleShip" identity so existing
# installs / the in-app updater / release links are unaffected.
VER="${SSB64_VERSION:-us}"
[[ "$VER" == "us" || "$VER" == "jp" ]] || { echo "SSB64_VERSION must be us|jp" >&2; exit 1; }
BUILD_DIR="$ROOT/build-bundle-$VER"
DIST_DIR="$ROOT/dist"
if [[ "$VER" == "jp" ]]; then
    APP_NAME="BattleShip-JP"
    APP_BUNDLE_ID="com.ssb-decomp-re.battleship-jp"
else
    APP_NAME="BattleShip"
    APP_BUNDLE_ID="com.ssb-decomp-re.battleship"
fi
APP="$DIST_DIR/$APP_NAME.app"
JOBS="${JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || echo 4)}"

# Bundle version for Info.plist. Mirrors CMake's BATTLESHIP_VERSION
# resolution (the string the in-app updater reports) so the Finder
# Get Info version matches what the app says about itself:
#   1. $BATTLESHIP_VERSION env — release CI sets this to the tag.
#   2. Nearest release tag from git.
#   3. "0.0.0" fallback for tarball builds (no git metadata).
# CFBundle*Version want dotted numerics, so strip the tag's leading "v"
# and fall back if what's left isn't numeric.
APP_TAG="${BATTLESHIP_VERSION:-$(git -C "$ROOT" describe --tags --abbrev=0 2>/dev/null || true)}"
APP_VERSION="${APP_TAG#v}"
[[ "$APP_VERSION" =~ ^[0-9]+(\.[0-9]+)*$ ]] || APP_VERSION="0.0.0"

step() { printf '\n\033[36m=== %s ===\033[0m\n' "$1"; }
fail() { printf '\033[31mERROR: %s\033[0m\n' "$1" >&2; exit 1; }
truthy() {
    case "${1:-}" in
        1|true|TRUE|yes|YES|on|ON) return 0 ;;
        *) return 1 ;;
    esac
}
json_get() {
    local field="$1"
    python3 -c 'import json, sys; print(json.load(sys.stdin).get(sys.argv[1], "") or "")' "$field"
}

[[ "$(uname -s)" == "Darwin" ]] || fail "package-macos.sh runs on macOS only"

# ── 0. Run codegen scripts that don't need the ROM ──
# Encoded credit files are gitignored (input text is in decomp/src/credits/),
# so a fresh checkout (CI or otherwise) must run the encoder before
# cmake builds scstaffroll.c. ROM-independent — same step CMake's
# GenerateCreditsAssets target runs.
step "Encoding credits text"
(
    cd "$ROOT/decomp/src/credits"
    for f in staff.credits.us.txt titles.credits.us.txt; do
        python3 "$ROOT/tools/creditsTextConverter.py" "$f" > /dev/null
    done
    for f in info.credits.us.txt companies.credits.us.txt; do
        python3 "$ROOT/tools/creditsTextConverter.py" -paragraphFont "$f" > /dev/null
    done
)

# ── 1. Configure + build with NON_PORTABLE=ON ──
# Use ccache as the compiler launcher when it's on PATH (CI installs it and
# warms a cross-run cache; harmless locally when it isn't present). Shrinks the
# compile window so a slow / mid-compile-stalling hosted runner is likelier to
# finish before the timeout. The ${CCACHE_ARGS[@]+...} guard keeps the
# empty-array expansion safe under `set -u` on the bash 3.2 macOS ships.
CCACHE_ARGS=()
if command -v ccache >/dev/null 2>&1; then
    CCACHE_ARGS=(-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache)
fi
step "Configuring release build with NON_PORTABLE=ON"
cmake -B "$BUILD_DIR" "$ROOT" \
    -DCMAKE_BUILD_TYPE=Release \
    -DNON_PORTABLE=ON \
    -DSSB64_VERSION="$VER" \
    "${CCACHE_ARGS[@]+"${CCACHE_ARGS[@]}"}" \
    >/dev/null

step "Building BattleShip + torch"
cmake --build "$BUILD_DIR" -j"$JOBS"

# Build the f3d.o2r shader archive (ROM-independent, just zips the LUS
# shaders directory). CMake's GenerateF3DO2R target produces this at
# $ROOT/f3d.o2r — reuse the same recipe rather than re-implement.
step "Packaging Fast3D shader archive"
F3D_O2R="$BUILD_DIR/f3d.o2r"
rm -f "$F3D_O2R"
( cd "$ROOT/libultraship/src/fast" && zip -rq "$F3D_O2R" shaders )
[[ -f "$F3D_O2R" ]] || fail "f3d.o2r was not created"

# ── 2. Locate built artifacts ──
# CMake's OUTPUT_NAME == SSB64_APP_NAME == $APP_NAME, so the binary is
# named BattleShip (US) or BattleShip-JP (JP).
SSB64_BIN="$BUILD_DIR/$APP_NAME"
TORCH_BIN="$BUILD_DIR/TorchExternal/src/TorchExternal-build/torch"
[[ -x "$SSB64_BIN" ]] || fail "BattleShip binary not found at $SSB64_BIN"
[[ -x "$TORCH_BIN" ]] || fail "torch binary not found at $TORCH_BIN"

# ── 3. Assemble the bundle ──
step "Assembling $APP"
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources/yamls/$VER"

cp "$SSB64_BIN"  "$APP/Contents/MacOS/$APP_NAME"
cp "$TORCH_BIN"  "$APP/Contents/MacOS/torch"
cp "$F3D_O2R"    "$APP/Contents/Resources/f3d.o2r"
cp "$ROOT/gamecontrollerdb.txt" "$APP/Contents/Resources/gamecontrollerdb.txt"
cp "$ROOT/config.yml" "$APP/Contents/Resources/config.yml"
cp "$ROOT/yamls/$VER/"*.yml "$APP/Contents/Resources/yamls/$VER/"
# Region-aware app icon: US uses assets/icon.icns, JP uses
# assets/icon-jp.icns (added with the JP bifurcation so a user with
# both .app bundles sees distinct icons in Finder / Dock). The bundle
# always names it AppIcon.icns to match Info.plist's CFBundleIconFile.
ICNS_SRC="$ROOT/assets/icon.icns"
[[ "$VER" == "jp" ]] && ICNS_SRC="$ROOT/assets/icon-jp.icns"
cp "$ICNS_SRC" "$APP/Contents/Resources/AppIcon.icns"

# Bundle the ESC menu fonts. Menu.cpp::FindMenuAssetPath walks up from
# RealAppBundlePath() (= Contents/Resources inside an .app on macOS)
# checking each parent for assets/custom/fonts/<name> — so placing the
# TTFs at Contents/Resources/assets/custom/fonts/ matches first
# iteration. Without this the menu silently falls back to ImGui's
# default font.
#
# OFL 1.1 §1 requires the license text to accompany each redistributed
# font file, so the *-OFL.txt files ship alongside the .ttf they govern.
mkdir -p "$APP/Contents/Resources/assets/custom/fonts"
cp "$ROOT/assets/custom/fonts/Montserrat-Regular.ttf"  "$APP/Contents/Resources/assets/custom/fonts/"
cp "$ROOT/assets/custom/fonts/Montserrat-OFL.txt"      "$APP/Contents/Resources/assets/custom/fonts/"
cp "$ROOT/assets/custom/fonts/Inconsolata-Regular.ttf" "$APP/Contents/Resources/assets/custom/fonts/"
cp "$ROOT/assets/custom/fonts/Inconsolata-OFL.txt"     "$APP/Contents/Resources/assets/custom/fonts/"

# Project LICENSE + verbatim upstream LICENSE files for the submodules
# whose compiled code is in this .app. MIT requires the upstream
# copyright + permission notice to ride along with redistributed copies.
cp "$ROOT/LICENSE" "$APP/Contents/Resources/LICENSE"
mkdir -p "$APP/Contents/Resources/licenses"
if [[ -f "$ROOT/libultraship/LICENSE" ]]; then
    cp "$ROOT/libultraship/LICENSE" "$APP/Contents/Resources/licenses/libultraship-LICENSE.txt"
else
    fail "libultraship/LICENSE not found — submodules not initialized?"
fi
if [[ -f "$ROOT/torch/LICENSE" ]]; then
    cp "$ROOT/torch/LICENSE" "$APP/Contents/Resources/licenses/torch-LICENSE.txt"
else
    fail "torch/LICENSE not found — submodules not initialized?"
fi
cat > "$APP/Contents/Resources/licenses/README.txt" <<'EOF'
This directory contains license texts for third-party components whose
compiled code is included in this BattleShip distribution:

  - libultraship-LICENSE.txt  (MIT, Copyright (c) 2022 kenix3)
  - torch-LICENSE.txt         (MIT, Copyright (c) 2023 Lywx)

Bundled font licenses (SIL Open Font License 1.1) live alongside the
font files at assets/custom/fonts/.

The BattleShip project's own MIT license is in ../LICENSE in this bundle.

Additional libraries dynamically linked at runtime (SDL2, GLEW, libzip,
nlohmann_json, tinyxml2, spdlog, fmt, hidapi-via-libultraship) are
distributed under their respective upstream licenses (zlib, modified
BSD, BSD-3-Clause, MIT). Refer to those upstream packages for full
license texts.
EOF

# ── 4. Info.plist ──
# Minimal but sufficient: bundle ID, version, executable name, high-DPI flag.
# CFBundleIdentifier picks the same reverse-DNS the user's app-data dir
# is scoped to (battleship), keeping save state stable across signed/unsigned
# rebuilds.
cat > "$APP/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>                <string>$APP_NAME</string>
    <key>CFBundleDisplayName</key>         <string>$APP_NAME</string>
    <key>CFBundleIdentifier</key>          <string>$APP_BUNDLE_ID</string>
    <key>CFBundleVersion</key>             <string>$APP_VERSION</string>
    <key>CFBundleShortVersionString</key>  <string>$APP_VERSION</string>
    <key>CFBundlePackageType</key>         <string>APPL</string>
    <key>CFBundleSignature</key>           <string>????</string>
    <key>CFBundleExecutable</key>          <string>$APP_NAME</string>
    <key>CFBundleIconFile</key>            <string>AppIcon</string>
    <key>LSMinimumSystemVersion</key>      <string>11.0</string>
    <key>NSHighResolutionCapable</key>     <true/>
    <key>NSSupportsAutomaticGraphicsSwitching</key> <true/>
</dict>
</plist>
EOF

# Make the binaries executable (cp preserves mode, but be defensive).
chmod +x "$APP/Contents/MacOS/$APP_NAME" "$APP/Contents/MacOS/torch"

# ── 4a. Bundle Homebrew dylib dependencies (fixes #43) ──
# CMake links BattleShip / torch against Homebrew dylibs (SDL2, GLEW,
# libzip, tinyxml2, spdlog, fmt, …) using their absolute install paths
# (`/opt/homebrew/opt/<pkg>/lib/lib*.dylib`). On a developer machine the
# .app launches fine because every load command resolves verbatim. On
# any user's Mac without Homebrew at the same prefix and exact package
# versions, dyld bails before main() with `Library not loaded:
# /opt/homebrew/*/libSDL2-2.0.0.dylib`. This is the macOS analogue of
# bundling SDL2.dll on Windows / `.so` deps via linuxdeploy on Linux —
# the package script must walk the binaries' transitive Homebrew deps,
# stage them into Contents/Frameworks/, rewrite each dylib's id to
# `@rpath/lib*.dylib`, retarget the binaries' references via
# `install_name_tool`, and add an `LC_RPATH` of `@executable_path/../Frameworks`.
#
# `dylibbundler` (Homebrew package) automates all of that. `-of` =
# overwrite-files (idempotent re-runs), `-b` = bundle transitive deps,
# `-x` = files to fix (repeatable for torch alongside BattleShip),
# `-d` = destination directory, `-p` = install_name prefix, `-cd` =
# create destination, `-ns` = skip dylibbundler's own adhoc codesign
# pass since the bundle-level `codesign --deep --force` below resigns
# everything in one shot.
#
# `-p @executable_path/../Frameworks/` makes the rewritten install_names
# fully self-resolving (dyld substitutes `@executable_path` with the
# directory of the loading executable — Contents/MacOS — so the path
# lands at Contents/Frameworks/lib*.dylib).  Setting `-p @rpath/` would
# require an LC_RPATH on the binary, and dylibbundler's existing-rpath
# rewrite stomps on that path with literal `@rpath/`, producing a
# recursive load command that dyld can't resolve.  Using the absolute
# `@executable_path` form sidesteps that.
step "Bundling Homebrew dylib dependencies"
command -v dylibbundler >/dev/null \
    || fail "dylibbundler not in PATH — install with: brew install dylibbundler"
# dylibbundler reads stdin and PROMPTS ("...does not exist. Try again") when it
# can't auto-locate a dependency — which BLOCKS FOREVER in non-interactive CI.
# The culprit is libtcc.dylib: TinyCC (the v1.4 mod-scripting runtime) is built
# SHARED in our own build tree (_deps/tinycc-*), not a system/Homebrew lib, so
# dylibbundler can't find it on its own. (Linux's linuxdeploy follows the
# binary's rpath and bundles libtcc.so fine — which is why only macOS broke.)
#   -s <libtcc dir>      : point dylibbundler at libtcc.dylib's build dir so it
#                          gets bundled + install-name-fixed — the fix.
#   -s /opt/homebrew/lib : insurance for the Homebrew deps.
#   </dev/null + hard timeout (CI, where gtimeout/timeout exists): dylibbundler
#                          can never block the pipeline again; a still-unresolved
#                          dep fails fast naming the missing lib instead of
#                          hanging. Local runs stay interactive for a human.
DYLIBBUNDLER_ARGS=(-of -b -cd -ns -s /opt/homebrew/lib)
TCC_DYLIB="$(find "$BUILD_DIR" -name 'libtcc.dylib' -type f 2>/dev/null | head -1)"
[[ -n "$TCC_DYLIB" ]] && DYLIBBUNDLER_ARGS+=(-s "$(dirname "$TCC_DYLIB")")
DYLIBBUNDLER_ARGS+=(
    -x "$APP/Contents/MacOS/$APP_NAME"
    -x "$APP/Contents/MacOS/torch"
    -d "$APP/Contents/Frameworks/"
    -p "@executable_path/../Frameworks/")
DB_TIMEOUT="$(command -v gtimeout || command -v timeout || true)"
if [[ -n "$DB_TIMEOUT" ]]; then
    "$DB_TIMEOUT" --signal=KILL 300 dylibbundler "${DYLIBBUNDLER_ARGS[@]}" </dev/null \
        || fail "dylibbundler failed/timed out — an unresolved dependency it tried to prompt for; see its output above."
else
    dylibbundler "${DYLIBBUNDLER_ARGS[@]}"
fi

# Homebrew's current `sdl2` formula is an alias for `sdl2-compat`, an SDL2 ABI
# wrapper that loads SDL3 with dlopen(). Because that SDL3 dependency is not a
# Mach-O LC_LOAD_DYLIB entry, dylibbundler cannot discover it from `otool -L`.
# Ship SDL3 next to SDL2 when the bundled SDL2 dylib is the compatibility layer.
SDL2_BUNDLED="$APP/Contents/Frameworks/libSDL2-2.0.0.dylib"
needs_sdl3=0
if [[ -f "$SDL2_BUNDLED" ]] && grep -qiE 'sdl2-compat|SDL2COMPAT|Failed loading SDL3' < <(strings "$SDL2_BUNDLED"); then
    needs_sdl3=1
fi
truthy "${SSB64_FORCE_BUNDLE_SDL3:-}" && needs_sdl3=1
if [[ "$needs_sdl3" -eq 1 ]]; then
    step "Bundling SDL3 for Homebrew sdl2-compat"
    SDL3_PREFIX=""
    if command -v brew >/dev/null 2>&1; then
        SDL3_PREFIX="$(brew --prefix sdl3 2>/dev/null || true)"
    fi
    SDL3_DYLIB=""
    for candidate in \
        "${SDL3_PREFIX:+$SDL3_PREFIX/lib/libSDL3.dylib}" \
        "${SDL3_PREFIX:+$SDL3_PREFIX/lib/libSDL3.0.dylib}" \
        /opt/homebrew/opt/sdl3/lib/libSDL3.dylib \
        /opt/homebrew/opt/sdl3/lib/libSDL3.0.dylib \
        /usr/local/opt/sdl3/lib/libSDL3.dylib \
        /usr/local/opt/sdl3/lib/libSDL3.0.dylib
    do
        [[ -n "$candidate" && -f "$candidate" ]] || continue
        SDL3_DYLIB="$candidate"
        break
    done
    [[ -n "$SDL3_DYLIB" ]] || fail "bundled SDL2 needs SDL3, but libSDL3.dylib was not found"
    SDL3_REAL="$(python3 -c 'import os, sys; print(os.path.realpath(sys.argv[1]))' "$SDL3_DYLIB")"
    [[ -f "$SDL3_REAL" ]] || SDL3_REAL="$SDL3_DYLIB"
    cp -f "$SDL3_REAL" "$APP/Contents/Frameworks/libSDL3.0.dylib"
    chmod +w "$APP/Contents/Frameworks/libSDL3.0.dylib"
    install_name_tool -id "@loader_path/libSDL3.0.dylib" "$APP/Contents/Frameworks/libSDL3.0.dylib"
    ln -snf libSDL3.0.dylib "$APP/Contents/Frameworks/libSDL3.dylib"
fi

# Sanity-check: no /opt/homebrew or /usr/local references should remain in
# the binaries' load commands. Catches the case where dylibbundler missed a
# transitive dep (rare, but worth failing loudly here rather than letting
# the .app ship and hit the user with a runtime "Library not loaded:").
for bin in "$APP/Contents/MacOS/$APP_NAME" "$APP/Contents/MacOS/torch"; do
    if otool -L "$bin" | grep -qE '(/opt/homebrew|/usr/local/Cellar)'; then
        echo "  remaining unbundled refs in $bin:" >&2
        otool -L "$bin" | grep -E '(/opt/homebrew|/usr/local/Cellar)' >&2
        fail "dylibbundler left non-portable load commands in $(basename "$bin")"
    fi
done

# ── 4a. De-duplicate LC_RPATH ──
# dyld aborts the process at load with "duplicate LC_RPATH '<path>'" if a
# Mach-O carries the same rpath twice. The libultraship/CMake macOS link
# already emits `@executable_path/../Frameworks/`, and dylibbundler's
# rewrite pass adds the same path again → the .app crashes immediately on
# launch (no main(), just the dyld abort — verified on the JP bundle).
# Collapse any duplicates to a single entry. install_name_tool
# -delete_rpath removes one occurrence per call; loop until one remains.
# Done before codesign so the subsequent `codesign --force` re-seals the
# final load commands. Version-independent (affects US and JP equally).
step "De-duplicating LC_RPATH load commands"
for bin in "$APP/Contents/MacOS/$APP_NAME" "$APP/Contents/MacOS/torch"; do
    rp='@executable_path/../Frameworks/'
    count_rpath() {
        otool -l "$1" | awk -v p="$rp" '
            /^[[:space:]]*cmd LC_RPATH$/ { in_rp=1; next }
            in_rp && /^[[:space:]]*path / { if ($2 == p) c++; in_rp=0 }
            END { print c+0 }'
    }
    n="$(count_rpath "$bin")"
    while [[ "${n:-0}" -gt 1 ]]; do
        install_name_tool -delete_rpath "$rp" "$bin" 2>/dev/null || break
        n=$((n - 1))
        printf '  %s: removed a duplicate LC_RPATH (%d left)\n' "$(basename "$bin")" "$n"
    done
done

# ── 4b. Sign the bundle as a unit ──
# Sign after dylibbundler and install_name_tool because both mutate Mach-O
# load commands. Developer ID CI uses hardened runtime + secure timestamp
# for notarization; local builds keep an ad-hoc fallback so packaging stays
# usable without Apple credentials. The main app signature always carries
# the JIT / unsigned-executable-memory entitlements required by the mod
# hook backend on Apple Silicon.
SIGNING_IDENTITY="${MACOS_CODESIGN_IDENTITY:-}"
SIGNING_MODE="developer-id"
if [[ -z "$SIGNING_IDENTITY" ]]; then
    SIGNING_IDENTITY="-"
    SIGNING_MODE="ad-hoc"
fi
if [[ "$SIGNING_MODE" == "ad-hoc" ]] && truthy "${SSB64_REQUIRE_MACOS_SIGNING:-}"; then
    fail "SSB64_REQUIRE_MACOS_SIGNING=1 but MACOS_CODESIGN_IDENTITY is empty"
fi

CODESIGN_KEYCHAIN_ARGS=()
if [[ -n "${MACOS_CODESIGN_KEYCHAIN:-}" ]]; then
    CODESIGN_KEYCHAIN_ARGS=(--keychain "$MACOS_CODESIGN_KEYCHAIN")
fi
CODESIGN_RUNTIME_ARGS=()
CODESIGN_TIMESTAMP_ARGS=(--timestamp=none)
if [[ "$SIGNING_MODE" == "developer-id" ]]; then
    CODESIGN_RUNTIME_ARGS=(--options runtime)
    CODESIGN_TIMESTAMP_ARGS=(--timestamp)
fi
CODESIGN_BASE_ARGS=(
    --force
    --sign "$SIGNING_IDENTITY"
    "${CODESIGN_KEYCHAIN_ARGS[@]+"${CODESIGN_KEYCHAIN_ARGS[@]}"}"
    "${CODESIGN_RUNTIME_ARGS[@]+"${CODESIGN_RUNTIME_ARGS[@]}"}"
    "${CODESIGN_TIMESTAMP_ARGS[@]}"
)

is_macho_file() {
    file "$1" | grep -q 'Mach-O'
}

step "$([[ "$SIGNING_MODE" == "developer-id" ]] && echo "Developer ID-signing bundle" || echo "Ad-hoc signing bundle")"
while IFS= read -r -d '' path; do
    [[ "$path" == "$APP/Contents/MacOS/$APP_NAME" ]] && continue
    if is_macho_file "$path"; then
        printf '  signing nested code: %s\n' "${path#$APP/}"
        codesign "${CODESIGN_BASE_ARGS[@]}" "$path"
    fi
done < <(find "$APP" -type f -print0)

codesign "${CODESIGN_BASE_ARGS[@]}" \
    --entitlements "$ROOT/cmake/macos_entitlements.plist" \
    "$APP"
codesign --verify --deep --strict --verbose=2 "$APP" \
    || fail "codesign verify failed on $APP"
if ! codesign -d --entitlements :- "$APP/Contents/MacOS/$APP_NAME" 2>/dev/null \
    | grep -q 'com.apple.security.cs.allow-jit'; then
    fail "signed app is missing the macOS mod-loader entitlements"
fi
echo "  signature verified ($SIGNING_MODE)"

# ── 5. Build a drag-and-drop DMG ──
# Two paths, because the pretty one isn't safe everywhere:
#
#   • Styled (create-dmg) — a Finder-arranged window with a background image,
#     the app icon, and an Applications drop-link. create-dmg drives Finder
#     through AppleScript (osascript) to place those. That AppleScript step
#     reliably HANGS on headless GitHub-hosted macOS runners: observed
#     stalling a release for 30+ min (toward the 6h job ceiling) while every
#     other platform finished in minutes — the build itself is only ~4 min,
#     so the time was entirely in this Finder step. Used only for
#     local/interactive builds, and even then capped + fallback-guarded.
#
#   • Plain (hdiutil) — a compressed image holding the .app plus an
#     /Applications symlink, no background art, no Finder automation. Builds
#     in seconds and cannot hang. This is the CI-safe fallback.
#
# Path selection:
#   DMG_PLAIN=1           force plain   (overrides styled unless styled is required)
#   DMG_STYLED=1          force styled  (attempt it even under CI; still falls back)
#   DMG_REQUIRE_STYLED=1  fail if the styled path fails or is unavailable
#   otherwise             plain when running under CI (GitHub Actions sets CI=true)
#                         or when create-dmg is absent; styled on a dev machine.
# The styled path falls back to plain on any failure or timeout, so this
# script always emits a working DMG unless DMG_REQUIRE_STYLED=1 is set.
DMG_VOLNAME="$APP_NAME"
DMG_BG_SRC="$ROOT/assets/macos_dmg_banner.png"
DMG_BG_LONG=600
DMG="$DIST_DIR/$APP_NAME.dmg"
DMG_STAGE="$DIST_DIR/dmg-stage"
DMG_BG_DIR="$DIST_DIR/dmg-bg"

# Unmount any stale volume of our name before hdiutil touches it — a leftover
# mount makes hdiutil's auto-mount step fail.
detach_dmg_volume() {
    if [[ -d "/Volumes/$DMG_VOLNAME" ]]; then
        hdiutil detach "/Volumes/$DMG_VOLNAME" -force >/dev/null 2>&1 || true
    fi
}

# Plain compressed DMG — no Finder, no AppleScript, no hang. The user drags
# the .app onto the bundled Applications symlink; functionally identical to
# install, it just lacks the background banner / icon layout.
build_plain_dmg() {
    rm -f "$DMG"
    ln -snf /Applications "$DMG_STAGE/Applications"
    detach_dmg_volume
    hdiutil create \
        -volname "$DMG_VOLNAME" \
        -srcfolder "$DMG_STAGE" \
        -fs HFS+ \
        -format UDZO \
        -ov -quiet \
        "$DMG"
}

# Styled DMG via create-dmg. Returns non-zero on failure/timeout so the caller
# can fall back. Capped with timeout/gtimeout when one is on PATH (macOS ships
# neither by default, so a local dev relies on Ctrl-C; CI never reaches this
# path). The `${cap[@]+...}` guard keeps the empty-array expansion safe under
# `set -u` on the bash 3.2 macOS still ships as /bin/bash.
build_styled_dmg() {
    sips -Z $((DMG_BG_LONG * 2)) "$DMG_BG_SRC" --out "$DMG_BG_DIR/bg@2x.png" >/dev/null
    sips -Z $DMG_BG_LONG          "$DMG_BG_SRC" --out "$DMG_BG_DIR/bg.png"    >/dev/null
    local w h
    w=$(sips -g pixelWidth  "$DMG_BG_DIR/bg.png" | awk '/pixelWidth/  {print $2}')
    h=$(sips -g pixelHeight "$DMG_BG_DIR/bg.png" | awk '/pixelHeight/ {print $2}')
    # Multi-rep TIFF: 1x at 72 dpi + 2x at 144 dpi so Finder picks the right
    # rep on retina vs non-retina displays.
    tiffutil -cathidpicheck "$DMG_BG_DIR/bg.png" "$DMG_BG_DIR/bg@2x.png" \
        -out "$DMG_BG_DIR/background.tiff" >/dev/null

    detach_dmg_volume
    rm -f "$DMG"

    local cap=() tmo
    tmo="$(command -v gtimeout || command -v timeout || true)"
    [[ -n "$tmo" ]] && cap=("$tmo" "${DMG_TIMEOUT:-300}")

    "${cap[@]+"${cap[@]}"}" create-dmg \
        --volname "$DMG_VOLNAME" \
        --background "$DMG_BG_DIR/background.tiff" \
        --window-pos 200 120 \
        --window-size "$w" "$h" \
        --icon-size 128 \
        --icon "$APP_NAME.app" $((w / 4))     $((h * 3 / 5)) \
        --app-drop-link        $((w * 3 / 4)) $((h * 3 / 5)) \
        --hide-extension "$APP_NAME.app" \
        --hdiutil-quiet \
        --no-internet-enable \
        "$DMG" \
        "$DMG_STAGE"
}

step "Building DMG"
rm -rf "$DMG_STAGE" "$DMG_BG_DIR" "$DMG"
mkdir -p "$DMG_STAGE" "$DMG_BG_DIR"
# Stage only the .app. The styled path injects Applications via
# --app-drop-link; the plain path adds its own Applications symlink.
cp -R "$APP" "$DMG_STAGE/"

want_styled=1
require_styled=0
truthy "${DMG_REQUIRE_STYLED:-}" && require_styled=1
if [[ -n "${DMG_PLAIN:-}" && "$require_styled" -eq 1 ]]; then
    fail "DMG_PLAIN and DMG_REQUIRE_STYLED cannot both be set"
fi
if [[ -n "${DMG_PLAIN:-}" ]]; then
    want_styled=0
elif [[ -n "${DMG_STYLED:-}" ]]; then
    want_styled=1
elif [[ -n "${CI:-}" ]]; then
    want_styled=0   # create-dmg's Finder/AppleScript step hangs on hosted CI
fi
if ! command -v create-dmg >/dev/null; then
    [[ "$require_styled" -eq 0 ]] || fail "DMG_REQUIRE_STYLED=1 but create-dmg is not in PATH"
    want_styled=0
fi

if [[ "$want_styled" -eq 1 ]]; then
    step "Building styled DMG (create-dmg)"
    if build_styled_dmg && [[ -f "$DMG" ]]; then
        echo "  styled DMG built"
    else
        [[ "$require_styled" -eq 0 ]] || fail "create-dmg failed or timed out and DMG_REQUIRE_STYLED=1"
        echo "  create-dmg failed or timed out — falling back to a plain hdiutil DMG" >&2
        detach_dmg_volume
        # create-dmg leaves a writable scratch image (rw.*.dmg) on failure.
        rm -f "$DIST_DIR"/rw.*.dmg "$DMG" 2>/dev/null || true
        step "Building plain DMG (hdiutil fallback)"
        build_plain_dmg
    fi
else
    step "Building plain DMG (hdiutil — CI-safe, no Finder)"
    build_plain_dmg
fi

rm -rf "$DMG_STAGE" "$DMG_BG_DIR"
[[ -f "$DMG" ]] || fail "DMG was not created"

if [[ "$SIGNING_MODE" == "developer-id" ]]; then
    step "Signing DMG"
    codesign \
        --force \
        --sign "$SIGNING_IDENTITY" \
        "${CODESIGN_KEYCHAIN_ARGS[@]+"${CODESIGN_KEYCHAIN_ARGS[@]}"}" \
        --timestamp \
        "$DMG"
    codesign --verify --verbose=2 "$DMG" \
        || fail "codesign verify failed on $DMG"
fi

WANT_NOTARIZE=0
if truthy "${SSB64_NOTARIZE:-}"; then
    WANT_NOTARIZE=1
elif [[ -n "${MACOS_NOTARY_APPLE_ID:-}${MACOS_NOTARY_TEAM_ID:-}${MACOS_NOTARY_PASSWORD:-}" ]]; then
    WANT_NOTARIZE=1
fi
if [[ "$WANT_NOTARIZE" -eq 1 ]]; then
    [[ "$SIGNING_MODE" == "developer-id" ]] \
        || fail "notarization requires Developer ID signing"
    missing=()
    for name in MACOS_NOTARY_APPLE_ID MACOS_NOTARY_TEAM_ID MACOS_NOTARY_PASSWORD; do
        if [[ -z "${!name:-}" ]]; then
            missing+=("$name")
        fi
    done
    if [[ "${#missing[@]}" -ne 0 ]]; then
        fail "missing macOS notarization secret(s): ${missing[*]}"
    fi

    step "Notarizing DMG"
    NOTARY_AUTH_ARGS=(
        --apple-id "$MACOS_NOTARY_APPLE_ID"
        --team-id "$MACOS_NOTARY_TEAM_ID"
        --password "$MACOS_NOTARY_PASSWORD"
    )
    NOTARY_WAIT_TIMEOUT="${MACOS_NOTARY_WAIT_TIMEOUT:-10m}"
    NOTARY_WAIT_ATTEMPTS="${MACOS_NOTARY_WAIT_ATTEMPTS:-3}"
    NOTARY_WAIT_RETRY_DELAY="${MACOS_NOTARY_WAIT_RETRY_DELAY:-30}"

    NOTARY_SUBMIT_JSON="$(xcrun notarytool submit "$DMG" \
        "${NOTARY_AUTH_ARGS[@]}" \
        --output-format json \
        --no-progress)"
    printf '%s\n' "$NOTARY_SUBMIT_JSON"
    NOTARY_SUBMISSION_ID="$(printf '%s' "$NOTARY_SUBMIT_JSON" | json_get id)"
    [[ -n "$NOTARY_SUBMISSION_ID" ]] \
        || fail "notarytool submit did not return a submission id"

    NOTARY_ACCEPTED=0
    attempt=1
    while [[ "$attempt" -le "$NOTARY_WAIT_ATTEMPTS" ]]; do
        printf '  waiting for notarization submission %s (attempt %d/%d, timeout %s)\n' \
            "$NOTARY_SUBMISSION_ID" "$attempt" "$NOTARY_WAIT_ATTEMPTS" "$NOTARY_WAIT_TIMEOUT"
        set +e
        NOTARY_WAIT_JSON="$(xcrun notarytool wait "$NOTARY_SUBMISSION_ID" \
            "${NOTARY_AUTH_ARGS[@]}" \
            --timeout "$NOTARY_WAIT_TIMEOUT" \
            --output-format json \
            --no-progress 2>&1)"
        wait_rc=$?
        set -e
        printf '%s\n' "$NOTARY_WAIT_JSON"

        if [[ "$wait_rc" -eq 0 ]]; then
            NOTARY_STATUS="$(printf '%s' "$NOTARY_WAIT_JSON" | json_get status)"
            if [[ "$NOTARY_STATUS" == "Accepted" ]]; then
                NOTARY_ACCEPTED=1
                break
            fi
            if [[ "$NOTARY_STATUS" == "Invalid" || "$NOTARY_STATUS" == "Rejected" ]]; then
                xcrun notarytool log "$NOTARY_SUBMISSION_ID" "${NOTARY_AUTH_ARGS[@]}" || true
                fail "notarization failed with status: $NOTARY_STATUS"
            fi
        else
            echo "  notarytool wait failed (exit $wait_rc); checking current submission status" >&2
            set +e
            NOTARY_INFO_JSON="$(xcrun notarytool info "$NOTARY_SUBMISSION_ID" \
                "${NOTARY_AUTH_ARGS[@]}" \
                --output-format json 2>&1)"
            info_rc=$?
            set -e
            printf '%s\n' "$NOTARY_INFO_JSON"
            if [[ "$info_rc" -eq 0 ]]; then
                NOTARY_STATUS="$(printf '%s' "$NOTARY_INFO_JSON" | json_get status)"
                if [[ "$NOTARY_STATUS" == "Accepted" ]]; then
                    NOTARY_ACCEPTED=1
                    break
                fi
                if [[ "$NOTARY_STATUS" == "Invalid" || "$NOTARY_STATUS" == "Rejected" ]]; then
                    xcrun notarytool log "$NOTARY_SUBMISSION_ID" "${NOTARY_AUTH_ARGS[@]}" || true
                    fail "notarization failed with status: $NOTARY_STATUS"
                fi
            fi
        fi

        if [[ "$attempt" -eq "$NOTARY_WAIT_ATTEMPTS" ]]; then
            break
        fi
        printf '  notarization is not complete yet; retrying status wait in %ss\n' \
            "$NOTARY_WAIT_RETRY_DELAY"
        sleep "$NOTARY_WAIT_RETRY_DELAY"
        attempt=$((attempt + 1))
    done

    [[ "$NOTARY_ACCEPTED" -eq 1 ]] \
        || fail "notarization did not reach Accepted for submission $NOTARY_SUBMISSION_ID"

    step "Stapling notarization ticket"
    xcrun stapler staple "$DMG"
    xcrun stapler validate "$DMG"
    spctl --assess --type open --context context:primary-signature --verbose=4 "$DMG" \
        || fail "spctl assessment failed for notarized DMG"
fi

# ── 6. Report ──
APP_KB=$(du -sk "$APP" | awk '{print $1}')
DMG_KB=$(du -sk "$DMG" | awk '{print $1}')
printf '\n\033[32m✓ Bundle: %s (%s KB)\033[0m\n' "$APP" "$APP_KB"
printf '\033[32m✓ DMG:    %s (%s KB)\033[0m\n' "$DMG" "$DMG_KB"
printf '   To run from the bundle:        open "%s"\n' "$APP"
printf '   To install from the DMG:       open "%s"  (then drag to Applications)\n' "$DMG"
printf '   App-data: ~/Library/Application Support/%s/\n' "$APP_NAME"
printf '   First launch will prompt for your ROM via the ImGui wizard.\n'
