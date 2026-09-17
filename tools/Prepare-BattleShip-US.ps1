param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$Rom,
    [string]$Output = (Join-Path $PSScriptRoot "BattleShip")
)

$ErrorActionPreference = "Stop"
$expectedSha1 = "e2929e10fccc0aa84e5776227e798abc07cedabf"
$actualSha1 = (Get-FileHash -LiteralPath $Rom -Algorithm SHA1).Hash.ToLowerInvariant()
if ($actualSha1 -ne $expectedSha1) {
    throw "Unsupported ROM. BattleShip UWP requires the US v1.0 ROM (SHA1 $expectedSha1); got $actualSha1."
}

New-Item -ItemType Directory -Force -Path $Output | Out-Null
& (Join-Path $PSScriptRoot "torch.exe") o2r $Rom -s $PSScriptRoot -d $Output
if ($LASTEXITCODE -ne 0) {
    throw "Asset extraction failed with exit code $LASTEXITCODE."
}

$archive = Join-Path $Output "BattleShip.o2r"
if (-not (Test-Path -LiteralPath $archive)) {
    throw "Extraction completed without producing $archive."
}

Write-Host "Ready: $Output"
Write-Host "Copy the BattleShip folder to the root of the Xbox USB drive."
