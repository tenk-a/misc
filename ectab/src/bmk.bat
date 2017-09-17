rem bcc32 -Ox -DNDEBUG ectab.c strtab.c c:\borland\bcc55\lib\wildargs.obj >err.txt
bcc32 -Ox -DNDEBUG ectab.c strtab.c ExArgv.c >err.txt
type err.txt
del *.bak
del *.obj
del *.tds
