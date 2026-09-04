# Training Mode crash selecting the three extra stages (issue #264)

**Date:** 2026-08-31
**Status:** FIXED (decomp; needs manual repro check)
**Symptom (reported):** "Crashes when selecting the three unlockable stages" — the crash dump failed to upload, so this was root-caused from code.

## Which stages

Not the vanilla unlockable (Mushroom Kingdom) — the port's stage select has a second page (`dMNMapsPageGkinds`, reached with L/R) exposing three 1P stages: Final Destination (`nGRKindLast` = 16), Meta Crystal (`nGRKindMetal` = 13), and Duel Zone/Battlefield (`nGRKindZako` = 14).

## Root cause

**Training Mode only.** `sc1PTrainingModeLoadWallpaper` and `sc1PTrainingModeInitDisplayVars` (`decomp/src/sc/sc1pmode/sc1ptrainingmode.c`) index `dSC1PTrainingModeWallpaperIDs[]` and `dSC1PTrainingModeWallpaperHeapOffsets[]` by battle gkind. Both tables had **9 entries** (Castle…Mushroom Kingdom); gkinds 13/14/16 read 4–7 words past the end (into `dSC1PTrainingModeFadeColor`/`dSC1PTrainingModeVideoSetup`), and the garbage then indexed the **3-entry** `dSC1PTrainingModeWallpaperDescs[]` unbounded. Downstream: wild descs read → SIGSEGV; or garbage `file_id` → NULL file → near-NULL `Sprite*` crash in `lbCommonMakeSObjForGObj` (`fault_addr=0x34`); or garbage heap offset → ~158 KB wallpaper memcpy to a wild address → heap corruption.

Introduced by decomp `76a034d8b` (issue #243, Training Mode extra-stage scroll): its writeup claimed "the training wallpaper paths already handle those gkinds", but only the **CSS-preview** tables in `mnmaps.c` (`dMNMapsWallpaperOffsets`, `dMNMapsTrainingModeWallpaperIDs`, both 17 entries) were extended — the runtime scene's copies in `sc1ptrainingmode.c` were missed. VS mode uses a different path and never touches these tables, which is why only Training Mode died.

## Fix

- Extended both `sc1ptrainingmode.c` tables to 17 entries, values mirroring their `mnmaps.c` counterparts (the three page-2 stages get wallpaper ID 2 = training blue, heap offset `0x26C88` like every other stage).
- PORT-only clamped accessors (`sc1PTrainingModeWallpaperID` / `sc1PTrainingModeWallpaperHeapOffset`) used by both call sites: an out-of-range gkind or wallpaper id now logs once and degrades to the blue wallpaper instead of faulting, so a future stage added to `dMNMapsPageGkinds` can't reintroduce this. Non-PORT builds keep the original expressions.

## Verification

- Build green; normal boot unaffected. Direct-boot repro (`SSB64_START_SCENE=54`) crashes identically with and without the fix in `sc1PTrainingModeFuncStart` — that's a pre-existing artifact of skipping CSS battle-state setup, not this bug.
- Manual repro check needed: Training Mode → stage select → R to page 2 → pick Final Destination / Meta Crystal / Duel Zone → should load with the blue training wallpaper; same stages in VS mode were already fine.

## Audit hook

When a gkind-indexed table gains new reachable gkinds (stage-select changes), grep for **every** table indexed by `gkind` — the CSS and the in-scene code keep separate copies (`mnmaps.c` vs `sc1ptrainingmode.c`), and fixing one does not fix the other. Related latent issue noted for a future pass: `mnMapsSetNamePosition`'s 9-entry `positions[]` is gkind-indexed on `REGION_JP` (harmless float garbage today).
