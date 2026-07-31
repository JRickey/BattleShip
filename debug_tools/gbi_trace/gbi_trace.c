/**
 * gbi_trace.c — Port-side GBI display list trace system
 *
 * Writes structured per-frame traces to debug_traces/ directory.
 * Output format is designed to be diffed against M64P trace plugin output.
 */
#include "gbi_trace.h"
#include "gbi_decoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#define MKDIR(path) mkdir(path, 0755)
#endif

/* ========================================================================= */
/*  State                                                                    */
/* ========================================================================= */

static int      sEnabled     = 0;
static int      sInitialized = 0;
static int      sFrameNum    = 0;
static int      sCmdIndex    = 0;
static int      sMaxFrames   = 300;  /* default: 5 seconds */
static int      sStartFrame  = 0;    /* SSB64_GBI_TRACE_START: skip until this VI frame */
static int      sViFrame     = 0;    /* VI frame index of the DL being traced (set by gameloop) */
static int      sSkipping    = 0;    /* current frame is before sStartFrame */
static int      sFlushIndex  = 0;    /* backend draw calls this frame (see gbi_trace_note_flush) */
static FILE    *sTraceFile   = NULL;
static char     sTraceDir[512] = "debug_traces";

/* Per-frame rolling file or single file modes */
/* We use a single file with frame delimiters for easier diffing */
static char     sTraceFilePath[1024];

/* ========================================================================= */
/*  Initialization                                                           */
/* ========================================================================= */

void gbi_trace_init(void)
{
	const char *env;

	if (sInitialized) return;
	sInitialized = 1;

	env = getenv("SSB64_GBI_TRACE");
	if (env && (env[0] == '1' || env[0] == 'y' || env[0] == 'Y')) {
		sEnabled = 1;
	}

	env = getenv("SSB64_GBI_TRACE_FRAMES");
	if (env) {
		int val = atoi(env);
		if (val > 0) sMaxFrames = val;
		if (val == 0) sMaxFrames = 0; /* unlimited */
	}

	env = getenv("SSB64_GBI_TRACE_DIR");
	if (env && env[0]) {
		snprintf(sTraceDir, sizeof(sTraceDir), "%s", env);
	}

	/* Skip tracing until this VI frame index (as reported by
	 * gbi_trace_set_vi_frame — the gameloop's PortPushFrame counter, same
	 * unit as SSB64_SCREENSHOT_FRAMES). The frame limit counts captured
	 * frames after the start point. */
	env = getenv("SSB64_GBI_TRACE_START");
	if (env) {
		int val = atoi(env);
		if (val > 0) sStartFrame = val;
	}

	if (!sEnabled) return;

	/* Create output directory */
	MKDIR(sTraceDir);

	/* Open the trace file */
	snprintf(sTraceFilePath, sizeof(sTraceFilePath), "%s/port_trace.gbi", sTraceDir);
	sTraceFile = fopen(sTraceFilePath, "w");
	if (!sTraceFile) {
		fprintf(stderr, "[gbi_trace] ERROR: cannot open %s for writing\n", sTraceFilePath);
		sEnabled = 0;
		return;
	}

	/* Header */
	fprintf(sTraceFile, "# GBI Trace — SSB64 PC Port\n");
	fprintf(sTraceFile, "# Format: [cmd_index] d=depth OPCODE  w0=XXXXXXXX w1=XXXXXXXX  params...\n");
	fprintf(sTraceFile, "# Source: port (Fast3D interpreter execution order)\n");
	fprintf(sTraceFile, "#\n");
	fflush(sTraceFile);

	fprintf(stderr, "[gbi_trace] Trace enabled, writing to %s (max %d frames)\n",
	        sTraceFilePath, sMaxFrames);
}

void gbi_trace_shutdown(void)
{
	if (sTraceFile) {
		fprintf(sTraceFile, "# END OF TRACE — %d frames captured\n", sFrameNum);
		fclose(sTraceFile);
		sTraceFile = NULL;
	}
	sEnabled = 0;
	sInitialized = 0;
}

/* ========================================================================= */
/*  Runtime control                                                          */
/* ========================================================================= */

void gbi_trace_set_enabled(int enabled)
{
	if (enabled && !sInitialized) {
		/* Force init */
		sEnabled = 1;
		gbi_trace_init();
		return;
	}
	sEnabled = enabled;
}

int gbi_trace_is_enabled(void)
{
	return sEnabled && sTraceFile != NULL;
}

void gbi_trace_set_max_frames(int max_frames)
{
	sMaxFrames = max_frames;
}

/* ========================================================================= */
/*  Frame lifecycle                                                          */
/* ========================================================================= */

void gbi_trace_set_vi_frame(int vi_frame)
{
	sViFrame = vi_frame;
}

int gbi_trace_get_vi_frame(void)
{
	return sViFrame;
}

void gbi_trace_begin_frame(void)
{
	if (!sEnabled || !sTraceFile) return;

	sSkipping = (sViFrame < sStartFrame);
	if (sSkipping) return;

	/* Check frame limit */
	if (sMaxFrames > 0 && sFrameNum >= sMaxFrames) {
		if (sFrameNum == sMaxFrames) {
			fprintf(sTraceFile, "# TRACE STOPPED — frame limit %d reached\n", sMaxFrames);
			fflush(sTraceFile);
			fprintf(stderr, "[gbi_trace] Frame limit %d reached, stopping trace\n", sMaxFrames);
		}
		sFrameNum++;
		return;
	}

	fprintf(sTraceFile, "\n=== FRAME %d (vi %d) ===\n", sFrameNum, sViFrame);
	sCmdIndex = 0;
	sFlushIndex = 0;
}

void gbi_trace_end_frame(void)
{
	if (!sEnabled || !sTraceFile) return;
	if (sSkipping) return;
	if (sMaxFrames > 0 && sFrameNum >= sMaxFrames) {
		sFrameNum++;
		return;
	}

	fprintf(sTraceFile, "=== END FRAME %d — %d commands ===\n", sFrameNum, sCmdIndex);
	fflush(sTraceFile);
	sFrameNum++;
}

/* ========================================================================= */
/*  Command logging                                                          */
/* ========================================================================= */

void gbi_trace_log_cmd(unsigned long long w0, unsigned long long w1, int depth)
{
	char decoded[512];

	if (!sEnabled || !sTraceFile) return;
	if (sSkipping) return;
	if (sMaxFrames > 0 && sFrameNum >= sMaxFrames) return;

	/* Extract lower 32 bits for the shared decoder (N64-compatible representation).
	 * Also log full 64-bit w1 when it differs (pointer addresses on PC). */
	uint32_t w0_lo = (uint32_t)(w0 & 0xFFFFFFFF);
	uint32_t w1_lo = (uint32_t)(w1 & 0xFFFFFFFF);

	gbi_decode_cmd(w0_lo, w1_lo, decoded, sizeof(decoded));

	/* Check if w1 upper bits are nonzero (widened pointer on PC) */
	uint32_t w1_hi = (uint32_t)(w1 >> 32);

	if (w1_hi != 0) {
		fprintf(sTraceFile, "[%04d] d=%d %s  (w1_64=%016llX)\n",
		        sCmdIndex, depth, decoded, w1);
	} else {
		fprintf(sTraceFile, "[%04d] d=%d %s\n",
		        sCmdIndex, depth, decoded);
	}

	/* SSB64_GBI_TRACE_DATA=1: also hexdump the payload behind pointer-carrying
	 * commands (G_MTX matrices, G_VTX vertices). The command stream alone can
	 * be byte-identical between a good and a corrupt frame when the divergence
	 * lives in CPU-computed matrix/vertex data; this exposes it. w1 is a host
	 * pointer on PC (w1_64), so the payload is directly readable here. */
	{
		static int sDumpData = -1;
		if (sDumpData < 0) {
			const char *env = getenv("SSB64_GBI_TRACE_DATA");
			sDumpData = (env != NULL && env[0] == '1') ? 1 : 0;
		}
		/* Only deref w1 when its upper 32 bits are set (a genuine widened
		 * host pointer). Segment-relative addresses (e.g. 0x0C000000) are
		 * resolved inside Fast3D and must not be dereferenced here. */
		if (sDumpData && w1_hi != 0) {
			uint8_t opcode = (uint8_t)((w0_lo >> 24) & 0xFF);
			const uint32_t *p = (const uint32_t *)(uintptr_t)w1;
			if (opcode == 0xDA && p != NULL) { /* F3DEX2 G_MTX: 64-byte fixed-point matrix */
				int i;
				fprintf(sTraceFile, "       MTXDATA");
				for (i = 0; i < 16; i++) fprintf(sTraceFile, " %08X", p[i]);
				fprintf(sTraceFile, "\n");
			} else if (opcode == 0xFD && p != NULL) { /* G_SETTIMG: checksum the pointed pixels */
				/* Length is unknown at SETTIMG time; 1 KB is enough to detect
				 * content changes between frames without risking a long read. */
				const uint8_t *b = (const uint8_t *)p;
				uint32_t sum = 0;
				int i;
				for (i = 0; i < 1024; i++) sum = sum * 31 + b[i];
				fprintf(sTraceFile, "       TIMGSUM %08X  first=%02X%02X%02X%02X%02X%02X%02X%02X\n",
				        sum, b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
			} else if (opcode == 0x01 && p != NULL) { /* F3DEX2 G_VTX: n verts, 16 bytes each */
				int n = (int)((w0_lo >> 12) & 0xFF);
				int i;
				if (n > 32) n = 32;
				for (i = 0; i < n; i++) {
					const int16_t *v = (const int16_t *)(p + i * 4);
					fprintf(sTraceFile, "       VTX[%02d] x=%d y=%d z=%d\n", i, v[0], v[1], v[2]);
				}
			}
		}
		/* Token-addressed G_VTX (w1_hi == 0): resolve through the port's
		 * reloc handle table and dump the backing vertex data. This is how
		 * per-frame CPU-built effect geometry is addressed, and content
		 * corruption there is invisible to the command-stream diff. */
		if (sDumpData && w1_hi == 0 && ((w0_lo >> 24) & 0xFF) == 0x01 && w1_lo != 0) {
			extern void* portRelocTryResolvePointer(uint32_t token);
			const uint32_t* vp = (const uint32_t*)portRelocTryResolvePointer(w1_lo);
			if (vp == NULL) {
				fprintf(sTraceFile, "       VTXTOK %08X -> UNRESOLVED\n", w1_lo);
			} else {
				int n = (int)((w0_lo >> 12) & 0xFF);
				int i;
				if (n > 8) n = 8;
				fprintf(sTraceFile, "       VTXTOK %08X -> %p\n", w1_lo, (const void*)vp);
				for (i = 0; i < n; i++) {
					const int16_t* v = (const int16_t*)(vp + i * 4);
					fprintf(sTraceFile, "       VTX[%02d] x=%d y=%d z=%d f=%04X uv=(%d,%d)\n", i, v[0], v[1],
					        v[2], (uint16_t)v[3], v[4], v[5]);
				}
			}
		}
	}

	sCmdIndex++;
}

/* Correlation marker for draw-dump analysis: one line per backend draw call
 * (Fast3D Flush), numbered in call order within the frame. */
void gbi_trace_note_flush(int num_tris)
{
	if (!sEnabled || !sTraceFile) { sFlushIndex++; return; }
	if (sSkipping) { sFlushIndex++; return; }
	if (sMaxFrames > 0 && sFrameNum >= sMaxFrames) { sFlushIndex++; return; }
	fprintf(sTraceFile, "       FLUSH #%d (%d tris)\n", sFlushIndex++, num_tris);
}
