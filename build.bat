REM https://hero.handmade.network/episode/code/day012 32:00

@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
mkdir c:\handmade\build
pushd c:\handmade\build
cl -DHANDMADE_WIN32=1 -FC -Zi c:\handmade\code\win32_winMain.cpp user32.lib Gdi32.lib
popd
