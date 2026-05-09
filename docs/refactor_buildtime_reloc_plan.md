# Build-time reloc-data refactor — multi-stage plan

## Context

The SSB64 PC port currently performs every endian / layout fix-up on relocatable
data **at runtime**, after a file is loaded from `BattleShip.o2r`. The runtime
pipeline (in `port/bridge/lbreloc_bridge.cpp` + `lbreloc_byteswap.cpp` +
`port_aobj_fixup.cpp`) executes 21 distinct transformations: a blanket BSWAP32
sweep, GBI-aware vertex/texture fixups, per-struct U16/U32 fixups, sprite/bitmap
deswizzles, fighter figatree halfswap, lazy AObjEvent32 unhalfswap, internal +
external reloc-chain walks that register pointers in a token table, and a heap-
reuse cache-eviction layer. This pipeline is correct and has been hardened
against ~30 documented bugs (`docs/bugs/*.md`), but it has two costs:

1. **Modders can't touch sub-assets.** Torch emits each reloc "file" as a single
   opaque blob. A fighter's animations, vertices, textures, and attributes are
   bundled. Tools like `fast64` (Blender plugin for N64 DL/Vtx/Tex authoring)
   need each sub-asset addressable on its own.
2. **The runtime is doing build-time work.** Pure deterministic byte
   transformations run every load, in a context where they're harder to test
   than a single-input/single-output build script.

The **goal** is to migrate the deterministic byte transformations into Torch's
extraction step, emit a per-sub-asset sub-index, and let users override
individual sub-assets with a side-loaded `.o2r`. Out of scope: mod manager UI,
anything beyond the loader's last-archive-wins resolution.

### Why this attempt is different

Two prior attempts failed:

- `agent/torch-reloc-walker` (2026-05-06, abandoned) — moved the chain walker
  into Torch + emitted OTR sub-resources. **Broken at every commit on the
  branch.** Failure mode: changed extraction format + halfswap timing + runtime
  resource lookup simultaneously. There was no per-file regression signal —
  failures only surfaced when playing a stage and noticing a corrupt vertex.
- `agent/relocdata-from-source` (2026-05-04, abandoned) — built a parallel
  "compile decomp .c source through clang" pipeline. **Broken at the first
  commit.** Per the user, "completely unsalvageable beyond starting over."

Common cause: **no per-file pass/fail signal**. A bug introduced for one of 2,132
files surfaces only as a graphical glitch in some scene long after the branch
has 30 commits on it. By the time you notice, you can't bisect.

This plan inverts the order: **establish a per-file pass/fail unit-test gate
*before* writing a single line of refactor code.** The runtime — which is
correct today — becomes the spec. Every transformation it applies to every
file becomes a frozen golden fixture. Then the migration proceeds one
transformation at a time, with all 2,132 tests staying green at every commit.
The moment one goes red, we have a per-file repro and a one-bisect commit to
inspect.

---

## Goals

1. **Per-file ground-truth fixtures** for all 2,132 reloc files, captured from
   the current runtime, byte-identical and replayable in CI.
2. **Per-file unit tests** (Google Test, in-tree, fast — full sweep < 60s) that
   each verify the runtime pipeline still produces the captured fixture.
3. **Migrate the byte-transformation half of the pipeline** (Pass 1, Pass 2,
   struct fixups, halfswap, deswizzle) to Torch. Tests stay green.
4. **Pre-compute reloc-chain offsets in Torch**; runtime's job shrinks to
   "register pointers + walk the pre-computed offset list".
5. **Emit per-sub-asset sub-resources** in `BattleShip.o2r` with stable hashed
   paths, alongside the container blobs. Container blobs remain primary loaders.
6. **Sub-asset override path**: a side-loaded `mods/foo.o2r` whose sub-resources
   override the matching paths from `BattleShip.o2r`. Uses libultraship's
   existing last-archive-wins behavior; no API changes needed.

### Non-goals

- Mod manager UI, mod discovery, mod metadata.
- Replacing reloc files as a *unit* of game data — they remain primary loaders.
- Decompressing 4c sprites at build time (decompression is on the runtime side
  of a sprite-bank flag and can stay there; only the post-decode deswizzle
  matters for the test gate).
- Source-compile path (the dead `agent/relocdata-from-source` direction). Not
  revisited.

---

## Architecture overview after refactor

```
Build time (Torch):
  baserom.us.z64
    └─> RelocFactory parses table entry (ID, offsets, sizes)
    └─> Decompress VPK0 if needed
    └─> Apply Pass 1: BSWAP32 every u32 word
    └─> Apply Pass 2: walk DL stream, fix each Vtx / Tex / Tlut by GBI op
    └─> Apply struct fixups via per-file struct-layout metadata table
        (Sprite/Bitmap/MObjSub/FTAttributes/SYInterpDesc/MPGroundData/...)
    └─> Apply halfswap to fighter figatree files; pre-unhalfswap AObjEvent32
        streams (since stream entry points are statically reachable via
        animation-table indexing)
    └─> Pre-compute chain target list: emit (slot_offset, target_offset,
        is_external, target_file_id) records as a sidecar
    └─> Emit:
          - Container resource at "reloc/<id>" — post-fixup bytes + sidecar
          - Sub-asset resources at "reloc/<id>/<kind>/<offset>"
              · "vtx/<offset>" for each Vtx run
              · "tex/<offset>" for each texture image
              · "tlut/<offset>" for each palette
              · "dl/<offset>" for each top-level DL
              · "anim/<offset>" for each AObjEvent32 head
              · "sprite/<offset>" for each Sprite/Bitmap pair
        All addressed by CRC64(path). Container references sub-assets by hash.
        Path scheme picks deterministic offsets from the file so re-extracting
        produces identical hashes.

Runtime (port/bridge):
  Game calls lbRelocGetFileData(fileId)
    └─> ResourceManager loads container "reloc/<id>"
    └─> If sub-resource override exists for any sub-asset of <id>:
          patch the container's bytes by overlaying the sub-resource bytes
          at the appropriate offset before chain walk
    └─> Walk pre-computed chain list, register each target as a token
    └─> Return container pointer
  No byteswap, no struct-fixup, no halfswap on the runtime side — those
  helpers exist as no-ops compiled out under PORT_BUILDTIME_RELOC_FIXUP.
```

Container-preserving design retains the existing chain-walk semantics (which
are correct today), keeps `PORT_RESOLVE`/`PORT_REGISTER` token API stable, and
lets us migrate one transformation at a time without invalidating the test
fixtures.

---

## Stages

### Stage 0 — Setup (one session, ~30min)

- Create worktree at `.claude/worktrees/buildtime-reloc` on a fresh branch
  `agent/buildtime-reloc` off **current main HEAD (`314dc95`)**, not off any
  prior dead branch. Submodules pinned to:
    - `decomp`  → `73d6c0d` (currently `port-patches`)
    - `libultraship` → `cc84306` (currently `ssb64`)
    - `torch`   → `2d0e510` (currently `ssb64`)
- Create per-submodule refactor branches (so submodule changes can ride
  alongside the outer branch without polluting main fork branches):
    - `decomp`        → branch `port-patches-buildtime-reloc` off `port-patches`
    - `libultraship`  → branch `ssb64-buildtime-reloc` off `ssb64`
    - `torch`         → branch `ssb64-buildtime-reloc` off `ssb64`
  Push each to its respective fork (`origin`).
- Confirm clean Debug build inside the worktree (`-j 4`, expect ~10 min).
  Run `ssb64` once headless against attract chain to confirm baseline-green
  before any planning code lands. **Do not skip this** — this is the
  "is the runtime green?" gate that the prior attempts skipped.
- Save this plan as `docs/refactor_buildtime_reloc_plan.md` so future
  sessions can pick it up.

**Verification**: `cmake --build <wt>/build --target ssb64 -j 4` clean;
`ssb64` plays attract chain to first stage select with no log spam.

---

### Stage 1 — Test framework + harness (one session, ~3h)

Goal: scaffolding to load one reloc file, run the runtime pipeline against it
in a deterministic environment, and snapshot the result for comparison.

- **Test target**: add `port/test/CMakeLists.txt` with a `port_reloc_tests`
  GTest executable. Reuse the GTest dependency already wired in
  `libultraship/tests/CMakeLists.txt` — same approach (`FetchContent` of
  `googletest@v1.16.0`, `gtest_discover_tests`).
- **Harness library** (`port/test/reloc_harness.{h,cpp}`):
    - Loads `BattleShip.o2r` once via the LUS `ArchiveManager` (test fixtures
      reuse one global instance to avoid 2,132 archive opens).
    - Provides `RelocFile harness::LoadAndProcess(file_id, MockHeap&)`:
        - Reads the RELO sub-resource for the requested file.
        - Drives `lbreloc_bridge`'s pipeline with a deterministic mock
          allocator (fixed-base bump heap, file_id × 0x100000 stride so
          dependency files don't collide).
        - Captures: post-fixup bytes; the set of registered halfswap ranges
          (file-relative offsets); the set of struct-fixup tracker entries
          (file-relative); the chain-walk target list (file-relative offsets,
          not heap pointers).
    - Provides `MockHeap` — a fixed-base bump allocator that yields
      reproducible addresses regardless of test-ordering.
    - Provides `Snapshot` — a serialized representation of the captured
      state, comparable byte-for-byte and dump-able for debug.
- **First-pass refactor** (small, contained, risk-bounded): factor
  `lbreloc_bridge.cpp` so the heap allocator and resource source are
  injectable. Today they're pinned to `Ship::Context::GetInstance()` and the
  game's heap APIs. Introduce a thin interface
  (`IRelocHeap`, `IRelocResourceSource`) and pass instances by reference
  through the load path. Game-time code passes the existing impls; tests pass
  mocks. **This is the only structural change before fixtures land.**
- **Sanity tests**: hand-write 8 tests covering one representative file from
  each major category (animation / stage geometry / menu sprite / MV opening /
  effect / item / fighter attributes / collision). They exist to validate the
  harness + snapshot scheme before fanning out to subagents.

**Verification**:
- Sanity tests pass deterministically across 10 runs.
- `cmake --build <wt>/build --target port_reloc_tests -j 4` clean.
- ASAN+UBSAN clean on the harness path.

---

### Stage 2 — Generate fixtures for all 2,132 files (subagent fan-out, ~1 session)

- **Fixture format**: a binary `.fixt` file per reloc ID in
  `port/test/fixtures/<id>_<name>.fixt`. Layout:
    - 32-byte header: magic, version, file_id, fixture_kind_flags, body_size
    - Post-fixup bytes (full file, after Pass1+Pass2+struct+halfswap)
    - Halfswap-range list: u32 count + (u32 offset, u32 size)[]
    - Struct-fixup-tracker list: u32 count + (u32 offset, u32 size, u8 kind)[]
    - Chain-target list (internal): u32 count + (u32 slot_off, u32 target_off)[]
    - Chain-target list (external): u32 count + (u32 slot_off, u16 file_id, u32 target_off)[]
    - 4c-deswizzle list: u32 count + (u32 buf_off)[] (post-decode)
    - SHA256 of the body
- Fixtures are committed to the repo (`port/test/fixtures/`) and reviewed via
  diff. They are the ground truth — once committed, they never regenerate
  silently. Regen is an explicit `tools/regen_reloc_fixtures.py` invocation
  that diffs against the committed copy and refuses to overwrite without
  `--force`. (Belt-and-suspenders: if the runtime drifts, we want a
  detectable test failure, not silent fixture rotation.)
- **Subagent fan-out**: 10 waves × 10 subagents × ~22 files each. Each
  subagent gets:
    - A list of 20–25 file IDs.
    - Instructions to run the harness (`tools/regen_reloc_fixtures.py
      --ids <list>`), inspect the captured snapshot for sanity, and write
      the fixture file.
    - **Pass criteria**: snapshot is deterministic across 3 harness invocations;
      bytes match a manual spot-check of a known-good post-fixup region; SHA256
      stable.
    - **Fail criteria**: any nondeterminism, any segfault, any halfswap range
      that doesn't span an integer multiple of 4 bytes — these are bugs in
      the harness, not the runtime, and need to be reported back.
- Each subagent reports: list of fixtures generated, any anomalies, no-go
  files (if any) for human triage.

**Verification**:
- 2,132 `.fixt` files exist.
- `port_reloc_tests` (next stage) loads each one and asserts the runtime
  pipeline still produces it.

#### Stage 2 — Status note (2026-05-08, end of session)

Stage 2 landed with these deviations from the plan above:

- **Fixture sections** match the plan's intent but not its exact bytes-per-
  field. Tracker entries are stored as `(u32 byte_offset, u8 family, u8[3] pad)` —
  the plan's `(u32 offset, u32 size, u8 kind)` triple was aspirational; the
  byteswap layer only retains a single address per insertion, with no size or
  4c-deswizzle kind beyond `family`. The post-decode 4c-deswizzle list collapses
  into `family=2` of the tracker section. Body integrity uses CRC64 (libultraship's
  StrHash64 — already linked) instead of SHA256.
- **No subagent fan-out for fixture generation.** The C++ harness already
  amortizes the archive open across all 2,132 file IDs in a single process
  (3.6 s wall, 22 MB total disk). The fan-out moved to a four-agent QA pass
  on the *generated* fixtures: animations (1,633), submotions+fighters+stages
  (308), menus+movies+scene+others (160), plus a cross-cutting integrity scan.
  All four reported clean. See `tools/inspect_fixtures.py` for the per-file
  anomaly checks the QA pass ran.
- **Lazy-fixup gap (known limitation).** The harness drives `lbRelocLoadAndRelocFile`
  to completion and snapshots the resulting state. It does NOT run the game
  loop afterwards, so the fixtures do NOT capture state added by the lazy
  runtime fixup paths:
    * `portRelocFixupVertexAtRuntime` — fires from `gfx_vtx_handler_f3dex2` during render.
    * `portRelocFixupTextureAtRuntime` — fires from `GfxDpLoadBlock`/`LoadTile`/`LoadTlut`.
    * `portFixupSprite`/`Bitmap`/`SpriteBitmapData` — fire from decomp game code (`lbCommonMakeSObjForGObj` etc.) when sprites are drawn.
    * `port_aobj_event32_unhalfswap_stream` — fires from `gcParseDObjAnimJoint`/`gcParseMObjMatAnimJoint` when an animation is read.

  The fixtures are still load-time-faithful: every fixup that fires from
  inside `lbRelocLoadAndRelocFile` (Pass 1, Pass 2, chain walk including
  `portRelocFixupTextureFromChain`, halfswap registration) lands in the
  snapshot. **This is sufficient for Stages 4–6** (Pass 1 → Torch, Pass 2 →
  Torch, struct fixups → Torch), since each of those moves load-time work to
  build time. **Stage 7 (AObjEvent32 unhalfswap → Torch) needs the gap closed
  before landing**, since the lazy un-halfswap walker is exactly the migration
  target. Two options for closing it: (a) extend the harness to drive a
  headless attract-chain run and snapshot tracker state at scene-end (matches
  the original plan's "full attract chain run" mitigation), or (b) add a
  synthetic DL-walker that fires the lazy fixups without rendering. Decision
  deferred to the start of Stage 7.

---

### Stage 3 — Generate per-file unit tests (subagent fan-out, ~1 session)

- **Test pattern** (one TEST per file, parameterized via TYPED_TEST or
  INSTANTIATE_TEST_SUITE_P over file_id):
    ```cpp
    TEST(RelocFile_500_FTMarioAnim001, RuntimePipelineMatchesFixture) {
        auto snap = harness::LoadAndProcess(500, MockHeap{});
        auto fix  = harness::LoadFixture("500_FTMarioAnim001");
        EXPECT_EQ(snap, fix);  // operator== diffs verbosely on fail
    }
    ```
- Tests are auto-generated by `tools/gen_reloc_tests.py` from the catalog
  (one `.cpp` per category, ~50–500 TESTs each). Generated files live in
  `port/test/generated/` and are checked in (so CI doesn't depend on Python).
  Header comment marks them auto-generated; manual edits get clobbered.
- **Subagent fan-out**: another 10 waves × 10 subagents to (a) verify each
  test compiles, (b) verify each runs green, (c) verify failure-mode output
  is human-readable (force a test failure by mutating one byte and confirm
  the diff localizes the corrupt offset).
- **Smoke gate**: full `ctest` sweep of `port_reloc_tests` — expect all 2,132
  green, total wall time < 60s with `-j 4`.

**Verification**:
- All 2,132 tests pass.
- Mutating one byte in any container surfaces a clear file_id + byte-offset
  failure message.
- Mutating struct-fixup metadata surfaces a clear "fixup tracker mismatch"
  failure.

**Commit gate**: at the end of Stage 3, commit the harness, fixtures, and
generated tests. This is the load-bearing reset point — every subsequent
migration stage is gated on `ctest` staying fully green at every commit.

---

### Stage 4 — Migrate Pass 1 (blanket BSWAP32) to Torch (~1 session)

- Add `swap_pass1` step to `torch/src/factories/ssb64/RelocFactory.cpp`
  immediately before the binary write-out. Walks the entire decompressed
  blob and applies `BSWAP32` to every u32 word.
- Add a `processing_flags` u32 to the RELO container header
  (post-`extern_file_ids[]`, pre-`decompressed_data[]`):
    - bit 0 = `PROC_PASS1_BSWAP_DONE`
- Runtime side: `port/bridge/lbreloc_bridge.cpp` reads the flag; if set,
  skips Pass 1 in `portRelocByteSwapBlob`.
- Bump torch SHA in submodule pointer; **wipe asset cache and re-extract**
  (per CLAUDE.md asset/torch tight-coupling note). Re-run all 2,132 tests.
  They must remain green — fixtures should not need regeneration since the
  post-pipeline state is identical regardless of where Pass 1 ran.

**Verification**:
- `ctest` fully green.
- Manual: play attract chain, training mode, 1P stage 1.
- Live game's Pass 1 timing line in `ssb64.log` should disappear when
  `processing_flags` is read.

---

### Stage 5 — Migrate Pass 2 (vertex / texture eager fixup) to Torch (~1–2 sessions)

- Port the GBI-walker logic from `lbreloc_byteswap.cpp::pass2_*` into
  `torch/src/factories/ssb64/RelocFactory.cpp`. The walker scans the full
  blob for top-level DL stream entry points (heuristic: known DL offsets
  from the chain, or all u32 words matching the F3DEX2 `G_ENDDL` / opcode
  signature; refer to existing port impl).
- For each G_VTX with `seg==0x0E`, apply per-Vtx rotate16 (s16 fields) +
  BSWAP32 (RGBA quad).
- For each G_SETTIMG + G_LOADBLOCK/G_LOADTLUT pair with `seg==0x0E`, apply
  format-specific fixup (BSWAP32 + TMEM line-deswizzle for non-32bpp,
  BSWAP32 only for 32bpp).
- Add `processing_flags` bit 1 = `PROC_PASS2_DONE`. Runtime skips Pass 2
  if set.
- The lazy paths (`portRelocFixupVertexAtRuntime`,
  `portRelocFixupTextureAtRuntime`) stay live for runtime-built DLs (e.g.
  MObjSub-driven texture loads). Their idempotency tracker means already-
  fixed words are no-ops, so they're safe.

**Verification**:
- `ctest` fully green.
- Manual: every stage played end-to-end (Dream Land, Yoster, Sector Z,
  Mushroom Kingdom, Pupupu, etc. — the cases historically sensitive to
  texture/vertex fixup ordering).

---

### Stage 6 — Migrate struct fixups to Torch (~2 sessions)

This is the largest stage. Torch needs a per-file "struct fixup table"
metadata describing which struct types live at which offsets in each file.
The runtime currently knows this implicitly (fixup helpers are called from
specific decomp source sites with specific offsets).

- **Step 6a — Mine the struct-fixup metadata.** Extract every
  `portFixupStructU16/U32/Sprite/Bitmap/MObjSub/FTAttributes` call site
  from decomp source via a static-analysis pass (`tools/mine_struct_fixups.py`,
  reads `decomp/src/**/*.c` with regex + AST hints). Output:
  `port/test/struct_fixup_catalog.json` — maps `(file_id, byte_offset) →
  fixup_kind`.
- **Step 6b — Validate the catalog** by replaying it through the runtime
  harness and comparing against the actual tracker entries captured in the
  Stage 2 fixtures. Any mismatch = a missing call site (find it in code
  review and add to the catalog).
- **Step 6c — Embed the catalog in Torch.** Torch reads the catalog at
  build time and applies each fixup to the bytes before write-out.
- **Step 6d — Add `processing_flags` bits 2–7** for each fixup family
  (StructU16, StructU32, Sprite, Bitmap, MObjSub, FTAttributes). Runtime
  skips each independently if its bit is set. This per-family isolation
  lets us roll one family at a time and bisect regressions to one helper.
- **Step 6e — Drop runtime calls site-by-site**, replacing each
  `portFixupStructU16(...)` with `assert(/* fixup already done at build */)`
  guarded by a debug-only flag, then later removing entirely.

**Verification**: `ctest` green per sub-step; full attract chain + 1P
clear + VS results screen between each sub-step.

---

### Stage 7 — Migrate halfswap + AObjEvent32 unhalfswap to Torch (~1 session)

- **Halfswap**: trivial — torch checks if file path matches
  `reloc_animations/FT*` / `reloc_submotions/FT*` /
  `reloc_scene/SCExplainMain*`, applies the rotate16 on every non-reloc-slot
  u32 word.
- **AObjEvent32 unhalfswap**: less trivial — runtime currently does this
  lazily because stream entry points aren't always statically known. But
  for fighter animations they ARE: every fighter animation file has a
  known top-level animation table indexed by animation ID. Torch walks
  that table at build time, follows each AObjEvent32 head, simulates the
  unhalfswap walker, and emits already-unhalfswapped bytes.
- Edge case: streams reachable only through cross-file references. Audit
  these via the Stage 2 fixtures — if any animation stream is only
  reachable through external chain, document and keep its lazy fixup.
  (Likely 0 cases, but verify.)
- `processing_flags` bits 8–9 = `PROC_HALFSWAP_DONE`,
  `PROC_AOBJEVENT32_UNHALFSWAP_DONE`.

**Verification**: `ctest` green; manual fighter intro for every character
(historically sensitive to AObjEvent32 corruption).

---

### Stage 8 — 4c sprite decompression decision (~0.5 session)

- 4c sprites need post-decode deswizzle. Decompression itself is in
  `lbCommonDecodeSpriteBitmapsSiz4b` and runs at runtime.
- **Option A**: keep decompression at runtime, keep deswizzle at runtime
  (status quo). No torch change.
- **Option B**: decompress at build, write decoded bytes + already-
  deswizzled. Requires the 4c codec in torch and a flag on the asset to
  signal "already decoded."
- **Recommendation**: Option A. The deswizzle is small, the codec is
  large, and runtime decompression has no correctness issues today.
  Document in `docs/bugs/` why we chose to leave this on the runtime side.

**Verification**: confirm 4c decode path still works; spot-check
sprites that historically used 4c (compressed character animations).

---

### Stage 9 — Pre-compute chain offsets in Torch (~1 session)

Instead of the runtime walking the encoded reloc chain (which requires
parsing a bit-packed list at load time), torch emits a flat sidecar:
`(slot_byte_offset, target_byte_offset, is_external, target_file_id)[]`.

- Torch reads `relocInternOffset` and `relocExternOffset` from the table
  entry, walks each chain to the end, emits the flat list.
- Runtime drops the chain-walking code in `lbreloc_bridge.cpp` and just
  iterates the flat list, registering each target as a token.
- `processing_flags` bit 10 = `PROC_CHAIN_FLATTENED`.

**Verification**: `ctest` green; chain-walk unit-tests in Stage 3 already
verify per-file chain target lists are byte-identical regardless of
whether they came from the encoded chain or the flattened sidecar.

---

### Stage 10 — Sub-resource emission for moddability (~2 sessions)

For each reloc file, torch emits sub-resources alongside the container:

- `reloc/<id>/vtx/<offset>` — each Vtx run discovered by Pass 2
- `reloc/<id>/tex/<offset>/<fmt>_<siz>` — each texture image
- `reloc/<id>/tlut/<offset>` — each palette
- `reloc/<id>/dl/<offset>` — each top-level DL
- `reloc/<id>/anim/<offset>` — each AObjEvent32 head
- `reloc/<id>/sprite/<offset>` — each Sprite/Bitmap pair
- `reloc/<id>/struct/<offset>/<type>` — each fixed-up struct (FTAttributes
  etc.) for moddability of stat/balance data

Sub-resource paths are stable across rebuilds (they key on file ID +
internal offset, both of which are deterministic). The container
references its sub-resources by CRC64 hash, mirroring the pattern already
used by `DisplayListFactory.cpp::260-318`.

Containers store sub-resource hashes in their header so the runtime can
ask the resource manager whether any are overridden — and if so, overlay
the sub-resource bytes into the container blob *before* chain-walking.

`processing_flags` bit 11 = `PROC_SUBRESOURCES_EMITTED`.

**Verification**: `ctest` green; one hand-rolled override test (replace a
Mario animation Vtx blob and confirm it renders).

---

### Stage 11 — Runtime simplification (~0.5 session)

After every `processing_flags` bit is set on every file:

- Delete the now-unused runtime byteswap / struct-fixup / halfswap /
  unhalfswap implementations (or guard them under a debug-only fallback
  for hypothetical pre-build-time-fixup archives).
- Tests stay green.
- Code shrinks meaningfully — `port/bridge/lbreloc_byteswap.cpp` shrinks
  from ~1500 LOC to a thin wrapper around chain registration; the
  `port_aobj_fixup` family becomes empty.

**Verification**: full game attract + 1P + VS + BTT + training mode.

---

### Stage 12 — Mod-override `.o2r` loading path (~1 session)

- `port/Ship::Context` initialization adds a `mods/` directory scan after
  `BattleShip.o2r` is loaded. Each `.o2r` in `mods/` is added to the
  `ArchiveManager` *after* the main archive — last-archive-wins means its
  resources override.
- Container loader checks for sub-resource overrides by CRC64 and overlays
  bytes if present.
- One canned mod ships in `tools/example_mod/` to demo the workflow:
  swap one Mario stock-sprite color.

**Verification**: drop the example mod into `mods/`, launch, confirm
the swap; remove, confirm vanilla.

---

### Stage 13 — Remove the from-source pipeline + Battle-ShipYard submodule (~1 session)

Once the build-time Torch pipeline is the canonical producer of
`BattleShip.o2r`, the parallel "compile decomp .c source through clang"
pipeline becomes dead weight. Per user 2026-05-08: Battle-ShipYard and
all its compile-from-reloc-source machinery are scheduled for removal
after this refactor is successful.

- Remove the `Battle-ShipYard` git submodule and delete
  `Battle-ShipYard/cmake/RelocData.cmake` from the build graph.
- Remove the `BattleShip.fromsource.o2r` opt-in branch from
  `port/port.cpp` (lines ~567–587 — the `SSB64_RELOC_FROMSOURCE` env
  var + alt-archive load).
- Remove `decomp/src/relocData/` — the per-reloc `.c` source files that
  the from-source pipeline compiled. (Keep them only if a reader of
  this plan later finds an unrelated use; default action: delete.)
- Remove `tools/build_reloc_resource.py`,
  `tools/struct_byteswap_tables.py`, `tools/extract_inc_c.py`,
  `tools/gen_reloc_cmake.py` — the from-source build-time tooling.
  Cross-reference against `cmake/RelocData.cmake` consumers to ensure
  nothing else in the port depends on them.
- Update `docs/architecture.md` to drop the from-source pipeline
  section.
- Update `MEMORY.md` entries that reference the from-source pipeline
  (currently `project_relocdata_from_source.md`,
  `project_battleshipyard_https_origin.md`) — mark resolved or remove.

**Verification**: full clean rebuild from scratch using only Torch's
`BattleShip.o2r`. All 2,132 fixtures still pass. No runtime change.

This stage is gated on Stage 11 (runtime simplification) being landed
and the from-source pipeline being demonstrably unused. If Stage 11
cuts deeper than expected and the from-source path is touched, do this
removal *before* Stage 11's drop of runtime fixup machinery.

---

## Verification strategy (across all stages)

1. **`ctest`** on `port_reloc_tests` — primary gate. Must be green at every
   commit.
2. **Manual playthrough sweeps** at every "user-visible" stage boundary
   (Stages 4, 5, 6e, 7, 10, 12). Sweep covers attract chain → 1P stage 1
   → VS → training → BTT → 1P clear screen.
3. **ASAN+UBSAN** Debug build runs cleanly on the harness and through
   one full attract.
4. **Fixture diff review**: between Stage 3 commit and Stage 11 final, no
   `.fixt` file should change. If one does, that's a bug in the migration
   and the diff localizes which file regressed.

---

## Critical files

### Existing files modified (no Stage adds new files outside their category):

- `port/bridge/lbreloc_bridge.cpp` — heap/resource injection; chain-walk
  shrinks to flat-list iteration; runtime fixups guarded out.
- `port/bridge/lbreloc_byteswap.{h,cpp}` — Pass 1/2 helpers gated by
  `processing_flags`; eventually shrunk to lazy-fallback only.
- `port/port_aobj_fixup.{h,cpp}` — halfswap walker eventually deleted.
- `port/resource/RelocFile.{h,cpp}` — header reads `processing_flags` +
  flat chain list + sub-resource hash table.
- `port/resource/RelocFileFactory.cpp` — versioned reader for header.
- `decomp/src/lb/lbreloc.c` — already gated under `#ifndef PORT`; touch
  only if struct-fixup call sites need adjustment.
- `torch/src/factories/ssb64/RelocFactory.{h,cpp}` — gains all transformation
  steps (Pass 1, Pass 2, struct fixups, halfswap, unhalfswap, chain
  flatten, sub-resource emit).

### New files:

- `port/test/CMakeLists.txt` — GTest target.
- `port/test/reloc_harness.{h,cpp}` — load + process + snapshot.
- `port/test/mock_heap.{h,cpp}` — deterministic bump allocator.
- `port/test/snapshot.{h,cpp}` — snapshot serialization + comparison.
- `port/test/fixtures/<id>_<name>.fixt` × 2,132 — committed fixtures.
- `port/test/generated/<category>_tests.cpp` — auto-generated TESTs.
- `port/test/struct_fixup_catalog.json` — mined struct-fixup metadata.
- `tools/regen_reloc_fixtures.py` — fixture regenerator.
- `tools/gen_reloc_tests.py` — test generator.
- `tools/mine_struct_fixups.py` — mines decomp source for fixup call sites.
- `docs/refactor_buildtime_reloc_plan.md` — this plan, persisted.

### Reused (no changes expected):

- `libultraship/tests/CMakeLists.txt` — pattern for GTest integration.
- `libultraship/include/ship/resource/ResourceManager.h` — `LoadResource(crc)`,
  `LoadResources(searchMask)`, last-archive-wins via `mFileToArchive`.
- `decomp/tools/relocFileDescriptions.us.txt` — file-ID → name catalog.
- `yamls/us/reloc_*.yml` — file-ID → asset path catalog.

---

## Risks & mitigations

| Risk | Mitigation |
|------|------------|
| Harness mock heap doesn't match real heap layout, fixtures aren't representative | MockHeap uses a wide stride (`0x100000` per file) so dependency ordering is observable; harness asserts no two files alias |
| Lazy runtime fixups (vertex/texture from runtime-built DLs) aren't captured in fixtures, regress later | Stage 2 captures `sTexFixupWords` and `sStructU16Fixups` deltas from a *full attract chain run*, not just file load. Lazy fixups end up in the fixture if any attract-chain code path triggers them. Document any not-attract-reachable paths and call them out in Stage 11 |
| Subagent fan-out introduces inconsistent fixture format | All subagents call the same `tools/regen_reloc_fixtures.py`; harness+writer is one binary. Subagents can't regenerate without the tool |
| Migration breaks something fixture didn't catch (e.g. heap-reuse interaction) | Stages 4, 5, 6 each get a manual playthrough sweep before commit. Any visual regression triggers a per-file bisect via the fixture diff |
| Sub-resource hash collisions across files | Path scheme: `reloc/<id>/<kind>/<offset>` — `id` is unique across all 2,132 files; collisions impossible |
| Override mod path conflicts with main resource for non-overridden sub-asset | LUS `mFileToArchive[hash]` is keyed on full path — only exact-match resources collide. Container reads sub-resource bytes by hash, so untouched sub-assets fall through to the main archive |
| One of the 21 transformations turns out non-deterministic (e.g. depends on global state at load time) | Sanity tests in Stage 1 (8 files across categories) are run 10 times each; nondeterminism caught before Stage 2 fan-out |

---

## Estimated session count

- Stage 0: 0.5 sessions
- Stage 1: 1 session
- Stage 2: 1 session (heavy subagent fan-out)
- Stage 3: 1 session (heavy subagent fan-out)
- **First commit gate** (Stage 3 done): ~3.5 sessions in
- Stages 4–7: 4 sessions
- Stage 8: 0.5 session
- Stages 9–10: 3 sessions
- Stages 11–12: 1.5 sessions
- Stage 13: 1 session (Battle-ShipYard + from-source removal)
- **Total**: ~13 sessions, give or take

The plan document itself (`docs/refactor_buildtime_reloc_plan.md`) plus the
fixture+test commit at the end of Stage 3 are the two load-bearing
artifacts that make this resumable across sessions.
