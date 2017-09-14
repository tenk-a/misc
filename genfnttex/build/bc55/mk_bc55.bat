@echo off
setlocal
set SRCDIR=..\..\src
bcc32 -I%SRCDIR% -I%SRCDIR%\for_old_compiler\bc %SRCDIR%\genfnttex.cpp %SRCDIR%\FontGetter.cpp %SRCDIR%\misc\fks_fname.cpp %SRCDIR%\misc\mbc.c %SRCDIR%\misc\tga_wrt.c
endlocal
