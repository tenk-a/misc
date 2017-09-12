rem %WATCOM%\src\startup\wildargv.c
dmc -o -w ectab.c strtab.c >err.txt
type err.txt
del *.bak
del *.obj
