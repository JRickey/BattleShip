/**
 * gameloop.cpp — PC game loop implementation for the SSB64 port.
 *
 * This file implements the frame loop that drives SSB64's game logic on PC.
 * The N64 game uses 5 OS threads communicating via message queues. On PC,
 * we run the entire game in a single coroutine that yields at blocking
 * points (osRecvMesg with OS_MESG_BLOCK).
 *
 * Architecture:
 *   main() loop
 *     -> PortPushFrame()
 *       -> post INTR_VRETRACE to scheduler queue
 *       -> resume game coroutine
 *         -> scheduler coroutine processes tick (sends to clients)
 *         -> game logic runs one frame
 *         -> display list built, submitted to scheduler
 *         -> scheduler calls osSpTaskStartGo -> port_submit_display_list
 *         -> Fast3D renders via DrawAndRunGraphicsCommands
 *         -> game coroutine yields at next osRecvMesg(BLOCK) on empty queue
 */

#include "gameloop.h"
#include "coroutine.h"
#include "enhancements/enhancements.h"
#include "widescreen/widescreen.h"
#include "port.h"
#include "port_watchdog.h"

#include <libultraship/libultraship.h>
#include <fast/Fast3dWindow.h>
#include <fast/interpreter.h>

#include <cstdio>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <string>
#include <cstdlib>
#include <cstring>

/* GBI trace system */
#include "../debug_tools/gbi_trace/gbi_trace.h"

#include "port_log.h"
#include "renderdoc_trigger.h"

/* Backbuffer screenshot capture — implemented in libultraship's DX11 backend.
 * Returns 1 on success, 0 on failure (silent). Never throws. */
extern "C" int portFastCaptureBackbufferPNG(const char *path);

/* Simulated N64 VI VBlank: propagates the queued framebuffer to "current"
 * so the scheduler's CheckReadyFramebuffer fnCheck can correctly stall a
 * gfx task when VI is still using the slot the game wants to draw into.
 * Implemented in port/stubs/n64_stubs.c. */
extern "C" void port_vi_simulate_vblank(void);

extern "C" void lbBackupApplyCheats(void);

/* ========================================================================= */
/*  External game symbols (C linkage)                                        */
/* ========================================================================= */

extern "C"
{

	/* The N64 game entry point — starts the whole boot chain. */
	extern void syMainLoop(void);

	/* Scheduler task message queue — we post INTR_VRETRACE here each frame. */
	extern OSMesgQueue gSYSchedulerTaskMesgQueue;

/* VI retrace interrupt value (matches scheduler.c local define). */
#define INTR_VRETRACE 1

} /* extern "C" */

/*
 * port_make_os_mesg_int() — fully-initialised OSMesg from an integer code.
 *
 * The decomp treats OSMesg as `void*` (PR/os.h); libultraship treats it as a
 * union { u8; u16; u32; void*; } (libultraship/libultra/message.h). Both are
 * 8 bytes, so the calling convention matches, but writing only one union
 * member from C++ leaves the remaining bytes uninitialised — whatever was on
 * the stack in that slot. On MSVC/x64 those bytes happened to be zero and
 * the bug was latent; on macOS/arm64 they held a stack pointer, and the
 * scheduler's `(SYTaskInfo*)mesg` cast in sySchedulerThreadMain then jumped
 * through a garbage `fnCheck` pointer.
 *
 * All port-layer (C++) sends of integer interrupt codes should go through
 * this helper so the full 8 bytes are well-defined on every platform.
 */
static inline OSMesg port_make_os_mesg_int(uint32_t code)
{
	OSMesg m{};		 /* zero-initialise every union member */
	m.data32 = code; /* then set the scalar we care about */
	return m;
}

/* ========================================================================= */
/*  Game coroutine state                                                     */
/* ========================================================================= */

static PortCoroutine *sGameCoroutine = NULL;

/* ========================================================================= */
/*  Game coroutine entry point                                               */
/* ========================================================================= */

/**
 * Wrapper for syMainLoop that matches the coroutine entry signature.
 *
 * syMainLoop creates Thread 1 (idle), which creates Thread 5 (game).
 * Each osStartThread creates a sub-coroutine. When Thread 1 finishes
 * (after starting Thread 5 and returning on PORT), syMainLoop returns.
 *
 * At that point, Thread 5's coroutine exists but is suspended. We need
 * to keep the game coroutine alive and act as the "scheduler" that
 * resumes Thread 5 (and other service threads) each frame.
 *
 * The game coroutine entry function runs syMainLoop (boot), then enters
 * an infinite yield loop. PortPushFrame resumes it each frame, and it
 * yields back immediately — the actual frame work happens when
 * PortPushFrame resumes the individual thread coroutines.
 */
static void game_coroutine_entry(void *arg)
{
	(void)arg;
	port_log("SSB64: Game coroutine started — entering syMainLoop\n");
	syMainLoop();
	port_log("SSB64: syMainLoop returned — boot chain complete\n");
	/* All thread coroutines are now created and suspended.
	 * PortPushFrame will resume them directly via port_resume_service_threads. */
}

/* ========================================================================= */
/*  Display list submission                                                  */
/* ========================================================================= */

/**
 * Called from osSpTaskStartGo (n64_stubs.c) when a GFX task is submitted.
 * Routes the N64 display list through Fast3D for rendering.
 */
static int sGbiTraceInitDone = 0;

/* Per-DL gfx-task wall-clock cost estimator (Phase 3 of the freeze-frame
 * fix). Each command processed by Fast3D's interpreter fires this
 * callback; we accumulate a coarse cost estimate from F3DEX opcodes that
 * dominate real-hw RDP/RSP budget on N64 (triangles, fillrects, texrects).
 *
 * After the DL is fully processed, port_get_last_dl_defer_n() turns the
 * accumulated cost into the deferral count N for the SP/DP-interrupt
 * delay queue in osSpTaskStartGo: light DLs N=1 (no SwitchContext
 * contention, smooth play), heavy climax DLs N=2 (contention triggers
 * the game-thread cap which holds one host frame = visible 1-frame
 * whole-screen freeze, matching real-hw / parallel-rdp behavior).
 *
 * F3DEX opcode reference (SSB64 ucode is gsp.fifo / F3DEX). Top byte of
 * w0 is the opcode. */
static int sFrameTriCount = 0;
static int sFrameRectPx = 0;
static int sFrameLoadBytes = 0;
static int sLastDLTris = 0;
static int sLastDLRectPx = 0;
static int sLastDLLoadBytes = 0;

/* Thin C wrapper for the trace callback (matches GbiTraceCallbackFn signature) */
static void gbi_trace_callback(uintptr_t w0, uintptr_t w1, int dl_depth)
{
	gbi_trace_log_cmd((unsigned long long)w0, (unsigned long long)w1, dl_depth);

	uint8_t opcode = (uint8_t)((w0 >> 24) & 0xFFu);
	switch (opcode)
	{
	/* SSB64 ucode is F3DEX2 (gsp.fifo). Tri opcodes are 0x05/0x06/0x07,
	 * NOT F3DEX's 0xBF/0xB1/0xB5 — confirmed by histogram showing 0x05
	 * and 0x06 emitted in the hundreds per frame. RDP rect/load opcodes
	 * (0xE4, 0xF3, 0xF4, 0xF6) are shared across F3DEX/F3DEX2. */
	case 0x05: /* F3DEX2 G_TRI1 — 1 triangle */
		sFrameTriCount += 1;
		break;
	case 0x06: /* F3DEX2 G_TRI2 — 2 triangles */
	case 0x07: /* F3DEX2 G_QUAD — 2 triangles */
		sFrameTriCount += 2;
		break;
	case 0xE4: /* G_TEXRECT, fillrate-bound */
	case 0xF6: /* G_FILLRECT */
	{
		/* Both encode (ulx,uly,lrx,lry) in 10.2 fixed-point.
		 * G_TEXRECT: w0 = 0xE4 | lrx<<12 | lry; w1 = tile<<24 | ulx<<12 | uly
		 * G_FILLRECT: w0 = 0xF6 | lrx<<12 | lry; w1 = ulx<<12 | uly
		 * Pixel area = (lrx - ulx) * (lry - uly) >> 4 (10.2 → integer). */
		int lrx = (int)((w0 >> 12) & 0xFFF);
		int lry = (int)(w0 & 0xFFF);
		int ulx = (int)((w1 >> 12) & 0xFFF);
		int uly = (int)(w1 & 0xFFF);
		int w_px = (lrx - ulx) >> 2;
		int h_px = (lry - uly) >> 2;
		if (w_px > 0 && h_px > 0)
		{
			sFrameRectPx += w_px * h_px;
		}
		break;
	}
	case 0xF3: /* G_LOADBLOCK */
	case 0xF4: /* G_LOADTILE */
		/* Coarse: each load contributes ~512 bytes worst case. Real cost
		 * depends on tex size but we just want order-of-magnitude. */
		sFrameLoadBytes += 512;
		break;
	default:
		break;
	}
}

/* Called by osSpTaskStartGo after Fast3D has processed the DL. Returns
 * the deferral count N for the SP/DP-interrupt delay queue.
 *
 * Heuristic linear cost: each tri ~= 75 RSP cycles, each fillrect/texrect
 * pixel ~= 1 RDP cycle, each load ~= the byte count. Budget is one VI
 * period ≈ 1.04M RCP cycles at 62.5 MHz. If accumulated cost exceeds the
 * budget, the DL would have overrun on real hw → return N=2 (force a
 * 1-frame contention next VI). Otherwise return N=1.
 *
 * Tunable via SSB64_RCP_CYCLE_BUDGET (default 1040000). Lower budget →
 * more freezes; higher → fewer. SSB64_RCP_FORCE_N overrides cost model
 * with a fixed N for testing (matches old SSB64_GFX_DEFER_VI semantics). */
extern "C" int port_get_last_dl_defer_n(void)
{
	static int sBudget = -1;
	static int sForceN = -1;
	static int sRectGate = -1;
	if (sBudget < 0)
	{
		/* Calibrated empirically against attract-mode DL cost histogram:
		 * p99 ≈ 394k, p99.5 ≈ 397k, max observed = 415k. Setting the
		 * budget at 400k means only the top ~0.1% of DLs trigger a
		 * delayed completion, giving a sparse climax-only freeze rather
		 * than uniform stutter.
		 * See `docs/freeze_frame_rcp_clock_design_2026-04-26.md` for
		 * the full design + tuning rationale. */
		const char *env = std::getenv("SSB64_RCP_CYCLE_BUDGET");
		sBudget = (env != NULL) ? std::atoi(env) : 400000;
		if (sBudget < 1000)
			sBudget = 1000;
		const char *fenv = std::getenv("SSB64_RCP_FORCE_N");
		sForceN = (fenv != NULL) ? std::atoi(fenv) : 0;
		/* Fillrate gate: a DL must ALSO push significant rect_px (texrect /
		 * fillrect coverage) before we treat it as an authored freeze.
		 * Empirical observation 2026-05-01: high-tri fighter-model DLs in
		 * the attract run-sequence cluster at cost 400–419k with rect_px
		 * 148–167k (false positives), while authored-climax effect DLs
		 * have rect_px ≥ 260k. The cost-only model couldn't distinguish.
		 * Default 200k sits cleanly between the two distributions. */
		const char *renv = std::getenv("SSB64_RCP_RECT_GATE");
		sRectGate = (renv != NULL) ? std::atoi(renv) : 200000;
		if (sRectGate < 0)
			sRectGate = 0;
		port_log("SSB64: RCP cost model — budget=%d cycles/VI, force_n=%d (0=cost-model, >=1=fixed), rect_gate=%d\n",
				 sBudget, sForceN, sRectGate);
	}
	if (sForceN >= 1)
		return sForceN;

	/* Scene-level allowlist gate. The cost model exists to recreate the
	 * authored climax-freeze frames in cutscenes / opening sequences /
	 * fighter intros (where original-game scene authors timed dialogue,
	 * camera, and audio around the natural N64 RDP overrun). It has no
	 * business firing in interactive menus or CSS — there are no authored
	 * freezes there to preserve, and the cost+rect heuristic mis-fires on
	 * UI-fillrate-heavy DLs (4-fighter VS CSS panels cluster at cost ≈420k,
	 * rect_px ≈236k, which trips both gates every frame and halves the
	 * game-tick rate). Issue #78. */
	extern unsigned char port_diag_get_scene_curr(void);
	extern int port_scene_wants_freeze_simulation(unsigned char scene_id);
	if (!port_scene_wants_freeze_simulation(port_diag_get_scene_curr()))
	{
		return 1;
	}

	int cost = sLastDLTris * 75 + sLastDLRectPx + sLastDLLoadBytes;
	/* Heavy-DL deferral. The port's coroutine scheduler observes the held
	 * framebuffer one host frame after slot contention, so climax DLs need
	 * one extra VI of synthetic RCP latency beyond the first visible stall.
	 * This shifts all authored freezes together (portrait banners, fighter
	 * poses, stage cuts) instead of patching scene timers individually. */
	int n;
	if (cost < sBudget)
	{
		n = 1;
	}
	else if (sLastDLRectPx < sRectGate)
	{
		/* Cost is over budget but the load is dominated by triangles, not
		 * fillrate — looks like a fighter model render, not an authored
		 * full-screen effect. Don't freeze. */
		n = 1;
	}
	else
	{
		n = 3;
	}
	if (n < 1)
		n = 1;
	if (n > 3)
		n = 3;
	return n;
}

/* Tracks whether this VI period submitted a rendered gfx task. On real N64,
 * VI still scans out the current framebuffer when a period has no new task;
 * the port mirrors that below by presenting the cached Fast3D framebuffer. */
static int sDLSubmitsThisFrame = 0;

extern "C" void port_submit_display_list(void *dl)
{
	/* Lazy-init the GBI trace system on first DL submit. Always install
	 * the callback because Phase 3's per-DL cost model also runs through
	 * it — gbi_trace_log_cmd is the no-op fast path when tracing is off. */
	sDLSubmitCount++;
#ifndef __SWITCH__
	if (sDLSubmitCount <= 60 || (sDLSubmitCount % 60 == 0))
	{
		port_log("SSB64: port_submit_display_list #%d dl=%p\n", sDLSubmitCount, dl);
	}
#endif

	/* Lazy-init the GBI trace system on first DL submit */
	if (!sGbiTraceInitDone)
	{
		sGbiTraceInitDone = 1;
		gbi_trace_init();
		gfx_set_trace_callback(gbi_trace_callback);
	}

	/* Reset per-DL accumulators before Fast3D walks the DL. */
	sFrameTriCount = 0;
	sFrameRectPx = 0;
	sFrameLoadBytes = 0;

	if (dl == NULL)
	{
		port_log("SSB64: WARNING — display list is NULL!\n");
		return;
	}

	auto context = Ship::Context::GetInstance();
	if (!context)
	{
		port_log("SSB64: WARNING — no Ship::Context in display list submit!\n");
		return;
	}

	auto window = std::dynamic_pointer_cast<Fast::Fast3dWindow>(context->GetWindow());
	if (!window)
	{
		port_log("SSB64: WARNING — no Fast3dWindow in display list submit!\n");
		return;
	}

	/* Begin trace frame before Fast3D processes the display list */
	gbi_trace_begin_frame();

	std::unordered_map<Mtx *, MtxF> mtxReplacements;
	try
	{
		window->DrawAndRunGraphicsCommands(static_cast<Gfx *>(dl), mtxReplacements);
	}
	catch (long hr)
	{
		port_log("SSB64: CAUGHT DX shader exception HRESULT=0x%08lX\n", hr);
		gbi_trace_end_frame();
		return;
	}
	catch (...)
	{
		port_log("SSB64: CAUGHT unknown exception while processing display list\n");
		gbi_trace_end_frame();
		return;
	}

	sDLSubmitsThisFrame++;

	/* End trace frame after processing */
	gbi_trace_end_frame();

	/* Capture this DL's cost so port_get_last_dl_defer_n() can use it
	 * to set N for THIS task's SP/DP-interrupt deferral. osSpTaskStartGo
	 * reads it AFTER port_submit_display_list returns. */
	sLastDLTris = sFrameTriCount;
	sLastDLRectPx = sFrameRectPx;
	sLastDLLoadBytes = sFrameLoadBytes;
}

/* ========================================================================= */
/*  Public API                                                               */
/* ========================================================================= */

void PortGameInit(void)
{
	port_log("SSB64: PortGameInit — initializing coroutine system\n");

	/* Convert the main thread to a fiber so it can participate in
	 * coroutine switching. */
	port_coroutine_init_main();

	/* Create the game coroutine with a large stack (1 MB).
	 * This will host the entire game: syMainLoop -> Thread 1 -> Thread 5
	 * -> scheduler, controller, audio init -> scManagerRunLoop. */
	sGameCoroutine = port_coroutine_create(game_coroutine_entry, NULL, 1024 * 1024);
	if (sGameCoroutine == NULL)
	{
		port_log("SSB64: FATAL — failed to create game coroutine\n");
		return;
	}

	/* Resume the game coroutine to start the boot chain.
	 * It will run through syMainLoop -> osInitialize -> create Thread 1
	 * -> start Thread 1 -> Thread 1 creates Thread 5 -> Thread 5 inits
	 * peripherals, creates scheduler/audio/controller threads.
	 *
	 * Each thread runs in its own sub-coroutine (created by osStartThread).
	 * They all yield when they hit osRecvMesg(BLOCK) on empty queues.
	 * Eventually control returns here after the entire boot chain has
	 * progressed as far as it can without VI ticks. */
	port_log("SSB64: Starting game coroutine (boot sequence)\n");
	port_coroutine_resume(sGameCoroutine);
	port_log("SSB64: Boot sequence yielded — entering frame loop\n");

	/* Start the hang watchdog now that the boot chain has yielded back.
	 * This avoids firing false alarms during the long synchronous boot. */
	port_watchdog_init();
}

static int sFrameCount = 0;

/* ========================================================================= */
/*  Screenshot capture (env-var driven)                                      */
/* ========================================================================= */
/*
 * Env vars:
 *   SSB64_SCREENSHOT_FRAMES=10,55,100  — frames to capture
 *   SSB64_SCREENSHOT_FRAMES=all        — capture every frame (heavy!)
 *   SSB64_SCREENSHOT_DIR=<path>        — output dir (default debug_traces/screenshots)
 *
 * When unset/empty, feature is OFF and adds only one bool check per frame.
 */

static bool sScreenshotInited = false;
static bool sScreenshotEnabled = false;
static bool sScreenshotAllFrames = false;
static std::unordered_set<int> sScreenshotFrames;
static std::string sScreenshotDir;

static void port_screenshot_init_once(void)
{
	if (sScreenshotInited)
	{
		return;
	}
	sScreenshotInited = true;

	const char *frames_env = std::getenv("SSB64_SCREENSHOT_FRAMES");
	if (frames_env == nullptr || frames_env[0] == '\0')
	{
		sScreenshotEnabled = false;
		return;
	}

	const char *dir_env = std::getenv("SSB64_SCREENSHOT_DIR");
	sScreenshotDir = (dir_env != nullptr && dir_env[0] != '\0')
						 ? dir_env
						 : "debug_traces/screenshots";

	if (std::strcmp(frames_env, "all") == 0)
	{
		sScreenshotAllFrames = true;
	}
	else
	{
		/* Parse comma-separated integer list. Ignores malformed tokens. */
		const char *p = frames_env;
		while (*p != '\0')
		{
			while (*p == ',' || *p == ' ' || *p == '\t')
			{
				p++;
			}
			if (*p == '\0')
			{
				break;
			}
			char *end = nullptr;
			long v = std::strtol(p, &end, 10);
			if (end != p && v >= 0 && v <= 0x7FFFFFFF)
			{
				sScreenshotFrames.insert(static_cast<int>(v));
			}
			if (end == p)
			{
				/* Couldn't parse — skip a char to avoid infinite loop. */
				p++;
			}
			else
			{
				p = end;
			}
		}
	}

	sScreenshotEnabled = sScreenshotAllFrames || !sScreenshotFrames.empty();

	if (sScreenshotEnabled)
	{
		std::error_code ec;
		std::filesystem::create_directories(sScreenshotDir, ec);
		if (ec)
		{
			port_log("SSB64: screenshot: failed to create dir '%s': %s\n",
					 sScreenshotDir.c_str(), ec.message().c_str());
		}
		if (sScreenshotAllFrames)
		{
			port_log("SSB64: screenshot capture ENABLED (all frames) dir='%s'\n",
					 sScreenshotDir.c_str());
		}
		else
		{
			port_log("SSB64: screenshot capture ENABLED (%zu frames) dir='%s'\n",
					 sScreenshotFrames.size(), sScreenshotDir.c_str());
		}
	}
}

static void port_screenshot_maybe_capture(int frame)
{
	if (!sScreenshotEnabled)
	{
		return;
	}
	if (!sScreenshotAllFrames && sScreenshotFrames.count(frame) == 0)
	{
		return;
	}

	char path[1024];
	std::snprintf(path, sizeof(path), "%s/frame_%d.png",
				  sScreenshotDir.c_str(), frame);

	int ok = portFastCaptureBackbufferPNG(path);
	if (ok)
	{
		port_log("SSB64: screenshot frame %d -> %s\n", frame, path);
	}
	else
	{
		port_log("SSB64: screenshot frame %d FAILED -> %s\n", frame, path);
	}
}

void PortPushFrame(void)
{
	// Process cheats safely before the frame updates
	lbBackupApplyCheats();

	/* Capture the wall-clock start of this PortPushFrame for the
	 * frame-pacing fallback below. */
	auto frameStart = std::chrono::steady_clock::now();

	/* Pump SDL events so the window stays responsive and WindowIsRunning
	 * detects the close button. HandleEvents also updates controller state. */
	auto context = Ship::Context::GetInstance();
	if (context)
	{
		auto window = context->GetWindow();
		if (window)
		{
			window->HandleEvents();
		}
	}
	/* Propagate the previous frame's queued framebuffer (if any) to VI's
	 * "current" slot. The scheduler's CheckReadyFramebuffer fnCheck reads
	 * osViGetCurrent/NextFramebuffer to decide whether the slot the game
	 * wants to draw into is still locked by VI; without this rotation
	 * those getters would always report NULL and the scheduler would
	 * never stall, so the intentional freeze frames during fighter
	 * intros and the desk-to-stage transition would never appear. Run
	 * this BEFORE posting INTR_VRETRACE so old gfx completions are
	 * delivered before the next game tick can tear down scene memory. */
	port_vi_simulate_vblank();

	/* Post a VI retrace event to the scheduler's message queue. See
	 * port_make_os_mesg_int() above for why we don't just write
	 * `(OSMesg)INTR_VRETRACE` here. */
	osSendMesg(&gSYSchedulerTaskMesgQueue, port_make_os_mesg_int(INTR_VRETRACE), OS_MESG_NOBLOCK);

	/* Resume all service thread coroutines that are waiting for messages.
	 * This runs multiple rounds to handle cascading messages:
	 *   Round 1: Scheduler picks up VRETRACE, sends ticks to clients
	 *   Round 2: Controller reads input, game logic runs one frame
	 *   Round 3+: Display list submitted, scheduler processes GFX task, etc.
	 * Each thread runs until it yields at osRecvMesg(BLOCK) on empty queue. */
	port_resume_service_threads();

	port_enhancement_stage_hazards_tick();
	port_widescreen_tick();

	sFrameCount++;

	/* VI-style idle presentation: when no gfx task was submitted this VI,
	 * original hardware still scans out the current RDRAM framebuffer.
	 * Fast3D normally presents only from DrawAndRunGraphicsCommands(), so
	 * 0-submit frames can otherwise hold an older swapchain image. Present
	 * the cached game framebuffer texture through the normal GUI/window path
	 * without re-running any display list or touching game memory. */
	if (sDLSubmitsThisFrame == 0)
	{
		bool idlePresented = false;
		static int sFreezePacing = -1;
		if (sFreezePacing < 0)
		{
			const char *env = std::getenv("SSB64_FREEZE_PACING");
			sFreezePacing = (env != nullptr) ? std::atoi(env) : 1;
		}
		if (sFreezePacing)
		{
			auto context = Ship::Context::GetInstance();
			auto window = context
							  ? std::dynamic_pointer_cast<Fast::Fast3dWindow>(context->GetWindow())
							  : nullptr;
			if (window)
			{
				idlePresented = window->PresentCurrentFramebuffer();
			}

			if (!idlePresented)
			{
				/* Fallback pace to one VI period if there is no cached
				 * framebuffer yet. Normal idle presents pace through the
				 * backend's SwapBuffers path. */
				auto target = frameStart + std::chrono::microseconds(16667);
				auto coarseTarget = target - std::chrono::microseconds(2000);
				auto now = std::chrono::steady_clock::now();
				if (now < coarseTarget)
				{
					std::this_thread::sleep_for(coarseTarget - now);
				}
				while (std::chrono::steady_clock::now() < target)
				{
					/* busy-wait */
				}
			}
		}
	}
	sDLSubmitsThisFrame = 0;

	/* Tell the hang watchdog a frame completed. */
	port_watchdog_note_frame_end();

	/* Screenshot capture: env-var driven, zero cost when disabled. */
	port_screenshot_init_once();
	port_screenshot_maybe_capture(sFrameCount);

	/* RenderDoc capture trigger: env-var driven, zero cost when disabled.
	 * TriggerCapture() tells RenderDoc to capture the NEXT Present interval,
	 * so calling it here causes frame (sFrameCount + 1) to be captured.
	 * The one-frame lag is small and consistent — document it in the
	 * feature's env-var contract. */
	portRenderDocOnFrame(static_cast<unsigned int>(sFrameCount));

	{
		static auto sStartTime = std::chrono::steady_clock::now();
		auto now = std::chrono::steady_clock::now();
		double elapsed = std::chrono::duration<double>(now - sStartTime).count();
#ifndef __SWITCH__
		if (sFrameCount <= 60 || (sFrameCount % 60 == 0))
		{
			port_log("SSB64: Frame %d complete (t=%.2fs)\n", sFrameCount, elapsed);
		}
#endif
	}
}

void PortGameShutdown(void)
{
	port_watchdog_shutdown();
	if (sGameCoroutine != NULL)
	{
		port_coroutine_destroy(sGameCoroutine);
		sGameCoroutine = NULL;
	}
	port_log("SSB64: Game coroutine destroyed\n");
}
