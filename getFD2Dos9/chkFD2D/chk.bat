rem winXp64 �ŃR���p�C���`�F�b�N���邾���̂���
cl -W4 -c -DONLY_COMPILE getFD2d.c >err.txt
del *.obj
type err.txt
rem del *.exe
