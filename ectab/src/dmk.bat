rem %WATCOM%\src\startup\wildargv.c
dmc -w -DNDEBUG ectab.c strtab.c ExArgv.c >err.txt
type err.txt
del *.bak
del *.obj
