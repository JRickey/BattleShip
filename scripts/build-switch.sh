#!/usr/bin/env bash
#
# build-switch.sh — Full SSB64 Switch .nro build pipeline.
#
# Usage:
#   ./scripts/build-switch.sh                  # build Release
#   ./scripts/build-switch.sh --debug           # build Debug
#   ./scripts/build-switch.sh --skip-extract    # skip asset extraction
#
# Prerequisites:
#   - devkitPro toolchain (dkp-pacman -S switch-dev)
#   - switch-sdl2, switch-glad, switch-tinyxml2, switch-libzip,
#     switch-spdlog, switch-nlohmann-json
#   - baserom.us.z64 at project root (for asset extraction)
#   - Native toolchain with CMake (for Torch extraction)
#
# This script:
#   1. Builds Torch natively (if not skipped) to extract .o2r assets
#   2. Cross-compiles BattleShip for Switch
#   3. Bundles .nro + .o2r + support files into switch_sd/ for SD card
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build-switch"

# Auto-detect devkitPro install path
if [[ -n "${DEVKITPRO:-}" ]]; then
    DKP="${DEVKITPRO}"
elif [[ -d "/opt/devkitpro" ]]; then
    DKP="/opt/devkitpro"
else
    echo "ERROR: devkitPro not found. Set DEVKITPRO env var or install to /opt/devkitpro."
    echo "  Install: https://devkitpro.org/wiki/Getting_Started"
    exit 1
fi
export DEVKITPRO="$DKP"

DKP_TOOLCHAIN="${DKP}/cmake/Switch.cmake"
if [[ ! -f "$DKP_TOOLCHAIN" ]]; then
    echo "ERROR: Switch toolchain not found at $DKP_TOOLCHAIN"
    echo "  Install: dkp-pacman -S switch-dev"
    exit 1
fi

CONFIG="Release"
SKIP_EXTRACT=0
NPROC="$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug)     CONFIG="Debug"; shift ;;
        --skip-extract) SKIP_EXTRACT=1; shift ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

echo "=== SSB64 Switch Build ==="
echo "Config:         $CONFIG"
echo "Build dir:      $BUILD_DIR"
echo "Skip extract:   $SKIP_EXTRACT"
echo ""

# ------------------------------------------------------------------
# Step 0: Check for devkitPro toolchain
# ------------------------------------------------------------------

# Add devkitPro to PATH
export PATH="${DKP}/devkitA64/bin:${PATH}"

if ! command -v aarch64-none-elf-g++ &>/dev/null; then
    echo "ERROR: aarch64-none-elf-g++ not found."
    echo "  Install devkitPro: dkp-pacman -S switch-dev"
    exit 1
fi

# ------------------------------------------------------------------
# Step 1: Build Torch natively + extract assets (skip if --skip-extract)
# ------------------------------------------------------------------
if [[ "$SKIP_EXTRACT" -eq 0 ]]; then
    echo "--- Step 1: Extracting assets ---"
    NATIVE_BUILD="${PROJECT_DIR}/build-native"

    cmake -B "$NATIVE_BUILD" -S "$PROJECT_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DUSE_STANDALONE=ON

    # Keep this single-threaded to avoid nested FetchContent sub-build
    # failures on macOS (e.g. tinyxml2 "resource temporarily unavailable").
    cmake --build "$NATIVE_BUILD" --target TorchExternal -- -j 1

    TORCH=""
    for candidate in "$NATIVE_BUILD/torch-install/bin/torch" \
                     "$NATIVE_BUILD/TorchExternal/src/TorchExternal-build/torch"; do
        if [[ -x "$candidate" ]]; then
            TORCH="$candidate"
            break
        fi
    done

    if [[ -z "$TORCH" ]]; then
        echo "ERROR: Torch binary not found after build."
        exit 1
    fi
    echo "Torch: $TORCH"

    cd "$PROJECT_DIR"
    "$TORCH" o2r baserom.us.z64
    echo "Assets extracted."

    # Clean up native build dir (optional, saves space)
    rm -rf "$NATIVE_BUILD"
else
    echo "--- Step 1: Skipping asset extraction ---"
fi

# ------------------------------------------------------------------
# Step 2: Cross-compile for Switch
# ------------------------------------------------------------------
echo ""
echo "--- Step 2: Cross-compiling BattleShip.nro ---"

cmake -B "$BUILD_DIR" -S "$PROJECT_DIR" \
    -DCMAKE_BUILD_TYPE="$CONFIG" \
    -DCMAKE_TOOLCHAIN_FILE="$DKP_TOOLCHAIN"

cmake --build "$BUILD_DIR" -- -j "$NPROC"

NRO="$BUILD_DIR/BattleShip.nro"
if [[ ! -f "$NRO" ]]; then
    echo "ERROR: $NRO not found after build."
    exit 1
fi
echo "Built: $NRO"

# ------------------------------------------------------------------
# Step 3: Bundle for SD card
# ------------------------------------------------------------------
echo ""
echo "--- Step 3: Bundling SD card files ---"

cmake --build "$BUILD_DIR" --target BundleSwitch -- -j "$NPROC"

SD_DIR="$BUILD_DIR/switch_sd/switch/BattleShip"
echo ""
echo "=== Build Complete ==="
echo "Copy the contents of:"
echo "  $SD_DIR"
echo "to the root of your Switch SD card."
echo ""
echo "Game path: sdmc:/switch/BattleShip/BattleShip.nro"
ls -la "$SD_DIR"
