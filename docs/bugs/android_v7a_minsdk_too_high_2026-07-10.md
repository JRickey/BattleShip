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
compile-validated).

### UPDATE 2026-07-10 (SYMBOLICATED): corrupted Fast3dWindow/mInterpreter pointer in the render

Captured a clean SIGSEGV under lldb (crash-register capture, ASLR off, break at the
render entry biases the nondeterministic crash toward SIGSEGV). Symbolicated:

```
SIGSEGV invalid permissions, fault addr = 0x3003083c   (data deref of a garbage pointer)
pc = Fast3dWindow::EndFrame()            libultraship/src/fast/Fast3dWindow.cpp:183
       (inlined std::shared_ptr<Fast::Interpreter>::operator->)
lr = port_drain_pending_display_list     port/gameloop.cpp:393
```

`Fast3dWindow::EndFrame()` (line 183 = `mInterpreter->EndFrame();`) faults reading
`mInterpreter` through a **corrupted `Fast3dWindow this` pointer** (~`0x3003083c`). The
corrupt value's SHAPE names the cause: across runs it was `0x2c03083c` then
`0x3003083c` — **low 24 bits `0x03083c` stable, high byte varying**. That is exactly an
**N64 segmented GBI address** `(segment << 24) | offset` (offset `0x03083c`). So a
display-list `w1` / segmented address has leaked into a C++ pointer slot — the reloc/
pointer confusion this bug family is known for, now on ILP32.

This closes the loop on the nondeterminism: the corrupt pointer is dereferenced as an
object; if that garbage address is non-accessible → SIGSEGV (seen), if it's a callable-
but-wrong `Interpreter*`/vtable → jump through it → runaway → StackOverflow/SIGABRT
(also seen). Same corrupted pointer, both crash faces.

The corruption happens DURING the render: `DrawAndRunGraphicsCommands` runs
`mInterpreter->StartFrame()` then `Run(commands)` then `EndFrame()`; `StartFrame`
(earlier) does NOT fault but `EndFrame` (after `Run`) does — so **`Interpreter::Run()`
(the GBI display-list processor) performs an out-of-bounds / wrong-width write that
clobbers the `Fast3dWindow`/`window` pointer with an N64 segmented address**.

Register-level (lldb SIGSEGV capture, disasm cross-checked against unstripped libmain):
- Fault instruction `Fast3dWindow::EndFrame` off 0x3a9d4a: `ldr r0, [r0, #0x3c]` — reads
  `mInterpreter` (member at object offset 0x3c) through `this` in r0.
- **r0 = this = 0x30030800** (corrupt). Across runs the corrupt value is `0xNN030800`
  (low 24 bits `0x030800` stable, high byte varies) — i.e. an N64 segmented address
  `(seg<<24)|offset`, offset `0x030800`. `mInterpreter` deref then faults at r0+0x3c =
  `0xNN03083c`.
- Reached via `port_drain_pending_display_list` (gameloop.cpp:382, the `try{}` call)
  → PLT thunk 0x5945c0 → `DrawAndRunGraphicsCommands` (r0=window, r1=dl, r2=mtx).

NEXT: find the OOB/wrong-width write in `Interpreter::Run` / a GBI command handler (an
ILP32 stride or pointer-width bug that writes a GBI `w1`/unresolved segment address into
an adjacent pointer slot — likely the reloc/SegAddr/fixup family the user flagged). The
written value `0xNN030800` (offset `0x030800`) may identify the source command/data.
Note: HW watchpoints do NOT work under lldb on this qemu/TCG emulator (fire spuriously);
use GDB (gdb-multiarch client → gdbserver) if a data watchpoint on the clobbered slot is
needed to catch the exact write.

### UPDATE 2026-07-10 (watchpoint attempt): TWO crash variants; TCG debugger limits

Tried to catch the exact corrupting write. Findings:
- **The crash has two faces depending on the debugger.** WITHOUT any debugger, 6/6
  launches crash as `SIGABRT` StackOverflow, and the (ART-unwound) stack histogram is
  the ImGui render chain `DrawAndRunGraphicsCommands → Gui::StartFrame →
  ImGuiWMNewFrame → ImGui_ImplSDL2_NewFrame → UpdateMonitors → SDL_GetDisplayDPI`
  (plus `ImGui_ImplOpenGL3_CreateDeviceObjects`, a first-frame GL-init path). The clean
  `SIGSEGV` in `Fast3dWindow::EndFrame` (corrupt `this=0x??030800`) only appears when a
  breakpoint at the render entry perturbs timing. Same underlying corruption, different
  downstream fault. So the dominant/real crash is the StackOverflow in the ImGui path,
  NOT `Fast3dWindow::EndFrame` (that was a debugger-induced variant).
- **Instrumentation guard** (log+bail if `this` low24==0x030800 in Fast3dWindow::
  Start/End/RunGuiOnly) NEVER fired across 6 unperturbed runs — the StackOverflow path
  does not go through those methods, and its corrupt value differs from the debugger's
  `0x??030800`.
- **Debugger tooling is broken on this qemu/TCG emulator** (a real blocker, not a skill
  issue): lldb HW watchpoints fire spuriously; GDB can't speak lldb-server's protocol
  ("Invalid hex digit"); GDB (17.1) segfaults internally against the NDK-r22 android
  `gdbserver`; and it stops on the wrong (main) thread. A data watchpoint to catch the
  write is not achievable here.

**Recommended next environment:** reproduce on REAL armeabi-v7a hardware (where GDB HW
watchpoints work) or a native ILP32 build (x86 32-bit / armhf under a non-TCG debugger),
then watchpoint the clobbered slot. Alternatively, a focused manual audit of ILP32
write sites in `Interpreter::Run` / the reloc/`SegAddr`/fixup path (the value shape is a
GBI segmented address leaking into a pointer). The install fix (committed) is unaffected
and complete; this runtime crash is a separate, still-open ILP32 corruption bug.

### (earlier) it's a CORRUPTED CODE POINTER, not "recursion"

Debugged under lldb (lldb-server on the ARMv7 emulator, box-side NDK client, ASLR
disabled for stable addresses). Key finding: **the crash is nondeterministic** —
across runs it presents as either `SIGABRT` (the `StackOverflowError` seen without a
debugger) OR `SIGSEGV: invalid permissions for mapped object (fault addr =
0x2c03083c)`. "Invalid permissions" at a small ILP32-shaped address means the code
**jumped to / called a corrupted pointer** that lands on a non-executable page.

That reconciles everything: a **bad function/DL/callback pointer** is being called on
the first content frame. Depending on heap/token state the garbage target either (a)
loops back into executable memory → runaway → stack overflow → SIGABRT, or (b) hits a
non-exec page → SIGSEGV. Same root cause; the "infinite recursion" chased earlier was
a *symptom* of the corrupted pointer, not the disease. The nondeterminism (SIGABRT vs
SIGSEGV across identical runs) is the signature of memory/pointer corruption.

**This is an ILP32 pointer-corruption bug** — matching the port's long history of
LP64 token/reloc/fixup issues, which on 32-bit ARM (ILP32, 4-byte pointers) either
don't apply or misfire. Prime suspect: `Interpreter::SegAddr` (interpreter.cpp:4620)
gates reloc-token resolution on `if (w1 <= UINT32_MAX)`. On LP64 a real host pointer
is > UINT32_MAX so it skips the token table; on **ILP32 every value fits in 32 bits**,
so real pointers are also fed to `portRelocTryResolvePointer` and can be misdecoded as
tokens → a wrong pointer used as a DL/function target. Other reloc/fixup call sites
that assume LP64 pointer width should be audited the same way. NEXT: catch the SIGSEGV
under lldb with an intact stack (symbolicate the crashing PC + the call site — the raw
crash PC is in libmain's range, symbolicate offline via addr2line + the `/proc/maps`
libmain base), which names the exact bad-pointer call site.

### (earlier) coroutine theory DISPROVEN — it's per-frame render-path stack growth

An SP-bounds probe in `PortPushFrame` (log current SP vs. this thread's
`pthread_getattr_np` stack bounds each frame) settled it:

```
frame=0 sp=0x995a5ae0 stack=[0x994a2000,+1042K) in_bounds=1 depth_used=3K
frame=1 sp=0x995a5ae0 ... in_bounds=1 depth_used=3K
frame=2 sp=0x995a5ae0 ... in_bounds=1 depth_used=3K
frame=3 sp=0x995a5ae0 ... in_bounds=1 depth_used=3K   <- then StackOverflow mid-frame
```

`PortPushFrame` runs exactly **4 times**, each entering at the *identical* SP,
**in bounds**, only **3K deep** — so the armv7 coroutine swap restores SP
perfectly and there is **no leak between frames and no corrupted/wrong SP**. The
coroutine backend is exonerated. Also NOT re-entrancy: a re-entrancy guard on
`PortPushFrame` never fired. The overflow happens *within frame 3* (the first
frame with real game content): the stack grows from 3K to 1038K inside one
`PortPushFrame` call, in the render path (`drain → DrawAndRunGraphicsCommands →
Gui::StartFrame → ImGui...`). The earlier "repeating SDL_main→PushFrame" backtrace
was unwinder garbage walking the corrupted deep stack, NOT real re-entry.

So it IS genuine deep stack growth (~1 MB) confined to a single render frame,
armv7-only. Exact recursing/deep function still to be pinned down (ART's unwind
over the overflowed stack is unreliable; needs lldb or a probe deeper in the
render path). Leading suspects: ImGui multi-viewport/monitor handling in
`Gui::StartFrame`, or a Fast3D display-list path that is deep only on the first
content frame. NOTE: the section below (JNI-from-coroutine) was the earlier,
now-superseded hypothesis — kept for history.

### (SUPERSEDED) Earlier hypothesis: JNI-from-coroutine infinite recursion

NOT a stack-budget shortfall — bumping the SDL thread stack to 8 MB just made it
overflow a 9 MB stack instead, confirming *unbounded recursion*. Symbolicating the
repeating cycle (unstripped libs in `android/app/build/intermediates/cxx/RelWithDebInfo/*/obj/armeabi-v7a/`,
`llvm-addr2line`) shows the game's per-frame render path re-entering SDL's Android
JNI helpers from inside a `port_coroutine` fiber:

```
SDL_main → PortPushFrame → port_drain_pending_display_list
  → Fast3dWindow::DrawAndRunGraphicsCommands → Gui::StartFrame → Gui::ImGuiWMNewFrame
    → ImGui_ImplSDL2_NewFrame → ImGui_ImplSDL2_UpdateMonitors
      → SDL_GetDisplayDPI → Android_JNI_GetDisplayDPI → [JNI] → (recurses)
```

This is the **same class** as the `SDL_GetDisplayUsableBounds` hazard `port.cpp`
already mitigates (comment at the `SDL_HINT_DISPLAY_USABLE_BOUNDS` block): SDL
Android helpers that Binder-IPC into Java misbehave when called from a coroutine
fiber (the ManagedStack head dangles across `port_coroutine_swap`). `port.cpp`
pre-seeds the *usable-bounds* hint and warms `bHasEnvironmentVariables` on the real
thread, but `ImGui_ImplSDL2_UpdateMonitors` *also* calls `SDL_GetDisplayDPI`, which
hits `Android_JNI_GetDisplayDPI` **directly** (not via a hint), so it is not covered
and still re-enters JNI every frame. `ImGui_ImplSDL2_NewFrame` calls
`UpdateMonitors()` unconditionally each frame, so this fires on every GFX-coroutine
frame. arm64 survives because its coroutine JNI transition happens to succeed.

Likely fix direction (not yet implemented): stop ImGui's per-frame monitor
enumeration from doing JNI on Android — e.g. populate `platform_io.Monitors` once on
the JVM-attached SDL_main thread and skip `UpdateMonitors()` on subsequent frames,
or disable the multi-viewport monitor path on Android. A stack bump is NOT a fix.
Expect more than one JNI-from-coroutine call site (manifest-env/getenv was one; DPI
is another) — the durable fix is to keep the game's coroutines off SDL's Android JNI
helpers entirely.

### Also: verify arm64-v8a
The shipped arm64-v8a build has NOT been confirmed to reach gameplay in this
investigation — the user reports it works on real devices/emulators. If it was only
install/launch-tested, re-check it against this same coroutine-JNI path.

## Audit hook

Any per-ABI `minSdk` must be justified by the *lowest* API each native
dependency needs on that ABI, not a copy-pasted default. For 32-bit ARM the
binding limit is libc++ `<fstream>` → `fseeko`/`ftello` at API 24. Forcing a
specific SDL audio driver by name pins the floor to that driver's availability
and disables SDL's fallback — gate driver hints by `android_get_device_api_level()`.
