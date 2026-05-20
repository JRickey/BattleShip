# Building RafaelSmashing
RafaelSmashing builds with CMake and the `libultraship` + `torch` submodules.
Both the **US** (NTSC-U v1.0, `NALE`) and **JP** (Nintendo All-Star!
Dairantou Smash Brothers, `NALJ`) versions are supported. The decomp game
code is region-conditionally compiled, so each version is its own build —
pick the version at configure time with `-DSSB64_VERSION=us|jp` and use a
separate build directory per version.

The flow is **three commands per version, per platform**: configure,
build, run. Asset extraction from your ROM happens automatically as part
of the build (cached afterwards) — there is no separate extract step.

## Prerequisites

- Git, CMake, Python 3, or Visual Studio 2022 +
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

## Notes

- Each version's assets and binary live entirely in that version's build
  directory; US and JP never clobber each other. Run the binary from its
  build dir (it loads `RafaelSmashing.o2r` relative to the working
  directory).
- Switching versions is just a different build dir + `-DSSB64_VERSION`;
  both can be built and kept side by side.
- A normal build also produces the standalone `torch` sidecar and copies
  `config.yml`, `yamls/<ver>`, `f3d.o2r`, `gamecontrollerdb.txt`, and the
  menu fonts next to the executable.
- The checked-in reloc YAMLs (`yamls/<ver>/reloc_*.yml`) and the
  generated `port/resource/RelocFileTable.<ver>.cpp` are treated as
  source inputs, not rebuilt on every compile.

### Advanced / manual targets

Normally unnecessary (the build does these for you). Append the target to
`cmake --build build-<ver>`:

| Target | Purpose |
|--------|---------|
| `ExtractAssets` | Re-extract `RafaelSmashing.o2r` from the ROM |
| `ExtractAssetHeaders` | Regenerate generated build-input headers |
| `RegenerateRelocYamls` | Regenerate the checked-in reloc YAMLs |

Packaged release builds ship no ROM: CMake installs a sidecar `torch` +
`config.yml`, and the game extracts assets from a user-supplied ROM on
first run instead of at build time.
