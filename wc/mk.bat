@echo off
rem
rem mk.bat [COMPILER] [debug|release] [tagetname]
rem

setlocal
set COMPILER=%1
set MOD=%2

rem �A�v�����Ƃ̐ݒ�.
set TGT=wc
set SRCS=wc.c ExArgv.c
set ERRFILE=err.txt

if not "%3"=="" (
  set TGT=%3
)

rem set EXENAME=%TGT%_%COMPILER%.exe
set EXENAME=%TGT%.exe

rem �R���p�C�����ݒ�.

if "%COMPILER%"=="" set COMPILER=vc

if /i "%COMPILER%"=="vc"      goto J_VC
if /i "%COMPILER%"=="cl"      goto J_VC
if /i "%COMPILER%"=="gcc"     goto J_MINGW
if /i "%COMPILER%"=="g++"     goto J_MINGW
if /i "%COMPILER%"=="mingw"   goto J_MINGW
if /i "%COMPILER%"=="borland" goto J_BCC
if /i "%COMPILER%"=="bcc"     goto J_BCC
if /i "%COMPILER%"=="dm"      goto J_DMC
if /i "%COMPILER%"=="dmc"     goto J_DMC
if /i "%COMPILER%"=="ow"      goto J_WATCOM
if /i "%COMPILER%"=="watcom"  goto J_WATCOM
if /i "%COMPILER%"=="clean"   goto J_CLEAN

:J_VC
  set CC=cl
  set O_CMN=-W4 -GR- -EHac -D_CRT_SECURE_NO_WARNINGS
  set O_REL=-Ox -DNDEBUG -MT
  set O_DEB=-ZI -MTd
  set O_OUT=-Fe
  set ERN=1
  goto COMP

:J_MINGW
  set CC=g++
  set O_CMN=-Wall -D_WIN32
  set O_REL=-O2 -DNDEBUG
  set O_DEB=-g
  set O_OUT=-o 
  set ERN=2
  goto COMP

:J_BCC
  set CC=bcc32
  set O_CMN=-w -w-8004 -w-8057 -w-8066 -w-8071 -w-8026 -w-8027
  set O_REL=-O2 -DNDEBUG -Ox
  set O_DEB=-v
  set O_OUT=-e
  set ERN=1
  goto COMP

:J_DMC
  set CC=dmc
  set O_CMN=-w -Ae
  set O_REL=-o -DNDEBUG
  set O_DEB=-g
  set O_OUT=-o
  set ERN=1
  goto COMP

:J_WATCOM
  set CC=owcc
  set O_CMN=-Wall
  set O_REL=-Ot -O3 -DNDEBUG
  set O_DEB=-gcodeview
  set O_OUT=-o 
  set ERN=1
  goto COMP

:J_CLEAN
  del *.obj
  del *.o
  del *.map
  del *.tds
  del *.ncb
  del *.ilk
  del *.suo
  del *.err
  goto :EOF

rem ���ۂ̃R���p�C��
:COMP
if "%MOD%"=="debug" (
  set OPTS=%O_DEB% %O_CMN% %OPTS_ADD%
) else (
  set OPTS=%O_REL% %O_CMN% %OPTS_ADD%
)
%CC% %OPTS% %O_OUT%%EXENAME% %SRCS% %ERN%>%ERRFILE%
if errorlevel 1 (
  type %ERRFILE%
  pause
) else (
  type %ERRFILE%
)
