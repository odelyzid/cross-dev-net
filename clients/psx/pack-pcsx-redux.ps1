#Requires -Version 5.0
<#
  Pack mixnet CPE/PS-EXE with PCSX-Redux tools: PS-EXE, optional .elf, and bootable disc image.
  Default Redux root: $env:PCSX_REDUX, else E:\Emulation\PSX\PCSX_REDUX
  Prereq: run build-psyq.bat so out\ has mixnet.cpe (and optionally mixnet.exe from CPE2X)
#>
param(
    [string]$ReduxRoot = $env:PCSX_REDUX,
    [string]$OutDir = "",
    [switch]$SkipIso,
    [switch]$SkipElf
)

$ErrorActionPreference = "Stop"
$here = $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($OutDir)) { $OutDir = Join-Path $here "out" }
if ([string]::IsNullOrWhiteSpace($ReduxRoot)) {
    $ReduxRoot = "E:\Emulation\PSX\PCSX_REDUX"
}
$OutDir = [System.IO.Path]::GetFullPath($OutDir)
$ReduxRoot = $ReduxRoot.TrimEnd([char[]]@('\', '/'))

$packer = Join-Path $ReduxRoot "ps1-packer.exe"
$exe2elf = Join-Path $ReduxRoot "exe2elf.exe"
$exe2iso = Join-Path $ReduxRoot "exe2iso.exe"

$toolList = @($packer, $exe2elf, $exe2iso)
foreach ($t in $toolList) {
    if (-not (Test-Path -LiteralPath $t)) {
        Write-Error "Missing PCSX-Redux tool: $t - set -ReduxRoot or environment PCSX_REDUX."
    }
}

if (-not (Test-Path -LiteralPath $OutDir)) {
    New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
}

$cpe = Join-Path $OutDir "mixnet.cpe"
$psyCpe2x = Join-Path $OutDir "mixnet.exe"
$workExe = Join-Path $OutDir "mixnet_pcsx.psx.exe"
$outElf = Join-Path $OutDir "mixnet_pcsx.elf"
$outDisc = Join-Path $OutDir "mixnet_pcsx_boot.iso"

if (Test-Path -LiteralPath $psyCpe2x) {
    Copy-Item -LiteralPath $psyCpe2x -Destination $workExe -Force
    Write-Host "[ok] Using CPE2X PS-EXE: $psyCpe2x"
} elseif (Test-Path -LiteralPath $cpe) {
    & $packer $cpe -o $workExe
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Host "[ok] ps1-packer: $cpe -> $workExe"
} else {
    Write-Error "No mixnet.cpe or mixnet.exe in $OutDir - run build-psyq.bat first."
}

if (-not $SkipElf) {
    & $exe2elf $workExe -o $outElf
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Host "[ok] exe2elf: $outElf"
}

if (-not $SkipIso) {
    & $exe2iso $workExe -o $outDisc
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Host "[ok] exe2iso: $outDisc  (raw PS1 CD image; extension is cosmetic)"
    Write-Host "     Open this file in PCSX-Redux as a CD/ISO image (or pass it to the disc image option)."
}

Write-Host ""
Write-Host "Outputs under $OutDir :"
Write-Host "  - $workExe  (PS-X EXE)"
if (-not $SkipElf) { Write-Host "  - $outElf" }
if (-not $SkipIso) { Write-Host "  - $outDisc" }
