#define SDL_MAIN_HANDLED
#include "port.h"
#include "gameloop.h"

#include <libultraship/libultraship.h>
#include <SDL2/SDL.h>
#include <libultraship/controller/controldeck/ControlDeck.h>
#include <fast/Fast3dWindow.h>
#include <ship/resource/File.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <typeinfo>

#include "resource/ResourceType.h"
#include "resource/RelocFileFactory.h"
#include <ship/resource/factory/BlobFactory.h>
#include <ship/resource/ResourceType.h>

#include "app_paths.h"
#include "bridge/audio_bridge.h"
#include "bridge/framebuffer_capture.h"
#include "enhancements/enhancements.h"
#include "first_run.h"
#include "gui/PortMenu.h"
#include "renderdoc_trigger.h"
#include "port_log.h"

#include <filesystem>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>
#include <crtdbg.h>
#include <signal.h>
#include <exception>
#include <ctime>
#pragma comment(lib, "dbghelp.lib")

static void portCrtInvalidParameter(const wchar_t* expr, const wchar_t* func,
                                    const wchar_t* file, unsigned line, uintptr_t)
{
	port_log("\n*** CRT INVALID PARAMETER ***\n");
	if (expr) port_log("    expr: %ls\n", expr);
	if (func) port_log("    func: %ls\n", func);
	if (file) port_log("    file: %ls:%u\n", file, line);
	port_log_close();
}

static void portTerminateHandler()
{
	port_log("\n*** std::terminate called (uncaught C++ exception) ***\n");
	port_log_close();
	std::abort();
}

static void portSignalHandler(int sig)
{
	const char *name = "?";
	switch (sig) {
		case SIGABRT: name = "SIGABRT"; break;
		case SIGFPE:  name = "SIGFPE";  break;
		case SIGILL:  name = "SIGILL";  break;
		case SIGINT:  name = "SIGINT";  break;
		case SIGSEGV: name = "SIGSEGV"; break;
		case SIGTERM: name = "SIGTERM"; break;
	}
	port_log("\n*** SIGNAL %s (%d) raised ***\n", name, sig);
	port_log_close();
}

static volatile LONG sMinidumpWritten = 0;

static void portResolveSymbol(void* addr, char* out, size_t cap)
{
	out[0] = '\0';
	HMODULE mod = nullptr;
	if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
	                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                       (LPCSTR)addr, &mod) && mod) {
		char modPath[MAX_PATH] = {0};
		GetModuleFileNameA(mod, modPath, sizeof(modPath));
		const char *modName = std::strrchr(modPath, '\\');
		modName = modName ? modName + 1 : modPath;
		std::snprintf(out, cap, "%s+0x%llx", modName,
		             (unsigned long long)((uintptr_t)addr - (uintptr_t)mod));
	}
}

static void portWriteMinidump(EXCEPTION_POINTERS* info, const char* prefix)
{
	char dumpPath[MAX_PATH];
	std::time_t t = std::time(nullptr);
	std::snprintf(dumpPath, sizeof(dumpPath), "crash_%lld.dmp", (long long)t);
	HANDLE hf = CreateFileA(dumpPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
	                        FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hf == INVALID_HANDLE_VALUE) {
		port_log("    %s minidump create failed (err=%lu)\n", prefix, GetLastError());
		return;
	}

	MINIDUMP_EXCEPTION_INFORMATION mei = {};
	mei.ThreadId = GetCurrentThreadId();
	mei.ExceptionPointers = info;
	mei.ClientPointers = FALSE;
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hf,
	                  (MINIDUMP_TYPE)(MiniDumpWithDataSegs | MiniDumpWithThreadInfo |
	                                  MiniDumpWithIndirectlyReferencedMemory),
	                  &mei, nullptr, nullptr);
	CloseHandle(hf);
	port_log("    %s minidump = %s\n", prefix, dumpPath);
}

// Decode an MSVC C++ throw (exception code 0xE06D7363) into a type-info name
// and, if the thrown object derives from std::exception, its what() string.
//
// MSVC ABI on x64: ExceptionInformation[] holds:
//   [0] magic = 0x19930520 (or 0x19930521/22)
//   [1] pointer to thrown object
//   [2] pointer to ThrowInfo (image-relative on x64)
//   [3] HMODULE base for the image-relative offsets in ThrowInfo
//
// ThrowInfo -> CatchableTypeArray -> CatchableType[0] -> std::type_info* + offsets.
//
// We only decode the *primary* catchable type (CatchableType[0]). That's
// the most-derived type the thrower used, which matches what's actually
// thrown (not a base of it). It's enough to identify the exception in 99%
// of cases — std::filesystem::filesystem_error, std::bad_alloc,
// std::system_error, etc.
struct PortMsvcThrowInfo {
	uint32_t attributes;
	int32_t  pmfnUnwind;            // image-relative
	int32_t  pForwardCompat;        // image-relative
	int32_t  pCatchableTypeArray;   // image-relative
};
struct PortMsvcCatchableTypeArray {
	uint32_t nCatchableTypes;
	int32_t  arrayOfCatchableTypes[1]; // image-relative array
};
struct PortMsvcCatchableType {
	uint32_t  properties;
	int32_t   pType;                // image-relative; std::type_info*
	struct { int32_t mdisp, pdisp, vdisp; } thisDisplacement;
	int32_t   sizeOrOffset;
	int32_t   copyFunction;
};

static void portLogMsvcCxxThrow(EXCEPTION_POINTERS* info)
{
	const EXCEPTION_RECORD* er = info->ExceptionRecord;
	if (er->NumberParameters < 3) {
		port_log("    C++ throw: (no parameters)\n");
		return;
	}

	uintptr_t imgBase = er->NumberParameters >= 4
	                  ? (uintptr_t)er->ExceptionInformation[3]
	                  : (uintptr_t)GetModuleHandleA(nullptr);
	void* thrownObj = (void*)er->ExceptionInformation[1];
	auto* throwInfo = (const PortMsvcThrowInfo*)er->ExceptionInformation[2];
	if (!throwInfo || !imgBase) {
		port_log("    C++ throw: thrown=%p (no ThrowInfo/imgBase)\n", thrownObj);
		return;
	}

	auto* cta = (const PortMsvcCatchableTypeArray*)
	            (imgBase + (uint32_t)throwInfo->pCatchableTypeArray);
	if (!cta || cta->nCatchableTypes == 0) {
		port_log("    C++ throw: thrown=%p (empty CatchableTypeArray)\n", thrownObj);
		return;
	}

	auto* ct = (const PortMsvcCatchableType*)
	           (imgBase + (uint32_t)cta->arrayOfCatchableTypes[0]);
	const std::type_info* ti = (const std::type_info*)
	                           (imgBase + (uint32_t)ct->pType);
	const char* tname = ti ? ti->name() : "(null)";

	// Most std exceptions live at offset 0 in the catchable layout (single
	// inheritance, no virtual bases). For multi-inherit / virtual-base
	// types this would need adjustments; keep it conservative and report
	// the type name regardless. We attempt what() only when offset 0 looks
	// safe per CatchableType properties.
	const char* what = nullptr;
	if (thrownObj && (ct->properties & 0x4) == 0) {
		// Try as std::exception. Catch any access violation just in case
		// the layout differs — vectored handlers must not throw.
		__try {
			const std::exception* ex = (const std::exception*)thrownObj;
			what = ex->what();
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			what = nullptr;
		}
	}
	port_log("    C++ throw: type=\"%s\" thrown=%p%s%s\n",
	         tname, thrownObj,
	         what ? " what=" : "",
	         what ? what     : "");
}

static LONG CALLBACK portWindowsVectoredHandler(EXCEPTION_POINTERS* info)
{
	DWORD code = info->ExceptionRecord->ExceptionCode;
	// Suppress noisy / non-actionable codes after capturing C++ throw
	// details. Issue #58: a heap corruption fault was reported in the
	// log but the *triggering* C++ exception was suppressed silently —
	// the unwind from that throw is what hit the heap-corruption
	// detector, so the throw site is the actual root cause we need.
	// Decode and log it, then keep returning EXCEPTION_CONTINUE_SEARCH
	// so SEH still gets a chance to catch it normally.
	if (code == 0xE06D7363) {
		static volatile LONG sCxxThrowsLogged = 0;
		// Cap to first 8 to avoid log spam if a hot path throws.
		if (InterlockedIncrement(&sCxxThrowsLogged) <= 8) {
			port_log("\n*** C++ THROW (first-chance) tid=%lu ***\n",
			         GetCurrentThreadId());
			portLogMsvcCxxThrow(info);
		}
		return EXCEPTION_CONTINUE_SEARCH;
	}
	if (code == EXCEPTION_BREAKPOINT ||
	    code == DBG_PRINTEXCEPTION_C || code == DBG_PRINTEXCEPTION_WIDE_C ||
	    code == 0x406D1388) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	void* addr = info->ExceptionRecord->ExceptionAddress;
	char sym[768] = {0};
	portResolveSymbol(addr, sym, sizeof(sym));
	port_log("\n*** VECTORED EXCEPTION (first-chance) tid=%lu code=0x%08X addr=%p %s ***\n",
	         GetCurrentThreadId(), (unsigned)code, addr, sym[0] ? sym : "(no sym)");
	if (code == EXCEPTION_ACCESS_VIOLATION && info->ExceptionRecord->NumberParameters >= 2) {
		const char* op = info->ExceptionRecord->ExceptionInformation[0] == 0 ? "read" :
		                 info->ExceptionRecord->ExceptionInformation[0] == 1 ? "write" : "execute";
		port_log("    AV: %s at %p\n", op, (void*)info->ExceptionRecord->ExceptionInformation[1]);
	}

	void* frames[32];
	WORD nframes = RtlCaptureStackBackTrace(0, 32, frames, nullptr);
	for (WORD i = 0; i < nframes; i++) {
		char fsym[768] = {0};
		portResolveSymbol(frames[i], fsym, sizeof(fsym));
		port_log("    [%2u] %p %s\n", i, frames[i], fsym[0] ? fsym : "(no sym)");
	}

	if (InterlockedCompareExchange(&sMinidumpWritten, 1, 0) == 0) {
		portWriteMinidump(info, "first-chance");
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

static LONG WINAPI portWindowsCrashFilter(EXCEPTION_POINTERS* info)
{
	const EXCEPTION_RECORD* er = info->ExceptionRecord;
	void* addr = er->ExceptionAddress;

	HMODULE mod = nullptr;
	char modname[MAX_PATH] = {0};
	if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
	                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                       (LPCSTR)addr, &mod)) {
		GetModuleFileNameA(mod, modname, sizeof(modname));
	}

	uintptr_t base = (uintptr_t)mod;
	uintptr_t rva  = (uintptr_t)addr - base;

	port_log("\n*** UNHANDLED EXCEPTION ***\n");
	port_log("  code     = 0x%08X\n", (unsigned)er->ExceptionCode);
	port_log("  address  = %p\n", addr);
	port_log("  module   = %s (base=%p, rva=0x%llx)\n",
	         modname[0] ? modname : "(unknown)", (void*)base, (unsigned long long)rva);
	if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && er->NumberParameters >= 2) {
		const char* op = er->ExceptionInformation[0] == 0 ? "read" :
		                 er->ExceptionInformation[0] == 1 ? "write" :
		                 er->ExceptionInformation[0] == 8 ? "execute" : "?";
		port_log("  AV: %s at %p\n", op, (void*)er->ExceptionInformation[1]);
	}
#if defined(_M_X64) || defined(_M_AMD64)
	const CONTEXT* c = info->ContextRecord;
	port_log("  RIP=%p RSP=%p RBP=%p\n", (void*)c->Rip, (void*)c->Rsp, (void*)c->Rbp);
	port_log("  RAX=%016llx RBX=%016llx RCX=%016llx RDX=%016llx\n",
	         (unsigned long long)c->Rax, (unsigned long long)c->Rbx,
	         (unsigned long long)c->Rcx, (unsigned long long)c->Rdx);
	port_log("  RSI=%016llx RDI=%016llx R8 =%016llx R9 =%016llx\n",
	         (unsigned long long)c->Rsi, (unsigned long long)c->Rdi,
	         (unsigned long long)c->R8,  (unsigned long long)c->R9);
#endif

	if (InterlockedCompareExchange(&sMinidumpWritten, 1, 0) == 0) {
		portWriteMinidump(info, "unhandled");
	}
	port_log("*** END EXCEPTION ***\n");
	port_log_close();
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

static std::shared_ptr<Ship::Context> sContext;

// Port-side replacement for Ship::Context::LocateFileAcrossAppDirs.
//
// LUS's version uses the throwing form of std::filesystem::exists, which
// raises filesystem_error if the underlying GetFileAttributesExW returns
// anything other than an ENOENT-equivalent. On Win10 19042 the
// NON_PORTABLE app-data probe path (SDL_GetPrefPath returns a
// backslash-terminated path, joined with a literal "/" — yielding
// e.g. "C:\\Users\\u\\AppData\\Roaming\\BattleShip\\/f3d.o2r")
// reportedly trips that error and the unwind from the throw fast-fails
// the process via the heap-corruption detector before any user-level
// catch can run (BattleShip issue #58).
//
// Two differences from the LUS version:
//   1. Use the noexcept exists(p, ec) overload — false is the right
//      answer for any failure, and probing a path should never throw.
//   2. Probe the bundle dir via ssb64::RealAppBundlePath(), which
//      returns the actual exe directory on Windows portable-zip
//      distros. Ship::Context::GetAppBundlePath() under NON_PORTABLE
//      returns the literal CMAKE_INSTALL_PREFIX ("BattleShip"), which
//      is wrong for any user that doesn't unzip into a "BattleShip"
//      subdir matching their cwd.
static std::string PortLocateFile(const std::string& basename) {
	namespace fs = std::filesystem;
	std::error_code ec;

	const fs::path appDir(Ship::Context::GetAppDirectoryPath());
	fs::path p1 = appDir / basename;
	if (fs::exists(p1, ec)) {
		return p1.lexically_normal().string();
	}

	const fs::path bundleDir(ssb64::RealAppBundlePath());
	fs::path p2 = bundleDir / basename;
	if (fs::exists(p2, ec)) {
		return p2.lexically_normal().string();
	}

	return "./" + basename;
}

extern "C" {

static int PortInitImpl(int argc, char* argv[]);

int PortInit(int argc, char* argv[]) {
	// Top-level catch so unhandled C++ exceptions during init land in
	// ssb64.log with their type and what() instead of bubbling up as an
	// opaque MSVC 0xE06D7363 throw that the user sees as "the app just
	// crashed". Issue #58 reported a Win10 19042 crash with no usable
	// signal beyond "ControlDeck OK"; the caught e.what() narrows it.
	try {
		return PortInitImpl(argc, argv);
	} catch (const std::exception& e) {
		port_log("\n*** PortInit: unhandled C++ exception ***\n"
		         "    type: %s\n    what: %s\n",
		         typeid(e).name(), e.what());
		port_log_close();
		return 1;
	} catch (...) {
		port_log("\n*** PortInit: unhandled non-std exception ***\n");
		port_log_close();
		return 1;
	}
}

static int PortInitImpl(int argc, char* argv[]) {
	port_log("SSB64: PortInit entered\n");

	/* Wire the SSB64-specific DL-range bounds-check and diag classifier
	 * into libultraship's GFX walker (see port/port_dl_ranges.h). Done
	 * before any GFX activity. libultraship has no compile-time symbol
	 * dependency on these — both are optional callbacks. */
	{
		extern void port_dl_ranges_init(void);
		port_dl_ranges_init();
	}

	sContext = Ship::Context::CreateUninitializedInstance(
		"BattleShip",
		"BattleShip",
		"BattleShip.cfg.json"
	);

	if (!sContext) {
		port_log("SSB64: Failed to create context instance\n");
		return 1;
	}

	port_log("SSB64: Context instance created\n");

	if (!sContext->InitLogging()) { port_log("SSB64: InitLogging failed\n"); return 1; }
	port_log("SSB64: Logging OK\n");

	if (!sContext->InitConfiguration()) { port_log("SSB64: InitConfiguration failed\n"); return 1; }
	if (!sContext->InitConsoleVariables()) { port_log("SSB64: InitConsoleVariables failed\n"); return 1; }
	port_log("SSB64: Config + CVars OK\n");

	/* Pillarbox the framebuffer to 4:3 every launch. The game emits 4:3
	 * GBI only — when LUS's viewport runs at the window aspect (LUS
	 * default: gAdvancedResolution.Enabled=0, viewport == content region),
	 * fullscreen on a 16:9 monitor draws into a 16:9 framebuffer that the
	 * game only fills 4:3 of, leaving the side strips as un-cleared
	 * garbage. Force LUS's aspect-correction path on with a 4:3 target so
	 * mCurDimensions and DrawGame()'s sPosX pillarbox the game image
	 * inside black bars.
	 *
	 * Skip the 4:3 force when the widescreen feature is enabled — its
	 * clip-x compression in Interpreter::AdjXForAspectRatio expands the 4:3
	 * GBI to fill the widened FB during battles, and outside battles we
	 * accept the 4:3-stretched menu look as a Phase 1 trade-off. Toggling
	 * the widescreen CVar takes effect on the next launch (the 4:3 force is
	 * latched here); changing it mid-session triggers an mCurDimensions FB
	 * resize that's racy and crashes. Document this on the menu tooltip. */
	if (auto cv = sContext->GetConsoleVariables()) {
		const bool widescreen_on = cv->GetInteger("gEnhancements.Widescreen", 1) != 0;
		cv->SetFloat("gAdvancedResolution.AspectRatioX", 4.0f);
		cv->SetFloat("gAdvancedResolution.AspectRatioY", 3.0f);

		/* Latch the Low Resolution Mode menu choice at boot. libultraship's
		 * Gui::CalculateGameViewport / Gui::DrawGame read gLowResMode and the
		 * gAdvancedResolution.* cvars every frame and rewrite mCurDimensions,
		 * which forces a Fast3D framebuffer reallocation. Toggling these
		 * mid-session races the ImGui Metal backend: the per-frame
		 * ImGui::Image cmd captures the old MTLTexture pointer at submit,
		 * but the FB resource releases it on dim change before Metal's
		 * encoder retains the texture, so the next setFragmentTexture:
		 * objc_retain hits a freed object and SIGSEGVs (same hazard
		 * documented above for the widescreen toggle). The menu writes the
		 * user's choice to gLowResModePending; we translate it into the
		 * right set of LUS cvars here, once per launch.
		 *
		 *   0  Off — window-resolution framebuffer
		 *   1  N64 stretched 4:3       (gLowResMode=1)
		 *   2  240p window aspect      (gLowResMode=2)
		 *   3  480p window aspect      (gLowResMode=3)
		 *   4..7  N64 pixel-perfect integer scale (PixelPerfectMode path,
		 *        fixed 320x240 framebuffer via VerticalResolutionToggle=1
		 *        + VerticalPixelCount=240, factor selected by sub-key) */
		if (cv->Get("gLowResModePending") == nullptr) {
			cv->SetInteger("gLowResModePending", cv->GetInteger("gLowResMode", 0));
		}
		const int32_t mode = cv->GetInteger("gLowResModePending", 0);

		int32_t low_res_mode = 0;
		bool integer_scale = false;
		int32_t integer_factor = 1;
		bool integer_auto_fit = false;
		switch (mode) {
			case 1: low_res_mode = 1; break;
			case 2: low_res_mode = 2; break;
			case 3: low_res_mode = 3; break;
			case 4: integer_scale = true; integer_auto_fit = true; break;
			case 5: integer_scale = true; integer_factor = 2; break;
			case 6: integer_scale = true; integer_factor = 3; break;
			case 7: integer_scale = true; integer_factor = 4; break;
			default: break;
		}

		cv->SetInteger("gLowResMode", low_res_mode);
		if (integer_scale) {
			/* PixelPerfect path needs Enabled=1 regardless of widescreen so
			 * ApplyResolutionChanges runs; with VerticalResolutionToggle=1
			 * and a 4:3 AspectRatio, mCurDimensions resolves to 320x240, then
			 * DrawGame's pixel-perfect branch draws it at integer factor. */
			cv->SetInteger("gAdvancedResolution.Enabled", 1);
			cv->SetInteger("gAdvancedResolution.VerticalResolutionToggle", 1);
			cv->SetInteger("gAdvancedResolution.VerticalPixelCount", 240);
			cv->SetInteger("gAdvancedResolution.PixelPerfectMode", 1);
			cv->SetInteger("gAdvancedResolution.IntegerScale.FitAutomatically", integer_auto_fit ? 1 : 0);
			cv->SetInteger("gAdvancedResolution.IntegerScale.Factor", integer_factor);
			cv->SetInteger("gAdvancedResolution.IntegerScale.NeverExceedBounds", 1);
		} else {
			/* Restore the widescreen-driven 4:3 pillarbox default when no
			 * integer-scale mode is selected. */
			cv->SetInteger("gAdvancedResolution.Enabled", widescreen_on ? 0 : 1);
			cv->SetInteger("gAdvancedResolution.VerticalResolutionToggle", 0);
			cv->SetInteger("gAdvancedResolution.PixelPerfectMode", 0);
		}

		/* Issue #96 migration. v0.7-beta stored the per-player NRage
		 * analog-remap enable flag at gEnhancements.AnalogRemap.PX —
		 * the same JSON path that PX.Deadzone / PX.Range hang off of.
		 * That made PX both a scalar and a parent, and Config::Save's
		 * unflatten() threw json::type_error.313 the first time the
		 * Deadzone or Range slider moved, truncating the file to
		 * empty before the throw. The enable cvar is now stored at
		 * gEnhancements.AnalogRemap.PX.Enabled (sibling of Deadzone
		 * / Range). Migrate any legacy scalar by copying its value
		 * to the new key and clearing the old key, otherwise the
		 * legacy entry would re-introduce the conflict on next save. */
		for (int p = 0; p < 4; ++p) {
			char legacy[64];
			char modern[64];
			std::snprintf(legacy, sizeof(legacy), "gEnhancements.AnalogRemap.P%d", p + 1);
			std::snprintf(modern, sizeof(modern), "gEnhancements.AnalogRemap.P%d.Enabled", p + 1);
			if (auto var = cv->Get(legacy); var && var->Type == Ship::ConsoleVariableType::Integer) {
				cv->SetInteger(modern, var->Integer);
				cv->ClearVariable(legacy);
			}
		}
	}

#ifdef __APPLE__
	/* Force the Metal backend on macOS.  The OpenGL backend works but
	 * Apple's GLD driver emits a one-shot
	 *   "GLD_TEXTURE_INDEX_2D is unloadable and bound to sampler type
	 *    (Float) - using zero texture because texture unloadable"
	 * the first time Fast3D draws with the TEXEL1 sampler declared but
	 * unbound (combine doesn't reference TEXEL1).  Metal-cpp accepts
	 * the same shader without complaint and avoids the GL deprecation
	 * path on Apple Silicon.  If Metal is somehow unavailable on the
	 * host, libultraship's Config::GetWindowBackend() fallback
	 * downgrades to OpenGL automatically — this write only changes the
	 * preferred default. */
	sContext->GetConfig()->SetInt(
		"Window.Backend.Id",
		static_cast<int>(Ship::WindowBackend::FAST3D_SDL_METAL));
	sContext->GetConfig()->SetString("Window.Backend.Name", "Metal");
#endif

	/* New init order:
	 *   1. CrashHandler / Console / ControlDeck     — resource-agnostic
	 *   2. ResourceManager bootstrapped with f3d.o2r only — the renderer
	 *      backend (Metal / OpenGL / D3D11) compiles shaders out of
	 *      f3d.o2r during InitWindow's first frame setup, so the
	 *      ResourceManager has to exist by then.  f3d.o2r is shipped
	 *      with the binary (always present, no ROM needed).
	 *   3. Window + Port Menu
	 *   4. First-run flow: silent in-process extraction, then ImGui wizard if needed.
	 *      Once BattleShip.o2r is on disk we add it via ArchiveManager.
	 *   5. Audio / GfxDebugger / FileDropMgr / factory registration. */
	if (!sContext->InitCrashHandler()) { port_log("SSB64: InitCrashHandler failed\n"); return 1; }
	if (!sContext->InitConsole()) { port_log("SSB64: InitConsole failed\n"); return 1; }
	port_log("SSB64: CrashHandler + Console OK\n");

	// ControlDeck MUST be initialized before Window — the DXGI window proc
	// calls ControllerUnblockGameInput on WM_SETFOCUS during window creation.
	//
	// Scoped so the local shared_ptr drops its ref inside the block: sContext
	// retains a copy via InitControlDeck, so when PortShutdown later calls
	// sContext.reset() it's the sole owner. Otherwise PortInit's locals
	// outlive sContext and their destructors run after spdlog::shutdown,
	// crashing in SPDLOG_DEBUG calls inside libultraship destructors.
	{
		auto controlDeck = std::make_shared<LUS::ControlDeck>();
		if (!sContext->InitControlDeck(controlDeck)) { port_log("SSB64: InitControlDeck failed\n"); return 1; }
	}
	port_log("SSB64: ControlDeck OK\n");

	{
		// Bootstrap ResourceManager with f3d.o2r only. Allow empty paths
		// so a missing f3d.o2r logs but doesn't fatal — the Window init
		// would still partially work for the wizard, which is enough.
		//
		// PortLocateFile replaces Ship::Context::LocateFileAcrossAppDirs
		// for issue #58 — see the helper's comment for why.
		port_log("SSB64: locating f3d.o2r ...\n");
		const std::string f3d = PortLocateFile("f3d.o2r");
		port_log("SSB64: bootstrap archive (shaders) -> %s\n", f3d.c_str());

		std::vector<std::string> bootstrapPaths = {f3d};
		port_log("SSB64: calling InitResourceManager (bootstrap) ...\n");
		if (!sContext->InitResourceManager(bootstrapPaths, {}, 0,
		                                   /*allowEmptyPaths=*/true)) {
			port_log("SSB64: bootstrap InitResourceManager failed\n");
			return 1;
		}
		port_log("SSB64: bootstrap ResourceManager OK\n");
	}

	// See controlDeck note above re: scoping.
	{
		port_log("SSB64: constructing Fast3dWindow ...\n");
		auto window = std::make_shared<Fast::Fast3dWindow>();
		port_log("SSB64: calling InitWindow ...\n");
		if (!sContext->InitWindow(window)) { port_log("SSB64: InitWindow failed\n"); return 1; }
		port_log("SSB64: Window OK\n");

		// Esc Menu screen, Toggle with Esc.
		if (auto gui = window->GetGui()) {
			port_log("SSB64: attaching Port menu ...\n");
			gui->SetMenu(std::make_shared<ssb64::PortMenu>());
			port_log("SSB64: Port menu attached\n");
		}
	}

	// Pin LUS to off-screen rendering so mGameFb is populated during
	// gameplay and the GPU readback at scene transitions captures the
	// prior frame rather than the post-Present swap-chain back buffer
	// (undefined contents under DXGI FLIP_DISCARD on D3D11). Required by
	// the 1P stage-clear frozen-wallpaper capture (issue #57) and the VS
	// match -> results-screen photo wipe (issue #81). Cost is one extra
	// full-screen blit per frame (sub-millisecond on any modern GPU).
	port_capture_set_force_render_to_fb(1);

	// FileDropMgr must come up before the first-run wizard so SDL_DROPFILE
	// events landing on the window during the wizard frame loop can be
	// polled and used to fill the ROM path field.
	if (!sContext->InitFileDropMgr()) { port_log("SSB64: InitFileDropMgr failed\n"); return 1; }
	port_log("SSB64: FileDropMgr OK\n");

	/* First-run flow:
	 *   1. Silent extraction: if a ROM sits at app-data / bundle / cwd we
	 *      just extract without bothering the user.
	 *   2. If still missing, drive an ImGui wizard modal in a pre-gameloop
	 *      render loop until the user provides a ROM and extraction
	 *      succeeds — or quits the window. */
	{
		const std::string targetO2r =
			Ship::Context::GetPathRelativeToAppDirectory("BattleShip.o2r");
		// silent=true: any failure during this auto-attempt should land in
		// the wizard's status text, not a native popup that races the
		// ImGui modal.
		ssb64::ExtractAssetsIfNeeded(targetO2r, /*silent=*/true);
		std::error_code ec;
		// noexcept exists / PortLocateFile rather than the throwing LUS
		// form — issue #58.
		if (!std::filesystem::exists(targetO2r, ec) &&
		    !std::filesystem::exists(PortLocateFile("BattleShip.o2r"), ec)) {
			if (!ssb64::RunFirstRunWizard(targetO2r)) {
				port_log("SSB64: first-run wizard cancelled — exiting\n");
				// PortShutdown drops audio bridge refs + resets sContext
				// before main returns. Without it, IResource destructors
				// run during static teardown after spdlog has already
				// closed, raising "mutex lock failed: Invalid argument"
				// from the Fast3dWindow destructor's SPDLOG_DEBUG and
				// terminating with SIGABRT.
				PortShutdown();
				return 1;
			}
		}
	}

	{
		// Add BattleShip.o2r to the running ResourceManager now that it exists.
		auto am = sContext->GetResourceManager()->GetArchiveManager();

		const std::string ssb64o2r = PortLocateFile("BattleShip.o2r");
		port_log("SSB64: adding game archive -> %s\n", ssb64o2r.c_str());
		if (!am->AddArchive(ssb64o2r)) {
			port_log("SSB64: AddArchive failed for %s\n", ssb64o2r.c_str());
			return 1;
		}
		port_log("SSB64: game archive registered\n");
	}

	{
		/* SSB64's audio synthesis path produces interleaved s16 stereo PCM at
		 * 32 kHz (sSYAudioFrequency, see src/sys/audio.c).  LUS's default
		 * AudioSettings::SampleRate is 44100 Hz — passing an empty {} settings
		 * struct makes the host audio device expect 44.1 kHz samples while we
		 * feed it 32 kHz, causing pitch-shift / time-stretch / aliasing that
		 * shows up as broadband noise in the output. */
		Ship::AudioSettings audio;
		audio.SampleRate = 32000;
		if (!sContext->InitAudio(audio)) { port_log("SSB64: InitAudio failed\n"); return 1; }
		port_log("SSB64: Audio initialized at %d Hz\n", (int)audio.SampleRate);
	}
	if (!sContext->InitGfxDebugger()) { port_log("SSB64: InitGfxDebugger failed\n"); return 1; }
	// InitFileDropMgr already happened earlier — see the wizard plumbing.
	port_log("SSB64: All subsystems initialized\n");

	// Register resource factories
	auto loader = sContext->GetResourceManager()->GetResourceLoader();
	loader->RegisterResourceFactory(
		std::make_shared<ResourceFactoryBinaryRelocFileV0>(),
		RESOURCE_FORMAT_BINARY,
		"SSB64Reloc",
		static_cast<uint32_t>(SSB64::ResourceType::SSB64Reloc),
		0
	);
	loader->RegisterResourceFactory(
		std::make_shared<Ship::ResourceFactoryBinaryBlobV0>(),
		RESOURCE_FORMAT_BINARY,
		"Blob",
		static_cast<uint32_t>(Ship::ResourceType::Blob),
		0
	);

	port_log("SSB64: Resource factories registered — init complete\n");
	return 0;
}

void PortShutdown(void) {
	// Drop audio bridge resource references before Ship::Context goes away.
	// Otherwise their shared_ptrs survive into __cxa_finalize_ranges and
	// Ship::IResource::~IResource() lands on a shut-down spdlog.
	portAudioShutdownAssets();
	// Stop any in-flight controller rumble while Context + ControlDeck +
	// gamepads + SDL are all still alive. SDLRumbleMapping::StopRumble walks
	// back through Context::GetInstance(), so this MUST run before
	// sContext.reset() — destructor re-entry through the Context singleton
	// has its own SIGSEGV trap. Issue #82: on Linux/evdev, the last
	// SDL_GameControllerRumble call uploads an FF effect with
	// SDL_MAX_RUMBLE_DURATION_MS (~32s); without an explicit stop, the
	// kernel runs the effect to completion after the process exits.
	if (sContext) {
		if (auto cd = sContext->GetControlDeck()) {
			// Shut down the raphnet pipeline FIRST. Each transport's Close
			// sends RQ_RNT_SUSPEND_POLLING(0) so the adapter is usable as a
			// plain HID joystick again after process exit (the firmware does
			// NOT auto-resume on hid_close). Order: ShutdownRaphnet → Stop-
			// AllRumble → sContext.reset() — same singleton-reentry caveat
			// applies as the rumble cleanup below.
			cd->ShutdownRaphnet();
			cd->StopAllRumble();
		}
	}
	sContext.reset();
	port_log_close();
}

int PortIsRunning(void) {
	return WindowIsRunning() ? 1 : 0;
}

} // extern "C"

int main(int argc, char* argv[]) {
	/* Use an absolute path for ssb64.log so it lands in a predictable
	 * place regardless of how the binary was launched (Finder / open /
	 * shell from any cwd). SDL_GetPrefPath returns the OS app-data dir
	 * (~/Library/Application Support/BattleShip/, %APPDATA%\BattleShip\,
	 * $XDG_DATA_HOME/BattleShip/) and creates it on demand — same dir
	 * Ship::Context will later use for the user's saves and o2r. */
	{
		std::string logPath;
#ifdef __SWITCH__
		// On Switch, write ssb64.log to the SD card alongside the .nro
		// so it's accessible without a PC tool.
		logPath = "sdmc:/switch/BattleShip/ssb64.log";
#else
		if (char* p = SDL_GetPrefPath(NULL, "BattleShip")) {
			logPath = std::string(p) + "ssb64.log";
			SDL_free(p);
		} else {
			logPath = "ssb64.log";  // last-resort cwd fallback
		}
#endif
		port_log_init(logPath.c_str());
	}

#ifdef _WIN32
	SetUnhandledExceptionFilter(portWindowsCrashFilter);
	AddVectoredExceptionHandler(1, portWindowsVectoredHandler);
	std::atexit([]() {
		port_log("\n*** atexit reached — process is shutting down voluntarily ***\n");
		port_log_close();
	});
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_ERROR,  _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR,  _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_WARN,   _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN,   _CRTDBG_FILE_STDERR);
	_set_invalid_parameter_handler(portCrtInvalidParameter);
	std::set_terminate(portTerminateHandler);
	signal(SIGABRT, portSignalHandler);
	signal(SIGFPE,  portSignalHandler);
	signal(SIGILL,  portSignalHandler);
	signal(SIGSEGV, portSignalHandler);
	signal(SIGTERM, portSignalHandler);
#endif

	// Initialize RenderDoc trigger BEFORE PortInit so the RenderDoc DLL
	// can hook D3D11 before LUS creates the device.
	portRenderDocInit();

	if (PortInit(argc, argv) != 0) {
		return 1;
	}

	// Wrap post-init (game boot + main loop + shutdown) in a top-level
	// catch so uncaught C++ exceptions get logged as ssb64.log entries
	// with type and what() before the process exits, instead of bubbling
	// up as an opaque MSVC 0xE06D7363 throw.
	try {

	// Initialize the game boot sequence (coroutines, thread init, etc.)
	PortGameInit();

	// Main frame loop — each iteration runs one frame of game logic
	// and rendering through the coroutine system. PortPushFrame posts
	// a VI tick, resumes the game coroutine, and display lists are
	// rendered via DrawAndRunGraphicsCommands inside the coroutine.
	//
	// SSB64_MAX_FRAMES=N — debug aid that forces a clean shutdown
	// after N frames. Goes through the same code path as the user
	// closing the window (Window::Close() sets mIsRunning=false).
	int maxFrames = 0;
	if (const char* env = std::getenv("SSB64_MAX_FRAMES")) {
		maxFrames = std::atoi(env);
	}
	int frame = 0;
	bool firstRunHintShown = false;
	while (WindowIsRunning()) {
		PortPushFrame();
		frame++;

		if (!firstRunHintShown && frame == 60) {
			auto cv = sContext->GetConsoleVariables();
			if (cv && cv->GetInteger("gFirstRunHintShown", 0) == 0) {
				cv->SetInteger("gFirstRunHintShown", 1);
				if (auto gui = sContext->GetWindow()->GetGui()) {
					gui->SaveConsoleVariablesNextFrame();
				}
			}
			firstRunHintShown = true;
		}
		if (maxFrames > 0 && frame >= maxFrames) {
			port_log("SSB64: SSB64_MAX_FRAMES=%d reached — triggering clean shutdown\n", maxFrames);
			if (auto ctx = Ship::Context::GetInstance()) {
				if (auto win = ctx->GetWindow()) {
					win->Close();
				}
			}
			break;
		}
	}

	port_log("SSB64: main loop exited cleanly at frame=%d (WindowIsRunning=%d)\n",
	         frame, WindowIsRunning());

	PortGameShutdown();
	port_log("SSB64: PortGameShutdown returned\n");

	PortShutdown();
	portRenderDocShutdown();
	return 0;

	} catch (const std::exception& e) {
		port_log("\n*** main: unhandled C++ exception ***\n"
		         "    type: %s\n    what: %s\n",
		         typeid(e).name(), e.what());
		port_log_close();
		return 1;
	} catch (...) {
		port_log("\n*** main: unhandled non-std exception ***\n");
		port_log_close();
		return 1;
	}
}
