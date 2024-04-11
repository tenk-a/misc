@echo off
setlocal
pushd %~dp0

set Compiler=
set Arch=
set BldTyp=
set Gen=
set Opt=
set forceMake=0
set make=make

:ARG_LOOP
  if "%1"=="" goto ARG_LOOP_EXIT

  if /I "%1"=="vc"       set Compiler=vc
  if /I "%1"=="vc71"     set Compiler=vc71
  if /I "%1"=="vc80"     set Compiler=vc80
  if /I "%1"=="vc90"     set Compiler=vc90
  if /I "%1"=="vc100"    set Compiler=vc100
  if /I "%1"=="vc110"    set Compiler=vc110
  if /I "%1"=="vc120"    set Compiler=vc120
  if /I "%1"=="vc140"    set Compiler=vc140
  if /I "%1"=="vc141"    set Compiler=vc141
  if /I "%1"=="vc142"    set Compiler=vc142
  if /I "%1"=="vc143"    set Compiler=vc143
  if /I "%1"=="vc2003"   set Compiler=vc71
  if /I "%1"=="vc2005"   set Compiler=vc80
  if /I "%1"=="vc2008"   set Compiler=vc90
  if /I "%1"=="vc2010"   set Compiler=vc100
  if /I "%1"=="vc2012"   set Compiler=vc110
  if /I "%1"=="vc2013"   set Compiler=vc120
  if /I "%1"=="vc2015"   set Compiler=vc140
  if /I "%1"=="vc2017"   set Compiler=vc141
  if /I "%1"=="vc2019"   set Compiler=vc142
  if /I "%1"=="vc2022"   set Compiler=vc143
  if /I "%1"=="msys"     set Compiler=msys
  if /I "%1"=="msys2"    set Compiler=msys
  if /I "%1"=="mingw"    set Compiler=mingw
  if /I "%1"=="clang"    set Compiler=clang
  if /I "%1"=="gcc"      set Compiler=gcc
  if /I "%1"=="watcom"   set Compiler=watcom
  if /I "%1"=="ow"       set Compiler=watcom
  if /I "%1"=="dmc"      set Compiler=dmc
  if /I "%1"=="borland"  set Compiler=borland

  if /I "%1"=="x86"      set Arch=Win32
  if /I "%1"=="Win32"    set Arch=Win32
  if /I "%1"=="x64"      set Arch=x64
  if /I "%1"=="Win64"    set Arch=x64

  if /I "%1"=="release"  set BldTyp=Release
  if /I "%1"=="rel"      set BldTyp=Release
  if /I "%1"=="debug"    set BldTyp=Debug
  if /I "%1"=="dbg"      set BldTyp=Debug
  if /I "%1"=="deb"      set BldTyp=Debug

  if /I "%1"=="make"     set forceMake=1
  if /I "%1"=="nmake"    set forceMake=1

  rem if /I "%1"=="clean"    set HasClean=clean

  shift
goto ARG_LOOP
:ARG_LOOP_EXIT

if /I "%Compiler%"=="vc" goto L_VCVER
if /I not "%Compiler%"=="" goto L_COMPILER
@if /I not "%PATH:borland=%"=="%PATH%" set Compiler=borland
@if /I not "%PATH:dm\bin=%"=="%PATH%"  set Compiler=dmc
@if /I not "%PATH:dmc\bin=%"=="%PATH%" set Compiler=dmc
@if /I not "%PATH:WATCOM=%"=="%PATH%"  set Compiler=watcom
@if /I not "%PATH:mingw=%"=="%PATH%"   set Compiler=mingw
@if /I not "%PATH:msys32=%"=="%PATH%"  set Compiler=msys
@if /I not "%PATH:msys64=%"=="%PATH%"  set Compiler=msys
:L_VCVER
@if /I not "%PATH:Microsoft Visual Studio .NET 2003=%"=="%PATH%" set Compiler=vc71
@if /I not "%PATH:Microsoft Visual Studio 8=%"=="%PATH%"    set Compiler=vc80
@if /I not "%PATH:Microsoft Visual Studio 9.0=%"=="%PATH%"  set Compiler=vc90
@if /I not "%PATH:Microsoft Visual Studio 10.0=%"=="%PATH%" set Compiler=vc100
@if /I not "%PATH:Microsoft Visual Studio 11.0=%"=="%PATH%" set Compiler=vc110
@if /I not "%PATH:Microsoft Visual Studio 12.0=%"=="%PATH%" set Compiler=vc120
@if /I not "%PATH:Microsoft Visual Studio 14.0=%"=="%PATH%" set Compiler=vc140
@if /I not "%PATH:Microsoft Visual Studio\2017=%"=="%PATH%" set Compiler=vc141
@if /I not "%PATH:Microsoft Visual Studio\2019=%"=="%PATH%" set Compiler=vc142
@if /I not "%PATH:Microsoft Visual Studio\2022=%"=="%PATH%" set Compiler=vc143

:L_COMPILER
if "%Compiler%"=="" goto ERR_1
if /I "%Compiler:~0,2%"=="vc" goto L_VC
if /I "%Compiler%"=="msys"    goto L_UnixMAKE
if /I "%Compiler%"=="dmc"     goto L_DmcMAKE
if /I "%Compiler%"=="watcom"  goto L_WMAKE
if /I "%Compiler%"=="borland" goto L_BolandMAKE
if /I "%Compiler%"=="mingw"   goto L_MingwMAKE
if /I "%Compiler%"=="gcc"     goto L_UnixMAKE
if /I "%Compiler%"=="clan"    goto L_UnixMAKE
if /I "%Compiler:~0,4%"=="msys" goto L_UnixMAKE
goto ERR_1

:L_VC
set VcVer=%Compiler%
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

if %forceMake%==1 goto L_NMAKE
if %VcVer% leq 80 goto L_NMAKE

set WORK=vc%VcVer%%Arch%
if not exist %WORK% mkdir %WORK%

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

if /I not "%Arch%"=="x64" goto L_MSBUILD

if %VcVer% geq 90 (
  if %VcVer% leq 141 (
    set "Gen=%Gen% Win64"
  ) else (
    set "Opt=-A x64 %Opt%"
  )
)

:L_MSBUILD
rem if /I "%BldTyp%"=="Release" set "Opt=%Opt% -DCMAKE_BUILD_TYPE=Release"
rem if /I "%BldTyp%"=="Debug"   set "Opt=%Opt% -DCMAKE_BUILD_TYPE=Debug"
pushd %WORK%
cmake -G "%Gen%" %Opt% ../..
for %%i in (*.sln) do set TGT=%%i
echo msbuild %TGT% -Property:Configuration=Release >mk_rel.bat
echo msbuild %TGT% -Property:Configuration=Debug   >mk_dbg.bat
call mk_dbg.bat
call mk_rel.bat
popd
goto END

:L_NMAKE
set Compiler=%Compiler%%Arch%
set "Gen=NMake Makefiles"
set make=nmake
goto L_MAKE

:L_UnixMAKE
set Compiler=msys
set "Gen=Unix Makefiles"
set make=make
goto L_MAKE

:L_MingwMAKE
set "Gen=MinGW Makefiles"
set make=make
goto L_MAKE

:L_DmcMAKE
rem Need dmc-cc.exe : https://github.com/tenk-a/cc_for_dmc
set CC=dmc-cc.exe
set CXX=dmc-cc.exe
set "Gen=Unix Makefiles"
set make=make
goto L_MAKE

:L_WMAKE
set "Gen=Watcom WMake"
set make=wmake
goto L_MAKE

:L_BolandMAKE
set "Gen=Borland Makefiles"
set make=tmake
goto L_MAKE

:L_MAKE
if /I "%BldTyp%"=="" set BldTyp=Debug
if /I "%BldTyp%"=="Release" set "Opt=%Opt% -DCMAKE_BUILD_TYPE=Release"
if /I "%BldTyp%"=="Debug"   set "Opt=%Opt% -DCMAKE_BUILD_TYPE=Debug"
set WORK=%Compiler%
if not "%BldTyp%"=="" set "WORK=%WORK%-%BldTyp%"
if not exist %WORK% mkdir %WORK%
pushd %WORK%
cmake -G "%Gen%" %Opt% ../..
%make%
popd
goto END

:ERR_1
@echo gen_bld [Compiler] [Arch] [Release/Debug]
@echo   Compiler vc80,vc90,vc100,vc110,vc120,vc140,vc141,vc142,vc143
@echo            msys watcom
@echo   Arch     Win32,x64

:END

popd
endlocal
