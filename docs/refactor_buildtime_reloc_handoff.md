# Build-time reloc refactor — Stage 9 handoff (2026-05-09, Stages 7 + 8 closed)

You are picking up the build-time reloc refactor described in
`docs/refactor_buildtime_reloc_plan.md` at **Stage 9 (chain-flatten —
the first material code change in the back half)**. Stages 7 and 8 are
closed:

- Stage 7 — halfswap shipped, AObjEvent32 unhalfswap explicitly skipped
  (ADR `docs/decision_aobjevent32_runtime_walker_2026-05-09.md`).
- Stage 8 — 4c sprite codec + post-decode deswizzle stay runtime-side
  (ADR `docs/decision_4c_sprite_codec_runtime_2026-05-09.md`). No code
  change, no `processing_flags` bit allocated, no fixture regen.

Stages 1–6 are fully landed.
**Read the plan first** — this handoff is the delta.

## Status (post-2026-05-09 session — Stages 7 + 8 closed)

Branch HEAD on `agent/buildtime-reloc`:

```
<HEAD>  docs: close Stage 8 — 4c sprite codec stays runtime-side (ADR)
2d948f3 docs: close Stage 7 — AObjEvent32 unhalfswap stays runtime-side (ADR)
6ac8a09 Bump torch + gate halfswap on PROC_HALFSWAP_DONE (Stage 7-HALFSWAP)
05c1b9d docs: handoff update — Stage 6 done, Stage 7 kickoff notes
dad47d6 Bump torch + regen fixtures: FTATTRIBUTES family migrated, Stage 6 complete
…
```

### Stage 7 closeout summary (carried forward)

Stage 7-Halfswap shipped clean (`6ac8a09` outer, torch `bfa2034` on
`ssb64-buildtime-reloc`):

- **Drift gate stayed green at 2132/2132 with NO fixture regen** —
  unlike Stage 6's lazy fixups, halfswap runs inside
  `lbRelocLoadAndRelocFile` so its post-transform bytes were already
  captured in load-time fixtures. Same shape as Stage 4 (Pass 1
  BSWAP32).
- Torch detects `reloc_animations/FT*` / `reloc_submotions/FT*` /
  `reloc_scene/SCExplainMain` from the OTR path (== `entryName` at
  the binary-exporter call site), walks both intern + extern reloc
  chains in the post-pass1+pass2+struct buffer to build the chain-slot
  mask, applies `rotate16` to every non-chain u32 word, sets bit 8.
- Runtime gate: `lbreloc_bridge.cpp` skips
  `portRelocFixupFighterFigatree` when `PROC_HALFSWAP_DONE` is set;
  `port_aobj_register_halfswapped_range` still runs every load (the
  lazy AObjEvent32 walker + ftkey FTKeyEvent reader consume that
  registry — heap-absolute side-effect, can't migrate to build time).

Stage 7-AObjEvent32 is **deliberately skipped**, not deferred. The
runtime walker in `port/port_aobj_fixup.{h,cpp}` stays. `processing_flags`
bit 9 is reserved-unused — bit 10 keeps its original assignment as
`PROC_CHAIN_FLATTENED` (Stage 9). Full reasoning in
`docs/decision_aobjevent32_runtime_walker_2026-05-09.md`.

### Stage 8 closeout summary (this session)

Stage 8 — the 4c sprite decompression decision — closed as a
documentation-only stage. **No code change, no submodule bump, no
`processing_flags` bit, no fixture regen.** Decision: keep
`lbCommonDecodeSpriteBitmapsSiz4b` (decomp) and
`portDeswizzleDecodedSprite4c` (port bridge) on the runtime side
permanently. Full ADR: `docs/decision_4c_sprite_codec_runtime_2026-05-09.md`.

The plan's non-goals already listed "Decompressing 4c sprites at build
time" as out of scope; this ADR formalizes the rationale. Triggers that
would re-open the decision are spelled out in the ADR's "What would
change this decision" section: 4c-aware authoring tools, demonstrated
hot-path cost, or a 4c-specific bug class hard to reproduce in fixture
testing.

The plan and Stage 11 (runtime simplification) wording have been updated
to reflect that `lbreloc_byteswap.cpp` retains a documented residual
(~150 LOC of heap-absolute trackers + 4c deswizzle + chain registration)
post-refactor rather than shrinking to a "thin wrapper" as the plan
originally claimed.

## What to do next — Stage 9 (chain-flatten)

Per `docs/refactor_buildtime_reloc_plan.md` Stage 9. This is the first
material code change in the back half of the refactor. The shape:

1. Torch reads `relocInternOffset` and `relocExternOffset` from each
   reloc table entry, walks the bit-packed chain to the end, and emits
   a flat sidecar list:
   `(slot_byte_offset, target_byte_offset, is_external, target_file_id)[]`.
2. Runtime drops the chain-walking code in `lbreloc_bridge.cpp`
   (`relocInternRel` + `relocExternRel` parser) and just iterates the
   flat list, calling `PORT_REGISTER` (or equivalent) per entry.
3. Allocate `processing_flags` bit 10 = `PROC_CHAIN_FLATTENED`. v0/v1
   archives without the bit fall through to the existing chain walker.

Drift-gate behaviour expected: chain-target lists captured in Stage 2
fixtures should be byte-identical regardless of whether they came from
the encoded chain or the flattened sidecar — the per-file unit tests
verify this directly. **Expect drift gate to stay green with no
fixture regen.** Same shape as Stages 4 + 7-Halfswap (transformations
that already ran inside `lbRelocLoadAndRelocFile` at fixture-capture
time).

After Stage 9: Stage 10 (sub-resource emission for moddability) is the
focal point — the first stage that actually delivers the moddability
goal that motivated the whole refactor.

## Open submodule branches (push when ready for review)

```
git -C torch        push origin ssb64-buildtime-reloc   # 9 commits (8 from Stage 4-6 + halfswap)
git -C libultraship push origin ssb64-buildtime-reloc   # no commits, harmless
git -C decomp       push origin port-patches-buildtime-reloc   # no commits, harmless
```

The `agent/buildtime-reloc` outer branch is mergeable to main if the
project wants to land Stages 1–7 standalone.

---

## Original Stage 6 status (archive)

Six per-family migrations on `agent/buildtime-reloc`:

```
dad47d6 Bump torch + regen fixtures: FTATTRIBUTES family migrated, Stage 6 complete
3e430e7 Bump torch + regen fixtures: STRUCT_U32 family migrated to torch
4cebb2d Bump torch + regen fixtures: STRUCT_U16 family migrated to torch
b2d5424 Bump torch + regen fixtures: MOBJSUB family migrated to torch
a2c2b80 Bump torch + regen fixtures: BITMAP family migrated to torch
68c9053 Bump torch + regen fixtures: SPRITE family migrated to torch
8427b2a port/test: populate struct fixup catalog (Stage 6b)
9383a7d docs: handoff update for end of Stage 6a/6c session
75aa0e2 port/resource: StructFixupCatalog runtime + flag bits (Stage 6c)
fe878d1 port/bridge: SSB64_FIXUP_CAPTURE catalog logger (Stage 6a)
```

Catalog: 2,592 tuples, 125 files. Per-family torch SHAs in the commit
log (`Bump torch + regen fixtures: <FAMILY> ...`). Drift gate green at
every commit. Visual sweep verified across attract → 1P → training →
VS (incl. Mushroom Kingdom) → Bonus 1 → fighter intros.

The plan's Stage 6 architecture is exactly what shipped: torch reads
the catalog at extraction (`StructFixupCatalogData.h` codegen,
gitignored in torch), applies per-family transforms to the post-
pass1+pass2 buffer, and sets `PROC_<FAMILY>_DONE` bits on
`processing_flags`. Runtime helpers consult catalog+flag and skip the
byte transform when both fire, while still inserting the heap-absolute
idempotency tracker entry. Catalog miss → runtime fallback (fail-safe).

The original Stage 6 status is preserved below for archive purposes.

---

## Stage 6 archive (original handoff content below)

Stage 6a + 6c done. Stage 6b + 6d open. Branch HEAD:

```
<current> port/resource: StructFixupCatalog runtime + flag bits (Stage 6c)
fe878d1   port/bridge: SSB64_FIXUP_CAPTURE catalog logger (Stage 6a)
6704fba   docs: handoff for Stage 6 (struct fixups → torch)   (← this doc)
5d43e46   Bump torch + complete pass2 buildtime migration (Stage 5)
```

**Stage 6a — capture wiring (committed):** every in-scope `portFixup*` helper
emits `(family, file_id, byte_offset_in_file, extra)` once per unique tuple
when `SSB64_FIXUP_CAPTURE=1`. On clean exit (atexit) the dedup'd set is
written as a JSON array to `$SSB64_FIXUP_CAPTURE_FILE` (default
`port_fixup_capture.json`). Spike: 40 sec attract chain produces 850 unique
tuples across 57 files, exercising all six families.

**Stage 6c — runtime catalog + flag bits (committed):**
- `RelocFile.h`: `PROC_STRUCT_U16_DONE` … `PROC_FTATTRIBUTES_DONE` on bits 2..7.
- `port/resource/StructFixupCatalog.{h,cpp}`: codegen target.
  `portStructFixupCatalogHasEntry(file_id, byte_offset, family) → bool` does
  binary search over a sorted u64-packed key array.
- `tools/generate_struct_fixup_catalog.py`: reads
  `port/test/struct_fixup_catalog.json`, emits the .cpp. Wired into the
  existing `GenerateRelocArtifacts` custom target.
- `port/bridge/lbreloc_bridge.cpp`: `PortRelocFileRange` now stores
  `proc_flags`. New helper `portRelocGetProcessingFlags(file_id)`.
- `port/bridge/lbreloc_byteswap.cpp`: each in-scope helper now consults
  `fixup_torch_already_did_it(family, PROC_<FAMILY>_DONE, target)` AFTER its
  tracker insertion. Skip-condition: catalog hit AND flag bit set on the file.
  Either gate missing → run the byte transform (fail-safe).
- `port/test/struct_fixup_catalog.json`: ships **empty**. Filling it is Stage 6b.

Drift gate stays green at every commit. Empty catalog + no torch-side flag
emission = every helper still runs its byte transform exactly as today.

## What to do next

### Stage 6b — populate the catalog (~30-60 min interactive)

Run the game with capture enabled across the scenes that exercise every
family:

```
cd .claude/worktrees/buildtime-reloc/build
SSB64_FIXUP_CAPTURE=1 \
SSB64_FIXUP_CAPTURE_FILE=$PWD/../port/test/struct_fixup_catalog.json \
./BattleShip
```

Coverage list (in order, one boot is fine — capture state accumulates across
scenes during a session):

1. Let attract chain run for ~1 min (covers title + demo + champ-tree screens)
2. 1P mode → Mario → stage 1 (Yoshi). Beat or quit out.
3. Training mode → pick a fighter not yet covered (e.g. Captain Falcon,
   Ness, Pikachu, Jigglypuff)
4. VS mode → 4-player free-for-all on a stage not in 1P
5. Results screen (let it auto-advance or A through it)
6. Bonus 1 (Break the Targets) → at least one fighter
7. Each fighter intro: pick the character on CSS, hold A briefly to see the
   "READY" portrait+sprite — covers the per-fighter sprite/bitmap files

Capture is per-process: when `BattleShip` exits cleanly (window-close, not
SIGKILL) the JSON dumps. Multiple runs append-and-dedup cleanly if you
concatenate the JSON arrays manually, or just do one long session.

After capture: regen the codegen + drift gate to confirm clean state:

```
python3 tools/generate_struct_fixup_catalog.py
cmake --build build --target ssb64 port_reloc_regen -j 4
./build/port/test/port_reloc_regen --all --check --quiet   # must stay green
```

Commit the JSON + regenerated .cpp together. Drift gate stays green because
no torch flag bits flip yet — the catalog content is dormant runtime data.

### Stage 6d — family-by-family torch migration (rest of Stage 6)

For each family in this order (smallest blast radius first):

1. **SPRITE** (visual; ~38 attract entries, more after Stage 6b)
2. **BITMAP** (visual; goes with SPRITE)
3. **MOBJSUB** (matanim color CCs — Whispy class regression risk)
4. **STRUCT_U16** (varied; includes MPGroundData stage collision)
5. **STRUCT_U32** (BSWAP32 family; also covers RawTextureBSWAP32)
6. **FTATTRIBUTES** (fighter physics; smallest count but most damage potential)

Per family:

1. Modify `torch/src/factories/ssb64/RelocFactory.cpp::RelocBinaryExporter::Export`:
   - Read the catalog (compile-time-embedded header is cleaner than a
     runtime JSON load; codegen a torch-side `StructFixupCatalogEntries.h`
     from the same JSON). Group entries by `(file_id, family)`.
   - For each entry whose family matches the migration target, apply the
     family's byte transform to `data` at `byte_offset` (the bytes are
     already post-pass1+pass2 at this point — see `data` after the existing
     `ApplyPass1BswapInPlace` + `ApplyPass2InPlace` calls).
   - For each `file_id` that had at least one applied entry, set
     `PROC_<FAMILY>_DONE` in `processingFlags`.
2. Commit in `torch/`. Push to `ssb64-buildtime-reloc`.
3. In the outer worktree: `git add torch && git commit -m "Bump torch: …"`.
4. Wipe + re-extract: `rm build/BattleShip.o2r BattleShip.o2r &&
   cmake --build build/TorchExternal/src/TorchExternal-build --target torch -j 4 &&
   cmake --build build --target ExtractAssets -j 4`.
5. Run drift gate. **Expect mismatches** on the catalog-touched files for
   this family — torch's pre-applied bytes are different from the previous
   "no fixup at load time" bytes. The runtime helper sees the flag set, the
   catalog hit, and skips the transform — bytes match because torch wrote
   them, but the *fixture on disk* still has the pre-migration bytes.
6. Regen the affected fixtures: `./build/port/test/port_reloc_regen --check
   --regen-mismatches --quiet` (or `--all --write` if you want to nuke and
   rewrite). **Inspect the diff** before committing — the byte changes should
   only be in the SPRITE word ranges (etc. for the family you're migrating).
   Anything outside that pattern is a bug, not a migration artifact.
7. Manual sweep: attract chain → 1P stage 1 → training → VS → BTT → each
   fighter intro. The handoff plan calls this out explicitly because struct
   fixups touch gameplay-critical state.
8. Commit the regenerated fixtures alongside the torch SHA bump (single
   commit per family, easy to revert if a regression surfaces later).

Roll forward family-by-family. Do not bundle multiple families per commit —
the per-family bisect granularity is the load-bearing safety net for the
gameplay-regression class of bugs.

### Open from this session (housekeeping)

- The two submodule branches `libultraship → ssb64-buildtime-reloc` and
  `torch → ssb64-buildtime-reloc` are still local-only. Push when ready
  for review (no commits in this session — all changes were port-side).

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
