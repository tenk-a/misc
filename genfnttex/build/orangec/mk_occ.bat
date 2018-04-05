echo not build
goto :EOF
@echo off
setlocal
set SRCDIR=..\..\src
occ -DNDEBUG -I%SRCDIR% %SRCDIR%\genfnttex.cpp %SRCDIR%\FontGetter.cpp %SRCDIR%\misc\fks_fname.cpp %SRCDIR%\misc\mbc.c %SRCDIR%\misc\tga_wrt.c
endlocal
