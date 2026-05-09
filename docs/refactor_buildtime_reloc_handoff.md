# Build-time reloc refactor — Stage 10 handoff (2026-05-09, Stages 7–9 closed)

You are picking up the build-time reloc refactor described in
`docs/refactor_buildtime_reloc_plan.md` at **Stage 10 (sub-resource
emission for moddability)** — the first stage that delivers the
user-visible mod path that motivated the whole refactor.

**Read the plan first** — this handoff is the delta on top of it.

## Status entering this session

Branch HEAD on `agent/buildtime-reloc`:

```
b70585b docs: close Stage 9 — chain-flatten landed, pivot handoff to Stage 10
fd6da52 Bump torch + flatten reloc chains at extraction (Stage 9)
f4c1d26 docs: close Stage 8 — 4c sprite codec stays runtime-side (ADR)
2d948f3 docs: close Stage 7 — AObjEvent32 unhalfswap stays runtime-side (ADR)
6ac8a09 Bump torch + gate halfswap on PROC_HALFSWAP_DONE (Stage 7-HALFSWAP)
…
```

Submodule pointers:

- `decomp` → `port-patches-buildtime-reloc` (no commits this session;
  no commits across all of Stages 1–9 actually — the refactor is
  port- and torch-side).
- `libultraship` → `ssb64-buildtime-reloc` @ `5b87696f` (Stage 1
  patch only; no commits since).
- `torch` → `ssb64-buildtime-reloc` @ `c37c2e3` (Stage 9 chain-flatten
  emitter; total 10 commits on the branch — Stage 4 + Stage 5 + Stage
  6 ×6 + Stage 7 halfswap + Stage 9).

Stages 1–9 are landed. The bit map of `processing_flags`:

| bit  | name                          | stage     | status                   |
|------|-------------------------------|-----------|--------------------------|
| 0    | `PROC_PASS1_BSWAP_DONE`       | 4         | done                     |
| 1    | `PROC_PASS2_DONE`             | 5         | done                     |
| 2    | `PROC_STRUCT_U16_DONE`        | 6d        | done                     |
| 3    | `PROC_STRUCT_U32_DONE`        | 6d        | done                     |
| 4    | `PROC_SPRITE_DONE`            | 6d        | done                     |
| 5    | `PROC_BITMAP_DONE`            | 6d        | done                     |
| 6    | `PROC_MOBJSUB_DONE`           | 6d        | done                     |
| 7    | `PROC_FTATTRIBUTES_DONE`      | 6d        | done                     |
| 8    | `PROC_HALFSWAP_DONE`          | 7         | done                     |
| 9    | (reserved-unused)             | —         | AObjEvent ADR carve-out  |
| 10   | `PROC_CHAIN_FLATTENED`        | 9         | done                     |
| 11   | `PROC_SUBRESOURCES_EMITTED`   | **10**    | **this stage**           |

The two stage-7 / stage-8 ADRs document the carve-outs that close out
those stages partial-but-deliberate. Read them before second-guessing
why the runtime walker / 4c codec are still in place:

- `docs/decision_aobjevent32_runtime_walker_2026-05-09.md`
- `docs/decision_4c_sprite_codec_runtime_2026-05-09.md`

## Sanity check first

```
cd .claude/worktrees/buildtime-reloc
./build/port/test/port_reloc_regen --all --check --quiet
```

Expected: `unchanged=2132, mismatch=0, …`. If it drifts:

- Stale `BattleShip.o2r` from a different torch SHA — wipe + re-extract:
  ```
  rm build/BattleShip.o2r BattleShip.o2r
  cmake --build build/TorchExternal/src/TorchExternal-build --target torch -j 4
  cmake --build build --target ExtractAssets -j 4
  ./build/port/test/port_reloc_regen --all --check --quiet
  ```
- The `port_reloc_regen` binary is gated on `-DSSB64_BUILD_TESTS=ON`.
  If `cmake` says "no rule to make target": `cmake -B build
  -DSSB64_BUILD_TESTS=ON` then build the target.

## What Stage 10 has to deliver

Per the plan (`docs/refactor_buildtime_reloc_plan.md` Stage 10):

> For each reloc file, torch emits sub-resources alongside the container:
>
> - `reloc/<id>/vtx/<offset>` per Vtx run discovered by Pass 2
> - `reloc/<id>/tex/<offset>/<fmt>_<siz>` per texture image
> - `reloc/<id>/tlut/<offset>` per palette
> - `reloc/<id>/dl/<offset>` per top-level DL
> - `reloc/<id>/anim/<offset>` per AObjEvent32 head
> - `reloc/<id>/sprite/<offset>` per Sprite/Bitmap pair
> - `reloc/<id>/struct/<offset>/<type>` per fixed-up struct
>
> Containers store sub-resource hashes in their header so the runtime
> can ask the resource manager whether any are overridden — and if so,
> overlay the sub-resource bytes into the container blob *before*
> chain-walking.
>
> `processing_flags` bit 11 = `PROC_SUBRESOURCES_EMITTED`.

The *user-visible* outcome is that a `mods/<thing>.o2r` shipped after
`BattleShip.o2r` overrides individual sub-assets (e.g. swap one Mario
stock-sprite color) without touching the rest of the container.

Stage 12 wires the `mods/` directory scan into `Ship::Context` init.
Stage 10 just emits the sub-resources and gates the container-blob
overlay step on the flag bit. The override path itself rides on
libultraship's existing `mFileToArchive` last-archive-wins resolution
— no LUS API changes needed (that's the plan's claim; verify it
holds when Stage 12 lands).

## What already exists you can build on

### Torch sub-asset enumerators that already work

- **Vtx, texture-bytes, texture-palettes**: `Pass2Region` records
  produced by `ScanDisplayLists` in
  `torch/src/factories/ssb64/RelocFactory.cpp:98..201`. Each region
  has `(offset, size, kind ∈ {Vertex, TexBytes, TexU16})`. This is
  the foundation — Pass 2 already walks every top-level DL and
  emits these for byte-fixup purposes; Stage 10 reuses the same
  walk to emit sub-resources.
- **Top-level DLs**: implicit in the same scan — `ScanDisplayLists`
  starts from each in-DL `G_DL` target. The scan tracks DL entry
  points; sub-resource emission needs to enumerate them
  deterministically (e.g. by entry-into-walk order) so re-extraction
  produces stable hashes.
- **Sprites, Bitmaps, MObjSubs, FTAttributes, StructU16/U32**: all
  enumerated by `port/test/struct_fixup_catalog.json` (Stage 6's
  catalog). Each entry is `(file_id, byte_offset, family)`. Torch
  already reads this catalog at extract via
  `tools/generate_struct_fixup_catalog.py` →
  `StructFixupCatalogData.h` → `ApplyStructFixupsInPlace`. Stage 10
  walks the same catalog to know which struct sub-resources to emit.
- **Reloc chain entries**: Stage 9's flat list. Useful for
  validating that emitted sub-asset offsets do or don't appear as
  chain targets (informs override-overlay timing relative to chain
  iteration).

### What does NOT have an enumerator today

- **AObjEvent32 anim heads**. The runtime walker
  (`port/port_aobj_fixup.{h,cpp}`) discovers them lazily via
  `gcParseDObjAnimJoint` / `gcParseMObjMatAnimJoint`. There is no
  static enumerator. The Stage 7 ADR
  (`decision_aobjevent32_runtime_walker_2026-05-09.md`) explicitly
  says authoring tools don't round-trip through AObjEvent32 streams,
  which means anim sub-assets aren't load-bearing for the
  moddability goal anyway. **Recommendation: defer anim sub-asset
  emission** the same way Stage 7-AObjEvent32 was deferred. Either
  emit a stub set of anim resources keyed off whatever heads the
  chain walk happens to register, or skip anim sub-assets entirely
  for Stage 10 and revisit if a future modding feature actually
  needs per-animation override.

### LUS sub-resource pattern reference

`torch/src/factories/DisplayListFactory.cpp:255..318` is the canonical
shape for emitting an OTR sub-resource and splicing a hash reference
into a parent stream:

```cpp
uint64_t hash = CRC64(path.c_str());
N64Gfx value = gsSPVertexOTR(diff, nvtx, didx);
writer.Write(value.words.w0);
writer.Write(value.words.w1);
w0 = hash >> 32;
w1 = hash & 0xFFFFFFFF;
```

The hash is `CRC64(path)`; the path is constructed deterministically
from the asset's identity (file_id + offset + kind for Stage 10's
purposes). Mirror that pattern when emitting sub-resources from
`RelocBinaryExporter::Export`.

The runtime resolves the hash via `mFileToArchive`
(`libultraship/src/ship/resource/archive/ArchiveManager.cpp`); a
`mods/foo.o2r` loaded after `BattleShip.o2r` claims any matching
hashes, so `LoadResource(hash)` returns the override. This is exactly
the plan's "no API changes needed" argument — verify against
`ArchiveManager::AddArchive` and `mFileToArchive` to confirm.

## Suggested implementation shape

This is one suggested shape, not a binding plan. Think first; the
plan and prior session-7 / session-8 / session-9 closeouts are all
your context.

### Phase A — torch sub-asset producer (~1 session)

1. In `torch/src/factories/ssb64/RelocFactory.cpp`, add a sub-asset
   enumerator that consumes (a) `Pass2Region` records and (b) catalog
   entries grouped by `file_id`. Group output by `(kind, byte_offset)`.
2. Decide the path scheme. The plan says `reloc/<id>/<kind>/<offset>`
   plus `<fmt>_<siz>` for tex. The id is `mFileId`, kind is one of
   `vtx`, `tex`, `tlut`, `dl`, `sprite`, `struct`, etc., offset is
   the byte offset into the post-pass1+pass2+struct+halfswap buffer.
   Tex variants need `<fmt>_<siz>` because the same offset can be
   read at different formats (rare but real).
3. Emit each sub-resource as a standard LUS `Blob`-style binary
   resource: header + raw bytes for the region. Use the existing
   `WriteHeader(writer, Torch::ResourceType::Blob, 0)` pattern.
   Hash the path with `CRC64`.
4. Container header gains a sub-resource hash list before the data
   block (still in v2 layout, OR bump to v3 — see "Risks" below):
   ```
   u32 num_subresource_hashes
   { u64 hash, u32 byte_offset, u32 size, u8 kind }[]
   ```
5. Set `PROC_SUBRESOURCES_EMITTED` (bit 11) when the list is
   non-empty.

### Phase B — runtime overlay (~1 session)

1. `port/resource/RelocFile.h` — add the sub-resource hash list to
   the struct.
2. `port/resource/RelocFileFactory.cpp` — V3 reader (or V2 extension
   if you went that route) reads the list.
3. `port/bridge/lbreloc_bridge.cpp` — *before* the existing
   `memcpy(ram_dst, relocFile->Data.data(), copySize)` (line ~479),
   build a mutable copy of `relocFile->Data`, walk the sub-resource
   hash list, ask `Ship::Context::GetResourceManager()->LoadResource(hash)`
   for each. If a sub-resource resolves to bytes that *differ* from
   the corresponding region in the current container, overlay the
   override bytes at `byte_offset` (size bytes) into the mutable
   copy. Then memcpy that copy.
4. The chain iterator (Stage 9 flat list) walks over the overlaid
   bytes — pointer registration still works because the chain slot
   offsets and target offsets are static and unchanged by overrides.

### Phase C — canned override test (~0.5 session)

The drift gate covers byte-identicality of the container itself —
*not* the override path. Build one canned mod:
- Take a Mario stock-sprite tex sub-resource.
- Hand-roll an alternate `.o2r` containing only that sub-resource
  with one color swapped.
- Drop in `build/mods/example.o2r`.
- Boot, navigate to the screen rendering the sprite, confirm the
  color swap.

This proves the overlay path end-to-end. Stage 12 will productize
the `mods/` directory scan; Phase C just exercises it via a manual
load.

## Risks specific to Stage 10

1. **Hash stability across rebuilds.** Sub-resource paths must be a
   pure function of file identity. Anything depending on
   non-deterministic enumeration order (hash-map iteration, parallel
   walk) will break override mods on the next torch SHA. Verify by
   running the extractor twice and diffing the emitted hash list per
   container.
2. **Container schema vs sub-resource schema cohabitation.** v2
   (chain-flatten) and v3 (sub-resources) want both sets of fields.
   Cleanest: bump container to v3 with sub-resource hashes appended
   after the chain-flatten sidecar. v2 fall-through means runtime
   pre-Stage-10 fixtures keep loading. Worst path: try to retrofit
   Stage 10 fields into v2 conditionally — error-prone.
3. **Overlay-vs-chain-iterator timing.** The Stage 9 chain iterator
   runs against `ram_dst` after `memcpy`. Override overlays MUST
   happen between `memcpy` and the chain walk; otherwise the chain
   slots / fixup offsets in `ram_dst` reflect the original bytes
   while the data they point at has been overlaid. Both edges of the
   sandwich (slot encoding in chain entries, target bytes at chain
   targets) are at static offsets, so timing the overlay in between
   is fine — but document the constraint with a clear comment.
4. **Texture-cache eviction**. `portTextureCacheDeleteRange` in
   `lbreloc_bridge.cpp` runs against `ram_dst`. If override bytes
   are at a different physical heap address than the original (they
   shouldn't be — same `ram_dst` slot — but verify), Fast3D's
   texture cache could serve stale uploads. Test the override path
   on a texture that's *already cached* before the override fires
   (e.g. menu sprite that pre-loads).
5. **Empty-override case must be a no-op.** `mods/` empty or
   override hash mismatches every container = exact same bytes.
   Drift gate stays green by construction. Worth a sanity assert in
   the overlay loop ("if no overlay applied, skip the mutable-copy
   allocation entirely").
6. **Sub-resource emission cost.** ~2,132 files × ~10–100
   sub-resources each = potentially hundreds of thousands of new
   resources in `BattleShip.o2r`. Watch the archive size and the
   asset-extract wall time. If it gets unwieldy, gate emission on
   a per-file-kind allowlist (e.g. emit struct/sprite/tex but skip
   per-Vtx-run). The plan has the maximalist list; trim if needed.

## File map (planned for Stage 10)

```
torch/src/factories/ssb64/RelocFactory.cpp     MODIFY: emit sub-resources
torch/src/factories/ssb64/RelocFactory.h       MODIFY: surface hash-list type
port/resource/RelocFile.h                      MODIFY: SubResourceHashes vector +
                                                       PROC_SUBRESOURCES_EMITTED
port/resource/RelocFileFactory.{h,cpp}         MODIFY: V3 reader (or extend V2)
port/port.cpp                                  MODIFY: register V3 factory
port/bridge/lbreloc_bridge.cpp                 MODIFY: overlay step before memcpy
port/test/reloc_harness.cpp                    MODIFY: register V3 factory

# Phase C (override test)
tools/example_mod/                             NEW:    canned mod showing the
                                                       override pathway
```

## Workflow reminders

Mirror the Stage 9 commit pattern:

1. Edit + commit in `torch/`. Push to `ssb64-buildtime-reloc` only at
   review time.
2. Build + extract:
   ```
   rm build/BattleShip.o2r BattleShip.o2r
   cmake --build build/TorchExternal/src/TorchExternal-build --target torch -j 4
   cmake --build build --target ExtractAssets -j 4
   ```
3. Drift gate:
   ```
   cmake --build build --target port_reloc_regen -j 4
   ./build/port/test/port_reloc_regen --all --check --quiet
   ```
4. Outer commit: `git add torch port/... && git commit -m "Bump
   torch + emit reloc sub-resources (Stage 10 Phase A/B/C)"`.

Build parallelism cap: **`-j 4`** on the M1 16 GB machine
(`MEMORY.md` → repo CLAUDE.md).

## Open submodule branches (push when ready for review)

```
git -C torch        push origin ssb64-buildtime-reloc   # 10 commits (Stages 4–7 + 9)
git -C libultraship push origin ssb64-buildtime-reloc   # 1 commit (Stage 1)
git -C decomp       push origin port-patches-buildtime-reloc   # 0 commits, harmless
```

The `agent/buildtime-reloc` outer branch is mergeable to main today
if the project wants to land Stages 1–9 standalone (the user-visible
mod path lands at Stage 10–12; the build-time refactor's correctness
benefits land throughout 4–9).

## Earlier-stage details

The per-stage closeout lives in commit messages on
`agent/buildtime-reloc` — `git log --oneline` for the index, full
bodies for substantive context. Stage 6 in particular shipped as 6
per-family commits (`Bump torch + regen fixtures: <FAMILY> migrated
to torch (Stage 6d-<FAMILY>)`) plus `port/test/struct_fixup_catalog.json`
(2,592 tuples, 125 files). The two ADRs above own the Stage 7 and
Stage 8 carve-outs.
