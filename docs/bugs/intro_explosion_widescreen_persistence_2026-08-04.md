# Intro explosion widescreen framebuffer persistence

**Date:** 2026-08-04  
**Status:** FIXED (shared Fast3D path; DirectX 11 captured, OpenGL runtime-tested)

## Symptom

The opening desk-to-stage explosion was correct in 4:3, but in widescreen the
area outside the expanding red mask turned black. DirectX 11 blackened the
entire area outside the mask; OpenGL retained the center while blackening the
widened side regions.

## Root cause

The transition intentionally consumes the previous frame's color buffer. The
shared widescreen frame setup normally clears untouched side strips so 4:3
menus and other authored 2D screens cannot expose stale scene pixels. That is
the correct default, but it destroys the widened room history that this one
transition needs.

DirectX and Metal also use the rendering API's conservative regional-clear
fallback, which clears the complete framebuffer. This made the same shared
policy more severe on those backends, but changing the fallback alone would
still leave OpenGL's side regions black.

## Fix

Fast3D now exposes an explicit widescreen framebuffer-persistence flag through
the graphics bridge. `mvOpeningRoom` enables it only while the redirect-to-Z
explosion objects are alive (ticks 1040 through 1079) and resets it when the
scene starts. During that interval, frame setup preserves the complete game
framebuffer; outside it, the existing side-strip clear remains unchanged.

The decision is made in shared interpreter code, so OpenGL, DirectX 11, and
Metal all follow the same transition contract without duplicating scene logic
inside a renderer backend.

## Verification

- Windows Release `ssb64` target built successfully with MSVC.
- DirectX 11 captures at frames 1040, 1050, 1060, and 1070 show the widened
  desk scene retained to both screen edges while the red mask expands; no
  black exterior or duplicated corner geometry remains.
- OpenGL completed the same widescreen OpeningRoom run through frame 1100 and
  exited cleanly.
- Metal uses the same shared path, but could not be compiled or visually gated
  on the Windows test machine.

