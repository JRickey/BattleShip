# Android first-run: Torch "succeeds" without writing BattleShip.o2r (issue #259)

**Date:** 2026-08-31
**Status:** FIXED (port bridge + BootActivity + torch); needs on-device verification
**Symptom (reported):** Fresh APK install, user picks a ROM, boot fails with `[Phase 4.4] Torch returned success but BattleShip.o2r is not in /storage/emulated/0/Android/data/com.jrickey.battleship/files`.

## Root cause

Not a path mismatch — Torch writes exactly where Phase 4.4 checks (`gDestinationDirectory / config["output"]["binary"]` = `externalFilesDir/BattleShip.o2r`).

`Companion::Process()` has four log-and-`return;` bail-outs that throw nothing: config.yml missing, **ROM SHA-1 not in config.yml** (the overwhelmingly likely user trigger — v1.1/iQue/trimmed/byteswapped/headered dumps), no config node for the hash, bad `gbi:` value. The JNI bridge (`port/android_torch_bridge.cpp`) mapped only *exceptions* to non-zero codes and returned 0 unconditionally otherwise — so an unrecognized ROM reported success with no archive on disk.

Compounding it, Torch's spdlog diagnostics (including `ROM not recognized. Got SHA-1: …` plus the supported-hash list) went to spdlog's default **stdout** sink, which Android discards for app processes — the one message explaining the failure was invisible even in logcat.

Desktop never hits this because the ROM is the developer's known-good baserom and `BattleShip.o2r` is a custom-command `OUTPUT`, so a missing file fails the build loudly.

## Fix

- `port/android_torch_bridge.cpp`: after `Init()`, capture `Companion::GetOutputPath()` — empty means `Process()` bailed before configuring output → **rc 3**; non-empty but not on disk → **rc 4**. Also routes Torch's spdlog default logger to a logcat `android_sink` (tag `ssb64.torch`) before extraction, and fixes the stale `src_dir` doc comment (`getExternalFilesDir(null)`, not `filesDir/torch_data`).
- `BootActivity.extractFromAbsolutePath`: rc 3 → "That ROM isn't a supported dump…" and **re-shows the ROM picker** (recoverable); rc 4 → "free up storage"; the old Phase-4.4 `haveExtractedRom()` branch is now a last-resort assertion.
- `torch` (`src/archive/ZWrapper.cpp`, `src/Companion.cpp`): `ZWrapper::Close()` owns the ofstream and returns non-zero on failed open / short write (miniz_cpp's `save(path)` never checks stream state — an ENOSPC half-written archive succeeded silently on every platform); both `Close()` call sites throw on failure.

## Verification

- Desktop torch rebuilt and re-ran a full `o2r` extraction (same 12,114,884-byte archive) — the hardened writer doesn't regress the good path.
- Android side compiles only under the NDK — needs a CI/on-device build; then repro by picking a v1.1 or byteswapped dump: expect the "not a supported dump" message + picker again, and the SHA-1 line in `logcat -s ssb64.torch torch`.

## Audit hook

A "returned success but output missing" report means a callee with log-and-return error paths behind an exceptions-only error contract. Check for silent `return;`s before trusting the return code, and check whether the callee's logging sink is even visible on the platform (Android discards stdout).
