# c2htm �̃R���p�C�� (for gnu make)
# vc �� bcc ����{.  dmc,wcl,lcc�̓R���p�C���ł��܂���(�����Ȃ���)

ERRFILE	=err.txt
TGT	=c2htm
LIBS	=
SRCS	=\
	c2htm.cpp 	\
	ConvCpp2Htm.cpp \
	cmisc.cpp 	\
	strtab.cpp 	\
	cfgfile.cpp 	\


#-----------------------------------------------------------------------------

ifeq ($(COMPILER), gcc)		# GNU C/C++
CFLAGS	=	-O3 -Wall $(ADD_CFLAGS)
C_OPT_O =	-o 
CC	=	g++ -c
LINK	=	g++
LINK_OPT_O =	-o 
ERR	=2>>$(ERRFILE)

else
ifeq ($(COMPILER), bcc)		# Borland C/C++
CFLAGS	=	-Ox -v $(ADD_CFLAGS)
#CFLAGS	=	-O2 -Oc -Oi -OS -Ov -x- -pr $(ADD_CFLAGS)
C_OPT_O =	-o
CC	=	bcc32 -c
LINK	=	bcc32
LINK_OPT_O =	-e
ERR	=>>$(ERRFILE)

else
ifeq ($(COMPILER), dmc)		# Digital Mars C/C++
# dm-c v8.41 ���݂� -j0 ��SJIS�Ή��ɂ���� �}�N���֌W�����������R���p�C���s�\.
#  -j0 ���Ƃ�� SJIS �������������ăR���p�C���s�\
#CFLAGS	=	-w -o -Bj -j0 $(ADD_CFLAGS)
CFLAGS	=	-w -o -Bj -Ae $(ADD_CFLAGS)
C_OPT_O =	-o
CC	=	dmc -c
LINK	=	dmc
LINK_OPT_O =	-o
ERR	=>>$(ERRFILE)

else
ifeq ($(COMPILER), wcl)		# Watcom-C/C++
# watcom �� stl �֌W�����r���[�ȏ�ԂȂ��߃R���p�C���s�\
CFLAGS	=	-w3 -ox -xs -xr $(ADD_CFLAGS)
C_OPT_O =	-fo
CC	=	wcl386 -c
LINK	=	wcl386
LINK_OPT_O =	-fe
ERR	=>>$(ERRFILE)

else
ifeq ($(COMPILER), occ)		# orange c++
# C++���Ή��Ȃ̂ŃR���p�C���s�\
CFLAGS	=	$(ADD_CFLAGS)
C_OPT_O =	/o
CC	=	occpr /c 
LINK	=	occpr
LINK_OPT_O =	/o
ERR	=>>$(ERRFILE)

else				# Visual-C/C++
#CFLAGS	=	-Yd -GX -Zi $(ADD_CFLAGS)
#CFLAGS	=	-GX -Ox -Ot -W3 $(ADD_CFLAGS)
CFLAGS	=	-EHsc -Ox -Ot -W3 -wd4996 $(ADD_CFLAGS)
C_OPT_O =	-Fo
CC	=	cl -c -TP
LINK	=	cl
LINK_OPT_O =	-Fe
ERR	=>>$(ERRFILE)

endif
endif
endif
endif
endif


#-----------------------------------------------------------------------------

.SUFFIXES:
.SUFFIXES: .obj .asm .c .cpp

.cpp.obj:
	$(CC) $(CFLAGS) $(C_OPT_O)$*.obj $*.cpp $(ERR)

.c.obj:
	$(CC) $(CFLAGS) $(C_OPT_O)$*.obj $*.c   $(ERR)


COBJS = $(SRCS:.c=.obj)
OBJS = $(COBJS:.cpp=.obj)

TGTFILE=$(TGT).exe

ALL: $(TGTFILE)

$(TGTFILE):	$(OBJS)
	$(LINK) $(CFLAGS) $(LINK_OPT_O)$(TGTFILE) $(OBJS) $(LIBS) $(ERR)
