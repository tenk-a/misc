@echo off
setlocal
pushd %~dp0

set TGT=ectab

set VcVer=%1
set Arch=%2

if /I "%VcVer%"=="vc" goto L_VCVER
if /I not "%VcVer%"=="" goto SKIP_VCVER
:L_VCVER
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
if "%VcVer%"=="" goto ERR_1
:SKIP_VCVER
if /I "%VcVer:~0,2%"=="vc" set /a "VcVer=%VcVer:~2%"
set /a VcVer=%VcVer%

if /I not "%Arch%"=="" goto SKIP_ARCH
@if "%VcVer%"=="143"      if /I not "%PATH:\bin\HostX64\x64=%"=="%PATH%" set Arch=x64
@if "%VcVer%"=="142"      if /I not "%PATH:\bin\HostX64\x64=%"=="%PATH%" set Arch=x64
@if "%VcVer%"=="141"      if /I not "%PATH:\bin\HostX64\x64=%"=="%PATH%" set Arch=x64
@if /I not "%PATH:Microsoft Visual Studio 14.0\VC\BIN\amd64=%"=="%PATH%" set Arch=x64
@if /I not "%PATH:Microsoft Visual Studio 13.0\VC\BIN\amd64=%"=="%PATH%" set Arch=x64
@if /I not "%PATH:Microsoft Visual Studio 12.0\VC\BIN\amd64=%"=="%PATH%" set Arch=x64
@if /I not "%PATH:Microsoft Visual Studio 11.0\VC\BIN\amd64=%"=="%PATH%" set Arch=x64
@if /I not "%PATH:Microsoft Visual Studio 10.0\VC\BIN\amd64=%"=="%PATH%" set Arch=x64
@if /I not "%PATH:Microsoft Visual Studio 9.0\VC\BIN\amd64=%"=="%PATH%"  set Arch=x64
@if /I not "%PATH:Microsoft Visual Studio 8\VC\BIN\amd64=%"=="%PATH%"    set Arch=x64
:SKIP_ARCH
if /I "%Arch%"=="" set Arch=Win32

rem echo %VcVer% %Arch%

set WORK=vc%VcVer%%Arch%
if not exist %WORK% mkdir %WORK%

:Gen
set Gen=
if %VcVer% leq 80 goto L_NMAKE
if %VcVer% geq 90 (
  if %VcVer% leq 110 (
    set "Gen=Visual Studio 9 2008"
  )
)
if %VcVer% equ 120 set "Gen=Visual Studio 12 2013"
if %VcVer% equ 140 set "Gen=Visual Studio 14 2015"
if %VcVer% equ 141 set "Gen=Visual Studio 15 2017"
if %VcVer% equ 142 set "Gen=Visual Studio 16 2019"
if %VcVer% equ 143 set "Gen=Visual Studio 17 2022"

set Opt=
if /I not "%Arch%"=="x64" goto L_MSBUILD

if %VcVer% geq 90 (
  if %VcVer% leq 141 (
    set "Gen=%Gen% Win64"
  ) else (
    set "Opt=-A x64"
  )
)

:L_MSBUILD
pushd %WORK%
cmake -G "%Gen%" %Opt% ../..
echo msbuild %TGT%.sln -Property:Configuration=Release >mk_rel.bat
echo msbuild %TGT%.sln -Property:Configuration=Debug   >mk_dbg.bat
popd
goto END

:L_NMAKE
set "Gen=NMake Makefiles"
pushd %WORK%
cmake -G "%Gen%" %Opt% ../..
popd
goto END

:ERR_1
@echo gen_vc VcVer [Arch]
@echo   VcVer  80,90,100,110,120,140,141,142,143
@echo   Arch   Win32,x64

:END

popd
endlocal
