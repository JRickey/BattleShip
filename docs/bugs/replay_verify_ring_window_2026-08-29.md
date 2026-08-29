# Replay playback verify could never pass for replays longer than 720 frames

**Date:** 2026-08-29
**Status:** FIXED (decomp `port-patches`, `src/sys/netreplay.c`)
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
walks the full 21600-entry replay stream. The two sides therefore hash
different tick ranges for any replay longer than the ring.

Confirmed arithmetically: `0x866CBB05` is exactly the FNV-1a fold of
`(player, tick, buttons=0, stick=0,0)` over ticks `[1080, 1800)` × 4 players;
`0x8A27A615` is the same fold over `[0, 1800)`.

Because the default record length (1800) exceeds the ring (720), the shipped
verify could not pass on any default recording.

## Fix

The playback checksum is now accumulated incrementally, one tick at a time,
from the frames `syNetInputFuncRead()` actually published for that tick
(`syNetInputGetPublishedFrame()`), using the same source-independent
`syNetInputAccumulateInputChecksum()` fold the recorder uses. This never
touches the ring, so replay length is bounded only by
`SYNETINPUT_REPLAY_MAX_FRAMES`. `syNetInputAccumulateInputChecksum` is now
declared in `netinput.h`.

The verify additionally tracks continuity: if a published frame's tick does not
match the expected tick (a skipped update), the result is FAIL with a
`lost continuity at tick=N player=P` line, rather than a silently shorter hash.

New debug env: `SSB64_RIG_EXIT=1` makes the process exit with code 0 (PASS) /
1 (FAIL) immediately after the verify line, so batch runs can gate on the exit
code. It uses `_exit()` deliberately — the render/audio threads are mid-frame
and normal teardown from the game coroutine is unsafe (see the SIGTERM
`terminate called without an active exception` abort) — after `fflush(NULL)`.

## Verification

Synthetic 1800-frame CPU-vs-CPU replay (Mario vs Fox, Dream Land, seed 1, all
slots neutral input):

- Before: `result=FAIL` on MSVC 14.43 and clang 18.
- After: `result=PASS continuous=1`, exit code 0 on both, ~31 s wall time.
- Negative control: same file with `header.input_checksum` overwritten →
  `result=FAIL`, exit code 1.

## Audit hook

Any end-of-run validation that re-reads a bounded ring is only valid for runs
shorter than the ring. Prefer accumulating as data flows past.
