# Build-time reloc refactor — Stage 12 handoff (2026-05-09, Stage 11 closed)

You are picking up the build-time reloc refactor described in
`docs/refactor_buildtime_reloc_plan.md`. Stages 1–11 plus Stage 10
follow-ups A (chain-resolved pixel-buffer enumeration), B (drift-gate
sub-resource validator), C (mods-scanner determinism), and E (V0/V1/V2
reader drop) have landed on `agent/buildtime-reloc`. This handoff is the
delta on top of the plan for Stage 12 — productizing the mods/ scanner.

**Read the plan first.** This handoff covers what's specific to
Stage 12; it does not re-explain the architecture.

---

## Status entering this session

Branch HEAD on `agent/buildtime-reloc`: see `git log --oneline -10`.
Recent landings (Stage 11 phased commits + Phase 2 revert):

```
b28352b Revert "Stage 11 Phase 2: drop struct fixup byte-transform bodies"
05b8fd0 docs: add Stage 12 next-agent kickoff prompt
67c8b64 docs: close Stage 11 — pivot handoff to Stage 12, mark follow-up E done
ba3098d Stage 11 Phase 5: drop V0/V1/V2 reloc-file readers (follow-up E)
ed056d7 Stage 11 Phase 4: rename portRelocByteSwapBlob → portRelocSeedVertexTrackers
4b77fe4 Stage 11 Phase 3: drop runtime halfswap helper
36c131a Stage 11 Phase 2: drop struct fixup byte-transform bodies   ← reverted
671eb43 Stage 11 Phase 1: drop pass1/pass2 byte-transform helpers
```

The bit map of `processing_flags` after Stage 11 (unchanged from
Stage 10 — Stage 11 only deleted runtime code that consumed each bit):

| bit  | name                          | stage     | status                   |
|------|-------------------------------|-----------|--------------------------|
| 0    | `PROC_PASS1_BSWAP_DONE`       | 4         | done — runtime helper deleted in P1 |
| 1    | `PROC_PASS2_DONE`             | 5         | done — runtime byte transforms deleted in P1 |
| 2    | `PROC_STRUCT_U16_DONE`        | 6d        | done — runtime fallback STAYS (catalog incomplete) |
| 3    | `PROC_STRUCT_U32_DONE`        | 6d        | done — runtime fallback STAYS (catalog incomplete) |
| 4    | `PROC_SPRITE_DONE`            | 6d        | done — runtime fallback STAYS (catalog incomplete) |
| 5    | `PROC_BITMAP_DONE`            | 6d        | done — runtime fallback STAYS (catalog incomplete) |
| 6    | `PROC_MOBJSUB_DONE`           | 6d        | done — runtime fallback STAYS (catalog incomplete) |
| 7    | `PROC_FTATTRIBUTES_DONE`      | 6d        | done — runtime fallback STAYS (catalog incomplete) |
| 8    | `PROC_HALFSWAP_DONE`          | 7         | done — runtime helper deleted in P3 |
| 9    | (reserved-unused)             | —         | AObjEvent ADR carve-out  |
| 10   | `PROC_CHAIN_FLATTENED`        | 9         | done                     |
| 11   | `PROC_SUBRESOURCES_EMITTED`   | 10        | done                     |

**Bits 2–7 are set per-file only when `port/test/struct_fixup_catalog.json`
has an entry for that file** (125 of 2,132 files). The other 94% of files
hit the runtime fallback that Phase 2 was supposed to delete. See
"Phase 2 carve-out" in `docs/refactor_buildtime_reloc_plan.md`.

V0/V1/V2 archive readers were dropped in Stage 11 Phase 5 — only V3
archives load now. Asset/torch coupling already forced re-extraction on
every torch SHA bump, so older archives are de-facto unsupported.

Two ADRs document the Stage 7 / Stage 8 carve-outs:

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

If either gate drifts, **stop and diagnose before any Stage 12 edits.**
Recovery steps live in `docs/refactor_buildtime_reloc_plan.md`
(wipe + re-extract); the asset/torch coupling means any torch SHA
change requires deleting `BattleShip.o2r` + the torch build dir +
re-running `cmake --build build --target ExtractAssets`.

---

## Stage 11 closeout note (Phase 2 reverted)

Stage 11 deleted the following:

- `port/bridge/lbreloc_byteswap.cpp` (Phases 1, 3, 4):
  pass1_swap_u32, scan-time apply_fixup_{vertex,tex_bytes,tex_u16},
  rename portRelocByteSwapBlob → portRelocSeedVertexTrackers + drop
  proc_flags param. **Phase 2 (struct fixup byte-transform bodies)
  reverted in `b28352b`** — all per-family byte-transform bodies and
  the catalog-aware skip gate restored.
- `port/bridge/lbreloc_bridge.cpp` (Phase 3): dropped
  portRelocFixupFighterFigatree + figatree_reloc_words tracking
  inside applyInternEntry/applyExternEntry.
  port_aobj_register_halfswapped_range still fires per Stage 7 ADR.
- `port/bridge/lbreloc_byteswap.h`: renamed portRelocByteSwapBlob to
  portRelocSeedVertexTrackers, dropped proc_flags parameter.
- `port/resource/RelocFileFactory.{h,cpp}` + the V0/V1/V2 register
  calls in `port/port.cpp` and `port/test/reloc_harness.cpp` — gone
  (Phase 5 / follow-up E).

### Why Phase 2 was reverted

The freshly-built post-Phase-5 binary surfaced sprite regressions
during a visual smoke test: Link's sword hit effect corrupted, 1P
stage-clear results screen showing only stray numbers, "GAME SET"
title-card missing letters. Diagnosis:
`port/test/struct_fixup_catalog.json` covers only 125 of 2,132
reloc files — `reloc_scene/SC1PStageClear1`, `SC1PStageClear3`, and
several wallpaper files have **no catalog entries**. Pre-Phase-2 the
runtime helpers used a catalog-aware short-circuit
(`fixup_torch_already_did_it`) and **fell through to a runtime
byte-transform fallback** when the catalog had no entry for a given
offset. Phase 2 deleted that fallback on the premise "torch did it
on every v3 file" — true only for pass1/pass2/halfswap, not for
the per-family struct fixups. Torch's
`ApplyStructFixupsInPlace` only emits transforms (and sets the
matching PROC_*_DONE bit) for catalog entries.

The drift gate was useless here: the harness only runs
`lbRelocLoadAndRelocFile`. portFixupSprite/Bitmap/etc. are called
later from decomp source code (e.g. `lbCommonMakeSObjForGObj`). The
fixtures snapshot pre-portFixup* bytes, so Phase 2 stayed gate-green
through five phases despite breaking real rendering.

**Decision**: revert Phase 2. Reframe the catalog as a build-time
optimization (skip the runtime body when torch already did it for
this specific offset), **not a correctness contract**. Disk struct
bytes are post-pass1 in files torch hasn't catalogued; the runtime
helpers transform them at first access exactly as before. Modders
authoring sprite/bitmap/etc. sub-resource overrides supply
post-pass1 bytes — one consistent format across all files.

For Phase 2 to be safely deletable, the catalog needs to be 100%
complete via static analysis of decomp source — see
`refactor_buildtime_reloc_followups.md` follow-up M. Until then,
the runtime fallback is the source of truth.

---

## Stage 12 — Productize mods/ scanner

Two UX gaps remain in the mod path:

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

The Stage 10 follow-up backlog is in
`docs/refactor_buildtime_reloc_followups.md`. A, B, C, E are done.
D, F, G, H, I, J, K, L remain — pick what fits a session, or focus on
Stage 12. K (mod-author guide) is naturally a Stage 12 deliverable.
**M (static-analysis catalog) is the prerequisite for retrying
Phase 2** — non-trivial; defer until modder demand justifies the
analyzer effort.

---

## Open submodule branches (push when ready for review)

```
git -C torch        push origin ssb64-buildtime-reloc   # 12 commits (Stages 4–7 + 9 + 10 + follow-up A)
git -C libultraship push origin ssb64-buildtime-reloc   # 1 commit (Stage 1)
git -C decomp       push origin port-patches-buildtime-reloc   # 0 commits, harmless
```

The `agent/buildtime-reloc` outer branch is mergeable to `main` today —
Stages 1–11 plus follow-ups A/B/C/E land the user-visible mod path (build
the example mod, drop in `mods/`, boot, see the override fire), close the
highest-impact addressability + correctness gaps, and remove all dead
runtime byteswap code. Stage 12 is UX refinement on top.

---

## Earlier-stage details

Per-stage closeout lives in commit messages on `agent/buildtime-reloc`
— `git log --oneline` for the index, full bodies for substantive
context. The two ADRs above own the Stage 7 and Stage 8 carve-outs.
The plan document (`docs/refactor_buildtime_reloc_plan.md`) has the
per-stage Status notes.
