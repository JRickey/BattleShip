# Reset button / Ctrl+Cmd-R did nothing — "reset" console command never registered

**Status:** FIXED (port + decomp PORT blocks)

**Symptoms:** The ESC-menu Reset button (red ↺) and the Ctrl+R / Cmd-R shortcut did nothing in any scene. With the console open, pressing either printed `> reset` followed by `[LUS] Command not found`.

**Root cause (three layers, found in sequence):**

1. **Command never registered.** `port/gui/Menu.cpp` and libultraship's `Gui.cpp` both `Dispatch("reset")` at the LUS console, but the only commands ever registered were LUS's built-ins (`set/get/help/clear/unbind/bind/bind-toggle`). The dispatch fell through to "Command not found". (SpaghettiKart, the pattern source, has the same dead console dispatch — its Reset *button* works because it bypasses the console and calls a `ProcessReset()` that writes MK64's global `gGamestateNext` state machine. SSB64 has no equivalent per-frame gamestate machine, so an equivalent needed real wiring.)

2. **1P mode's nested scene chain.** First implementation set `gSCManagerSceneData.scene_curr` to the boot scene and called `syTaskmanSetLoadScene()` — the same thing every scene's own exit path does. That works for top-level scenes, but `sc1PManagerUpdateScene` chains 1P sub-scenes back-to-back and rewrites `scene_curr` itself between them. A mid-match reset in 1P Game ended the battle task, then the manager chained into the next stage intro against half-torn-down battle state → SIGSEGV in `ftManagerSetupFilesAllKind` (NULL + 0x30). Grep-audit result: `scmanager.c` and `sc1pmanager.c` are the ONLY two files in the tree that chain scenes via `*StartScene()`.

3. **Battle-end interpretation.** A VS-match reset skipped to Sudden Death: `scVSBattleSetScoreCheckSuddenDeath` read the aborted match as "time expired, tied score". The game already has native abort semantics for this — `gSCManagerSceneData.is_reset`, set by the original A+B+R+Z combo (`ifcommon.c`), honored by `scvsbattle.c` (skips sudden death), the VS results screen, and the 1P bonus/challenger flows, and cleared at every battle start.

**Fix:**

- `port/port.cpp` registers `reset` at console init; the handler calls `portSCManagerRequestReset()` (decomp-side, `#ifdef PORT` — port C++ can't include decomp headers, and mirroring `SCCommonData`'s layout in the port would invite exactly the layout-drift class documented in `debug_ido_bitfield_layout.md`).
- `scmanager.c portSCManagerRequestReset()`: latches a STICKY pending flag + target scene (boot scene `nSCKindOpeningRoom`, or the VS CSS when Boot-to-CSS is active — mirroring the port's boot flow), sets the native `is_reset` flag, runs the same audio wind-down every scene exit uses (`func_800266A0_272A0`), writes `scene_prev/curr`, and calls `syTaskmanSetLoadScene()`.
- `scmanager.c portSCManagerConsumeReset()`: if pending, re-applies the target (undoing any nested-manager `scene_curr` clobber), clears the flag, returns TRUE. Consumed at the top of `scManagerRunLoop`'s dispatch (top-level case + stale-flag guarantee) and by `sc1pmanager.c`'s `SC1P_PORT_RESET_BAIL()` after **every** of its 12 sub-scene calls, which unwinds straight back to `scManagerRunLoop` (whose existing scene-boundary GObj sweep then runs as for any transition).

Playtested: reset from title, menus, mid-VS-match (no sudden death), mid-1P-match (no crash), 1P bonus stage; all land at the opening scene with audio stopped.

**Audit hook:** Anything that force-ends a scene must (a) survive the two scene-chaining managers (`scmanager.c`, `sc1pmanager.c` — re-grep `StartScene();` if more appear) and (b) speak `is_reset`, the game's own match-abort flag, so battle-end interpreters don't misread the abort. A scene_curr write alone is not a transition request.
