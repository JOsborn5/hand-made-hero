REM https://hero.handmade.network/episode/code/day016 12:50

SET OUTPUT_DIR=bin

@echo off
call "%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
mkdir %OUTPUT_DIR%
pushd %OUTPUT_DIR%
cl -WX -W4 -wd4100 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -FC -Zi ..\code\win32_winMain.cpp user32.lib Gdi32.lib
popd
