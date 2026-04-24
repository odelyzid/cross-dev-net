@echo off
setlocal EnableExtensions
REM Pack mixnet out\mixnet.cpe (or .exe) using PCSX-Redux tools next to pcsx-redux.exe.
REM Default: PCSX_REDUX=E:\Emulation\PSX\PCSX_REDUX  (override with set PCSX_REDUX=...)

if not defined PCSX_REDUX set "PCSX_REDUX=E:\Emulation\PSX\PCSX_REDUX"

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0pack-pcsx-redux.ps1" -ReduxRoot "%PCSX_REDUX%" %*
exit /b %ERRORLEVEL%
