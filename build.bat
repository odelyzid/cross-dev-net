@echo off
setlocal EnableExtensions
REM 68mixCross — portable build (Genesis SGDK, optional ASM68K, server)
REM Usage:  build.bat [genesis^|server^|asm^|all^|clean^|help]
REM Set GDK_WIN to your SGDK path if not using bundled _compilers\sgdk

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

if "%GDK_WIN%"=="" set "GDK_WIN=%ROOT%\_compilers\sgdk"
set "GDK=%GDK_WIN:\=/%"
set "PATH=%GDK_WIN%\bin;%PATH%"

set "CMD=%~1"
if /I "%CMD%"=="" set "CMD=genesis"
if /I "%CMD%"=="help" goto help

if /I "%CMD%"=="clean" goto clean
if /I "%CMD%"=="server" goto server
if /I "%CMD%"=="asm" goto asm
if /I "%CMD%"=="all" goto all
if /I "%CMD%"=="genesis" goto genesis
echo Unknown: %CMD%
goto help

:genesis
echo [build] Genesis - GDK_WIN=%GDK_WIN%
set "GPROJ=%ROOT%\clients\genesis"
pushd "%GPROJ%" || exit /b 1
if not exist "%GDK_WIN%\bin\make.exe" (
  echo ERROR: make.exe not found. Set GDK_WIN to a valid SGDK root.
  popd
  exit /b 1
)
"%GDK_WIN%\bin\make.exe" -f "%GDK_WIN%\makefile.gen" -j2 release
set ERR=%ERRORLEVEL%
popd
if not "%ERR%"=="0" exit /b %ERR%
echo [build] Done - see clients\genesis\out\rom.bin
exit /b 0

:asm
set "ASM=%ROOT%\_compilers\ASM68K\asm68k.exe"
set "SRC=%ROOT%\clients\genesis\src\ozworld_init_generic_genesis.s"
set "OUTDIR=%ROOT%\build\genesis"
if not exist "%ASM%" (
  echo ERROR: %ASM% not found.
  exit /b 1
)
if not exist "%OUTDIR%" mkdir "%OUTDIR%"
echo [build] asm68k - %SRC%
"%ASM%" /p /i /w /ov+ /oos+ /oop+ /oow+ /ooz+ /ooaq+ /oosq+ /oomq+ /ow+ /d "%SRC%,%OUTDIR%\OZWORLD.BIN,ozworld" 1> "%OUTDIR%\build.log" 2>&1
if errorlevel 1 ( echo asm68k failed - see %OUTDIR%\build.log & exit /b 1 )
echo [build] OZWORLD.BIN
exit /b 0

:server
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%\build.ps1" -Target Server
if errorlevel 1 exit /b 1
echo [build] mixnetd: server\target\x86_64-pc-windows-gnu\release\mixnetd.exe
exit /b 0

:all
call "%~f0" genesis || exit /b 1
call "%~f0" server
exit /b %ERRORLEVEL%

:clean
echo [clean] build\genesis, clients\genesis\out, server\target
if exist "%ROOT%\server\target" rmdir /s /q "%ROOT%\server\target"
if exist "%ROOT%\clients\genesis\out" del /q "%ROOT%\clients\genesis\out\*.*" 2>nul
if exist "%ROOT%\build\genesis" del /q "%ROOT%\build\genesis\*.o" 2>nul
if exist "%ROOT%\build\genesis" del /q "%ROOT%\build\genesis\*.bin" 2>nul
echo [clean] done.
exit /b 0

:help
echo 68mixCross build.bat
echo   build.bat         - SGDK release (rom in clients\genesis\out\)
echo   build.bat all     - Genesis + mixnetd server
echo   build.bat server  - mixnetd only (uses build.ps1)
echo   build.bat asm     - optional ASM68K ozworld init
echo   build.bat clean   - remove common build outputs
echo Set GDK_WIN if SGDK is not at ROOT\_compilers\sgdk
echo Use PowerShell:  .\build.ps1 -Target All
exit /b 0
