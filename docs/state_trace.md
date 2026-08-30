# Gameplay state trace (`SSB64_SYNC_TRACE` / `SSB64_SYNC_VERIFY`)

Purpose: prove, per tick, that the VS simulation evolved identically — across two runs on one
machine, across compilers/platforms, and (later) across peers. Replay verification
(`SSB64_REPLAY_PLAY`) only proves the *inputs* were fed identically; this proves the *state*.
It is the oracle every later netcode step (snapshot/restore, SyncTest, rollback) is checked
against.

## What it records

Once per simulated tick, right after `ifCommonBattleUpdateInterfaceAll()` in
`scVSBattleFuncUpdate()` (end of the simulation step, before `syNetReplayUpdate()`),
`syNetSyncRecordTick()` computes twelve FNV-1a 32-bit hashes and writes one text line:

```
# ssb64h v1 tick full rng battle fighters items weapons stage objman input joints camera vars
0 809832C5 53589704 D5403D9E D8CDFA18 9DDCA10B 050C5D1F 41B3C8F1 44A42265 06481D31 3D1D6252 8746A390 CEEABA37
```

| column | covers | gated |
|---|---|---|
| `rng` | `syUtilsRandSeed()` and whether the seed pointer still targets the default seed | yes |
| `battle` | `SCBattleState` incl. every `SCPlayerData` (scores, stocks, stale-move queue), `gFTManager*` counters | yes |
| `fighters` | every non-union scalar of `FTStruct`, physics, `MPCollData` (incl. the TopN translate it points at), timers, all 45 bitfields by name, inputs, `FTComputer`, attack/damage collisions incl. hit records, motion-script cursors, part status, callbacks as non-null bits | yes |
| `items` | `ITStruct` likewise, the item spawner (`gITManagerAppearActor`), random-weight scalars, the Poké Ball roll queue (`gITManagerMonsterData`) | yes |
| `weapons` | `WPStruct` likewise | yes |
| `stage` | `gMPCollisionBounds`, update tic, line/yakumono counts, yakumono speeds and poses | yes |
| `objman` | per gameplay link (1–5): object count and XOR of object keys | yes |
| `full` | fold of the gated columns | yes |
| `input` | the published-input checksum netreplay verifies — recorded for cross-reference | no |
| `joints` | `DObj` pose of every fighter joint (translate/rotate/scale/anim) | no |
| `camera` | `GMCamera` scalars | no |
| `vars` | the per-kind unions (`status_vars`, `passive_vars`, `item_vars`, `weapon_vars`, `GRStruct`) hashed word-wise with pointer slots masked from a generated table (`netsyncvars.inc`) | no |

Rules the hasher follows: no address is ever hashed — a `GObj *` becomes a stable object key
(player slot for fighters, per-match creation serial for items and weapons; `gobj->id` is only
the object *kind*), any other pointer becomes a non-null bit; floats are hashed by bit pattern;
bitfields by name (layout-independent); linked lists are accumulated per slot / per key and
merged order-independently. `joints` and `camera` are kept out of `full` so a divergence there is
attributed rather than folded into `fighters`; `vars` is kept out until layout equality across
builds is demonstrated, because it doubles as a layout probe.

Deliberately not hashed (render/audio only): `attack_matrix`, `colanim`, `afterimage`,
`magnify_pos`, `arrow_gobj`, sfx handles/ids, `display_mode`, fog/shade colors. Not yet covered:
map vertex data, effects.

## Usage

```
SSB64_SYNC_TRACE=<path.ssb64h>     write the trace for the next VS match
SSB64_SYNC_VERIFY=<path.ssb64h>    load a trace and compare every tick as the match runs
SSB64_SYNC_DUMP_TICK=<n>           log per-fighter group hashes and hit records at ticks n..n+2
```

Log lines (`SSB64 SyncTrace:`): `sizeof ...` layout probe at startup, `trace start/wrote`,
`verify start`, `FIRST DIVERGENCE tick=N column=<name> expected=… actual=…` once per column
(with `(not gated)` where applicable), and the summary
`verify ticks=… compared=… divergences=… first_tick=… first_mask=… result=PASS|FAIL`.

With `SSB64_RIG_EXIT=1` the replay verdict becomes the exit code: 0 PASS, 1 input FAIL,
2 INCOMPLETE, 3 LOADFAIL (replay or verify trace could not be loaded), **4 DESYNC** (inputs
replayed identically but a gated state column diverged, or the verify trace was shorter than the
run). A diverged trace wins over INCOMPLETE.

Offline: `tools/tracediff.py a.ssb64h b.ssb64h` (in the BattleShip-dev rig) reports the first
divergent tick per column and exits 1 on any gated difference.

## Typical loop

1. Record a replay (`SSB64_REPLAY_RECORD`) or generate one from metadata.
2. Replay it with `SSB64_SYNC_TRACE` on build A.
3. Replay it with `SSB64_SYNC_VERIFY=<that trace>` on build B (other compiler, other machine,
   or the same build again). Exit 0 = identical simulation; exit 4 + the `FIRST DIVERGENCE`
   line names the tick and subsystem.
4. Re-run build B with `SSB64_SYNC_DUMP_TICK=<tick>` on both sides and diff the `SyncDump`
   lines to find the field.

## Results so far (2026-08-29)

- Same build, two runs: identical on all 12 columns (MSVC and clang).
- MSVC 14.43 (Windows) vs clang 18 (Linux): identical on **all 12 columns** — gated and ungated,
  including `vars` — on the 4-file corpus (~10k ticks) and on an 18-file sweep covering all 9 VS
  stages, all 12 fighters, items on, random human inputs (18 × 1800 ticks). `vars` matching means
  the per-kind unions have the same word layout under both compilers; `FTCommandVars` (below) is
  the known outlier and is hashed by field name.
- Sensitivity control: replaying a recording with one button flipped at tick 900 against the
  clean trace reports `FIRST DIVERGENCE tick=900 column=fighters`.
- Layout probe: the two compilers lay out several structs differently (`FTStruct` 3768/3760,
  `ITStruct` 1216/1200, `SCBattleState` 520/512, `union FTCommandVars` 20/16 — MSVC starts a new
  storage unit when bitfield types change). The simulation still matches because the code
  accesses fields by name; the exception is `FTItemThrowFlags` (see `docs/bugs/`), where the
  `item_throw` bitfield view of `FTCommandVars` matches the script-written `flags` words only
  under the N64's MSB-first allocation — both PC hosts read wrong bits, each differently.
- What the trace cannot see: two hosts that are *consistently* wrong in the same way (or in
  ways a given replay never exercises) still match. The trace proves "same as the other build",
  not "same as the N64"; the bug above was found by reading, and its verification needs the ROM.
- A hashed field written from a draw proc (`is_magnify_show`, `ftDisplayMainProcDisplay()`)
  produced isolated single-tick `fighters` divergences between hosts when the renderer dropped
  a draw pass (`syTaskmanRunTask` skips `task_draw` when no gfx context is free). Rule: fields
  written from any `*ProcDisplay` are never hashed in a gated column.
