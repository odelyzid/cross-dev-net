# Build mixnetd with the MSYS2 MinGW64 toolchain on PATH.
# MSVC rustc invokes MSYS2 gcc as the linker; gcc must find ld/as in the same bin dir.
# Override install root:  $env:MSYS2_ROOT = "C:\msys64"
param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]] $CargoArgs = @("build", "--release")
)
$root = if ($env:MSYS2_ROOT) { $env:MSYS2_ROOT } else { "D:\__SDKs Modding\msys64" }
$mingwBin = Join-Path $root "mingw64\bin"
$gcc = Join-Path $mingwBin "x86_64-w64-mingw32-gcc.exe"
if (-not (Test-Path $gcc)) {
    Write-Error "MSYS2 MinGW64 not found. Expected: $gcc`nSet MSYS2_ROOT to your msys64 folder."
    exit 1
}
$env:PATH = "$mingwBin;$env:PATH"
Set-Location $PSScriptRoot
$cargo = Get-Command 'cargo' -ErrorAction Stop
if ($cargo.CommandType -ne 'Application' -or [string]::IsNullOrEmpty($cargo.Path)) {
    Write-Error "cargo must be cargo.exe on PATH (Rust installer). Got: $($cargo.CommandType)"
    exit 1
}
& $cargo.Path @CargoArgs
exit $LASTEXITCODE
