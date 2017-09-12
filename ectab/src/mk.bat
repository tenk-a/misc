rem bcc32 ectab.c strtab.c c:\tool\borland\bcc55\lib\wildargs.obj |bccEr2Hm ]err.txt
cl -O2 -W3 ectab.c strtab.c setargv.obj >err.txt
type err.txt
del *.bak
del *.obj
