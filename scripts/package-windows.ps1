# Builds BattleShip as a self-contained Windows release zip.
#
# Output: <repo-root>\dist\BattleShip-windows.zip
#
# Layout produced (extracted):
#   BattleShip\
#     BattleShip.exe             — main executable
#     torch.exe                  — sidecar for first-run extraction
#     f3d.o2r                    — Fast3D shader archive (ROM-independent)
#     config.yml                 — Torch extraction config
#     yamls\us\*.yml             — Torch extraction recipes
#     gamecontrollerdb.txt       — SDL controller mappings
#     SDL2.dll                   — runtime dependency (vcpkg-bundled)
#     <other vcpkg DLLs>         — picked up by Get-ChildItem from build dir
#
# The default US Windows package includes TCC scripting support. It statically
# links libtcc into BattleShip.exe and stages only TinyCC headers plus libtcc1.a
# under .tcc, avoiding the tcc.dll false-positive signature while ScriptLoader
# relocates mod code in memory.
#
# Portable: drop the extracted folder anywhere and run BattleShip.exe.
# Save data and config (ssb64_save.bin, BattleShip.cfg.json, logs/) land
# next to the .exe in the extraction directory — move the folder, the
# saves move with it. BattleShip.o2r is NOT bundled; the first-run wizard
# extracts it from the user's ROM into the same directory as BattleShip.exe.
#
# We intentionally do NOT pass -DNON_PORTABLE=ON. NON_PORTABLE bakes
# CMAKE_INSTALL_PREFIX into libultraship's install_config.h at configure
# time, and CMake resolves any relative prefix against the configure cwd —
# so on the GitHub Actions runner that bakes the runner workspace path
# (e.g. "D:/a/BattleShip/BattleShip/BattleShip"), which v0.7.2 shipped
# and which crashed user machines whose D: drive returned ERROR_NOT_READY
# when libultraship probed it. Building portable side-steps the entire
# class of bug: the runtime resolves resource paths via GetModuleFileNameW
# (LUS Context::GetAppBundlePath, _WIN32 branch) and saves via the same
# mechanism, so the only paths the binary ever touches are exe-relative.

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
# ROM version: us (default) or jp. The JP build is a SEPARATE
# application — own .exe name, zip, app-data dir — so a user can keep
# both and they never touch each other's ROM/o2r/saves. $AppName
# mirrors CMake SSB64_APP_NAME / OUTPUT_NAME. US keeps the historical
# "BattleShip" identity so existing links / the in-app updater are
# unaffected.
$Ver = if ($env:SSB64_VERSION) { $env:SSB64_VERSION } else { "us" }
if ($Ver -ne "us" -and $Ver -ne "jp") { Write-Error "SSB64_VERSION must be us|jp"; exit 1 }
if ($env:SSB64_ENABLE_SCRIPTING) {
    $EnableScripting = $env:SSB64_ENABLE_SCRIPTING -in @("1", "ON", "TRUE", "YES", "on", "true", "yes")
} else {
    $EnableScripting = $Ver -eq "us"
}
if ($EnableScripting -and $Ver -eq "jp") {
    Write-Error "JP source modding is not release-supported yet; the TCC mod API still compiles mods with US region defines."
    exit 1
}
$DisableScripting = if ($EnableScripting) { "OFF" } else { "ON" }
$PackageFlavor = if ($EnableScripting) { "scripting" } else { "standard" }
$ZipSuffix = "-windows"
$BuildDir = Join-Path $Root "build-bundle-win-$Ver-$PackageFlavor"
$DistDir = Join-Path $Root "dist"
$AppName = if ($Ver -eq "jp") { "BattleShip-JP" } else { "BattleShip" }
$StageName = $AppName
$StageDir = Join-Path $DistDir $StageName
$ZipPath = Join-Path $DistDir "$AppName$ZipSuffix.zip"
$Jobs = if ($env:NUMBER_OF_PROCESSORS) { [int]$env:NUMBER_OF_PROCESSORS } else { 4 }

function Write-Step($msg) { Write-Host "`n=== $msg ===" -ForegroundColor Cyan }
function Fail($msg) { Write-Host "ERROR: $msg" -ForegroundColor Red; exit 1 }

# ── 0. Run codegen scripts that don't need the ROM ──
# Encoded credit files are gitignored (input text is in decomp/src/credits/),
# so a fresh checkout (CI or otherwise) must run the encoder before
# cmake builds scstaffroll.c. ROM-independent — same step CMake's
# GenerateCreditsAssets target runs.
Write-Step "Encoding credits text"
Push-Location (Join-Path $Root "decomp/src/credits")
foreach ($f in @("staff.credits.us.txt", "titles.credits.us.txt")) {
    & python "$Root/tools/creditsTextConverter.py" $f | Out-Null
    if ($LASTEXITCODE -ne 0) { Pop-Location; Fail "credits encode failed: $f" }
}
foreach ($f in @("info.credits.us.txt", "companies.credits.us.txt")) {
    & python "$Root/tools/creditsTextConverter.py" -paragraphFont $f | Out-Null
    if ($LASTEXITCODE -ne 0) { Pop-Location; Fail "credits encode failed: $f" }
}
Pop-Location

# ── 1. Configure + build (Release, portable) ──
Write-Step "Configuring release build (portable, scripting=$EnableScripting)"
# No NON_PORTABLE, no CMAKE_INSTALL_PREFIX. LUS resolves the bundle path
# via GetModuleFileNameW at runtime, and the port's port_save.cpp +
# Ship::Context::GetAppDirectoryPath() route saves/config to the cwd
# (= BattleShip.exe's directory when launched normally). See the file
# header for the v0.7.2 crash this avoids.
#
# Pin Python3_EXECUTABLE to the same `python` the workflow's
# `pip install Pillow` step ran under. On Windows GHA runners the image
# ships several Python installs (hostedtoolcache, Strawberry Perl, vcpkg,
# Windows Store stub); without a hint, CMake's find_package(Python3) can
# resolve to a different interpreter than the one pip targeted, and the
# CSS-arrow asset rules then fail with "Pillow is required" mid-build.
$PythonExe = (Get-Command python).Source
Write-Host "Using Python: $PythonExe"
cmake -B $BuildDir $Root `
    -DCMAKE_BUILD_TYPE=Release `
    "-DDISABLE_SCRIPTING=$DisableScripting" `
    "-DSSB64_VERSION=$Ver" `
    "-DPython3_EXECUTABLE=$PythonExe" `
    | Out-Null
if ($LASTEXITCODE -ne 0) { Fail "cmake configure failed" }

Write-Step "Building BattleShip + torch"
cmake --build $BuildDir --config Release -j $Jobs
if ($LASTEXITCODE -ne 0) { Fail "build failed" }

# ── 2. Build f3d.o2r (zip of LUS shaders, ROM-independent) ──
Write-Step "Packaging Fast3D shader archive"
$F3DO2R = Join-Path $BuildDir "f3d.o2r"
if (Test-Path $F3DO2R) { Remove-Item $F3DO2R -Force }
$ShaderSrc = Join-Path $Root "libultraship\src\fast"
Push-Location $ShaderSrc
Compress-Archive -Path "shaders" -DestinationPath $F3DO2R -CompressionLevel Optimal
Pop-Location
if (-not (Test-Path $F3DO2R)) { Fail "f3d.o2r was not created" }

# ── 3. Locate built artifacts ──
# CMake OUTPUT_NAME == SSB64_APP_NAME == $AppName, so the exe is
# BattleShip.exe (US) or BattleShip-JP.exe (JP).
$GameExe = Join-Path $BuildDir "Release\$AppName.exe"
if (-not (Test-Path $GameExe)) {
    # Fall back to non-multi-config layout (Ninja).
    $GameExe = Join-Path $BuildDir "$AppName.exe"
}
$TorchExe = $null
foreach ($cand in @(
    "TorchExternal\src\TorchExternal-build\Release\torch.exe",
    "TorchExternal\src\TorchExternal-build\torch.exe",
    "torch-install\bin\torch.exe"
)) {
    $p = Join-Path $BuildDir $cand
    if (Test-Path $p) { $TorchExe = $p; break }
}
if (-not (Test-Path $GameExe))   { Fail "$AppName.exe not found at $GameExe" }
if (-not $TorchExe)              { Fail "torch.exe not found in $BuildDir" }

# ── 4. Stage the release tree ──
Write-Step "Staging $StageDir"
if (Test-Path $StageDir) { Remove-Item -Recurse -Force $StageDir }
New-Item -ItemType Directory -Path $StageDir | Out-Null
New-Item -ItemType Directory -Path (Join-Path $StageDir "yamls\$Ver") | Out-Null

Copy-Item $GameExe        (Join-Path $StageDir "$AppName.exe")
Copy-Item $TorchExe      (Join-Path $StageDir "torch.exe")
Copy-Item $F3DO2R        (Join-Path $StageDir "f3d.o2r")
Copy-Item (Join-Path $Root "gamecontrollerdb.txt") $StageDir
Copy-Item (Join-Path $Root "config.yml") $StageDir
# -Recurse: deblob child slice maps + manifests live in subdirectories
# (yamls\<v>\reloc_fighters_main\, docs/deblob.md) and must ship, or
# first-run extraction produces a slice-less archive.
Copy-Item (Join-Path $Root "yamls\$Ver\*") (Join-Path $StageDir "yamls\$Ver") -Recurse
# Standalone .ico for shortcut/installer use — the icon is also embedded
# directly in BattleShip.exe via port/ssb64.rc, so Explorer picks it up
# without this file. Keep it bundled for future installer work.
# Region-aware: JP picks assets\icon-jp.ico, US keeps assets\icon.ico.
# (Note: ssb64.rc still embeds assets\icon.ico unconditionally, so the
# .exe's own Explorer-icon is US-art for both regions until that .rc
# is taught to pick per-region — leave for follow-up if needed.)
$IcoSrc = if ($Ver -eq "jp") { Join-Path $Root "assets\icon-jp.ico" }
          else                { Join-Path $Root "assets\icon.ico"   }
Copy-Item $IcoSrc (Join-Path $StageDir "$AppName.ico")

# Bundle the ESC menu fonts. Menu.cpp::FindMenuAssetPath walks up from
# RealAppBundlePath() and from current_path(); placing the TTFs at
# <staging>\assets\custom\fonts\ next to the .exe matches the first
# iteration of the walker rooted at the .exe's directory. Without this
# the menu falls back to ImGui's default font silently.
#
# OFL 1.1 §1 requires the license text to accompany each redistributed
# font file, so the *-OFL.txt files ship alongside the .ttf they govern.
$FontsDir = Join-Path $StageDir "assets\custom\fonts"
New-Item -ItemType Directory -Path $FontsDir -Force | Out-Null
Copy-Item (Join-Path $Root "assets\custom\fonts\Montserrat-Regular.ttf")  $FontsDir
Copy-Item (Join-Path $Root "assets\custom\fonts\Montserrat-OFL.txt")      $FontsDir
Copy-Item (Join-Path $Root "assets\custom\fonts\Inconsolata-Regular.ttf") $FontsDir
Copy-Item (Join-Path $Root "assets\custom\fonts\Inconsolata-OFL.txt")     $FontsDir

# Project LICENSE + verbatim upstream LICENSE files for the submodules
# whose compiled code is in this distribution. MIT requires the upstream
# copyright + permission notice to ride along with redistributed copies.
# .txt suffix follows Windows convention so users can double-click open.
Copy-Item (Join-Path $Root "LICENSE") (Join-Path $StageDir "LICENSE.txt")
$LicensesDir = Join-Path $StageDir "licenses"
New-Item -ItemType Directory -Path $LicensesDir -Force | Out-Null
$LusLicense = Join-Path $Root "libultraship\LICENSE"
$TorchLicense = Join-Path $Root "torch\LICENSE"
if (-not (Test-Path $LusLicense))   { Fail "libultraship\LICENSE not found - submodules not initialized?" }
if (-not (Test-Path $TorchLicense)) { Fail "torch\LICENSE not found - submodules not initialized?" }
Copy-Item $LusLicense   (Join-Path $LicensesDir "libultraship-LICENSE.txt")
Copy-Item $TorchLicense (Join-Path $LicensesDir "torch-LICENSE.txt")
@'
This directory contains license texts for third-party components whose
compiled code is included in this BattleShip distribution:

  - libultraship-LICENSE.txt  (MIT, Copyright (c) 2022 kenix3)
  - torch-LICENSE.txt         (MIT, Copyright (c) 2023 Lywx)

Bundled font licenses (SIL Open Font License 1.1) live alongside the
font files at assets\custom\fonts\.

The BattleShip project's own MIT license is in ..\LICENSE.txt.

Additional libraries dynamically linked at runtime (SDL2, GLEW, libzip,
nlohmann_json, tinyxml2, spdlog, fmt, hidapi-via-libultraship) are
distributed under their respective upstream licenses (zlib, modified
BSD, BSD-3-Clause, MIT). Refer to those upstream packages for full
license texts.
'@ | Set-Content -Path (Join-Path $LicensesDir "README.txt") -Encoding UTF8

# Bundle DLLs that landed next to BattleShip.exe (vcpkg drops SDL2.dll, etc.).
$ExeBuildDir = Split-Path $GameExe -Parent
Get-ChildItem -Path $ExeBuildDir -Filter "*.dll" | ForEach-Object {
    Copy-Item $_.FullName $StageDir
}

if ($EnableScripting) {
    Write-Step "Staging TCC scripting runtime"
    $TccDir = Join-Path $ExeBuildDir ".tcc"
    if (-not (Test-Path $TccDir)) { Fail ".tcc scripting runtime not found at $TccDir" }
    Copy-Item $TccDir (Join-Path $StageDir ".tcc") -Recurse -Force
    $TccPeFiles = Get-ChildItem -Path (Join-Path $StageDir ".tcc") -Recurse -File |
        Where-Object { $_.Extension -in @(".dll", ".exe") }
    if ($TccPeFiles) {
        $Names = ($TccPeFiles | ForEach-Object { $_.FullName }) -join "`n"
        Fail ".tcc scripting runtime contains PE executables:`n$Names"
    }
}

# ── 5. Zip ──
Write-Step "Compressing $ZipPath"
if (Test-Path $ZipPath) { Remove-Item $ZipPath -Force }
Compress-Archive -Path "$StageDir\*" -DestinationPath $ZipPath -CompressionLevel Optimal
if (-not (Test-Path $ZipPath)) { Fail "zip was not created" }

$ZipKB = [int]((Get-Item $ZipPath).Length / 1024)
Write-Host "`n✓ Release zip ready: $ZipPath ($ZipKB KB)" -ForegroundColor Green
Write-Host "   Portable: extract anywhere; save data lives next to BattleShip.exe."
if ($EnableScripting) {
    Write-Host "   Includes TCC scripting support for C mods."
} else {
    Write-Host "   TCC scripting disabled for this local/package variant."
}
Write-Host "   First launch will prompt for your ROM via the ImGui wizard."
