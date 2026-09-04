#include "app_paths.h"

#include <libultraship/libultraship.h>

#include <filesystem>
#include <system_error>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace ssb64 {

std::string RealAppBundlePath() {
#if defined(__linux__)
    std::error_code ec;
    std::filesystem::path exe = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec && !exe.empty()) {
        return exe.parent_path().string();
    }
#elif defined(_WIN32)
    wchar_t buf[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, buf, MAX_PATH);
    if (len != 0 && len < MAX_PATH) {
        std::filesystem::path exe(buf, buf + len);
        return exe.parent_path().string();
    }
#endif
    return Ship::Context::GetAppBundlePath();
}

std::string LocateExistingFile(const std::string& rel_path) {
    namespace fs = std::filesystem;
    std::error_code ec;

    // noexcept exists(p, ec) throughout — probing a path must never throw
    // (Windows fs::exists can throw on malformed paths, BattleShip issue #58).
    fs::path p1 = fs::path(Ship::Context::GetAppDirectoryPath()) / rel_path;
    if (fs::exists(p1, ec)) {
        return p1.lexically_normal().string();
    }
    fs::path p2 = fs::path(RealAppBundlePath()) / rel_path;
    if (fs::exists(p2, ec)) {
        return p2.lexically_normal().string();
    }
    return std::string();
}

} // namespace ssb64
