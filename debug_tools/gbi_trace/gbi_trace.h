/**
 * gbi_trace.h — Port-side GBI display list trace system
 *
 * Captures every F3DEX2/RDP command as it executes through Fast3D's
 * interpreter, producing structured per-frame trace files that can be
 * compared against N64 emulator traces.
 *
 * Usage:
 *   Set environment variable SSB64_GBI_TRACE=1 before launching.
 *   Traces are written to debug_traces/ in the working directory.
 *
 * Can also be enabled at runtime via:
 *   gbi_trace_set_enabled(1);
 */
#ifndef GBI_TRACE_H
#define GBI_TRACE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the trace system. Call once at startup.
 * Checks SSB64_GBI_TRACE env var and opens the output directory.
 */
void gbi_trace_init(void);

/**
 * Shut down the trace system and flush any open files.
 */
void gbi_trace_shutdown(void);

/**
 * Enable or disable tracing at runtime.
 */
void gbi_trace_set_enabled(int enabled);

/**
 * Returns nonzero if tracing is currently active.
 */
int gbi_trace_is_enabled(void);

/**
 * Mark the beginning of a new frame. Increments the frame counter
 * and starts a new trace block.
 */
void gbi_trace_begin_frame(void);

/* Report the gameloop's VI frame index (PortPushFrame counter) for the DL
 * about to be traced. Used by SSB64_GBI_TRACE_START and logged in the
 * frame header so traces correlate with SSB64_SCREENSHOT_FRAMES output. */
void gbi_trace_set_vi_frame(int vi_frame);
int gbi_trace_get_vi_frame(void);

/**
 * Mark the end of the current frame and flush the trace buffer.
 */
void gbi_trace_end_frame(void);

/**
 * Log a single GBI command during interpreter execution.
 * Called from the Fast3D interpreter callback.
 *
 * w0/w1 are the raw (potentially 64-bit) command words.
 * depth is the current display list call depth.
 */
void gbi_trace_log_cmd(unsigned long long w0, unsigned long long w1, int depth);

/**
 * Set the maximum number of frames to trace (0 = unlimited).
 * Default: 300 (5 seconds at 60fps).
 * Controlled by SSB64_GBI_TRACE_FRAMES env var.
 */
void gbi_trace_set_max_frames(int max_frames);

#ifdef __cplusplus
}
#endif

#endif /* GBI_TRACE_H */
