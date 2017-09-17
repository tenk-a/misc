rem bcc32 ectab.c strtab.c c:\tool\borland\bcc55\lib\wildargs.obj |bccEr2Hm ]err.txt
rem cl -O2 -W3 ectab.c strtab.c setargv.obj >err.txt
cl -O2 -W3 -DNDEBUG -wd4996 ectab.c strtab.c ExArgv.c >err.txt
type err.txt
del *.bak
del *.obj
