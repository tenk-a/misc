#
# Program: asm
#

.c.obj:
#	bcc -ml -c $*.c
#	wcl386 -c /4r /zk /ml /Wx $*.c
	wcl386 -c /4r /zk /Wx $*.c

TARGET= asm68k
OBJS  = codegen.obj directiv.obj error.obj eval.obj globals.obj  \
	instlook.obj listing.obj main.obj movem.obj object.obj  \
	opparse.obj symbol.obj assemble.obj build.obj insttabl.obj \
	file.obj

$(TARGET).exe : $(OBJS)
#	bcc -ml -e$(TARGET) $(OBJS)
#	+wcl386 /ml /zk /Wx $(OBJS)
	wcl386 /zk /Wx $(OBJS)

main.obj	:	 main.c 	asm.h 
codegen.obj	:	 codegen.c	asm.h 
directiv.obj	:	 directiv.c	asm.h 
error.obj	:	 error.c	asm.h 
eval.obj	:	 eval.c 	asm.h 
globals.obj	:	 globals.c	asm.h 
instlook.obj	:	 instlook.c	asm.h 
listing.obj	:	 listing.c	asm.h 
movem.obj	:	 movem.c	asm.h 
object.obj	:	 object.c	asm.h 
opparse.obj	:	 opparse.c	asm.h 
symbol.obj	:	 symbol.c	asm.h 
assemble.obj	:	 assemble.c	asm.h 
build.obj	:	 build.c	asm.h 
insttabl.obj	:	 insttabl.c	asm.h 
file.obj	:	 file.c		asm.h 
