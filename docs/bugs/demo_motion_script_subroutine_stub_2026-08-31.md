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

## Follow-up / audit hook

- `docs/fighter_intro_animation_investigation.md` Lead 1 ("Yoshi's mouth doesn't open / Fox's eye blink" in the opening movie) chased a matanim/`AObjEvent32` hypothesis; opening statuses index the same `submotion` scripts, so re-test the opening movie before spending more time there.
- Audit hook: any compiled-C data table that emits `0` where the N64 build stored a pointer needs a port-side link pass — search for `#ifdef PORT` placeholder macros in `ftdef.h`-style headers. `PORT_RESOLVE(0)` is silent by design (NULL token); treat an unexpectedly-NULL script/jump target as a stubbed operand until proven otherwise.
