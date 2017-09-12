rem bcc5.5.1でコンパイルできるが printf で ll 指定が効かないのでテキスト出力がバグる
bcc32 -Ox -d -I..\src\old_vc -I..\src -eacpp.exe  ..\src\acpp.c ..\src\filn.c ..\src\mbc.c
del *.obj
del ..\*.tds
