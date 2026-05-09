# Build-time reloc refactor — session handoff (2026-05-08)

You are picking up the build-time reloc-data refactor described in
`docs/refactor_buildtime_reloc_plan.md`. **Read that plan end-to-end first** —
this handoff is the delta on top of it, not a replacement.

## Where the work lives

- **Outer branch**: `agent/buildtime-reloc` off main `314dc95` (this commit:
  see `git log` below).
- **Worktree**: `.claude/worktrees/buildtime-reloc`. All commands in this
  document run from there. The main tree is left undisturbed.
- **Submodule branches** (for changes that ride alongside the outer commits):
    - `decomp` → `port-patches-buildtime-reloc` (no changes yet — clean off `port-patches`)
    - `libultraship` → `ssb64-buildtime-reloc` (one commit: see below)
    - `torch` → `ssb64-buildtime-reloc` (no changes yet — clean off `ssb64`)

`./scripts/new-worktree.sh` doesn't apply here — the worktree already exists.
Just `cd` into it.

## What is committed and verified

### Outer branch (`agent/buildtime-reloc`)

```
64cda95 port/test: commit ground-truth fixtures for all 2,132 reloc files
a04f016 port/test: fixture format + regen tool + ctest hook (Stage 2)
9c9e7d3 port/test: exclude port/test/ from the main ssb64 source glob
85cbb54 port/test: GTest harness for the runtime reloc pipeline (Stage 1)
5adf40e port: add test-only DI hooks to lbreloc bridge
b424ad0 docs: refactor plan for build-time reloc-data migration
```

### libultraship (`ssb64-buildtime-reloc`)

```
5b87696f Context: guard Window/Config null in destructor
```

### What is verified at HEAD

```
cmake -B build -DSSB64_BUILD_TESTS=ON
cmake --build build --target ssb64 -j 4              # main game still builds
cmake --build build --target port_reloc_tests -j 4   # GTest harness
cmake --build build --target port_reloc_regen -j 4   # fixture regenerator
./build/port/test/port_reloc_tests                   # 10 tests, ~7s, all pass
./build/port/test/port_reloc_regen --all --check --quiet  # 2132 unchanged, ~4.4s
ctest --test-dir build/port/test                     # 11 tests pass (10 + fixture-check)
```

## What this gives you (the contract for stages 4+)

**`port_reloc_check` is the per-commit drift gate.** Every subsequent
migration step in Stage 4+ MUST keep it green at every commit:

```
ctest --test-dir build/port/test -R fixture-check
```

It loads `BattleShip.o2r`, drives the bridge through `lbRelocLoadAndRelocFile`
for every one of the 2,132 reloc files, and compares the resulting state
byte-for-byte against the committed `port/test/fixtures/*.fixt`. If you break
the load-time pipeline for one file, the test names that file. Mutating one
byte of the post-fixup output for one file is enough to fail.

**This is the load-time half of the runtime pipeline.** Specifically the
fixtures pin:

- the `memcpy` of decompressed bytes into the heap slot (Torch already
  decompressed VPK0 at extraction time)
- Pass 1 blanket `BSWAP32`
- Pass 2 DL-walk eager fixups (in-file vertex / texture via segment 0x0E)
- internal + external chain walk (token registration + the slot rewrites)
- `portRelocFixupTextureFromChain` for chain slots whose preceding GBI cmd is
  G_SETTIMG / G_VTX
- halfswap registration for fighter figatrees (1,633 anims + 142 submotions
  + 1 `SCExplainMain`)

**It does NOT cover the lazy paths** that fire later from rendering or
game-tick code. See "Known gap" below.

## File map (where things live)

```
port/bridge/lbreloc_bridge_testing.h    test-only DI surface
port/bridge/lbreloc_bridge.cpp          chain walk + observer hooks
port/bridge/lbreloc_byteswap.cpp        Pass1/Pass2 + tracker enumeration
port/test/CMakeLists.txt                two binaries (tests + regen) + ctest
port/test/mock_heap.{h,cpp}             deterministic bump allocator
port/test/snapshot.{h,cpp}              comparable + diff-able post-load state
port/test/reloc_harness.{h,cpp}         single-process LoadAndProcess driver
port/test/fixture_format.{h,cpp}        .fixt binary layout (CRC64 trailer)
port/test/regen_fixtures_main.cpp       port_reloc_regen CLI
port/test/test_main.cpp                 GTest entry; SIGSEGV bt handler
port/test/test_stubs.cpp                no-op stubs for game-side externs
port/test/sanity_tests.cpp              8 hand-picked sanity tests
port/test/fixtures/<id>_<name>.fixt     2,132 committed ground-truth files
port/resource/RelocPointerTable.{h,cpp} added portRelocResetPointerTableForTest
tools/inspect_fixtures.py               offline Python anomaly scanner
docs/refactor_buildtime_reloc_plan.md   the master plan; Stage 2 status note
                                        at the bottom flags the lazy-fixup gap
```

## Gotchas you'll hit (in order of how easy it is to forget)

1. **LSP / clangd warnings inside this worktree are mostly noise.** The IDE
   doesn't pick up CMake's include paths, so you'll see "`gtest/gtest.h` file
   not found", "no member named `filesystem` in namespace `std`", "no member
   named `Ship` in the global namespace", etc., all over `port/test/`. These
   are bogus — the actual compile under cmake works fine. Don't chase them.

2. **`cmake --build build --target ssb64 -j 4` on M1 16GB.** Stay at `-j 4`,
   per `CLAUDE.md`. Each Debug C++ TU here is 1–2 GB resident.

3. **Each test process re-opens `BattleShip.o2r` (~2.5 s startup).** A single
   `port_reloc_tests` invocation pays this once. `ctest` with
   `gtest_discover_tests` runs each test as its own process, so 11 tests ×
   2.5 s = ~30 s instead of the ~7 s a single-process run takes.
   **Implication for Stage 3**: do NOT generate 2,132 separate `TEST()`
   definitions and let `gtest_discover_tests` shard them into 2,132
   processes (would be ~90 minutes). Use `INSTANTIATE_TEST_SUITE_P` over a
   single `TEST_P` so the whole sweep runs in one process and gets per-id
   names from gtest's automatic naming.

4. **Token table determinism between test loads.** Production
   `portRelocResetPointerTable()` increments the generation counter (so stale
   tokens from prior scenes don't resolve through the new table). That breaks
   load-to-load byte equality in tests, because token bytes change every
   reset. The harness uses **`portRelocResetPointerTableForTest()`** which
   pins the generation. If you add new test entry points, use the test
   variant.

5. **The bridge needs separate intern + extern heaps.** Recursive dependency
   loads via `nLBFileLocationDefault` (the harness's default) go through
   `lbRelocGetInternBufferFile`, which uses the bounded
   `sLBRelocInternBuffer.heap_ptr/end`. `lbRelocGetExternBufferFile` uses
   `sLBRelocExternFileHeap` (unbounded). They MUST be distinct buffers — the
   bump pointers don't share bookkeeping. `portRelocBridgeSetupForTest` takes
   both.

6. **`Ship::Context::~Context()` was patched in the libultraship submodule.**
   It null-guards `GetWindow()->SaveWindowToConfig()` and `GetConfig()->Save()`
   so headless callers (only ResourceManager initialized, no Window) can tear
   down cleanly. If you bump libultraship to a fresh upstream, re-apply this
   patch (`5b87696f`).

7. **Fixture path resolution.** The regen tool walks up from cwd looking for
   a directory containing `port/test/sanity_tests.cpp` (to disambiguate the
   source tree from the build tree, both of which contain `port/test/`).
   `SSB64_FIXTURES_DIR` env var overrides. The GTest binary uses
   `SSB64_TEST_O2R_PATH` env var (defaulted to `${CMAKE_BINARY_DIR}/BattleShip.o2r`
   by `gtest_discover_tests`).

8. **Asset/Torch tight coupling (per CLAUDE.md memory).** Any `torch` SHA bump
   in a future commit means: wipe `BattleShip.o2r`, wipe the torch build
   dir, re-extract. Otherwise the runtime can ingest stale bytes that the
   new torch version produces differently.

9. **Don't run `port_reloc_regen` while subagents are inspecting fixtures.**
   The regen tool overwrites in place (with `--force`). Inspecting subagents
   see torn output if you regen mid-flight. Cheap to avoid: only regen
   between agent waves.

## Known gap (will bite Stage 7)

The fixtures capture only what `lbRelocLoadAndRelocFile` produces. They do
NOT capture state added later by lazy fixup paths:

- `portRelocFixupVertexAtRuntime` — fires from `gfx_vtx_handler_f3dex2` during render
- `portRelocFixupTextureAtRuntime` — fires from `GfxDpLoadBlock` / `LoadTile` / `LoadTlut`
- `portFixupSprite` / `portFixupBitmap` / `portFixupSpriteBitmapData` — fire
  from decomp `lbCommonMakeSObjForGObj` etc. when sprites are drawn
- `port_aobj_event32_unhalfswap_stream` — fires from
  `gcParseDObjAnimJoint` / `gcParseMObjMatAnimJoint` when an animation is read

**Sufficient for Stages 4–6** (Pass 1, Pass 2, struct fixups all migrate
load-time work to build time; the lazy paths stay live and untouched).

**Insufficient for Stage 7** (AObjEvent32 unhalfswap → Torch is exactly the
lazy walker). Two options when you get there:

- **(a)** Extend the harness to drive a headless attract-chain run — boot
  the game without a window, tick the cooperative scheduler through enough
  scenes to exercise every lazy path, snapshot tracker state at scene-end.
  Matches the original plan's "full attract chain run" mitigation.
- **(b)** Add a synthetic DL-walker that fires the lazy fixups without
  rendering — instrument the bridge to walk every reachable DL after load
  and call the lazy fixup helpers manually.

Decision deferred. Re-read the bottom of `docs/refactor_buildtime_reloc_plan.md`
when you reach Stage 7.

## What to do next

The plan's stages are sequential but not all equally load-bearing. In order
of priority:

### Option A: Stage 3 (per-file unit tests) — RECOMMENDED skip-or-merge

The plan calls for `tools/gen_reloc_tests.py` to auto-generate a `TEST()`
per file_id from the catalog. **Reconsider this.** The existing
`port_reloc_check` ctest target already runs the same comparison for all
2,132 files in 4.4 s, with file-id-tagged failure output. Per-file `TEST()`
definitions would just multiply ctest's bookkeeping for the same coverage.

Two variants of "Stage 3" worth considering instead:

- **Variant 3a (cheap)**: skip per-file tests entirely. Treat `port_reloc_check`
  as the per-file gate. Move directly to Stage 4. The Stage 1 sanity tests
  + `port_reloc_check` together give: explicit assertions on the
  representative cases (halfswap presence/absence, determinism), plus
  byte-equality across the full 2,132. That's the same coverage Stage 3
  promised.
- **Variant 3b (single-process TEST_P)**: ONE `TEST_P` parameterized over
  `file_id` 0..2131 inside `port_reloc_tests`, instantiated via
  `INSTANTIATE_TEST_SUITE_P` with a custom name function that produces
  per-id test names (e.g. `RelocFile_500_FTMarioAnim001`). Granular failure
  reporting, single-process execution. ~5 minutes of work.

Recommend variant **3a or 3b** depending on how much per-id ctest reporting
you want. Both deliver the plan's goal.

### Option B: jump to Stage 4 (Pass 1 → Torch)

Per the plan:

- Add `swap_pass1` step to `torch/src/factories/ssb64/RelocFactory.cpp`
  immediately before the binary write-out.
- Walk the entire decompressed blob, apply `BSWAP32` to every u32 word.
- Add a `processing_flags` u32 to the RELO container header
  (post-`extern_file_ids[]`, pre-`decompressed_data[]`). Bit 0 =
  `PROC_PASS1_BSWAP_DONE`.
- Bump `torch` SHA in submodule pointer; **wipe asset cache + re-extract**.
- Read the flag in `port/bridge/lbreloc_bridge.cpp` and skip `pass1_swap_u32`
  when set.
- Run `ctest -R fixture-check` at every commit. Must stay green throughout
  — the fixtures should NOT change since the post-pipeline state is
  identical regardless of whether Pass 1 ran in Torch or in the bridge.

The non-obvious risk: Torch must use the EXACT same byte order as the
runtime expects. Torch reads ROM in BE order (it has its own endian
handling). Carefully verify: after Torch's BSWAP32, the bytes Torch writes
into the o2r are in HOST byte order? Or in BE that the runtime would then
NOT byte-swap? Trace through `Cartridge::Normalize` and the writer.

If you break a fixture: the failure prints the file_id and first divergent
byte offset. Open the corresponding `port/test/fixtures/<id>_*.fixt` and
the runtime output (the bridge produced it just now) and diff.

### Open from Stage 1 (still pending)

- **ASAN+UBSAN clean run on the harness.** Plan promised this in Stage 1's
  verification. Never executed. Build with
  `-DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"` in a separate build dir
  and run `port_reloc_tests`. Likely clean, but unverified.

### Submodule pushes (housekeeping, do once)

The libultraship branch `ssb64-buildtime-reloc` only exists locally. Push
it to your fork so the outer branch's submodule pointer resolves on a fresh
clone:

```
git -C libultraship push origin ssb64-buildtime-reloc
```

The decomp + torch submodules' branches were created but have no commits;
push isn't strictly needed yet but it doesn't hurt:

```
git -C decomp push origin port-patches-buildtime-reloc
git -C torch  push origin ssb64-buildtime-reloc
```

The outer worktree's branch is local only too. Push when ready for review.

## Quick command reference

```
# Configure (one-time)
cmake -B build -DSSB64_BUILD_TESTS=ON

# Build
cmake --build build --target ssb64 -j 4              # game
cmake --build build --target port_reloc_tests -j 4   # tests
cmake --build build --target port_reloc_regen -j 4   # regen CLI

# Run tests
./build/port/test/port_reloc_tests                   # 10 sanity tests
./build/port/test/port_reloc_regen --all --check     # all 2,132 fixtures
ctest --test-dir build/port/test                     # full sweep (per-process)
ctest --test-dir build/port/test -R fixture-check    # the drift gate

# Regenerate fixtures (when intentional, e.g. Stage 11)
./build/port/test/port_reloc_regen --all --force --quiet

# Anomaly scan from python
python3 tools/inspect_fixtures.py
python3 tools/inspect_fixtures.py --range 500 700 --json
```

## What the user has explicitly asked for / agreed to

- Use Sonnet for spawned subagents (memory directive).
- Don't bypass git hooks; create new commits rather than amend.
- Phased execution, max 5 files per phase. (Stages 1 + 2 each came in
  multiple commits; Stage 4 should follow the same pattern — small commits
  the drift gate can catch one at a time.)
- Test fidelity gap is acknowledged; user did not pick option (a) or (b)
  for closing it. Decision deferred to Stage 7.

## Sanity check the next session should run first

Before touching anything, run the drift gate to confirm the inherited state
matches the committed fixtures:

```
cd .claude/worktrees/buildtime-reloc
cmake --build build --target port_reloc_regen -j 4
./build/port/test/port_reloc_regen --all --check --quiet
```

Expected: `done in ~4400 ms — written=0, unchanged=2132, missing=0,
mismatch=0, skipped_empty=0, failed=0`.

If you see ANY mismatch, **stop and investigate before proceeding**.
Either the worktree got mutated since the handoff, or the BattleShip.o2r
in `build/` is from a different toolchain SHA than the fixtures were
generated against. Re-extract or check submodule pointers before doing any
new work.
