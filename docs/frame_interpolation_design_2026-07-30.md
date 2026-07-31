# Enhanced Framerate Mode — Frame Interpolation Design

**Date:** 2026-07-30
**Branch:** `agent/frame-interp`
**Status:** Design finalized; implementation in phases below.

## Goal

Render at a multiple of 60 FPS (120/180/240) while game logic stays locked at
exactly 60 Hz, by interpolating object/camera matrices between the two most
recent logic ticks — the Ship-of-Harkinian approach, adapted to this port's
architecture. Opt-in, default off, zero behavior change when disabled.

## Non-negotiable constraint: the game clock

Everything in the port derives its 60 Hz cadence from one fact: **one
`PortPushFrame()` call = one VI period = one game tick**, and `PortPushFrame`
takes 16.67 ms of wall time only because the render backend's
`SyncFramerateWithTime()` (at `mTargetFps`, default 60) blocks inside
`Interpreter::EndFrame() → SwapBuffersBegin()`.

Verified time consumers (2026-07-30 survey):

- **Audio is hard-coupled to tick count**: `sys/audio.c` synthesizes exactly
  1/60 s of PCM per VRETRACE tick message (`sSYAudioFrequency = rate/60.0F`,
  `syAudioGetPortSampleCount` accumulates error per tick). Wrong tick rate ⇒
  immediate audio drift/starvation.
- `osGetTime`/`osGetCount` map to wall clock in LUS but are consumed only by
  the debug perf overlay and `syUtilsRandTime*` (cosmetic random picks). No
  gameplay delta-time logic exists anywhere — all gameplay is per-tick.
- The freeze-frame machinery (`sPortPendingSPInts` deferral queues,
  `port_vi_simulate_vblank`, idle-present) is keyed to "one call = one VI".
- Hitlag, pause, and training slow-mo are *entity-level* skips inside a
  normally-ticking 60 Hz loop — the tick cadence itself never changes.

**Design rule:** the per-tick section of `PortPushFrame` (cheats, HandleEvents,
vblank simulation, VRETRACE post, coroutine resume, enhancement ticks, events,
watchdog, cost-model latch) is untouched and runs exactly once per tick. Only
the render tail fans out. The tick's 16.67 ms wall duration is preserved
because k subframe presents each pace at 1/(60k) s via `SetTargetFps(60·k)` —
the same absolute-schedule pacer that provides the 60 Hz cadence today.

## Architecture

```
PortPushFrame()                            ── once per game tick (60 Hz)
  ├─ [unchanged per-tick section]
  │    └─ game draw phase calls portInterpRecordMtx(mtx, owner, ordinal, tag)
  │       at each gSPMatrix emission (decomp hooks, #ifdef PORT)
  └─ port_drain_pending_display_list()
       ├─ snapshot: decode recorded Mtx values (s15.16 → float, interpreter's
       │  exact formula), build cur map keyed by (owner, ordinal, tag, occurrence)
       ├─ pair cur ↔ prev by key; guards decide lerp vs snap per matrix
       └─ for j = 1..k:                    ── k = targetFps/60 subframes
            f = j/k
            map = { curMtxPtr → lerp(prevVal, curVal, f) }   (empty when j == k)
            window->DrawAndRunGraphicsCommands(dl, map)      (paces 1/(60k) s)
       └─ rotate prev ← cur
```

- **Interpolation window:** subframes sweep `prev → cur`, landing exactly on
  `cur` at the last subframe (empty replacement map ⇒ interpreter reads game
  memory verbatim — bit-exact, no float round-trip). Visual latency cost: up
  to one tick (16.7 ms) of *displayed-position* lag, standard for
  interpolation; input→logic latency unchanged.
- **Replacement mechanism:** the pinned LUS fork already implements
  `Interpreter::Run(Gfx*, const std::unordered_map<Mtx*, MtxF>&)` with lookup
  in `GfxSpMatrix` (interpreter.cpp:2534). The port already passes an empty
  map (`port/gameloop.cpp:380`). **No libultraship changes needed.**
- **Recording hooks (decomp, `#ifdef PORT`, observational only):**
  - `sys/objdisplay.c` `gcPrepDObjMatrix` — single emission point covers every
    3D DObj: fighters (via `ftDisplayMainDrawParts`), items, weapons, stages,
    effects. Key: `(dobj, per-call ordinal sp2CC)`.
  - `sys/objdisplay.c` `gcPrepCameraMatrix` — persp/ortho/lookat arms for
    menu/generic scenes. Key: `(cobj, xobj slot)`.
  - `gm/gmcamera.c` `gmCameraPrepLookAtFuncMatrix` /
    `gmCameraPrepProjectionFuncMatrix` — in-match camera (combined view×persp
    loaded as PROJECTION). Lerp of the combined matrix is mathematically exact
    for a fixed projection because the product is linear in the view operand.
- **Pairing:** semantic keys, not DL positions — robust against
  spawn/despawn reshuffling the DL. Spawned matrix (no prev) ⇒ snap. Despawned
  ⇒ nothing to do. Duplicate keys (camera re-prepped between layer groups) are
  disambiguated by per-tick occurrence index, stable across frames.

## Guards

1. **Static skip:** prev bits == cur bits ⇒ no map entry (stage geometry,
   ortho HUD projections, cached `XObj.mtx` — interpreter reads memory).
2. **Teleport snap:** translation delta above threshold (default 500 units,
   `SSB64_INTERP_SNAP_DIST`) ⇒ snap. Catches respawns, screen-cut cameras,
   recycled DObj slots.
3. **Rotation flip snap:** any basis-column dot product < 0 between prev/cur
   (>90°/tick) ⇒ snap.
4. **Projection sanity:** Frobenius-ratio blowup ⇒ snap.
5. **Auto-throttle (clock guard):** EMA of tick wall duration; sustained
   >17.2 ms with k>1 ⇒ step k down (and `SetTargetFps`) with a log line.
   Catches vsync-limited displays (e.g. 60 Hz display + k=2 would halve game
   speed — throttle restores 60 Hz within seconds), weak GPUs, compositor
   stalls. Verified 2026-07-31: 240 fps target on a 144 Hz vsynced display
   stepped 240→180→120 automatically and the clock recovered to exactly
   60 Hz. (No explicit refresh-rate query; the throttle subsumes it.)
6. **Freeze frames:** a tick with no DL keeps `prev` untouched and
   idle-presents k paced subframes of the held frame; the following real tick
   pairs against the last *drawn* tick. Authored freezes render as authored.
7. **Diagnostics compat:** interpolation forces k=1 while GBI tracing
   (`GBI_TRACE_START`/`SSB64_DUMP_DRAWS`) is active so trace workflows see one
   walk per tick. RCP cost model latches from the first subframe run only
   (accumulators reset per run) so the freeze-frame model sees 1× cost.

## Known v1 limitations (documented, graceful)

- **Moveword-MVP paths don't interpolate:** XObj kinds 41–50 and the
  `lbcommon.c` billboard funcs patch matrix rows via `G_MW_MATRIX` immediates
  embedded in the DL — not reachable by the pointer-keyed map. Those objects
  (some billboarded effects) update at 60 Hz. Invisible in practice for
  short-lived sparks; revisit with DL-word patching if ever needed.
- **SObj/HUD sprites** are screen-space texrects — correctly *not*
  interpolated.
- **Vertex-animated geometry** (positions baked into gSPVertex data per tick)
  moves at 60 Hz. Matrices dominate motion in SSB64, so this is minor.
- **Auxiliary direct emissions** in `ftdisplaymain.c` (hurtbox display
  spheres, afterimages) and `it/wp` display aux sites: not hooked in v1.
- **Netplay/rollback:** rendering-only feature; no game state is read outside
  the tick or written at all. Current netcode has no resimulation; if rollback
  lands later, resimulated ticks that skip the draw phase never touch the
  recorder (hooks fire only in draw procs), so pairing stays display-aligned.

## Configuration

- CVar (persisted, PortMenu): `gEnhancedFps` — 0 (off, default) / 120 / 180 /
  240. Menu: Enhancements section, combo "Enhanced Framerate (Interpolated)".
- Env override for testing: `SSB64_INTERP_FPS`.
- `SSB64_INTERP_SNAP_DIST` — teleport threshold tuning.
- `SSB64_INTERP_LOG=1` — per-second stats (recorded/paired/snapped counts,
  tick-duration EMA).

## Phases

1. **Port module + subframe loop** (`port/interpolation/frame_interpolation.{h,cpp}`,
   `gameloop.cpp` drain restructure, pacing, throttle, config, menu). With no
   hooks yet, renders k identical subframes — verifies the game clock stays
   60 Hz under k=2/3/4 before any visual change exists. Verification: frame-60
   log timestamps advance 1.00 s per 60 ticks at every k; audio clean.
2. **Decomp hooks** — `objdisplay.c` (both preps) + `gmcamera.c` (two camera
   funcs). Verify replaced-matrix counts > 0 in gameplay/attract and smooth
   motion at 120.
3. **Polish** — thresholds tuning, freeze/hitlag/slow-mo spot checks, docs.

## Rejected alternatives

- **DL-walker structural matching (no decomp hooks):** positional pairing
  breaks every time an effect spawns/despawns (constant in matches);
  mispairs adjacent bones for a tick. Needs a port-side reimplementation of
  interpreter addressing (segments, reloc tokens, multi-word commands).
  Semantic hooks are ~10 lines of decomp diff and pair perfectly.
- **SoH-style full op recording/replay:** records every matrix op with
  parameters and replays with lerped args. Highest quality for 20 Hz games;
  unnecessary at a 60 Hz base where per-tick deltas are small, and would
  require instrumenting dozens of decomp draw paths.
- **Extrapolation (zero added visual latency):** overshoot artifacts on every
  direction change; rejected.
- **Decoupled free-running render thread:** the port's fiber model serializes
  game and render on one OS thread by design (Android JNI constraints,
  coroutine ownership); a render thread would be a rewrite with data-race
  surface across the whole DL/heap lifecycle.
