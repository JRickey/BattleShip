# Reloc token generation ↔ GBI opcode misparse (chain-walk time bomb)

**Date:** 2026-07-31
**Status:** FIXED (port bridge only — no decomp/libultraship changes)
**Symptoms:** garbled particle/effect textures appearing only after long play
sessions; intermittent `abort()` in `portRelocRegisterPointer`
("token index capacity exhausted") during a scene transition, observed at the
start of the second attract cycle (~frame 15400) loading
`reloc_effects/EFCommonEffects2`.
**Sibling of:** [texcache_stale_identity_aliasing_2026-07-30](texcache_stale_identity_aliasing_2026-07-30.md)
— same "in-place byte-permutation applied to the wrong bytes" family, different
trigger: this one is **generation-dependent, not timing-dependent**.

## Root cause

Reloc tokens are `[12-bit slot-generation][20-bit slot-index]`, so a token's
**top byte is `gen >> 4`**. Slot generations bump on every
`portRelocInvalidateRange` recycle (≈ once per scene transition per hot slot).
After ~13 transitions, hot freelist slots reach **generation 16**, and every
token minted from them looks like `0x01xxxxxx`.

The chain walk in `portRelocLoadFileFromBytes` calls
`portRelocFixupTextureFromChain(base, size, slot_off, target_off)` for every
chain entry, which reads the entry's "command word" from `slot_off - 4` and
dispatches on its top byte: `0x01` → `chain_fixup_vertex`, `0xFD` →
`chain_fixup_settimg`. But chain entries are frequently **adjacent words**
(consecutive slots 1290,1291,…), so for entry *k+1* the word at `slot-4` is
**the token the walk just wrote into entry *k***.

At gen 16..31 that token parses as a G_VTX command — and passes every
structural check by construction:

| check | why a gen-16 token passes |
|---|---|
| opcode `0x01` | gen 16..31 → top byte `0x01` |
| reserved bits [23:20] == 0 | gen low nibble lands in [23:20]; gen 16 = `0x010` → 0 |
| bit 0 == 0 | token low bit = slot-index parity (even half the time) |
| `num_vtx` in 1..32 | bits [19:12] = index bits; small hot indices give 1..0x40-ish |
| vertex coord sanity | the "target" is chain-descriptor data: two u16 file offsets < file size — indistinguishable from small positive vertex coords |

`chain_fixup_vertex` then applies its per-vertex byte permutation (rot16 ×3 +
BSWAP32, stride 4) over `target .. target + num_vtx*16` — **fresh file bytes**.
Usually the range is texel/geometry data → garbled particle textures, no
crash. In the captured crash it covered five *not-yet-walked chain
descriptors* (words 2350–2369 of EFCommonEffects2): the walk read a
halfword-rotated descriptor, took the rotated *target* field as "next",
wandered off-chain, and entered a terminal feedback loop — every freshly
minted token in the `0x0017xxxx` range re-decodes as chain word "next = slot
23", so the walk re-visited slot 23 forever, registering a token per
iteration. 505,890 registrations later the 20-bit index space could not
double again → `abort()`.

There is a second hazard band: gens 4048..4063 mint `0xFDxxxxxx` tokens
(G_SETTIMG) → `chain_fixup_settimg` BSWAP32-bulldozes a "texel" range the
same way.

## Why it was so hard to see

- **Intermittent by generation drift, not by race.** 8 consecutive 30k-frame
  soaks passed while the 9th aborted: whether a *hot* slot's gen hits 16 by
  the transition that loads an effects file depends on run-to-run variance in
  effect/particle load composition (RNG demo fights). Any sufficiently long
  session eventually arms the band — matching the original "corrupted particle
  textures … eventually crashes" long-session reports.
- All the timing/threading theories were falsifiable: DL consumption is
  synchronous inside `osSpTaskStartGo` (only the SP/DP interrupt *signals* are
  deferred), so there is no stale in-flight render task to blame.
- The abort's `fault_addr=0x3e8000be6af` is SIGABRT siginfo (uid 1000, pid
  0xbe6af), not a pointer — a red herring unless decoded.

## Autopsy trail (technique notes)

Core of the abort (`coredumpctl`, gdb python over
`'(anonymous namespace)::sSlots'`):

1. Slot-table census: 524,287 live slots, freelist empty, 96.5% pointing into
   the scene arena, top duplicates registered 9–28× → runaway registration
   loop, not a leak.
2. Frozen locals of `portRelocLoadFileFromBytes`: `words_num=0xFFFE`,
   `target_byte_off=262136` (233 KB past a 28 KB file), token `0x17FFFE`
   re-decoding as `next=23` → self-loop identified.
3. Byte-diff of in-core `ram_dst` vs the still-alive pristine `src_bytes`
   (`shared_ptr` in a parent frame): mutated words are exactly
   rot16/rot16/rot16/BSWAP32 stride-4 → `chain_fixup_vertex`'s signature,
   based at word 2350 — an offset **no pristine chain entry targets**
   (replayed all 166 entries' decisions offline from the pristine bytes).
4. The in-core chain tokens are gen 16 (`0x0100401e`…) → the misparse source
   is the walk's own freshly written tokens.

## Fix (three layers, all in `port/bridge/` + `port/resource/`)

1. **`portRelocFixupTextureFromChain` refuses tokens as commands**
   (`lbreloc_byteswap.cpp`): if the w0 address is a registered chain slot
   (`sChainSlotAddrs`) whose word still resolves via
   `portRelocTryResolvePointer`, it is a live token, never a command → skip
   (log marker `chainFixup SKIP w0-is-chain-token`, first 16). Corpse entries
   are pruned so a stale registry can't suppress real fixups.
2. **All three chain walks are contained** (`lbreloc_bridge.cpp`: public
   intern, extern, and `portRelocLoadFileFromBytesPrivate`): slot offset must
   lie inside the file, step count can't exceed the file's word count, and
   intern targets must be in-bounds. Violation → `chainWalk STOP` log + break
   (file keeps all fixups applied so far; no table explosion; scene keeps
   running).
3. **Deterministic repro knob** (`RelocPointerTable.cpp`):
   `SSB64_RELOC_GEN_SEED=<n>` pre-ages all slot generations at boot.
   Seed 15 arms the G_VTX band from the first registration; seed 4047 arms
   the SETTIMG band. Zero cost unset.

Verified: seeded runs (`15` and `4047`) show the guard firing
(`chainFixup SKIP w0-is-chain-token w0=0x010000E2` / `0xFD0000E2`) with zero
`chainWalk STOP`, zero crashes, full clean intro. Unseeded 10-run × 30k-frame
soak on the hardened build: see soak notes in the branch write-up.

## Audit hooks

- Any address-keyed registry consulted by a byte-permuting fixup must
  distinguish *live* entries from *corpses* (validate the word still resolves)
  — third instance of this pattern (runtime clamp, texcache verify, now w0
  guard).
- Any value space that overlaps GBI opcode space (tokens, indices, handles
  stored into DL-adjacent words) will eventually be parsed as a command by
  heuristic scanners. Either tag the value space (top-bit marker) or make
  every scanner consult the registry of written slots.
- `abort()`-on-exhaustion in a handle table is a *symptom amplifier*: the
  table filling at 500k+ registrations means a producer loop upstream. Census
  the live pointers by range and look for duplicates first.
