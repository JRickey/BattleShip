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
- `GfxDpFillRectangle` redirect branch: computes the written Z from
  `fill_color` (FILL cycle) or `prim_color` (1-cycle), packs RGBA→5551,
  snaps ≥0xFFFC to exactly 1.0, and clears **only the fill rect region**
  via the new `GfxRenderingAPI::ClearDepthRegion(x0,y0,x1,y1,depth)`
  (normalized to the game FB; default implementation falls back to the old
  full-buffer clear so non-GL backends keep their previous behavior).
- Tri path: when redirect-active, `depth_mask` is forced on, depth test
  follows `Z_CMP` (RDP semantics), the constant combiner output (from
  `prim_color`, packed 5551 → /65535) is written as depth by reusing the
  `G_ZS_PRIM` constant-depth vertex path, and framebuffer color writes are
  suppressed via the new `GfxRenderingAPI::SetColorWriteMask(bool)`
  (GL implements `glColorMask`; other backends default to a no-op =
  status-quo).
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
the newcomers burst) shows no regressions. Known remaining cosmetic delta:
the silhouette's red outer spikes read slightly dimmer than the reference in
the first ~6 frames of the expansion.

## Don't break

- The per-camera Z-clear (`objdisplay.c`) uses a FILL-mode redirect fill with
  `GPACK_ZDZ(G_MAXFBZ,0)` = 0xFFFC → must keep snapping to a full clear-to-
  far (handled by the ≥0xFFFC snap).
- The wallpaper PrimDepth z=0.56 layering and the fighter-portrait card
  (see `primdepth_unimplemented_2026-04-25.md`) still depend on the
  `G_ZS_PRIM` path; the redirect override only engages while the color image
  targets the Z buffer.
- Non-GL backends compile against the new RAPI hooks via safe defaults but
  do not yet implement them; DX11/Metal keep pre-existing behavior until
  ports of `SetColorWriteMask` / `ClearDepthRegion` / `ClearColorRegion`
  are added there.
