# Build-time reloc refactor — Stage 10 follow-up backlog

Tracker for the post-Stage-10 follow-ups. Pulled out of
`docs/refactor_buildtime_reloc_handoff.md` so the handoff can focus on
the next active stage (Stage 12 — productize the mods/ scanner).

A, B, C, E are already shipped on `agent/buildtime-reloc`; their commit
messages are the canonical write-up. D, F-L remain — pick what fits a
session, or pivot to Stage 12.

| ID | Topic                                       | Status                  | Effort      |
|----|---------------------------------------------|-------------------------|-------------|
| A  | Chain-resolved pixel-buffer enumeration     | DONE — commit `0ad444f` | —           |
| B  | Drift-gate sub-resource validator           | DONE — commit `0b81d3e` | —           |
| C  | Mods-scanner determinism                    | DONE — commit `77afbc5` | —           |
| D  | Override identity-check optimization        | Defer until profiled    | ~1 session  |
| E  | V0/V1/V2 reader drop coordination           | DONE — commit `ba3098d` | —           |
| F  | OTR path stability vs. yaml refactors       | Doc-only or schema bump | 10 min – 1 session |
| G  | `__sub` suffix collision audit              | Audit + doc / rename    | ~10 min     |
| H  | Pixel-format disambiguation in tex paths    | Audit first             | ~30 min     |
| I  | Catalog-drift CI gate                       | One-line CI gate        | ~15 min     |
| J  | Per-file census + collision observability   | Tooling                 | ~0.5 session |
| K  | Mod-author guide                            | Stage-12 deliverable    | ~0.5 session |
| L  | Full-container override pathway docs        | Doc-only                | ~10 min     |

---

## D — Override identity-check optimization

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
- Status quo with `sAnyOverridesActive` short-circuit handles the common
  no-mod case for free; optimization only matters when ≥1 mod is loaded.

Defer until profiling shows it matters.

---

## E — Container schema cohabitation when V0/V1/V2 readers are dropped

**DONE — commit `ba3098d` (Stage 11 Phase 5).** V0/V1/V2 readers have
been removed: their `RegisterResourceFactory` calls in `port.cpp` and
`reloc_harness.cpp` are gone, and the matching factory classes in
`port/resource/RelocFileFactory.{h,cpp}` are deleted. Only the V3 reader
is registered. Asset/torch coupling already forced re-extract on every
torch SHA bump, so the path was unused in practice; deleting it lets
Stage 11 fully drop the byteswap helpers V0/V1/V2 relied on without a
slow-path fallback.

---

## F — OTR path stability vs. yaml refactors

**Problem.** Sub-resource paths embed the yaml-defined asset name —
e.g. `reloc_fighters_main/MarioMain__sub/ftattributes/1064`. If someone
renames `MarioMain` → `MarioMainV2` in `yamls/us/reloc_fighters_main.yml`,
**all** of MarioMain's sub-resource hashes change. The container's
recorded hash list re-computes (internally consistent), but external
mods built against the old name break.

**Mitigation options.**
1. **Doc-only** — note the constraint in `docs/architecture.md` and
   `tools/example_mod/README.md`: "yaml asset names are part of the mod
   ABI; renames bump the mod compatibility version."
2. **Stable-id alternative** — torch could optionally emit at
   `reloc_subres/<file_id>/<kind>/<offset>` (file_id is part of the ROM
   layout, never changes). Trade-off: less human-readable paths.
3. **Both** — emit at both paths, container records both hashes. Doubles
   hash list size but lets mods choose which surface to bind to.

(1) is ~10 min; (2) and (3) require torch + container schema changes.
Recommend (1) until a yaml refactor breaks something.

---

## G — `__sub` suffix collision risk

**Problem.** Path scheme assumes no yaml asset legitimately ends with
`__sub`. Audit: scan `yamls/us/*.yml` for any asset name matching
`*__sub*`. If clean, document the constraint as a torch-side check; if
not clean, swap to a more unique sentinel like `__ssb64_subres__`.

Quick check: `grep -r '__sub' yamls/us/*.yml`. ~10 min total.

---

## H — Pixel-format disambiguation in tex paths

**Problem.** The original handoff specified `tex/<offset>/<fmt>_<siz>`
because the same offset can be read at different `(fmt, siz)`
combinations. Today we collapse to `tex/<offset>` bucketed by
`Pass2Kind`. If a future texture is read as both 4b-CI and 8b-IA at
the same offset, the dedup collapses both into one path → modders
can't distinguish.

**Audit step.** Instrument `ScanDisplayLists` (or a one-shot torch
log run) to record any `Pass2Region` entries with the same offset but
different `Pass2Kind` / siz. If none exist, document the constraint.
If hits exist, expand the path scheme to `tex/<offset>/<fmt>_<siz>`
and update the dedup key.

~30 min for audit. If hits exist, path-scheme expansion is a torch +
runtime overlay touch (~1 session).

---

## I — Catalog drift CI gate

**Problem.** `tools/generate_struct_fixup_catalog.py` mines decomp
source for `portFixup*` call sites. If a decomp PR adds a new fixup
without re-running the generator, the catalog stays stale and torch
emits no sub-resources for that file — silent regression with no test
signal.

**Fix.** CI runs `generate_struct_fixup_catalog.py` and fails if the
output differs from the committed `port/test/struct_fixup_catalog.json`.
One-line `add_test` in CMake. ~15 min.

---

## J — Catch-all observability

**Per-file census.** Many of the 2,132 files emit zero sub-resources
(1,633 animation files, mostly correct since AObjEvent is deferred). A
`tools/inspect_fixtures.py --census-subres` mode could surface
"fighter-state files with zero sub-resources" — likely indicating the
catalog missed a fixup site.

**Hash collision sweep.** Already folded into B's `--check-subres`
mode (`unique_hashes` vs. validated count is a one-line collision
check). Leaving this here for traceability only.

---

## K — Mod-author guide

`tools/example_mod/README.md` documents the demo workflow but there's no
end-to-end "how to write a mod" doc. Probably a Stage 12 deliverable but
worth listing — modders need to know:

- Path scheme (`<container_path>__sub/<kind>/<dec_offset>`).
- Which kinds are addressable today (the 9 listed in the plan, plus
  chain-resolved Tex / Tlut from follow-up A).
- LUS Blob format (header + u32 size + bytes).
- CRC64 path hashing matching `libultraship/src/ship/utils/StrHash64.cpp::CRC64`.
- Last-archive-wins resolution and how the scanner orders mods (after
  follow-up C: alphabetical by full path).

---

## L — Full-container override pathway (orthogonal to sub-resources)

A user during the Stage 10 closeout asked for a Mario↔Captain-Falcon
moveset/model swap. That use case wants animation streams + character
motion files swapped — content the Stage 10 enumerator does NOT emit
(see follow-up A for textures, plus the Stage 7 ADR for AObjEvent
streams). But LUS's underlying path-based last-archive-wins works
regardless of Stage 10: a mod can override
`reloc_fighters_main/CaptainMain` as a *whole container* and the
runtime serves the mod's bytes verbatim through the existing
`LoadResource(path_hash)` path — no Stage 10 machinery involved.

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

This pathway is **already functional** today via Stage 10's `mods/`
scanner — it just isn't sub-resource-overlay; it's the original
LUS `mFileToArchive` resolution. Document in the mod-author guide (K)
as the right tool when sub-resources don't reach the bytes you need.
