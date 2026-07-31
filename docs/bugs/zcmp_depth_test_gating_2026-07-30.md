# Depth test gated on Z_CMP, not G_ZBUFFER (intro red starburst border)

**Status: RESOLVED — Fast3D now enables depth comparison only when the render
mode requests it, matching RDP semantics.**

## Symptom

The intro desk→stage explosion transition was missing its big **red starburst
border** around the expanding star (user-reported vs. real-hardware footage;
confirmed against the angrylion reference: emulator frames f1094–f1111 show
red coverage ramping to ~28% of the screen, the port showed none). Everything
else about the transition (#10 redirect-to-Z emulation) was correct.

## Investigation trail (what it was NOT)

- Not missing geometry: per-tri interpreter logging of the whole frame
  showed the 128-tri Outline starburst annulus **is** submitted to GL each frame
  (`GfxSpTri1` passes it; wireframe render of the logged verts reproduces the
  spiky annulus shape exactly).
- Not clip rejection and not backface culling: 0 of the 128 tris carry
  shared clip flags; the mesh has no `CULL_BACK` losses beyond hardware's.
- Not a color/combiner bug: framebuffer diff across the ring's GL draw
  showed **zero pixels changed** — the draw executed and every fragment was
  depth-rejected.

## Root cause

Frame structure of a transition frame (VI-authentic ordering, priority 95):

1. Full-screen redirect FILLRECT writes ~0x0001 (**near**) into the whole Z
   buffer through the color path (this is correct #10 emulation — it is what
   freezes the exterior).
2. The 128-tri red ring (`Outline` mesh from `MVOpeningRoomTransition`) draws
   to the **normal framebuffer** with geometry mode `ZBUF|SHADE|CULL_BACK`
   but othermode `00552048` — **Z_CMP=0, Z_UPD=0**.
3. The 62-tri star silhouette stamps **far** into the Z buffer (redirect).
4. Scene + wallpaper draw with Z_CMP, so they only land inside the star.

On the RDP, depth *comparison* is enabled solely by `Z_CMP` in
`other_mode_l`; `G_ZBUFFER` in the geometry mode only makes the RSP emit
per-vertex Z. Step 2 therefore **never depth-tests on hardware** — the ring
paints unconditionally over the stale desk frame, and step 4's Z_CMP draws
then cover its interior, leaving red spikes outside the star. That is the
entire hardware mechanism; no mid-frame Z trickery is involved.

Fast3D derived `depth_test` from `G_ZBUFFER` alone. With the #10 emulation
in place, step 1 legitimately fills the Z buffer with near — and the port
then Z-rejected every ring pixel against it (GL_LESS vs. depth≈0). Before
the #10 work, the old build's visible red was itself an artifact of its
buggy full-depth-clear-to-far.

## Fix (`libultraship/src/fast/interpreter.cpp`, `GfxSpTri1`)

```cpp
bool has_vertex_z = (mRsp->geometry_mode & G_ZBUFFER) == G_ZBUFFER;
bool depth_test   = has_vertex_z && (mRdp->other_mode_l & Z_CMP) == Z_CMP;
bool depth_mask   = (mRdp->other_mode_l & Z_UPD) == Z_UPD;
```

This also subsumes the redirect-active special case that previously set
`depth_test` from `Z_CMP` only while the color image targeted the Z buffer —
the gating is now unconditional, because it is what the hardware does.
The GL backend already handles `(test=false, mask=true)` correctly
(depth test enabled with `GL_ALWAYS` + write, since GL only writes depth
while testing is enabled).

## Blast radius

Draws with `G_ZBUFFER` set but `Z_CMP` clear previously depth-tested and now
do not — which is what hardware does, so any visual change is an accuracy
improvement. Verified no regressions: explosion window (red ring restored,
dim exterior/bright interior structure intact), Yoshi segment (#72 wedge
still gone), montage, newcomers burst, title screen, and the demo fight
(fighters/stage/UI/shadows all correct).

## Related

- `color_image_redirect_z_emulation_2026-07-30.md` — the #10 emulation this
  interacts with (the near fill that exposed the bug).
