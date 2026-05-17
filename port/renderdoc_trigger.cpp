/**
 * renderdoc_trigger.cpp — In-process RenderDoc capture trigger.
 *
 * See renderdoc_trigger.h for the public contract and env var docs.
 *
 * Implementation notes:
 *  - renderdoc_app.h is vendored from the RenderDoc install (MIT licensed).
 *    We do not link against any RenderDoc library — the DLL is loaded at
 *    runtime via LoadLibrary/GetProcAddress. If renderdoc.dll isn't present
 *    (RenderDoc not installed, or not on the DLL search path), we log a
 *    warning and quietly disable capture.
 *  - The init function bails immediately if SSB64_RENDERDOC_FRAMES is unset,
 *    so the dynamic library load happens only when explicitly requested.
 *  - All logging goes through port_log so it lands in ssb64.log alongside
 *    the other port diagnostics.
 */
#include "renderdoc_trigger.h"

#ifndef __SWITCH__

#include "renderdoc_app.h"
#include "port_log.h"

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#elif !defined(__SWITCH__)
  #include <dlfcn.h>
#endif

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <set>
#include <string>

namespace {

RENDERDOC_API_1_4_2 *sRdocApi = nullptr;
std::set<unsigned int> sCaptureFrames;
bool sCaptureAll = false;
bool sInitTried = false;

// Parse "10,55,100" or "10, 55 ,100" into a set<unsigned int>.
// Skips empty entries and non-numeric tokens silently.
std::set<unsigned int> parse_frame_list(const char *s)
{
    std::set<unsigned int> out;
    if (s == nullptr) {
        return out;
    }

    std::string token;
    auto flush = [&out, &token]() {
        if (token.empty()) {
            return;
        }
        // Trim whitespace
        size_t start = token.find_first_not_of(" \t\r\n");
        size_t end = token.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) {
            token.clear();
            return;
        }
        std::string trimmed = token.substr(start, end - start + 1);
        token.clear();

        // Parse unsigned int
        char *endp = nullptr;
        unsigned long v = std::strtoul(trimmed.c_str(), &endp, 10);
        if (endp != nullptr && *endp == '\0') {
            out.insert(static_cast<unsigned int>(v));
        }
    };

    for (const char *p = s; *p != '\0'; ++p) {
        if (*p == ',') {
            flush();
        } else {
            token.push_back(*p);
        }
    }
    flush();
    return out;
}

} // namespace

extern "C" void portRenderDocInit(void)
{
    if (sInitTried) {
        return;
    }
    sInitTried = true;

    const char *frames_env = std::getenv("SSB64_RENDERDOC_FRAMES");
    if (frames_env == nullptr || *frames_env == '\0') {
        // Feature disabled — zero overhead, no logging noise.
        return;
    }

    if (std::strcmp(frames_env, "all") == 0) {
        sCaptureAll = true;
    } else {
        sCaptureFrames = parse_frame_list(frames_env);
        if (sCaptureFrames.empty()) {
            port_log("SSB64: RenderDoc SSB64_RENDERDOC_FRAMES='%s' parsed to empty set; disabling\n",
                     frames_env);
            return;
        }
    }

#ifdef _WIN32
    // If RenderDoc was injected at process launch, the DLL is already loaded.
    // Otherwise try to load it from the default DLL search path, then fall
    // back to the standard installer location. The installer does NOT add
    // the install dir to PATH by default, so the hard-coded fallback catches
    // the common "RenderDoc is installed but PATH isn't set" case.
    HMODULE mod = GetModuleHandleA("renderdoc.dll");
    if (mod == nullptr) {
        mod = LoadLibraryA("renderdoc.dll");
    }
    if (mod == nullptr) {
        // Common install locations. Users can override by adding the
        // install dir to PATH or by injecting RenderDoc via the UI.
        static const char *const kFallbackPaths[] = {
            "C:\\Program Files\\RenderDoc\\renderdoc.dll",
            "C:\\Program Files (x86)\\RenderDoc\\renderdoc.dll",
        };
        for (const char *path : kFallbackPaths) {
            mod = LoadLibraryA(path);
            if (mod != nullptr) {
                port_log("SSB64: RenderDoc loaded from fallback path %s\n", path);
                break;
            }
        }
    }
    if (mod == nullptr) {
        port_log("SSB64: RenderDoc requested but renderdoc.dll not loadable (not on PATH and not in default install dirs)\n");
        return;
    }
    pRENDERDOC_GetAPI getApi =
        reinterpret_cast<pRENDERDOC_GetAPI>(
            reinterpret_cast<void *>(GetProcAddress(mod, "RENDERDOC_GetAPI")));
#elif !defined(__SWITCH__)
    // Linux/macOS: try librenderdoc.so. RTLD_NOLOAD first so we only probe
    // an already-loaded instance, then fall back to a regular load.
    void *mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD);
    if (mod == nullptr) {
        mod = dlopen("librenderdoc.so", RTLD_NOW);
    }
    if (mod == nullptr) {
        port_log("SSB64: RenderDoc requested but librenderdoc.so not loadable\n");
        return;
    }
    pRENDERDOC_GetAPI getApi = reinterpret_cast<pRENDERDOC_GetAPI>(
        dlsym(mod, "RENDERDOC_GetAPI"));
#else
    // Switch: RenderDoc is not available on console.
    return;
#endif

    if (getApi == nullptr) {
        port_log("SSB64: RENDERDOC_GetAPI symbol not found in loaded DLL\n");
        return;
    }

    int ok = getApi(eRENDERDOC_API_Version_1_4_2,
                    reinterpret_cast<void **>(&sRdocApi));
    if (!ok || sRdocApi == nullptr) {
        port_log("SSB64: RENDERDOC_GetAPI returned failure (api version unsupported?)\n");
        sRdocApi = nullptr;
        return;
    }

    // Build the capture file path template. RenderDoc appends
    // "_<datetime>_frame<N>.capture" (a.k.a. .rdc) to whatever we supply.
    const char *dir_env = std::getenv("SSB64_RENDERDOC_DIR");
    std::string dir = (dir_env != nullptr && *dir_env != '\0')
        ? std::string(dir_env)
        : std::string("debug_traces/renderdoc");

    try {
        std::filesystem::create_directories(dir);
    } catch (const std::exception &e) {
        port_log("SSB64: RenderDoc create_directories('%s') failed: %s (continuing)\n",
                 dir.c_str(), e.what());
        // Non-fatal — RenderDoc will fall back to its default capture path.
    } catch (...) {
        port_log("SSB64: RenderDoc create_directories('%s') failed (unknown); continuing\n",
                 dir.c_str());
    }

    std::string template_path = dir + "/ssb64";
    sRdocApi->SetCaptureFilePathTemplate(template_path.c_str());

    port_log("SSB64: RenderDoc API initialized, capture template=%s, frames=%s\n",
             template_path.c_str(), frames_env);
}

extern "C" void portRenderDocOnFrame(unsigned int frame_count)
{
    if (sRdocApi == nullptr) {
        return;
    }

    const bool match = sCaptureAll || (sCaptureFrames.count(frame_count) > 0);
    if (!match) {
        return;
    }

    sRdocApi->TriggerCapture();
    port_log("SSB64: RenderDoc TriggerCapture at frame %u\n", frame_count);
}

extern "C" void portRenderDocShutdown(void)
{
    // RenderDoc flushes capture files automatically on Present.
    // Nothing to do here, but keep the symbol so callers can follow the
    // init/shutdown symmetry without ifdefs.
    sRdocApi = nullptr;
}

#else  // __SWITCH__ — RenderDoc not available on console

extern "C" void portRenderDocInit(void) {}
extern "C" void portRenderDocOnFrame(unsigned int) {}
extern "C" void portRenderDocShutdown(void) {}
extern "C" void portRenderDocTriggerCaptureStart(int) {}

#endif // __SWITCH__
