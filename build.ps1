#Requires -Version 5.0
<#
.SYNOPSIS
  68mixCross monorepo build: Genesis (SGDK), optional ASM68K, mixnetd server, clean.

.DESCRIPTION
  Set GDK_WIN before running to point at your SGDK root; default is <repo>\\_compilers\\sgdk.
  For the Rust server, MinGW64 bin should be on PATH, or set MSYS2_ROOT (see server\\build-mingw.ps1).

.PARAMETER Target
  All (default) | Genesis | Server | Asm68k | Clean

.EXAMPLE
  .\build.ps1
  .\build.ps1 -Target Server
  $env:GDK_WIN = "E:\Emulation\sgdk211"; .\build.ps1 -Target Genesis
#>
[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet('All', 'Genesis', 'Server', 'Clean')]
    [string] $Target = 'All'
)

$ErrorActionPreference = 'Stop'
$Root = $PSScriptRoot

function Get-GdkRoot {
    $candidates = @()
    if ($env:GDK_WIN) { $candidates += $env:GDK_WIN }
    $candidates += (Join-Path $Root '_compilers\sgdk')
    if (Test-Path 'E:\Emulation\sgdk211') { $candidates += 'E:\Emulation\sgdk211' }
    if (Test-Path 'E:\Emulation\SGDK_NEW') { $candidates += 'E:\Emulation\SGDK_NEW' }
    $tried = @()
    foreach ($p in $candidates) {
        if ([string]::IsNullOrWhiteSpace($p)) { continue }
        if (-not (Test-Path -LiteralPath $p)) { $tried += "missing folder: $p"; continue }
        $make = Join-Path $p 'bin\make.exe'
        if (Test-Path -LiteralPath $make) { return (Resolve-Path -LiteralPath $p).Path }
        $tried += "no make.exe: $make"
    }
    throw ("SGDK not found (expected bin\make.exe under GDK root).`n" + ($tried -join "`n"))
}

function Invoke-GenesisBuild {
    $gdk = Get-GdkRoot
    $env:GDK_WIN = $gdk
    $env:GDK = $gdk -replace '\\', '/'
    $env:PATH = "$(Join-Path $gdk 'bin')$([IO.Path]::PathSeparator)$($env:PATH)"
    $make = Join-Path $gdk 'bin\make.exe'
    if (-not (Test-Path $make)) { throw "make.exe not found: $make" }
    $gproj = Join-Path $Root 'clients\genesis'
    Write-Host ('[build] Genesis (SGDK) GDK=' + $gdk + ' PROJ=' + $gproj)
    Push-Location $gproj
    try {
        & $make -f (Join-Path $gdk 'makefile.gen') -j2 release
        if ($LASTEXITCODE -ne 0) { throw "make release failed with exit $LASTEXITCODE" }
    } finally { Pop-Location }
    Write-Host '[build] Genesis OK — see clients\genesis\out\ (rom.bin, etc.)'
}

function Invoke-ServerBuild {
    $ms = if ($env:MSYS2_ROOT) { $env:MSYS2_ROOT } else { "D:\__SDKs Modding\msys64" }
    $mingwBin = Join-Path $ms 'mingw64\bin'
    if (Test-Path $mingwBin) {
        $env:PATH = $mingwBin + [IO.Path]::PathSeparator + $env:PATH
    } else {
        Write-Warning "MSYS2 MinGW not at $mingwBin - if Rust link fails, set MSYS2_ROOT."
    }
    $serverDir = Join-Path $Root 'server'
    Write-Host ('[build] mixnetd in ' + $serverDir)
    Push-Location $serverDir
    try {
        $cargo = Get-Command 'cargo' -ErrorAction Stop
        if ($cargo.CommandType -ne 'Application' -or [string]::IsNullOrEmpty($cargo.Path)) {
            throw "cargo on PATH is not cargo.exe (type=$($cargo.CommandType)). Add Rust to PATH (e.g. $env:USERPROFILE\.cargo\bin)."
        }
        & $cargo.Path @('build', '--release')
        if ($LASTEXITCODE -ne 0) { throw "server build failed with exit $LASTEXITCODE" }
    } finally { Pop-Location }
    $exe = Join-Path $Root 'server\target\x86_64-pc-windows-gnu\release\mixnetd.exe'
    if (Test-Path $exe) { Write-Host ('[build] server OK: ' + $exe) } else { Write-Warning "Expected exe not found: $exe (check cargo --target in server\.cargo\config)" }
}

function Invoke-Clean {
    Write-Host '[clean] build\genesis, clients\genesis\out, server\target'
    @(
        (Join-Path $Root 'build\genesis'),
        (Join-Path $Root 'clients\genesis\out')
    ) | ForEach-Object { Remove-Item -Recurse -Force $_ -ErrorAction SilentlyContinue }
    $st = Join-Path $Root 'server\target'
    if (Test-Path $st) { Remove-Item -Recurse -Force $st -ErrorAction SilentlyContinue }
    Write-Host '[clean] done.'
}

switch ($Target) {
    'Clean'  { Invoke-Clean; break }
    'Genesis' { Invoke-GenesisBuild; break }
    'Server'  { Invoke-ServerBuild; break }
    'All' {
        Invoke-GenesisBuild
        Invoke-ServerBuild
    }
}
