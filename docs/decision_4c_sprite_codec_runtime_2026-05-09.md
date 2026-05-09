# Decision: keep 4c sprite codec + post-decode deswizzle on the runtime side

**Date:** 2026-05-09
**Status:** Accepted
**Context:** Build-time reloc refactor, Stage 8
**Related:** `docs/refactor_buildtime_reloc_plan.md`,
`docs/refactor_buildtime_reloc_handoff.md`,
`decomp/src/lb/lbcommon.c:2383..2422` (`lbCommonDecodeBitmapSiz4b` /
`lbCommonDecodeSpriteBitmapsSiz4b`),
`decomp/src/lb/lbcommon.c:3067..3125` (`lbCommonMakeSObjForGObj` —
the sole 4c call site),
`port/bridge/lbreloc_byteswap.cpp:2174..2222`
(`portDeswizzleDecodedSprite4c`),
`docs/bugs/sprite_32bpp_tmem_swizzle_2026-04-20.md` (where the
"4c excluded from pre-decode deswizzle" carve-out was hardened)

## TL;DR

Stage 8 of the build-time reloc refactor was a decision point: move the
4c sprite codec (`lbCommonDecodeSpriteBitmapsSiz4b`) and its post-decode
deswizzle (`portDeswizzleDecodedSprite4c`) into torch, or leave both at
runtime. Decision: **leave at runtime**. The plan already telegraphed
this — its non-goals list says "Decompressing 4c sprites at build time
is not in scope" (`refactor_buildtime_reloc_plan.md:76-78`); this ADR
formalizes the rationale and closes the stage. **No code change, no new
`processing_flags` bit, no fixture regen.**

## What was on the table

`lbCommonMakeSObjForGObj` (sole 4c call site, `decomp/src/lb/lbcommon.c:3067`)
runs three things when `sprite->bmsiz == G_IM_SIZ_4c`:

1. `lbCommonDecodeSpriteBitmapsSiz4b(sprite)` — walks
   `sprite->bitmap[i]`, calls `lbCommonDecodeBitmapSiz4b` per bitmap to
   expand 2bpp source bytes into 4bpp output **in-place** in the same
   `bitmap->buf` that PORT_RESOLVE returns. After decode the sprite is
   relabeled `bmsiz = G_IM_SIZ_4b`.
2. `portDeswizzleDecodedSprite4c(sprite, bitmaps)` — applies the XOR-4
   TMEM-line deswizzle that the pre-decode `portFixupSpriteBitmapData`
   path explicitly skips for 4c (carve-out documented at
   `port/bridge/lbreloc_byteswap.h:119..127` and on the in-cpp comment
   `lbreloc_byteswap.cpp:2120..2127`).

The migration target was: have torch decode every 4c bitmap to 4bpp at
extraction, apply the deswizzle, write decoded bytes plus a flag bit
(`PROC_4C_DECODED`?) so the runtime sees `bmsiz != G_IM_SIZ_4c` and
both helpers no-op.

## Why we are leaving it at runtime

### 1. The decoded bytes don't have a home in the file

The codec writes back into `bitmap->buf`, which is a PORT-resolved
pointer to runtime heap. The on-disk file holds the 2bpp source — that
is its canonical layout. Pre-decoding at build time means *replacing*
the 2bpp file bytes with 4bpp output, which doubles texel-data size
(every 4c bitmap goes from `(width_img/4)*height` bytes to
`(width_img/2)*height`) and changes the field
`sprite->bmsiz`. The container `Sprite` struct then no longer matches
what decomp source still asserts on (`if (sprite->bmsiz == G_IM_SIZ_4c)`)
unless we also rewrite that check under `#ifdef PORT`. Cascading
runtime invariant changes for a "no-op'd" path.

The other migrated transforms (Pass 1, Pass 2, struct fixups, halfswap)
share the property that the post-transform layout fits in the same byte
budget as the pre-transform layout — the file is the same size, only
the byte order of fields changes. 4c → 4b breaks that invariant.

### 2. Status-quo cost is negligible

- **Decoder runs once per sprite, on first SObj construction.** Each
  call decodes one nibble per source byte; total work for a fully-
  populated character intro sprite (~16 bitmaps, ~64×64 each) is well
  under a frame.
- **`portDeswizzleDecodedSprite4c` is idempotent** via the dedicated
  `sDeswizzle4cFixups` tracker (`lbreloc_byteswap.cpp:644`); repeat
  calls on the same `bitmap->buf` are O(1).
- **Heap reuse evicts both trackers** (`lbreloc_bridge.cpp` →
  `evict_set(sDeswizzle4cFixups)` at `lbreloc_byteswap.cpp:1240`),
  so reused slots get re-decoded correctly on the next load.

The runtime costs nothing measurable. Build-time migration recoups
nothing visible.

### 3. The codec doesn't unlock moddability

Stage 10's premise is that modders override sub-assets (vertices,
textures, TLUTs, sprites, structs) via authoring tools that emit the
canonical wire format. For 4c specifically:

- A modder swapping a sprite's pixels would author the *result* (a
  4bpp or 16bpp image), not the 2bpp compressed source. There is no
  authoring tool that round-trips through 4c encoding.
- If anyone needs 4c-equivalent compression on a custom sprite,
  Stage 10's sub-asset path (`reloc/<id>/tex/<offset>/<fmt>_<siz>`)
  carries the format/size pair. A mod can override the 4c sprite with
  a 4b or 16b override; the runtime decoder is gated by `bmsiz` and
  simply doesn't fire. Build-time decode would force-collapse the
  `bmsiz` field, removing the modder's ability to ship 4c-compressed
  custom sprites if they wanted to.

Either way the codec doesn't bind moddability. Keeping it preserves
optionality.

### 4. The codec is small but coupled to runtime decisions

The two functions together are ~34 LOC of nibble unpacking
(`lbCommonGetBitmapDecodeNibble` table lookup +
`lbCommonDecodeBitmapSiz4b` reverse-walk). Trivial to port —
that's not where the cost is.

The cost is in the **side effects bound to the runtime call site**:

- Mutating `sprite->bmsiz` from `G_IM_SIZ_4c` to `G_IM_SIZ_4b`
  *after* decode is what the rest of the rendering pipeline reads.
  Doing this at build time means the post-pass1 bytes already report
  4b, but the file still has the 4c-sized buffer (or a doubled buffer
  if we resize). Either choice is a layered invariant change.
- The decoder's reverse-walk-into-same-buffer pattern
  (`lbCommonDecodeBitmapSiz4b`: `bitmap_csr` walks down from
  buf+(res-1), `bitmap_buf` walks down from buf+(res/2-1)) only works
  if the buffer was sized for the 4bpp output but the data starts as
  2bpp source — it consumes from the back half and writes from the
  middle, with the heads passing each other. That allocation-shape
  decision is bound to the file format. Migrating it to torch means
  pre-allocating the output region in the extracted blob and shifting
  the source bytes into it, which is more invasive than just running
  the codec.

`portDeswizzleDecodedSprite4c` is similarly small (~50 LOC) but
mechanically chained to the decoder — it must run after, on the same
buffer the decoder just wrote. Migrating one without the other is
nonsensical; migrating both together inherits the bmsiz/buffer-size
invariant problem above.

### 5. The plan already calls this out

`refactor_buildtime_reloc_plan.md:76-78` (non-goals):

> Decompressing 4c sprites at build time (decompression is on the
> runtime side of a sprite-bank flag and can stay there; only the
> post-decode deswizzle matters for the test gate).

The Stage 8 description (`refactor_buildtime_reloc_plan.md:440-455`)
explicitly recommends Option A. The existing handoff (Stage 6 archive,
`refactor_buildtime_reloc_handoff.md` "Gotchas" item 1) restates that
`portFixupSpriteBitmapData` is the post-decode 4c sprite path and is
intentionally left runtime-side. This ADR is the formal closeout; the
substance was already settled.

## What this changes

- **No `processing_flags` bit allocated.** Bits 9 (Stage 7-AObjEvent32
  carve-out, reserved-unused) and the existing 0..8 stand. Stage 9
  takes bit 10 (`PROC_CHAIN_FLATTENED`), Stage 10 takes bit 11
  (`PROC_SUBRESOURCES_EMITTED`) as originally planned.
- **No code change.** `lbCommonDecodeSpriteBitmapsSiz4b` stays in
  `decomp/src/lb/lbcommon.c`. `portDeswizzleDecodedSprite4c` stays in
  `port/bridge/lbreloc_byteswap.cpp`. Both remain compiled in for all
  builds.
- **No fixture regen.** Fixtures don't capture post-decode buffer
  state today (heap-absolute, not file-static); leaving the runtime
  in place keeps that property.
- **Stage 11 (runtime simplification) gets a documented carveout**:
  `lbreloc_byteswap.cpp` cannot be reduced to "thin chain-registration
  wrapper" — it retains the 4c deswizzle helper plus the heap-absolute
  trackers. The doc-level shrinkage estimate from `~1500 LOC to thin
  wrapper` should be tempered to "~1500 LOC to ~150 LOC of trackers
  + 4c deswizzle + chain registration."

## What would change this decision

Three triggers:

- **Authoring-tool support for 4c.** If a future Blender/fast64-style
  workflow round-trips through 4c encoding for SSB64 sprites, the
  argument that "modders don't author 4c" goes away. At that point a
  build-time codec gives modders a stable byte representation to
  override.
- **Demonstrated runtime hot-path cost.** The decode runs once per
  sprite per session. If a profile ever shows it on a frame critical
  path (e.g. a stage with hundreds of newly-spawned 4c sprites
  per second), build-time decode trades file size for frame budget.
- **A 4c-specific bug with a hard-to-test runtime fingerprint.** If
  a regression class lands that's hard to reproduce in fixture
  testing because it depends on heap allocation patterns at decode
  time, lifting the codec to build time turns it into a deterministic
  byte-transform that the existing drift gate covers. Today the
  signal is the opposite: the 4c path has been stable since the
  2026-04-20 sprite-decode work
  (`docs/bugs/sprite_decode_stride_mismatch_2026-04-20.md`,
  `sprite_32bpp_tmem_swizzle_2026-04-20.md`).

Until one of those triggers fires, runtime is the right home for
both pieces.
