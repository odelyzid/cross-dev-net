# ASCII + CRLF only (SDevTC CCPSX GetPrivateProfile is picky; PS here-strings can yield bare LF)
param(
    [Parameter(Mandatory = $true)][string]$Path,
    [Parameter(Mandatory = $true)][string]$Inipre,
    [Parameter(Mandatory = $true)][string]$IncDir
)
$rows = @(
    '[ccpsx]',
    'stdlib=libc.lib libapi.lib libsn.lib',
    "compiler_path=$Inipre\bin",
    "assembler_path=$Inipre\bin",
    "linker_path=$Inipre\bin",
    "library_path=$Inipre\psx\lib",
    "c_include_path=$IncDir",
    "cplus_include_path=$IncDir"
)
$text = [string]::Join([char]13 + [char]10, $rows) + ([char]13 + [char]10)
[System.IO.File]::WriteAllBytes($Path, [System.Text.Encoding]::ASCII.GetBytes($text))
