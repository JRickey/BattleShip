#include "native_dialog.h"
#include "port_log.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shlwapi.h>
#endif

namespace ssb64 {

namespace {

#ifndef _WIN32
// popen/pclose helper: run the command, capture the first non-empty line of
// stdout, return it (trimmed of trailing newline). Returns "" on any error.
std::string RunAndCaptureFirstLine(const std::string& cmd) {
    FILE* f = popen(cmd.c_str(), "r");
    if (!f) return {};
    std::string line;
    char buf[1024];
    while (std::fgets(buf, sizeof(buf), f) != nullptr) {
        line = buf;
        // Strip trailing newline / CR.
        while (!line.empty() &&
               (line.back() == '\n' || line.back() == '\r')) {
            line.pop_back();
        }
        if (!line.empty()) break;
    }
    int rc = pclose(f);
    if (rc != 0) return {};
    return line;
}

// Quote a string for embedding inside an AppleScript double-quoted literal.
// AppleScript only requires escaping " and \.
std::string AppleScriptQuote(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    return out;
}

// Quote a string for embedding inside a single-quoted shell argument:
// close the quote, escape, reopen.
std::string ShellSingleQuote(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
}
#endif // !_WIN32

} // namespace

std::string OpenFileDialog(const std::string& title,
                           const std::vector<std::string>& extensions) {
#ifdef __APPLE__
    // Build a `{"z64","n64","v64"}` extension list.
    std::string typesList;
    for (size_t i = 0; i < extensions.size(); ++i) {
        if (i) typesList += ", ";
        typesList += "\"" + extensions[i] + "\"";
    }

    std::string script = "choose file with prompt \"" +
                         AppleScriptQuote(title) + "\"";
    if (!typesList.empty()) {
        script += " of type {" + typesList + "}";
    }
    // POSIX path of (choose file ...) — gives a Unix path without "Macintosh HD:..."
    script = "POSIX path of (" + script + ")";

    std::string cmd = "osascript -e " + ShellSingleQuote(script) + " 2>/dev/null";
    return RunAndCaptureFirstLine(cmd);

#elif defined(__linux__) || defined(__unix__)
    // Inside an AppImage, our AppRun sets LD_LIBRARY_PATH to the
    // bundled $APPDIR/usr/lib so the game's own .so deps resolve
    // against versions matching the build host. That same path
    // poisons system tools like zenity / kdialog which load the
    // distro's libssl/libcrypto/libcurl/etc. — the bundled
    // libcrypto.so.3 from the build host is older than the system
    // libssl.so.3 expects, so the dialog dies with `version
    // OPENSSL_3.X.0 not found` before it can show a window. Strip
    // LD_LIBRARY_PATH (and LD_PRELOAD, for symmetry) on the spawned
    // dialog process so it only sees the system loader path.
    constexpr const char* env_strip =
        "env -u LD_LIBRARY_PATH -u LD_PRELOAD ";

    // zenity first.
    {
        std::string cmd = env_strip;
        cmd += "zenity --file-selection --title=" +
               ShellSingleQuote(title);
        if (!extensions.empty()) {
            std::string filter = "--file-filter=";
            for (size_t i = 0; i < extensions.size(); ++i) {
                filter += (i ? " " : "") + std::string("*.") + extensions[i];
            }
            cmd += " " + ShellSingleQuote(filter);
        }
        cmd += " 2>/dev/null";
        std::string r = RunAndCaptureFirstLine(cmd);
        if (!r.empty()) return r;
    }
    // Fall back to kdialog if zenity isn't installed or returned empty.
    {
        std::string cmd = env_strip;
        cmd += "kdialog --getopenfilename ~ ";
        if (!extensions.empty()) {
            std::string filter;
            for (size_t i = 0; i < extensions.size(); ++i) {
                filter += (i ? " " : "") + std::string("*.") + extensions[i];
            }
            filter += "|" + title;
            cmd += ShellSingleQuote(filter);
        }
        cmd += " 2>/dev/null";
        return RunAndCaptureFirstLine(cmd);
    }

#elif defined(_WIN32)
    // Build a Windows commdlg filter:  "Title\0*.z64;*.n64;*.v64\0\0".
    std::wstring filterDesc;
    for (wchar_t c : title) filterDesc.push_back(c);
    std::wstring filterPat;
    for (size_t i = 0; i < extensions.size(); ++i) {
        if (i) filterPat += L";";
        filterPat += L"*.";
        for (char c : extensions[i]) filterPat.push_back((wchar_t)c);
    }
    if (filterPat.empty()) filterPat = L"*.*";

    std::wstring filter = filterDesc + L'\0' + filterPat + L'\0' + L'\0';

    wchar_t fileBuf[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = filter.c_str();
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = MAX_PATH;
    std::wstring titleW;
    for (char c : title) titleW.push_back((wchar_t)c);
    ofn.lpstrTitle = titleW.c_str();
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameW(&ofn)) {
        return {};
    }
    // UTF-16 -> UTF-8 conversion for the returned path.
    int len = WideCharToMultiByte(CP_UTF8, 0, fileBuf, -1, nullptr, 0,
                                  nullptr, nullptr);
    if (len <= 0) return {};
    std::string out(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, fileBuf, -1, out.data(), len,
                        nullptr, nullptr);
    return out;

#elif defined(__SWITCH__)
    // Switch has no native file dialog. The first-run wizard falls back
    // to manual path entry; the Browse button is simply a no-op.
    return {};
#else
    (void)title;
    (void)extensions;
    return {};
#endif
}

} // namespace ssb64
