# Intro memory-corruption family — runtime texture fixup bulldozer + texture-cache identity aliasing

**Date:** 2026-07-30/31
**Class:** Port-side caches and lazy fixups keyed by raw address / identity, colliding with
the game's recycled-address memory model (issue #103/#128 family; spatial staleness invisible
to ASan)
**Symptom:** Intermittent corrupted particle textures and stage backgrounds during the intro
movie chain, varying launch-to-launch; textureless ("flat") fighters/objects in the opening
clips; escalates to SIGSEGV during a stage (observed: `gcSetupCustomDObjsWithMObj` walking
off a DObjDesc array on Link's bomb spawn, `wpMarioFireballMakeWeapon` NULL `->mobj`,
`GfxSpVertex` walking `vertices=0x1000000`).
**Status:** FIXED — five patches, defense in depth. Root mechanism reproduced 3/3 on natural
boot and verified via paired core-dump autopsy (healthy vs corrupted process).

## The decisive evidence

Two core dumps of the same code path — one healthy (direct entry into the Link opening
clip), one corrupted (natural full boot) — show the **same file at the same offset**
(`gFTDataLinkMain` + 0x4218, the Link bomb's DObjDesc array):

- Healthy: correctly byte-swapped descriptors, tokenized `dl` slots (`0x00100788`), LE floats.
- Corrupted: **the identical bytes BSWAP32'd wholesale — including the token slots**
  (`0x17462000` = token `0x00204617` byte-reversed; the logged "impossible" token
  `0xF04F3000` = valid token `0x00304FF0` byte-reversed).

So the file loaded and relocated correctly, and was then re-byte-swapped in place
afterwards. The only code that BSWAP32s reloc-file memory post-load is
**`portRelocFixupTextureAtRuntime`** (`port/bridge/lbreloc_byteswap.cpp`) — the lazy
texel-order fixup invoked from the interpreter's `GfxDpLoadBlock`/`LoadTile`/`LoadTLUT`
on the current SETTIMG address. Its only gate was "is the address inside a reloc file".
A single garbage/stale SETTIMG operand pointing into file data (seeded by whatever stale
DL the earlier intro scenes produce) makes the renderer byte-reverse kilobytes of live
descriptors, tokens, and animation data **in game memory**. Every later consumer of that
file then misbehaves:

- BE ids in DObjDesc arrays mask to 0 and the terminator is unrecognizable → item spawn
  walks run off the array into float tables → textureless objects, then SIGSEGV.
- Byte-reversed tokens resolve to NULL (`invalid/stale token` warnings with impossible
  generations) → SETTIMG NULL → the `null texture address` bursts (~6 draws/frame for a
  whole clip) → flat fighters and wrong stage textures.

Why intermittent per launch: whether the bogus SETTIMG lands on (and how the guard bits
line up with) live data depends on token values and heap/session state; the natural-boot
save state reproduced 3/3 while `SSB64_START_SCENE` direct entries (different unlock mask
and load sequence) stayed clean.

Why ASan never saw it: every access is to validly mapped, currently-owned memory. The
ASan attract loop (with the new `SSB64_SCENE_HEAP_FREE=1` arena-free mode) ran multiple
full cycles with zero reports while the Debug build crashed 3/3 — this family is spatial
corruption, not temporal UAF.

## Fixes

1. **Chain-slot clamp on the runtime texture fixup** (`port/bridge/lbreloc_byteswap.{h,cpp}`,
   `lbreloc_bridge.cpp`): both reloc chain walks now record every tokenized slot address
   (`portRelocNoteChainSlot`, evicted with the existing range/reset evictors). The
   corrupting requests turned out to be *legitimate* texture draws whose N64 fixed-size
   LoadBlock **over-reads past the true texel span** into the adjacent struct area —
   harmless on hardware (extra bytes only landed in TMEM) but destructive here because
   the fixup mutates source memory. Texel data never contains chain slots, so
   `portRelocFixupTextureAtRuntime` now clamps the swap at the first slot in the range
   (logging `runtimeTexFix CLAMPED ...`): the real texture bytes get correct BE
   restoration, the pointer-bearing tail is left untouched, the cascade never starts.
   (A first-cut version *rejected* the whole swap — memory-safe, but it left the
   affected particle textures visibly garbled; the clamp supersedes it. The stable
   repeat offenders live in files 106/157/349 (`ExternDataBank106`, `MiscDataBank157`,
   `SamusSpecial2` — the charge-shot effect file), ~2-4 KB loads.)

   **Clamp addendum — corpse registry entries.** First user test of the clamp still
   showed garbled textures at the Master Hand room (Mario's entrance): live-core
   inspection showed the "slots" bounding those clamps held `0x0` / animation data,
   not tokens — the registry carried **corpse addresses from earlier load epochs**.
   Cause: `lbRelocGetForceExternHeapFile` rewinds the force heap and reloads at the
   same addresses, but each load evicts only its own byte range, so a smaller reload
   leaves the dead tail's entries in every address-keyed registry (chain slots, fixup
   memos, texture cache, DL ranges). Two further fixes: (a) the clamp validates the
   candidate slot's word via `portRelocTryResolvePointer` — non-resolving words are
   corpses, dropped from the registry, and the texture swaps fully; (b) the force-heap
   rewind now calls `port_taskman_evict_arena_caches` over the whole rewound region
   up front. (b) also closes the pre-existing sibling hole where corpse *fixup memos*
   made the runtime swap skip words of freshly-loaded texel data.

2. **Guards un-disabled on Linux** (`libultraship/src/fast/interpreter.cpp`,
   `gfxPointerInLoadedModule`): the SETTIMG/G_VTX low-address guards had an exception for
   "pointer inside a loaded module" (TCC mod DLLs). The port links non-PIE on Linux, so the
   main executable itself occupies the low VA range and `dladdr` resolved every stale
   N64-segment leftover (0x01000000, 0x0E000000, …) as "in a module" — the guards were
   structurally dead, which is how `GfxSpVertex(vertices=0x1000000)` SIGSEGV'd instead of
   being skipped. The check now excludes the main program image on both Linux and Windows.

3. **Content-verified texture cache** (`libultraship` `interpreter.{h,cpp}`): Fast3D's
   texture cache keys on raw pointer + shape and never re-reads memory on a hit, so any
   recycled buffer serving a different texture at an aliased key silently renders stale
   texels. `TextureCacheValue` now stores an FNV-1a of the source bytes; every hit
   re-hashes (exactly the range a miss would read) and a mismatch evicts + re-imports +
   logs `TextureCache stale-content hit healed`. Default ON; `SSB64_TEXCACHE_VERIFY=0`
   disables. Verified live: healed real aliased hits (48x48 IA8) during attract runs.

4. **Particle bank lockstep eviction** (`port/bridge/particle_bank_bridge.cpp`): the
   per-bank working buffers were freed and re-allocated with no cache eviction — not in
   the scene arena, not routed through `lbRelocLoadAndRelocFile`, so none of the three
   range evictors ever ran on them. glibc hands the freed chunk back to the same-size
   replacement, so texture-cache keys aliased across bank reloads. Now evicts
   texture/packed-DL/struct-fixup caches over both buffers before the free — the same
   contract the reloc loader honors.

5. **OOB fighter-scale seed + stale effect-file scrub** (`decomp/src/ft/ftport.c`,
   `decomp/src/ef/efmanager.c`, `decomp/src/sys/taskman.c`):
   `port_fighter_seed_vanilla` read the 12-row `dSCSubsysFighterScales` for all 27 vanilla
   fkinds (caught by ASan at first boot; polygon/boss/metal rows got adjacent-global
   garbage — clamped to `nFTKindPlayableEnd`, 1.0 fallback). `gEFManagerFiles[0..2]` now
   cleared in `syTaskmanStartTask` after the arena wipe so scenes that skip
   `efManagerInitEffects` (title) hit `efManagerMakeEffect`'s NULL guard instead of
   carrying non-NULL-but-stale arena pointers.

## New diagnostic: `SSB64_SCENE_HEAP_FREE=1`

`decomp/src/sys/taskman.c` (`syTaskmanStartTask`): env-gated sanitizer mode that
free()s + malloc()s the 16 MiB scene arena per transition instead of recycling it. Under
ASan, any stale cross-scene physical read becomes a heap-use-after-free report with full
stacks. Zero cost when unset. Multiple clean full-cycle ASan runs with this mode prove
the temporal-UAF class is currently clean — the remaining corruption family is spatial.

## On "memory that is never freed during the intro"

Audited: the two blocks first allocated during the intro and never freed are
`gPortSceneHeap` (16 MiB arena, first allocated at the N64-logo scene) and the reloc
token table `RelocPointerTable::sSlots` (~4 MiB). **Both are by design and load-bearing.**
The original game bump-allocates from one arena and "frees" by pointer reset; long-lived
references into it are legal on N64. Freeing the arena per scene (tried as PR #144) or
resetting the token table per scene (the historical `portRelocResetPointerTable` call)
reintroduces the #103/#128 crash family. Everything else on the intro path is bounded or
correctly torn down; there is no growing per-attract-iteration host leak.

## Audit hooks

- Any port-side lazy fixup that mutates game memory must positively identify its target
  (whitelist), not merely bounds-check it — "inside a reloc file" is not a type check.
- Any buffer carrying texture/DL/struct data freed outside `lbRelocLoadAndRelocFile`
  must call the three range evictors first. The content-verified cache turns a missed one
  into a `stale-content hit healed` log line instead of silent corruption.
- Guard predicates that consult `dladdr`/module ranges must exclude the main image on
  non-PIE targets.

## Related

- `docs/bugs/dl_link_stale_pointer_guard_2026-05-09.md` — temporal sibling; the
  recycled-arena band-aid and the "resume the hunt" recipe this session used
- `docs/bugs/linux_stale_scene_data_family_2026-05-11.md` — the variants this write-up
  finally explains (variant 2's `0x80808000…` reads = BSWAP'd data, variant 3's
  impossible-generation tokens = byte-reversed tokens)
- `docs/issue128_investigation_2026-05-03.md` — the four leads (all landed on main)
- `docs/bugs/particle_banks_silent_empty_2026-04-20.md` — the bridge whose buffers alias
