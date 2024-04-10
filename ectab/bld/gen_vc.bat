pushd %~dp0
set TGT=vc
if not exist %TGT% mkdir %TGT%
pushd %TGT%
cmake %* ../..
echo msbuild ectab.sln -Property:Configuration=Release >mk_rel.bat
echo msbuild ectab.sln -Property:Configuration=Debug   >mk_dbg.bat
popd
popd
