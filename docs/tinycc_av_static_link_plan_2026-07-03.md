# TinyCC Antivirus Flagging — Static-Link Plan + Windows AV Test Protocol

**Date:** 2026-07-03
**Status:** Plan approved, implementation + testing handed off to Windows agent
**Branch:** `claude/tinycc-antivirus-flagging-6hi2j3`

---

## 1. Problem

The Windows modding package trips antivirus (Microsoft Defender false
positives, per `scripts/package-windows.ps1` header). Two distinct detection
layers are in play:

1. **Static signatures on the TCC binaries.** Malware families bundle TinyCC
   to compile payloads on victim machines, so AV vendors carry detections
   keyed to tcc.dll / tcc.exe hashes and byte patterns. We ship `tcc.dll`
   next to `BattleShip.exe` (the exe imports it at process start — see the
   `.tcc/` staging block in the root `CMakeLists.txt`).
2. **Behavioral heuristics.** The ScriptLoader (libultraship fork, `ssb64`
   branch) compiles each mod's amalgamated source **to a temp DLL on disk and
   loads it** (`port/port.cpp` ~line 1052). "Process writes a fresh unsigned
   DLL to temp and immediately loads it" is a classic malware behavior
   pattern on its own.

Current mitigation is packaging-level only: the standard Windows zip disables
scripting (`SSB64_ENABLE_SCRIPTING` off by default); modders opt into a
separate `-modding` zip that carries the flagged files.

### Rejected alternative: bundling a C interpreter

Considered and rejected. An interpreter (PicoC et al.) would dodge the
signatures, but the mod system's three pillars all assume mods are native
code:

- `mod_install_hook` → funchook patches engine `.text` to jump to the
  replacement, which must be a real function pointer with an **arbitrary
  signature**. Interpreted functions have no machine-code address; faking it
  needs libffi-style closures — runtime W→X codegen again, i.e. the same
  heuristic class we're escaping.
- Event listeners pass native function pointers to `REGISTER_LISTENER`.
- Mods cast `void*` payloads to PORT-layout decomp structs (`FTStruct*`,
  `FTAttackColl*`) and include real engine headers — an embeddable C
  interpreter can neither parse those headers nor guarantee host-ABI struct
  layout.

Plus a 100×+ interpretation penalty on detoured hot-path engine functions.
If a sandboxed-mods redesign is ever wanted, WASM (wasm3/WAMR interpreter
mode) is the better target than a C interpreter — but that is a v2 of the mod
API, not a fix for this bug.

---

## 2. The plan: static-link libtcc + compile mods to memory

Attack the two artifacts AV actually matches on:

- **No `tcc.dll` on disk** → the known-bad file hash disappears from the
  package.
- **No temp DLL drop-and-load** → the flagged behavior disappears; mod code
  lives in `tcc_relocate`'d memory inside the (signed) game process.

Residual risk: in-memory JIT pages (libtcc relocation) and funchook `.text`
patching can still bother the most aggressive *behavioral* heuristics. That
is exactly what the test protocol in §4 measures — do not skip the runtime
tests.

### Phase 1 — libultraship fork (`ssb64` branch): ScriptLoader memory mode

All changes in the ScriptLoader (the temp-DLL compile+load path):

1. `tcc_set_output_type(s, TCC_OUTPUT_MEMORY)` instead of DLL output.
2. **Symbol provisioning.** Mod DLLs currently link engine exports via the
   build-generated `.tcc/lib/BattleShip.def` import library. Memory mode
   resolves at `tcc_relocate` time instead:
   - Keep the build-time `tcc -impdef` step — the `.def` is now consumed as a
     **text list of export names**, not an import lib.
   - At `CompileAll`, parse the `.def`, resolve each name against the running
     process (`GetProcAddress(GetModuleHandle(NULL), name)` on Windows —
     `ENABLE_EXPORTS` keeps the exe's export table; `dlsym(RTLD_DEFAULT,…)`
     on Unix — we already link `-Wl,-export-dynamic`), and register it with
     `tcc_add_symbol(s, name, addr)`. The existing `SymbolResolver` machinery
     in `port/mods/` does the same lookup and can likely be reused.
3. `tcc_relocate(s)` then `tcc_get_symbol(s, "ModInit")` /
   `"ModExit"` instead of `LoadLibrary` + `GetProcAddress`.
4. **Hot reload / unload:** replace `FreeLibrary` with `tcc_delete(s)` (frees
   the relocated image). Ordering already holds: HookManager removes the
   mod's owned hooks and `ModExit` unregisters listeners before the image is
   freed — verify nothing else caches pointers into mod memory across reload.
5. Keep `-B <.tcc dir>` so tcc finds `tccdefs.h`, `include/`, and
   `lib/libtcc1.a`. **libtcc1.a is still required at runtime** — memory mode
   pulls runtime helpers (long-long ops, alloca, …) from it during relocate.
6. Make memory mode the single path on **all three desktop platforms**, not a
   Windows special case. On macOS this also removes the unsigned-mod-dylib
   `dlopen`, so `com.apple.security.cs.disable-library-validation` in
   `cmake/macos_entitlements.plist` should become droppable — but **verify on
   hardware before removing it**; `allow-jit` /
   `allow-unsigned-executable-memory` stay (funchook + libtcc relocation
   still need them; see
   `docs/bugs/funchook_macos_arm64_patch_anchor_2026-06-15.md`).

### Phase 2 — superproject `CMakeLists.txt`

In the `.tcc/` staging + def-generation blocks (~lines 1022–1115) and the
tinycc FetchContent setup:

1. Build **libtcc STATIC** and link it into `BattleShip` (drop the
   `copy_if_different $<TARGET_FILE:libtcc>` step — no tcc.dll next to the
   exe, and the exe no longer imports it).
2. Windows `.tcc/` staging slims down:
   - **Keep:** `include/` (both tinycc include dirs), `lib/libtcc1.a`
     (still recompile it via `tcc.exe` — MSVC COFF output is unreadable by
     tcc; the tcc.exe *build tool* never ships), `lib/BattleShip.def`
     (now a runtime data file for symbol provisioning).
   - **Drop:** `lib/dllcrt1.o`, `lib/dllmain.o` (DLL-output-only CRT stubs;
     remove them from the `tcc -ar` bundling too).
3. Linux/macOS staging blocks are already header+libtcc1-only; no change
   beyond the static-link switch.
4. `scripts/package-windows.ps1`: the "Staging TCC scripting runtime" step
   still copies `.tcc/` — confirm the staged tree contains **no PE
   executables** afterward (`Get-ChildItem -Recurse .tcc | Where Extension
   -in '.dll','.exe'` must be empty). Update the header comment describing
   the Defender rationale.
5. `scripts/package-macos.sh` + `.github/workflows/release.yml`: no
   structural change expected; keep the standard/modding split **until the
   test protocol below passes**, then folding modding back into the standard
   package becomes a follow-up decision.

### Phase 3 — validation gates (before any packaging changes ship)

- Full build on all three platforms, `-DDISABLE_SCRIPTING=ON` and `OFF`.
- All three workspace mods (`template` heartbeat, `hooktest` detour,
  `playertint` fighter events) load, run, and **hot-reload** cleanly from
  both unpacked-folder and `.o2r` form. Watch `ssb64.log` for tcc compile
  errors — mod compile failures surface at load time.
- Then run §4 on the Windows machine.

---

## 3. Windows agent — scope of your takeover

1. Implement Phase 1 in the libultraship fork (`JRickey/libultraship`,
   branch `ssb64`), Phase 2 in this repo on the branch above, bump the
   submodule pointer (workflow in `CLAUDE.md` → Parallel Sessions).
2. Run the full test protocol in §4 on the Windows machine.
3. Record results in `docs/tinycc_av_test_results_<YYYY-MM-DD>.md` using the
   template in §5 and link it from this doc.
4. If clean: file the Microsoft WDSI submission (§4.6) and note the
   submission ID in the results doc.
5. On completion, add a `docs/bugs/` entry
   (`tinycc_av_flagging_<date>.md`) per repo convention and link it from
   `docs/bugs/README.md`.

---

## 4. AV test protocol (Windows machine)

Run on default consumer Defender settings — real-time protection ON,
cloud-delivered protection ON, automatic sample submission ON. That is what
players run. Do **not** add any Defender exclusions for the test directories.

### 4.0 Prep + positive control

The positive control validates the rig: the **current** (pre-change) modding
zip must flag. If it doesn't, a "clean" result for the new build means
nothing.

```powershell
Update-MpSignature
Get-MpComputerStatus | Select AMEngineVersion, AntivirusSignatureLastUpdated,
    RealTimeProtectionEnabled, MAPSReporting, SubmitSamplesConsent
& "$env:ProgramFiles\Windows Defender\MpCmdRun.exe" -ValidateMapsConnection

# Positive control: stage the CURRENT release's modding zip and scan it.
Start-MpScan -ScanType CustomScan -ScanPath "C:\avtest\old-modding"
Get-MpThreatDetection | Sort InitialDetectionTime -Desc | Select -First 5
```

Record the detection name(s) hit on tcc.dll — that's the signature we're
trying to shed. If real-time protection quarantines the files during
staging, that itself is the positive result; note it and restore.

### 4.1 Static scan of the new package

```powershell
Start-MpScan -ScanType CustomScan -ScanPath "C:\avtest\new-modding"
Get-MpThreatDetection | Sort InitialDetectionTime -Desc | Select -First 5
```

Expected: no detections (no tcc.dll to match).

### 4.2 Mark-of-the-Web + SmartScreen

Detection differs for internet-sourced files. Either download the built zip
from a real URL (GitHub Actions artifact / Releases) on the test machine, or
stamp MotW manually:

```powershell
Set-Content -Path .\BattleShip-windows-modding.zip -Stream Zone.Identifier `
    -Value "[ZoneTransfer]`nZoneId=3"
```

Extract **with Explorer** (propagates MotW to contents), launch
`BattleShip.exe` by double-click. Record two *separate* outcomes:

- **Defender detection** — the false positive we're hunting.
- **SmartScreen "Windows protected your PC"** — a *reputation* warning, not a
  detection. Unsigned or fresh binaries get this regardless of content; only
  code-signing + accumulated downloads clears it. Note it, don't count it as
  a failure of this change.

### 4.3 Runtime behavioral test — the one that matters

This change trades "drop temp DLL + LoadLibrary" for in-process JIT memory.
Behavior monitoring only judges that while the process runs. On the
MotW-launched build:

1. Boot the game fully.
2. Ensure all three workspace mods are in `mods/` — **`hooktest`
   especially** (funchook patching engine `.text` is the other
   heuristic-adjacent behavior to exercise).
3. Hot-reload the mods from the Mods menu several times (repeated
   compile/relocate cycles).
4. Play a few minutes of a match with `playertint` active, exit cleanly.

Then interrogate Defender:

```powershell
Get-MpThreat
Get-MpThreatDetection
Get-WinEvent -LogName "Microsoft-Windows-Windows Defender/Operational" -MaxEvents 100 |
    Where-Object { $_.Id -in 1116,1117,1015 } |
    Format-List TimeCreated, Id, Message
```

(1116 = detection, 1117 = action taken, 1015 = suspicious behavior.) Also
check Windows Security → **Protection history** in the UI — some
cloud-delivered verdicts appear there without a matching local event.

### 4.4 Baseline: standard (non-modding) package

Run 4.1–4.3 (minus the mod steps) on the scripting-disabled zip too. If
*that* flags, the problem isn't TCC and we're chasing the wrong fix.

### 4.5 Multi-engine sweep: VirusTotal

Upload `BattleShip.exe` and both zips to virustotal.com. Caveats:

- Uploads are shared with all vendors/researchers — fine for this project,
  but it is public.
- VT runs command-line engine builds **without** cloud/behavioral
  components: VT-clean ≠ Defender-clean on a real machine, and 1–2 hits from
  obscure engines are noise. Weight Defender (our test box) and whichever
  products users actually reported.

Record: total engines flagging, which engines, detection names. Compare
old-modding vs new-modding vs standard.

### 4.6 Pre-clear with Microsoft

Independent of local results, submit the final `BattleShip.exe` + modding zip
at <https://www.microsoft.com/en-us/wdsi/filesubmission> as a **software
developer / false positive** submission. An analyst verdict gets baked into
definitions and protects against a later cloud-signature regression. Record
the submission ID.

### 4.7 Acceptance criteria

The change passes when **all** hold:

| # | Criterion |
|---|-----------|
| 1 | Positive control (old modding zip) IS detected by the rig |
| 2 | New modding package: zero Defender detections on static scan |
| 3 | New modding package: zero Defender detections/behavior events through boot + mod load + 3× hot reload + gameplay + exit (MotW launch) |
| 4 | No PE executables under staged `.tcc/` (`.dll`/`.exe` sweep empty) |
| 5 | VT engine-hit count for new modding package ≤ standard-package baseline |
| 6 | All 3 workspace mods functionally verified (heartbeat log, hook fires, tint renders) — AV-clean but broken is a fail |

SmartScreen reputation warnings are recorded but excluded from pass/fail.

### 4.8 Verdict drift

Cloud protection and reputation mean verdicts move: clean at release can flag
a week later, and every build's hash resets file-level reputation. So:

- Re-check VT on the shipped artifacts ~1–2 weeks after release.
- Re-run this protocol per release (it's cheap once scripted — consider
  committing a `scripts/test-av.ps1` that stages, stamps MotW, scans, and
  dumps the event log to a report).
- Longer term: code-sign the releases. Reputation then attaches to the
  certificate, not the per-build hash, and compounds with the WDSI clearance.

---

## 5. Results template

Copy into `docs/tinycc_av_test_results_<YYYY-MM-DD>.md`:

```markdown
# TinyCC AV Test Results — <date>

Build under test: <commit sha / CI run link>
Defender engine/signature versions: <from Get-MpComputerStatus>
Machine: <Windows version, edition>

| Test | Package | Result | Detection name(s) / notes |
|------|---------|--------|---------------------------|
| 4.0 positive control | old modding | | |
| 4.1 static scan | new modding | | |
| 4.2 MotW launch | new modding | | SmartScreen: yes/no (recorded, not pass/fail) |
| 4.3 runtime + hot reload | new modding | | |
| 4.4 baseline | standard | | |
| 4.5 VirusTotal | old modding / new modding / standard | x/70, y/70, z/70 | engines + names |
| 4.6 WDSI submission | new modding | submitted | ID: |

Mod functional checks: template <ok?> / hooktest <ok?> / playertint <ok?>
Acceptance criteria: <n>/6 met
Follow-ups: <...>
```
