# Android armeabi-v7a APK "can't install" — minSdk floor too high

**Status:** FIXED (working tree; needs commit + a release build to ship)

## Symptoms

A user reported the `armeabi-v7a` APK from the v1.5 GitHub Release "isn't even
able to be installed" on their (pre-Android-8.0) device. The install fails
before the app ever runs.

Reproduced on a real ARMv7 API-25 (Android 7.1) emulator:

```
adb install BattleShip-android-armeabi-v7a.apk
Failure [INSTALL_FAILED_OLDER_SDK: ... Requires newer sdk version #26
        (current version is #25)]
```

## Root cause

`android/app/build.gradle.kts` defaulted `minSdk = 26` (Android 8.0). The value
was copy-pasted into the first Gradle skeleton (`6f09bdd7`) with the comment
"matches CMake -DANDROID_PLATFORM=android-26" — a circular justification: AGP
*derives* the native platform from `minSdk`, there is no separate
`-DANDROID_PLATFORM` anymore. It was never an analyzed floor.

Any device below API 26 rejects the APK at parse time with
`INSTALL_FAILED_OLDER_SDK`. armeabi-v7a devices are exactly the old/budget
hardware most likely to be < API 26, so the 32-bit build shipped with a floor
that excluded much of its own target audience.

Audit of what actually constrains the floor:

| Layer | Real minimum API |
|---|---|
| NDK r29 toolchain | 21 (Android 5.0) |
| SDL `org.libsdl.app` Java | stock, self-guards for old APIs |
| **libc++ `<fstream>` (via libultraship)** | **24** — see below |
| **Custom Java `java.nio.file` usage** | **26** — see below (runtime crash, NOT caught by lint) |
| Forced AAudio audio driver (`port.cpp`) | 26 — but only if left forced |

Two real constraints, both removable/lowerable from 26:

1. **AAudio (API 26).** `port/port.cpp` forced
   `SDL_SetHint(SDL_HINT_AUDIODRIVER, "aaudio")`. AAudio's `libaaudio.so` only
   exists at API 26+. The in-code comment claimed "SDL falls back to OpenSL ES
   if the device rejects it" — **false**: with a driver name pinned,
   `SDL_AudioInit` does not fall back (it fails with "Audio target 'aaudio' not
   available"). This was the only thing making 26 look mandatory.

2. **`fseeko`/`ftello` (API 24).** libc++ `<fstream>` calls `::fseeko`/`::ftello`.
   In Bionic these are `__INTRODUCED_IN(24)` on ILP32 (32-bit ARM). Below API 24
   they are undeclared and the native build fails to compile:
   `error: no member named 'fseeko' in the global namespace`. This is the hard
   floor — it cannot be lowered without rewriting file I/O across the whole
   libultraship dependency tree. (This is why `minSdk=21` compiles fine for
   arm64-v8a/LP64 but breaks armeabi-v7a: LP64 gets these symbols earlier.)

So the lowest buildable-and-audible floor for the 32-bit ARM target is **API 24
(Android 7.0)**.

3. **`java.nio.file` (API 26), runtime.** Lowering `minSdk` to 24 exposed a
   *latent* crash the old floor of 26 had masked: `AssetExtractor.java` and
   `PackImporter.java` used `java.nio.file.Files` / `File.toPath()` /
   `StandardCopyOption`. The whole `java.nio.file` package is API 26+, so on an
   API-24/25 device the app *installs* but crashes at first boot:
   `java.lang.NoSuchMethodError: No virtual method toPath()...` in
   `AssetExtractor.extractIfNeeded`. **`lintVitalRelease` did NOT flag this** —
   the minSdk-24 build passed lint clean and only the on-device run caught it, so
   an install check is not sufficient; the app must actually be launched. Fixed
   by rewriting the three call sites in plain `java.io` (stream read/write;
   `File.renameTo` with a delete+rename fallback for the atomic pack publish).

## Fix

- **`port/port.cpp`** — gate the AAudio hint behind
  `android_get_device_api_level() >= 26`; below 26, leave the hint unset so SDL
  auto-selects OpenSL ES (works to API 9). Modern devices keep low-latency
  AAudio; API 24/25 devices get working audio instead of a dead audio subsystem.
  `<android/api-level.h>` is included at *file scope* under `#if
  defined(__ANDROID__)` — for `__ANDROID_API__ < 29` that header defines
  `android_get_device_api_level` as a `static __inline`, which C++ forbids
  inside a function body, so the include cannot be local to the function.

- **`android/app/build.gradle.kts`** — default `ssb64.minSdk` `26 → 24`.
  Drops the install floor from Android 8.0 to Android 7.0.

arm64-v8a is unaffected (its own `minSdk` handling is independent and LP64 has
`fseeko`/`ftello` regardless).

## Verification

Both APKs installed on a real `armeabi-v7a,armeabi` API-25 emulator (Google APIs
ARM EABI v7a image, booted with the legacy Android Emulator 30.2.2 — modern
emulator 36.x refuses ARM images with "CPU Architecture 'arm' is not supported
by the QEMU2 emulator", on both macOS and Linux):

- **Shipped APK (minSdk 26):** `INSTALL_FAILED_OLDER_SDK ... Requires newer sdk
  version #26 (current version is #25)` — reproduces the user's failure.
- **Fixed APK (minSdk 24):** installs successfully.

The ARMv7 emulator was run on the x86 Linux GPU box (see CLAUDE.md GPU notes),
not the Mac: the Mac path is `emulator (Rosetta) → qemu-system-armel (TCG)`,
a double translation that is unusably slow; the x86 box drops the Rosetta layer.
Note KVM does **not** accelerate an ARM guest on an x86 host (cross-arch), so the
ARMv7 image is still full TCG software emulation — installs of the ~111 MB APK
take several minutes. An x86 API-25 image was also tried but has no ARM
translation (`abilist=x86` only, `ro.dalvik.vm.native.bridge=0`), so it rejects
armeabi-v7a APKs with `INSTALL_FAILED_NO_MATCHING_ABIS` — an x86 image cannot
substitute for a true ARM device here.

## Known follow-up: native game crash on the ARMv7 emulator

With both fixes in, the APK installs, launches, and renders its Java ROM-picker
UI, and asset extraction completes — but entering the native game
(`BattleShipActivity` → SDL `nativeRunMain`) dies with a
`java.lang.StackOverflowError (stack size 1038KB)` on the `SDLThread`, ~10 s in.
The game's native `main()` runs on SDL's Android thread, whose stack is only
~1 MB (`1038KB`), and 32-bit ARM init overflows it.

**Confirmed a real bug, not an emulator artifact.** First seen with the emulator's
forced CheckJNI (`ro.kernel.android.checkjni=1`) turning the pending
`StackOverflowError` into a `SIGABRT` at a JNI boundary, which suggested a CheckJNI
amplification. But re-running the emulator with `-no-jni` (CheckJNI off, verified
`ro.kernel.android.checkjni` empty) reproduced the **same** `StackOverflowError` —
so the ~1 MB SDL-thread stack is genuinely exhausted by armeabi-v7a native init.

This is **separate from and not a regression of** the minSdk fix — it is the first
actual *runtime* exercise of the armeabi-v7a build (the ILP32 port was only ever
compile-validated). Open follow-ups:
- The likely fix is enlarging the SDL main-thread stack (SDL creates `SDLThread`
  in `SDLActivity.java` with the ~1 MB Android default; desktop `main()` assumes
  ~8 MB). Options: bump the vendored SDL thread stack, or move heavy init off the
  SDL thread.
- **Verify whether the shipped arm64-v8a build actually reaches gameplay** — if
  arm64 was only ever install/launch-tested, it may hit the same 1 MB stack wall.

## Audit hook

Any per-ABI `minSdk` must be justified by the *lowest* API each native
dependency needs on that ABI, not a copy-pasted default. For 32-bit ARM the
binding limit is libc++ `<fstream>` → `fseeko`/`ftello` at API 24. Forcing a
specific SDL audio driver by name pins the floor to that driver's availability
and disables SDL's fallback — gate driver hints by `android_get_device_api_level()`.
