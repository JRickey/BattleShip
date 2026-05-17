cmake_minimum_required(VERSION 3.24.0 FATAL_ERROR)

project(ssb64 LANGUAGES C CXX ASM)

include(ExternalProject)

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

################################################################################
# Options
################################################################################

option(NON_MATCHING "Build without matching decomp constraints" ON)
option(NON_EQUIVALENT "Build without equivalent decomp constraints" ON)
option(USE_AUTO_VCPKG "Use automated vcpkg for dependency management" ON)
option(SSB64_STAGE_CYCLE_DEMO "Boot directly into attract-demo stage cycle for rendering regression testing" OFF)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

################################################################################
# libultraship configuration (set before add_subdirectory)
################################################################################

# O2R (ZIP) only, no MPQ/StormLib
set(INCLUDE_MPQ_SUPPORT OFF CACHE BOOL "" FORCE)
set(GBI_UCODE "F3DEX_GBI_2" CACHE STRING "" FORCE)

# Build Torch as the original standalone sidecar executable.
set(USE_STANDALONE ON CACHE BOOL "" FORCE)
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

if (NOT CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
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
endif() # NOT NintendoSwitch

set(SSB64_RELOC_YAML_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/yamls/us/reloc_animations.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/yamls/us/reloc_bonus.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/yamls/us/reloc_effects.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/yamls/us/reloc_extern_data.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/yamls/us/reloc_fighters_common.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/yamls/us/reloc_fighters_main.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/yamls/us/reloc_interface.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/yamls/us/reloc_items.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/yamls/us/reloc_menus.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/yamls/us/reloc_misc_named.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/yamls/us/reloc_movies.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/yamls/us/reloc_scene.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/yamls/us/reloc_stages.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/yamls/us/reloc_submotions.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/yamls/us/reloc_transitions.yml
)

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
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_yamls.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Regenerating reloc YAML configs"
)

add_custom_target(GenerateRelocArtifacts
    BYPRODUCTS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/reloc_data.h
        ${CMAKE_CURRENT_SOURCE_DIR}/port/resource/RelocFileTable.cpp
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_reloc_stubs.py
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_reloc_table.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating reloc headers and RelocFileTable.cpp"
)

add_custom_target(GenerateCreditsAssets
    BYPRODUCTS ${SSB64_CREDITS_GENERATED_FILES}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/creditsTextConverter.py staff.credits.us.txt
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/creditsTextConverter.py titles.credits.us.txt
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/creditsTextConverter.py -paragraphFont -multiline info.credits.us.txt
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/creditsTextConverter.py -paragraphFont companies.credits.us.txt
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/decomp/src/credits
    COMMENT "Encoding credits text assets"
)

add_custom_target(PrepareBuildInputs
    DEPENDS GenerateRelocArtifacts GenerateCreditsAssets
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
file(GLOB_RECURSE SSB64_SRC_PORT "port/*.cpp" "port/*.c" "port/*.h")
list(FILTER SSB64_SRC_PORT EXCLUDE REGEX "port/stubs/")

if(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    list(APPEND SSB64_SRC_PORT "port/switch/context_switch.s")
endif()

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

################################################################################
# Excluded directories (not compiled):
#   src/ovl8/   - Debug overlay
#   src/db/     - Debug battle/menu testing
#   src/libultra/ - Replaced by libultraship
################################################################################

# Shared compile definitions used by both the decomp object library and the
# main executable.
set(SSB64_COMPILE_DEFS
    # Region
    REGION_US=1
    VERSION_US=1

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
)

if(SSB64_STAGE_CYCLE_DEMO)
    list(APPEND SSB64_COMPILE_DEFS PORT_STAGE_CYCLE_DEMO=1)
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
    # NOTE: sprite.c is NOT included — drawbitmap is assembly-only (MIPS).
    # Sprite functions are stubbed in port/stubs/n64_stubs.c instead.
)
target_sources(ssb64_game PRIVATE ${SSB64_LIBULTRA_PORT})

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
            -Wno-parentheses-equality
            -Wno-constant-conversion
            -Wno-tautological-constant-out-of-range-compare
        )
    endif()
endif()

################################################################################
# Executable target — port C++ code + decomp object library
################################################################################

add_executable(${PROJECT_NAME} ${SSB64_SRC_PORT} ${SSB64_DEBUG_TOOLS} $<TARGET_OBJECTS:ssb64_game>)

# User-facing binary name. The CMake target keeps the historical
# `ssb64` identifier (matches the repo, build dirs, internal symbols),
# but the produced executable is `BattleShip` / `BattleShip.exe`.
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "BattleShip")

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
)

################################################################################
# Link libultraship and Torch
################################################################################

if(NOT CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
add_dependencies(${PROJECT_NAME} libultraship TorchExternal)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE libultraship)

################################################################################
# Platform-specific settings
################################################################################

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_options(${PROJECT_NAME} PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
    target_link_libraries(${PROJECT_NAME} PRIVATE shell32)
    # DMA functions (osEPiStartDma, osPiStartDma) are provided by LUS.
    # Game DMA code is no-opped via #ifdef PORT in dma.c, so no /FORCE needed.
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
    # Embed the application icon (Explorer / taskbar / window).
    target_sources(${PROJECT_NAME} PRIVATE port/ssb64.rc)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,-export-dynamic)
elseif (CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    target_compile_definitions(ssb64_game PRIVATE __SWITCH__ _GNU_SOURCE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE __SWITCH__ _GNU_SOURCE)

    find_program(NACPTOOL_EXECUTABLE
        NAMES nacptool
        HINTS
            "$ENV{DEVKITPRO}/tools/bin"
            "${DEVKITPRO}/tools/bin"
        REQUIRED
    )

    find_program(ELF2NRO_EXECUTABLE
        NAMES elf2nro
        HINTS
            "$ENV{DEVKITPRO}/tools/bin"
            "${DEVKITPRO}/tools/bin"
        REQUIRED
    )

    # Aggressive optimization for Tegra X1 (Cortex-A57).
    # -march is already set by the devkitPro toolchain; we only add -mtune and -O3.
    set(SWITCH_OPT_FLAGS -O3 -mtune=cortex-a57 -ffast-math)
    target_compile_options(ssb64_game PRIVATE ${SWITCH_OPT_FLAGS})
    target_compile_options(${PROJECT_NAME} PRIVATE ${SWITCH_OPT_FLAGS})
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,--gc-sections)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${PROJECT_NAME}>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/BattleShip.elf.keep
        COMMAND ${NACPTOOL_EXECUTABLE}
            --create
            BattleShip
            JRickey
            ${BATTLESHIP_VERSION}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/control.nacp
        COMMAND ${ELF2NRO_EXECUTABLE}
            $<TARGET_FILE:${PROJECT_NAME}>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/BattleShip.nro
            --alignedheader
            --icon=/opt/devkitpro/libnx/default_icon.jpg
            --nacp=$<TARGET_FILE_DIR:${PROJECT_NAME}>/control.nacp
        COMMENT "Packaging BattleShip.nro from BattleShip.elf"
    )

    if(TARGET TorchExternal)
        add_dependencies(${PROJECT_NAME} TorchExternal)
    endif()
endif()

################################################################################
# Copy runtime data files to build directory
################################################################################

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
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/yamls/us
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
        ${CMAKE_SOURCE_DIR}/yamls/us
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/yamls/us
    COMMAND ${CMAKE_COMMAND} -E echo "Post-build copy complete"
)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TORCH_EXECUTABLE}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/${TORCH_SIDECAR_NAME}
    )
endif()

################################################################################
# Asset extraction targets
################################################################################

# Accept .z64 (big-endian), .n64 (little-endian), or .v64 (byteswapped).
# Torch's Cartridge::Normalize converts non-z64 byte orders in-memory.
set(SSB64_BASEROM "")
foreach(ext z64 n64 v64)
    if(EXISTS "${CMAKE_SOURCE_DIR}/baserom.us.${ext}")
        set(SSB64_BASEROM "${CMAKE_SOURCE_DIR}/baserom.us.${ext}")
        break()
    endif()
endforeach()
if(NOT SSB64_BASEROM)
    set(SSB64_BASEROM "${CMAKE_SOURCE_DIR}/baserom.us.z64")
endif()

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
        OUTPUT ${CMAKE_SOURCE_DIR}/BattleShip.o2r
        DEPENDS TorchExternal PrepareBuildInputs GenerateF3DO2R ${SSB64_BASEROM}
        COMMAND ${TORCH_EXECUTABLE} o2r ${SSB64_BASEROM} -s ${CMAKE_SOURCE_DIR} -d ${CMAKE_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/BattleShip.o2r
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/BattleShip.o2r
        COMMENT "Extracting assets from ${SSB64_BASEROM} into O2R archive"
    )
    add_custom_target(ExtractAssets DEPENDS ${CMAKE_SOURCE_DIR}/BattleShip.o2r)
else()
    add_custom_target(ExtractAssets
        COMMAND ${CMAKE_COMMAND} -E echo
            "ERROR: ExtractAssets requires baserom.us.{z64,n64,v64} in source root (${CMAKE_SOURCE_DIR})"
        COMMAND ${CMAKE_COMMAND} -E false
        COMMENT "ExtractAssets stub — baserom missing"
    )
    message(STATUS "BattleShip.o2r: baserom absent — ExtractAssets stub installed")
endif()

if(NOT CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
add_custom_target(GenerateBattleShipO2R
    DEPENDS ExtractAssets
    COMMENT "Generating BattleShip.o2r"
)

add_custom_target(GeneratePortO2R
    DEPENDS TorchExternal PrepareBuildInputs
    COMMAND ${TORCH_EXECUTABLE} pack ${CMAKE_SOURCE_DIR}/assets ${CMAKE_SOURCE_DIR}/ssb64.o2r o2r
    COMMENT "Packing port-specific assets into ssb64.o2r"
)
endif() # NOT NintendoSwitch

################################################################################
# Switch deployment helpers (extract assets on PC, bundle alongside .nro)
################################################################################

if (CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    set(SWITCH_SD_DIR "${CMAKE_BINARY_DIR}/switch_sd")
    set(SWITCH_GAME_DIR "${SWITCH_SD_DIR}/switch/BattleShip")

    file(MAKE_DIRECTORY "${SWITCH_GAME_DIR}")

    add_custom_target(BundleSwitch
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/BattleShip.nro"
            "${SWITCH_GAME_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/BattleShip.o2r"
            "${SWITCH_GAME_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/f3d.o2r"
            "${SWITCH_GAME_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/gamecontrollerdb.txt"
            "${SWITCH_GAME_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${SWITCH_GAME_DIR}/shader_cache"
        COMMENT "Bundling Switch deployment files into ${SWITCH_GAME_DIR}"
    )
endif()
