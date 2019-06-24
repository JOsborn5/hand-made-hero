@echo off
REM https://hero.handmade.network/episode/code/day016 1:01:00

SET OUTPUT_DIR=bin

call "%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
mkdir %OUTPUT_DIR%
pushd %OUTPUT_DIR%
REM cl -MT -nologo -Gm- -GR- -EHa- -Oi -WX -W4 -wd4100 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -FC -Z7 -Fmwin32_handmap.map ..\code\win32_winMain.cpp /link -opt:ref -subsystem:windows,5.1 user32.lib Gdi32.lib
cl -MT -nologo -Gm- -GR- -EHa- -Oi -WX -W4 -wd4100 -wd4201 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -FC -Z7 -Fmwin32_handmap.map ..\code\win32_winMain.cpp /link -opt:ref user32.lib Gdi32.lib
popd
