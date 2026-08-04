# Native host pointer mistaken for segment-E display-list address

**Status:** Fixed on 2026-08-04

**Affected path:** Shared Fast3D interpreter; reproduced with Windows OpenGL

**Symptom:** The opening fighter-introduction sequence crashed in `Fast::gfx_step`.

## Reproduction

Starting directly in scene 28 with OpenGL and widescreen enabled crashed consistently after about 30.9 seconds, at VI 1805. The user's minidump reported access violation `0xc0000005` while `gfx_step` dereferenced the current `Gfx` command. A 400 MB GBI trace ended with:

```text
[0939] d=5 G_DL w0=DE000000 w1=0EC0D820 (w1_64=00007FF60EC0D820)
```

The interpreter then descended to depth 6 and read roughly 48,000 apparent `G_NOOP` commands from unrelated heap data before faulting. Resolving the full address through the Windows symbols identified the intended display list as `dFTShadowNoPrevLinkDL`, which has a valid `G_ENDDL` terminator.

## Root cause

`gfx_dl_handler_common` has SSB64-specific handling for segment `0x0E`. Runtime material display lists use 32-bit N64 addresses of the form `0x0Exxxxxx`, whose byte offsets must be converted from the packed 8-byte N64 `Gfx` stride to the port's widened native `Gfx` stride.

The handler detected segment E solely from bits 24–31 of `w1`. On this Windows build, ASLR placed the valid native pointer to `dFTShadowNoPrevLinkDL` at `0x00007FF60EC0D820`. Its bits 24–31 coincidentally equal `0x0E`, so the handler treated this 64-bit host pointer as an N64 segmented address, kept only its low 24-bit offset, and redirected execution into the segment-E runtime heap. The resulting command walk interpreted arbitrary heap contents until it reached unmapped memory.

This is a shared interpreter bug rather than an OpenGL rendering bug. OpenGL exposed it because this particular executable layout made the pointer alias deterministic; another backend or build may receive a different ASLR layout.

## Fix

Both segment-E special cases now require `w1 <= UINT32_MAX` before interpreting bits 24–31 as an N64 segment selector. Native 64-bit pointers continue through `SegAddr` unchanged, while genuine 32-bit `0x0Exxxxxx` tokens retain the widened-stride correction and reloc-file fallback.

## Verification

- Windows Release target rebuilt successfully.
- OpenGL + widescreen scene 28 ran for 65 seconds, more than 34 seconds past the formerly deterministic crash, and was then stopped manually.
- DirectX scene 28 ran for 40 seconds with the shared fix and was then stopped manually.
- Neither post-fix run produced a new crash dump.

## Audit hook

Before interpreting any field inside a widened pointer-sized GBI word as a 32-bit N64 address, first prove the entire value fits in 32 bits. Segment-byte tests alone can alias arbitrary bytes in native addresses under ASLR.
