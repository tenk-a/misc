rem %WATCOM%\src\startup\wildargv.c
rem wcl386 -w3 -ox ectab.c strtab.c >err.txt
wcl386 -w3 -k2000000 ectab.c strtab.c >err.txt
type err.txt
del *.bak
del *.obj
