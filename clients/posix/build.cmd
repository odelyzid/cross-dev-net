@echo off
REM Build the POSIX mixnet client — requires WSL, Linux, or macOS.
REM This script does NOT work on native Windows cmd (needs <arpa/inet.h>).
REM
REM On WSL / Linux / macOS:
REM   gcc -std=c99 -O2 -pthread -o mixnet mixnet.c ../common/mixnet_packet.c
REM   -I../include -I../common
REM
echo [build] POSIX client requires WSL, Linux, or macOS. Skipping on Windows.
exit /b 0
