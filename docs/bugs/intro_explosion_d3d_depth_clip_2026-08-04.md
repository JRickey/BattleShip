# Intro explosion DirectX corner smears — scoped depth clipping

**Date:** 2026-08-04  
**Status:** FIXED (libultraship DirectX 11 backend)

## Symptom

During the opening desk-to-stage red vertex explosion, fragments of the
expanding mask appeared duplicated or smeared into the screen corners on
DirectX 11. The same transition rendered correctly on OpenGL in 4:3 mode.

## Root cause

The color-image-redirect-to-Z emulation suppresses color writes while drawing
the mesh that synthesizes the reveal depth mask. OpenGL already disables its
global `GL_DEPTH_CLAMP` for those draws, restoring real near/far clipping.
DirectX kept `D3D11_RASTERIZER_DESC::DepthClipEnable = false` for every draw.
Triangles in the mask that cross the camera plane therefore rasterized as an
inflated unclipped shape and wrote synthesized depth into the corners.

## Fix

The DirectX rasterizer cache now includes whether color writes are disabled.
Redirect-to-Z mask draws set `DepthClipEnable = true`; the following ordinary
color draw restores the established permissive `false` state. Swap-chain state
resets invalidate this cached bit alongside the existing depth and decal state.
No ordinary game geometry changes clipping behavior.

## Verification

- Windows Release build and package completed successfully with MSVC.
- User visual gate passed for both DirectX 11 and OpenGL in regular 4:3 mode.
- DirectX no longer shows duplicated red-explosion geometry in the corners.
- OpenGL behavior is unchanged and remains the parity reference.

