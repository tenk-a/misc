cl -utf-8 -O2 -DNDEBUG -Feacpp.exe acpp.c filn.c mbc.c
::gcc -Wall -DNDEBUG -o acpp.exe  acpp.c filn.c mbc.c
::wcl386 -ox -DNDEBUG -Feacpp.exe acpp.c filn.c mbc.c
::dmc -j0 -J -DNDEBUG -oacpp.exe  acpp.c filn.c mbc.c
::occ -C+u -DNDEBUG -oacpp.exe    acpp.c filn.c mbc.c
del *.obj *.tds
