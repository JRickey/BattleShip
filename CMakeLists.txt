cmake_minimum_required(VERSION 3.24.0 FATAL_ERROR)

project(ssb64 LANGUAGES C CXX)

# Android needs ASM for the aarch64 coroutine context-switch
# (port/coroutine_aarch64.S). Enable it before any add_library/add_executable
# call sees an .S source. ASM is preprocessed by clang on the way through.
if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    enable_language(ASM)
endif()

# Mod system master switch (TinyCC source scripting + funchook hook backend +
# port/mods). DISABLE_SCRIPTING gates it everywhere — funchook FetchContent,
# port/mods sources, the no-inline linkage tweaks, the post-build .tcc/ staging,
# and the in-engine call sites (#ifndef DISABLE_SCRIPTING). Forced ON for
# Android: funchook/TinyCC have no Android target and the loader is desktop-only.
# Set BEFORE add_subdirectory(libultraship) so the engine half sees it too.
if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(DISABLE_SCRIPTING ON CACHE BOOL "Disable the mod system (scripting + hooks)" FORCE)
endif()

if (DISABLE_SCRIPTING)
    # Define for every superproject target (ssb64_game + the exe + port glue)
    # so the #ifndef DISABLE_SCRIPTING guards in port/*.cpp engage. libultraship
    # additionally defines it PUBLIC from its own CMake when the var is set.
    add_compile_definitions(DISABLE_SCRIPTING)
endif()

include(ExternalProject)
include(FetchContent) # pull discord-rpc from GitHub (desktop only)

# Version string for the built-in updater. String-compared exactly against
# the name returned by GitHub's /tags API, so it must match the release tag
# character-for-character (suffix-and-all). Resolution order:
#   1. -DBATTLESHIP_VERSION=... passed by the caller (release CI sets this
#      to $GITHUB_REF_NAME).
#   2. `git describe --tags --abbrev=0` for local builds at a tagged commit.
#   3. "dev" fallback for source tarballs / shallow CI checkouts — those
#      builds always show "update available", which is the right signal.
if(NOT BATTLESHIP_VERSION)
    find_package(Git QUIET)
    if(Git_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE BATTLESHIP_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
    endif()
endif()
if(NOT BATTLESHIP_VERSION)
    set(BATTLESHIP_VERSION "dev")
endif()
message(STATUS "BattleShip updater version: ${BATTLESHIP_VERSION}")

# Pulled-in BEFORE the libultraship subdirectory so its
# find_package(tinyxml2) sees our pkg-config-fallback shim. Required
# for Ubuntu 22.04 jammy, where libtinyxml2-dev ships no CMake config.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} -fobjc-arc")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

# Disable MSVC's vectorized STL helpers (added in 14.40 / VS 17.10).
# Some VS installs ship the headers without the matching vcruntime
# symbols, which produces unresolved external errors at link
# (__std_find_first_of_trivial_pos_1 etc). The fallback non-vectorized
# variants are functionally equivalent and always work.
if(MSVC)
    add_compile_definitions(_USE_STD_VECTOR_ALGORITHMS=0)
endif()

################################################################################
# Options
################################################################################

option(NON_MATCHING "Build without matching decomp constraints" ON)
option(NON_EQUIVALENT "Build without equivalent decomp constraints" ON)
option(USE_AUTO_VCPKG "Use automated vcpkg for dependency management" ON)
option(SSB64_STAGE_CYCLE_DEMO "Boot directly into attract-demo stage cycle for rendering regression testing" OFF)

# ROM version to build. The decomp game code is region-conditionally
# compiled (#if REGION_US / REGION_JP), so US and JP are separate builds —
# use a dedicated build dir per version. The user still supplies their own
# ROM (baserom.<version>.{z64,n64,v64}); no assets are shipped.
set(SSB64_VERSION "us" CACHE STRING "ROM version to build: us or jp")
set_property(CACHE SSB64_VERSION PROPERTY STRINGS us jp)
if(NOT SSB64_VERSION MATCHES "^(us|jp)$")
    message(FATAL_ERROR "SSB64_VERSION must be 'us' or 'jp' (got '${SSB64_VERSION}')")
endif()
# Android ships US-only. The Android build pipeline (Gradle asset staging,
# touch overlay, JNI bridges) has only ever been tested against the US
# decomp; bifurcating the APK / app-data dir per region the way desktop
# does (PR #191) would also need a JP-specific BootActivity/AssetExtractor.
# Until someone signs up for the JP Android effort, force US.
if (CMAKE_SYSTEM_NAME STREQUAL "Android" AND NOT SSB64_VERSION STREQUAL "us")
    message(FATAL_ERROR
        "Android build is US-only (got SSB64_VERSION=${SSB64_VERSION}). "
        "Drop -DSSB64_VERSION=jp from the Gradle externalNativeBuild args.")
endif()
string(TOUPPER "${SSB64_VERSION}" SSB64_VERSION_UC)
message(STATUS "SSB64 ROM version: ${SSB64_VERSION}")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

################################################################################
# libultraship configuration (set before add_subdirectory)
################################################################################

# O2R (ZIP) only, no MPQ/StormLib
set(INCLUDE_MPQ_SUPPORT OFF CACHE BOOL "" FORCE)
set(GBI_UCODE "F3DEX_GBI_2" CACHE STRING "" FORCE)

# Android needs GLES3 instead of desktop GL + GLEW. Pin USE_OPENGLES ON
# before the libultraship subdirectory so its src/CMakeLists.txt picks the
# GLESv3 branch and the ENABLE_OPENGL/USE_OPENGLES compile defs are set.
if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(USE_OPENGLES ON CACHE BOOL "" FORCE)
endif()

# Build Torch as the original standalone sidecar executable on desktop.
# Android ships Torch as a SHARED library (libtorch_runner.so) inside the
# APK so the first-run UI can extract the user's ROM on-device via JNI —
# see the torch_runner target at the bottom of this file.
if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(USE_STANDALONE OFF CACHE BOOL "" FORCE)
else()
    set(USE_STANDALONE ON CACHE BOOL "" FORCE)
endif()
set(BUILD_STORMLIB OFF CACHE BOOL "" FORCE)
set(BUILD_SM64 OFF CACHE BOOL "" FORCE)
set(BUILD_MK64 OFF CACHE BOOL "" FORCE)
set(BUILD_SF64 OFF CACHE BOOL "" FORCE)
set(BUILD_PM64 OFF CACHE BOOL "" FORCE)
set(BUILD_FZERO OFF CACHE BOOL "" FORCE)
set(BUILD_MARIO_ARTIST OFF CACHE BOOL "" FORCE)
set(BUILD_NAUDIO ON CACHE BOOL "" FORCE)
set(BUILD_SSB64 ON CACHE BOOL "" FORCE)

################################################################################
# Subdirectories: libultraship
################################################################################

add_subdirectory(libultraship ${CMAKE_BINARY_DIR}/libultraship)

################################################################################
# funchook - cross-platform runtime function-detouring lib used by the mod
# loader. Replaces MinHook (which is x86/x64-Windows-only) so the hook system
# works on every release target: Windows x64, Linux x64/arm64, and
# macOS arm64 (Apple Silicon). funchook does the prologue rewrite +
# trampoline emit; HookManager layers the per-target hot-reload chain on top.
#
# funchook pulls its disassembler backend itself (distorm on x86, capstone
# on arm64). Its hardcoded capstone (aquynh/capstone @ 4.0.2) is patched up
# to capstone-engine/capstone @ 5.0.1 via the PATCH_COMMAND below — 4.0.2's
# `cmake_policy(CMP0048 OLD)` is rejected outright by CMake >=4.
# Pinned to v1.1.3 (latest release; the project is archived but the
# instruction sets it rewrites are stable). GPLv2 + linking exception —
# the exception means it does NOT impose GPL on the rest of the codebase.
#
# Desktop-only: the mod system is excluded on Android, where DISABLE_SCRIPTING
# is forced ON, so funchook is not fetched there.
################################################################################
if (NOT DISABLE_SCRIPTING)
    include(FetchContent)
    set(FUNCHOOK_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(FUNCHOOK_BUILD_STATIC ON  CACHE BOOL "" FORCE)
    set(FUNCHOOK_BUILD_TESTS  OFF CACHE BOOL "" FORCE)
    set(FUNCHOOK_INSTALL      OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        funchook
        GIT_REPOSITORY https://github.com/kubo/funchook.git
        GIT_TAG        v1.1.3
        PATCH_COMMAND  ${CMAKE_COMMAND}
                       -DCAPSTONE_TMPL=cmake/capstone.cmake.in
                       -DFUNCHOOK_CML=CMakeLists.txt
                       -DFUNCHOOK_UNIX=src/funchook_unix.c
                       -P ${CMAKE_SOURCE_DIR}/cmake/funchook_capstone_bump.cmake
    )
    FetchContent_MakeAvailable(funchook)

    # Stale-cache guard. FetchContent runs PATCH_COMMAND only on the FIRST
    # populate of funchook-src; an already-populated _deps/funchook-src (e.g.
    # from an older/partial patch) is reused silently. If the macOS arm64
    # mach_vm_protect rewrite isn't wired into its call site, inline detours
    # fail at runtime with mprotect(RWX) EINVAL — and nothing catches it until
    # a mod tries to hook. Fail loudly at configure time instead. See
    # docs/bugs/funchook_macos_arm64_patch_anchor_2026-06-15.md.
    if (APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(_fh_unix "${funchook_SOURCE_DIR}/src/funchook_unix.c")
        if (EXISTS "${_fh_unix}")
            file(READ "${_fh_unix}" _fh_unix_contents)
            if (NOT _fh_unix_contents MATCHES "funchook_mac_arm64_set_rw\\(mstate->addr")
                message(FATAL_ERROR
                    "funchook_unix.c lacks the Apple Silicon mach_vm_protect patch "
                    "(stale FetchContent cache). Remove ${CMAKE_BINARY_DIR}/_deps/funchook-* "
                    "and re-configure. See docs/bugs/funchook_macos_arm64_patch_anchor_2026-06-15.md")
            endif()
            unset(_fh_unix_contents)
        endif()
        unset(_fh_unix)
    endif()
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    # Android: build Torch in-tree as a STATIC lib (USE_STANDALONE=OFF
    # was forced above). The libtorch_runner.so SHARED library at the
    # bottom of this file wraps it for JNI.
    add_subdirectory(torch ${CMAKE_BINARY_DIR}/torch)
else()
    # Desktop: build Torch as a host executable via ExternalProject so it
    # can run at our build time to extract ROM assets.
    ExternalProject_Add(TorchExternal
        PREFIX TorchExternal
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/torch
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/torch-install
            -DUSE_STANDALONE=ON
            -DBUILD_STORMLIB=OFF
            -DBUILD_SM64=OFF
            -DBUILD_MK64=OFF
            -DBUILD_SF64=OFF
            -DBUILD_PM64=OFF
            -DBUILD_FZERO=OFF
            -DBUILD_MARIO_ARTIST=OFF
            -DBUILD_NAUDIO=ON
            -DBUILD_SSB64=ON
    )

    ExternalProject_Get_Property(TorchExternal install_dir)
    if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
        # Torch is built via ExternalProject_Add and inherits the parent generator.
        # Multi-config generators (Visual Studio) place the binary under <CONFIG>/;
        # single-config generators (Ninja, NMake) drop it directly in the build dir.
        if(CMAKE_CONFIGURATION_TYPES)
            set(TORCH_EXECUTABLE ${install_dir}/src/TorchExternal-build/$<CONFIG>/torch.exe)
        else()
            set(TORCH_EXECUTABLE ${install_dir}/src/TorchExternal-build/torch.exe)
        endif()
        set(TORCH_SIDECAR_NAME torch.exe)
    else()
        set(TORCH_EXECUTABLE ${install_dir}/src/TorchExternal-build/torch)
        set(TORCH_SIDECAR_NAME torch)
    endif()
endif()

# Reloc YAML category files (same category set for every version; the
# per-version ROM offsets differ inside them). yamls/${SSB64_VERSION}/ is
# produced by tools/generate_yamls.py --version ${SSB64_VERSION}.
set(SSB64_RELOC_YAML_CATEGORIES
    animations bonus effects extern_data fighters_common fighters_main
    interface items menus misc_named movies scene stages submotions
    transitions
)
set(SSB64_RELOC_YAML_FILES "")
foreach(_cat ${SSB64_RELOC_YAML_CATEGORIES})
    list(APPEND SSB64_RELOC_YAML_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/yamls/${SSB64_VERSION}/reloc_${_cat}.yml)
endforeach()

set(SSB64_CREDITS_GENERATED_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/decomp/src/credits/staff.credits.encoded
    ${CMAKE_CURRENT_SOURCE_DIR}/decomp/src/credits/staff.credits.metadata
    ${CMAKE_CURRENT_SOURCE_DIR}/decomp/src/credits/titles.credits.encoded
    ${CMAKE_CURRENT_SOURCE_DIR}/decomp/src/credits/titles.credits.metadata
    ${CMAKE_CURRENT_SOURCE_DIR}/decomp/src/credits/info.credits.encoded
    ${CMAKE_CURRENT_SOURCE_DIR}/decomp/src/credits/info.credits.metadata
    ${CMAKE_CURRENT_SOURCE_DIR}/decomp/src/credits/companies.credits.encoded
    ${CMAKE_CURRENT_SOURCE_DIR}/decomp/src/credits/companies.credits.metadata
)

add_custom_target(RegenerateRelocYamls
    BYPRODUCTS ${SSB64_RELOC_YAML_FILES}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_yamls.py --version ${SSB64_VERSION}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_fighter_slices.py --version ${SSB64_VERSION}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Regenerating reloc YAML configs + deblob slice maps (${SSB64_VERSION})"
)

# One command per intent (docs/deblob.md): regenerate every deblob
# artifact — parent yamls, child slice maps, manifests, rebuild specs —
# for BOTH regions from their baseroms.
add_custom_target(DeblobRegen
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_yamls.py --version us
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_fighter_slices.py --version us
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_yamls.py --version jp
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_fighter_slices.py --version jp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Regenerating all deblob artifacts (both regions)"
)

add_custom_target(GenerateRelocArtifacts
    BYPRODUCTS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/reloc_data.${SSB64_VERSION}.h
        ${CMAKE_CURRENT_SOURCE_DIR}/port/resource/RelocFileTable.${SSB64_VERSION}.cpp
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_reloc_stubs.py --version ${SSB64_VERSION}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_reloc_table.py --version ${SSB64_VERSION}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating reloc headers and RelocFileTable (${SSB64_VERSION})"
)

add_custom_target(GenerateCreditsAssets
    BYPRODUCTS ${SSB64_CREDITS_GENERATED_FILES}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/creditsTextConverter.py staff.credits.${SSB64_VERSION}.txt
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/creditsTextConverter.py titles.credits.${SSB64_VERSION}.txt
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/creditsTextConverter.py -paragraphFont -multiline info.credits.${SSB64_VERSION}.txt
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/creditsTextConverter.py -paragraphFont companies.credits.${SSB64_VERSION}.txt
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/decomp/src/credits
    COMMENT "Encoding credits text assets (${SSB64_VERSION})"
)

add_custom_target(PrepareBuildInputs
    DEPENDS GenerateRelocArtifacts GenerateCreditsAssets
)

# ── Window icon embed (US/JP) ─────────────────────────────────────────────
# Linux SDL only renders an app icon on the WM if SDL_SetWindowIcon() is
# called at runtime — there's no .rc-equivalent that the WM reads at
# launch. The PNG bytes are baked into the binary via tools/embed_icon.py
# so port/port_window_icon.cpp doesn't have to chase the icon across
# build/dev-tree/AppImage layouts. Region-aware: US uses assets/icon.png,
# JP uses assets/icon-jp.png.
set(SSB64_ICON_PNG ${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.png)
if(SSB64_VERSION STREQUAL "jp")
    set(SSB64_ICON_PNG ${CMAKE_CURRENT_SOURCE_DIR}/assets/icon-jp.png)
endif()
set(SSB64_ICON_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(SSB64_ICON_GEN ${SSB64_ICON_GEN_DIR}/port_icon_data.h)
add_custom_command(
    OUTPUT ${SSB64_ICON_GEN}
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_icon.py
            ${SSB64_ICON_PNG} ${SSB64_ICON_GEN} kSSB64IconPng
    DEPENDS
        ${SSB64_ICON_PNG}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_icon.py
    COMMENT "Embedding window icon (${SSB64_VERSION}) -> ${SSB64_ICON_GEN}"
    VERBATIM
)

# ── CSS icon pipeline ─────────────────────────────────────────────────────
# Stage-specific assets (background wallpapers + CSS thumbnails) are now
# extracted from baserom.us.z64 at build time by tools/derive_stage_assets.py
# and loaded at runtime as PNG files. No Nintendo-derived bytes are baked
# into the binary.
#
# The arrow sprite remains baked (it is original port artwork).
set(SSB64_CSS_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Stage asset PNGs — extracted from baserom.us.z64 by derive_stage_assets.py.
# Desktop-only: Android packaging of these assets is a follow-up task.
# The add_custom_command that invokes derive_stage_assets.py is declared later
# (after SSB64_BASEROM is resolved) in the "Asset extraction targets" section.
# The output paths are set here so the POST_BUILD copy can reference them.
set(SSB64_CSS_STAGE_ASSET_DIR ${CMAKE_CURRENT_BINARY_DIR}/assets/css_icons)
set(SSB64_CSS_FD_BG_PNG      ${SSB64_CSS_STAGE_ASSET_DIR}/final_destination_background.png)
set(SSB64_CSS_FD_SMALL_PNG   ${SSB64_CSS_STAGE_ASSET_DIR}/final_destination_small.png)
set(SSB64_CSS_FD_NAME_PNG    ${SSB64_CSS_STAGE_ASSET_DIR}/final_destination_name.png)
set(SSB64_CSS_FD_EMBLEM_PNG  ${SSB64_CSS_STAGE_ASSET_DIR}/final_destination_emblem.png)
# Metal Cavern (nGRKindMetal) — no emblem PNG (mnMapsMakeEmblem skips port
# stages until the IA4 render path is debugged; matches FD).
set(SSB64_CSS_MC_BG_PNG      ${SSB64_CSS_STAGE_ASSET_DIR}/metal_cavern_background.png)
set(SSB64_CSS_MC_SMALL_PNG   ${SSB64_CSS_STAGE_ASSET_DIR}/metal_cavern_small.png)
set(SSB64_CSS_MC_NAME_PNG    ${SSB64_CSS_STAGE_ASSET_DIR}/metal_cavern_name.png)
# Battlefield (nGRKindZako) — same shape as Metal Cavern.
set(SSB64_CSS_BF_BG_PNG      ${SSB64_CSS_STAGE_ASSET_DIR}/battlefield_background.png)
set(SSB64_CSS_BF_SMALL_PNG   ${SSB64_CSS_STAGE_ASSET_DIR}/battlefield_small.png)
set(SSB64_CSS_BF_NAME_PNG    ${SSB64_CSS_STAGE_ASSET_DIR}/battlefield_name.png)

# Scroll arrow — generated procedurally with apex angle ~149°. Canvas width is
# padded to a multiple of 4 px inside the script so RGBA16 row stride stays
# 8-byte-aligned (W=6 visual → W=8 canvas, transparent padding centered).
# Two variants: right-pointing (default) and left-pointing (--mirror) so the
# second-page indicator doesn't depend on negative SObj scalex.
set(SSB64_CSS_ARROW_PNG       ${CMAKE_CURRENT_SOURCE_DIR}/port/assets/css_icons/arrow.png)
set(SSB64_CSS_ARROW_LEFT_PNG  ${CMAKE_CURRENT_SOURCE_DIR}/port/assets/css_icons/arrow_left.png)
set(SSB64_CSS_ARROW_H         ${SSB64_CSS_GEN_DIR}/arrow_data.h)
set(SSB64_CSS_ARROW_LEFT_H    ${SSB64_CSS_GEN_DIR}/arrow_left_data.h)

# Baked nameplate headers for the port-introduced stages. Rendered from text
# strings only (tools/render_nameplates.py — NO ROM data), so unlike the
# stage icons these are legal to compile into shipped binaries. They are the
# release fallback for portCSSGetStageNameSprite when no
# assets/css_icons/<name>_name.png override exists; keep the strings in sync
# with STAGES in tools/derive_stage_assets.py.
set(SSB64_CSS_NAMEPLATES
    "final_destination|FINAL DESTINATION|nameFinalDestination"
    "metal_cavern|METAL CAVERN|nameMetalCavern"
    "battlefield|BATTLEFIELD|nameBattlefield")
set(SSB64_CSS_NAMEPLATE_HEADERS "")
foreach(entry IN LISTS SSB64_CSS_NAMEPLATES)
    string(REPLACE "|" ";" fields "${entry}")
    list(GET fields 0 np_stem)
    list(GET fields 1 np_text)
    list(GET fields 2 np_symbol)
    set(np_png ${SSB64_CSS_GEN_DIR}/${np_stem}_name.png)
    set(np_h   ${SSB64_CSS_GEN_DIR}/${np_stem}_name_data.h)
    add_custom_command(
        OUTPUT ${np_png}
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/render_nameplates.py
                ${np_text}
                ${np_png}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/render_nameplates.py
        COMMENT "Rendering nameplate PNG (${np_text})"
        VERBATIM
    )
    add_custom_command(
        OUTPUT ${np_h}
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/png_to_c_array.py
                ${np_png}
                ${np_h}
                ${np_symbol}
        DEPENDS ${np_png}
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/png_to_c_array.py
        COMMENT "Baking nameplate -> ${np_h}"
        VERBATIM
    )
    list(APPEND SSB64_CSS_NAMEPLATE_HEADERS ${np_h})
endforeach()
add_custom_command(
    OUTPUT ${SSB64_CSS_ARROW_PNG}
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_arrow_svg.py
            ${SSB64_CSS_ARROW_PNG}
            6 44
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_arrow_svg.py
    COMMENT "Generating CSS scroll-arrow PNG (visual 6x44, canvas 8x44 padded)"
    VERBATIM
)
add_custom_command(
    OUTPUT ${SSB64_CSS_ARROW_LEFT_PNG}
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_arrow_svg.py
            ${SSB64_CSS_ARROW_LEFT_PNG}
            6 44 --mirror
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_arrow_svg.py
    COMMENT "Generating CSS scroll-arrow PNG (left-mirrored)"
    VERBATIM
)
add_custom_command(
    OUTPUT ${SSB64_CSS_ARROW_H}
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/png_to_c_array.py
            ${SSB64_CSS_ARROW_PNG}
            ${SSB64_CSS_ARROW_H}
            arrow
    DEPENDS
        ${SSB64_CSS_ARROW_PNG}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/png_to_c_array.py
    COMMENT "Baking scroll-arrow -> ${SSB64_CSS_ARROW_H}"
    VERBATIM
)
add_custom_command(
    OUTPUT ${SSB64_CSS_ARROW_LEFT_H}
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/png_to_c_array.py
            ${SSB64_CSS_ARROW_LEFT_PNG}
            ${SSB64_CSS_ARROW_LEFT_H}
            arrowLeft
    DEPENDS
        ${SSB64_CSS_ARROW_LEFT_PNG}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/png_to_c_array.py
    COMMENT "Baking scroll-arrow (left) -> ${SSB64_CSS_ARROW_LEFT_H}"
    VERBATIM
)

add_custom_target(ExtractAssetHeaders
    DEPENDS PrepareBuildInputs
    COMMENT "BattleShip asset/header generation is driven by PrepareBuildInputs"
)

################################################################################
# Collect game sources
################################################################################

# Decomp game code (exclude debug overlay and debug battle/menu)
file(GLOB_RECURSE SSB64_SRC_CREDITS "decomp/src/credits/*.c" "decomp/src/credits/*.h")
file(GLOB_RECURSE SSB64_SRC_EF      "decomp/src/ef/*.c"      "decomp/src/ef/*.h")
file(GLOB_RECURSE SSB64_SRC_FT      "decomp/src/ft/*.c"      "decomp/src/ft/*.h")
file(GLOB_RECURSE SSB64_SRC_GM      "decomp/src/gm/*.c"      "decomp/src/gm/*.h")
file(GLOB_RECURSE SSB64_SRC_GR      "decomp/src/gr/*.c"      "decomp/src/gr/*.h")
file(GLOB_RECURSE SSB64_SRC_IF      "decomp/src/if/*.c"      "decomp/src/if/*.h")
file(GLOB_RECURSE SSB64_SRC_IT      "decomp/src/it/*.c"      "decomp/src/it/*.h")
file(GLOB_RECURSE SSB64_SRC_LB      "decomp/src/lb/*.c"      "decomp/src/lb/*.h")
file(GLOB_RECURSE SSB64_SRC_MN      "decomp/src/mn/*.c"      "decomp/src/mn/*.h")
file(GLOB_RECURSE SSB64_SRC_MP      "decomp/src/mp/*.c"      "decomp/src/mp/*.h")
file(GLOB_RECURSE SSB64_SRC_MV      "decomp/src/mv/*.c"      "decomp/src/mv/*.h")
file(GLOB_RECURSE SSB64_SRC_SC      "decomp/src/sc/*.c"      "decomp/src/sc/*.h")
file(GLOB_RECURSE SSB64_SRC_SYS     "decomp/src/sys/*.c"     "decomp/src/sys/*.h")
file(GLOB_RECURSE SSB64_SRC_WP      "decomp/src/wp/*.c"      "decomp/src/wp/*.h")

# Port layer (C++ glue) — excludes port/stubs/ which is compiled as part of ssb64_game
# CONFIGURE_DEPENDS: new port TUs (e.g. a new port/enhancements/*.cpp whose
# symbol a decomp TU references) must be picked up by already-configured build
# dirs, or a plain `cmake --build` after pulling hits an undefined-reference
# link error against a stale glob.
file(GLOB_RECURSE SSB64_SRC_PORT CONFIGURE_DEPENDS "port/*.cpp" "port/*.c" "port/*.h")
list(FILTER SSB64_SRC_PORT EXCLUDE REGEX "port/stubs/")
# RelocFileTable.{us,jp}.cpp both define gRelocFileTable — compile only the
# selected version's table (the rest are siblings for the other version).
list(FILTER SSB64_SRC_PORT EXCLUDE REGEX "port/resource/RelocFileTable\\.(us|jp)\\.cpp$")
list(APPEND SSB64_SRC_PORT
    ${CMAKE_CURRENT_SOURCE_DIR}/port/resource/RelocFileTable.${SSB64_VERSION}.cpp)
# Same pattern for the generated deblob rebuild specs (docs/deblob.md):
# both regions define gSyntheticRelocSpecs — compile the selected one only.
list(FILTER SSB64_SRC_PORT EXCLUDE REGEX "port/bridge/ssb64_reloc_rebuild\\.(us|jp)\\.cpp$")
list(APPEND SSB64_SRC_PORT
    ${CMAKE_CURRENT_SOURCE_DIR}/port/bridge/ssb64_reloc_rebuild.${SSB64_VERSION}.cpp)
# coroutine_test.cpp has its own main(); only built standalone when
# SSB64_BUILD_COROUTINE_TEST=ON below. Never link it into the game.
list(FILTER SSB64_SRC_PORT EXCLUDE REGEX "port/coroutine_test\\.cpp$")
# port/mods/ (HookManager + SymbolResolver) wraps funchook, which isn't
# fetched when the mod system is disabled. Drop it so nothing references the
# missing backend. port/hooks/ (the event dispatch system) is funchook-free
# and uses core libultraship events, so it stays compiled on every target.
if (DISABLE_SCRIPTING)
    list(FILTER SSB64_SRC_PORT EXCLUDE REGEX "port/mods/")
endif()
if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    # libtorch_runner.so wraps android_torch_bridge.cpp separately; keep
    # it out of libmain.so so the static libtorch + its FetchContent'd
    # deps don't bloat the SDL-bound game library.
    list(FILTER SSB64_SRC_PORT EXCLUDE REGEX "port/android_torch_bridge\\.cpp$")
    # Drop features we explicitly disable on Android (no curl-driven
    # self-updater, no discord-rpc link, no libretro shader downloader).
    # The corresponding menu entries are gated with #if !defined(__ANDROID__)
    # in port/gui/PortMenu.cpp so nothing references the removed symbols.
    # (port/hires/ is NOT dropped here — it compiles on Android but runs
    # opt-in with a reduced LRU budget; see the PORT_HIRES_ENABLED block.)
    list(FILTER SSB64_SRC_PORT EXCLUDE REGEX
        "port/enhancements/(Updater|DiscordRichPresence|ShaderDownloader)\\.cpp$")
    list(FILTER SSB64_SRC_PORT EXCLUDE REGEX "port/port_window_icon\\.cpp$")
endif()
# Treat the generated icon header as a source so CMake schedules it ahead
# of any port TU compilation (port_window_icon.cpp #includes it).
if (NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
    list(APPEND SSB64_SRC_PORT ${SSB64_ICON_GEN})
endif()
# Arrow + nameplate baked headers — added as sources so CMake schedules their
# generation before any port TU compilation.  Stage background/icon PNGs are
# ROM-derived and therefore disk-loaded/o2r-derived at runtime; no baked
# headers for those.
list(APPEND SSB64_SRC_PORT
    ${SSB64_CSS_ARROW_H}
    ${SSB64_CSS_ARROW_LEFT_H}
    ${SSB64_CSS_NAMEPLATE_HEADERS}
)

# Debug tools (GBI trace + Acmd trace systems)
file(GLOB SSB64_DEBUG_TOOLS
    "debug_tools/gbi_trace/*.c" "debug_tools/gbi_trace/*.h"
    "debug_tools/acmd_trace/*.c" "debug_tools/acmd_trace/*.h"
)

set(SSB64_DECOMP_SOURCES
    ${SSB64_SRC_CREDITS}
    ${SSB64_SRC_EF}
    ${SSB64_SRC_FT}
    ${SSB64_SRC_GM}
    ${SSB64_SRC_GR}
    ${SSB64_SRC_IF}
    ${SSB64_SRC_IT}
    ${SSB64_SRC_LB}
    ${SSB64_SRC_MN}
    ${SSB64_SRC_MP}
    ${SSB64_SRC_MV}
    ${SSB64_SRC_SC}
    ${SSB64_SRC_SYS}
    ${SSB64_SRC_WP}
)

# Exclude .inc.c files (inline data included by other .c files)
list(FILTER SSB64_DECOMP_SOURCES EXCLUDE REGEX "\\.inc\\.c$")

# JP build excludes the US-only startup/congratulations menu sources
# (matches the upstream decomp Makefile's VERSION=jp C_FILES filter-out).
if(SSB64_VERSION STREQUAL "jp")
    list(FILTER SSB64_DECOMP_SOURCES EXCLUDE REGEX
        "decomp/src/mn/mncommon/(mncongra|mnstartup)\\.c$")
endif()

################################################################################
# Excluded directories (not compiled):
#   src/ovl8/   - Debug overlay
#   src/db/     - Debug battle/menu testing
#   src/libultra/ - Replaced by libultraship
################################################################################

# User-facing application identity. The JP build is a SEPARATE application:
# distinct binary name, distinct app-data dir (saves/config/logs/o2r), so a
# user can have both installed and they never touch each other's ROM/o2r.
# US keeps the historical "BattleShip" so existing installs / the in-app
# updater / release links are unaffected. This single value drives the
# binary OUTPUT_NAME, the libultraship Context name/shortName (which scopes
# the app-data directory), and the per-platform package scripts.
if(SSB64_VERSION STREQUAL "jp")
    set(SSB64_APP_NAME "BattleShip-JP")
else()
    set(SSB64_APP_NAME "BattleShip")
endif()
message(STATUS "SSB64 app identity: ${SSB64_APP_NAME}")

# Asset-recipe fingerprint: identifies the extraction pipeline (torch +
# config.yml + recipe yamls) that a BattleShip.o2r was produced by. The
# port stamps it into a "<o2r>.recipe" sidecar after extraction and
# re-extracts automatically when a binary update changes the recipe —
# stale archives from older builds caused reloc-token storms and menu
# crashes (issues #217/#221) until users manually re-extracted.
execute_process(
    COMMAND git -C ${CMAKE_CURRENT_SOURCE_DIR}/torch rev-parse HEAD
    OUTPUT_VARIABLE SSB64_TORCH_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
if(NOT SSB64_TORCH_SHA)
    set(SSB64_TORCH_SHA "unknown-torch")
endif()
file(SHA1 ${CMAKE_CURRENT_SOURCE_DIR}/config.yml SSB64_RECIPE_CONFIG_SHA1)
set(SSB64_RECIPE_CONCAT "${SSB64_TORCH_SHA}:${SSB64_RECIPE_CONFIG_SHA1}")
# GLOB_RECURSE: the deblob child yamls live in per-category subdirectories
# (yamls/<v>/reloc_fighters_main/*.yml, docs/deblob.md) and must invalidate
# the recipe hash — a stale archive missing slices would fail synthesis.
file(GLOB_RECURSE SSB64_RECIPE_YAMLS ${CMAKE_CURRENT_SOURCE_DIR}/yamls/${SSB64_VERSION}/*.yml)
list(SORT SSB64_RECIPE_YAMLS)
foreach(_recipe_yaml ${SSB64_RECIPE_YAMLS})
    file(SHA1 ${_recipe_yaml} _recipe_yaml_sha1)
    string(APPEND SSB64_RECIPE_CONCAT ":${_recipe_yaml_sha1}")
endforeach()
string(SHA1 SSB64_ASSET_RECIPE_HASH "${SSB64_RECIPE_CONCAT}")
message(STATUS "SSB64 asset recipe hash: ${SSB64_ASSET_RECIPE_HASH}")

# Shared compile definitions used by both the decomp object library and the
# main executable.
set(SSB64_COMPILE_DEFS
    # Region (REGION_US/VERSION_US or REGION_JP/VERSION_JP per SSB64_VERSION)
    REGION_${SSB64_VERSION_UC}=1
    VERSION_${SSB64_VERSION_UC}=1
    # App identity (binary name + libultraship app-data scope). Quoted so
    # it expands to a C string literal at the use site.
    SSB64_APP_NAME=\"${SSB64_APP_NAME}\"
    # Per-region archive filename: US writes/reads "BattleShip.o2r"
    # (preserves the historical name so existing installs keep working);
    # JP writes/reads "BattleShip-JP.o2r" so a stray archive in CWD or an
    # accidental shared install directory can't be picked up by the wrong
    # build (PortLocateFile falls back to CWD on miss, and the bifurcated
    # app-data dirs from issue #190 don't catch that case).
    SSB64_O2R_NAME=\"${SSB64_APP_NAME}.o2r\"

    # Port mode
    NON_MATCHING=1
    NON_EQUIVALENT=1
    AVOID_UB=1
    PORT=1

    # GBI / graphics
    F3DEX_GBI_2=1
    _LANGUAGE_C

    # Audio — SSB64 uses the compact "n_audio" microcode (N_MICRO).
    # This controls buffer layout and command set in the synthesis pull chain.
    N_MICRO=1

    # Platform compat
    _USE_MATH_DEFINES

    # ImGui
    CIMGUI_DEFINE_ENUMS_AND_STRUCTS

    # Updater
    BATTLESHIP_CURRENT_VERSION=\"${BATTLESHIP_VERSION}\"

    # Asset-recipe fingerprint (see the computation above SSB64_COMPILE_DEFS)
    SSB64_ASSET_RECIPE_HASH=\"${SSB64_ASSET_RECIPE_HASH}\"
)

if(SSB64_STAGE_CYCLE_DEMO)
    list(APPEND SSB64_COMPILE_DEFS PORT_STAGE_CYCLE_DEMO=1)
endif()

# Hi-res texture pack support is US-only (desktop AND Android). On Android
# it compiles but defaults to OFF and runs with a smaller decoded-RGBA8 LRU
# budget (see port/hires/HiResPack.h: kHiResEnabledDefault / kDefaultLruBudgetMB)
# plus a per-texture upscale cap, so a pack can't blow the mobile memory
# budget and trip the low-memory killer; the dev dump-tooling menu items are
# still gated off touch UI in PortMenu.cpp.
# Pack PNGs are addressed by CRC32-IEEE of the decoded RGBA8 buffer that
# came out of the US ROM fast-path; JP rendering hits different hash inputs
# (different glyph art for VS-records, JP-only menus, plus palette/sprite
# nuances on shared assets), so a US-built pack would silently miss on JP
# and no JP-only pack has been authored. Drop port/hires/ from the JP build
# entirely and gate the port.cpp / PortMenu.cpp integration on PORT_HIRES_ENABLED.
if(SSB64_VERSION STREQUAL "us")
    list(APPEND SSB64_COMPILE_DEFS PORT_HIRES_ENABLED=1)
else()
    list(FILTER SSB64_SRC_PORT EXCLUDE REGEX "port/hires/")
endif()

################################################################################
# Decomp C code — OBJECT library with custom include/ shadowing system headers
################################################################################

add_library(ssb64_game OBJECT ${SSB64_DECOMP_SOURCES})

# Port stub definitions (C code that satisfies N64 linker symbols)
file(GLOB SSB64_STUBS "port/stubs/*.c")
target_sources(ssb64_game PRIVATE ${SSB64_STUBS})

# Selectively include decomp libultra files that provide needed functionality.
# These are pure C implementations that work on any platform.
set(SSB64_LIBULTRA_PORT
    decomp/src/libultra/gu/mtxcatf.c       # guMtxCatF
    decomp/src/libultra/gu/mtxutil.c        # guMtxF2L, guMtxL2F, guMtxIdentF
    decomp/src/libultra/gu/mtxxfmf.c        # guMtxXFMF
    decomp/src/libultra/gu/normalize.c      # guNormalize
    decomp/src/libultra/gu/sinf.c           # __sinf — SGI polynomial + Cody-Waite reduction
    decomp/src/libultra/gu/cosf.c           # __cosf — same (all game trig routes here, not host libm)
    # NOTE: sprite.c is NOT included — drawbitmap is assembly-only (MIPS).
    # Sprite functions are stubbed in port/stubs/n64_stubs.c instead.
)
target_sources(ssb64_game PRIVATE ${SSB64_LIBULTRA_PORT})

# The SGI trig sources type-pun floats through int* (guint.h du/fu idiom) and
# their double-precision polynomials must evaluate identically on every host:
# AArch64 contracts a*b+c into fmadd by default, which perturbs the last ulp
# vs. the N64's separate mul/add. Their #pragma weak sinf/cosf aliases also
# capture the handful of bare sinf()/cosf() decomp call sites, matching how
# libultra linked on hardware (MSVC ignores the pragma; those sites keep host
# libm there). -fno-builtin keeps GCC/Clang from constant-folding or
# sincos-fusing bare calls past the aliases.
if (NOT MSVC)
    set_source_files_properties(
        decomp/src/libultra/gu/sinf.c
        decomp/src/libultra/gu/cosf.c
        PROPERTIES COMPILE_OPTIONS "-ffp-contract=off;-fno-strict-aliasing")
    target_compile_options(ssb64_game PRIVATE -fno-builtin-sinf -fno-builtin-cosf)
endif()

# N64 audio library — synthesis driver, sequence player, voice management.
# These decomp files are compiled with PORT defines; the CPU Acmd interpreter
# (port/audio/mixer.h) replaces RSP microcode via macro substitution.
set(SSB64_NAUDIO_PORT
    decomp/src/libultra/n_audio/n_env.c             # Core: synth driver, audio frame, pull chain, CSP, FX, event queue
    decomp/src/libultra/n_audio/n_synaddplayer.c     # Add player to synth driver
    decomp/src/libultra/n_audio/n_synallocfx.c       # FX bus allocation
    decomp/src/libultra/n_audio/n_synallocvoice.c    # Voice allocation + stealing
    decomp/src/libultra/n_audio/n_synfreevoice.c     # Voice free
    decomp/src/libultra/n_audio/n_syndelete.c        # Synth delete
    decomp/src/libultra/n_audio/n_synsetfxmix.c      # Voice FX mix
    decomp/src/libultra/n_audio/n_synsetpan.c        # Voice pan
    decomp/src/libultra/n_audio/n_synsetpitch.c      # Voice pitch
    decomp/src/libultra/n_audio/n_synsetpriority.c   # Voice priority
    decomp/src/libultra/n_audio/n_synsetvol.c        # Voice volume
    decomp/src/libultra/n_audio/n_synstartvoiceparam.c # Voice start with params
    decomp/src/libultra/n_audio/n_synstopvoice.c     # Voice stop
    decomp/src/libultra/n_audio/n_cspplay.c          # CSP play
    decomp/src/libultra/n_audio/n_cspsetbank.c       # CSP set bank
    decomp/src/libultra/n_audio/n_cspsetfxmix.c      # CSP set FX mix
    decomp/src/libultra/n_audio/n_cspsetpriority.c   # CSP set priority
    decomp/src/libultra/n_audio/n_cspsetseq.c        # CSP set sequence
    decomp/src/libultra/n_audio/n_cspsetvol.c        # CSP set volume
    decomp/src/libultra/n_audio/n_cspstop.c          # CSP stop
    decomp/src/libultra/n_audio/n_seq.c              # Sequence data reader
    decomp/src/libultra/n_audio/n_seqplayer.c        # Sequence player voice handler
    decomp/src/libultra/n_audio/n_seqpgetchlvol.c    # Get channel volume
    decomp/src/libultra/n_audio/n_seqpgetvol.c       # Get master volume
    decomp/src/libultra/audio/cents2ratio.c          # alCents2Ratio — pitch conversion
)
target_sources(ssb64_game PRIVATE ${SSB64_NAUDIO_PORT})

target_compile_definitions(ssb64_game PRIVATE ${SSB64_COMPILE_DEFS})

target_include_directories(ssb64_game PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include          # Port-tree generated headers (reloc_data.h)
    ${CMAKE_CURRENT_SOURCE_DIR}/decomp/include   # Decomp C headers (shadows system headers)
    ${CMAKE_CURRENT_SOURCE_DIR}/decomp/src       # Decomp game module headers
    ${CMAKE_CURRENT_SOURCE_DIR}/port             # Port headers (coroutine.h, etc.)
    ${CMAKE_CURRENT_SOURCE_DIR}/debug_tools      # Acmd trace (mixer.h calls acmd_trace_log_cmd)
    # Event system headers: decomp PORT hooks fire engine events directly
    # (hooks/Events.h -> ship/events/EventTypes.h + bridge/eventsbridge.h).
    ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src
    ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/include
)

# Clang 16+ promoted several historically-warning diagnostics to errors by
# default, which breaks the decomp C code on macOS/Linux. The decomp was
# written against IDO 7.1 and relies on implicit function declarations, loose
# int/pointer conversions, and incompatible library redeclarations — all of
# which MSVC merely warns about. Downgrade those here so the port can build
# without modifying every decomp source file.
if (NOT MSVC)
    # Common warnings that both Clang and GCC accept.
    target_compile_options(ssb64_game PRIVATE
        -Werror=implicit-function-declaration
        -Wno-implicit-int
        -Werror=int-conversion
        -Werror=incompatible-pointer-types
        -Wno-shift-negative-value
        -Wno-parentheses-equality
        -Wno-pointer-sign
        -Wno-unused-value
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -Wno-unused-function
        -Werror=return-type
    )
    # Clang-only diagnostics. -Wincompatible-library-redeclaration is
    # Clang-specific; -Wreturn-mismatch / -Wconstant-conversion /
    # -Wtautological-constant-out-of-range-compare exist in modern Clang
    # but only landed in GCC 14+ (or never).
    if (CMAKE_C_COMPILER_ID MATCHES "Clang")
        target_compile_options(ssb64_game PRIVATE
            -Werror=incompatible-library-redeclaration
            -Werror=return-mismatch
            -Wno-constant-conversion
            -Wno-tautological-constant-out-of-range-compare
        )
    endif()
endif()

################################################################################
# Fetch Discord RPC (desktop only — Android drops Discord Rich Presence
# along with the rest of the network-dependent features).
################################################################################
if (NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
    # discord-rpc v3.4.0's CMakeLists declares cmake_minimum_required(VERSION 2.x),
    # which CMake 3.30+ refuses to honor. Force a baseline policy version for the
    # fetched subdir so the build still configures with current CMake.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE INTERNAL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    # BattleShip links against the static CRT on Windows (see the
    # MSVC_RUNTIME_LIBRARY setting on the ssb64 / ssb64_game targets below).
    # discord-rpc's own CMakeLists guards the static-CRT switch behind
    # USE_STATIC_CRT — opt in so the produced discord-rpc.lib uses the same
    # /MT[d] runtime, otherwise the final link fails with LNK2038.
    set(USE_STATIC_CRT ON CACHE BOOL "" FORCE)
    # The bundled rapidjson-1.1.0 declares an ill-formed GenericStringRef
    # operator= (assigns to a const member); clang 22+ rejects it outright.
    # Upstream rapidjson removed the operator (rapidjson#1364) — drop the
    # line at fetch time the same way.
    # NOTE: the rapidjson patch script must NOT be a PATCH_COMMAND here.
    # It would be ineffective anyway (rapidjson is downloaded later by
    # discord-rpc's own configure — see the execute_process below), and
    # FetchContent serializes PATCH_COMMAND args into a generated CMake
    # file where a backslashed Windows ${Python3_EXECUTABLE} path becomes
    # an invalid escape sequence, failing the whole configure on MSVC CI.
    FetchContent_Declare(
        discord_rpc
        GIT_REPOSITORY https://github.com/discordapp/discord-rpc.git
        GIT_TAG        v3.4.0
        PATCH_COMMAND  ${CMAKE_COMMAND} -E rm -f .clang-format
    )
    FetchContent_MakeAvailable(discord_rpc)
    # rapidjson is downloaded by discord-rpc's own configure (inside
    # MakeAvailable), after the FetchContent patch step ran — apply the
    # clang-22 compat patch now that the file exists.
    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/patch-discord-rapidjson.py
        WORKING_DIRECTORY ${discord_rpc_SOURCE_DIR}
        COMMAND_ERROR_IS_FATAL ANY)
    # Belt and suspenders: enforce the runtime-library property on the
    # produced target directly so it survives a future cmake_policy(CMP0091)
    # propagation behavior change.
    if(MSVC AND TARGET discord-rpc)
        set_property(TARGET discord-rpc PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    # Suppress GCC 14 strict template errors in discord-rpc's bundled RapidJSON
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "14.0")
        target_compile_options(discord-rpc PRIVATE -Wno-template-body)
    endif()
endif()

################################################################################
# Executable target — port C++ code + decomp object library
################################################################################

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    # SDLActivity (Java) loads a shared library and calls SDL_main from it
    # via dlsym. Output name `main` matches SDL2's default `libmain.so`
    # lookup. Both ARM coroutine asm shims are pulled in here so the SHARED
    # target builds with the .S preprocessed by clang; each file's body is
    # #if-guarded on the target arch (__aarch64__ / __arm__), so only the
    # one matching ANDROID_ABI emits code.
    add_library(${PROJECT_NAME} SHARED
        ${SSB64_SRC_PORT}
        ${SSB64_DEBUG_TOOLS}
        $<TARGET_OBJECTS:ssb64_game>
        port/coroutine_aarch64.S
        port/coroutine_armv7.S
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "main")
else()
    add_executable(${PROJECT_NAME} ${SSB64_SRC_PORT} ${SSB64_DEBUG_TOOLS} $<TARGET_OBJECTS:ssb64_game>)

    # User-facing binary name. The CMake target keeps the historical
    # `ssb64` identifier (matches the repo, build dirs, internal symbols),
    # but the produced executable is `${SSB64_APP_NAME}` — `BattleShip`
    # (US) or `BattleShip-JP` (JP), so the two are distinct applications.
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${SSB64_APP_NAME}")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_target_properties(ssb64_game PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()

################################################################################
# Compile definitions
################################################################################

target_compile_definitions(${PROJECT_NAME} PRIVATE ${SSB64_COMPILE_DEFS})

################################################################################
# Include paths (no include/ here — port code uses system headers)
################################################################################

target_include_directories(${PROJECT_NAME} PRIVATE
    # NOTE: Do NOT add decomp/include here. The C decomp shim headers (stdlib.h,
    # stddef.h, ...) shadow the C++ standard library when picked up by port/*.cpp,
    # producing libc++ <cstdlib> errors. Only ssb64_game (C decomp) consumes them.
    ${CMAKE_CURRENT_SOURCE_DIR}/decomp/src       # Decomp game module headers (sys/, ft/, sc/, ...)
    ${CMAKE_CURRENT_SOURCE_DIR}/port
    ${CMAKE_CURRENT_SOURCE_DIR}/debug_tools
    ${SSB64_CSS_GEN_DIR}                         # CSS icon baked headers (arrow_data.h)
)
if (NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${SSB64_ICON_GEN_DIR}                         # Generated icon header (port_icon_data.h)
        ${discord_rpc_SOURCE_DIR}/include             # Discord Rich Presence
    )
else()
    # Android: include the generated dir for CSS icon baked headers.
    # (SSB64_ICON_GEN_DIR is already the same dir; listed explicitly for clarity.)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${SSB64_CSS_GEN_DIR}                          # CSS icon baked headers
    )
endif()

################################################################################
# Link libultraship and Torch
################################################################################

if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_dependencies(${PROJECT_NAME} libultraship)
    target_link_libraries(${PROJECT_NAME} PRIVATE libultraship)
else()
    add_dependencies(${PROJECT_NAME} libultraship TorchExternal discord-rpc)
    target_link_libraries(${PROJECT_NAME} PRIVATE libultraship discord-rpc)
endif()

# macOS: the port calls CFPreferences* directly (ApplePressAndHoldEnabled fix
# in main()), so link CoreFoundation explicitly rather than relying on it
# arriving transitively through libultraship's PRIVATE framework links.
if (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework CoreFoundation")
endif()
add_dependencies(ssb64_game GenerateCreditsAssets)
if (NOT DISABLE_SCRIPTING)
    # funchook hook backend (libultraship is already linked above).
    target_link_libraries(${PROJECT_NAME} PRIVATE funchook-static)

    ############################################################################
    # Mod-system linkage requirements
    #
    # 1. Disable function inlining on the main executable so engine
    #    functions remain resolvable + hookable by name. Without this, the
    #    optimizer can collapse a hookable function into its caller and
    #    eliminate the address mods would target. /Ob0 (MSVC) and
    #    -fno-inline-functions (GCC/Clang) preserve every function as a
    #    standalone symbol.
    #
    #    Cost: marginal. The decomp is already non-matching for the port
    #    build and the renderer is the perf bottleneck, not game logic.
    #
    # 2. Ensure PDB (Windows) / debug symbols (Linux/macOS) are produced
    #    for Release builds too, since symbol resolution depends on them.
    ############################################################################
    if (MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /Ob0)
        target_compile_options(ssb64_game     PRIVATE /Ob0)
        # /Zi on compile + /DEBUG on link emits a PDB for every config,
        # not just Debug. Release builds otherwise strip symbols.
        target_compile_options(${PROJECT_NAME} PRIVATE /Zi)
        target_compile_options(ssb64_game     PRIVATE /Zi)
        # NOTE: do NOT add /OPT:REF or /OPT:ICF here. /OPT:REF strips
        # unreferenced symbols (mods reach in by-name even when nothing
        # in the binary references the function); /OPT:ICF folds
        # identical code sections (two functions with the same body
        # collapse to one symbol, breaking hook-by-name when two ports
        # of the same function exist). Plain /DEBUG is enough; it just
        # tells the linker to emit a full PDB.
        target_link_options(${PROJECT_NAME}    PRIVATE /DEBUG)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -fno-inline-functions -g)
        target_compile_options(ssb64_game     PRIVATE -fno-inline-functions -g)
    endif()
endif()

################################################################################
# Platform-specific settings
################################################################################

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_options(${PROJECT_NAME} PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
    target_link_libraries(${PROJECT_NAME} PRIVATE shell32)
    # DMA functions (osEPiStartDma, osPiStartDma) are provided by LUS.
    # Game DMA code is no-opped via #ifdef PORT in dma.c, so no /FORCE needed.
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
    # Embed the application icon (Explorer / taskbar / window). The .rc
    # is generated per-region from port/ssb64.rc.in: US embeds
    # assets/icon.ico, JP embeds assets/icon-jp.ico. The icon also
    # surfaces as the .exe's Explorer thumbnail because RC resources
    # carry over to the PE's icon group.
    set(SSB64_RC_ICON_PATH "${CMAKE_SOURCE_DIR}/assets/icon.ico")
    if(SSB64_VERSION STREQUAL "jp")
        set(SSB64_RC_ICON_PATH "${CMAKE_SOURCE_DIR}/assets/icon-jp.ico")
    endif()
    configure_file(
        ${CMAKE_SOURCE_DIR}/port/ssb64.rc.in
        ${CMAKE_BINARY_DIR}/port/ssb64.rc
        @ONLY)
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/port/ssb64.rc)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,-export-dynamic)
endif()
# NOTE (APPLE): the mod hook backend needs __TEXT max-protection = rwx so
# the JIT-entitled process can flip an engine code page writable to patch
# a prologue. `-Wl,-segprot,__TEXT,rwx,r-x` would do it, but Apple's new
# linker (ld-prime, ld-12xx) silently ignores -segprot. It's instead done
# as a deterministic post-link Mach-O edit below (before codesign).

################################################################################
# TCC mod scripting: export the engine's full symbol table so libtcc-compiled
# mods can resolve any engine function by name. On Windows this is what makes
# `tcc.exe -impdef BattleShip.exe -o BattleShip.def` produce a useful export
# list for ScriptLoader's memory-mode symbol registration; on Unix the linker
# default-visibility + -Wl,-export-dynamic above handles it.
################################################################################
if(NOT DISABLE_SCRIPTING)
    # ENABLE_EXPORTS makes the EXE produce an import library (.lib) and
    # surface its symbols to runtime-loaded modules. Without it
    # WINDOWS_EXPORT_ALL_SYMBOLS only generates a .def file but the linker
    # doesn't actually emit an export table on the EXE - the .exe ends up
    # without an .edata section and tcc -impdef returns "no symbols found".
    set_target_properties(${PROJECT_NAME} PROPERTIES
        ENABLE_EXPORTS TRUE
        WINDOWS_EXPORT_ALL_SYMBOLS TRUE
    )
endif()

################################################################################
# Copy runtime data files to build directory (desktop only — Android stages
# these into the APK assets/ at Gradle time via android/app/build.gradle.kts).
################################################################################

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
    configure_file(
        ${CMAKE_SOURCE_DIR}/gamecontrollerdb.txt
        ${CMAKE_BINARY_DIR}/gamecontrollerdb.txt
        COPYONLY
    )

    add_custom_target(GenerateF3DO2R
        BYPRODUCTS ${CMAKE_SOURCE_DIR}/f3d.o2r
        COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_SOURCE_DIR}/f3d.o2r
        COMMAND ${CMAKE_COMMAND} -E tar cf ${CMAKE_SOURCE_DIR}/f3d.o2r --format=zip shaders
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/libultraship/src/fast
        COMMENT "Packaging Fast3D shaders into f3d.o2r"
    )

    add_dependencies(${PROJECT_NAME} GenerateF3DO2R)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/custom/fonts
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/yamls/${SSB64_VERSION}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/f3d.o2r
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/f3d.o2r
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/gamecontrollerdb.txt
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/gamecontrollerdb.txt
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/assets/custom/fonts/Montserrat-Regular.ttf
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/custom/fonts/Montserrat-Regular.ttf
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/assets/custom/fonts/Inconsolata-Regular.ttf
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/custom/fonts/Inconsolata-Regular.ttf
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/config.yml
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/config.yml
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/yamls/${SSB64_VERSION}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/yamls/${SSB64_VERSION}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TORCH_EXECUTABLE}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/${TORCH_SIDECAR_NAME}
        # CSS stage asset PNGs (final_destination_background.png, etc.) are
        # copied in a separate POST_BUILD step added by the DeriveStageAssets
        # block further below, after SSB64_BASEROM is resolved.
        # Arrow data is baked into the binary via arrow_data.h / arrow_left_data.h.
    )
else()
    # Android: produce f3d.o2r in the source tree so Gradle's stageGameAssets
    # task can pick it up at APK build time. No POST_BUILD copies — the APK
    # asset pipeline handles staging into externalFilesDir on first launch.
    add_custom_target(GenerateF3DO2R
        BYPRODUCTS ${CMAKE_SOURCE_DIR}/f3d.o2r
        COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_SOURCE_DIR}/f3d.o2r
        COMMAND ${CMAKE_COMMAND} -E tar cf ${CMAKE_SOURCE_DIR}/f3d.o2r --format=zip shaders
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/libultraship/src/fast
        COMMENT "Packaging Fast3D shaders into f3d.o2r (Android)"
    )
    add_dependencies(${PROJECT_NAME} GenerateF3DO2R)
endif()

################################################################################
# TCC mod scripting: post-build .tcc/ population + BattleShip.def generation
#
# At runtime the engine's ScriptLoader feeds mod source through libtcc with
# include paths under .tcc/include/ and library paths under .tcc/lib/. We
# populate both from the FetchContent'd TinyCC source tree so the binary is
# self-contained; modders don't have to install TinyCC.
#
# On Windows we additionally run `tcc.exe -impdef BattleShip.exe -o
# BattleShip.def` to generate the export-name list that ScriptLoader resolves
# against the running process before TCC relocates mod code. WINDOWS_EXPORT_ALL_SYMBOLS
# (set above) makes the EXE's export table contain every engine function so
# the .def covers them all.
################################################################################
if(NOT DISABLE_SCRIPTING)
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMENT "Copying libtcc headers and runtime libraries..."
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/"
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/lib/"
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/include/"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_BINARY_DIR}/_deps/tinycc-src/include/"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/include/"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_BINARY_DIR}/_deps/tinycc-src/win32/include/"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/include/"
            # libtcc1.a built by MSVC is COFF format which TCC can't
            # parse. Use tcc.exe itself to recompile the runtime source
            # into a TCC-readable archive. -B points tcc at .tcc/ for
            # tccdefs.h (auto-included via the command line).
            COMMAND "$<TARGET_FILE:tcc>"
                -B "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc"
                -c "${CMAKE_BINARY_DIR}/_deps/tinycc-src/lib/libtcc1.c"
                -o "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/lib/libtcc1.o"
            COMMAND "$<TARGET_FILE:tcc>" -ar rcs
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/lib/libtcc1.a"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/lib/libtcc1.o"
            COMMAND ${CMAKE_COMMAND} -E rm -f
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/lib/libtcc1.o"
            VERBATIM
        )
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMENT "Generating BattleShip.def via tcc -impdef..."
            COMMAND "$<TARGET_FILE:tcc>"
                    -impdef "$<TARGET_FILE:${PROJECT_NAME}>"
                    -o "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/lib/BattleShip.def"
            VERBATIM
        )
        add_dependencies(${PROJECT_NAME} tcc)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMENT "Copying libtcc headers and libs..."
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/"
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/lib/"
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/include/"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_BINARY_DIR}/_deps/tinycc-src/include/"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/include/"
            COMMAND ${CMAKE_COMMAND} -E copy
                "$<TARGET_FILE:libtcc1>"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/lib/libtcc1.a"
            VERBATIM
        )
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMENT "Copying libtcc headers and libs..."
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/"
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/lib/"
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/include/"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_BINARY_DIR}/_deps/tinycc-src/include/"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/include/"
            COMMAND ${CMAKE_COMMAND} -E copy
                "$<TARGET_FILE:libtcc1>"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/.tcc/lib/libtcc1.a"
            VERBATIM
        )
    endif()
endif()

################################################################################
# macOS: re-sign with JIT entitlements so the mod loader's hook backend
# (funchook) can patch engine __TEXT at runtime.
#
# Apple Silicon enforces W^X on code-signed text pages: funchook's inline
# detour mprotect(RWX) is denied ("Failed to unprotect memory ...
# Permission denied") on a linker-/ad-hoc-signed binary with no
# entitlements. Re-signing ad-hoc WITH the JIT entitlements lifts that —
# the cs.allow-* keys are honored for ad-hoc signatures. Must run LAST
# (any later binary mutation would invalidate the signature). No-op on
# Intel macs too, but harmless; gated to APPLE.
################################################################################

if(APPLE AND NOT DISABLE_SCRIPTING)
    find_package(Python3 QUIET COMPONENTS Interpreter)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMENT "macOS: __TEXT->rwx maxprot + ad-hoc re-sign w/ JIT entitlements (mod hook backend)"
        # 1. Raise __TEXT max-protection to rwx so the hook backend can
        #    flip an engine code page writable at runtime.
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/cmake/macho_text_rwx.py
            "$<TARGET_FILE:${PROJECT_NAME}>"
        # 2. Re-sign LAST (seals the modified binary). The cs.allow-*
        #    entitlements are honored for ad-hoc signatures.
        COMMAND codesign --force --sign - --timestamp=none
            --entitlements "${CMAKE_SOURCE_DIR}/cmake/macos_entitlements.plist"
            "$<TARGET_FILE:${PROJECT_NAME}>"
        VERBATIM
    )
endif()

################################################################################
# Asset extraction targets (desktop only — Android extracts on-device from
# the user's ROM via libtorch_runner.so at first launch; see
# port/android_torch_bridge.cpp + RomImporter.java).
################################################################################

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
# Accept .z64 (big-endian), .n64 (little-endian), or .v64 (byteswapped).
# Torch's Cartridge::Normalize converts non-z64 byte orders in-memory.
set(SSB64_BASEROM "")
foreach(ext z64 n64 v64)
    if(EXISTS "${CMAKE_SOURCE_DIR}/baserom.${SSB64_VERSION}.${ext}")
        set(SSB64_BASEROM "${CMAKE_SOURCE_DIR}/baserom.${SSB64_VERSION}.${ext}")
        break()
    endif()
endforeach()
if(NOT SSB64_BASEROM)
    set(SSB64_BASEROM "${CMAKE_SOURCE_DIR}/baserom.${SSB64_VERSION}.z64")
endif()

# BattleShip.o2r is per-ROM-version. The decomp is region-split, so US and
# JP use separate build dirs — extracting into the SHARED source root made a
# US build and a JP build clobber each other's archive. Extract into a
# per-build-dir sink (${CMAKE_BINARY_DIR}/extracted) instead, then stage it
# next to the binary. Torch writes config.yml's `output.binary`
# (BattleShip.o2r, identical for both regions) into its `-d` dir,
# independent of `-s` (where it reads config/yamls). The runtime archive
# name stays the fixed "BattleShip.o2r"; only the dev-build location is
# isolated. Packaged-release first-run extraction (sidecar torch +
# config.yml, its own -d) is unaffected.
set(SSB64_O2R_DIR ${CMAKE_BINARY_DIR}/extracted)
set(SSB64_O2R     ${SSB64_O2R_DIR}/BattleShip.o2r)

# Two configurations:
#
#   (a) Baserom present (typical dev / Torch-extract first-run): declare the
#       o2r as an add_custom_command(OUTPUT ...) so downstream rules that
#       DEPENDS on it as a file connect to this producer through CMake's
#       file-level dependency graph. The wrapping custom_target keeps
#       `cmake --build . --target ExtractAssets` working.
#
#   (b) Baserom absent (CI release pipeline never ships the copyrighted ROM):
#       provide a stub ExtractAssets target that fails with a clear error if
#       invoked, but does NOT register the file-level rule. Any caller that
#       genuinely needs the o2r will hit ExtractAssets and get the
#       explanatory error.
if(EXISTS "${SSB64_BASEROM}")
    add_custom_command(
        OUTPUT ${SSB64_O2R}
        # SSB64_RECIPE_YAMLS + config.yml: editing any extraction recipe
        # (child slice maps included) re-extracts — previously a yaml edit
        # silently left a stale archive.
        DEPENDS TorchExternal PrepareBuildInputs GenerateF3DO2R ${SSB64_BASEROM}
                ${SSB64_RECIPE_YAMLS} ${CMAKE_SOURCE_DIR}/config.yml
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SSB64_O2R_DIR}
        COMMAND ${TORCH_EXECUTABLE} o2r ${SSB64_BASEROM} -s ${CMAKE_SOURCE_DIR} -d ${SSB64_O2R_DIR}
        COMMENT "Extracting ${SSB64_VERSION} assets from ${SSB64_BASEROM} -> ${SSB64_O2R}"
    )
    add_custom_target(ExtractAssets DEPENDS ${SSB64_O2R})
    # Deblob validation gate (docs/deblob.md): builds every deblobbed
    # bundle through the real archive/factory/synthesis path and fails on
    # any I5/I7/I9 violation. Mandatory local pre-merge for changes to
    # torch/, yamls/, or the synthesis code (CI has no staged archive).
    # Runs headless-ish (exits before the game boots; a window may flash).
    add_custom_target(synth_verify
        COMMAND ${CMAKE_COMMAND} -E env SSB64_SYNTH_SELFTEST=1 $<TARGET_FILE:ssb64>
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ssb64
        COMMENT "deblob synth_verify: synthesizing every deblobbed bundle (I5 gate)"
        USES_TERMINAL
    )
    # Baserom present (dev build): make the normal build also stage the
    # o2r next to the binary, so the flow is a true 3 commands —
    # configure, build, run — with no separate ExtractAssets step. The
    # o2r is an OUTPUT custom command, so it only re-extracts when its
    # inputs (torch / build inputs / baserom) change; cached otherwise.
    # The release pipeline ships no baserom and hits the else() branch,
    # so packaged builds are unaffected (runtime first-run extracts).
    #
    # Staging the o2r into $<TARGET_FILE_DIR:ssb64> is done as a POST_BUILD
    # step on ssb64 rather than inside the ExtractAssets custom_command —
    # referencing $<TARGET_FILE_DIR:ssb64> inside the command would create
    # a back-edge from ExtractAssets to ssb64 (CMake genex creates target
    # deps), and combined with `add_dependencies(ssb64 ExtractAssets)`
    # below produces a strongly-connected-component error at generate time.
    add_dependencies(${PROJECT_NAME} ExtractAssets)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SSB64_O2R}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/BattleShip.o2r
        COMMENT "Staging BattleShip.o2r next to ssb64 binary"
    )
else()
    add_custom_target(ExtractAssets
        COMMAND ${CMAKE_COMMAND} -E echo
            "ERROR: ExtractAssets requires baserom.${SSB64_VERSION}.{z64,n64,v64} in source root (${CMAKE_SOURCE_DIR})"
        COMMAND ${CMAKE_COMMAND} -E false
        COMMENT "ExtractAssets stub — baserom missing"
    )
    message(STATUS "BattleShip.o2r: baserom absent — ExtractAssets stub installed")
endif()

add_custom_target(GenerateBattleShipO2R
    DEPENDS ExtractAssets
    COMMENT "Generating BattleShip.o2r"
)

# ── CSS stage asset extraction ─────────────────────────────────────────────
# derive_stage_assets.py extracts the FD background + icon PNGs from the ROM,
# and synthesizes the FD nameplate PNG (96x10, PIL bitmap font, no ROM data).
# These PNGs are then copied to the run directory so the game can load them
# at runtime without any Nintendo-derived bytes baked into the binary.
#
# Desktop-only.  Android is a follow-up task (stage PNGs need to be packaged
# into the APK assets/ and extracted to externalFilesDir on first launch).
if(EXISTS "${SSB64_BASEROM}")
    add_custom_command(
        OUTPUT
            ${SSB64_CSS_FD_BG_PNG}
            ${SSB64_CSS_FD_SMALL_PNG}
            ${SSB64_CSS_FD_NAME_PNG}
            ${SSB64_CSS_FD_EMBLEM_PNG}
            ${SSB64_CSS_MC_BG_PNG}
            ${SSB64_CSS_MC_SMALL_PNG}
            ${SSB64_CSS_MC_NAME_PNG}
            ${SSB64_CSS_BF_BG_PNG}
            ${SSB64_CSS_BF_SMALL_PNG}
            ${SSB64_CSS_BF_NAME_PNG}
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/derive_stage_assets.py
                ${SSB64_BASEROM}
                ${SSB64_CSS_STAGE_ASSET_DIR}
        DEPENDS
            ${SSB64_BASEROM}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/derive_stage_assets.py
        COMMENT "Extracting CSS stage assets from ${SSB64_BASEROM}"
        VERBATIM
    )
    add_custom_target(DeriveStageAssets
        DEPENDS
            ${SSB64_CSS_FD_BG_PNG}
            ${SSB64_CSS_FD_SMALL_PNG}
            ${SSB64_CSS_FD_NAME_PNG}
            ${SSB64_CSS_FD_EMBLEM_PNG}
            ${SSB64_CSS_MC_BG_PNG}
            ${SSB64_CSS_MC_SMALL_PNG}
            ${SSB64_CSS_MC_NAME_PNG}
            ${SSB64_CSS_BF_BG_PNG}
            ${SSB64_CSS_BF_SMALL_PNG}
            ${SSB64_CSS_BF_NAME_PNG}
    )
    add_dependencies(${PROJECT_NAME} DeriveStageAssets)
    # Copy extracted PNGs into the run-directory asset subtree so the game
    # can find them at  <run-dir>/assets/css_icons/<name>_*.png .
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/css_icons
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SSB64_CSS_FD_BG_PNG}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/css_icons/final_destination_background.png
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SSB64_CSS_FD_SMALL_PNG}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/css_icons/final_destination_small.png
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SSB64_CSS_FD_NAME_PNG}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/css_icons/final_destination_name.png
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SSB64_CSS_FD_EMBLEM_PNG}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/css_icons/final_destination_emblem.png
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SSB64_CSS_MC_BG_PNG}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/css_icons/metal_cavern_background.png
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SSB64_CSS_MC_SMALL_PNG}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/css_icons/metal_cavern_small.png
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SSB64_CSS_MC_NAME_PNG}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/css_icons/metal_cavern_name.png
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SSB64_CSS_BF_BG_PNG}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/css_icons/battlefield_background.png
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SSB64_CSS_BF_SMALL_PNG}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/css_icons/battlefield_small.png
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SSB64_CSS_BF_NAME_PNG}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/css_icons/battlefield_name.png
        COMMENT "Staging CSS stage asset PNGs next to ssb64 binary"
    )
else()
    add_custom_target(DeriveStageAssets
        COMMENT "DeriveStageAssets: skipped (baserom absent)"
    )
    message(STATUS "CSS stage assets: baserom absent — DeriveStageAssets stub installed")
endif()

# NOTE: there is intentionally no port-asset o2r target. `ssb64.o2r` was an
# early-development artifact; the runtime loads only BattleShip.o2r
# (ROM-derived, user-extracted at first run) and f3d.o2r (shaders). No
# ROM-derived data is ever produced by the build.

endif() # NOT Android — ExtractAssets / GenerateBattleShipO2R block

################################################################################
# Android-only targets: torch_runner SHARED library + coroutine_test harness
################################################################################
if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    # libtorch_runner.so — wraps the static libtorch.a (from add_subdirectory
    # at the top) with a small extern "C" entry point that the first-run
    # Activity calls via JNI to extract the user's ROM into BattleShip.o2r.
    # Lives outside libmain.so so Java can load it BEFORE SDLActivity/libSDL2
    # are initialized — the ROM picker is a regular Activity that runs before
    # the SDL game loop starts.
    add_library(torch_runner SHARED port/android_torch_bridge.cpp)
    target_include_directories(torch_runner PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/torch/src
    )
    # The extractor + its FetchContent'd deps (yaml-cpp, tinyxml2, libgfxd,
    # BinaryTools, N64Graphics, spdlog, GSL, ...). All are static libs from
    # torch's CMakeLists.txt under USE_STANDALONE=OFF.
    target_link_libraries(torch_runner PRIVATE torch)
    set_target_properties(torch_runner PROPERTIES OUTPUT_NAME "torch_runner")

    # Standalone ARM coroutine test (off by default — opt in for asm
    # regression validation, see docs/android_port_status_*.md). Pushes a
    # tiny binary to /data/local/tmp/ to verify the asm context-switch
    # without needing the full game. Both .S shims are arch-#if-guarded, so
    # the same target builds for arm64-v8a and armeabi-v7a.
    option(SSB64_BUILD_COROUTINE_TEST
        "Build the ARM coroutine standalone test binary (Android)" OFF)
    if (SSB64_BUILD_COROUTINE_TEST)
        add_executable(coroutine_test
            port/coroutine_test.cpp
            port/coroutine_android.cpp
            port/coroutine_aarch64.S
            port/coroutine_armv7.S
        )
        target_include_directories(coroutine_test PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/port
        )
    endif()
endif()

################################################################################
# Workspace TCC mods (desktop only — the mod system is excluded on Android)
#
# Each mod under workspace/ has its own CMakeLists.txt that bundles the
# mod's source + assets into output/ and stages a copy under
# build/<config>/mods/<ModName>/ so the runtime loader picks it up.
# ModO2R provides the optional ssb64_add_mod_o2r_target helper for
# packaging into a single .o2r distributable.
################################################################################
if (NOT DISABLE_SCRIPTING)
    include(ModO2R)
endif()
