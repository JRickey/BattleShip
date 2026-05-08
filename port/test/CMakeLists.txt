################################################################################
# port_reloc_tests — GTest harness for the runtime reloc-data pipeline
#
# Goal (Stage 1 of docs/refactor_buildtime_reloc_plan.md): drive the bridge's
# load+fixup pipeline against a deterministic mock heap with snapshot capture,
# so a per-file unit test can pin the runtime's output byte-for-byte. Each
# subsequent migration stage (move Pass1 to Torch, move Pass2, etc.) keeps
# every snapshot green.
#
# Builds when -DSSB64_BUILD_TESTS=ON. Compiles a small slice of port/ sources
# directly (the bridge, byteswap helpers, port_aobj_fixup, and the resource
# layer) — separate from the ssb64 executable's own compile of those sources.
# Cheap (~3 .cpp files of bridge code; libultraship is reused via link).
################################################################################

include(FetchContent)

# Reuse libultraship's googletest if it brought one in (it does when
# LUS_BUILD_TESTS=ON). Otherwise pull our own — same SHA so versions match.
if (NOT TARGET gtest_main)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.16.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

set(PORT_TEST_PRODUCTION_SOURCES
    ${CMAKE_SOURCE_DIR}/port/bridge/lbreloc_bridge.cpp
    ${CMAKE_SOURCE_DIR}/port/bridge/lbreloc_byteswap.cpp
    ${CMAKE_SOURCE_DIR}/port/port_aobj_fixup.cpp
    ${CMAKE_SOURCE_DIR}/port/port_log.c
    ${CMAKE_SOURCE_DIR}/port/resource/RelocFile.cpp
    ${CMAKE_SOURCE_DIR}/port/resource/RelocFileFactory.cpp
    ${CMAKE_SOURCE_DIR}/port/resource/RelocFileTable.cpp
    ${CMAKE_SOURCE_DIR}/port/resource/RelocPointerTable.cpp
)

set(PORT_TEST_HARNESS_SOURCES
    mock_heap.cpp
    snapshot.cpp
    reloc_harness.cpp
    test_main.cpp
    test_stubs.cpp
)

set(PORT_TEST_CASE_SOURCES
    sanity_tests.cpp
)

add_executable(port_reloc_tests
    ${PORT_TEST_PRODUCTION_SOURCES}
    ${PORT_TEST_HARNESS_SOURCES}
    ${PORT_TEST_CASE_SOURCES}
)

set_property(TARGET port_reloc_tests PROPERTY CXX_STANDARD 20)
set_property(TARGET port_reloc_tests PROPERTY C_STANDARD 11)

target_include_directories(port_reloc_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/port
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# The bridge sources are guarded by the same #defines as ssb64's normal build.
# F3DEX_GBI_2 is read by lbreloc_byteswap.cpp's GBI opcode constants.
target_compile_definitions(port_reloc_tests PRIVATE
    PORT=1
    F3DEX_GBI_2=1
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF
)

# libultraship gives us Ship::ResourceManager + Ship::Context plus the
# texture-cache / DL-cache extern symbols the bridge calls into during a
# load. Tests use a hand-rolled test_main.cpp (controlled Ship::Context
# teardown via gtest Environment), so we link against the gtest library
# without gtest_main.
target_link_libraries(port_reloc_tests PRIVATE
    libultraship
    gtest
)

include(GoogleTest)
gtest_discover_tests(port_reloc_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    PROPERTIES
        ENVIRONMENT "SSB64_TEST_O2R_PATH=${CMAKE_BINARY_DIR}/BattleShip.o2r"
)
