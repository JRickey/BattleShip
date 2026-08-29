# Netcode Agent Rules

## Purpose

This file tracks netcode-specific guidance for future agent sessions. It is a project handoff document, not a Cursor rules file, and it should not override the repository's main developer guidance.

Use this document when working on rollback netcode, netplay input, saved input replay, prediction, session ownership, or interactions between those systems and the core game loop.

## Current Netcode Goal

Build rollback netcode in phases:

1. Resolve local, remote, predicted, and saved inputs into a deterministic per-tick input stream.
2. Publish resolved inputs through existing game-facing controller globals.
3. Add deterministic validation for replayed inputs.
4. Add debug P2P networking and remote input delivery.
5. Add state snapshot/restore.

## Implemented So Far

- `src/sys/netinput.h` defines `SYNetInputFrame`, `SYNetInputSource`, input history APIs, remote/saved input staging APIs, VS session lifecycle APIs, replay metadata scaffolding, and the VS controller callback.
- `src/sys/netinput.c` owns per-player input source state, resolved input history, remote confirmed input history, saved input history, full-match replay frame storage, prediction fallback, VS-local ticks, recording counters, replay metadata, and publishing into `gSYControllerDevices`.
- `src/sys/netreplay.c` and `src/sys/netreplay.h` implement a debug VS replay file runner for explicit metadata plus `SYNetInputFrame` streams.
- `src/sys/netpeer.c` and `src/sys/netpeer.h` implement the debug UDP P2P transport, explicit input packet serialization, packet checksums, and local/remote player slot ownership.
- `src/sys/netpeer.c` can bootstrap matching VS battle metadata with `MATCH_CONFIG`, pre-VS `READY` / `START`, and a shared RNG seed.
- `src/sys/netpeer.c` now runs a bootstrap-only in-battle start barrier with `BATTLE_READY` / `BATTLE_START` before netinput advances from tick 0.
- `src/sys/netpeer.c` exposes a bootstrap-only VS execution gate so `scVSBattleFuncUpdate()` can hold battle/interface updates until both peers have crossed the start barrier.
- `src/sys/netpeer.c` now logs rolling validation lines prefixed `SSB64 NetSync:` that pair sliding input-history checksum windows (`syNetInputGetHistoryInputValueChecksumWindow`), cumulative remote-input fingerprints, UDP sequence continuity counters, newest observed remote frame bundle ticks, and a narrow deterministic fighter hash (`syNetSyncHashBattleFighters()` in `src/sys/netsync.c`).
- `src/sc/sccommon/scvsbattle.c` routes VS battle input through `syNetInputFuncRead`.
- `src/sc/sccommon/scvsbattle.c` calls `syNetInputStartVSSession()` at VS match start so repeated matches do not inherit previous input history or slot ownership.
- `src/sc/sccommon/scvsbattle.c` calls `syNetReplayStartVSSession()` and `syNetReplayUpdate()` for debug record/playback.
- `src/sc/sccommon/scvsbattle.c` calls `syNetPeerStartVSSession()`, `syNetPeerUpdate()`, and `syNetPeerStopVSSession()` for debug P2P sessions.
- `src/sc/scmanager.c` calls `syNetPeerInitDebugEnv()` during port startup, next to replay debug env setup.
- `src/sc/sc1pmode/sc1pgame.c`, `src/sc/sc1pmode/sc1ptrainingmode.c`, and `src/sc/sc1pmode/sc1pbonusstage.c` remain on `syControllerFuncRead`.
- `docs/netplay_architecture.md` documents the current input architecture.

## Architecture Rules

- Keep `src/sys/controller.c` focused on local hardware input and N64-style controller behavior.
- Put new netplay-only input functions in `src/sys/netinput.c` and declarations in `src/sys/netinput.h`.
- Put deterministic gameplay-state hashing needed for rollback or netplay parity near `netsync`/netpeer rather than scattering ad-hoc checks through fighter code unless a subsystem truly owns the invariant.
- Keep reusable netplay execution readiness checks in `src/sys/netpeer.c`; VS scenes should call a query such as `syNetPeerCheckBattleExecutionReady()` rather than duplicating bootstrap state checks.
- Do not add remote, predicted, or saved input metadata to `SYController`.
- Resolve input source first, then publish the selected result into `gSYControllerDevices[player]`.
- Existing battle/fighter code should continue reading `SYController` fields without knowing the input source.
- Treat `gSYControllerDevices[player]` as the compatibility boundary, not as the source of truth for rollback history.
- Keep netinput integration scoped to VS battle unless the project explicitly decides to support rollback in another mode.
- Keep replay files explicit. Do not serialize raw `SCBattleState`; serialize named metadata fields and rebuild battle state on load.
- Do not involve framebuffer contents in rollback input design. Framebuffers are render output, not simulation input state.

## Match And Session Lifecycle

- Reset net input state at the start of every VS match/session before configuring player slots.
- Clear saved input history unless the session is intentionally a replay.
- Clear confirmed remote input history and last confirmed prediction seed at match start.
- Clear last published input so `button_tap` and `button_release` derive from the new match only.
- Configure player slot ownership after reset: local, remote confirmed/predicted, saved, or future CPU/spectator modes.
- Use the VS-local netinput tick for input frame history; do not key rollback input history directly on `dSYTaskmanUpdateCount`.
- Debug replay record/playback is controlled by `SSB64_REPLAY_RECORD`, `SSB64_REPLAY_PLAY`, and optional `SSB64_REPLAY_RECORD_FRAMES`. `SSB64_RIG_EXIT=1` turns the playback verify verdict into the process exit code (0 PASS / 1 FAIL / 2 INCOMPLETE when the match ends before the replay does).
- Debug P2P is controlled by `SSB64_NETPLAY`, `SSB64_NETPLAY_LOCAL_PLAYER`, `SSB64_NETPLAY_REMOTE_PLAYER`, `SSB64_NETPLAY_BIND`, `SSB64_NETPLAY_PEER`, `SSB64_NETPLAY_DELAY`, `SSB64_NETPLAY_SESSION`, `SSB64_NETPLAY_BOOTSTRAP`, `SSB64_NETPLAY_HOST`, and `SSB64_NETPLAY_SEED`.
- The start barrier should only gate bootstrap P2P sessions. Non-netplay, replay, and manual P2P sessions should return true from `syNetPeerCheckStartBarrierReleased()` and advance netinput normally.
- The VS execution gate should only hold bootstrap P2P sessions. Non-netplay, replay, and manual P2P sessions should return true from `syNetPeerCheckBattleExecutionReady()` and run scene updates normally.

## Determinism Rules

- Canonical netplay input is held buttons plus stick X/Y for a player and tick.
- `button_tap`, `button_release`, and `button_update` are derived fields when publishing to `SYController`.
- Prediction should be deterministic: repeat last confirmed input, or neutral if no confirmed input exists.
- Saved input replay should feed the same canonical frame stream into the same publish path as remote/local input.
- Saved VS replay files should be deterministic engine re-runs using metadata plus `SYNetInputFrame` streams, not video captures.
- Use source-independent input checksums for replay validation so recorded local inputs and replayed saved inputs compare by tick/buttons/stick, not by source tag.
- Any future state hash should be separate from the input checksum and should cover gameplay state, not framebuffer pixels.

## Current Risks

- `SYNETINPUT_HISTORY_LENGTH` is fixed at 720 frames. Revisit this when delay, max rollback window, replay length, and memory ownership are better defined.
- `button_update` currently mirrors newly pressed buttons. Confirm whether any battle path requires the original controller repeat behavior before relying on it for menus or non-battle scenes.
- The current replay serializer is a debug runner, not a final user-facing replay browser or stable long-term file contract.
- Bootstrap P2P now aligns netinput tick 0 and holds VS execution until the start barrier releases, but tick drift after start still requires comparing **NetSync** checksum windows versus **fighter hashes** logged by `netsync`.
- Duplicate log lines expose transport health: `gap`/`dup`/`ooo` flag sequence regressions independently of staged frame counters because redundant frame bundles hide single packet loss.
- One-way packet diagnosis is still manual. Keep logging role, local/remote players, tick, staged frames, highest remote tick, dropped packets, late frames, and barrier state when changing netpeer.
- Narrow gameplay/state hashes exist (`src/sys/netsync.c`), but snapshot/rollback coverage is intentionally shallow until input parity is nailed down.
- No rollback snapshot/restore API exists yet.
- No STUN/TURN, NAT traversal, relay fallback, matchmaking, or final session owner model exists yet.

## Desync triage playbook

When investigating bootstrap P2P mismatches:

1. Compare `SSB64 NetSync` lines at the **same simulator tick cadence**. Note the earliest tick window where combined (`all`) or any per-slot (`p0..p3`) checksum disagrees relative to `hist_win`.
2. If input checksums diverge first, reconcile packet payloads, redundancy, deserialization, staging order, and prediction before touching rollback pacing.
3. If input checksums agree but **`figh`** diverges, widen the fighter hash footprint (RNG, collision, weapons, hazards) deterministically rather than blaming UDP blindly.
4. If both traces stay aligned subjective drift likely lives in pacing, presentation ordering, or non-sim telemetry — collect additional tick barriers after locking simulation hashes.

When editing `src/sys/netpeer.c`:

- Maintain explicit big-endian serializers and checklist coverage for checksum fields (currently includes `magic`, wire version `2`, `session_id`, `ack_tick`, `packet_seq`, local player ids, redundant frame payloads, and rolling stats).
- When adding sibling sources under `src/sys/`, re-run CMake so `file(GLOB_RECURSE)` picks them up (`cmake -B build`).

## Suggested Next Steps

- Narrow or broaden `syNetSyncHashBattleFighters()` once desync hotspots are identified via NetSync divergence reports.
- Design the final VS-facing menu flow: Local VS, Netplay, and Replays as separate user-facing entries.
- Define save-state boundaries for rollback after replayed input determinism is validated.

## Verification Expectations

- Build after touching netplay input or battle controller callbacks.
- Test local device input in VS battle after routing through `syNetInputFuncRead`.
- Test that menus, 1P Game, Training Mode, Bonus 1 Practice, and Bonus 2 Practice still use `syControllerFuncRead`.
- For replay work, run from `build/`, record with `SSB64_REPLAY_RECORD=/tmp/test.ssb64r`, replay with `SSB64_REPLAY_PLAY=/tmp/test.ssb64r`, compare the logged input checksum first, then add stronger gameplay-state validation. The playback checksum is accumulated inside `syNetInputFuncRead()` as ticks advance (`syNetInputGetPublishedInputChecksum()`) — never re-read `sSYNetInputHistory` at the end, it is a 720-entry ring (`docs/bugs/replay_verify_ring_window_2026-08-29.md`).
- For manual P2P, run two instances from `build/` with reciprocal `SSB64_NETPLAY_BIND` / `SSB64_NETPLAY_PEER` values and no bootstrap; confirm remote inputs still stage.
- For bootstrap P2P, grep `SSB64 NetSync` / `NetPeer:` / `execution begin`; confirm input windows and fighter hashes evolve identically minute-to-minute unless you expect divergence at the reproduced stress gesture.

## Packaging Note

`scripts/package-linux.sh` currently includes host DT_RELR handling for AppImage packaging. If Linux AppImage generation stops before creating `dist/BattleShip.AppDir/AppRun`, inspect the DT_RELR probe and avoid early-exit pipelines under `set -euo pipefail`.
