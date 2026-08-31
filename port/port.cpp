// On desktop we own `int main` directly. On Android, SDLActivity calls into
// the .so via dlsym("SDL_main"), so we let SDL_main.h's `#define main SDL_main`
// rename the entry point during preprocessing — which is exactly what
// SDL_MAIN_HANDLED would suppress.
#if !defined(__ANDROID__)
#define SDL_MAIN_HANDLED
#endif
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
#include <functional>
#include <typeinfo>
#include <unordered_set>

#include "resource/ResourceType.h"
#include "resource/RelocFileFactory.h"
#include <ship/resource/factory/BlobFactory.h>
#include <ship/resource/ResourceType.h>
#include <fast/resource/ResourceType.h>
#include <fast/resource/factory/DisplayListFactory.h>
#include <fast/resource/factory/LightFactory.h>
#include <fast/resource/factory/MatrixFactory.h>
#include <fast/resource/factory/TextureFactory.h>
#include <fast/resource/factory/VertexFactory.h>

#include "app_paths.h"
#include "bridge/audio_bridge.h"
#include "bridge/framebuffer_capture.h"
#include "enhancements/enhancements.h"
#include "first_run.h"
#include "gui/PortMenu.h"
#ifdef PORT_HIRES_ENABLED
#include "hires/HiResHook.h"
#include "hires/HiResPack.h"
#endif
#if !defined(__ANDROID__)
#include "port_window_icon.h"
#endif
#if defined(__ANDROID__)
#include <android/api-level.h>  // android_get_device_api_level (audio-driver gate)
#endif
#ifndef DISABLE_SCRIPTING
#include "mods/HookManager.h"
#include "mods/SymbolResolver.h"
#endif
#include "renderdoc_trigger.h"
#include "port_log.h"
#include "fighter_registry.h"
#include "focus.h"

#ifndef DISABLE_SCRIPTING
#include <ship/scripting/ScriptLoader.h>
#include <libultraship/bridge/scriptingbridge.h>
#endif

extern "C" void PortRegisterEvents(void);

#ifndef DISABLE_SCRIPTING
/* Force the linker to pull bridge .obj files into the EXE so their
 * exported symbols land in the .edata table. Mods call these by name;
 * nothing in the EXE itself references them, so without an explicit
 * anchor they get dropped at link time and the post-build tcc -impdef
 * step doesn't see them. */
extern "C" int   mod_install_hook(const char* symbol_name, void* replacement, void** original_out);
extern "C" void* mod_resolve_symbol(const char* symbol_name);
extern "C" void* sScriptingBridgeAnchor = (void*)&ScriptGetFunction;
extern "C" void* sModBridgeAnchorHook    = (void*)&mod_install_hook;
extern "C" void* sModBridgeAnchorResolve = (void*)&mod_resolve_symbol;

/* MSVC's WINDOWS_EXPORT_ALL_SYMBOLS pass exports global *functions* but
 * not most non-trivial global *data* objects, so engine globals like
 * gGCCommonLinks don't make it into BattleShip.def and TCC mods can't
 * resolve them by name. Wrap the ones mods commonly want behind tiny
 * accessor functions; functions always export. */
extern "C" struct GObj;
extern "C" struct GObj *gGCCommonLinks_Ref(int link_id);
extern "C" struct GObj *gGCCommonLinks_Ref(int link_id) {
    extern struct GObj *gGCCommonLinks[];
    /* No bounds-check: callers pass the engine's own enum constants. */
    return gGCCommonLinks[link_id];
}
extern "C" void* sModBridgeAnchorFighterListRef = (void*)&gGCCommonLinks_Ref;

/* Stage geometry: mods that distribute spawn positions across the
 * active stage (StaryuSquad's followers etc) need map_bound_left /
 * map_bound_right / map_bound_bottom from the engine's ground data.
 * Same export hole as gGCCommonLinks -- wrap the pointer. */
struct MPGroundData;
extern "C" struct MPGroundData *gMPCollisionGroundData_Ref(void);
extern "C" struct MPGroundData *gMPCollisionGroundData_Ref(void) {
    extern struct MPGroundData *gMPCollisionGroundData;
    return gMPCollisionGroundData;
}
extern "C" void* sModBridgeAnchorGroundDataRef = (void*)&gMPCollisionGroundData_Ref;

/* Fighter data-files lookup table. Exposed so mod-loader code can
 * write a registered character's FTData* into the trailing NULL-sentinel
 * slot (index nFTKindEnumCount) during ftManagerMakeFighter and have
 * the engine's internal `fp->data = dFTManagerDataFiles[fp->fkind]`
 * indexing resolve to that entry. Same export-hole workaround as the
 * other globals. */
struct FTData;
extern "C" struct FTData **dFTManagerDataFiles_Ref(void);
extern "C" struct FTData **dFTManagerDataFiles_Ref(void) {
    extern struct FTData *dFTManagerDataFiles[];
    return dFTManagerDataFiles;
}
extern "C" void* sModBridgeAnchorDataFilesRef = (void*)&dFTManagerDataFiles_Ref;
#endif

#include <filesystem>
#include <system_error>

#include <ship/debug/Console.h>
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

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

static volatile LONG sMinidumpWritten = 0;
static void portResolveSymbol(void* addr, char* out, size_t cap);
static void portWriteMinidump(EXCEPTION_POINTERS* info, const char* prefix);

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
	port_log("\n*** SIGNAL %s (%d) raised - tid=%lu ***\n",
	         name, sig, (unsigned long)GetCurrentThreadId());

	/* signal() handlers get no EXCEPTION_POINTERS and abort() does not run the
	 * SEH crash filter, so without this the abort path leaves no stack in the
	 * log (and writes no dump). Capture + symbolize it here so a re-crash
	 * self-reports the faulting frames. An uncaught C++ exception routes here
	 * too (terminate -> abort -> SIGABRT), so the throw site is still on the
	 * stack below. */
	void* frames[48];
	WORD nframes = RtlCaptureStackBackTrace(0, 48, frames, nullptr);
	for (WORD i = 0; i < nframes; i++) {
		char fsym[768] = {0};
		portResolveSymbol(frames[i], fsym, sizeof(fsym));
		port_log("    [%2u] %p %s\n", i, frames[i], fsym[0] ? fsym : "(no sym)");
	}

	if (InterlockedCompareExchange(&sMinidumpWritten, 1, 0) == 0) {
		portWriteMinidump(nullptr, "signal");
	}
	port_log_close();
}

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

		// Try SymFromAddr for the function name. The mod loader's
		// SymbolResolver::Init runs SymInitialize during port startup,
		// so by the time a crash filter fires, symbols are loaded.
		// Fall back to module+RVA if the lookup fails.
		constexpr size_t kBufSize = sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR);
		char symBuf[kBufSize];
		SYMBOL_INFO* si = reinterpret_cast<SYMBOL_INFO*>(symBuf);
		std::memset(si, 0, kBufSize);
		si->SizeOfStruct = sizeof(SYMBOL_INFO);
		si->MaxNameLen   = MAX_SYM_NAME;
		DWORD64 disp = 0;
		if (SymFromAddr(GetCurrentProcess(), (DWORD64)addr, &disp, si) && si->Name[0] != '\0') {
			std::snprintf(out, cap, "%s+0x%llx %s+0x%llx",
				modName,
				(unsigned long long)((uintptr_t)addr - (uintptr_t)mod),
				si->Name, (unsigned long long)disp);
		} else {
			std::snprintf(out, cap, "%s+0x%llx", modName,
				(unsigned long long)((uintptr_t)addr - (uintptr_t)mod));
		}
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
	                  info ? &mei : nullptr, nullptr, nullptr);
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

#ifndef DISABLE_SCRIPTING
namespace ssb64 {

/* Recursively walk mods/, mounting any folder that contains a
 * manifest.json or any .o2r/.otr/.zip file as a separate archive
 * with the LUS ArchiveManager. Folders without a manifest.json are
 * treated as category dirs and traversed for nested mods.
 *
 * This means modders can organize via either layout:
 *
 *   mods/MasterBall/manifest.json            ← flat
 *   mods/items/MasterBall/manifest.json      ← categorical
 *   mods/pokemon/Celebi/manifest.json        ← categorical
 *   mods/some_pack.o2r                       ← packaged
 *
 * Idempotent: AddArchive is only called for paths not already
 * present in the manager's archive list. Safe to call repeatedly
 * (e.g. from the Mods → Reload menu, after a modder dropped a
 * brand-new mod folder in mods/ while the engine was running). */
void MountModsDir() {
	namespace fs = std::filesystem;
	auto rm = sContext ? sContext->GetResourceManager() : nullptr;
	if (!rm) return;
	auto am = rm->GetArchiveManager();
	if (!am) return;

	const fs::path modsDir(ssb64::RealAppBundlePath() + "/mods");
	std::error_code ec;
	if (!fs::exists(modsDir, ec)) {
		return;
	}

	/* Build a set of already-mounted archive paths so we don't
	 * double-mount on a rescan. Path comparison is by exact string;
	 * the LUS manager keeps the path the caller passed to AddArchive. */
	auto existing_list = am->GetArchives();
	std::unordered_set<std::string> existing;
	if (existing_list) {
		for (const auto& a : *existing_list) {
			if (a) existing.insert(a->GetPath());
		}
	}

	auto try_mount = [&](const fs::path& p) {
		const std::string path = p.generic_string();
		if (existing.contains(path)) {
			return;
		}
		if (am->AddArchive(path)) {
			port_log("SSB64: mounted mod archive -> %s\n", path.c_str());
			existing.insert(path);
		} else {
			port_log("SSB64: failed to mount mod archive -> %s\n", path.c_str());
		}
	};

	std::function<void(const fs::path&)> walk = [&](const fs::path& dir) {
		std::error_code ec_local;
		for (const auto& entry : fs::directory_iterator(dir, ec_local)) {
			const fs::path p = entry.path();
			if (entry.is_regular_file(ec_local)) {
				const std::string ext = p.extension().string();
				if (ext == ".o2r" || ext == ".otr" || ext == ".zip") {
					try_mount(p);
				}
				continue;
			}
			if (!entry.is_directory(ec_local)) {
				continue;
			}
			/* A folder containing manifest.json is a mod. Otherwise
			 * treat it as a category directory and recurse. */
			if (fs::exists(p / "manifest.json", ec_local)) {
				try_mount(p);
			} else {
				walk(p);
			}
		}
	};
	walk(modsDir);
}

/* Unmount mod archives whose on-disk source no longer exists. MountModsDir
 * only ever ADDS archives, so a mod folder/.o2r deleted at runtime stays
 * mounted in the ArchiveManager for the rest of the session — Hot Reload would
 * recompile it from the still-mounted VFS and Rescan would still list it.
 * Removing the stale archive here lets both reflect deletions. Only archives
 * carrying a manifest.json are considered, so the base game + shader archives
 * are never touched. Call with mod scripts already unloaded (the loaded image
 * is independent of the archive, but unloading first keeps state consistent). */
void UnmountMissingMods() {
	namespace fs = std::filesystem;
	auto rm = sContext ? sContext->GetResourceManager() : nullptr;
	if (!rm) return;
	auto am = rm->GetArchiveManager();
	if (!am) return;
	auto archives = am->GetArchives();
	if (!archives) return;

	/* Collect first, then remove — don't mutate the manager's list mid-walk. */
	std::vector<std::string> stale;
	for (const auto& a : *archives) {
		if (!a) continue;
		if (!a->HasFile("manifest.json")) continue; /* not a mod */
		const std::string path = a->GetPath();
		std::error_code ec;
		if (!fs::exists(fs::path(path), ec)) {
			stale.push_back(path);
		}
	}
	for (const auto& path : stale) {
		am->RemoveArchive(path);
		port_log("SSB64: unmounted deleted mod archive -> %s\n", path.c_str());
	}
}

} // namespace ssb64
#endif

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

/* ── Console "reset" command ─────────────────────────────────────────────
 * The ESC-menu Reset button (port/gui/Menu.cpp) and the Ctrl/Cmd-R shortcut
 * (libultraship Gui.cpp) both Dispatch("reset") at the LUS console, but the
 * command was never registered, so both fell through to "[LUS] Command not
 * found" and did nothing.
 *
 * The handler performs an in-game reset back to the boot scene — the same
 * "return to title" semantics as a console reset — via the scene manager's
 * normal transition path. The mechanics live decomp-side in
 * portSCManagerRequestReset() (decomp/src/sc/scmanager.c) because the port
 * layer can't include decomp headers (the C shim stdlib shadows libc++ —
 * see the include-path note in CMakeLists.txt) and mirroring the scene
 * struct layout here would invite exactly the layout-drift bugs
 * docs/debug_ido_bitfield_layout.md warns about. */
extern "C" void portSCManagerRequestReset(void);

static int32_t ResetCommandHandler(std::shared_ptr<Ship::Console> console, std::vector<std::string> args,
                                   std::string* output) {
	portSCManagerRequestReset();
	if (output) {
		*output = "Resetting to the opening scene...";
	}
	return 0;
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

	/* App identity comes from CMake (SSB64_APP_NAME = "BattleShip" for US,
	 * "BattleShip-JP" for JP). The shortName scopes libultraship's
	 * app-data directory (Context::GetAppDirectoryPath →
	 * ~/Library/Application Support/<shortName>, $XDG_DATA_HOME/<shortName>,
	 * %APPDATA%\<shortName>), so US and JP get fully separate
	 * saves/config/logs/BattleShip.o2r and can never read each other's
	 * ROM-derived data. The config filename stays fixed — it already
	 * lives inside the per-app (bifurcated) directory. */
#ifndef SSB64_APP_NAME
#define SSB64_APP_NAME "BattleShip"
#endif
	sContext = Ship::Context::CreateUninitializedInstance(
		SSB64_APP_NAME,
		SSB64_APP_NAME,
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

	/* Latch the Classic Co-op menu choice for this launch — the toggle
	 * swaps which CSS overlay Classic mode enters, so it only applies on
	 * the next boot ("(Needs reload)" on the menu widget). */
	port_classic_coop_latch();

#ifdef PORT_HIRES_ENABLED
	// Hi-res texture pack: scan <app-data>/mods/ for GLideN64-named PNGs
	// and register the Fast3D substitution hook. Master enable lives in
	// the gHiResTextures.Enabled CVar (default 1) — toggled from the
	// Assets → Mods menu, takes effect next cache miss. US-only (see
	// CMakeLists.txt — JP builds drop port/hires/ entirely).
	ssb64::hires::HiResPack::Get().Init();
	ssb64_hires_register();
#endif

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

#if defined(__ANDROID__)
		/* Android has no useful exclusive-display-mode fullscreen for this
		 * port. Use SDL_WINDOW_FULLSCREEN_DESKTOP so SDL keeps the native
		 * surface size reported by the OS instead of asking for a fixed
		 * fullscreen mode that can collapse ultrawide devices back to a
		 * compatibility aspect on relaunch. */
		cv->SetInteger("gSdlWindowedFullscreen", 1);
#endif

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

	/* Back the ESC-menu Reset button / Ctrl+Cmd-R Dispatch("reset") with a
	 * real command — see the reset block above PortInit for the mechanism. */
	sContext->GetConsole()->AddCommand("reset", { ResetCommandHandler, "Resets the game to the opening scene" });

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

#if !defined(__ANDROID__)
		// Linux: WMs only show the app icon if SDL_SetWindowIcon is called
		// on the live window. .ico/.icns paths are baked into the .exe /
		// .app on Windows / macOS so this is a no-op there. Android pulls
		// its launcher icon from the APK resources at install time, so
		// the runtime SDL_SetWindowIcon path is skipped entirely there
		// (and port_window_icon.cpp isn't compiled into libmain.so).
		ssb64::SetWindowIcon();
#endif
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
			Ship::Context::GetPathRelativeToAppDirectory(SSB64_O2R_NAME);
		// silent=true: any failure during this auto-attempt should land in
		// the wizard's status text, not a native popup that races the
		// ImGui modal.
		ssb64::ExtractAssetsIfNeeded(targetO2r, /*silent=*/true);
		std::error_code ec;
		// noexcept exists / PortLocateFile rather than the throwing LUS
		// form — issue #58.
		if (!std::filesystem::exists(targetO2r, ec) &&
		    !std::filesystem::exists(PortLocateFile(SSB64_O2R_NAME), ec)) {
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
		// Add the per-region game archive to the running ResourceManager
		// now that it exists (SSB64_O2R_NAME = BattleShip.o2r on US,
		// BattleShip-JP.o2r on JP — see CMakeLists.txt).
		auto am = sContext->GetResourceManager()->GetArchiveManager();

		// Optional: BattleShip.fromsource.o2r contains relocData resources
		// produced by compiling decomp/src/relocData/*.c via the source-
		// compile pipeline (tools/build_reloc_resource.py). Adding it BEFORE
		// the Torch-extracted BattleShip.o2r means the LUS ArchiveManager's
		// FIFO-wins lookup serves source-compiled entries for matching
		// resource paths. The file is gitignored / produced by the
		// BuildBattleShipFromSource CMake target, which is gated on clang
		// availability — when missing the load is silently skipped and
		// runtime behaves exactly as the pre-M2 Torch-only path.
		if (const char *fromsource = std::getenv("SSB64_RELOC_FROMSOURCE");
			fromsource && fromsource[0] == '1') {
			const std::string fs = PortLocateFile("BattleShip.fromsource.o2r");
			if (!fs.empty()) {
				port_log("SSB64: SSB64_RELOC_FROMSOURCE=1 -> adding %s ahead of BattleShip.o2r\n",
				         fs.c_str());
				if (!am->AddArchive(fs)) {
					port_log("SSB64: AddArchive failed for %s (continuing with Torch-extracted reloc data)\n",
					         fs.c_str());
				}
			} else {
				port_log("SSB64: SSB64_RELOC_FROMSOURCE=1 set but BattleShip.fromsource.o2r not found\n");
			}
		}

		const std::string ssb64o2r = PortLocateFile(SSB64_O2R_NAME);
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

	if (!sContext->InitEventSystem()) { port_log("SSB64: InitEventSystem failed\n"); return 1; }
	port_log("SSB64: EventSystem initialized\n");

	// Allocate runtime EventIDs for every event declared in port/hooks/list/*.
	// Must happen AFTER InitEventSystem (PortRegisterEvents calls
	// EventSystemRegisterEvent, which dereferences Context::GetEventSystem()).
	PortRegisterEvents();
	ssb64::RegisterFocusListener();
	port_log("SSB64: Engine events registered\n");

#ifndef DISABLE_SCRIPTING
	// TCC mod scripting: configure the include paths + library paths under
	// .tcc/ that the engine populates post-build (see CMakeLists.txt). On
	// Windows BattleShip.def is an export-name list that ScriptLoader resolves
	// against the running EXE before TCC relocates mods into memory; on Unix
	// mods resolve symbols dynamically via the host process's exported symbols.
	{
		std::unordered_map<std::string, std::string> defines = {
			{ "PORT", "1" },
			{ "REGION_US", "1" },
			{ "VERSION_US", "1" },
			{ "F3DEX_GBI_2", "1" },
			{ "_LANGUAGE_C", "1" },
			{ "_USE_MATH_DEFINES", "1" },
			{ "NON_MATCHING", "1" },
			{ "NON_EQUIVALENT", "1" },
			{ "AVOID_UB", "1" },
		};
		std::vector<std::string> includePaths = {
			Ship::Context::GetPathRelativeToAppDirectory(".tcc/include"),
			Ship::Context::GetPathRelativeToAppDirectory(".tcc/include/tcc"),
			Ship::Context::GetPathRelativeToAppDirectory(".tcc/include/winapi"),
			Ship::Context::GetPathRelativeToAppDirectory(".tcc/include/sys"),
			Ship::Context::GetPathRelativeToAppDirectory(".tcc/include/sec_api"),
		};
		std::vector<std::string> libraryPaths = {
			Ship::Context::GetPathRelativeToAppDirectory(".tcc/lib"),
		};
#ifdef _WIN32
		std::vector<std::string> libraries = { "BattleShip.def", "tcc1" };
#else
		std::vector<std::string> libraries = { "tcc1" };
#endif
		constexpr int kCodeVersion = 1;
		/* -mms-bitfields makes TCC pack bitfields the way MSVC does
		 * (start a new storage unit on type changes, e.g. between
		 * `ub32 X : 1` and `s32 Y : 2`). The engine is built with MSVC
		 * and several decomp structs (notably FTStruct ~lines
		 * 1507-1551) mix `ub32`/`s32`/`u32` bitfield types — without
		 * this flag, TCC packs them all into a single 32-bit unit
		 * while MSVC splits them across multiple units, shifting every
		 * field afterwards (e.g. `attr`, `joints`) and causing mod
		 * reads to land on adjacent function-pointer fields. */
		if (!sContext->InitScriptLoader(defines, kCodeVersion, "-mms-bitfields",
		                                includePaths, libraryPaths, libraries)) {
			port_log("SSB64: InitScriptLoader failed\n");
			return 1;
		}
		port_log("SSB64: ScriptLoader initialized (codeVersion=%d)\n", kCodeVersion);
	}
#endif

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

	/* Fast3D typed-asset factories for deblobbed reloc slices
	 * (docs/deblob.md): the synthesis engine loads these resource types
	 * and re-serializes them into the bundle's big-endian image. */
	loader->RegisterResourceFactory(
		std::make_shared<Fast::ResourceFactoryBinaryDisplayListV0>(),
		RESOURCE_FORMAT_BINARY, "DisplayList",
		static_cast<uint32_t>(Fast::ResourceType::DisplayList), 0);
	loader->RegisterResourceFactory(
		std::make_shared<Fast::ResourceFactoryXMLDisplayListV0>(),
		RESOURCE_FORMAT_XML, "DisplayList",
		static_cast<uint32_t>(Fast::ResourceType::DisplayList), 0);
	loader->RegisterResourceFactory(
		std::make_shared<Fast::ResourceFactoryBinaryLightV0>(),
		RESOURCE_FORMAT_BINARY, "Light",
		static_cast<uint32_t>(Fast::ResourceType::Light), 0);
	loader->RegisterResourceFactory(
		std::make_shared<Fast::ResourceFactoryBinaryMatrixV0>(),
		RESOURCE_FORMAT_BINARY, "Matrix",
		static_cast<uint32_t>(Fast::ResourceType::Matrix), 0);
	loader->RegisterResourceFactory(
		std::make_shared<Fast::ResourceFactoryBinaryTextureV0>(),
		RESOURCE_FORMAT_BINARY, "Texture",
		static_cast<uint32_t>(Fast::ResourceType::Texture), 0);
	loader->RegisterResourceFactory(
		std::make_shared<Fast::ResourceFactoryBinaryTextureV1>(),
		RESOURCE_FORMAT_BINARY, "Texture",
		static_cast<uint32_t>(Fast::ResourceType::Texture), 1);
	loader->RegisterResourceFactory(
		std::make_shared<Fast::ResourceFactoryBinaryVertexV0>(),
		RESOURCE_FORMAT_BINARY, "Vertex",
		static_cast<uint32_t>(Fast::ResourceType::Vertex), 0);
	loader->RegisterResourceFactory(
		std::make_shared<Fast::ResourceFactoryXMLVertexV0>(),
		RESOURCE_FORMAT_XML, "Vertex",
		static_cast<uint32_t>(Fast::ResourceType::Vertex), 0);

	port_log("SSB64: Resource factories registered\n");

#ifndef DISABLE_SCRIPTING
	/* Mod runtime: bring up symbol resolution + the hook table so TCC
	 * mods can install hooks during their MOD_INIT below. Hooks land on
	 * engine functions in memory; the game coroutine started later via
	 * PortGameInit() picks them up automatically because the patches
	 * are in the running .text. */
	if (!ssb64::mods::SymbolResolver::Init()) {
		port_log("SSB64: SymbolResolver::Init failed - mods will not load\n");
	}
	if (!ssb64::mods::HookManager::Init()) {
		port_log("SSB64: HookManager::Init failed - mods will not load\n");
	}
#endif

	/* Seed the per-fkind dispatch registry from the vanilla decomp arrays
	 * BEFORE any mod runs. Mods can overwrite vanilla rows or add synth
	 * rows past nFTKindEnumCount via port_fighter_register at MOD_INIT;
	 * the engine reads through registry accessors instead of fixed-size
	 * vanilla arrays, so synth fkinds never OOB those tables. */
	port_fighter_seed_vanilla();
	port_log("SSB64: fighter registry seeded\n");

#ifndef DISABLE_SCRIPTING
	/* Mount mods/ entries (folders, .o2r, .zip) into the LUS
	 * ArchiveManager so ScriptLoader can iterate them. */
	ssb64::MountModsDir();

	// TCC scripting: compile + load any .o2r / folder mod under mods/ that
	// declares a `main` entry in its manifest.json. Each mod's source files
	// are amalgamated by the ScriptLoader, compiled and relocated into memory
	// via libtcc, and ModInit is called by name. The pre/post-init callbacks
	// tag every HookManager::InstallHook call with the current mod name so
	// hot-reload can selectively uninstall hooks per mod without touching others.
	if (auto scripting = sContext->GetScriptLoader()) {
		try {
			scripting->CompileAll();
			scripting->LoadAll(
				/*preInit=*/[](const std::string& mod) {
					ssb64::mods::HookManager::SetCurrentOwner(mod.c_str());
				},
				/*postInit=*/[](const std::string&) {
					ssb64::mods::HookManager::ClearCurrentOwner();
				});
			port_log("SSB64: TCC scripted mods compiled + loaded\n");
		} catch (const std::exception& e) {
			port_log("SSB64: TCC ScriptLoader threw — continuing without scripted mods: %s\n", e.what());
		}
	}
#endif

	port_log("SSB64: init complete\n");
	return 0;
}

void PortShutdown(void) {
#ifndef DISABLE_SCRIPTING
	// Tear down mod hooks first so no replacement function fires during
	// engine shutdown (replacement code might call into ssb64_game state
	// that's about to be torn down).
	ssb64::mods::HookManager::Shutdown();
	ssb64::mods::SymbolResolver::Shutdown();

	// Unload mod scripts now, while the Context's EventSystem is still fully
	// alive, so each mod's ModExit (which calls UNREGISTER_LISTENER) can reach
	// it. Otherwise the only UnloadAll happens inside ~Context via the
	// sContext.reset() below — by which point the EventSystem is being torn
	// down, so EventSystemUnregisterListener -> Context::GetEventSystem
	// dereferences a dead shared_ptr<EventSystem> and crashes on exit whenever
	// a listener-registering mod (e.g. one that REGISTER_LISTENERs in ModInit)
	// is loaded. The later ~Context UnloadAll then finds nothing loaded.
	if (sContext) {
		if (auto scripting = sContext->GetScriptLoader()) {
			scripting->UnloadAll();
		}
	}
#endif

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
#if defined(__ANDROID__)
	// Detach the touch-overlay virtual joystick and reset its statics while
	// SDL is still up. The process survives Activity relaunches on Android,
	// so stale handles here become use-after-free in the next SDL session.
	extern void port_touch_overlay_shutdown(void);
	port_touch_overlay_shutdown();
#endif
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
		if (char* p = SDL_GetPrefPath(NULL, "BattleShip")) {
			logPath = std::string(p) + "ssb64.log";
			SDL_free(p);
		} else {
			logPath = "ssb64.log";  // last-resort cwd fallback
		}
		port_log_init(logPath.c_str());
	}

#ifdef __APPLE__
	/* Disable the macOS press-and-hold accent/diacritic popup for this app.
	 * SDL keeps a Cocoa text-input context alive for the game window, so
	 * holding a movement key (e.g. WASD) makes AppKit pop the accent picker
	 * instead of delivering key-repeat — the held key stops registering as
	 * down. AppKit reads ApplePressAndHoldEnabled from the app's user
	 * defaults when the text-input context is first created, so this must
	 * run before SDL creates the window (i.e. before PortInit). Writing it
	 * per-app (kCFPreferencesCurrentApplication) leaves the global/system
	 * default untouched. Key repeat still works. */
	CFPreferencesSetAppValue(CFSTR("ApplePressAndHoldEnabled"), kCFBooleanFalse,
	                         kCFPreferencesCurrentApplication);
	CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
#endif

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

#if defined(__ANDROID__)
	// === Android JNI cache warm-up ===
	//
	// Our cooperative scheduler runs as port_coroutines (aarch64 fibers)
	// stack-switched on the SDL_main thread. ART tracks JNI transition
	// frames per OS thread via a ManagedStack list whose head lives on
	// the native stack — when port_coroutine_swap moves SP to a different
	// fiber, the head pointer dangles and any JNI call from the fiber
	// aborts with "invalid JNI transition frame reference".
	//
	// SDL2's Android backend lazy-initializes a few caches via JNI on
	// first use; if the first use happens from a coroutine, we crash.
	// Force-warm them here on the real thread so subsequent reads from
	// inside coroutines hit the in-process cache without re-entering JNI.

	// 1. SDL_INIT_GAMECONTROLLER → HIDDeviceManager.initialize.
	SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
	if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) {
		port_log("SSB64: pre-init SDL_INIT_GAMECONTROLLER failed: %s\n",
		         SDL_GetError());
	}

	// Prefer AAudio (low-latency) only where it exists — API 26+. AAudio's
	// libaaudio.so is absent below that, and SDL_AudioInit does NOT fall back
	// once a driver name is pinned (it fails with "Audio target not
	// available"), so forcing it on older devices kills audio outright. Below
	// 26 we leave the hint unset and let SDL auto-select OpenSL ES.
	if (android_get_device_api_level() >= 26) {
		SDL_SetHint(SDL_HINT_AUDIODRIVER, "aaudio");
	}

	// 2. Suppress ImGui's per-frame SDL_GetDisplayUsableBounds JNI path.
	//    ImGui_ImplSDL2_UpdateMonitors runs on the SSB64 GFX coroutine
	//    every frame; it calls SDL_GetDisplayUsableBounds →
	//    ParseDisplayUsableBoundsHint → SDL_GetHint(SDL_HINT_DISPLAY_USABLE_BOUNDS).
	//    On Android SDL_GetHint falls through to SDL_getenv, which
	//    Binder-IPCs into Java's PackageManager.getApplicationInfo —
	//    that fails CheckJNI ("invalid JNI transition frame reference")
	//    when called from inside a port_coroutine fiber.
	//
	//    Setting the hint here populates SDL2's per-program hint store;
	//    SDL_GetHint then returns from the local cache without env
	//    lookup, never re-entering JNI from the coroutine. Query the real
	//    display bounds while we're still on the JVM-attached SDL_main
	//    thread (the hint is unset yet, so this hits the actual display,
	//    not the hint) — a hardcoded 1080p clamp misplaces floating ImGui
	//    windows on ultrawide/high-res devices. Fall back to 1080p only
	//    if the query fails.
	{
		char bounds[64] = "0,0,1920,1080";
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) == 0) {
			SDL_Rect r;
			if (SDL_GetDisplayUsableBounds(0, &r) == 0 && r.w > 0 && r.h > 0) {
				SDL_snprintf(bounds, sizeof(bounds), "%d,%d,%d,%d", r.x, r.y, r.w, r.h);
			}
		}
		SDL_SetHint(SDL_HINT_DISPLAY_USABLE_BOUNDS, bounds);
	}
	// Warm bHasEnvironmentVariables so any other SDL_getenv that we
	// missed is also cached. The SDL_main thread is JVM-attached at
	// this point, so the JNI roundtrip succeeds.
	(void)SDL_getenv("__ssb64_jni_warmup__");
#endif

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
