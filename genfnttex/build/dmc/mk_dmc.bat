@echo off
setlocal
set SRCDIR=..\..\src
dmc -DNDEBUG -I%SRCDIR% -I%SRCDIR%\for_old_compiler\dmc %SRCDIR%\genfnttex.cpp %SRCDIR%\FontGetter.cpp %SRCDIR%\misc\fks_fname.cpp %SRCDIR%\misc\mbc.c %SRCDIR%\misc\tga_wrt.c kernel32.lib gdi32.lib
endlocal
