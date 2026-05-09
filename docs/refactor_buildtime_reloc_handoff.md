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
