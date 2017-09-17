/***********************************************************************
 *
 *		GLOBALS.C
 *		Global Variable Declarations for 68000 Assembler
 *
 *      Author: Paul McKee
 *		ECE492    North Carolina State University
 *
 *        Date:	12/13/86
 *
 ************************************************************************/


#include <stdio.h>
#include "asm.h"


/* General */

long    loc;					/* The assembler's location counter */
long	cur_adr;				/* =loc. current address '*' */
char    pass2;					/* Flag telling whether or not it's the second pass */
char    endFlag;				/* Flag set when the END directive is encountered */


/* File pointers */

FILE   *listFile;				/* Listing file */
FILE   *objFile;				/* Object file */


/* Listing information */

char    line[256];				/* Source line */
int     lineNum;				/* Source line number */
char   *listPtr;				/* Pointer to buffer where a listing line is assembled */
char    continuation;			/* TRUE if the listing line is a continuation */


/* Option flags with default values */

char    listFlag = FALSE;			/* True if a listing is desired */
char    objFlag  = OBJMODE_SFORMAT;	/* True if an object code file is desired */
char    xrefFlag = FALSE;			/* True if a cross-reference is desired */
char    cexFlag  = FALSE;			/* True is Constants are to be EXpanded */
char	littleEndianFlag = FALSE;	/* True is little endian mode */
char	csrcWrtMode = 1;			/* gen.c-src 1=BYTE 2=WORD 4=DWORD */

char    *fileName, outName[260];


int     errorCount, warningCount;		/* Number of errors and warnings */

#if 1
symbolDef	*sscbLinkTop = NULL;		/* SS-CB xdef-name links */
symbolDef	*sscbLinkCur = NULL;		/* SS-CB xdef-name links */
char	sscbMode = 0;					/* SS-CB mode */
char	opt_toupper = 1;
#endif
