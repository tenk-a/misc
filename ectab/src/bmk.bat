rem bcc32 ectab.c strtab.c c:\tool\borland\bcc55\lib\wildargs.obj |bccEr2Hm >err.txt
bcc32 -Ox ectab.c strtab.c c:\borland\bcc55\lib\wildargs.obj >err.txt
type err.txt
del *.bak
del *.obj
del *.tds
