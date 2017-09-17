/***********************************************************************
 *
 *		OBJECT.C
 *		Object File Routines for 68000 Assembler
 *
 *    Function: initObj()
 *		Opens the specified object code file for writing. If
 *		the file cannot be opened, then the routine prints a
 *		message and exits.
 *
 *		outputObj()
 *		Places the data whose size, value, and address are
 *		specified in the object code file. If the new data
 *		would cause the current S-record to exceed a certain
 *		length, or if the address of the current item doesn't
 *		follow immediately after the address of the previous
 *		item, then the current S-record is written to the file
 *		(using writeObj) and a new S-record is started,
 *		beginning with the specified data.
 *
 *		writeObj()
 *		Writes the current S-record to the object code file.
 *		The record length and checksum fields are filled in
 *		before the S-record is written. If an error occurs
 *		during the writing, the routine prints a message and
 *		exits.
 *
 *		finishObj()
 *		Flushes the S-record buffer by writing out the data in
 *		it (using writeObj), if any, then writes a termination
 *		S-record and closes the object code file. If an error
 *		occurs during this write, the routine prints a messge
 *		and exits.
 *
 *	 Usage: initObj(name)
 *		char *name;
 *
 *		outputObj(newAddr, data, size)
 *		int data, size;
 *
 *		writeObj()
 *
 *		finishObj()
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

/* Define the maximum number of bytes (address, data, and checksum) that can be in one S-record */

#define SRECSIZE  36

static char sRecord[80*4], *objPtr;
static char byteCount, checksum, lineFlag;
static long objAddr;
static char objErrorMsg[] = "Error writing to object file\n";
static char csrcBlkFlg = 0;
static unsigned char csrcWrtBuf[256];

void	OpenObj(char *name, char *mode)
{
	objFile = fopen(name, mode);
	if (objFile == NULL) {
		puts("Can't open object file");
		exit(0);
	}
}

int     initObj(char *name)
{
	switch (objFlag) {
	case OBJMODE_SFORMAT:
		OpenObj(name, "wt");
		/* Output S-record file header */
		/* fputs("Here comes an S-record...\n", objFile); */
		fputs("S004000020DB\n", objFile);
		break;
	case OBJMODE_CSRC:
		OpenObj(name, "wt");
		byteCount = 0;
		csrcBlkFlg = 0;
		memset(csrcWrtBuf, 0, sizeof csrcWrtBuf);
		break;
	case OBJMODE_BIN:
		OpenObj(name, "wb");
		break;
	}
	lineFlag = FALSE;
	return NORMAL;
}

int     writeObj(void)
{
	char    recLen[3];

	if (objFlag == OBJMODE_SFORMAT) {
		/* Fill in the record length (including the checksum in the record length */
		sprintf(recLen, "%02X", ++byteCount);
		strncpy(sRecord + 2, recLen, 2);
		/* Add the checksum (including in the checksum the record length) */
		checksum += byteCount;
		sprintf(objPtr, "%02X\n", (~checksum & 0xFF));
		/* Output the S-record to the object file */
		/*fputs("Here comes an S-record...\n", objFile); */
		fputs(sRecord, objFile);
		if (ferror(objFile)) {
			fputs(objErrorMsg, STDERR);
			exit(0);
		}
	}
	return NORMAL;
}


long    checkValue(long data)
{
	return (data + (data >> 8) + (data >> 16) + (data >> 24)) & 0xFF;
}

int		outputObjS(long newAddr, long data, int size)
{
	/* If the new data doesn't follow the previous data,
	   or if the S-record would be too long,
	   then write out this S-record and start a new one */
	if ((lineFlag && (newAddr != objAddr)) || (byteCount + size > SRECSIZE)) {
		writeObj();
		lineFlag = FALSE;
	}

	/* If no S-record is already being assembled, then start making one */
	if (lineFlag == FALSE) {
		if ((newAddr & 0xFFFF) == newAddr) {
			sprintf(sRecord, "S1  %04X", newAddr);
			byteCount = 2;
		} else if ((newAddr & 0xFFFFFF) == newAddr) {
			sprintf(sRecord, "S2  %06X", newAddr);
			byteCount = 3;
		} else {
			sprintf(sRecord, "S3  %08lX", newAddr);
			byteCount = 4;
		}
		objPtr = sRecord + 4 + byteCount * 2;
		checksum = (char) (checkValue(newAddr) & 0xff);
		objAddr = newAddr;
		lineFlag = TRUE;
	}
	/* Add the new data to the S-record */
	switch (size) {
	case BYTE:
		data &= 0xFF;
		sprintf(objPtr, "%02X", data);
		byteCount++;
		checksum += (char) (data & 0xff);
		break;
	case WORD:
		data &= 0xFFFF;
		sprintf(objPtr, "%04X", data);
		byteCount += 2;
		checksum += (char) (checkValue(data) & 0xff);
		break;
	case LONG:
		sprintf(objPtr, "%08lX", data);
		byteCount += 4;
		checksum += (char) (checkValue(data) & 0xff);
		break;
	default:
		printf("outputObj: INVALID SIZE CODE!\n");
		exit(0);
	}
	objPtr += size * 2;
	objAddr += (long) size;
	return NORMAL;
}

static void BinPutc(int c)
{
	fputc(c, objFile);
	if (ferror(objFile)) {
		fputs(objErrorMsg, STDERR);
		exit(0);
	}
}

int		outputObjBIN(long data, int size)
{
	switch (size) {
	case BYTE:
		BinPutc(data & 0xff);
		break;
	case WORD:
		data &= 0xFFFF;
		BinPutc((data>>8)&0xff);
		BinPutc(data&0xff);
		break;
	case LONG:
		BinPutc((data>>24)&0xff);
		BinPutc((data>>16)&0xff);
		BinPutc((data>> 8)&0xff);
		BinPutc( data     &0xff);
		break;
	default:
		printf("outputObj: INVALID SIZE CODE!\n");
		exit(0);
	}
	return NORMAL;
}

void outputXdefName(char *name)
{
	if (objFlag == OBJMODE_CSRC) {
	  #if 0
//		if (byteCount)
//			fprintf(objFile,"\n");
	  #endif
		if (csrcBlkFlg)
			fprintf(objFile, "};\n");
		switch (csrcWrtMode) {
		case 1:fprintf(objFile, "unsigned char  %s[] = {\n", name); break;
		case 2:fprintf(objFile, "unsigned short %s[] = {\n", name); break;
		case 4:fprintf(objFile, "unsigned long  %s[] = {\n", name); break;
		}
		csrcBlkFlg = 1;
		byteCount = 0;
	}
}

static void CsrcPut32(void)
{
	int i;

	fprintf(objFile, "\t");
	for (i = 0; i < byteCount; i += csrcWrtMode) {
		switch (csrcWrtMode) {
		case 1: fprintf(objFile,"0x%02x,", csrcWrtBuf[i]); break;
		case 2: fprintf(objFile,"0x%02x%02x,", csrcWrtBuf[i], csrcWrtBuf[i+1]); break;
		case 4: fprintf(objFile,"0x%02x%02x%02x%02x,", csrcWrtBuf[i], csrcWrtBuf[i+1],csrcWrtBuf[i+2], csrcWrtBuf[i+3]); break;
		}
	}
	fprintf(objFile, "\n");
	byteCount = 0;
	memset(csrcWrtBuf, 0, sizeof csrcWrtBuf);
}

static void CsrcPutc(int c)
{
  #if 1
	csrcWrtBuf[byteCount++] = c;
	if (byteCount == 32) {
		CsrcPut32();
	}
  #else
//	if (byteCount == 0)
//		fprintf(objFile, "\t");
//	fprintf(objFile, "0x%02x,", c);
//	++byteCount;
//	if (byteCount == 32) {
//		fprintf(objFile, "\n");
//		byteCount = 0;
//	}
  #endif
}

int		outputObjCSRC(long data, int size)
{
	switch (size) {
	case BYTE:
		CsrcPutc(data & 0xff);
		break;
	case WORD:
		data &= 0xFFFF;
		CsrcPutc((data>>8)&0xff);
		CsrcPutc(data&0xff);
		break;
	case LONG:
		CsrcPutc((data>>24)&0xff);
		CsrcPutc((data>>16)&0xff);
		CsrcPutc((data>> 8)&0xff);
		CsrcPutc( data     &0xff);
		break;
	default:
		printf("outputObj: INVALID SIZE CODE!\n");
		exit(0);
	}
	return NORMAL;
}

int     outputObj(long newAddr, long data, int size)
{
	if (littleEndianFlag) {
		if (size == BYTE) {
			;
		} else if (size == WORD) {
			data = ((data&0xff00)>>8) + ((data&0xff)<<8);
		} else {
			data = ((data&0xff000000L)>>24) + ((data&0x00ff0000)>>8) + ((data&0xff00)<<8) + ((data&0xff)<<24);
		}
	}
	switch (objFlag) {
	case OBJMODE_SFORMAT:
		return outputObjS(newAddr, data, size);
	case OBJMODE_CSRC:
		return outputObjCSRC(data, size);
	case OBJMODE_BIN:
		return outputObjBIN(data, size);
	}
	return NORMAL;
}

int 	finishObj(void)
{
	if (objFlag == OBJMODE_SFORMAT) {
		/* Write out the last real S-record, if present */
		if (lineFlag)
			writeObj();
		/* Write out a termination S-record and close the file */
		fputs("S9030000FC\n", objFile);
		if (ferror(objFile)) {
			fputs(objErrorMsg, STDERR);
			exit(0);
		}
	} else if (objFlag == OBJMODE_CSRC) {
	  #if 1
		if (byteCount) {
			CsrcPut32();
		}
		if (csrcBlkFlg)
			fprintf(objFile, "};\n");
		csrcBlkFlg = 0;
	  #else
//		if (byteCount)
//			fprintf(objFile,"\n");
//		if (csrcBlkFlg)
//			fprintf(objFile, "};\n");
//		csrcBlkFlg = 0;
	  #endif
	}
	fclose(objFile);
	return NORMAL;
}


void outputSscbMap(char *name)
{
	FILE *fp;
	symbolDef *p;

	fp = fopen(name,"wt");
	if (fp == NULL) {
		puts("Can't open SSCB-MAP file");
		exit(0);
	}
	for (p = sscbLinkTop; p; p = p->sscblink) {
		fprintf(fp, "%-23s\t0x%08x\t%d\n", p->sscbName, p->value, p->sscbmode);
	}
	fclose(fp);
}

