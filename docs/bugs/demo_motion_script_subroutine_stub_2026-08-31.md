# Demo motion scripts: Subroutine/Goto operands stubbed to 0 (issue #240)

**Date:** 2026-08-31
**Status:** FIXED (decomp PORT block + port bridge)
**Symptom (reported):** Jigglypuff's eyes never blink on the character select screen or victory screens; works in the original game / mupen64plus.

## Root cause

Character eye-blink / mouth animation on the demo poses (CSS idle & lock-in, victory/lose results, opening movie) is driven by motion scripts in `decomp/src/sc/scsubsys/scsubsysdata*.c` — compiled-C `s32` arrays of the same opcode stream format as the relocData mainmotion binaries. The blink itself is a shared 4-step `nFTMotionEventSetTexturePartID` subroutine (eye texture 4 → 5 → 4 → 0), entered via `nFTMotionEventSubroutine` and looped via `nFTMotionEventGoto`.

On N64 the Subroutine/Goto operand word held a raw overlay address. On LP64 a pointer can't live in an `s32` slot (an `ADDR64` reloc would spill into adjacent data), so `ftMotionCommandSubroutineS2`/`GotoS2` in `decomp/src/ft/ftdef.h` compile to a `0` placeholder under `PORT`. `PORT_RESOLVE(0)` returns NULL **silently** (token 0 is the NULL token, no stale-token log), and `ftMainParseMotionEvent`'s event loop kills the script at its `p_script == NULL` guard.

Net effect: every demo script died at its first `Subroutine`/`Goto`. 62 call sites across 8 fighters (mario 13, purin 16, pikachu 9, kirby 8, link/luigi/ness/yoshi 4 each). Jigglypuff was just the most visible — largest eye textures, blinks in all six demo poses. Fox/Samus/DK/Falcon have no demo-script subroutines (they don't blink on CSS in vanilla either), which is why the class went unnoticed.

**Why matches were unaffected:** in-match statuses read the *same* blink subroutines from the relocData binary (`232_PurinMainMotion` etc. inside `BattleShip.o2r`), where the operands are genuine reloc tokens produced by the bridge. Only the compiled-C delivery path of the identical data was broken.

## Fix

Same pattern as `gmColScriptsLinkRelocTargets()` (`gmcolscripts.c`, the collision-script equivalent):

- `decomp/src/sc/scsubsys/scsubsysportfix.c` (new, PORT-only): a generated table of 62 `{ script, operand_index, target }` links; `scSubsysMotionLinkRelocTargets()` writes `portRelocRegisterPointer(target)` into each operand slot. Each entry validates that the preceding word's opcode is `Subroutine`/`Goto` before patching, so future `scsubsysdata*.c` edits that shift word indices produce a log line instead of a mispatched script. The table was generated mechanically from the `ftMotionCommandSubroutine`/`Goto` call sites (each macro spans two `s32` words: opcode, operand).
- `port/bridge/lbreloc_bridge.cpp`: `lbRelocInitSetup` calls it right after `gmColScriptsLinkRelocTargets()` (every scene init; overwrite-idempotent).
- `decomp/src/ft/ftmain.c`: the Subroutine/Goto parse cases now log (once per run each) when a token resolves to NULL — the silence of `PORT_RESOLVE(0)` is what let this survive multiple animation investigations.

## Verification

- Boot log shows `scSubsysMotionLinkRelocTargets: 62 demo-script Subroutine/Goto targets linked` with zero stale-table or NULL-resolve warnings; `SSB64_START_SCENE=16` (VS CSS) runs clean.
- Expected visible behavior (manual check): Jigglypuff blinks on the CSS idle pose (90/10/80-frame cadence), 5 blinks over the lock-in "Selected" pose, blinks on Win1/Win2/Win3/Win4/Lose results poses; Mario, Pikachu, Kirby, Link, Luigi, Ness blink and Yoshi's mouth animates on their demo poses.

## Stress-test results (one unreproduced flake, then an extended clean campaign)

During initial verification, 1 of 16 attract-loop boot runs with the fix active crashed in scene 30 (OpeningMario): SIGSEGV in `gfx_check_image_signature` on a `G_SETTIMG` whose operand was the untranslated N64-style value `0x0fcb0520` (paired `G_SETTILE` says IA16), inside a game-built DL. The last game-side log line was an in-match-style status (`status=0xd0 motion=183`) — the mainmotion/relocData path, which runs with or without this fix. Its signature matches the previously documented intro texture-corruption family; core `117329` (2026-08-31) holds the reference. Those early runs also shared `~/.local/share/BattleShip/ssb64.log` with another session's boot tests, so their bookkeeping is suspect.

An extended isolated campaign (per-run `XDG_DATA_HOME`, fix-on/fix-off interleaved run-for-run) then came back **60/60 clean**:
- 40× direct `SSB64_START_SCENE=30` boots chaining through all eight fighter opening scenes + montage (20 per config);
- 8× synthesized-replay runs (`SSB64_REPLAY_PLAY`) driving an unattended VS battle to the **results screen**, where the revived Win/Lose pose scripts run — both Mario/Jigglypuff matchups, both configs; a fix-off control run logged the expected `Subroutine token 00000000 resolved to NULL` there, proving these runs genuinely exercise the revived scripts;
- 12× 120 s attract-loop soaks (6 per config).

ASan (clang, `detect_leaks=0`): 9 fast/soak runs + 2×480 s battle→results replay runs with the fix on — **zero reports**. (An earlier ASan replay run flagged a global-buffer-overflow in `ftParamGetCommonKnockback`; that was the synthesized replay's zeroed handicap indexing `dFTCommonDataHandicapTable[-1]`, an out-of-domain input, not this fix — now clamped in `syNetReplayApplyBattleMetadata`.)

If the scene-30 flake ever recurs: `SSB64_NO_MOTIONFIX=1` disables only this fix for bisecting.

## Follow-up / audit hook

- `docs/fighter_intro_animation_investigation.md` Lead 1 ("Yoshi's mouth doesn't open / Fox's eye blink" in the opening movie) chased a matanim/`AObjEvent32` hypothesis; opening statuses index the same `submotion` scripts, so re-test the opening movie before spending more time there.
- Audit hook: any compiled-C data table that emits `0` where the N64 build stored a pointer needs a port-side link pass — search for `#ifdef PORT` placeholder macros in `ftdef.h`-style headers. `PORT_RESOLVE(0)` is silent by design (NULL token); treat an unexpectedly-NULL script/jump target as a stubbed operand until proven otherwise.
