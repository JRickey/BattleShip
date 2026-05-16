#include "enhancements.h"

#include "port_log.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include <nlohmann/json.hpp>

#if defined(__linux__)
#include <sys/stat.h>
#endif

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

namespace ssb64 {
namespace enhancements {

// We use atomics for the state flags so the UI thread doesn't have to lock a mutex 60 times a second!
static std::atomic<bool> s_updateChecked{false};
static std::atomic<bool> s_updateAvailable{false};
static std::atomic<bool> s_isDownloading{false};
static std::atomic<bool> s_downloadComplete{false};
static std::atomic<bool> s_isCheckingForUpdates{false};
static std::atomic<bool> s_updateCheckFailed{false};

static std::string s_latestVersion = "";
static std::string s_downloadUrl = "";
static std::string s_updateStatus = "";
static std::string s_downloadStatus = "";
static std::mutex s_stringMutex; // Only locks when reading/writing the actual text

namespace {

#ifndef BATTLESHIP_CURRENT_VERSION
#define BATTLESHIP_CURRENT_VERSION "v1.0.0"
#endif

constexpr const char* kLatestReleaseApi =
    "https://api.github.com/repos/JRickey/BattleShip/releases/latest";
constexpr const char* kReleasePageUrl =
    "https://github.com/JRickey/BattleShip/releases/latest";

#if defined(__linux__)
constexpr const char* kPlatformAssetName = "BattleShip-x86_64.AppImage";
#elif defined(_WIN32)
constexpr const char* kPlatformAssetName = "BattleShip-windows.zip";
#elif defined(__APPLE__)
constexpr const char* kPlatformAssetName = "BattleShip.dmg";
#else
constexpr const char* kPlatformAssetName = "";
#endif

#if !defined(_WIN32)
std::string ShellSingleQuote(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out += c;
        }
    }
    out += "'";
    return out;
}
#endif

void SetUpdateStatus(const std::string& status) {
    std::lock_guard<std::mutex> lock(s_stringMutex);
    s_updateStatus = status;
}

void SetDownloadStatus(const std::string& status) {
    std::lock_guard<std::mutex> lock(s_stringMutex);
    s_downloadStatus = status;
}

#if defined(_WIN32)
// Run a child process with stdout/stderr redirected and no console window.
// Required on Windows because _popen() / system() shell through cmd.exe;
// when called from a /SUBSYSTEM:WINDOWS binary, that creates a visible console.
int RunCaptureNoWindow(std::string cmd,
                       const std::function<void(const std::string&)>& onLine) {
    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &sa, 0)) return -1;
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = writePipe;
    si.hStdError = writePipe;
    si.hStdInput = nullptr;

    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, TRUE,
                             CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(writePipe);
    if (!ok) {
        CloseHandle(readPipe);
        return -1;
    }

    std::string line;
    char buf[512];
    DWORD got = 0;
    while (ReadFile(readPipe, buf, sizeof(buf), &got, nullptr) && got > 0) {
        for (DWORD i = 0; i < got; ++i) {
            char c = buf[i];
            if (c == '\r' || c == '\n') {
                if (!line.empty()) {
                    onLine(line);
                    line.clear();
                }
            } else {
                line += c;
            }
        }
    }
    if (!line.empty()) onLine(line);
    CloseHandle(readPipe);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return static_cast<int>(code);
}
#endif

int RunCapture(std::string cmd, const std::function<void(const std::string&)>& onLine) {
#ifdef __SWITCH__
    // No point in trying to capture output on Switch.
    return 0;
#elif defined(_WIN32)
    return RunCaptureNoWindow(std::move(cmd), onLine);
#else
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return -1;

    int c;
    std::string currentLine;
    while ((c = fgetc(pipe)) != EOF) {
        if (c == '\r' || c == '\n') {
            if (!currentLine.empty()) onLine(currentLine);
            currentLine.clear();
        } else {
            currentLine += static_cast<char>(c);
        }
    }
    if (!currentLine.empty()) onLine(currentLine);
    return pclose(pipe);
#endif
}

std::string CurlCommandPrefix() {
#if defined(__linux__)
    // AppRun prepends bundled AppImage libs for the game itself. Those libs
    // poison distro tools like curl/xdg-open, so strip loader overrides here.
    return "env -u LD_LIBRARY_PATH -u LD_PRELOAD curl";
#else
    return "curl";
#endif
}

std::string BuildCheckCommand() {
#if defined(_WIN32)
    return std::string("curl -fLsS -m 10 -H \"User-Agent: BattleShip-Updater\" ") +
           "\"" + kLatestReleaseApi + "\"";
#else
    return CurlCommandPrefix() +
           " -fLsS -m 10 -H " + ShellSingleQuote("User-Agent: BattleShip-Updater") +
           " " + ShellSingleQuote(kLatestReleaseApi) + " 2>&1";
#endif
}

std::string BuildDownloadCommand(const std::string& destPath, const std::string& url) {
#if defined(_WIN32)
    return "curl -fL -# -o \"" + destPath + "\" \"" + url + "\"";
#else
    return CurlCommandPrefix() + " -fL -# -o " + ShellSingleQuote(destPath) +
           " " + ShellSingleQuote(url) + " 2>&1";
#endif
}

bool FindPlatformAssetUrl(const nlohmann::json& release, std::string* outUrl) {
    if (!release.contains("assets") || !release["assets"].is_array()) return false;

    for (const auto& asset : release["assets"]) {
        if (!asset.contains("name") || !asset.contains("browser_download_url")) continue;
        if (!asset["name"].is_string() || !asset["browser_download_url"].is_string()) continue;
        if (asset["name"].get<std::string>() == kPlatformAssetName) {
            *outUrl = asset["browser_download_url"].get<std::string>();
            return true;
        }
    }
    return false;
}

void ResetDownloadStateForNewCheck() {
    s_downloadComplete.store(false);
    SetDownloadStatus("");
}

} // namespace

void CheckForUpdatesAsync(bool force) {
#ifndef __SWITCH__
    // Check our atomic flags (no locks required)
    if (s_isCheckingForUpdates.load() || s_isDownloading.load()) return;
    if (!force && s_updateChecked.load()) return;

    s_updateChecked.store(true);
    s_isCheckingForUpdates.store(true);
    s_updateCheckFailed.store(false);
    s_updateAvailable.store(false);
    ResetDownloadStateForNewCheck();
    SetUpdateStatus("");

    std::thread([]() {
        std::string response;
        int exitCode = RunCapture(BuildCheckCommand(), [&](const std::string& line) {
            response += line;
        });
        if (exitCode != 0) {
            s_updateCheckFailed.store(true);
            SetUpdateStatus("Unable to check for updates.");
            s_isCheckingForUpdates.store(false);
            return;
        }

        if (!response.empty()) {
            try {
                auto release = nlohmann::json::parse(response);
                if (!release.contains("tag_name") || !release["tag_name"].is_string()) {
                    throw std::runtime_error("latest release missing tag_name");
                }

                std::string latestTag = release["tag_name"].get<std::string>();
                {
                    std::lock_guard<std::mutex> lock(s_stringMutex);
                    s_latestVersion = latestTag;
                    s_downloadUrl.clear();
                }

                if (latestTag != BATTLESHIP_CURRENT_VERSION) {
                    std::string assetUrl;
                    if (!FindPlatformAssetUrl(release, &assetUrl)) {
                        s_updateCheckFailed.store(true);
                        SetUpdateStatus("Latest release has no update for this platform.");
                    } else {
                        {
                            std::lock_guard<std::mutex> lock(s_stringMutex);
                            s_downloadUrl = assetUrl;
                        }
                        s_updateAvailable.store(true);
                        SetUpdateStatus("");
                    }
                } else {
                    SetUpdateStatus("Up to date.");
                }
            } catch (...) {
                s_updateCheckFailed.store(true);
                SetUpdateStatus("Unable to read update information.");
            }
        } else {
            s_updateCheckFailed.store(true);
            SetUpdateStatus("Unable to check for updates.");
        }

        s_isCheckingForUpdates.store(false);
    }).detach();
#endif
}

void StartGameUpdate() {
#ifndef __SWITCH__
    if (s_isDownloading.load()) return;
    s_isDownloading.store(true);

    {
        std::lock_guard<std::mutex> lock(s_stringMutex);
        if (s_downloadUrl.empty()) {
            s_downloadStatus = "Error: No update download available.";
            s_isDownloading.store(false);
            return;
        }
        s_downloadStatus = "Initializing download...";
    }

    std::thread([]() {
        std::string url;
        {
            std::lock_guard<std::mutex> lock(s_stringMutex);
            url = s_downloadUrl;
        }

        #if defined(__linux__)
        const char* appImagePath = getenv("APPIMAGE");
        if (!appImagePath) {
            {
                std::lock_guard<std::mutex> lock(s_stringMutex);
                s_downloadStatus = "Error: Not running as AppImage";
            }
            s_isDownloading.store(false);
            return;
        }

        std::string tempPath = std::string(appImagePath) + ".part";
        std::string cmd = BuildDownloadCommand(tempPath, url);

        #elif defined(_WIN32)
        std::string tempZip = "update_temp.zip";
        std::string cmd = BuildDownloadCommand(tempZip, url);

        #elif defined(__APPLE__)
        std::string tempDmg = "/tmp/BattleShip_Update.dmg";
        std::string cmd = BuildDownloadCommand(tempDmg, url);
        #endif

        auto onProgressLine = [](const std::string& line) {
            if (line.find('%') == std::string::npos) return;
            std::string pctStr;
            for (char ch : line) {
                if ((ch >= '0' && ch <= '9') || ch == '.') {
                    pctStr += ch;
                }
            }
            if (!pctStr.empty()) {
                SetDownloadStatus("Downloading... " + pctStr + "%");
            }
        };

        int exitCode = RunCapture(cmd, onProgressLine);

        if (exitCode == 0) {
            #if defined(__linux__)
            chmod(tempPath.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
            if (std::rename(tempPath.c_str(), appImagePath) == 0) {
                {
                    std::lock_guard<std::mutex> lock(s_stringMutex);
                    s_downloadStatus = "Update complete! Please restart the game.";
                }
                s_downloadComplete.store(true);
            } else {
                std::lock_guard<std::mutex> lock(s_stringMutex);
                s_downloadStatus = "Error: Failed to replace AppImage.";
            }
            #elif defined(_WIN32)
            FILE* bat = fopen("update_game.bat", "w");
            if (bat) {
                fprintf(bat, "@echo off\n");
                fprintf(bat, "echo Update downloaded! Waiting for BattleShip to close before applying...\n");
                fprintf(bat, ":wait\n");
                fprintf(bat, "tasklist /FI \"IMAGENAME eq BattleShip.exe\" 2>NUL | find /I /N \"BattleShip.exe\">NUL\n");
                fprintf(bat, "if \"%%ERRORLEVEL%%\"==\"0\" (\n");
                fprintf(bat, "    timeout /t 1 /nobreak > NUL\n");
                fprintf(bat, "    goto wait\n");
                fprintf(bat, ")\n");
                fprintf(bat, "echo Installing update...\n");
                fprintf(bat, "tar -xf update_temp.zip\n");
                fprintf(bat, "del update_temp.zip\n");
                fprintf(bat, "start BattleShip.exe\n");
                fprintf(bat, "(goto) 2>nul & del \"%%~f0\"\n");
                fclose(bat);

                {
                    std::lock_guard<std::mutex> lock(s_stringMutex);
                    s_downloadStatus = "Update ready! Close the game to apply.";
                }
                s_downloadComplete.store(true);
                ShellExecuteA(nullptr, "open", "update_game.bat", nullptr, nullptr, SW_SHOWNORMAL);
            } else {
                std::lock_guard<std::mutex> lock(s_stringMutex);
                s_downloadStatus = "Error: Failed to create updater script.";
            }
            #elif defined(__APPLE__)
            {
                std::lock_guard<std::mutex> lock(s_stringMutex);
                s_downloadStatus = "Download complete! Opening DMG...";
            }
            s_downloadComplete.store(true);
            std::string openCmd = "open \"" + tempDmg + "\"";
            system(openCmd.c_str());
            #endif
        } else {
            {
                std::lock_guard<std::mutex> lock(s_stringMutex);
                s_downloadStatus = "Error: Download failed.";
            }
            #if defined(__linux__)
            std::remove(tempPath.c_str());
            #elif defined(_WIN32)
            std::remove(tempZip.c_str());
            #elif defined(__APPLE__)
            std::remove(tempDmg.c_str());
            #endif
        }

        s_isDownloading.store(false);
    }).detach();
#endif
}

// OS-aware URL opener for Windows and Mac fallback
void OpenReleasePage() {
#ifndef __SWITCH__

    const char* url = kReleasePageUrl;

    #if defined(_WIN32)
    ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
    #elif defined(__APPLE__)
    std::string cmd = std::string("open ") + url;
    system(cmd.c_str());
    #else
    std::string cmd = std::string("env -u LD_LIBRARY_PATH -u LD_PRELOAD xdg-open ") +
                      ShellSingleQuote(url) + " &";
    system(cmd.c_str());
    #endif
#endif
}

// Atomic reads - no locks required for the UI thread!
bool IsCheckingForUpdates() { return s_isCheckingForUpdates.load(); }
bool IsUpdateAvailable() { return s_updateAvailable.load(); }
bool DidUpdateCheckFail() { return s_updateCheckFailed.load(); }
bool IsDownloading() { return s_isDownloading.load(); }
bool IsDownloadComplete() { return s_downloadComplete.load(); }

// String getters still need the lock, but they are incredibly fast now
std::string GetUpdateStatus() { std::lock_guard<std::mutex> lock(s_stringMutex); return s_updateStatus; }
std::string GetDownloadStatus() { std::lock_guard<std::mutex> lock(s_stringMutex); return s_downloadStatus; }
std::string GetLatestVersion() { std::lock_guard<std::mutex> lock(s_stringMutex); return s_latestVersion; }

} // namespace enhancements
} // namespace ssb64
