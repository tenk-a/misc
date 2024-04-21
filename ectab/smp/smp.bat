abx -x -s base/*.txt =..\bld\vc143x64\Release\ectab.exe -v -pSJIS  -denc-cp932 $f
abx -x -s base/*.txt =..\bld\vc143x64\Release\ectab.exe -v -pUTF8  -denc-utf8  $f
abx -x -s base/*.txt =..\bld\vc143x64\Release\ectab.exe -v -pUTF8bom -denc-utf8bom  $f
abx -x -s base/*.txt =..\bld\vc143x64\Release\ectab.exe -v -pUTF8n  -denc-utf8nobom  $f
abx -x -s base/*.txt =..\bld\vc143x64\Release\ectab.exe -v -pEUCJP -denc-eucjp $f

abx -x -s base/*.c   =..\bld\vc143x64\Release\ectab.exe -v -t4s4mb2 -dopt_t4s4mb2 $f
abx -x -s base/*.c   =..\bld\vc143x64\Release\ectab.exe -v -t4b    -dopt_t4b $f
abx -x -s base/*.c   =..\bld\vc143x64\Release\ectab.exe -v -t4s8a   -dopt_t4s8a $f
abx -x -s base/*.c   =..\bld\vc143x64\Release\ectab.exe -v -t4s4qmb -dopt_t4s4qmb $f
abx -x -s base/*.c   =..\bld\vc143x64\Release\ectab.exe -v -t4mb2 -n5:10:90 -dopt_t4mb2n5_10_90 $f
abx -x -s base/*.c   "=..\bld\vc143x64\Release\ectab.exe -v -t4s4mb2 $^-n3:1:0: : $^ -dopt_t4s4mb2n3_1_0_S $f"

