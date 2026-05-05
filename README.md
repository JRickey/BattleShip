<p align="center">
  <img src="assets/BattleShip.svg" alt="BattleShip" width="640">
</p>

# BattleShip

**BattleShip** is a PC port of **Super Smash Bros. (N64, NTSC-U v1.0)** built on top of the [VetriTheRetri/ssb-decomp-re](https://github.com/vetritheretri/ssb-decomp-re) decompilation, using [libultraship](https://github.com/Kenix3/libultraship) for PC-native rendering / audio / input and [Torch](https://github.com/HarbourMasters/Torch) for extracting assets out of the ROM at build time.

Runs natively on macOS (Apple Silicon), Linux, and Windows.

Android is planned and work-in-progress.

## No copyrighted assets are included in this repository

**None of Nintendo's assets (code, textures, audio, models, text, ROM data) are checked into this repo or distributed with builds.** The port is a pure C/C++ source tree; every byte of Nintendo-owned data is extracted at build time from a ROM that *you* supply. If you do not own a legal copy of Super Smash Bros. for the Nintendo 64, you cannot build or run this project.

The required ROM is **NTSC-U v1.0** (game code `NALE`, internal name `SMASH BROTHERS`):

| Hash   | Value                                      |
|--------|--------------------------------------------|
| SHA‑1  | `e2929e10fccc0aa84e5776227e798abc07cedabf` |
| MD5    | `f7c52568a31aadf26e14dc2b6416b2ed`         |

If your dump does not match those hashes, it will not work.

## Features

### Toggleable Port Specific Enhancements

- Tap Jump
- C-Stick Smash
- D-Pad Jump
- Hitbox Visual Mode
- Nrage Stick Config with Customizable Ranges

## Controls

- Controller and Rumble Support Powered by SDL 2
- Native Raphnet Support up to 4 Channels Through Hidapi

## Rendering

All Powered by LibultraShip

- Resolution scaling
- Anti Aliasing
- Tri Point Texture Filtering

## Rollback Netcode and Online Play (WORK IN PROGRESS)

## Author's notes

### About the project

This is a work in progress port that I started and developed alone with Claude until the v0.3-beta release. It took a little over a month of night and day work to get to the first beta release, and a massive 4 day long sprint working all day resolving bugs from v0.3-beta to v0.7-beta. As of now, this is no longer a pure AI project. Many people have offered their time playtesting, their knowledge of the game, or their experience with modding or competitive play; to bring improvements, bug fixes, new feature suggestions, and additions. Without them, the project would not be what it is today.

If you find anything that you would like to see improved, create an issue on Github. I'll fix it.

This port was started when the original decomp was 96% complete code only. The remaining 4% is the internal debug menu and Libultra functions that aren't necessary to port the game. The relocatable data was also not decompiled at the time. The port was designed with this in mind, and it is why certain design decisions had to have been made. I like to move fast, obviously.

This project uses a heavily modified LibUltraShip and Torch, those two modules obviously don't work with every n64 game out of the box and a lot of game specific things needed to be implemented. The original game is quite unique and uses rendering techniques and n64 hardware tricks in ways other LUS projects did not need to account for.

This is a passion project for me. Not out of nostalgia, but to actually make something. I don't have too much nostalgia for the original game, considering I didn't own a copy as a kid, but I remember playing it at a friend's house a handful of times. What motivates me to work on this is because I can. Because it's hard, because it hasn't been done before (at least not open source). It's required a ton of my personal time, even using AI. It's forced me to get creative, to design custom tools, to force the agents into certain boxes.

People may not like it because I used AI, and that's okay. I'm not going to argue with your opinion or try to say that my way is the correct way of doing things. It works for me and that's what matters to me. My code is open source, it's free, it's MIT Licensed, and everyone can learn from it. That's what should matter to you.

I hope you enjoy the project.

---

## Building

If you want to manually compile BattleShip, please consult the [building instructions](BUILDING.md).

### Nintendo Switch

The Switch port cross-compiles from macOS or Linux using the [devkitPro](https://devkitpro.org) toolchain. It runs at 60fps in handheld (720p) and docked (1080p) via OpenGL ES (Mesa/nouveau).

**Prerequisites:**

```bash
# Install devkitPro and Switch packages
dkp-pacman -S switch-dev switch-sdl2 switch-glad switch-tinyxml2 \
               switch-libzip switch-spdlog switch-nlohmann-json
```

**One-command build:**

```bash
./scripts/build-switch.sh
```

This script:
1. Builds Torch natively to extract `BattleShip.o2r` from your ROM
2. Cross-compiles the game for Switch (ARM64, Cortex-A57)
3. Bundles the `.nro` + `.o2r` files into `build-switch/switch_sd/`

**Manual steps (if you prefer):**

```bash
# 1. Extract assets (requires baserom.us.z64 at project root)
cmake -S . -B build-native -DCMAKE_BUILD_TYPE=Release
cmake --build build-native --target TorchExternal -- -j1
./build-native/TorchExternal/src/TorchExternal-build/torch o2r baserom.us.z64

# 2. Cross-compile for Switch
cmake -S . -B build-switch \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Switch.cmake
cmake --build build-switch -- -j2

# 3. Package for SD card
/opt/devkitpro/tools/bin/elf2nro build-switch/BattleShip.elf build-switch/BattleShip.nro
cmake --build build-switch --target BundleSwitch
```

**Deploy:** Copy `build-switch/switch_sd/switch/BattleShip/` to your SD card's `sdmc:/switch/` directory. Launch via hbmenu.

**First boot note:** OpenGL ES shaders are compiled at runtime. The first playthrough will populate `sdmc:/switch/BattleShip/shader_cache/` — subsequent runs will be smooth. If you delete the cache, expect shader-compilation hitches on the next boot.

## Architecture

The port has three layers and they are kept deliberately separate:

```
┌──────────────────────────────────────────────────────────────┐
│  decompiled game code  (decomp/src/)                         │
│  Unmodified C produced by the decomp project. Talks to the   │
│  N64 the same way the original ROM did: GBI display lists,   │
│  ALSeqPlayer audio, OS threads, OSContPad input.             │
├──────────────────────────────────────────────────────────────┤
│  port layer            (port/)                               │
│  Modern C++ glue. Translates N64-shaped APIs into LUS calls, │
│  fixes endianness on freshly-loaded data, owns Ship::Context │
│  and the resource factories, and quarantines every change    │
│  the decomp doesn't need to know about.                      │
├──────────────────────────────────────────────────────────────┤
│  libultraship         (libultraship/)                        │
│  PC-native runtime: Fast3D renderer (OpenGL / Metal / D3D),  │
│  SDL2 input, miniaudio output, OTR/O2R resource manager,     │
│  ImGui overlay.                                              │
└──────────────────────────────────────────────────────────────┘
```

### Asset pipeline

`baserom.us.z64` is never read at runtime. At build time, **Torch** walks the ROM with the YAMLs under `yamls/us/` and emits `BattleShip.o2r` — a zip-format archive of typed resources (textures, sequences, sample banks, animations, reloc files). At launch, libultraship's resource manager mounts `BattleShip.o2r` + `f3d.o2r` and the port code requests resources by path. This is the same pipeline used by Ship of Harkinian, Starship, SpaghettiKart, etc.

The relocatable-data files (fighter tables, item tables, effects, sprites) are SSB64-specific and required custom factories on the Torch side and a custom loader (`port/resource/RelocFileFactory.cpp`) on the runtime side.

### Build-time codegen

A small amount of generated code lives outside the source tree (gitignored) and is regenerated on every build from `tools/reloc_data_symbols.us.txt`:

- `include/reloc_data.h` — extern declarations for every relocatable symbol
- `yamls/us/reloc_*.yml` — Torch extraction configs
- `port/resource/RelocFileTable.cpp` — the runtime symbol table

If you ever see "undefined reference to `dFooBarReloc`" you regenerated the table without rebuilding, or vice versa.

---

## Code conventions

### `#ifdef PORT` — what it is and what it isn't

Every meaningful change to a decomp source file is wrapped in `#ifdef PORT` / `#else` / `#endif`. The discipline this enforces:

- **The original decomp code path stays intact and compilable** under the IDO toolchain on a real N64 build. This is non-negotiable — if it ever stops being true, upstreaming improvements back to the decomp project becomes impossible.
- **The PORT branch is allowed to be ugly** — an explicit endian conversion, a struct rewrite, a function shim — as long as the contract it presents to the rest of the file is the same as the N64 branch.
- **Reloc tokens vs. raw pointers**: a field declared `u32` under `#ifdef PORT` where the N64 branch declared `T*` is a *reloc token*, not a raw pointer. Resolving it requires `PORT_RESOLVE(token)`. Assigning a real pointer with `(uintptr_t)ptr` will silently truncate on LP64 — wrap post-load writes in `PORT_REGISTER`.

### Decomp preservation: behavior, not bytes

The repo follows a single principle for changes to `decomp/src/`:

> **Accuracy to game behavior > accuracy to ROM bytes.**

That means IDO idioms that encode original N64 semantics — odd casts, `goto` flow, deliberate temporaries — are load-bearing and stay. But **compiler-compat shims** (warning suppressions, permissive flags, header shortcuts) that mask real bugs on modern LP64 toolchains do *not* survive. The most expensive lesson of the project was that `-Wno-implicit-function-declaration` was silently truncating 64-bit pointer returns to 32-bit `int` in dozens of places — see `docs/bugs/item_arrow_gobj_implicit_int_2026-04-20.md`. The shim is gone; the real declarations are in.

### Naming prefixes

The decomp uses two-letter module prefixes throughout. Knowing them makes the source tree navigable:

| Prefix | Meaning |
|--------|---------|
| `ft`   | Fighter (`ftMario`, `ftKirby`, `ftFox`, …) |
| `it`   | Item (`itAttribute`, `itManager`) |
| `wp`   | Weapon |
| `ef`   | Effect / particle |
| `gm`   | Game mode |
| `gr`   | Stage (ground) |
| `mp`   | Map / collision |
| `mn`   | Menu |
| `sc`   | Scene |
| `sy`   | System (engine internals) |
| `sf`   | Saved-state / save file |
| `db`   | Debug |
| `cm`   | Camera |
| `lb`   | Library (low-level utilities) |
| `obj`  | GObj / DObj / OMObj — game-object wrappers |

Full reference: [`docs/c_conventions.md`](docs/c_conventions.md).

### Endianness handling

The N64 is big-endian; PC targets are little-endian. The port handles this in three layers:

1. **Gross byteswap at load** — `lbRelocLoadAndRelocFile` byteswaps relocatable files word-by-word during load.
2. **Per-struct fixups** — small `portFixupStructU16` / `portFixupStructU8` helpers fix sub-word fields that the gross swap got wrong (e.g., `{u16, u8, u8}` patterns where the two `u8`s end up swapped).
3. **Per-stream walkers** — animation events, spline interpolators, and other variable-length streams are halfswapped at file-bake time and need a per-stream un-halfswap on first access. These live in `port/port_aobj_fixup.{h,cpp}` and friends.

If you find a new struct that reads garbage, the playbook in `docs/n64_reference.md` will tell you which layer it belongs in.

### Bitfield layout

The IDO compiler packs small bitfields into preceding `u16` pad gaps, MSB-first. Modern compilers (Clang, GCC) on LE targets pack LSB-first into the next storage unit. Bitfield structs that travel through file data must be **rewritten under `#ifdef PORT`** to match the IDO physical layout — see `docs/debug_ido_bitfield_layout.md` for the workflow (compile + rabbitizer disasm to verify bit positions before porting).

Patching the *reads* in game code instead of the *layout* is forbidden by team policy; the bug always comes back. See `feedback_struct_rewrite_over_overrides.md` in the project's memory for the long version.

---

## Why the submodules are forks

Both `libultraship` and `torch` are pinned to **personal forks** rather than the upstream Harbour Masters repos, because each needs SSB64-specific changes that don't exist upstream.

### [`libultraship`](https://github.com/JRickey/libultraship/tree/ssb64) — fork of [Kenix3/libultraship](https://github.com/Kenix3/libultraship)

SSB64 drives the RDP differently than the Zelda / Mario 64 / Star Fox 64 titles libultraship was originally built for — in particular around tile masks, `SetTileSize` extents, IA/I4 texture uploads, `gDPSetPrimDepth` for 2D layering, and Metal sampler binding for matanim CCs. Upstreaming the fixes is on the list, but until then the fork carries:

- Clamping `ImportTexture*` upload width/height to the active `SetTileSize` extent (fixes the fighter "black squares" bug and the IA8/I4 stretch bug)
- Honouring `SetTile` mask/maskt in the Import* path
- A real implementation of `gDPSetPrimDepth` (was a `TODO Implement` stub upstream — broke every `G_ZS_PRIM` 2D sprite)
- A 1×1 black fallback texture for sampler slots that would otherwise alias the screen drawable on Metal (fixed the Whispy canopy stripe)
- A no-logging path in `IResource`'s destructor to prevent a shutdown-time crash

### [`torch`](https://github.com/JRickey/Torch/tree/ssb64) — fork of [HarbourMasters/Torch](https://github.com/HarbourMasters/Torch)

Torch is the tool that reads the ROM and emits `BattleShip.o2r`. Upstream supports OoT, MM, SF64, MK64, PM64, etc., but has no knowledge of SSB64's file formats. The fork adds:

- An `SSB64` build flag and game target
- A reloc-file factory for SSB64's relocatable data blobs (fighters, items, effects, sprites)
- `libvpk0` integration for VPK0-compressed segments

Both forks live as submodules so their history stays their own and so upstream changes can be merged in cleanly when/if the fixes are accepted.

---

## Repo layout

```
port/         modern C++ port layer — Ship::Context, resource factories,
              endian fixups, bridges between decomp code and libultraship
include/      headers (some generated: reloc_data.h)
decomp/       submodule — decompiled SSB64 C source (largely unchanged
              game logic). Major subdirs inside the submodule:
                src/sys/        main loop, DMA, scheduling, audio,
                                controllers, threading
                src/ft/         fighters (ftmario/, ftkirby/, ftfox/, …)
                src/sc/ gm/ gr/ scene / game modes / stage rendering
                src/mn/ it/ ef/ menus / items / effects
                src/relocData/  reloc data sources
libultraship/ submodule — PC-native render / audio / input / resource mgr
torch/        submodule — asset extractor (ROM → BattleShip.o2r)
yamls/us/     Torch YAML extraction configs (some generated)
tools/        Python helpers: reloc stubs, YAML gen, credits encoder
docs/         architecture notes, bug write-ups, debugging guides
debug_tools/  optional disasm / diff utilities (not required for a build)
scripts/      packaging (.dmg / .AppImage / .zip), worktree helper
```

### Further reading

- [`docs/architecture.md`](docs/architecture.md) — project status, ROM info, dependency map, source-tree layout
- [`docs/c_conventions.md`](docs/c_conventions.md) — decomp naming prefixes, IDO idioms to preserve, code style, macros
- [`docs/n64_reference.md`](docs/n64_reference.md) — RDRAM, RSP/RDP, GBI, audio, threading, endianness primer
- [`docs/build_and_tooling.md`](docs/build_and_tooling.md) — CMake details, reloc stub regen, runtime logs, LP64 compat notes
- [`docs/debug_gbi_trace.md`](docs/debug_gbi_trace.md) — capturing GBI traces from the port and a M64P plugin, diffing with `gbi_diff.py`
- [`docs/debug_ido_bitfield_layout.md`](docs/debug_ido_bitfield_layout.md) — verifying ported struct bit positions against IDO output via rabbitizer
- [`docs/bugs/README.md`](docs/bugs/README.md) — index of resolved bugs with per-bug root-cause and fix write-ups (~45 entries)

---

## Contributing

PRs are welcome but please don't be offended if responses are slow — this is a side project. If you're opening a bug report, the most useful things to include are:

- SHA-1 of your `baserom.us.z64`
- OS + architecture (especially macOS ARM64 vs x86_64, since LP64 bitfield layout has bitten us several times — see `docs/bugs/`)
- The contents of `ssb64.log` from the run that reproduces the issue
- A GBI trace if the issue is rendering-related (see `docs/debug_gbi_trace.md`)

## Credits & licensing

- Game code, data, sound, textures, models, and trademarks: **© Nintendo / HAL Laboratory.** Not included in this repository, not redistributed, and not endorsed by them.
- Decompilation: [VetriTheRetri/ssb-decomp-re](https://github.com/VetriTheRetri/ssb-decomp-re) and its contributors. Vendored as the `decomp/` submodule. At the time of writing the upstream project does not publish an explicit license; this repository makes no copyright claim over the decompiled source and refers to the upstream project for any rights, terms, or restrictions on reuse.
- Runtime framework: [libultraship](https://github.com/Kenix3/libultraship) — Copyright (c) 2022 kenix3, MIT-licensed. Originated by the Harbour Masters team (Ship of Harkinian) and now maintained at Kenix3/libultraship. Vendored as the `libultraship/` submodule.
- Asset pipeline: [Torch](https://github.com/HarbourMasters/Torch) — Copyright (c) 2023 Lywx (Harbour Masters), MIT-licensed. Vendored as the `torch/` submodule.
- Menu fonts: [Montserrat](https://github.com/JulietaUla/Montserrat) and [Inconsolata](https://github.com/cyrealtype/Inconsolata), both bundled under the [SIL Open Font License 1.1](https://openfontlicense.org). License texts ship alongside the font files in [`assets/custom/fonts/`](assets/custom/fonts/).
- Reference ports I learned from and adapted code from: [Starship](https://github.com/HarbourMasters/Starship) (SF64) and [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright) (OoT) — both MIT-licensed by The Harbour Masters; see [`LICENSE`](LICENSE) for the per-file attribution.
- Reference ports I learned from but did not borrow code from: [SM64 PC Port](https://github.com/sm64-port/sm64-port) (SM64), [SpaghettiKart](https://github.com/HarbourMasters/SpaghettiKart) (MK64).
- Port work: me ([JRickey](https://github.com/JRickey)), with an enormous amount of help, debugging, and feature suggestions from contributors in our Discord server.

This project is **not affiliated with, endorsed by, or authorized by Nintendo.** It is a personal, non-commercial research and preservation effort. Do not upload ROMs, extracted `.o2r` archives, or any other Nintendo-owned data to issues or pull requests.

This project is **not affiliated with, endorsed by, or authorized by Harbour Masters** either. It uses libultraship (originated by the Harbour Masters team and now maintained at [Kenix3/libultraship](https://github.com/Kenix3/libultraship)) and Torch (the [HarbourMasters/Torch](https://github.com/HarbourMasters/Torch) asset extractor) as upstream dependencies via personal forks, but it is an independent fan effort. Issues, bugs, and support questions about this port should not be directed to the Harbour Masters team.

## License

Source code in this repository (everything outside the `decomp/`, `libultraship/`, and `torch/` submodules — each of which carries its own attribution) is released under the [MIT License](LICENSE) — free to use, modify, and redistribute, with no warranty and no liability. See [`LICENSE`](LICENSE) for the full text.

The MIT grant covers only the port-specific code (the `port/` layer, build scripts, tools, docs). It does **not** extend to:
- Game assets, code, audio, textures, models, or any other content owned by Nintendo / HAL Laboratory — none of which is in this repository.
- The decompilation vendored as the `decomp/` submodule, sourced from [VetriTheRetri/ssb-decomp-re](https://github.com/VetriTheRetri/ssb-decomp-re). The upstream project does not currently publish an explicit license; this repository makes no copyright claim over the decompiled source and incorporates it by reference. Defer to the upstream project and its contributors for any rights or restrictions on reuse.
- Decomp-derived content elsewhere in the tree (vendored or auto-generated from the decomp's symbol/file-name tables): `tools/reloc_data_symbols.us.txt`, `tools/relocFileDescriptions.us.txt`, `include/reloc_data.h`, `port/resource/RelocFileTable.cpp`, and `yamls/us/reloc_*.yml`. These follow the same upstream-deference as the `decomp/` submodule. The generator scripts that produce them (`tools/generate_*.py`) are port-authored and remain MIT.
- The `libultraship` submodule — Copyright (c) 2022 kenix3, MIT-licensed (originated by the Harbour Masters team).
- The `torch` submodule — Copyright (c) 2023 Lywx (Harbour Masters), MIT-licensed.
- The bundled menu fonts under `assets/custom/fonts/`, which are licensed under the SIL Open Font License 1.1 (per-font license files in that directory).
