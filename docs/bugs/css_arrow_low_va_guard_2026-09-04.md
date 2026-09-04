# CSS Scroll Arrow Garbled on Non-PIE Builds (Low-VA Guard vs Baked .rodata)

**Status:** Fixed.

## Symptom

On the VS/Training stage-select screen, the orange page-scroll arrow (">")
renders as a narrow strip of colored noise instead of the chevron. Only on
**non-PIE** Linux builds (local Fedora Release/AppImage builds); official CI
AppImages were unaffected. First noticed while smoke-testing the
`agent/stage-icon-regression` AppImage, but present since late July on any
non-PIE build — not a regression of that branch.

## Root Cause

libultraship `2f186d2c` ("Fix Linux-dead low-VA guards", 2026-07-31 — part
of the intro membug-hunt hardening, shipped in v1.6) made the interpreter's
SETTIMG/G_VTX low-VA guards treat the **main program image as untrusted**:
a non-PIE build's VA range numerically overlaps stale N64-segment leftovers
(`0x01000000`, `0x0E000000`, ...), so `gfxPointerInLoadedModule()` must
exclude the main image or the guards never fire at all.

Collateral damage: the CSS scroll-arrow texture is a **baked `.rodata`
array in the main binary** (`kArrowRGBA16` in the generated `arrow_data.h`,
via `port/css_icons/port_css_arrow.cpp`). In a non-PIE build it sits at
~`0x010E1D20` — inside the guard's `<= 0x0FFFFFFF` window and inside the
main image — so its `G_SETTIMG` was silently dropped and the arrow quad
sampled whatever texture was previously bound (the noise strip).

PIE builds (the official CI AppImages — verified: the v1.6 binary is
`ET_DYN`, loading at a randomized high base) never enter the guard window,
which is why releases looked fine while local builds didn't.

Diagnosis notes: the baked array, the live process memory, the cached
Sprite/Bitmap structs, and the token indirections were all verified intact
via gdb on the running game — the corruption was purely the dropped
texture-set command.

## Fix

New registry in `libultraship/src/fast/interpreter.cpp`:

- `gfxRegisterTrustedLowVARange(base, size)` — registers an exact static
  data range as legitimate texture/Vtx memory.
- `gfxPointerInTrustedLowVARange(ptr)` — consulted by the SETTIMG guard and
  `gfx_vtx_addr_is_unresolved` alongside `gfxPointerInLoadedModule`, so
  registered bytes pass while every other low-VA value is still rejected.

`port_css_arrow.cpp` registers `kArrowRGBA16` / `kArrowLeftRGBA16` in
`getOrBuild()` (idempotent, before the first draw ever happens).

## Audit Hook

Any future texture or Vtx data baked into the main binary as a C array
(rather than heap-built or o2r-loaded) MUST call
`gfxRegisterTrustedLowVARange()` before its first draw, or it will render
garbage on non-PIE builds while looking fine on PIE CI artifacts — a
build-flavor-dependent bug that survives release testing. Also note the
flavor split itself: CI produces PIE binaries, local builds non-PIE; any
low-VA heuristic in the graphics stack behaves differently between them.
