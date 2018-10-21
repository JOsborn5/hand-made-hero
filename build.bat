REM https://hero.handmade.network/episode/code/day014 26:33

@echo off
call "%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
mkdir build
pushd build
cl -DHANDMADE_WIN32=1 -FC -Zi ..\code\win32_winMain.cpp user32.lib Gdi32.lib
popd
