# Color-image-redirect-to-Z emulation (issue #10 — intro desk→stage explosion)

**Status: RESOLVED — real emulation implemented in libultraship's Fast3D; the
two game-side PORT workarounds in `mvopeningroom.c` are removed.**

## Hardware ground truth (new this session)

Captured RDRAM dumps from a cycle-accurate reference (mupen64plus pure
interpreter + cxd4 LLE RSP + angrylion-plus RDP, patched out-of-tree to dump
RDRAM at chosen VIs) during the transition. The Z buffer during the expansion
is: **≈0x0000 (near) everywhere except the expanding star shape, which holds
far values with live-scene depths inside**. The visible frame's exterior (the
dim desk room, frozen hand included) is **stale framebuffer content from
pre-explosion frames** — nothing overdraws it because every draw outside the
star is Z-rejected. The interior shows the destination wallpaper sprite
(PrimDepth z=0.56) plus the near (< 0.56) live objects. That confirms and
refines the mechanism in `intro_explosion_alpha_cutout_handoff_2026-04-25.md`.

The effect therefore needs four pieces of RDP behavior the port lacked:

1. Redirect-active **fills** write their color-path output into the Z buffer
   as a 16-bit value, scoped to the fill rectangle.
2. Redirect-active **triangles** write their (constant) combiner output as
   depth, unconditionally, with **no framebuffer color side effect**.
3. **Rectangles are depth-tested/written when Z_CMP/Z_UPD are set**, using
   the primitive depth register (rects have no per-pixel Z on the RDP).
4. **Framebuffer color persists across frames** (VI scans RDRAM; games clear
   only what they choose to).

## Implementation (libultraship)

- `Interpreter::RdpColorImageIsZBuffer()` — redirect detection matches on
  resolved pointers **or** the raw (pre-`SegAddr`) operands of
  `G_SETCIMG`/`G_SETZIMG`, so DLs carrying baked N64 addresses still match.
- `GfxDpFillRectangle` redirect branch: stages the written value into
  `prim_color` (FILL cycle copies `fill_color` in; 1-cycle already drives
  the combiner with `prim_color`), forces a PRIMITIVE combiner, applies the
  FILL/COPY inclusive lower-right +1, and routes the rect through
  `GfxDrawRectangle` — the redirect-active tri path below then turns it
  into a value- and region-accurate depth fill on **every** backend with no
  backend-specific clear hook (the earlier `ClearDepthRegion` RAPI hook was
  removed in favor of this draw-based path).
- Tri path: when redirect-active, `depth_mask` is forced on, the constant
  combiner output (from `prim_color`, packed 5551, ≥0xFFFC snapping to
  exactly 1.0 → /65535) is written as depth by reusing the `G_ZS_PRIM`
  constant-depth vertex path, and framebuffer color writes are suppressed
  via the new `GfxRenderingAPI::SetColorWriteMask(bool)` (GL implements
  `glColorMask` and also scopes out `GL_DEPTH_CLAMP` for these draws;
  DX11 implements it via a zero-write-mask blend state — untested until
  Windows CI; Metal defaults to a no-op = status-quo).
- Depth *testing* for all draws follows RDP semantics — enabled only when
  `Z_CMP` is set (and the RSP emits Z, i.e. `G_ZBUFFER` in geometry mode).
  This is load-bearing for the transition's red outer ring; see
  `zcmp_depth_test_gating_2026-07-30.md`.
- `GfxDrawRectangle`: sets `G_ZBUFFER` in the transient geometry mode when
  `other_mode_l & Z_CMP`, routing rects through the depth-tested path with
  the prim-depth constant — this is what lets the destination-wallpaper
  strips be masked by the redirect-written Z.
- Frame-start clear: the widescreen-mode full color clear is replaced by
  clearing **only the pillarbox side strips**
  (`GfxRenderingAPI::ClearColorRegion`); the 4:3 content area keeps its
  prior-frame pixels in every mode. Depth is still cleared each frame.

## Game-side changes

- `mvOpeningRoomTransitionOverlayProcDisplay`: PORT skip removed — original
  N64 code runs.
- `mvOpeningRoomMakeTransitionCamera`: `dl_link_priority` restored 30 → 95.
- `gmCameraMakeMovieCamera`: `port_sim_load_stall(2)` — the opening montage
  motion windows are created mid-scene together with expensive stage+fighter
  setup that overruns the frame on N64; the first 1–2 authored camera states
  are display-degenerate (verified: the first tic has eye−at dist.z == 0)
  and never reached the screen on hardware. The port drops those two gfx
  submissions and idle-presents the held frame, matching hardware.

## Verified

Frame-by-frame against the angrylion reference (`emu_full/` 1085–1160 vs
port captures): dim-room exterior persists with the frozen hand, the star
punches through to the destination image, near objects stay, the reveal
completes at the same animation stage (±3-frame global alignment skew).
Regression sweep of the whole opening (15 checkpoints from the chest room to
the newcomers burst) shows no regressions.

The transition's **red starburst border** initially went missing entirely
under this emulation (every ring pixel Z-rejected against the redirect's
full-screen near fill) — root cause and fix in
`zcmp_depth_test_gating_2026-07-30.md`. With that fix the full hardware
composite reproduces: red spikes clipped outside the star silhouette,
wallpaper interior, stale desk exterior.

## Don't break

- The per-camera Z-clear (`objdisplay.c`) uses a FILL-mode redirect fill with
  `GPACK_ZDZ(G_MAXFBZ,0)` = 0xFFFC → must keep snapping to a full clear-to-
  far (handled by the ≥0xFFFC snap).
- The wallpaper PrimDepth z=0.56 layering and the fighter-portrait card
  (see `primdepth_unimplemented_2026-04-25.md`) still depend on the
  `G_ZS_PRIM` path; the redirect override only engages while the color image
  targets the Z buffer.
- Non-GL backends compile against the new RAPI hooks via safe defaults.
  DX11 has a `SetColorWriteMask` implementation (zero-write-mask blend
  state selected in `DrawTriangles`) that still needs a Windows CI
  compile + behavior check; Metal keeps the no-op default. The depth-fill
  path itself is draw-based and backend-independent — only
  `SetColorWriteMask`/`ClearColorRegion` are per-backend.
