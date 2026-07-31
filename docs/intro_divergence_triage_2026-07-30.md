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

## Issue #72 — root cause chain (PINNED to the draw)

Reproduces deterministically at port VI frames **1985–1986 on screen**
(screenshot labels 1984–1985 due to the GL one-frame capture lag): the Yoshi
motion window is covered by a smooth gold gradient with a hard diagonal edge
for exactly 2 frames, mid catch animation (Yoshi-scene tics 52–53, catch
anim_frame 16–18; window created at frame 1947 = tic 15; tic = frame − 1932
in the current build).

Eliminated by experiment: RCP freeze-sim (`SSB64_RCP_FORCE_N=1` — no change),
freeze pacing (`SSB64_FREEZE_PACING=0` — no change), stale present/idle-present
(final game-FBO content is clean at 1984, wedge is *drawn* during 1985),
wallpaper parallax excursion (clamps keep it covering; separate finding below),
texture payload changes (SETTIMG checksums identical across the wedge).

Positive identification (draw_dump + FLUSH markers, frame vi1985):
- The wedge is painted by **flush #84** = 2 triangles = the 4-vertex textured
  quad submitted at trace cmds `[2473-2474]` (`G_VTX` token `0x002013CA`,
  CI4 32×21 texture + 16-color TLUT, clamp both axes), the **last window-pass
  object** (depth 11, after `clr=ZBUF`, so depth test off — `draw_state.log`
  confirms depth_test=0, viewport/scissor = the motion window).
- The quad's tris sit buffered until the **band camera's Z-clear redirect fill**
  (`[2584-2586]`: `SETCIMG → 0x0241E860` (the Z buffer), `SETFILLCOLOR FFFC`,
  `FILLRECT (10,150)-(309,229)`) hits `GfxDpFillRectangle`'s redirect branch,
  whose `Flush()` renders them. (The redirect detection itself works — both
  the CIMG and the depth image resolve through the same address form.)
- Command streams and matrices for vi1984 (clean) vs vi1985 (wedge) are
  structurally identical (only normal animation deltas) ⇒ the quad's
  **CPU-built vertex data** is what explodes across the window on those two
  frames. The blow-up coincides with a `gcEjectGObj` (id=1011, dl_link 65) at
  the same moment and with the `dobj_wait=-FLT_MAX` state that the existing
  `ftCommonCatchProcUpdate` trace logs — stale-data family suspicion.
- On hardware/emulator those frames render normally (the emulator presents
  the same tics with sane content), so this is a port-side data bug, not a
  masked-frame artifact.

Open thread for the fix: identify which effect builds that quad (CI4 32×21 +
TLUT, rebuilt per frame in the DL heap, drawn depth-off at the end of the
window pass — dust/eject-related billboard) and why its vertices are garbage
for exactly the 2 frames after the eject. The `G_VTX` operand is a reloc token
(`0x0020134C/D` neighbors it as SETTIMG), so extend the trace data dump to
resolve tokens via the interpreter's SegAddr if payload data is needed.

Also fixed/kept from this hunt:
- `port_sim_load_stall(n)` (gameloop) + a 2-frame stall in
  `gmCameraMakeMovieCamera` — mirrors the hardware setup overrun at motion
  window creation, whose first 1–2 authored camera states are
  display-degenerate (first frame literally has eye−at dist.z == 0, slamming
  `grWallpaperCalcPersp`'s atan2 parallax to the clamps). Not the #72 wedge,
  but hardware never displayed those frames and now the port doesn't either.

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

## Issue #136 — timing model (analysis, fix not attempted this session)

The pre-silhouette stutter window (clash 3975 → newcomers 4155) shows on the
port as runs of 2–3-frame micro-freezes where the emulator shows one long
authored hold (~0.95 s). Established this session:
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
- A separate crash was observed once at frame ~3095 (DL ExecStack recursion
  diag → SIGABRT) during a trace run in the standoff/clash window; did not
  reproduce in screenshot runs. Noted for a future stability pass.
