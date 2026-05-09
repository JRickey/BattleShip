# Build-time reloc refactor — Stage 11/12 handoff (2026-05-09, Stage 10 closed)

You are picking up the build-time reloc refactor described in
`docs/refactor_buildtime_reloc_plan.md` after Stage 10 (sub-resource
emission for moddability) landed. The remaining stages are mostly
cleanup: Stage 11 trims the now-unused runtime byteswap/halfswap code
that the per-flag gates compile out, and Stage 12 productizes the
ad-hoc `mods/` scanner that Stage 10 dropped in.

**Read the plan first** — this handoff is the delta on top of it.

## Status entering this session

Branch HEAD on `agent/buildtime-reloc`: see `git log --oneline -10`.
Stages 1–10 are landed.

The bit map of `processing_flags`:

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
| 11   | `PROC_SUBRESOURCES_EMITTED`   | 10        | done                     |

Two stage-7 / stage-8 ADRs document the carve-outs — read these before
removing the runtime walkers / 4c codec under Stage 11:

- `docs/decision_aobjevent32_runtime_walker_2026-05-09.md`
- `docs/decision_4c_sprite_codec_runtime_2026-05-09.md`

## Sanity check first

```
cd .claude/worktrees/buildtime-reloc
./build/port/test/port_reloc_regen --all --check --quiet
```

Expected: `unchanged=2132, mismatch=0, …`. If it drifts, follow the
recovery steps in `docs/refactor_buildtime_reloc_plan.md` (wipe + re-extract).

## Stage 10 follow-ups (not yet addressed)

Stage 10 shipped enough machinery for end-to-end mod-override verification
(Phase C demo: byte-level overlay confirmed), but several capabilities and
guardrails are deferred. Decide which to tackle before Stage 11/12 — some
are blockers for "real" modding, others are polish.

### A — Pixel-buffer enumeration (DONE 2026-05-09)

Shipped via `EmitChainPixelBuffers` in torch's `RelocFactory.cpp`. The
helper iterates each SPRITE catalog entry, looks up the file's chain
slots at `S+0x20` (LUT), `S+0x34` (bitmap array), and per-bitmap
`B+0x08` (pixel buffer) in a `(slot_byte_off → target_byte_off)` map
built from the Stage-9 chain entries, reads `nbitmaps`/`nTLUT`/`bmsiz`
+ per-bitmap `width_img`/`actualHeight` from the post-pass1+rotate16
buffer, computes `(w × h × bpp + 7) / 8` word-aligned, and emits one
Tex sub-resource per chain-resolved pixel buffer plus one Tlut per
sprite palette.

Bitmaps reachable from a Sprite but absent from the BITMAP catalog are
filtered out — without struct fixup their `width_img` lives at a
different byte offset and the read would yield garbage. 4c (`bmsiz=4`)
is also skipped: on-disk it's a half-width compressed source, so the
runtime's `bpp=4` size formula would over-cover the buffer by 2× and a
modder's override would clobber adjacent file bytes; 4c modding falls
back to the whole-container override pathway (follow-up L).

**Result.** Sub-resource count grew from **2,593 → 4,048**:
`tex` 0 → 1,417, `tlut` 0 → 38. Drift gate stays green at
2,132 unchanged. Archive 13.1 MB → 14.8 MB. Coverage by family:
433 stages, 359 menus, 317 movies, 117 interface, 84 extern_data,
64 scene, 41 fighters_main / fighters_common / `*Model`, 10 misc_named.

**Known gap (out of scope for follow-up A).** Fighter-mesh character
textures drawn through direct DL `G_SETTIMG` with `seg≠0x0E` (cross-file
texture refs into another loaded file's bitmap region) are still not
addressable: the in-DL scanner only catches `seg=0x0E`, and these
textures aren't reachable through the Sprite/Bitmap struct path either.
A future enumeration pass that walks DL streams for `seg=0x06`/`seg=0x07`
`G_SETTIMG` and resolves the segment via the file's chain-slot table
would close this gap. For now mod authors use the whole-container
override pathway (follow-up L) to swap entire `*Model` containers.

### B — Drift gate doesn't validate sub-resources

**Problem.** `port_reloc_regen --check` only compares post-pipeline
`Data` bytes against committed `.fixt` fixtures. It does NOT validate:

- That every entry in `SubResourceHashes` resolves to a Blob the LUS
  factory can load.
- That each sub-resource's bytes equal `Data[byte_offset..+size]` (the
  identity invariant the overlay path relies on for its short-circuit).
- That `CRC64(emitted_path) == hash` recorded in the container header.
- That no two sub-resources in the entire archive share a CRC64 hash.

A future torch refactor (e.g. renaming a kind, changing the path
delimiter) could silently break the override path while keeping the
drift gate green.

**Implementation sketch.** Extend `port_reloc_regen` with a
`--check-subres` mode that, for each loaded `RelocFile`:

1. Iterates `SubResourceHashes`.
2. For each entry, calls `ArchiveManager::HasFile(hash)` then
   `LoadResource` and casts to `Ship::Blob`.
3. Asserts `memcmp(blob.Data.data(), Data + byte_offset, size) == 0`
   (modulo the 16-byte LUS Blob trailing padding).
4. Tracks all (file_id, hash) tuples globally; asserts no hash collisions.

Add to the standard CI gate alongside `--all --check --quiet`.

**Effort estimate:** ~0.5 session. Mostly harness code.

### C — Mod-collision determinism

**Problem.** The Stage 10 mods scanner uses
`std::filesystem::directory_iterator` which has filesystem-dependent
order (APFS sorts differently from ext4 from NTFS). When two mods claim
the same hash, LUS's last-archive-wins picks the most-recently-added —
so cross-platform mod authors can hit "works on my machine" surprises.

**Fix.** Sort entries alphabetically by filename before AddArchive in
`port/port.cpp::560`-ish. One-line change.

### D — Override identity-check optimization

**Problem.** `portRelocBuildOverlay` memcmps every sub-resource against
its container slice on every reloc-file load when overrides are active,
to detect "real override" vs. "base archive's own copy". For 100K+
sub-resource lookups across an attract-chain run this is measurable.

**Options.**
- Store `u64 content_hash` (CRC64 of bytes) alongside `(byte_offset,
  size, kind)` in the v3 schema. Compare hash-of-loaded-blob vs.
  recorded; skip overlay when equal. Cuts memcmp to fixed 8-byte
  comparison. Schema bump to v4.
- LUS API extension: `ArchiveManager::GetArchiveForHash(hash)` →
  `shared_ptr<Archive>`. Skip overlay when archive == base. No schema
  change but requires LUS patch.
- Status quo with sAnyOverridesActive short-circuit handles the common
  no-mod case for free; optimization only matters when ≥1 mod is loaded.

Defer until profiling shows it matters.

### E — Container schema cohabitation when V0/V1/V2 readers are dropped

**Problem.** When Stage 11 trims runtime byteswap helpers, archives
written by V0/V1/V2 (where flags would be unset and the runtime would
compensate) lose their fallback path. Two options:

- **Keep V0/V1/V2 readers** with a slow-path that re-runs byteswap
  helpers. Maintains compatibility with stale archives but doesn't let
  Stage 11 fully delete the helpers — they're kept as a fallback.
- **Drop V0/V1/V2 entirely.** Asset/torch coupling already forces
  re-extract on every torch SHA bump, so older archives are de-facto
  unsupported anyway. Simpler, smaller binary.

Recommend **drop**, gated on a torch SHA bump that emits v3 and a
smoke pass on a freshly-extracted archive. Document the transition in
the Stage 11 commit message.

### F — OTR path stability vs. yaml refactors

**Problem.** Sub-resource paths embed the yaml-defined asset name —
e.g. `reloc_fighters_main/MarioMain__sub/ftattributes/1064`. If someone
renames `MarioMain` → `MarioMainV2` in `yamls/us/reloc_fighters_main.yml`,
**all** of MarioMain's sub-resource hashes change. The container's
recorded hash list re-computes (internally consistent), but external
mods built against the old name break.

**Mitigation options.**
- Document the constraint in `docs/architecture.md` and
  `tools/example_mod/README.md`: "yaml asset names are part of the mod
  ABI; renames bump the mod compatibility version."
- Offer a stable-id alternative: torch could optionally emit at
  `reloc_subres/<file_id>/<kind>/<offset>` (file_id is part of the ROM
  layout, never changes). Trade-off: less human-readable paths.
- Or both — emit at both paths, container records both hashes. Doubles
  hash list size but lets mods choose which surface to bind to.

Document for now; revisit if a yaml refactor breaks downstream mods.

### G — `__sub` suffix collision risk

**Problem.** Path scheme assumes no yaml asset legitimately ends with
`__sub`. Audit: scan `yamls/us/*.yml` for any asset name matching
`*__sub*`. If clean, document the constraint as a torch-side check; if
not clean, swap to a more unique sentinel like `__ssb64_subres__`.

### H — Pixel-format disambiguation in tex paths

**Problem.** The handoff originally specified `tex/<offset>/<fmt>_<siz>`
because the same offset can be read at different (fmt, siz) combinations.
Today we collapse to `tex/<offset>` bucketed by Pass2Kind. If a future
texture is read as both 4b-CI and 8b-IA at the same offset, the dedup
collapses both into one path → modders can't distinguish. Audit by
checking if any current Pass2Region entries have duplicate offsets with
different fmts.

### I — Catalog drift CI gate

**Problem.** `tools/generate_struct_fixup_catalog.py` mines decomp
source for `portFixup*` call sites. If a decomp PR adds a new fixup
without re-running the generator, the catalog stays stale and torch
emits no sub-resources for that file — silent regression with no test
signal.

**Fix.** CI runs `generate_struct_fixup_catalog.py` and fails if the
output differs from the committed `port/test/struct_fixup_catalog.json`.
One-line CI gate.

### J — Catch-all observability

**Per-file census.** Many of the 2,132 files emit zero sub-resources
(1,633 animation files, mostly correct since AObjEvent is deferred). A
`tools/inspect_fixtures.py --census-subres` mode could surface
"fighter-state files with zero sub-resources" — likely indicating the
catalog missed a fixup site.

**Hash collision check.** Probability of two sub-resource paths colliding
is ~2⁻⁶⁴ across N=4,048 entries (negligible) but a one-time check costs
nothing and rules out a misformatted-path bug. Fold into B above.

### K — Mod-author guide

`tools/example_mod/README.md` documents the demo workflow but there's no
end-to-end "how to write a mod" doc. Probably a Stage 12 deliverable but
worth listing — modders need to know:

- Path scheme (`<container_path>__sub/<kind>/<dec_offset>`).
- Which kinds are addressable today (the 9 listed in the plan).
- LUS Blob format (header + u32 size + bytes).
- CRC64 path hashing matching `libultraship/src/ship/utils/StrHash64.cpp::CRC64`.
- Last-archive-wins resolution and how the scanner orders mods.

### L — Full-container override pathway (orthogonal to sub-resources)

A user during the Stage 10 closeout asked for a Mario↔Captain-Falcon
moveset/model swap. That use case wants animation streams + character
motion files swapped — content the Stage 10 enumerator does NOT emit
(see follow-up A for textures, plus the Stage 7 ADR for AObjEvent
streams). But LUS's underlying path-based last-archive-wins works
regardless of Stage 10: a mod can override `reloc_fighters_main/CaptainMain`
as a *whole container* and the runtime serves the mod's bytes verbatim
through the existing `LoadResource(path_hash)` path — no Stage 10
machinery involved.

Caveats:

- The override file must itself be a valid v3 reloc container — same
  schema, same chain entries, same `processing_flags`. The simplest
  authoring path is "copy a different existing container's bytes
  verbatim" (e.g. mod's `CaptainMain` = base archive's `MarioMain`
  bytes), but cross-references to other reloc files via `ExternFileIds`
  may resolve to the wrong file_id post-swap.
- Total size must fit in the heap allocation the game prepared. The
  bridge truncates oversized loads (`lbRelocLoadAndRelocFile`'s
  `copySize > bytes_num` clamp). Smaller-than-original is safe; larger
  silently truncates.
- Cross-container references (e.g. `CaptainMain` ↔ `CaptainMainMotion`
  via file_id) won't auto-translate. A real swap mod needs to override
  the whole linked set: main + motion + animation files + … . A modder
  can list these by walking `ExternFileIds` of each container.

This pathway is **already functional** today via Stage 10's mods/
scanner — it just isn't sub-resource-overlay; it's the original
LUS mFileToArchive resolution. Document in the mod-author guide (K)
as the right tool when sub-resources don't reach the bytes you need.

---

## Stage 11 — Runtime simplification

Per the plan: now that every transformation bit (except the deliberately
reserved bit 9) is set on every file in v3 archives, the runtime
byteswap/halfswap helpers are dead code along the hot path. Stage 11
trims them.

### What to delete

- `port/bridge/lbreloc_byteswap.cpp` — the pass1/pass2/struct/halfswap
  helpers. Keep:
  - `portDeswizzleDecodedSprite4c` (Stage 8 ADR — runtime stays).
  - The heap-absolute trackers (`sStructU16Fixups`,
    `portTextureCacheDeleteRange` callers, `portEvictStructFixupsInRange`
    etc.) which the bridge uses for cache invalidation, not byteswap.
- `port/port_aobj_fixup.{h,cpp}` — Stage 7 ADR says this STAYS. Skip.
- `port/bridge/lbreloc_bridge.cpp::portRelocByteSwapBlob` — every flag
  it consults is set, so the call becomes a no-op. Either delete the
  call or keep it as a defensive assert that all flags are set.

### Risks

- v0/v1/v2 archives will no longer load if their flags are unset and
  the runtime code that handled the unset case is gone. Two options:
  - Keep V0/V1/V2 readers but make them fall back to a slow path that
    re-runs the byteswap helpers (current behavior).
  - Drop the V0/V1/V2 factories entirely (asset/torch coupling already
    forces a re-extract on every torch SHA bump anyway).
  Option 2 is simpler and aligns with how aggressively this refactor
  is allowed to break older torch archives.

- `lbreloc_byteswap.cpp` is large (~1500 LOC) and mixes byteswap helpers
  with heap trackers + sprite-codec utilities. Don't `rm` it — split
  into a thin tracker/cache-invalidation header and delete the byteswap
  guts. The plan's commit message: "lbreloc_byteswap shrinks from ~1500
  LOC to ~150 LOC of heap-absolute trackers".

## Stage 12 — Productize mods/ scanner

Stage 10 shipped a minimal `mods/` directory scanner in
`port/port.cpp` (right after BattleShip.o2r is added). It's enough to
demo Phase C but missing two things Stage 12 should add:

1. **Discoverability UI**: list active mods in the Esc menu
   (`port/PortMenu.{h,cpp}`), expose toggles to enable/disable per-mod
   without restart. The override path can re-engage simply by toggling
   `portRelocSetOverridesActive` and re-loading affected reloc files —
   the plan proposes only a restart-required UX for v1.
2. **Sub-resource discovery**: list which sub-resources a given mod
   overrides (cross-reference mod's archive entries against
   `RelocFile::SubResourceHashes`). Useful for end-users debugging
   "which container is this mod hitting".

### The example mod

`tools/example_mod/build_example_mod.py` builds a canned override that
flips one byte of Mario's FTAttributes. Stage 12 can:
- Promote the script into the menu ("Generate Example Mod" button).
- Provide a wider set of demo mods showing different override kinds.

## Open submodule branches (push when ready for review)

```
git -C torch        push origin ssb64-buildtime-reloc   # 11 commits (Stages 4–7 + 9 + 10)
git -C libultraship push origin ssb64-buildtime-reloc   # 1 commit (Stage 1)
git -C decomp       push origin port-patches-buildtime-reloc   # 0 commits, harmless
```

The `agent/buildtime-reloc` outer branch is mergeable to `main` today
if the project wants to land Stages 1–10 standalone — Stage 10 delivers
the user-visible mod path (build the example mod, drop in `mods/`,
boot, see the override fire). Stages 11–12 are correctness/UX refinements
on top.

## Earlier-stage details

The per-stage closeout lives in commit messages on
`agent/buildtime-reloc` — `git log --oneline` for the index, full
bodies for substantive context. The two ADRs above own the Stage 7 and
Stage 8 carve-outs. The plan document
(`docs/refactor_buildtime_reloc_plan.md`) has the per-stage Status notes.
