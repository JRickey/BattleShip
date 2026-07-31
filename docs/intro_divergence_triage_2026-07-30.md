# Intro divergences vs cycle-accurate emulator — triage session 2026-07-30

Covers GitHub issues **#10** (explosion transition), **#72** (Yoshi-segment
"camera blocked" gold wedge), **#136** (stutter before unlockable silhouettes).

## Reference setup (reproducible)

Built mupen64plus from source at `/home/jrick/dev/n64-ref-emu/` (out of tree —
GPL, must not enter this repo): `mupen64plus-core` (`OSD=0 VULKAN=0 NO_ASM=1`,
**Pure Interpreter**), `mupen64plus-ui-console`, `mupen64plus-rsp-cxd4` (LLE),
`ata4/angrylion-rdp-plus` (cycle-accurate software RDP). Frame capture via
`--testshots 1,2,...` (≤1000 shots per run — chunk and re-run; boot is
deterministic). Full every-frame reference of the intro captured at
`/home/jrick/dev/n64-ref-emu/emu_full/` (frames 1–4200), port equivalent via
`SSB64_SCREENSHOT_FRAMES` (see below).

**Index-space warning:** the emulator capture counts *presented* frames — VIs
where the game held the framebuffer (scene loads, anchor waits, RCP stalls)
produce no new emulator frame. The port screenshot counter counts *every* VI.
By the fighter montage the two indices differ by ~150+; do content-based
alignment per window, never global index alignment.

## New/extended debug tooling (this session, all in-tree)

- `portFastCaptureBackbufferPNG` now has a real **OpenGL implementation**
  (`gfx_opengl.cpp`): request staged, PNG written at next `EndFrame` pre-swap
  — so `SSB64_SCREENSHOT_FRAMES` output on GL **lags the label by one frame**.
- `SSB64_GBI_TRACE_START=<vi>` — skip tracing until that VI frame; frame
  headers now include the VI index (`=== FRAME n (vi 1985) ===`).
- `SSB64_GBI_TRACE_DATA=1` — hexdump G_MTX matrices / G_VTX verts / SETTIMG
  payload checksums for host-pointer operands (tokens are skipped).
- `FLUSH #n (m tris)` markers in traces (from `Interpreter::Flush`) correlate
  trace positions with backend draw calls.
- `SSB64_DUMP_DRAWS=<vi>` or `<a>-<b>` — per-draw viewport-region snapshots to
  `draw_dump/draw_f<vi>_<idx>.png` + `draw_state.log` (GL viewport, scissor,
  depth state per draw). GL only.

## The opening's timing skeleton (relevant to all three issues)

Every mvOpening scene start busy-waits on a global scheduler-tic anchor
(`while (sySchedulerGetTicCount() < N)`): portraits 1335, then fighters at
1515/1605/…/2145 (90-tic pitch), run 2250, cliff 2500, yamabuki 2690, jungle
2880, yoster 3230, sector 3420, standoff 3610, clash 3975, newcomers 4155.
Each fighter segment: name ribbon tics 0–15, motion window tics 15–60 (window
created mid-scene at tic 15 → heavy setup), next scene loads at 60. On
hardware the setup/load time shows as held frames; the port does it instantly.

## Issue #72 — RESOLVED: missing near-plane trivial rejection in Fast3D

**Root cause (proven, fixed):** the Yoshi catch-flash effect emits a 4-vertex
CI4-textured quad whose vertices sit entirely outside the near plane — two in
front of it (z < -w) and two behind the camera (w < 0; VBO dump showed
w = -182 / +162 / +121 / -224). The RSP trivially rejects such triangles
(every vertex sharing the NEAR clip code), so the quad never rasterizes on
hardware or under cxd4-LLE — verified by RDRAM framebuffer dumps at the exact
tics: all three swap buffers clean. Fast3D inherited an upstream line that
**comments out the CLIP_NEAR flag** (`interpreter.cpp`, `clip_rej |= 16`),
so negative-w vertices got semantically garbage L/R/T/B flags instead; on
most frames those flags accidentally shared a bit (tri rejected — clean), but
tiny per-frame coordinate drift flipped them for exactly 2 frames, letting
the camera-plane-straddling quad reach the rasterizer, where its homogeneous
wrap-around covers the motion window as a smooth gold gradient with a hard
diagonal edge — the "camera blocked" wedge. Loose-timing emulators that skip
RSP-accurate trivial rejection can show the same artifact (user report).

**Fix:** restore CLIP_NEAR with a robust formulation
(`w <= 0 || z < -w` → flag; rejection still requires all three verts to
share it, so geometry merely crossing the near plane renders exactly as
before). Verified: wedge gone across the full window (frames 1978–1997
flat), 18-checkpoint intro regression sweep clean, explosion transition
unaffected. The same fix protects the other montage climax flashes (Samus
grapple, DK smash) which use the same authored off-frustum idiom.

Investigative notes kept for posterity: the wedge draw was isolated via the
per-draw dump (`SSB64_DUMP_DRAWS`) to flush #84/#192 with identical GL
state, texture, shader, and near-identical VBO between clean and corrupt
frames — the only remaining difference was the clip-flag luck described
above. Earlier hypotheses eliminated along the way: freeze-sim/idle-present,
wallpaper parallax, stale reloc tokens (token constant across the wedge),
texture payloads (checksums identical), effect vertex-data corruption (the
token-resolved VTX dump shows static, sane model-space verts), and
authored-but-timing-masked output (accurate-emulator RDRAM dumps show the
quad is never rendered at all).

## Issue #10 — confirmed visual contract (emulator reference)

Emulator frames (emu_full): explosion starburst grows over the **dim desk
room** 1097→1106; interior reveals the bright castle-desk ~1106–1109; fully
bright by 1112. The port (pre-fix) shows the bright destination from the first
starburst frame. Everything else in
`docs/intro_explosion_alpha_cutout_handoff_2026-04-25.md` §mechanism holds.
**RESOLVED this session** — full emulation implemented in libultraship; see
`docs/bugs/color_image_redirect_z_emulation_2026-07-30.md`. Key extra finding
beyond the old handoff: the hardware exterior during the expansion is *stale
framebuffer content* (N64 FBs persist across frames), so FB persistence was a
required fourth piece alongside the Z-write emulation.

**Red starburst border (follow-up, RESOLVED):** the emulation above initially
*removed* the transition's big red starburst outline (user-reported; emulator
shows red ramping to ~28% screen coverage f1094–f1111, port had none). It was
never a Z-value or ordering mystery: the 128-tri Outline annulus is drawn to
the normal FB with `G_ZBUFFER` set but `Z_CMP`/`Z_UPD` clear — hardware never
depth-tests it — while Fast3D derived depth testing from `G_ZBUFFER` alone
and rejected every fragment against the redirect's full-screen near fill.
Depth testing is now gated on `Z_CMP` for all draws (RDP semantics), which
reproduces the full hardware composite with no special casing: ring paints
unconditionally, wallpaper/scene (Z_CMP) fill the star interior over it, red
spikes persist outside. Root-cause chain and blast-radius verification in
`docs/bugs/zcmp_depth_test_gating_2026-07-30.md`. (Two dead ends worth
remembering: the old build's visible red was an artifact of its buggy
full-depth-clear-to-far, and near-clip/`w<=0` flagging was unrelated — the
ring's verts are all in front of the camera.)

## Issue #136 — RESOLVED: sustained-over-budget stutter paced by carry model

**Root cause (measured):** the newcomers/silhouette scene sustains cost
460–515k (white-background rects 262k px + 1820 tris) over the 400k budget
on EVERY frame, so the shipped threshold rule fired N=3 fifteen times in a
row (frames 4177–4191 + 4213 in the attract log) — a freeze-advance-freeze
cadence of five+ 2-frame holds. The scene-boundary long holds themselves
were never the problem: the port already produces them naturally (46-frame
no-DL hold at clash→newcomers, 29-frame at newcomers→logo — the anchor
busy-waits and loads), closely matching the accurate emulator's 0.5–0.95 s
holds.

**Fix (gameloop.cpp):** isolated over-budget climaxes still fire the full
N=3 hold (the authored montage pose freezes at frames 1800/1881 are
unchanged), but after a fire a cooldown (default 30 frames,
`SSB64_RCP_COOLDOWN`) redirects continued over-budget frames into a cycle
carry that drops a single frame (N=2) only when the accumulated EXCESS
reaches a full budget — the cadence the excess actually implies (60–115k
excess/frame → one drop every ~4–6 frames). Under-budget frames pay the
carry down. Live result in the window: N3 @4177, N2 @4183/4187/4192,
N3 @4213 — visually one 2-frame hold + isolated single drops + the natural
long hold, matching the issue's accurate-emulator capture (~7 gentle drops
+ long holds) instead of the machine-gun.

Background analysis retained:
- Anchors make wall-clock pacing roughly correct on the port, but hardware's
  hold is *contiguous* (one long load/anchor stall), while the port fragments
  it: game content advances between the RCP-sim deferrals, so the freeze is
  distributed as stutter.
- The RCP cost model's per-DL trigger (`port_get_last_dl_defer_n`) fires
  repeatedly across consecutive climax DLs in these scenes; on hardware the
  equivalent stall happens once per scene boundary (load+anchor), not per DL.
- Suggested direction: in the opening's movie scenes, replace (or gate) the
  per-DL cost model with scene-boundary load stalls (the
  `port_sim_load_stall` mechanism) sized from the emulator's presented-frame
  gaps, so holds are contiguous like hardware.
- New instrumentation added during the #72 endgame: `SSB64_RCP_LOG=1`
  (per-DL cost-model inputs/decisions), `SSB64_RCP_TRI_WEIGHT_PCT` (opt-in
  triangle-fillrate term; measured: montage window frames carry ~1.1M px of
  tri coverage, so naive weights >= 1 freeze the whole montage — a
  debt/carry RCP clock is the right shape, see
  docs/freeze_frame_rcp_clock_design_2026-04-26.md), `draw_vbo.log` +
  program/texture/blend in `draw_state.log`, and token-resolving G_VTX
  payload dumps in the GBI trace.
- A separate crash was observed once at frame ~3095 (DL ExecStack recursion
  diag → SIGABRT) during a trace run in the standoff/clash window; did not
  reproduce in screenshot runs. Noted for a future stability pass.
