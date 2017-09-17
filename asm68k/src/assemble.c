/***********************************************************************
 *
 *		ASSEMBLE.C
 *		Assembly Routines for 68000 Assembler
 *
 *    Function: processFile()
 *		Assembles the input file. For each pass, the function
 *		passes each line of the input file to assemble() to be
 *		assembled. The routine also makes sure that errors are
 *		printed on the screen and listed in the listing file
 *		and keeps track of the error counts and the line
 *		number.
 *
 *		assemble()
 *		Assembles one line of assembly code. The line argument
 *		points to the line to be assembled, and the errorPtr
 *		argument is used to return an error code via the
 *		standard mechanism. The routine first determines if the
 *		line contains a label and saves the label for later
 *		use. It then calls instLookup() to look up the
 *		instruction (or directive) in the instruction table. If
 *		this search is successful and the parseFlag for that
 *		instruction is TRUE, it defines the label and parses
 *		the source and destination operands of the instruction
 *		(if appropriate) and searches the flavor list for the
 *		instruction, calling the proper routine if a match is
 *		found. If parseFlag is FALSE, it passes pointers to the
 *		label and operands to the specified routine for
 *		processing.
 *
 *	 Usage: processFile()
 *
 *		assemble(line, errorPtr)
 *		char *line;
 *		int *errorPtr;
 *
 *      Author: Paul McKee
 *		ECE492    North Carolina State University
 *
 *        Date:	12/13/86
 *
 ************************************************************************/


#include <stdio.h>
#include <ctype.h>
#include "asm.h"

int     processFile(char *name)
{
	int     error;
	char    pass;

	pass2 = FALSE;
	for (pass = 0; pass < 2; pass++) {
		IX_Open(name);
		loc = 0;
		/*lineNum = 1;*/
		endFlag = FALSE;
		errorCount = warningCount = 0;
		while (!endFlag && IX_GetsInc(line, 256)) {
			error = OK;
			continuation = FALSE;
			if (pass2 && listFlag)
				listLoc();
			assemble(line, &error);
			if (pass2) {
				if (error > MINOR)
					errorCount++;
				else if (error > WARNING)
					warningCount++;
				if (listFlag) {
					listLine();
					printError(listFile, error, -1);
				}
				printError(STDERR, error, lineNum);
			}
			/*lineNum++;*/
		}
		if (!pass2) {
			pass2 = TRUE;
/*			puts("************************************************************");
			puts("********************  STARTING PASS 2  *********************");
			puts("************************************************************"); */
		}
		/*IX_Rewind();*/
	}

	return NORMAL;

}

char   *skipSpace(char *p)
{
	while (isspace(*p))
		p++;
	return p;
}


int     strcap(char *d,char *s)
{
	char    capFlag;

	capFlag = TRUE;
	while (*s) {
		if (capFlag && opt_toupper)
			*d = toupper(*s);
		else
			*d = *s;
		if (*s == '\'')
			capFlag = (char) !capFlag;
		d++;
		s++;
	}
	*d = '\0';
	return NORMAL;
}


char	*GetLabel(char *line, char *label)
{
	int i;
	char *p;

	i = 0;
	p = line;
		do {
		if (i < SIGCHARS)
			label[i++] = *p;
		p++;
	} while (isalnum(*p) || *p == '.' || *p == '_' || *p == '$');
	label[i] = '\0';
	return p;
}

int 	assemble(char *line, int *errorPtr)
{
	char    capLine[256];
	instruction *tablePtr;
	flavor *flavorPtr;
	opDescriptor source, dest;
	char   *p, *start, label[SIGCHARS + 1], xdefName[SIGCHARS + 1];
	char	size, f, sourceParsed, destParsed;
	unsigned short mask, xdefFlag;
	symbolDef	*symbol;

	strcap(capLine, line);
	cur_adr = loc;
	p = start = skipSpace(capLine);
  #if 1
	if (sscbMode && *p == ';' && p[1] == '<') {
		if (strnicmp(p,";<CHAR>",7) == 0) {
			sscbMode = 2;
		} else if (strnicmp(p,";<IND>",6) == 0) {
			sscbMode = 3;
		} else if (strnicmp(p,";<INDIND>",9) == 0) {
			sscbMode = 4;
		} else if (strnicmp(p,";<CHAR2>",8) == 0) {
			sscbMode = 5;
		} else {
			sscbMode = 1;
		}
		/*printf("@@ %d %s\n", sscbMode, p);*/
	}
  #endif
	if (*p && *p != '*' && *p != ';' && !(p[0] == '/' && p[1] == '/')) {
		p = GetLabel(p, label);
		xdefFlag = 0;
		if ((isspace(*p) && start == capLine) || *p == ':') {
			if (*p == ':') {
				p++;
				if (*p == ':') {
					p++;
					xdefFlag = 1;
					GetLabel(skipSpace(line), xdefName);
				}
			}
			p = skipSpace(p);
			if (*p == '\0' || *p == '*' || *p == ';' || (p[0] == '/'&& p[1] == '/')) {
				symbol = define(label, loc, pass2, errorPtr);
				if (xdefFlag) {
					symbol->flags |= XDEF_MODE;
					if (pass2) {
						outputXdefName(xdefName);
					  #if 1
						if (sscbMode) {
							if (sscbLinkTop == NULL) {
								sscbLinkTop = symbol;
							} else {
								sscbLinkCur->sscblink = symbol;
							}
							sscbLinkCur = symbol;
							sscbLinkCur->sscbName = strdup(xdefName);
							sscbLinkCur->sscbmode = sscbMode;
							/*printf("@@  %-23s %08x %d(%d)\n", sscbLinkCur->sscbName, sscbLinkCur->value, sscbLinkCur->sscbmode,sscbMode);*/
						}
					  #endif
					}
				}
				return NORMAL;
			}
		} else {
			p = start;
			label[0] = '\0';
		}
		p = instLookup(p, &tablePtr, &size, errorPtr);
		if (*errorPtr > SEVERE)
			return NORMAL;
		p = skipSpace(p);
		if (tablePtr->parseFlag) {
			/* Move location counter to a word boundary and fix the listing before assembling an instruction */
			if (loc & 1) {
				autoeven();
			}
			if (*label) {
				symbol = define(label, loc, pass2, errorPtr);
				if (xdefFlag) {
					symbol->flags |= XDEF_MODE;
					if (pass2) {
						outputXdefName(xdefName);
					  #if 1
						if (sscbMode) {
							if (sscbLinkTop == NULL) {
								sscbLinkTop = symbol;
							} else {
								sscbLinkCur->sscblink = symbol;
							}
							sscbLinkCur = symbol;
							sscbLinkCur->sscbName = strdup(xdefName);
							sscbLinkCur->sscbmode = sscbMode;
							/*printf("@@  %-23s %08x %d(%d)\n", sscbLinkCur->sscbName, sscbLinkCur->value, sscbLinkCur->sscbmode,sscbMode);*/
						}
					  #endif
					}
				}
			}
			if (*errorPtr > SEVERE)
				return NORMAL;
			sourceParsed = destParsed = FALSE;
			flavorPtr = tablePtr->flavorPtr;
			for (f = 0; (f < tablePtr->flavorCount); f++, flavorPtr++) {
				if (!sourceParsed && flavorPtr->source) {
					p = opParse(p, &source, errorPtr);
					if (*errorPtr > SEVERE)
						return NORMAL;
					sourceParsed = TRUE;
				}
				if (!destParsed && flavorPtr->dest) {
					if (*p != ',') {
						NEWERROR(*errorPtr, SYNTAX);
						return NORMAL;
					}
					p = opParse(p + 1, &dest, errorPtr);
					if (*errorPtr > SEVERE)
						return NORMAL;
					if (!isspace(*p) && *p) {
						NEWERROR(*errorPtr, SYNTAX);
						return NORMAL;
					}
					destParsed = TRUE;
				}
				if (!flavorPtr->source) {
					mask = pickMask((int) size, flavorPtr, errorPtr);
					(*flavorPtr->exec) (mask, (int) size, &source, &dest, errorPtr);
					return NORMAL;
				} else if ((source.mode & flavorPtr->source) && !flavorPtr->dest) {
					if (!isspace(*p) && *p) {
						NEWERROR(*errorPtr, SYNTAX);
						return NORMAL;
					}
					mask = pickMask((int) size, flavorPtr, errorPtr);
					(*flavorPtr->exec) (mask, (int) size, &source, &dest, errorPtr);
					return NORMAL;
				} else if (source.mode & flavorPtr->source
						   && dest.mode & flavorPtr->dest) {
					mask = pickMask((int) size, flavorPtr, errorPtr);
					(*flavorPtr->exec) (mask, (int) size, &source, &dest, errorPtr);
					return NORMAL;
				}
			}
			NEWERROR(*errorPtr, INV_ADDR_MODE);
		} else {
			(*tablePtr->exec) ((int) size, label, p, errorPtr);
			return NORMAL;
		}
	}
	return NORMAL;
}



int     pickMask(int size,flavor *flavorPtr,int *errorPtr)
{
	if (!size || size & flavorPtr->sizes) {
		if (size & (BYTE | SHORT))
			return flavorPtr->bytemask;
		else if (!size || size == WORD)
			return flavorPtr->wordmask;
		else
			return flavorPtr->longmask;
	}
	NEWERROR(*errorPtr, INV_SIZE_CODE);
	return flavorPtr->wordmask;
}


int NoInpl(int size,char *label,char *op,int *errorPtr)
{
	NEWERROR(*errorPtr, WARN_NO_INPL);
	return NORMAL;
}


int NoProblem(int size,char *label,char *op,int *errorPtr)
{
	return NORMAL;
}

