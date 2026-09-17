cmake_minimum_required(VERSION 3.24)

set(CMAKE_SYSTEM_NAME WindowsStore)
set(CMAKE_SYSTEM_VERSION 10.0.19041.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(BattleShip-UWP LANGUAGES CXX)
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT ${PROJECT_NAME})

set(BATTLESHIP_BUILD_DIR "" CACHE PATH "BattleShip native build directory")
set(BATTLESHIP_PACKAGE_VERSION "1.0.0.0" CACHE STRING "Four-part MSIX package version")
if(NOT BATTLESHIP_BUILD_DIR)
    message(FATAL_ERROR "BATTLESHIP_BUILD_DIR must point to the native BattleShip build")
endif()
if(NOT BATTLESHIP_PACKAGE_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "BATTLESHIP_PACKAGE_VERSION must be a four-part MSIX version")
endif()
set(BATTLESHIP_NATIVE_CONFIG "Release" CACHE STRING "Native BattleShip configuration to package")

set(BATTLESHIP_MANIFEST "${CMAKE_CURRENT_BINARY_DIR}/Package.appxmanifest")
configure_file(Package.appxmanifest "${BATTLESHIP_MANIFEST}" @ONLY)

add_executable(${PROJECT_NAME} WIN32 main.cpp)

set(NATIVE_DIR "${BATTLESHIP_BUILD_DIR}/${BATTLESHIP_NATIVE_CONFIG}")
set(UWP_DEPS "${BATTLESHIP_BUILD_DIR}/_deps/uwplibs-src/x64")

set(BIN_DEPS
    "${NATIVE_DIR}/BattleShip.dll"
    "${NATIVE_DIR}/f3d.o2r"
    "${NATIVE_DIR}/gamecontrollerdb.txt"
    "${UWP_DEPS}/bin/dxil.dll"
    "${UWP_DEPS}/bin/glfw3.dll"
    "${UWP_DEPS}/bin/libgallium_wgl.dll"
    "${UWP_DEPS}/bin/libuwp.dll"
    "${UWP_DEPS}/bin/opengl32.dll"
    "${UWP_DEPS}/bin/SDL2.dll"
    "${UWP_DEPS}/bin/z-1.dll"
    "${BATTLESHIP_MANIFEST}"
    INSTALL.txt
)
set_source_files_properties(${BIN_DEPS} PROPERTIES
    VS_COPY_TO_OUT_DIR Always
    VS_DEPLOYMENT_CONTENT TRUE
    VS_DEPLOYMENT_LOCATION "."
)

set(PKG_ASSETS
    Assets/LockScreenLogo.scale-200.png
    Assets/Square44x44Logo.targetsize-24_altform-unplated.png
    Assets/Square44x44Logo.scale-200.png
    Assets/Square150x150Logo.scale-200.png
    Assets/StoreLogo.png
    Assets/Wide310x150Logo.scale-200.png
)
set_source_files_properties(${PKG_ASSETS} PROPERTIES
    VS_DEPLOYMENT_CONTENT TRUE
    VS_DEPLOYMENT_LOCATION "Assets"
)

file(GLOB FONT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../assets/custom/fonts/*")
foreach(FONT_FILE ${FONT_FILES})
    set_source_files_properties("${FONT_FILE}" PROPERTIES
        VS_COPY_TO_OUT_DIR Always
        VS_DEPLOYMENT_CONTENT TRUE
        VS_DEPLOYMENT_LOCATION "assets/custom/fonts"
    )
endforeach()

target_sources(${PROJECT_NAME} PRIVATE ${BIN_DEPS} ${PKG_ASSETS} ${FONT_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE "${UWP_DEPS}/include")
target_link_libraries(${PROJECT_NAME} PRIVATE
    "${UWP_DEPS}/lib/libuwp.lib"
    "${UWP_DEPS}/lib/SDL2.lib"
    OneCore.lib
    WindowsApp.lib
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION 10.0.19041.0
    VS_GLOBAL_AppxPackage true
    VS_GLOBAL_AppxBundle Always
    VS_GLOBAL_AppxBundlePlatforms x64
    VS_GLOBAL_UapAppxPackageBuildMode SideloadOnly
)
