#include "first_run.h"
#include "app_paths.h"
#include "native_dialog.h"
#include "port_log.h"

#include <libultraship/libultraship.h>
#include <fast/Fast3dWindow.h>
#include <ship/window/FileDropMgr.h>
#include <ship/window/gui/Gui.h>

#include <SDL2/SDL.h>
#include <imgui.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace ssb64 {

namespace {

// Search candidates in order; return the first that exists.
std::string FindExisting(const std::vector<std::string>& candidates) {
    for (const auto& c : candidates) {
        if (!c.empty() && fs::exists(c)) {
            return c;
        }
    }
    return {};
}

std::string TryOpenLogPath(const fs::path& candidate) {
    if (candidate.empty()) {
        return {};
    }

    std::error_code ec;
    if (candidate.has_parent_path()) {
        fs::create_directories(candidate.parent_path(), ec);
    }

    std::ofstream out(candidate, std::ios::app);
    if (!out) {
        return {};
    }

    // Always return an absolute path. RunTorchCommand on POSIX runs the
    // shell with `cd workingDir && torch … > logPath 2>&1`; a relative
    // logPath then resolves against the new cwd (workingDir/torch-work)
    // and the redirect target's parent doesn't exist there, so sh aborts
    // with exit 1 before torch runs. GetPathRelativeToAppDirectory()
    // returns "./logs/…" on macOS in portable mode, which is what
    // tripped this.
    fs::path absolute = fs::absolute(candidate, ec);
    if (ec || absolute.empty()) {
        return candidate.string();
    }
    return absolute.string();
}

void AppendLogLine(const std::string& logPath, const std::string& line) {
    if (logPath.empty()) {
        return;
    }

    std::ofstream out(logPath, std::ios::app);
    if (!out) {
        return;
    }

    out << line << '\n';
}

ExtractionResult BuildFailure(const std::string& error, const std::string& logPath) {
    AppendLogLine(logPath, "ERROR: " + error);
    return { false, {}, error, logPath };
}

std::string QuoteCommandArg(const std::string& arg) {
    std::string quoted = "\"";
    for (char c : arg) {
        if (c == '"') {
            quoted += "\\\"";
        } else {
            quoted += c;
        }
    }
    quoted += "\"";
    return quoted;
}

std::string ResolveLogPath(const std::string& preferredLogPath, const std::string& sourceDir,
                           const std::string& destinationDir) {
    const std::vector<fs::path> candidates = {
        fs::path(preferredLogPath),
        fs::path(destinationDir) / "logs" / "asset-extract.log",
        fs::path(sourceDir) / "logs" / "asset-extract.log",
        fs::current_path() / "logs" / "asset-extract.log",
    };

    for (const auto& candidate : candidates) {
        const std::string resolved = TryOpenLogPath(candidate);
        if (!resolved.empty()) {
            return resolved;
        }
    }

    return {};
}

std::string FindTorchExecutable() {
    std::vector<std::string> candidates;
    for (const auto& base : {
             RealAppBundlePath(),
             // macOS .app: RealAppBundlePath returns Contents/Resources
             // (where assets live); the sidecar torch ships in
             // Contents/MacOS alongside the main binary. On non-bundle
             // layouts this candidate either doesn't exist or aliases
             // the previous one — FindExisting skips harmlessly.
             RealAppBundlePath() + "/../MacOS",
             Ship::Context::GetAppDirectoryPath(),
             std::string("."),
             RealAppBundlePath() + "/..",
         }) {
#ifdef _WIN32
        candidates.push_back(base + "/torch.exe");
#endif
        candidates.push_back(base + "/torch");
    }

    const std::string found = FindExisting(candidates);
    if (found.empty()) {
        return {};
    }

    // Absolute-ize: RunTorchCommand on POSIX runs `cd workingDir &&
    // <torch> ...`, so a relative torch path (e.g. "./torch" from
    // candidate base=".") resolves against the new cwd after cd and
    // sh aborts with exit 127 (32512 from std::system) before torch
    // runs.
    std::error_code ec;
    fs::path absolute = fs::absolute(fs::path(found), ec);
    if (ec || absolute.empty()) {
        return found;
    }
    return absolute.string();
}

bool RunTorchCommand(const std::string& commandLine, const std::string& workingDir,
                     const std::string& logPath, std::string& error) {
#ifdef _WIN32
    if (logPath.empty()) {
        error = "Could not resolve a writable extraction log path";
        return false;
    }

    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE logFile =
        CreateFileA(logPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, nullptr);
    if (logFile == INVALID_HANDLE_VALUE) {
        error = "Could not open extraction log for writing";
        return false;
    }

    SetFilePointer(logFile, 0, nullptr, FILE_END);

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = logFile;
    si.hStdError = logFile;

    PROCESS_INFORMATION pi = {};
    std::vector<char> mutableCmd(commandLine.begin(), commandLine.end());
    mutableCmd.push_back('\0');

    std::vector<char> mutableCwd(workingDir.begin(), workingDir.end());
    mutableCwd.push_back('\0');

    const BOOL started =
        CreateProcessA(nullptr, mutableCmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                       mutableCwd.data(), &si, &pi);
    if (!started) {
        const DWORD code = GetLastError();
        CloseHandle(logFile);
        error = "Failed to launch torch (CreateProcess error " + std::to_string(code) + ")";
        return false;
    }

    CloseHandle(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(logFile);

    if (exitCode != 0) {
        error = "Torch exited with code " + std::to_string(exitCode);
        return false;
    }
    return true;
#else
    const std::string shellCommand =
        "cd " + QuoteCommandArg(workingDir) + " && " + commandLine + " > " + QuoteCommandArg(logPath) + " 2>&1";
    const int exitCode = std::system(shellCommand.c_str());
    if (exitCode != 0) {
        error = "Torch exited with code " + std::to_string(exitCode);
        return false;
    }
    return true;
#endif
}

// Locate the directory that contains config.yml (the integrated extraction
// config plus yamls/us/*.yml recipes used by standalone Torch.
std::string FindAssetConfigDir() {
    std::vector<fs::path> roots = {
        fs::path(RealAppBundlePath()),
        fs::current_path(),
    };

    for (const auto& root : roots) {
        fs::path dir = root;
        while (!dir.empty()) {
            if (fs::exists(dir / "config.yml") && fs::exists(dir / "yamls" / "us")) {
                return dir.string();
            }

            const fs::path parent = dir.parent_path();
            if (parent.empty() || parent == dir) {
                break;
            }
            dir = parent;
        }
    }

    return {};
}

std::string FindBaseRom() {
    const std::string appData = Ship::Context::GetAppDirectoryPath();
    const std::string bundle = RealAppBundlePath();
    std::vector<std::string> candidates;
    for (const auto& base : {appData, bundle, bundle + "/..", std::string(".")}) {
        for (const char* ext : {"z64", "n64", "v64"}) {
            candidates.push_back(base + "/baserom.us." + ext);
        }
    }

    const std::string found = FindExisting(candidates);
    if (found.empty()) {
        return {};
    }

    std::error_code ec;
    const fs::path absolute = fs::absolute(fs::path(found), ec);
    if (!ec) {
        return absolute.string();
    }

    return found;
}

void DrawWizardFrame(const std::function<void()>& drawContents) {
    auto context = Ship::Context::GetInstance();
    auto window = context->GetWindow();
    auto gui = window->GetGui();

    window->HandleEvents();
    gui->StartDraw();
    window->StartFrame();
    drawContents();
    window->RunGuiOnly();
    gui->EndDraw();
    window->EndFrame();
}

} // namespace

ExtractionResult ExtractAssetsIfNeeded(const std::string& target_o2r_path, bool silent,
                                       const std::string& romOverridePath) {
    std::error_code ec;
    const fs::path absoluteTargetPath = fs::absolute(fs::path(target_o2r_path), ec);
    const fs::path targetPath = ec ? fs::path(target_o2r_path) : absoluteTargetPath;
    if (fs::exists(targetPath)) {
        return { true, targetPath.string(), {}, {} };
    }

#ifdef __SWITCH__
    // On Switch, assets must be pre-extracted on a PC and placed alongside
    // the .nro on the SD card. Torch cannot run on-device.
    port_log("first_run: %s missing on Switch — place pre-extracted .o2r on SD card\n",
             target_o2r_path.c_str());
    return { false, {}, "Assets not found — pre-extract on PC with 'torch o2r baserom.us.z64' and copy .o2r to sdmc:/switch/BattleShip/", {} };
#endif

    port_log("first_run: %s missing — running asset extraction\n",
             target_o2r_path.c_str());

    std::string rom = romOverridePath.empty() ? FindBaseRom() : romOverridePath;
    if (rom.empty()) {
        const std::string appData = Ship::Context::GetAppDirectoryPath();
        port_log("first_run: ERROR no ROM found.\n"
                 "  Drop a baserom.us.{z64,n64,v64} into %s\n",
                 appData.c_str());
        const std::string msg =
            "Asset extraction needs your Super Smash Bros. NTSC-U v1.0 ROM.\n\n"
            "Place a baserom.us.z64 (or .n64 / .v64) into:\n  " + appData +
            "\n\nThen launch the game again.";
        if (!silent) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                     "ROM not found", msg.c_str(), nullptr);
        }
        return { false, {}, "ROM not found", {} };
    }

    ec.clear();
    const fs::path absoluteRomPath = fs::absolute(fs::path(rom), ec);
    if (!ec) {
        rom = absoluteRomPath.string();
    }
    port_log("first_run: using ROM %s\n", rom.c_str());

    const std::string cfgDir = FindAssetConfigDir();
    if (cfgDir.empty()) {
        port_log("first_run: ERROR could not locate config.yml + yamls/us/\n");
        return { false, {}, "Could not locate config.yml + yamls/us", {} };
    }
    port_log("first_run: asset config dir -> %s\n", cfgDir.c_str());

    const std::string torchExe = FindTorchExecutable();
    if (torchExe.empty()) {
        port_log("first_run: ERROR could not locate torch executable\n");
        return { false, {}, "Could not locate torch executable", {} };
    }
    port_log("first_run: torch executable -> %s\n", torchExe.c_str());

    fs::create_directories(targetPath.parent_path());
    const fs::path workDir = fs::absolute(targetPath.parent_path() / "torch-work");
    fs::remove_all(workDir, ec);
    ec.clear();
    fs::create_directories(workDir, ec);
    if (ec) {
        port_log("first_run: ERROR could not create %s: %s\n", workDir.string().c_str(), ec.message().c_str());
        return { false, {}, "Could not create torch work directory: " + ec.message(), {} };
    }
    fs::copy_file(fs::path(cfgDir) / "config.yml",
                  workDir / "config.yml",
                  fs::copy_options::overwrite_existing, ec);
    if (ec) {
        port_log("first_run: ERROR could not stage config.yml: %s\n", ec.message().c_str());
        return { false, {}, "Could not stage config.yml: " + ec.message(), {} };
    }

    fs::copy(fs::path(cfgDir) / "yamls", workDir / "yamls",
             fs::copy_options::recursive |
                 fs::copy_options::overwrite_existing,
             ec);
    if (ec) {
        port_log("first_run: ERROR could not stage yamls/: %s\n", ec.message().c_str());
        return { false, {}, "Could not stage yamls/: " + ec.message(), {} };
    }
    const std::string runDir = workDir.string();
    port_log("first_run: torch work dir -> %s\n", runDir.c_str());

    const std::string preferredLogPath = Ship::Context::GetPathRelativeToAppDirectory("logs/asset-extract.log");
    const std::string logPath = ResolveLogPath(preferredLogPath, cfgDir, runDir);
    if (!logPath.empty()) {
        std::ofstream truncate(logPath, std::ios::trunc);
    }

    const std::string commandLine = QuoteCommandArg(torchExe) + " o2r " + QuoteCommandArg(rom) +
                                    " -s " + QuoteCommandArg(runDir) + " -d " + QuoteCommandArg(runDir);
    port_log("first_run: > %s\n", commandLine.c_str());
    AppendLogLine(logPath, "Command: " + commandLine);

    std::string commandError;
    if (!RunTorchCommand(commandLine, runDir, logPath, commandError)) {
        port_log("first_run: ERROR extractor failed: %s\n", commandError.c_str());
        const std::string msg =
            "BattleShip failed to extract assets from your ROM.\n\n"
            "Error:\n  " + commandError +
            "\n\nCheck the extraction log for details:\n  " + logPath +
            "\n\nThe most common cause is a non-NTSC-U-v1.0 ROM. Verify your "
            "dump's SHA-1 matches a supported hash printed in that log.";
        if (!silent) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                     "Asset extraction failed", msg.c_str(),
                                     nullptr);
        }
        return BuildFailure(commandError, logPath);
    }

    const std::string emitted = (workDir / "BattleShip.o2r").string();
    if (!fs::exists(emitted)) {
        port_log("first_run: ERROR extractor reported success but %s is missing\n",
                 emitted.c_str());
        return { false, {}, "Torch reported success but BattleShip.o2r is missing", logPath };
    }

    fs::rename(emitted, targetPath, ec);
    if (ec) {
        ec.clear();
        fs::copy_file(emitted, targetPath, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            port_log("first_run: ERROR failed to install %s -> %s: %s\n", emitted.c_str(),
                     targetPath.string().c_str(), ec.message().c_str());
            return { false, {}, "Failed to install BattleShip.o2r: " + ec.message(), logPath };
        }
    }

    fs::remove_all(workDir, ec);

    port_log("first_run: extracted BattleShip.o2r -> %s\n", targetPath.string().c_str());
    return { true, targetPath.string(), {}, logPath };
}

bool RunFirstRunWizard(const std::string& target_o2r_path) {
    port_log("first_run: launching ImGui wizard\n");

#ifdef __SWITCH__
    // Switch cannot run Torch on-device. Assets must be pre-extracted on a PC
    // and placed alongside the .nro on the SD card. Show a clean error and exit.
    // Write error to ssb64.log (SDL message boxes don't work on Switch).
    port_log("first_run: assets not found — place pre-extracted .o2r on SD card\n");
    port_log("  Run on PC: torch o2r baserom.us.z64\n");
    port_log("  Then copy .o2r files to sdmc:/switch/BattleShip/\n");
    return false;
#endif

    auto context = Ship::Context::GetInstance();
    auto window = context->GetWindow();
    if (!window || !window->GetGui()) {
        port_log("first_run: ERROR wizard requires Window+Gui already up\n");
        return false;
    }

    const std::string appData = Ship::Context::GetAppDirectoryPath();

    // 256 char ImGui input buffer. Pre-fill with the conventional path the
    // user is most likely to try first; they can edit/replace freely.
    char romPath[1024] = {0};
    std::snprintf(romPath, sizeof(romPath), "%s/baserom.us.z64",
                  appData.c_str());

    enum class State { WaitingForRom, Extracting, Done, Cancelled };
    State state = State::WaitingForRom;
    std::string statusMsg;

    // Test hook: SSB64_WIZARD_AUTOCANCEL=N cancels after the Nth frame.
    // Lets CI / manual smoke tests exercise the cancel exit path without
    // having to drive the ImGui modal interactively.
    int autoCancelFrame = -1;
    if (const char* env = std::getenv("SSB64_WIZARD_AUTOCANCEL")) {
        autoCancelFrame = std::atoi(env);
    }
    int frameCount = 0;

    auto fdm = context->GetFileDropMgr();

    /* Background extraction is driven by a std::async future so the wizard
     * keeps rendering while extraction runs. The future is set
     * when the user clicks Extract; each frame we poll wait_for(0) to see
     * if it's done. While the future is pending, the modal renders an
     * indeterminate progress bar animated on frameCount. */
    std::future<ExtractionResult> extractFuture;
    bool extractRunning = false;

    /* Register a "consume everything" drop handler for the duration of
     * the wizard.  FileDropMgr::CallHandlers fires synchronously from
     * the SDL_DROPFILE event in the SDL2 backend; if no handler claims
     * the drop (returns true), it falls through to
     *   gui->GetGameOverlay()->TextDrawNotification("Unsupported file
     *                                                dropped, ignoring")
     * which queues an overlay notification.  GameOverlay's font hasn't
     * been initialized yet (Gui::Init / GameOverlay::Init normally runs
     * during the first game frame), so the notification's later
     * ImGui::PushFont(nullptr) deref crashes once the user clicks
     * Extract and another frame draws.
     *
     * We register and unregister around the wizard so this only changes
     * behavior here — outside the wizard, drops still surface the
     * standard "unsupported" toast. */
    using DropFunc = bool (*)(char*);
    DropFunc consumer = [](char*) -> bool { return true; };
    if (fdm) {
        fdm->RegisterDropHandler(consumer);
    }

    while (state != State::Done && state != State::Cancelled) {
        if (!window->IsRunning()) {
            state = State::Cancelled;
            break;
        }
        if (autoCancelFrame >= 0 && frameCount++ >= autoCancelFrame) {
            port_log("first_run: SSB64_WIZARD_AUTOCANCEL fired at frame %d\n",
                     frameCount);
            state = State::Cancelled;
            break;
        }

        // Drag-drop integration: if the user dropped a file on the window
        // since last frame, paste its path into the input. SDL already
        // routes SDL_DROPFILE events to FileDropMgr via Fast3dWindow's
        // event handler; we just consume them here.
        if (fdm && fdm->FileDropped()) {
            if (const char* p = fdm->GetDroppedFile()) {
                std::snprintf(romPath, sizeof(romPath), "%s", p);
                statusMsg.clear();
                port_log("first_run: drag-drop -> %s\n", p);
            }
            fdm->ClearDroppedFile();
        }

        DrawWizardFrame([&] {
            ImGui::OpenPopup("First-run setup");

            const ImVec2 viewportCenter =
                ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Always,
                                    ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(620, 0));

            if (ImGui::BeginPopupModal("First-run setup", nullptr,
                                       ImGuiWindowFlags_AlwaysAutoResize |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoSavedSettings)) {
                ImGui::TextWrapped(
                    "BattleShip needs to extract assets from your "
                    "Nintendo 64 ROM before it can launch.");
                ImGui::Spacing();
                ImGui::TextWrapped(
                    "Required: a Super Smash Bros. (NTSC-U v1.0) dump in "
                    ".z64, .n64, or .v64 format.");
                ImGui::Separator();

                ImGui::Text("ROM path:");
                // InputText takes most of the row, Browse takes the right
                // edge. -100 reserves room for a 90 px button + padding.
                ImGui::SetNextItemWidth(-100);
                ImGui::InputText("##rompath", romPath, sizeof(romPath));
                ImGui::SameLine();
                if (ImGui::Button("Browse...", ImVec2(90, 0))) {
                    std::string picked = ssb64::OpenFileDialog(
                        "Select your Super Smash Bros. ROM",
                        {"z64", "n64", "v64"});
                    if (!picked.empty()) {
                        std::snprintf(romPath, sizeof(romPath), "%s",
                                      picked.c_str());
                        statusMsg.clear();
                        port_log("first_run: native picker -> %s\n",
                                 picked.c_str());
                    }
                }

                ImGui::TextDisabled(
                    "Tip: drag a .z64/.n64/.v64 onto the window, or click");
                ImGui::TextDisabled(
                    "     Browse... and pick your ROM dump.");
                ImGui::Spacing();

                if (state == State::Extracting) {
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
                                       "Extracting assets — please wait...");
                    // Indeterminate progress bar. Extraction does not emit
                    // progress callbacks, so we animate a marquee-style
                    // fraction that loops every ~2 s. ImGui doesn't
                    // ship a marquee helper; pass a fraction in [0,1]
                    // and an empty overlay string with a sliding
                    // SetNextItemWidth to fake it.
                    const float t = (frameCount % 120) / 120.0f;
                    ImGui::ProgressBar(t, ImVec2(-1, 0), "Extracting BattleShip.o2r...");
                } else if (!statusMsg.empty()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                                       "%s", statusMsg.c_str());
                }

                ImGui::Spacing();

                const bool busy = (state == State::Extracting);
                ImGui::BeginDisabled(busy);

                if (ImGui::Button("Extract", ImVec2(120, 0))) {
                    if (!fs::exists(romPath)) {
                        statusMsg = "ROM not found at that path.";
                    } else {
                        state = State::Extracting;
                        statusMsg.clear();
                        std::string target = target_o2r_path;
                        std::string selectedRom = romPath;
                        extractFuture = std::async(std::launch::async, [target, selectedRom]() {
                            return ExtractAssetsIfNeeded(target, true, selectedRom);
                        });
                        extractRunning = true;
                    }
                }

                ImGui::SameLine();
                if (ImGui::Button("Quit", ImVec2(120, 0))) {
                    state = State::Cancelled;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndDisabled();
                ImGui::EndPopup();
            }
        });

        // Poll the background extraction. wait_for(0) returns ready
        // immediately if the thread finished since last frame.
        if (extractRunning && extractFuture.valid()) {
            auto status =
                extractFuture.wait_for(std::chrono::milliseconds(0));
            if (status == std::future_status::ready) {
                auto result = extractFuture.get();
                extractRunning = false;
                if (result.success) {
                    state = State::Done;
                } else {
                    state = State::WaitingForRom;
                    statusMsg = "Extraction failed";
                    if (!result.error.empty()) {
                        statusMsg += ": " + result.error;
                    }
                    if (!result.logPath.empty()) {
                        statusMsg += "\nLog: " + result.logPath;
                    }
                }
            }
        }
    }
    // If we're exiting with a future still in flight (cancel during
    // extraction), block on it so the thread doesn't outlive the
    // future's stored state.
    if (extractFuture.valid()) {
        extractFuture.wait();
    }

    if (fdm) {
        fdm->UnregisterDropHandler(consumer);
    }

    if (state == State::Cancelled) {
        port_log("first_run: wizard cancelled by user\n");
        return false;
    }
    port_log("first_run: wizard completed successfully\n");
    return true;
}

} // namespace ssb64
