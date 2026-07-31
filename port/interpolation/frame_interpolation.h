/**
 * frame_interpolation.h — Enhanced framerate mode (interpolated rendering).
 *
 * Renders k subframes per 60 Hz game tick (k = target fps / 60) with object
 * and camera matrices interpolated between the two most recent ticks. Game
 * logic cadence is untouched: PortPushFrame still runs exactly one tick, and
 * the tick's 16.67 ms wall duration is preserved because each of the k
 * subframe presents paces at 1/(60k) s via Fast3dWindow::SetTargetFps(60*k).
 *
 * Recording side: decomp draw code (gcPrepDObjMatrix, camera preps) calls
 * portInterpRecordMtx() at each gSPMatrix emission with the Mtx pointer and a
 * stable semantic key (owner object + ordinal). At drain time the port pairs
 * this tick's matrices against the previous drawn tick's by key and hands
 * per-subframe lerped values to Fast3D through the mtx_replacements map in
 * DrawAndRunGraphicsCommands. The final subframe uses an empty map, so the
 * landed frame is bit-exact game memory.
 *
 * Design doc: docs/frame_interpolation_design_2026-07-30.md
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Semantic tags for recorded matrices — selects pairing/guard behavior. */
#define PORT_INTERP_TAG_DOBJ 0        /* modelview from a DObj hierarchy */
#define PORT_INTERP_TAG_CAMERA_PROJ 1 /* projection (possibly view*persp combined) */
#define PORT_INTERP_TAG_CAMERA_VIEW 2 /* standalone view/lookat modelview */

/* CVar holding the target fps (0 = off, 120/180/240). Shared with PortMenu. */
#define PORT_INTERP_CVAR_FPS "gEnhancedFps"

/**
 * Recording hook, called from decomp draw code (C) at gSPMatrix emission.
 * `mtx` is the fixed-point Mtx the DL will reference; `owner` is a stable
 * object pointer (a DObj or CObj) used only as an opaque key; `ordinal`
 * disambiguates multiple matrices emitted for one owner in one pass.
 * Near-zero cost no-op while the feature is disabled.
 */
void portInterpRecordMtx(void *mtx, void *owner, int ordinal, int tag);

/**
 * Number of render subframes for the current tick (>= 1). Also lazily
 * initializes config and applies SetTargetFps when the effective value
 * changes. Returns 1 when disabled, GBI tracing is active, or the
 * auto-throttle stepped down.
 */
int portInterpActiveSubframes(void);

/**
 * Build this tick's snapshot from recorded matrices and pair against the
 * previous drawn tick. Call once per drained DL, before the subframe loop.
 */
void portInterpBeginDraw(void);

/** Rotate snapshots (prev <- cur) and clear the record list. Call after the
 * subframe loop (including on render-exception paths). */
void portInterpEndDraw(void);

/**
 * Feed the wall-clock duration of the completed PortPushFrame to the
 * auto-throttle. If the host cannot sustain 60 ticks/s at the configured k,
 * the throttle steps k down to protect the game clock.
 */
void portInterpNoteTicDuration(long long micros);

/** Re-read CVar/env config; resets the auto-throttle cap. Called lazily on
 * first use and from the menu when the setting changes. */
void portInterpApplyConfig(void);

#ifdef __cplusplus
} /* extern "C" */

#include <unordered_map>
#include <fast/types.h>

/**
 * Replacement map for subframe j of `total` (1-based). Returns a reference to
 * an internal map keyed by this tick's Mtx pointers with values lerped at
 * fraction j/total; the last subframe returns an empty map (render from game
 * memory, bit-exact). Valid until the next portInterpBeginDraw/EndDraw.
 */
const std::unordered_map<Mtx *, MtxF> &portInterpGetReplacements(int subframe, int total);

#endif /* __cplusplus */
