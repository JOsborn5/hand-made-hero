@echo off
REM https://hero.handmade.network/episode/code/day018 1:00:00

SET OUTPUT_DIR=bin

call "%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
IF NOT EXIST %OUTPUT_DIR% mkdir %OUTPUT_DIR%
pushd %OUTPUT_DIR%

SET COMMON_COMPILER_FLAGS=-MT -nologo -Gm- -GR- -EHa- -Oi -WX -W4 -wd4100 -wd4201 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -FC -Z7 -Fmwin32_handmap.map
SET COMMON_LINKER_FLAGS=-opt:ref user32.lib Gdi32.lib winmm.lib

REM 32-bit build
REM cl %COMMON_COMPILER_FLAGS% ..\code\win32_winMain.cpp /link -subsystem:windows,5.1  %COMMON_LINKER_FLAGS%

REM 64-bit build
cl %COMMON_COMPILER_FLAGS% ..\code\win32_winMain.cpp /link %COMMON_LINKER_FLAGS%
popd
