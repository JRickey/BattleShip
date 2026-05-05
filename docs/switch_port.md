# SSB64 → Nintendo Switch Homebrew (.nro) Port Plan

## Summary

Port the libultraship-based SSB64 PC port to Nintendo Switch homebrew. The Switch homebrew
stack runs **OpenGL 2.1 via Mesa/nouveau** on top of SDL2, so the existing OpenGL rendering
backend is largely reusable. A community developer (Dire-Tomatoes/libultraship-switch-patches)
has already ported libultraship for Ship of Harkinian — this plan follows the same approach.

---

## Phase 1: Build System + Toolchain

**Goal:** CMake cross-compiles for Nintendo Switch via devkitPro.

- Add NintendoSwitch platform detection in `libultraship/CMakeLists.txt`
- Create `libultraship/cmake/dependencies/switch.cmake` — find SDL2, glad, EGL
- Add `-D__SWITCH__` + `USE_OPENGLES=ON` in `libultraship/src/CMakeLists.txt`
- Add Switch link flags (specs file, NRO output) in root `CMakeLists.txt`
- Source file filtering in `fast/CMakeLists.txt` — Switch falls into the default
  (include OpenGL, exclude D3D/Metal), no changes needed.

**Files to touch:**
- `libultraship/CMakeLists.txt` — +6 lines
- `libultraship/cmake/dependencies/switch.cmake` — New, ~30 lines
- `libultraship/src/CMakeLists.txt` — +10 lines
- `CMakeLists.txt` (root) — +10 lines

---

## Phase 2: Rendering Backend — Adapt OpenGL for Switch

**Goal:** `gfx_opengl.cpp` (1052 lines) renders correctly on Switch.

- **Replace GLEW with glad** — GLEW's dlopen path doesn't work on Switch;
  glad uses `SDL_GL_GetProcAddress`
- **Force GL 2.1 Core profile** — `SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2)`,
  minor=1, core profile
- **Remove/downgrade unavailable GL calls** — `glDepthBoundsEXT`, `glClampColor`, etc.
  The GLES path in the backend already handles this
- **GLSL shader tweaks** — verify GLES `#version 300 es` path compiles, add
  `precision mediump float` if needed for Switch GPU
- **Wire up in Fast3dWindow** — add `FAST3D_SDL_OPENGL` case for `#ifdef __SWITCH__`

**Files to touch:**
- `libultraship/src/fast/backends/gfx_opengl.cpp` — GLEW→glad, GL version
- `libultraship/src/fast/Fast3dWindow.cpp` — Switch backend registration
- `libultraship/src/fast/shaders/opengl/default.shader.glsl` — GLES precision

---

## Phase 3: Window Manager — Adapt SDL2 for Switch

**Goal:** `gfx_sdl2.cpp` (757 lines) creates windows, handles input, swaps buffers.

- `Init()` — no DPI hints, skip resizable window, SDL_GL_CreateContext works via Mesa
- `HandleEvents()` — add SDL_CONTROLLERDEVICEADDED, SDL_FINGERDOWN/UP/MOTION
- `SetFullscreen()` → no-op (always fullscreen)
- Mouse/getters → no-ops (no mouse on Switch)
- `GetDisplaySize()` → query `appletGetOperationMode()` (1920x1080 docked,
  1280x720 handheld)
- Keyboard callbacks → stubs (virtual keyboard handled in Phase 5)

**Files to touch:**
- `libultraship/src/fast/backends/gfx_sdl2.cpp` — ~30 #ifdef blocks

---

## Phase 4: Platform Glue — Paths, Audio, Config

**Goal:** Game finds assets, saves config, produces audio.

- `Context.cpp` — add Switch blocks in `GetAppBundlePath()` (return SD card path),
  `GetAppDirectoryPath()` (return `SDL_GetPrefPath`), OTR-not-found exit
- `Audio.cpp` — Switch falls through to SDL backend, no change needed
- `Config.cpp` — Switch hits SDL fallback for audio default, force OpenGL window backend
- `coroutine_posix.cpp` — verify ucontext.h availability on Switch; write `coroutine_switch.cpp` if needed
- `port_watchdog.cpp` — add Switch aarch64 crash register extraction
- `native_dialog.cpp` — `#else` fallback returns `""`, acceptable for Switch
- `first_run.cpp` — skip on-device ROM extraction (pre-extract on PC)

**Files to touch:**
- `libultraship/src/ship/Context.cpp` — +20 lines
- `libultraship/src/ship/config/Config.cpp` — +6 lines
- `port/port_watchdog.cpp` — +15 lines
- `port/coroutine_posix.cpp` or `port/coroutine_switch.cpp` — verify or implement
- `port/native_dialog.cpp` — ensure fallback works
- `port/first_run.cpp` — ROM extraction skip for Switch

---

## Phase 5: Switch-Specific Features

**Goal:** Native-feeling quality-of-life features.

- `libultraship/src/port/switch/SwitchImpl.h` — New: `Ship::Switch` class API
- `libultraship/src/port/switch/SwitchImpl.cpp` — New: display mode detection,
  system fonts via `plGetSharedFontByType()`, virtual keyboard, CPU overclock via
  `pcvSetClockRate`
- `libultraship/src/port/switch/SwitchPerformanceProfiles.h` — New: CPU clock presets
- `Gui.cpp` — add Switch block for ImGui init (GL 2.1, `#version 140`), 2x UI scale
  for handheld, disable viewports
- `StatsWindow.cpp` — add Switch RAM reporting via libnx `svcGetInfo()`
- Audio reinit on sleep resume — detect sleep/wake via `appletGetAppletType()`

**Files to touch:**
- `libultraship/src/port/switch/SwitchImpl.h` — New, ~40 lines
- `libultraship/src/port/switch/SwitchImpl.cpp` — New, ~250 lines
- `libultraship/src/port/switch/SwitchPerformanceProfiles.h` — New, ~30 lines
- `libultraship/src/ship/window/gui/Gui.cpp` — +15 lines

---

## Phase 6: Input — Controller Mappings

**Goal:** Joy-Con / Pro Controller work for gameplay and menus.

- Add Switch SDL game controller mappings (A/B/X/Y, sticks, D-Pad, Home, Capture)
- Map N64 controls: A→A, B→B, C-buttons→Right Stick, Z→ZR, R→R, L→L,
  Start→+, D-Pad→D-Pad
- Add touch screen handling for menus

**Files to touch:**
- `libultraship/src/ship/controller/controldevice/controller/mapping/sdl/` — new mappings
- `libultraship/src/fast/backends/gfx_sdl2.cpp` — touch events

---

## Phase 7: Asset Pipeline

**Goal:** ROM-extracted `.o2r` assets on Switch SD card.

- Run Torch extraction on PC: `torch extract baserom.us.z64`
- Bundle `ssb64.o2r` + `f3d.o2r` alongside `.nro` on SD card
- Optionally embed as ROMFS for cleaner distribution

**Implementation (already done):**
- `CMakeLists.txt` (root) — Torch section wrapped in `if(NOT CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")` (Phase 1)
- `port/first_run.cpp` — Switch guards in `ExtractAssetsIfNeeded` + `RunFirstRunWizard` (Phase 4)
- `CMakeLists.txt` (root) — New `BundleSwitch` custom target copies `.nro` + `.o2r` + `gamecontrollerdb.txt` into `switch_sd/` for easy deployment
- `scripts/build-switch.sh` — Full orchestration: native Torch extract → cross-compile → bundle

**Deploy:**
```bash
# 1. Extract assets (on PC with ROM)
./scripts/build-switch.sh

# 2. Copy switch_sd/ to SD card root
cp -r build-switch/switch_sd/* /path/to/sdcard/

# 3. On Switch: Launch via Homebrew Menu
#    sdmc:/switch/BattleShip/BattleShip.nro
```

**Files:**
- `CMakeLists.txt` (root) — +28 lines (BundleSwitch target)
- `scripts/build-switch.sh` — New, ~110 lines

---

## Phase 8: Testing & Polish

**Goal:** Stable 60fps gameplay on Switch.

- Handheld + docked mode testing
- Menu → CSS → match → results loop
- 4-player matches on all stages
- Sleep/resume cycle
- Edge cases: controller disconnect, SD card removal
- Performance tuning / CPU overclock profiles

---

## Estimated Effort

| Phase | Complexity | Est. Hours |
|-------|-----------|------------|
| 1. Build System | Low | 4-6 |
| 2. OpenGL Backend Adapt | Medium | 8-12 |
| 3. SDL2 Window Adapt | Medium | 8-12 |
| 4. Platform Glue | Low-Medium | 6-10 |
| 5. Switch Features | Medium | 10-16 |
| 6. Controller Mappings | Low | 4-6 |
| 7. Asset Pipeline | Low | 2-4 |
| 8. Testing & Polish | Medium | 8-12 |
| **Total** | | **50-78 hours** |

---

## Prerequisites

```bash
# Install devkitPro toolchain
sudo dkp-pacman -S switch-dev        # devkitA64 + libnx
sudo dkp-pacman -S switch-sdl2       # SDL2 portlib
sudo dkp-pacman -S switch-glad       # glad GL loader
```

## Reference

- [Dire-Tomatoes/libultraship-switch-patches](https://github.com/Dire-Tomatoes/libultraship-switch-patches)
  — existing Switch port of libultraship (Ship of Harkinian)
- [devkitPro switch-dev](https://devkitpro.org/wiki/Getting_Started) — toolchain
