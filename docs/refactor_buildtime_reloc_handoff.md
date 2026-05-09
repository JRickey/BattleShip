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
Recent landings (Stage 11 commits in commit-order):

```
ba3098d Stage 11 Phase 5: drop V0/V1/V2 reloc-file readers (follow-up E)
ed056d7 Stage 11 Phase 4: rename portRelocByteSwapBlob → portRelocSeedVertexTrackers
4b77fe4 Stage 11 Phase 3: drop runtime halfswap helper
36c131a Stage 11 Phase 2: drop struct fixup byte-transform bodies
671eb43 Stage 11 Phase 1: drop pass1/pass2 byte-transform helpers
d830ee4 docs: add Stage 11 next-agent kickoff prompt
57e2b52 docs: split follow-up backlog out of handoff, focus handoff on Stage 11
6e43ef0 docs: mark Stage 10 follow-ups B + C done in handoff
0b81d3e port_reloc_regen: add --check-subres v3 sub-resource validator (B)
77afbc5 Sort mods/ scanner entries alphabetically (C)
```

The bit map of `processing_flags` after Stage 11 (unchanged from
Stage 10 — Stage 11 only deleted runtime code that consumed each bit):

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

## Stage 11 closeout note

Stage 11 deleted the following along with all dead-on-v3 byte transforms:

- `port/bridge/lbreloc_byteswap.cpp`: 2,367 → 2,009 LOC. Deletions:
  pass1_swap_u32, scan-time apply_fixup_{vertex,tex_bytes,tex_u16},
  static fixup_{rotate16,bswap32,u16_u8u8}, fixup_torch_already_did_it,
  the kProc<Family>Done constants, and the byte-transform bodies of
  portFixupStruct{U16,U32}, portFixupRawTextureBSWAP32, portFixupSprite,
  portFixupBitmap, portFixupMObjSub, portFixupFTAttributes. Survived:
  every tracker (sStructU16Fixups + cache-invalidation friends), the
  4c deswizzle, portFixupSpriteBitmapData (chain-resolved Bitmap.buf
  bytes are NOT pre-transformed by torch — see follow-up A scope),
  chain_fixup_*, runtime lazy fixup, all diagnostics.
- `port/bridge/lbreloc_bridge.cpp`: dropped portRelocFixupFighterFigatree
  + figatree_reloc_words tracking inside applyInternEntry/applyExternEntry.
  port_aobj_register_halfswapped_range still fires per Stage 7 ADR.
- `port/bridge/lbreloc_byteswap.h`: renamed portRelocByteSwapBlob to
  portRelocSeedVertexTrackers, dropped proc_flags parameter.
- `port/resource/RelocFileFactory.{h,cpp}` + the V0/V1/V2 register
  calls in `port/port.cpp` and `port/test/reloc_harness.cpp` — gone.

The plan-target outcome ("~150 LOC of trackers + sprite codec") was
optimistic. Real residual ~2k LOC because the chain-walk fixup
(chain_fixup_settimg/vertex), runtime lazy fixup
(portRelocFixupVertex/TextureAtRuntime), sprite bitmap data BSWAP +
non-4c TMEM deswizzle (portFixupSpriteBitmapData), and ~550 LOC of
env-gated diagnostics (stage audit / tex log / tex dump) all do real
work or are kept by the Stage 8 ADR. Whether to push more of those
into torch is a future-work decision; the present file is "no dead
code", not "no code".

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
