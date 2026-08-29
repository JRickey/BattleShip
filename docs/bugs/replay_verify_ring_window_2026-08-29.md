# Replay playback verify could never pass for replays longer than 720 frames

**Date:** 2026-08-29
**Status:** FIXED (decomp `port-patches`: `src/sys/netinput.c`, `src/sys/netinput.h`, `src/sys/netreplay.c`; BattleShip: `port/port_log.c`, `port/port_log.h`)
**Class:** verification harness defect (false negative) — the oracle, not the game

## Symptom

Every `SSB64_REPLAY_PLAY` run of a replay recorded with the default
`SSB64_REPLAY_RECORD_FRAMES=1800` logged

```
SSB64 Replay: playback verify frames=1800 expected=0x8A27A615 actual=0x866CBB05 result=FAIL
```

even though the inputs were replayed exactly. The same file produced the same
`actual` value on Windows/MSVC and Linux/clang, so it was not a determinism
problem.

## Root cause

`syNetReplayUpdate()` computed the playback side of the comparison with
`syNetInputGetHistoryInputChecksum(frame_count)`, which walks
`sSYNetInputHistory` — the resolved-input ring used for rollback, sized
`SYNETINPUT_HISTORY_LENGTH = 720` and indexed by `tick % 720`. Reads validate
the stored `tick`, so at verify time (tick ≥ 1800) only the entries for ticks
1080–1799 still match and contribute; ticks 0–1079 were overwritten and are
silently skipped. The recorded side, `syNetInputGetReplayInputChecksum()`,
walks the recorded replay stream (`sSYNetInputRecordedFrameCount` entries). The two sides therefore hash
different tick ranges for any replay longer than the ring.

Confirmed arithmetically: `0x866CBB05` is exactly the FNV-1a fold of
`(player, tick, buttons=0, stick=0,0)` over ticks `[1080, 1800)` × 4 players;
`0x8A27A615` is the same fold over `[0, 1800)`.

Because the default record length (1800) exceeds the ring (720), the shipped
verify could not pass on any default recording.

## Fix

The playback checksum is now accumulated inside `syNetInputFuncRead()` at the exact
point the VS tick is advanced, over the frames just published for every slot, with
the same source-independent `syNetInputAccumulateInputChecksum()` fold the recorder
uses. Because it is gated by the same `sSYNetInputTick++` decision, a tick that is
re-published while the bootstrap P2P start barrier holds is counted once — exactly
as the recorder's `syNetInputSetReplayFrame()` overwrite makes it — so recorder and
verifier hash the same tick set by construction. Playback sets
`syNetInputSetPublishedChecksumLimit(frame_count)` so the value is frozen at exactly
`frame_count` advanced ticks even if `syNetReplayUpdate()` is skipped for a frame
behind the VS execution gate. The ring is never read; replay length is bounded only
by `SYNETINPUT_REPLAY_MAX_FRAMES`.

The now-unused `syNetInputGetHistoryInputChecksum()` is deleted (separate commit):
it silently produced a partial hash for any `frame_count` above the ring size, which
is precisely the trap this bug was.

If the match ends before the replay stream does (a synthetic or truncated file),
`syNetReplayFinishVSSession()` now logs `playback ended early ... result=INCOMPLETE`
instead of leaving the verdict unreported.

New debug env `SSB64_RIG_EXIT=1` (parsed with `atoi`, like the other replay env vars)
makes the process exit right after the verdict — 0 PASS, 1 FAIL, 2 INCOMPLETE, and
3 LOADFAIL when `SSB64_REPLAY_PLAY` names a file that fails to open or validate
(nothing would ever arm playback, so a batch run would otherwise sit at the title
screen forever) — so batch runs can gate on the exit code. The loader also rejects
`frame_count == 0`: a zero-frame file used to verify PASS immediately. It calls a new `port_exit_process()`
(`port/port_log.c`): flush the log and all stdio streams, then terminate
immediately — `_exit()` on POSIX, `TerminateProcess()` on Windows. Normal teardown
from the game coroutine is not safe (render/audio threads are mid-frame), and on
Windows even `_exit()` is not enough: it runs `ExitProcess`, which kills the other
threads and then runs every DLL's `DLL_PROCESS_DETACH` on the calling thread —
observed as `STATUS_FAIL_FAST_EXCEPTION` (0xC0000602) instead of exit code 2 when
the INCOMPLETE path fired at VS scene teardown. `TerminateProcess` skips all of
that. `port_log` writes through a buffered `FILE*` and does not flush per call,
which is why the flush is explicit.

Latent, not introduced here: the writer emits zeroed `is_valid=0` frames for gaps in
the recorded stream, while the loader's `syNetInputSetReplayFrame()` forces
`is_valid=TRUE`, so a gap would be skipped by the recorded hash and included by
playback. Nothing produces gaps today.

## Verification

Synthetic 1800-frame CPU-vs-CPU replay (Mario vs Fox, Dream Land, seed 1, all
slots neutral input):

- Before: `result=FAIL` on MSVC 14.43 and clang 18.
- After: `result=PASS`, exit code 0 on both, ~31 s wall time.
- Same with a human slot driven by 1800 frames of pseudo-random held inputs
  through the Saved path: `result=PASS` on both.
- Negative controls: `header.input_checksum` overwritten → `result=FAIL`, exit 1;
  one button flipped in one frame after the checksum was computed →
  `result=FAIL`, exit 1.
- Replay longer than the match (7200 frames, 1-minute time rule) →
  `result=INCOMPLETE`, exit 2.
- `SSB64_REPLAY_PLAY` pointing at a missing file, and at a well-formed header with
  `frame_count = 0` → `result=LOADFAIL`, exit 3 (both compilers). These two, the
  loader check, and the Status file list came out of a review fan-out over the
  submitted patch (GLM 5.3 Flash × 24 samples via OpenRouter, claims hand-verified).

Live round trip: a match recorded with `SSB64_REPLAY_RECORD` on the MSVC build
(human Fox vs three CPUs, Kongo Jungle, 2 stocks, 1800 frames, P1 non-neutral on
1067 of 1800 frames, `checksum=0x9E7DCC4E`) replays `result=PASS`, exit 0, on
both MSVC and clang; the same file with one button flipped at tick 900 →
`result=FAIL`, exit 1. Every other file above was generated from metadata.

## Audit hook

Any end-of-run validation that re-reads a bounded ring is only valid for runs
shorter than the ring. Prefer accumulating as data flows past.
