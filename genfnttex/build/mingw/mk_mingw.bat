@echo off
setlocal
set SRCDIR=..\..\src
g++ -o genfnttex.exe -Ofast -Wall -DNDEBUG -I%SRCDIR% %SRCDIR%\genfnttex.cpp %SRCDIR%\FontGetter.cpp %SRCDIR%\misc\fks_fname.cpp %SRCDIR%\misc\mbc.c %SRCDIR%\misc\tga_wrt.c -lkernel32 -lgdi32
endlocal
