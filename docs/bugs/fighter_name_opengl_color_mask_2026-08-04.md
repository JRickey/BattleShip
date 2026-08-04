# OpenGL fighter-name cards lost after depth redirect

**Resolved:** 2026-08-04

**Affected path:** libultraship Fast3D presentation, most visibly OpenGL

## Symptom

During the opening movie, the black fighter-name cards between the vertex
explosion and the short Mario / Donkey Kong / Yoshi / Samus animations were
blank in OpenGL. DirectX displayed the white fighter names correctly.

## Root cause

The name sprites and textures were valid. Per-draw OpenGL captures showed the
complete `MARIO` card in `mGameFb` immediately before presentation.

The display list's last draw redirects the N64 color image to the depth buffer.
Fast3D emulates that operation by disabling framebuffer color writes. When the
display list ended in that state, OpenGL's global `glColorMask` remained false.
`ComposeFinalFrame()` cleared the swap-chain framebuffer and ImGui then tried to
present `mGameFb`, but its image quad inherited the disabled mask and wrote no
pixels. The held-VI-frame path repeated the same black presentation. DirectX's
ImGui renderer installs its own blend/write state, which is why it did not show
the regression.

## Fix

Before the GUI presentation boundary, restore color writes and update
Fast3D's cached `color_write_enabled` state. This is done in both:

- `ComposeFinalFrame()` for newly rendered frames.
- `PresentCurrentFramebuffer()` for held VI frames.

Synchronizing the cache is important: a later redirect draw must observe the
restored state and disable color writes again.

## Verification

- Windows Release `ssb64` target built successfully.
- Full opening run from scene 28 on OpenGL: `MARIO` is visible on the black card
  and persists through the held intro frames before Mario's animation.
- The same scene-28 capture on DirectX still shows `MARIO` correctly.
