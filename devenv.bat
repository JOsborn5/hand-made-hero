@echo off
SET OUTPUT_DIR=bin

start "" "%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\Common7\IDE\devenv.exe" ".\%OUTPUT_DIR%\win32_winMain.exe"