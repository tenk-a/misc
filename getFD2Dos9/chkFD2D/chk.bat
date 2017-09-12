rem winXp64 でコンパイルチェックするだけのもの
cl -W4 -c -DONLY_COMPILE getFD2d.c >err.txt
del *.obj
type err.txt
rem del *.exe
