@echo off
setlocal EnableExtensions EnableDelayedExpansion
REM Build Mixnet PS1 image: CCPSX -> PSYLINK -> CPE2X. Requires SDevTC / PSYQ Win32.
REM CCPSX reads "sn.ini" / "PSYQ.INI" in the *same* folder as CCPSX.EXE, not the cwd.
REM
REM   set PSYQ=C:\Psyq   (recommended: mklink /J C:\Psyq E:\Emulation\psyq  — see BUILD-PS1.md)
REM
if not "%1"=="__MIXW32" if exist "%SystemRoot%\SysWOW64\cmd.exe" (
  "%SystemRoot%\SysWOW64\cmd.exe" /c call "%~f0" __MIXW32 %*
  exit /b %ERRORLEVEL%
)
if "%1"=="__MIXW32" shift

set "PSX=%~dp0"
REM If this .bat is ever started with %%0%% = repo root, %%~dp0%% is wrong; lock PSX to clients\psx\ where the sources live.
if not exist "%PSX%mixnet_stub.c" if exist "%~dp0clients\psx\mixnet_stub.c" set "PSX=%~dp0clients\psx\"
if not exist "%PSX%mixnet_stub.c" if exist "%~dp0psx\mixnet_stub.c" set "PSX=%~dp0psx\"
REM Prefer C:\Psyq when present (junction) so paths match INI/cc1; env PSYQ=E:\ would otherwise skip this.
if exist "C:\Psyq\bin\CCPSX.EXE" set "PSYQ=C:\Psyq" & goto :psok
if not "%PSYQ_ROOT%"=="" set "PSYQ=%PSYQ_ROOT%" & goto :psok
if not "%PSYQ%"=="" goto :psok
set "PSYQ=E:\Emulation\psyq"
:psok

if not exist "%PSYQ%\bin\CCPSX.EXE" (
  echo ERROR: CCPSX not found. Set PSYQ or PSYQ_ROOT, or install a junction: mklink /J C:\Psyq ^<your-psyq-root^>
  exit /b 1
)
if not exist "%PSYQ%\psx\lib\2MBYTE.OBJ" (
  echo ERROR: "%PSYQ%\psx\lib\2MBYTE.OBJ" not found. Check PSYQ install.
  exit /b 1
)

REM Win32 SDevTC CCPSX often rejects E: in sn.ini. Must NOT use ( ) for INIPRE: %% inside ( ) is expanded at ( parse, before runs.
if exist "C:\Psyq\bin\CCPSX.EXE" set "INIPRE=C:\Psyq" & goto inipre_done
set "INIPRE=%PSYQ%"
echo.
echo WARNING: No C:\Psyq. If CCPSX reports sn.ini/psyq.ini errors, create a junction^:
echo   mklink /J C:\Psyq "%PSYQ%"
echo.
:inipre_done

set "OUT=%PSX%out"
if not exist "%OUT%" mkdir "%OUT%"

REM --- INI paths: CCPSX 32-bit reads cwd sn.ini; paths inside must use the same root as a working install ^(C:\Psyq^) ---
if exist "%PSYQ%\bin\PSYQ.INI" if not exist "%OUT%\PSYQ.INI.bak" copy /Y "%PSYQ%\bin\PSYQ.INI" "%OUT%\PSYQ.INI.bak" >nul
if exist "%PSYQ%\bin\sn.ini"    if not exist "%OUT%\sn.ini.bak"    copy /Y "%PSYQ%\bin\sn.ini"    "%OUT%\sn.ini.bak"    >nul

set "INCSN=!INIPRE!\psx\include"
if not exist "%PSYQ%\psx\include\STRING.H" if exist "%PSYQ%\include\STRING.H" set "INCSN=!INIPRE!\include"
REM One ASCII file (no cmd OEM + PS mix): CCPSX often rejects mixed encodings
powershell -NoProfile -ExecutionPolicy Bypass -File "%PSX%write_ccpsx_sn.ps1" "%OUT%\_ccpsx_sn.ini" "!INIPRE!" "!INCSN!"
if errorlevel 1 ( echo write_ccpsx_sn.ps1 failed & exit /b 1 )
copy /Y "%OUT%\_ccpsx_sn.ini" "%OUT%\sn.ini" >nul
copy /Y "%OUT%\_ccpsx_sn.ini" "%OUT%\psyq.ini" >nul
copy /Y "%OUT%\_ccpsx_sn.ini" "%PSYQ%\bin\PSYQ.INI" >nul 2>nul
copy /Y "%OUT%\_ccpsx_sn.ini" "%PSYQ%\bin\sn.ini"    >nul 2>nul
copy /Y "%OUT%\_ccpsx_sn.ini" "%PSYQ%\bin\psyq.ini"  >nul 2>nul
REM 32-bit CCPSX on Win64 can fail to open E:\…\sn.ini; cwd for compile must be on C:.
set "CWD32=C:\Psyq\bin"
if not exist "C:\Psyq\bin\CCPSX.EXE" set "CWD32=%OUT%"
if not exist "%CWD32%" exit /b 1
copy /Y "%OUT%\_ccpsx_sn.ini" "%CWD32%\sn.ini" >nul
copy /Y "%OUT%\_ccpsx_sn.ini" "%CWD32%\psyq.ini" >nul

REM CCPSX 3.06 Win32: must set SN_PATH and PSYQ_PATH to the bin/ dir that holds sn.ini/psyq.ini (not optional; cwd alone is not enough)
set "SN_PATH=%CWD32%"
set "PSYQ_PATH=%CWD32%"
set "COMPILER_PATH=%CWD32%"
set "LIBRARY_PATH=%PSYQ%\psx\lib"
set "C_INCLUDE_PATH=%INIPRE%\psx\include"

set "PATH=%PSYQ%\bin;%PATH%"

set "CCINC=-I%PSX% -I%PSX%.. -I%PSX%..\include -I%PSX%..\common -I%PSYQ%\psx\include -I%PSYQ%\include"
set "COPTS=-c -O2 -G0 %CCINC%"

pushd "%CWD32%" || exit /b 1
"%PSYQ%\bin\CCPSX" %COPTS% "%PSX%mixnet_navigator.c" -o"%OUT%\mixnet_navigator.obj" || ( popd & exit /b 1 )
"%PSYQ%\bin\CCPSX" %COPTS% "%PSX%mixnet_stub.c"        -o"%OUT%\mixnet_stub.obj"        || ( popd & exit /b 1 )
popd
del /q "%OUT%\mixnet.lnk" 2>nul

REM Link with CCPSX (not PSYLINK + .lnk): same pattern as psx\sample\serial\SIO\TUTO2\MAKEFILE.MAK
pushd "%CWD32%" || exit /b 1
"%PSYQ%\bin\CCPSX" -Xo$80010000 "%PSYQ%\psx\lib\2mbyte.obj" "%OUT%\mixnet_navigator.obj" "%OUT%\mixnet_stub.obj" -o"%OUT%\mixnet.cpe","%OUT%\mixnet.sym","%OUT%\mixnet.map"
set ERR=%ERRORLEVEL%
popd
if not "%ERR%"=="0" (
  echo Link ^(ccpsx^) failed: %ERR%
  exit /b 1
)

REM CPE2X in many PSYQ rips is 16-bit and will not run on 64-bit Windows; .cpe is still valid for emulators
"%PSYQ%\bin\CPE2X" /C %OUT%\mixnet.cpe
if errorlevel 1 ( echo [note] CPE2X not runnable on this host — use clients\psx\out\mixnet.cpe in an emulator, or run CPE2X from 16/32-bit. )

echo.
if exist "%OUT%\mixnet.exe" (
  echo [ok]  %OUT%\mixnet.exe
) else (
  echo [ok]  %OUT%\mixnet.cpe  ^(DuckStation / PCSX-Redux: run .cpe or System menu load executable^)
)
exit /b 0
