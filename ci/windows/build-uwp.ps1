param(
    [string]$Source = "C:\src",
    [string]$Output = "C:\out"
)

$ErrorActionPreference = "Stop"
$nativeBuild = Join-Path $Source "build-windows"
$packageBuild = Join-Path $Source "build-uwp"
$env:VCPKG_ROOT = Join-Path $Source "vcpkg"

if (-not (Test-Path (Join-Path $Source "libultraship\CMakeLists.txt"))) {
    throw "Submodules are missing. Clone with --recurse-submodules before mounting C:\src."
}

cmake -S $Source -B $nativeBuild -G "Visual Studio 17 2022" -T v143 -A x64 `
    -DBATTLESHIP_UWP=ON `
    -DSSB64_VERSION=us `
    -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw "Native configure failed ($LASTEXITCODE)." }

cmake --build $nativeBuild --config Release --parallel 10
if ($LASTEXITCODE -ne 0) { throw "Native build failed ($LASTEXITCODE)." }

cmake -S (Join-Path $Source "uwp") -B $packageBuild -G "Visual Studio 17 2022" -A x64 `
    -DBATTLESHIP_BUILD_DIR=$nativeBuild `
    -DBATTLESHIP_NATIVE_CONFIG=Release
if ($LASTEXITCODE -ne 0) { throw "Package configure failed ($LASTEXITCODE)." }

$cert = New-SelfSignedCertificate `
    -Type Custom `
    -Subject "CN=JRickey" `
    -KeyUsage DigitalSignature `
    -FriendlyName "BattleShip UWP container" `
    -CertStoreLocation "Cert:\CurrentUser\My" `
    -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3")

msbuild (Join-Path $packageBuild "BattleShip-UWP.vcxproj") `
    /p:Configuration=Release `
    /p:Platform=x64 `
    /p:AppxBundle=Always `
    /p:AppxBundlePlatforms=x64 `
    /p:UapAppxPackageBuildMode=SideloadOnly `
    /p:AppxPackageSigningEnabled=true `
    /p:PackageCertificateThumbprint=$($cert.Thumbprint)
if ($LASTEXITCODE -ne 0) { throw "MSIX build failed ($LASTEXITCODE)." }

New-Item -ItemType Directory -Force $Output | Out-Null
Copy-Item -Recurse -Force (Join-Path $packageBuild "AppPackages\*") $Output
Write-Host "BattleShip UWP package copied to $Output"
