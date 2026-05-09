# Build-time reloc refactor — Stage 11 handoff (2026-05-09, Stage 10 + follow-ups A/B/C closed)

You are picking up the build-time reloc refactor described in
`docs/refactor_buildtime_reloc_plan.md`. Stages 1–10 plus Stage 10
follow-ups A (chain-resolved pixel-buffer enumeration), B (drift-gate
sub-resource validator), and C (mods-scanner determinism) have landed
on `agent/buildtime-reloc`. This handoff is the delta on top of the
plan for Stage 11 — runtime simplification.

**Read the plan first.** This handoff covers what's specific to
Stage 11; it does not re-explain the architecture.

---

## Status entering this session

Branch HEAD on `agent/buildtime-reloc`: see `git log --oneline -10`.
Recent landings:

```
6e43ef0 docs: mark Stage 10 follow-ups B + C done in handoff
0b81d3e port_reloc_regen: add --check-subres v3 sub-resource validator (B)
77afbc5 Sort mods/ scanner entries alphabetically (C)
0ad444f Bump torch + emit chain-resolved pixel buffers (A)
80baeb0 docs: enumerate Stage 10 follow-ups in handoff before Stage 11/12
7deca7a Bump torch + emit reloc sub-resources + overlay path (Stage 10)
```

The bit map of `processing_flags` after Stage 10:

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

In v3 archives every flag bits 0–8, 10, 11 is set on every file
(bit 9 is the deliberate Stage-7 carve-out). The runtime byteswap path
that consults each flag has been a no-op on v3 input since Stage 10
landed.

Two ADRs document the carve-outs — read these before deleting anything
runtime-side under Stage 11:

- `docs/decision_aobjevent32_runtime_walker_2026-05-09.md` — Stage 7:
  the AObjEvent32 unhalfswap walker stays runtime-side. Bit 9 is
  reserved for a future torch capture if/when that decision flips.
- `docs/decision_4c_sprite_codec_runtime_2026-05-09.md` — Stage 8:
  `portDeswizzleDecodedSprite4c` stays runtime-side (it runs after the
  4c decoder, which is also runtime-side).

---

## Sanity check first

```bash
cd .claude/worktrees/buildtime-reloc
./build/port/test/port_reloc_regen --all --check --check-subres --quiet
```

Expected:
```
regen: done in ~4500 ms — written=0, unchanged=2132, missing=0, mismatch=0, …
regen: subres — validated=4048, failed=0 (all categories 0), unique_hashes=4048
```

If either gate drifts, **stop and diagnose before any Stage 11 edits.**
Recovery steps live in `docs/refactor_buildtime_reloc_plan.md`
(wipe + re-extract); the asset/torch coupling means any torch SHA
change requires deleting `BattleShip.o2r` + the torch build dir +
re-running `cmake --build build --target ExtractAssets`.

---

## Stage 11 — Runtime simplification

Per the plan: now that every transformation bit (except the deliberately
reserved bit 9) is set on every file in v3 archives, the runtime
byteswap/halfswap helpers are dead code along the hot path. Stage 11
trims them.

### Scope

- `port/bridge/lbreloc_byteswap.cpp` is ~1500 LOC mixing three concerns:
  1. Byteswap helpers (pass1, pass2, struct, halfswap) — **dead on v3**.
  2. Heap-absolute trackers (`sStructU16Fixups`, the cache-eviction
     callbacks `portTextureCacheDeleteRange` etc.) — **stay**, used by
     the bridge for cache invalidation, unrelated to byteswap.
  3. Sprite codec utilities including `portDeswizzleDecodedSprite4c` —
     **stay** per Stage 8 ADR.

- `port/port_aobj_fixup.{h,cpp}` — **stay** per Stage 7 ADR. Skip.

- `port/bridge/lbreloc_bridge.cpp::portRelocByteSwapBlob` — every flag
  it consults is set on v3 input, so the call is a no-op. Either
  delete the call or keep it as a defensive `assert(all flags set)`.

Plan-target outcome: `lbreloc_byteswap.cpp` shrinks from ~1500 LOC
to ~150 LOC of heap-absolute trackers + the kept sprite codec helpers.
Don't `rm` the file; split it into a thin tracker/cache-invalidation
header + delete the byteswap guts.

### V0/V1/V2 reader decision (Stage 10 follow-up E)

Coupled to Stage 11. Once the byteswap helpers are gone, V0/V1/V2
archives (where the flags are unset and the runtime *was* the
fallback) won't load correctly. Two options:

- **Keep V0/V1/V2 readers** with a slow path that re-runs the byteswap
  helpers. Forces Stage 11 to keep the helpers as a fallback —
  partial gain only.
- **Drop V0/V1/V2 entirely** (recommended). Asset/torch coupling
  already forces a re-extract on every torch SHA bump; older archives
  are de-facto unsupported. Strictly simpler + smaller binary.

Recommend **drop**. Implementation: remove the V0/V1/V2
`RegisterResourceFactory` calls in `port/port.cpp` +
`port/test/reloc_harness.cpp`, delete the corresponding factory
classes in `port/resource/RelocFileFactory.cpp`. Document in the
Stage 11 commit message.

See `docs/refactor_buildtime_reloc_followups.md` for the full
follow-up E write-up.

### Verification

After each delete:
1. `cmake --build build --target ssb64 -j 4` — clean build.
2. `cmake --build build --target port_reloc_regen -j 4` — gate tool
   still compiles after the byteswap helpers it links against shrink.
3. `./build/port/test/port_reloc_regen --all --check --check-subres --quiet`
   — both gates green: 2,132 unchanged, 4,048 validated.
4. Run the binary headless on an attract chain (or just boot to title)
   to catch anything the gate doesn't cover (e.g. tracker code paths
   that invoked a deleted helper as a side effect).

Phase the work — don't try to delete everything in one commit.
Reasonable phasing:

- Phase 1: delete the pass1/pass2 helpers + their call sites (the
  bridge already early-returns when both flags are set; verify and
  delete).
- Phase 2: delete the struct fixup helpers (each family individually
  if you want bisectable commits).
- Phase 3: delete the halfswap helpers.
- Phase 4: delete `portRelocByteSwapBlob` (or convert to assert).
- Phase 5: V0/V1/V2 factory drop (E) — last, since the helpers it
  needed are already gone by then.

Keep the gate green between phases.

---

## Stage 12 — Productize mods/ scanner (deferred)

After Stage 11, two UX gaps remain in the mod path:

1. **Discoverability UI**: list active mods in the Esc menu
   (`port/PortMenu.{h,cpp}`), expose toggles to enable/disable per-mod
   without restart. The override path can re-engage simply by toggling
   `portRelocSetOverridesActive` and re-loading affected reloc files —
   the plan proposes only a restart-required UX for v1.
2. **Sub-resource discovery**: list which sub-resources a given mod
   overrides (cross-reference mod's archive entries against
   `RelocFile::SubResourceHashes`). Useful for end-users debugging
   "which container is this mod hitting".

`tools/example_mod/build_example_mod.py` builds a canned override
that flips one byte of Mario's FTAttributes; Stage 12 can promote
this into the menu ("Generate Example Mod" button) and provide a
wider set of demo mods showing different override kinds.

---

## Open follow-ups

The Stage 10 follow-up backlog (D, E, F, G, H, I, J, K, L) is in
`docs/refactor_buildtime_reloc_followups.md`. **E is tightly coupled
to Stage 11** — pick it up while doing the byteswap trim, not
separately. The rest can be picked off independently or pivoted to
after Stage 11 lands.

A, B, C are done; their canonical write-ups are in commits
`0ad444f`, `0b81d3e`, `77afbc5`.

---

## Open submodule branches (push when ready for review)

```
git -C torch        push origin ssb64-buildtime-reloc   # 12 commits (Stages 4–7 + 9 + 10 + follow-up A)
git -C libultraship push origin ssb64-buildtime-reloc   # 1 commit (Stage 1)
git -C decomp       push origin port-patches-buildtime-reloc   # 0 commits, harmless
```

The `agent/buildtime-reloc` outer branch is mergeable to `main` today
if the project wants to land Stages 1–10 plus follow-ups A/B/C
standalone — Stage 10 delivers the user-visible mod path (build the
example mod, drop in `mods/`, boot, see the override fire) and the
follow-ups close the highest-impact addressability + correctness gaps.
Stages 11–12 are correctness/UX refinements on top.

---

## Earlier-stage details

Per-stage closeout lives in commit messages on `agent/buildtime-reloc`
— `git log --oneline` for the index, full bodies for substantive
context. The two ADRs above own the Stage 7 and Stage 8 carve-outs.
The plan document (`docs/refactor_buildtime_reloc_plan.md`) has the
per-stage Status notes.
