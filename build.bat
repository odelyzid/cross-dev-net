@echo off
setlocal EnableExtensions
REM 68mixCross — portable build (Genesis SGDK, server, C clients)
REM Usage:  build.bat [genesis^|server^|client^|all^|clean^|help]
REM Set GDK_WIN to your SGDK root if auto-detection misses it.

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

REM --- Prefer a real JDK for rescomp/sizebnd (Windows javapath stubs can be broken) ---
if exist "C:\Program Files\Java\jdk-22\bin\java.exe" set "JAVA_HOME=C:\Program Files\Java\jdk-22"
if exist "C:\Program Files\Java\jdk-17\bin\java.exe" set "JAVA_HOME=C:\Program Files\Java\jdk-17"
if exist "C:\Program Files\Java\jdk-11\bin\java.exe" set "JAVA_HOME=C:\Program Files\Java\jdk-11"
if not "%JAVA_HOME%"=="" set "PATH=%JAVA_HOME%\bin;%PATH%"

REM --- Find SGDK root: GDK_WIN env, then bundled, then known installs ---
if not "%GDK_WIN%"=="" goto gdk_found
set "GDK_WIN="
if exist "%ROOT%\_compilers\sgdk\bin\make.exe" set "GDK_WIN=%ROOT%\_compilers\sgdk"
if exist "E:\Emulation\sgdk211\bin\make.exe" set "GDK_WIN=E:\Emulation\sgdk211"
if exist "E:\Emulation\SGDK_NEW\bin\make.exe" set "GDK_WIN=E:\Emulation\SGDK_NEW"
if not "%GDK_WIN%"=="" goto gdk_found
echo ERROR: SGDK not found. Set GDK_WIN to a root containing bin\make.exe.
exit /b 1
:gdk_found
set "GDK=%GDK_WIN:\=/%"
set "PATH=%GDK_WIN%\bin;%PATH%"

set "CMD=%~1"
if /I "%CMD%"=="" set "CMD=genesis"
if /I "%CMD%"=="help" goto help

if /I "%CMD%"=="clean" goto clean
if /I "%CMD%"=="server" goto server
if /I "%CMD%"=="client" goto client
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

:server
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%\build.ps1" -Target Server
if errorlevel 1 exit /b 1
echo [build] mixnetd: server\target\x86_64-pc-windows-gnu\release\mixnetd.exe
exit /b 0

:client
echo [build] C clients (Win9x + POSIX)...
call "%ROOT%\clients\build_clients.cmd"
exit /b %ERRORLEVEL%

:all
call "%~f0" genesis || exit /b 1
call "%~f0" server
exit /b %ERRORLEVEL%

:clean
echo [clean] build\genesis, clients\genesis\out, server\target
if exist "%ROOT%\server\target" rmdir /s /q "%ROOT%\server\target"
if exist "%ROOT%\clients\genesis\out" rmdir /s /q "%ROOT%\clients\genesis\out"
if exist "%ROOT%\build\genesis" rmdir /s /q "%ROOT%\build\genesis"
echo [clean] done.
exit /b 0

:help
echo 68mixCross build.bat
echo   build.bat         - SGDK release (rom in clients\genesis\out\)
echo   build.bat all     - Genesis + mixnetd server
echo   build.bat server  - mixnetd only (uses build.ps1)
echo   build.bat client  - Build C clients (Win9x + POSIX/MinGW)
echo   build.bat clean   - remove common build outputs
echo Set GDK_WIN if SGDK is not at a known install (see script for candidates).
echo Use PowerShell:  .\build.ps1 -Target All
exit /b 0
