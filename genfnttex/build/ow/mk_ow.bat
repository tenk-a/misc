@echo off
setlocal
set SRCDIR=..\..\src
wcl386.exe -cc++ -ox -xs -xr -bcl=nt -I%SRCDIR% -I%SRCDIR%\for_old_compiler\ow %SRCDIR%\genfnttex.cpp %SRCDIR%\FontGetter.cpp %SRCDIR%\misc\fks_fname.cpp %SRCDIR%\misc\mbc.c %SRCDIR%\misc\tga_wrt.c
endlocal
