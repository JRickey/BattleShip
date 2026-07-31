# Residual: null-SETTIMG burst in the opening clash scene (~50% of boots)

**Status:** OPEN — characterized, tripwired, harmless (no crash, no visible
artifact). Left unfixed deliberately; this note is the handoff.

## Symptom

In ~half of all boots (binary per-boot coin flip — 5 affected runs in a
10×30k-frame soak, each affected run identical), the log shows:

- `TextureCache stale-content hit healed: … fmt=3 siz=1 64x64` — exactly 64
  of them, one per frame, starting in the OpeningRoom scene (same arena
  offset every time).
- `ImportTexture: null texture address` + `GfxDpLoadBlock: missing texture
  image` bursts (~6/frame, ~1650/run) starting at **exactly frame 3669**
  (opening clash cinematic, Mario vs Kirby field) and recurring through
  later loops.

Screenshots at 200-frame granularity are pixel-equivalent to clean runs —
the null draw is skipped by the port (on N64 a null image pointer would
sample physical address 0: garbage texels, likely invisible). Exit clean,
no corruption markers, all 30k frames complete.

## What the tripwire captured (armed in `GfxDpLoadBlock`, 3 dumps/run)

- The null SETTIMG lives in a **frame-built DL at `scene_arena+0x5e0`** —
  the first-drawn layer of the clash scene's frame graph (root
  `scene_arena+0x14b0/0x1680` → `sSYRdpResetDisplayList` →
  `dMNTitleDisplayList` → branch chain `+0x4e0 → +0x530 → +0x5e0`).
  The builder emits `SETTIMG(NULL)` because some game-side image pointer
  field reads 0 (memset arena value) in affected boot layouts.
- Deterministic within a run; the affected/unaffected split is decided at
  boot (allocation/timing interleave shifting scene-arena layout — the
  64-heal animated texture landing on a previously-cached address is the
  same coin).

## Why not fixed now

Identifying the exact game-side holder needs a live affected process:
attach gdb, `break` on the diag (`null-settimg-loadblock` path,
`interpreter.cpp` GfxDpLoadBlock null branch), then walk the gfx heap
allocation records / game callstack to name the builder writing the DL at
`scene_arena+0x5e0`. ~50% repro per boot; frame 3669 every time. Do NOT
patch the interpreter side — skipping the draw is already correct-enough
port behavior; the fix belongs at the builder (init the field or skip the
object).

## Diagnostics that stay in the tree

- `null-settimg-loadblock` exec-stack dump (first 3 per run).
- `SSB64_SCREENSHOT_FRAMES`/`SSB64_SCREENSHOT_DIR` frame capture.
- TextureCache heal log (also confirms the content-verify cache is doing
  per-frame work on one animated 64×64 IA8 texture — a cheap future
  optimization: key that texture's cache entry on content, or accept the
  re-import).
