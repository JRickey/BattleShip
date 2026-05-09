# Build-time reloc refactor — Stage 6 handoff (2026-05-08, evening)

You are picking up the build-time reloc refactor described in
`docs/refactor_buildtime_reloc_plan.md` at **Stage 6 (struct fixups → torch)**.
**Read the plan first** — this handoff is the delta.

## Where the work lives

- **Outer worktree**: `.claude/worktrees/buildtime-reloc` on branch
  `agent/buildtime-reloc`. The main tree is left undisturbed.
- **Submodules**:
  - `decomp` → `port-patches-buildtime-reloc` (clean, no commits yet)
  - `libultraship` → `ssb64-buildtime-reloc` @ `5b87696f` (Stage 1 patch)
  - `torch` → `ssb64-buildtime-reloc` @ `78a2984` (Stages 4 + 5)

`./scripts/new-worktree.sh` doesn't apply — the worktree exists. Just `cd`.

## State at HEAD

```
5d43e46 Bump torch + complete pass2 buildtime migration (Stage 5)
4da6a42 port/bridge: split pass2 byte transforms from tracker bookkeeping
99fd326 Bump torch + plumb v1 reloc resource: pass1 BSWAP32 → torch (Stage 4)
8b167d4 port/bridge: skip pass1 BSWAP32 when ProcessingFlags advertises it
1a52dc4 port/resource: ProcessingFlags + RelocFile v1 factory
2083aa6 docs: handoff doc for the next session   (← prior handoff, superseded)
64cda95 port/test: commit ground-truth fixtures for all 2,132 reloc files
…
```

Stages 1–5 are committed and the drift gate is green. Stages 4 + 5 lifted Pass 1
(blanket BSWAP32) and Pass 2 (DL-walk per-Vtx + per-format texture fixups)
into `torch/src/factories/ssb64/RelocFactory.cpp`.

## Sanity check first

```
cd .claude/worktrees/buildtime-reloc
./build/port/test/port_reloc_regen --all --check --quiet
```

Expected: `unchanged=2132, mismatch=0, …`. If it drifted, the o2r is from a
different torch SHA — `cmake --build build --target ExtractAssets` after wiping
`build/BattleShip.o2r`.

## The contract you inherit (read carefully)

`RelocFile::ProcessingFlags` (in the v1 RELO container header) is a `u32`
bitmask advertising which transforms torch already applied:

| bit | name                        | who owns                     |
|-----|-----------------------------|------------------------------|
| 0   | `PROC_PASS1_BSWAP_DONE`     | Stage 4 (done)               |
| 1   | `PROC_PASS2_DONE`           | Stage 5 (done)               |
| 2–7 | reserved struct-fixup families | **Stage 6 (you)**         |
| 8–9 | reserved halfswap/AObjEvent | Stage 7                      |
| 10  | reserved chain-flatten      | Stage 9                      |
| 11  | reserved sub-resource emit  | Stage 10                     |

The runtime (`portRelocByteSwapBlob`, `portFixupStruct*` helpers) reads the
flags and skips its own byte transforms when the matching bit is set.

### Stage 5 invariant — Stage 6 will hit this hard

Stage 5 surfaced an issue Stage 6 will hit on every helper: **the byte
transform half of a fixup migrates to torch, but tracker bookkeeping does
NOT.** `sStructU16Fixups` (and its siblings `sTexFixupWords`,
`sDeswizzle4cFixups` in `lbreloc_byteswap.cpp`) are heap-absolute idempotency
state, keyed by `(uintptr_t) &fixed_address`. Lazy paths
(`portRelocFixupVertexAtRuntime`, `portRelocFixupTextureAtRuntime`,
sprite/MObjSub helpers) consult them to skip already-fixed addresses.

Stage 5's pattern (`apply_fixups` in `lbreloc_byteswap.cpp:870` area):

```cpp
case FIXUP_VERTEX:
    if (!transforms_done_at_build)
        apply_fixup_vertex(region_words, num_words);   // ← byte transform
    // tracker insertion runs unconditionally — heap-absolute, can't move
    for (size_t v = 0; v < n_vtx; v++)
        sStructU16Fixups.insert(base + v * 16);
    break;
```

The fixtures pin tracker contents — `port_reloc_check` will fail if you skip
the insertions. The drift gate enforces the invariant for free.

For Stage 6, every `portFixupStructU16/U32/Sprite/Bitmap/MObjSub/FTAttributes`
helper has the same shape:

```c
void portFixupStructU16(void *base, unsigned int byte_offset, unsigned int num_words) {
    uintptr_t key = (uintptr_t)base + byte_offset;
    if (sStructU16Fixups.count(key)) return;            // 1. already-fixed?
    sStructU16Fixups.insert(key);                       // 2. record
    /* … byte transform body … */                       // 3. transform
}
```

Stage 6's migration: **step 3 → torch, steps 1 + 2 → still in runtime.**

## Eight helpers in scope

Defined in `port/bridge/lbreloc_byteswap.cpp`:

```
portFixupStructU16        line 1031
portFixupStructU32        line 1050
portFixupRawTextureBSWAP32  line 1109   ← niche; called from chain helper
portFixupSprite           line 1710
portFixupBitmap           line 1758
portFixupBitmapArray      line 1783
portFixupSpriteBitmapData line 1817   ← post-decode 4c; LEAVE RUNTIME-SIDE (Stage 8)
portFixupMObjSub          line 2055
portFixupFTAttributes     line 2148
```

99 call sites across `decomp/src/**/*.c`:

```
$ grep -rln 'portFixup\(StructU16\|StructU32\|Sprite\|Bitmap\|MObjSub\|FTAttributes\)\b' \
    decomp/src --include='*.c' | wc -l
```

## Catalog approach — pick one

The plan calls for static-analysis mining. **Reconsider.** Most call sites
look like `portFixupStructU16(wp->weapon_vars.smog.attr, 0x10, 6)` — `base`
is a runtime pointer assembled through dereferences whose target file_id is
not visible from source. Static analysis would need data-flow + type-to-file
mapping; runtime capture is a one-liner.

### Option A — runtime capture (recommended)

1. Add a one-shot logger inside each `portFixup*` helper that emits
   `(file_id, base_offset_in_file, byte_offset, num_words, family)` to a
   side file when `SSB64_FIXUP_CAPTURE=1`. Use the existing
   `portRelocFindFileIdAndBase(base, &file_base)` to recover file_id +
   address-relative-to-file-base.
2. Boot the game and run attract chain + 1P stage 1 + training + VS results
   + BTT + each fighter intro. Each `unique` tuple becomes a catalog row.
3. Dedup, commit `port/test/struct_fixup_catalog.json` (or `.bin` for size).

**Coverage caveat**: any code path not exercised during capture is missing
from the catalog → runtime fallback fires → drift gate stays green. The
migration is fail-safe; an incomplete catalog just means torch can't skip
those calls. Not a blocker.

### Option B — static analysis

Per the plan. Workable but more brittle; you'll rediscover the data-flow
issue on call sites like `lb/lbcommon.c:1071`.

## Migration shape (per family)

For each family bit (2..7):

1. Torch reads the catalog and applies the family's byte transform to the
   relevant offsets in the post-pass1+pass2 buffer.
2. Torch sets `PROC_<FAMILY>_DONE` in `processing_flags`.
3. Bump torch SHA, re-extract o2r, rerun drift gate. Should stay green —
   the runtime helper sees the bit, skips the transform body, but still
   does the tracker insertion + idempotency check. Bytes match (torch did
   them), trackers match (runtime updated them).

Per-family rollout means you can bisect the regression to one family if
something breaks.

## Decomp side — the hard part

The bridge needs to know "did torch already fix this `(file_id, byte_offset)`?"
when `portFixupStructU16` is called. Current sketch:

```c
extern "C" void portFixupStructU16(void *base, unsigned int byte_offset, unsigned int num_words) {
    uintptr_t key = (uintptr_t)base + byte_offset;
    if (sStructU16Fixups.count(key)) return;
    sStructU16Fixups.insert(key);

    uintptr_t file_base;
    int file_id = portRelocFindFileIdAndBase(base, &file_base);
    bool torch_did_it = (file_id >= 0)
        && portStructFixupCatalog::HasEntry(file_id,
                                            (uintptr_t)base + byte_offset - file_base,
                                            kFamilyStructU16);
    if (torch_did_it && portRelocFileHasFlag(file_id, PROC_STRUCT_U16_DONE))
        return;   // bytes already correct, tracker already inserted

    /* … existing byte transform body … */
}
```

Generate the catalog as a runtime lookup (`port/resource/StructFixupCatalog.{h,cpp}`)
alongside the existing `RelocFileTable.cpp` codegen.

## Manual playthrough is critical here (not optional)

Pass 1+2 are pure deterministic byte functions; the drift gate covers them
end-to-end. Struct fixups touch gameplay-critical state:

- `FTAttributes`: fighter physics, throw frame data
- `MPGroundData`: stage collision (memory: Sector Z, Mushroom Kingdom)
- `MObjSub`: matanim color command overlays (memory: Whispy canopy)
- `Sprite`/`Bitmap`: menu artwork, HUD

A wrong byte order in any of these is a gameplay regression that doesn't
crash. **Sweep at each family rollout**: attract chain → 1P stage 1 →
training mode → VS → BTT → each character intro.

## File map (Stage 6 additions)

```
tools/capture_struct_fixups.py         NEW: aggregate runtime capture
                                            (or mine_struct_fixups.py for Option B)
port/test/struct_fixup_catalog.json    NEW: committed catalog
port/resource/StructFixupCatalog.{h,cpp}   NEW: runtime lookup
torch/src/factories/ssb64/RelocFactory.cpp   MODIFY: apply catalog entries
port/bridge/lbreloc_byteswap.cpp       MODIFY: each portFixupStruct* helper
                                                consults the catalog
port/resource/RelocFile.h              MODIFY: add PROC_STRUCT_*_DONE bits
```

## Gotchas

1. **Don't migrate `portFixupSpriteBitmapData`**. It's the post-decode 4c
   sprite path; the data isn't in the file's static layout (heap-allocated
   by the runtime decoder). Plan's Stage 8 explicitly keeps the 4c codec
   on the runtime side. Catalog-mining will naturally skip these (the
   `base` doesn't resolve to a file via `portRelocFindFileIdAndBase`).

2. **`portFixupSprite` bundles a texture-cache eviction.** It calls
   `portTextureCacheDeleteRange` to invalidate Fast3D's upload cache
   when the underlying bytes change. That eviction is heap-absolute
   side-effect work — keep it in the runtime even when torch does the
   byte transform. Same shape as Stage 5's tracker insertion.

3. **`portFixupMObjSub` operates on chain-targeted data.** Reachable via
   `chain_fixup_*` from `relocIntern`/`relocExtern`, not the static DL
   stream pass2 walks. Capture-from-runtime (Option A) handles this for
   free; static analysis would need to follow the chain encoding too.

4. **Heap-reuse evicts trackers** (per `portRelocEvictFileRangesInRange`
   in `lbreloc_bridge.cpp`). When a fresh load lands in a reused address
   range, the bytes come from the new file (torch-fixed) and trackers are
   empty for those addresses. The next decomp call to `portFixupStructU16`
   sees `count(key)==0`, inserts the tracker, and (if the catalog says
   torch did it) skips the transform. All correct. Don't break this
   chain by short-circuiting before the tracker insertion.

5. **The catalog is keyed per-file, not heap-absolute.** Catalog stores
   `(file_id, byte_offset_in_file, family)`. The bridge translates
   `(base, file_id, file_base)` → `byte_offset_in_file = (uintptr_t)base
   + byte_offset - file_base` before lookup. If `portRelocFindFileIdAndBase`
   returns "not in any file", catalog miss → fall through to transform.

## Open from Stages 4-5 (housekeeping, do once when ready)

The submodule branches are local. Push when ready for review:

```
git -C torch        push origin ssb64-buildtime-reloc
git -C libultraship push origin ssb64-buildtime-reloc
git -C decomp       push origin port-patches-buildtime-reloc   # no commits yet but harmless
```

## What to do FIRST in the next session

1. Run the sanity check (top of this doc).
2. Read `port/bridge/lbreloc_byteswap.cpp:1031..1090` (the two simplest
   helpers, `portFixupStructU16` and `portFixupStructU32`) to internalize
   the "tracker key + idempotency check + transform" shape.
3. Decide capture strategy (Option A recommended). Spike a one-shot
   capture for a single helper:
   - Add a stderr log in `portFixupStructU16` keyed off
     `getenv("SSB64_FIXUP_CAPTURE")`.
   - Run `./build/BattleShip` through attract chain.
   - Aggregate, sanity-check the output shape.
4. Once the capture format is locked, generate catalog + commit, then
   proceed to family-by-family rollout.

Total Stage 6 estimate: 2–3 sessions. Catalog format + capture pipeline is
the load-bearing decision; once locked, the rest is mechanical with the
drift gate as the safety net.

## Quick command reference (Stage 6)

```
# Sanity
./build/port/test/port_reloc_regen --all --check --quiet

# After torch change
rm build/BattleShip.o2r BattleShip.o2r
cmake --build build/TorchExternal/src/TorchExternal-build --target torch -j 4
cmake --build build --target ExtractAssets -j 4
./build/port/test/port_reloc_regen --all --check --quiet      # must stay green

# Manual sweep
cd build && ./BattleShip
```
