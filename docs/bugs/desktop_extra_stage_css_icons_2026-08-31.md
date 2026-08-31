# Desktop Extra-Stage CSS Icons (Question Marks in Every Shipped Build)

**Status:** Fixed (reported via issue #267's "unfinished thumbnails" remark).

## Symptom

On the VS / Training stage-select screen, the three port-added stages
(Final Destination, Metal Cavern, Battlefield) show the question-mark
placeholder instead of their icon thumbnails — on **every desktop release
build, always**. Dev builds are unaffected.

This was initially suspected to be a regression ("the icons used to work"),
but inspection of the v1.3 and v1.6 Linux AppImages proved otherwise:
no shipped desktop artifact has ever contained `assets/css_icons/` PNGs,
and no shipped binary had derivation code (`strings` shows only the loader).
The icons only ever worked in dev builds, where CMake's `DeriveStageAssets`
step extracts the PNGs from the local baserom and stages them next to the
binary.

## Root Cause

The extra-stage CSS pipeline had exactly one desktop asset source:
build-time PNGs from `tools/derive_stage_assets.py`. Those are ROM-derived,
so packaging scripts can never legally bundle them, and the first-run
extraction flow (`port/first_run.cpp`) never produced them. Release users
therefore had no PNG → `portCSSGetStageIconSprite()` returned NULL →
`mnMapsMakeIcons()` fell back to the question-mark sprite
(`decomp/src/mn/mnmaps/mnmaps.c`).

This is precisely the gap the Android fix's audit hook warned about:
`docs/bugs/android_extra_stage_css_icons_2026-06-24.md` fixed the same
symptom for Android by deriving the assets at first-run from the user's ROM,
and noted that every port-only CSS asset must answer *separately* how
desktop and Android get the file. Desktop never got its answer.

## Fix

`port/css_icons/port_css_stage_assets.cpp` gained a second, platform-neutral
source: derive the background and icon at runtime from the user's own
extracted archive (`BattleShip.o2r`). The stage's reloc file
(`STAGE_TABLE.wp_file_id` — 96 / 0x62 / 0x61, all with the 300x220 RGBA16
wallpaper Sprite at 0x26c88) is loaded through the LUS ResourceManager;
`RelocFile::Data` is the pristine big-endian decompressed file image, so the
Sprite is parsed exactly the way the build-time Python script parses the ROM
file: RELOC pointer resolution (`(raw & 0xFFFF) * 4`), TMEM odd-row XOR4
un-swizzle, rendered-rows-only stitch. The icon is a center-crop +
bilinear 48x36 downscale (same filter as the Android deriver).

Lookup order per asset:

1. `assets/css_icons/<name><suffix>.png` in the app-data dir (moddable
   override; where dev builds and the Android deriver put files),
2. the same path in the real executable/bundle dir (portable builds —
   this also fixes the old loader's single-path
   `GetPathRelativeToAppDirectory` lookup, which missed when CWD differed
   from the binary dir),
3. o2r wallpaper derivation (backgrounds + icons only).

Nameplates stay PNG-only — they're synthesized text with no ROM sprite, and
`mnmaps.c` already falls back to rendering the stage name with the in-game
subtitle font. Emblems also stay PNG-only (`mnMapsMakeEmblem` skips them
entirely pending the IA4 render fix).

Every parse step is bounds-checked and geometry-validated against
`STAGE_TABLE`; any mismatch (e.g. a JP archive where the sprite moved)
degrades to the old question-mark behavior, never a garbage image.

## Verification

- Runtime derivation exercised in the running game via a temporary
  boot-time self-test (removed after use): all three derived backgrounds
  were **byte-identical** to an independent Python decode of the same o2r
  entries; derived icons visually matched the build-time LANCZOS PNGs.
- JP verified: parsing `baserom.jp.z64` (reloc table @ 0x1ACAF0, 2107 files)
  shows files 96 / 0x62 / 0x61 carry the same 300x220/44-bitmap RGBA16
  wallpaper Sprite at 0x26c88, and decoding them confirms the same stage
  mapping (FD starfield / Meta Crystal / Duel Zone). The derivation works
  unmodified on JP builds.

## Related: "Bonus Stages" Toggle (issue #267)

The same issue asked for a way to hide the three stages entirely.
`port/enhancements/BonusStages.cpp` adds `gEnhancements.BonusStagesEnabled`
(LUS menu → Settings → Gameplay → "Bonus Stages", default ON). When off,
`mnMapsCheckLocked()` reports the three gkinds as locked, which in one
stroke: hides their icons, refuses page-jump landings (the bonus page
becomes unreachable and `mnMapsMakeArrows` suppresses the right arrow via
`mnMapsPageHasUnlockedSlot`), excludes them from random stage selection,
and reroutes a restored cursor to page 0. Per the audit hook in
`training_mode_extra_stage_scroll_2026-06-24.md`: here the visual gating
and navigation gating are *intentionally* the same thing — the toggle must
remove the page from navigation, not just blank the icons.

## Audit Hook

A ROM-derived runtime asset now has three sources (dev build-time PNG,
Android first-run PNG, o2r derivation). When adding a fourth stage, update
all three manifests together: `STAGES` in `tools/derive_stage_assets.py`,
`kAndroidCssStages` in `port/android_torch_bridge.cpp`, and `STAGE_TABLE`
in `port/css_icons/port_css_stage_assets.cpp` (the latter now carries
`wp_file_id`/`wp_sprite_off` for the o2r path).
