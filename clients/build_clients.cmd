@echo off
REM Build all Windows-compatible mixnet C clients.
REM Usage:  build_clients.cmd

setlocal
set "ROOT=%~dp0.."

echo === Building all C clients ===

echo.
echo --- Win9x ---
call "%ROOT%\clients\win9x\build.cmd" || exit /b 1

echo.
echo --- POSIX (MinGW) ---
call "%ROOT%\clients\posix\build.cmd" || exit /b 1

echo.
echo === All clients built successfully ===
exit /b 0
