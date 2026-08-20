pushd %~dp0
git clone https://github.com/tenk-a/ccwrap.git

if "%WATCOM%"=="" goto SKIP_OW
call "%CD%\ccwrap\watcom\lib\gen.bat"
:SKIP_OW

popd
