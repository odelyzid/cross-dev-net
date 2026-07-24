@echo off
REM Build the Win9x mixnet client (MinGW).
REM Usage:  build.cmd
REM Requires: gcc (MinGW) on PATH, wsock32 library

setlocal
set "ROOT=%~dp0..\.."
set "OUT=%~dp0mixnet.exe"

echo [build] Win9x client...
gcc -std=c99 -O2 -Wall -Wextra -o "%OUT%" "%~dp0mixnet.c" "%ROOT%\clients\common\mixnet_packet.c" -lwsock32 -I"%ROOT%\clients\include" -I"%ROOT%\clients\common"
if errorlevel 1 (
    echo [build] FAILED
    exit /b 1
)
echo [build] OK: %OUT%
exit /b 0
