setlocal
pushd %~dp0

set TGT=ectab

set VcVer=
@if /I not "%PATH:Microsoft Visual Studio .NET 2003=%"=="%PATH%" set /a VcVer=71
@if /I not "%PATH:Microsoft Visual Studio 8=%"=="%PATH%"    set /a VcVer=80
@if /I not "%PATH:Microsoft Visual Studio 9.0=%"=="%PATH%"  set /a VcVer=90
@if /I not "%PATH:Microsoft Visual Studio 10.0=%"=="%PATH%" set /a VcVer=100
@if /I not "%PATH:Microsoft Visual Studio 11.0=%"=="%PATH%" set /a VcVer=110
@if /I not "%PATH:Microsoft Visual Studio 12.0=%"=="%PATH%" set /a VcVer=120
@if /I not "%PATH:Microsoft Visual Studio 14.0=%"=="%PATH%" set /a VcVer=140
@if /I not "%PATH:Microsoft Visual Studio\2017=%"=="%PATH%" set /a VcVer=141
@if /I not "%PATH:Microsoft Visual Studio\2019=%"=="%PATH%" set /a VcVer=142
@if /I not "%PATH:Microsoft Visual Studio\2022=%"=="%PATH%" set /a VcVer=143
if "%VcVer%"=="" goto ERR

set Arch=86
@if "%VcVer%"=="143"    if /I not "%PATH:\bin\HostX64\x64=%"=="%PATH%" set Arch=64
@if "%VcVer%"=="142"    if /I not "%PATH:\bin\HostX64\x64=%"=="%PATH%" set Arch=64
@if "%VcVer%"=="141"    if /I not "%PATH:\bin\HostX64\x64=%"=="%PATH%" set Arch=64
@if /I not "%PATH:Microsoft Visual Studio 14.0\VC\BIN\amd64=%"=="%PATH%" set Arch=64
@if /I not "%PATH:Microsoft Visual Studio 13.0\VC\BIN\amd64=%"=="%PATH%" set Arch=64
@if /I not "%PATH:Microsoft Visual Studio 12.0\VC\BIN\amd64=%"=="%PATH%" set Arch=64
@if /I not "%PATH:Microsoft Visual Studio 11.0\VC\BIN\amd64=%"=="%PATH%" set Arch=64
@if /I not "%PATH:Microsoft Visual Studio 10.0\VC\BIN\amd64=%"=="%PATH%" set Arch=64
@if /I not "%PATH:Microsoft Visual Studio 9.0\VC\BIN\amd64=%"=="%PATH%"  set Arch=64
@if /I not "%PATH:Microsoft Visual Studio 8\VC\BIN\amd64=%"=="%PATH%"    set Arch=64
rem echo %VcVer% %Arch%

set Gen=
@if %VcVer% leq 80  set "Gen=NMake Makefiles"
@if %VcVer% equ 90  set "Gen=Visual Studio 9 2008"
@if %VcVer% equ 100 set "Gen=Visual Studio 9 2008"
@if %VcVer% equ 110 set "Gen=Visual Studio 9 2008"
@if %VcVer% equ 120 set "Gen=Visual Studio 12 2013"
@if %VcVer% equ 140 set "Gen=Visual Studio 14 2015"
@if %VcVer% equ 141 set "Gen=Visual Studio 15 2017"
@if %VcVer% equ 142 set "Gen=Visual Studio 16 2019"
@if %VcVer% equ 143 set "Gen=Visual Studio 17 2022"

@set Opt=
@if /I not "%Arch%"=="64" goto SKIP_X64

@if %VcVer% leq 80 goto SKIP_X64

@if %VcVer% leq 141 (
  set "Gen=%Gen% Win64"
) else (
  opt "Opt=-A x64"
)

:SKIP_X64
set WORK=vc%VcVer%x%Arch%

if not exist %WORK% mkdir %WORK%

pushd %WORK%
cmake -G "%Gen%" %Opt% ../..
echo msbuild %TGT%.sln -Property:Configuration=Release >mk_rel.bat
echo msbuild %TGT%.sln -Property:Configuration=Debug   >mk_dbg.bat
popd

popd
endlocal
