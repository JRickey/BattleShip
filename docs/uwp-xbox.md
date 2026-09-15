# BattleShip UWP / Xbox developer mode

This branch builds the US version only. It does not contain or download ROM
content. The supported ROM is Super Smash Bros. (USA) v1.0, SHA-1
`e2929e10fccc0aa84e5776227e798abc07cedabf` (MD5
`f7c52568a31aadf26e14dc2b6416b2ed`).

## Install

Download both CI artifacts: the signed UWP package and `BattleShip-US-asset-tool`.
Extract the tool, then in PowerShell run:

```powershell
.\Prepare-BattleShip-US.ps1 "C:\path\to\Super Smash Bros. (USA).z64"
```

Sign in to an Xbox profile before installing; Xbox UWP deployment requires an
interactive user session. Copy the generated `BattleShip` directory to the root of an Xbox-formatted
media USB drive. The game archive must resolve as
`E:\BattleShip\BattleShip.o2r`. Install the MSIX bundle and certificate through
Xbox Device Portal, choose **Game** for the app type, and launch it.

The package includes `f3d.o2r`, controller mappings, and fonts. Saves and the
configuration file remain in UWP LocalState, so replacing the package or USB
game data does not overwrite saves. Mods are loaded from `E:\BattleShip\mods`.

The optional custom-stage CSS preview images are ROM-derived and therefore are
not distributed. They can be generated with `tools/derive_stage_assets.py`
(Python + Pillow) and copied to `E:\BattleShip\assets\css_icons`; gameplay does
not depend on them.

## Controller behavior

View/Back toggles the ImGui menu. A/B, the D-pad, and left stick provide normal
ImGui navigation. Controller navigation is enabled by default on UWP.

SDL gamepads are assigned to the first free N64 port as they connect: the first
pad controls player 1, the second player 2, then players 3 and 4. Existing routes
survive hot-plug refreshes instead of collapsing all pads onto player 1.

## Display-matched interpolation

The Graphics menu's **Match Display Refresh Rate** option works on Xbox,
desktop, Android, and iOS. BattleShip keeps its fixed 60 Hz simulation and uses
a fractional subframe cadence to match the reported display refresh, capped at
240 FPS. Rates such as 90, 120, 144, and 165 Hz therefore receive correctly
paced and temporally positioned interpolation frames without changing game
logic speed. Match mode re-samples the active display while running, so moving a
desktop window between monitors or a phone changing adaptive-refresh modes does
not leave it locked to the old rate.

## Reproducible Windows build environment

GitHub Actions gives each package a unique build version. When the repository
defines `UWP_SIGNING_PFX_BASE64` and `UWP_SIGNING_PFX_PASSWORD`, builds use that
stable development certificate; forks without those secrets receive a
disposable self-signed certificate. A matching Windows-container definition
and one-command build script live in `ci/windows`.
The image requires a Windows Server 2022 or compatible Windows 11 container
host; it cannot run on a Linux Docker daemon.
