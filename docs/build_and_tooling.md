# Build & Tooling Rules

## Build System
- CMake is the build system
- libultraship and Torch are git submodules
- MSVC on Windows, Apple Clang on macOS, GCC/Clang on Linux
- The decomp's original MIPS toolchain (IDO 7.1) is NOT used for the port
  - **Windows**: `& 'C:\Program Files\CMake\bin\cmake' -S . -B "build/x64" -A x64` (CMake auto-picks the newest installed Visual Studio + default toolset)
  - **macOS / Linux**: `cmake -S . -B build-cmake -GNinja`
- Build with:
  - `& 'C:\Program Files\CMake\bin\cmake.exe' --build .\build\x64`
- Regenerate generated build inputs with:
  - `& 'C:\Program Files\CMake\bin\cmake.exe' --build .\build\x64 --target ExtractAssetHeaders`
- The executable target is `ssb64`, but the produced binary is `BattleShip`.

### Editing Torch and asset recipes

Use the normal `ssb64` build after editing Torch, libultraship, or an asset
recipe. Desktop builds now invoke Torch's own incremental dependency check
each time; existing source edits and newly added Torch source files are
picked up without deleting its build directory. libultraship already builds
in the main dependency graph; new files must be added to its source lists.

Tracked `yamls/<version>/` files are authoring inputs. Ordinary builds do
not regenerate or write them. CMake copies the selected region's recipes
and `config.yml` into `<build>/asset-recipes/`, and extraction reads that
copy. Updated recipes and the current Torch executable are staged beside
the game on every build, including builds that do not relink the game.
Deleted/renamed recipes are removed from the staged copies.

The extraction fingerprint includes the contents and names of Torch's
source files and the recipes, including uncommitted changes. Editing either
re-extracts the archive automatically and refreshes its `.recipe` sidecar.
An unchanged build preserves the archive, generated tables, and executable.

Regeneration remains available explicitly: `RegenerateRelocYamls` regenerates
the configured region; `DeblobRegen` regenerates both regions from their
baseroms. These commands intentionally update tracked generated assets.
See [the build dependency investigation](bugs/asset_build_dependencies_2026-09-14.md).

### ROM version (US / JP)

`-DSSB64_VERSION=us` (default) or `-DSSB64_VERSION=jp` selects the ROM
version. The decomp game code is region-conditionally compiled
(`#if REGION_US/REGION_JP`), so **US and JP are separate builds — use a
separate build dir per version.** The switch drives the region defines,
`yamls/<v>` extraction configs, `baserom.<v>.{z64,n64,v64}` selection,
credits text, the JP-only source exclusions (`mncongra.c`/`mnstartup.c`),
and the per-version generated reloc artifacts
(`reloc_data.<v>.h`, `RelocFileTable.<v>.{cpp,h}`, selected via committed
shims that mirror the decomp's `reloc_data.h` selector). The user still
supplies their own ROM; no assets are shipped. Full design + bring-up
notes: `docs/jp_rom_compat_plan_2026-05-17.md`.

## Runtime Logs
After running the game:
- **Game trace log**: `ssb64.log` in the cwd — `port_log()` output (boot sequence, thread creation, frame milestones). Overwritten each run.
- **LUS/spdlog log**: `logs/Super Smash Bros. 64.log` — libultraship logging (resource loading, rendering, errors). Cumulative.
- On Windows the binary lives at `build/Debug/ssb64.exe` (multi-config MSVC), on macOS/Linux at `build/ssb64` (single-config Make/Ninja).

## IDO 7.1 Compiler Patterns
The decompiled C code contains patterns that are artifacts of the original IDO 7.1 MIPS compiler. These are intentional and should be preserved in decomp code:
- Specific register allocation patterns may produce odd-looking variable usage
- `goto` statements used to match branch patterns
- Unusual cast chains or temporary variables to match instruction sequences
- `do { } while (0)` wrappers
- These exist to produce matching assembly output against the original ROM and should NOT be "cleaned up" in the decomp source files

## Compiler Compatibility

**The LP64 `long` class of bug.** The N64 SDK headers type several fixed-width 32-bit fields as `long`, which is 4 bytes under MSVC LLP64 and 8 bytes under clang/gcc LP64. Every such field silently doubles in size on macOS/Linux, corrupting every struct that mirrors N64 file data or that gets shared with libultraship (which uses 32-bit field types internally). A full sweep of `include/` and `src/` under the `\blong\b` pattern found only three real offenders — all fixed:

- **`include/PR/ultratypes.h`** — `u32`/`s32`/`vu32`/`vs32` were `unsigned long`/`long`. Every decomp struct that packed N64 file data is affected. Fixed under `#ifdef PORT && !defined(_MSC_VER)` to use `unsigned int`/`int`. MSVC keeps the original definitions.
- **`include/PR/gbi.h`** — `Mtx_t` was `long[4][4]`. Doubles the matrix from 64 to 128 bytes on LP64, so `guMtxF2L`'s 4-byte-stride int writes land in alternating padding slots and Fast3D's reader sees garbage. The smoking gun was that `libultraship/include/fast/types.h:3` *already* defined `Mtx_t` as `int[4][4]` — LUS and the decomp headers disagreed about the matrix element size, and MSVC accidentally reconciled them because `long == int` under LLP64. On LP64 the two headers diverge and every 3D transform collapses to `x==y, z=0, w=0`. Fixed by aligning `gbi.h`'s `Mtx_t` to `int[4][4]` under `PORT && !_MSC_VER`.
- **`include/PR/gbi.h`** — `Gsetcolor::color`, `TexRect::w{0..3}`, `Hilite::force_structure_alignment`, `Gsetcolor`-family typed union views — all declared as `long` but **never accessed by field name** anywhere in `src/` / `port/` / `libultraship/`. The decomp builds display lists exclusively through the raw `Gwords` view (`g->words.w0`/`w1`), so the typed-struct LP64 drift is invisible and harmless. Left as-is.

**Audit clean at time of writing:**
- `src/` contains no `long`-typed struct fields (comment / macro-name matches only).
- `include/PR/ramrom.h`, `u64gio.h`, `PRimage.h`, `stdlib.h::ldiv_t` are `long`-typed but none of them are referenced by any compiled translation unit — the port never pulls in the N64 ramrom debug I/O path or the SGI image format.
- `include/PR/gu.h`'s `FTOFIX32(x)` macro returns `long`, but every caller assigns the result to an `int` before further use, so the extra width is truncated harmlessly.
- `stdarg.h`'s `(long)list` alignment macros use `long` as a pointer-sized integer; LP64 actually makes that *more* correct than LLP64.
- Every `_Static_assert(sizeof(X) == N)` struct (Bitmap, Sprite, DObjDesc, MObjSub, FTAccessPart, …) compiles clean, which means the reconciled types produce the correct byte layouts on both ABIs.

When adding new decomp sources or vendored SDK headers, re-run this sweep (`grep -rn '\blong\b' decomp/include/ decomp/src/ port/ | grep -v 'long long'`) before assuming the new code is LP64-safe.

Other compiler-compat notes:
- `ultratypes.h` defines `u32`/`s32` as `unsigned int`/`int` under `#ifdef PORT && !defined(_MSC_VER)` (and as `unsigned long`/`long` everywhere else). This is the LP64 fix: on clang/gcc `long` is 8 bytes, which silently corrupts every file-backed N64 struct the decomp touches. MSVC (LLP64) keeps the original SDK definitions.
- `size_t` is provided in the same header via `__SIZE_TYPE__` (GCC/Clang builtin) when `_MIPS_SZLONG` isn't defined, so the decomp can use `size_t` without pulling in the system `<stddef.h>`.
- The `__attribute__((aligned(x)))` macro in `macros.h` is immediately undefined by `#define __attribute__(x)` — this is an IDO compatibility hack. The port will need to fix this for GCC/Clang/MSVC.
- `#ifdef __sgi` guards IDO-specific code paths. The port uses `__GNUC__` or `_MSC_VER` paths.
- Clang ≥16 promotes `-Wimplicit-function-declaration`, `-Wint-conversion`, `-Wincompatible-pointer-types`, `-Wreturn-mismatch`, etc. to errors by default. The decomp relies on IDO-era loose typing for all of these, so `ssb64_game` (the decomp object library) carries a bundle of `-Wno-…` overrides in `CMakeLists.txt`. Keep new decomp compile errors contained to that target — do not spread the overrides to the port layer, which should stay strict.
- `gmColCommandGotoS2` / `SubroutineS2` / `ParallelS2` narrow a 64-bit address to `u32` inside a file-scope `u32[]` initializer. That's legal on N64 (32-bit `void*`) and tolerated by MSVC, but clang/gcc C11 rejects it as a non-constant initializer. Under `PORT && !_MSC_VER` these macros expand to `((u32)0)` — color-script goto targets would need to be patched at runtime if that feature is ever re-enabled.
