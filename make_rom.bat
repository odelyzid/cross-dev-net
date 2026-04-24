@echo off
REM Deprecated alias: same as "build.bat genesis" (kept for old scripts)
call "%~dp0build.bat" genesis
exit /b %ERRORLEVEL%
