#pragma once

#include <string>

namespace ssb64 {

// Resolve the directory containing this process's executable.
//
// Ship::Context::GetAppBundlePath() on Linux/Windows + NON_PORTABLE
// returns the literal CMAKE_INSTALL_PREFIX, which is wrong for any
// distribution that doesn't install to that prefix (AppImage, Windows
// portable zip, dev cmake builds). This wrapper queries the OS for the
// actual binary location:
//   Linux   — readlink /proc/self/exe → parent dir
//   Windows — GetModuleFileNameW(NULL) → parent dir
//   macOS   — NSBundle resourcePath via Ship::Context (already correct
//             for both .app bundles and ad-hoc binaries)
// Falls back to Ship::Context::GetAppBundlePath() if the OS query fails.
std::string RealAppBundlePath();

// Probe the LUS app-data directory, then the real executable/bundle
// directory, for rel_path. Returns the first existing path (normalized),
// or "" when the file exists in neither. This is the shared probe order
// for optional runtime files (see PortLocateFile in port.cpp for the
// variant that falls back to "./<name>" for open-and-report-error flows).
std::string LocateExistingFile(const std::string& rel_path);

} // namespace ssb64
