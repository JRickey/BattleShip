# `FTCommandVars` item-throw overlay used host-dependent bitfield layout

**Date found:** 2026-08-29

**Fixed:** 2026-09-28

**Status:** FIXED in the decomp port (`src/ft/fttypes.h`, `ft/ftcommon/ftcommonitemthrow.c`)

**Class:** a union written through raw words and read through compiler-dependent bitfields

## What was proven

`FTStruct.motion_vars` contains four words written by motion-script `SetFlag0..3` events. The
port also exposed the same storage through this item-throw view:

```c
struct FTItemThrowFlags {
    sb32 is_throw_item;
    u8 unk1;
    u32 damage : 24;
    u8 unk2;
    u32 vel : 12;
    s32 angle : 12;
};
```

That overlay only has the intended layout with the N64 compiler:

- N64 IDO: 16 bytes; `damage = flag1 & 0xFFFFFF`,
  `vel = (flag2 >> 12) & 0xFFF`, and `angle` is signed bits 11..0.
- clang/GCC on little-endian hosts: 16 bytes, but the bitfields occupy different bit positions.
- MSVC: 20 bytes; `damage` moves into the third word and `vel`/`angle` move beyond the four words
  written through the `flags` view.

The state trace's startup layout probe exposed the MSVC size difference. Source inspection then
proved that any nonzero item-throw `flag1` or `flag2` would be decoded incorrectly on the port.

## Gameplay scope

No player-visible failure from this overlay has been reproduced. The stock Mario item-throw
scripts inspected during follow-up use `flag0` to release the item and `flag3` for turn timing;
the throw state otherwise retains its default damage multiplier, velocity multiplier, and angle.
The replay corpus that found the layout discrepancy did not exercise a `flag1`/`flag2` override.

This is therefore a definite layout defect and a potentially reachable gameplay bug, not evidence
that the earlier Mario Fireball regression returned. The Fireball issue was the separate, resolved
`WPAttributes` ROM-layout bug documented in `wpattributes_bitfield_padding_2026-04-20.md`.

## Fix

For `PORT` builds, the item-throw bitfield member is no longer part of `FTCommandVars`.
`ftCommonItemThrowProcUpdate` decodes the N64-defined command words directly:

```c
damage = flag1 & 0x00FFFFFF;
vel = (flag2 >> 12) & 0xFFF;
angle = BITFIELD_SEXT(flag2 & 0xFFF, 12);
is_throw_item = flag0;
```

The original bitfield view and accesses remain in the non-port build so the matching N64 build is
unchanged. A port-side static assertion now requires `sizeof(union FTCommandVars) == 16`, preventing
MSVC or a future declaration change from silently moving subsequent `FTStruct` fields again.

## Verification

- GCC 16 full `ssb64` build passes with the corrected port path and the 16-byte assertion.
- Build both a clang/GCC host and MSVC; the static assertion must pass and the startup probe must
  report `FTCommandVars=16` on both.
- Exercise ordinary and smash item throws to ensure the existing `flag0`/`flag3` behavior remains
  unchanged.
- If a script using `flag1`/`flag2` is identified, compare its resulting damage, velocity, and
  angle directly against the N64 ROM. Host-to-host agreement alone is not an N64 correctness test.

## Audit hook

Any union written through one member and read through another is suspect when either member mixes
plain fields and bitfields. Prefer raw fixed-width words plus masks and shifts at the interpretation
site; never use host bitfield placement as an interchange format.
