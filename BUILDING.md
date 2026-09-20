# Building BattleShip

BattleShip builds with CMake and the `libultraship` + `torch` submodules.
Both the **US** (NTSC-U v1.0, `NALE`) and **JP** (Nintendo All-Star!
Dairantou Smash Brothers, `NALJ`) versions are supported. The decomp game
code is region-conditionally compiled, so each version is its own build —
pick the version at configure time with `-DSSB64_VERSION=us|jp` and use a
separate build directory per version.

The flow is **three commands per version, per platform**: configure,
build, run. Asset extraction from your ROM happens automatically as part
of the build (cached afterwards) — there is no separate extract step.

## Prerequisites

- Git, CMake, Python 3, Ninja (Linux/macOS), or Visual Studio 2022 +
  Windows SDK (Windows)
- Submodules: `git submodule update --init --recursive`
- A legal ROM at the repo root matching the version you are building:
  - **US:** `baserom.us.z64` — `cmake -DSSB64_VERSION=us` (default)
  - **JP:** `baserom.jp.z64` — `cmake -DSSB64_VERSION=jp`

  | Version | Game code | SHA‑1 | MD5 |
  |---------|-----------|-------|-----|
  | US (NTSC-U v1.0) | `NALE` | `e2929e10fccc0aa84e5776227e798abc07cedabf` | `f7c52568a31aadf26e14dc2b6416b2ed` |
  | JP (NALJ v1.0)   | `NALJ` | `4b71f0e01878696733eefa9c80d11c147ecb4984` | `66db457b130d31a286a23d6e4dd9726e` |

  A dump that doesn't match these hashes will not work.

`.z64` is shown above; `.n64`/`.v64` are also accepted.

## Linux / macOS

Replace `<ver>` with `us` or `jp`.

```bash
# 1. configure
cmake -S . -B build-<ver> -GNinja -DSSB64_VERSION=<ver>

# 2. build (compiles the game and extracts your ROM's assets)
cmake --build build-<ver> -j

# 3. run
cd build-<ver> && ./BattleShip
```

For example, to build and run both:

```bash
cmake -S . -B build-us -GNinja -DSSB64_VERSION=us && cmake --build build-us -j && (cd build-us && ./BattleShip)
cmake -S . -B build-jp -GNinja -DSSB64_VERSION=jp && cmake --build build-jp -j && (cd build-jp && ./BattleShip)
```

Add `-DCMAKE_BUILD_TYPE=Release` to the configure command for an
optimized build (Ninja is single-config, so the build type is fixed at
configure time — use a distinct build dir, e.g. `build-<ver>-release`).

## Windows

From a Developer PowerShell, replace `<ver>` with `us` or `jp`
(`cmake.exe` is typically at `C:\Program Files\CMake\bin\cmake.exe`):

```powershell
# 1. configure
cmake -S . -B "build\<ver>" -A x64 -DSSB64_VERSION=<ver>

# 2. build (compiles the game and extracts your ROM's assets)
cmake --build "build\<ver>" --config Release

# 3. run
.\build\<ver>\Release\BattleShip.exe
```

Use `--config Debug` (and run `.\build\<ver>\Debug\BattleShip.exe`) for a
debug build. CMake auto-detects the newest installed Visual Studio; pin
it with `-G "Visual Studio 17 2022" -T v143` if a runner has several.

## Notes

- Each version's assets and binary live entirely in that version's build
  directory; US and JP never clobber each other. Run the binary from its
  build dir (it loads `BattleShip.o2r` relative to the working
  directory).
- Switching versions is just a different build dir + `-DSSB64_VERSION`;
  both can be built and kept side by side.
- A normal build also produces the standalone `torch` sidecar and copies
  `config.yml`, `yamls/<ver>`, `f3d.o2r`, `gamecontrollerdb.txt`, and the
  menu fonts next to the executable.
- The checked-in reloc YAMLs (`yamls/<ver>/reloc_*.yml`) and the
  generated `port/resource/RelocFileTable.<ver>.cpp` are treated as
  source inputs, not rebuilt on every compile.

### GameCube adapter (WUP-028) device access

The native GameCube adapter driver talks to the adapter over libusb, so the
adapter has to be reachable from user space. This is a one-time OS-level
setup, unrelated to building, and it is the same setup Dolphin needs.

**Linux** — by default udev creates USB device nodes `0664 root:root`, which a
normal user cannot open, so the adapter is silently unavailable. Install a
rule granting access:

```bash
sudo tee /etc/udev/rules.d/51-gcadapter.rules >/dev/null <<'EOF'
# Nintendo WUP-028 GameCube Controller Adapter
SUBSYSTEM=="usb", ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="057e", ATTRS{idProduct}=="0337", MODE="0660", TAG+="uaccess"
EOF
sudo udevadm control --reload-rules
```

Then unplug and replug the adapter. `TAG+="uaccess"` is what actually grants
the logged-in user an ACL on the device node; `MODE` alone is a blunter
alternative. You may already have this via another package — `steam-devices`
ships an identical rule, and Dolphin's instructions add one too — in which
case the adapter works without any action here.

The kernel's `usbhid` driver binding the adapter is **not** a problem: the
driver detaches it at claim time (`libusb_set_auto_detach_kernel_driver`) and
the kernel reattaches it on exit. No auto-detach rule is needed.

**Windows** — the adapter must be bound to the **WinUSB** driver, using
[Zadig](https://zadig.akeo.ie/) or Dolphin's adapter driver installer. With
the stock driver Windows will not start the device in a way libusb can open.
This is the same requirement Dolphin and Slippi have, so a machine already set
up for those needs nothing further.

**macOS** — nothing to install. The adapter is usable as soon as it is plugged
in.

If the adapter is not detected, run with logging and look for `[gcadapter]`
lines in `logs/BattleShip.log`; a permission or driver problem is reported
there with the fix for your platform.

### Advanced / manual targets

Normally unnecessary (the build does these for you). Append the target to
`cmake --build build-<ver>`:

| Target | Purpose |
|--------|---------|
| `ExtractAssets` | Re-extract `BattleShip.o2r` from the ROM |
| `ExtractAssetHeaders` | Regenerate generated build-input headers |
| `RegenerateRelocYamls` | Regenerate the checked-in reloc YAMLs |

Packaged release builds ship no ROM: CMake installs a sidecar `torch` +
`config.yml`, and the game extracts assets from a user-supplied ROM on
first run instead of at build time.
