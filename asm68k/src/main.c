/***********************************************************************
 *
 *		MAIN.C
 *		Main Module for 68000 Assembler
 *
 *    Function: main()
 *		Parses the command line, opens the input file and
 *		output files, and calls processFile() to perform the
 *		assembly, then closes all files.
 *
 *	 Usage: main(argc, argv);
 *		int argc;
 *		char *argv[];
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


int     main(int argc,char *argv[])
{
	char	*p;
	int     i;

	puts("68000 Absolute Assembler   by PGM        1986");
	puts("                  modified by M.Kitamura 1996-1998");
	setFlags(argc, argv, &i);
	/* Check whether a name was specified */
	if (i >= argc) {
		fputs("No input file specified\n\n", STDERR);
		help();
	}
	if (!strcmp("?", argv[i]))
		help();

	fileName = argv[i];
	{FILE *fp;
		fp = fopen(fileName,"rt");
		if (fp == NULL) {
			printf(STDERR, "Input file '%s' not found\n", fileName);
			exit(0);
		}
		fclose(fp);
	}

		/****************************************************************
		*																*
		*	NOTE: The following filename manipulations are				*
		*	intended for VMS filenames only. Other file naming			*
		*	conventions may require changes here.						*
		*																*
		*	VMS filenames have the form									*
		*																*
		*	    node::device:[dir1.dir2.dir3]filename.ext;vers			*
		*																*
		*	For use here, we want the listing and object files			*
		*	to be the most recent version in the default				*
		*	directory, so we strip off the node, device, and			*
		*	directory names, extension, and version number, then		*
		*	append the appropriate file extension (.LST or .H68)		*
		*	for each file. 												*
		*																*
		****************************************************************/

	/* Process output file names in their own buffer */
	strcpy(outName, fileName);

	/* Delete version number from output file names */
	p = strchr(outName, ';');
	if (p)
		*p = '\0';

	/* Remove directory name from output file names */
	p = outName + strlen(outName);
	while (p >= outName && *p != ':' && *p != ']')
		p--;
	if (p >= outName)
		strcpy(outName, ++p);

	/* Change extension to .LST */
	p = strchr(outName, '.');
	if (!p)
		p = outName + strlen(outName);

	if (listFlag) {
		strcpy(p, ".LST");
		initList(outName);
	}
	if (objFlag) {
		static char *ext[] = {".H68",".C",".BIN"};
		strcpy(p, ext[objFlag-1]);
		initObj(outName);
		*p = 0;
	}

  #if 1 /* */
	sscbLinkTop = NULL;
	sscbLinkCur = NULL;
  #endif

	/* Assemble the file */
	processFile(fileName);

  #if 0
	/* Close files and print error and warning counts */
	IX_Close();
  #endif

	if (listFlag) {
		putc('\n', listFile);
		if (errorCount > 0)
			fprintf(listFile, "%d error%s detected\n", errorCount,
					(errorCount > 1) ? "s" : "");
		else
			fprintf(listFile, "No errors detected\n");
		if (warningCount > 0)
			fprintf(listFile, "%d warning%s generated\n", warningCount,
					(warningCount > 1) ? "s" : "");
		else
			fprintf(listFile, "No warnings generated\n");
		fclose(listFile);
	}

	if (objFlag)
		finishObj();
	if (errorCount > 0)
		fprintf(STDERR, "%d error%s detected\n", errorCount,
				(errorCount > 1) ? "s" : "");
	else
		fprintf(STDERR, "No errors detected\n");
	if (warningCount > 0)
		fprintf(STDERR, "%d warning%s generated\n", warningCount,
				(warningCount > 1) ? "s" : "");
	else
		fprintf(STDERR, "No warnings generated\n");

  #if 1
	if (sscbMode) {
		strcpy(p, ".INT");
		outputSscbMap(outName);
	}
  #endif

	return NORMAL;
}

#if 1

int     setFlags(int argc,char *argv[],int *argi)
{
	int		c,i;
	char	*p;

	for (i = 1; i < argc; i++) {
		p = argv[i];
		if (*p != '-') {
			break;
		} else {
			++p, c = *p++, c = tolower(c);
			switch (c) {
			case 'd':
				cexFlag  = TRUE;
				break;
			case 'l':
				listFlag = TRUE;
				break;
			case 'n':
				objFlag  = OBJMODE_NON;
				break;
			case 'b':
				objFlag  = OBJMODE_BIN;
				break;
			case 'i':
				littleEndianFlag = TRUE;
				break;
			case 'c':
				objFlag  = OBJMODE_CSRC;
				csrcWrtMode = 1;
				if      (*p == '1')
					csrcWrtMode = 1;
				else if (*p == '2')
					csrcWrtMode = 2;
				else if (*p == '4')
					csrcWrtMode = 4;
				else
					goto ERR;
				break;
			case 'g':
				sscbMode = 1;
				objFlag  = OBJMODE_BIN;
				break;
			case 'y':
				opt_toupper = 0;
				break;
			case 'h':
			case '?':
			case '\0':
				help();
				break;
			default:
		  ERR:
				fprintf(STDERR, "Unknown option %s\n", argv[i]);
			}
		}
	}
	*argi = i;
	return NORMAL;
}


#else

int     setFlags(int argc,char *argv[],int *argi)
{
	int     option;

	while ((option = getopt(argc, argv, "clnbicghy?", argi)) != EOF) {
		switch (option) {
		case 'd':
			cexFlag  = TRUE;
			break;
		case 'l':
			listFlag = TRUE;
			break;
		case 'n':
			objFlag  = OBJMODE_NON;
			break;
		case 'b':
			objFlag  = OBJMODE_BIN;
			break;
		case 'i':
			littleEndianFlag = TRUE;
			break;
		case 'c':
			objFlag  = OBJMODE_CSRC;
			break;
		case 'g':
			sscbMode = 1;
			objFlag  = OBJMODE_BIN;
			break;
		case 'y':
			opt_toupper = 0;
			break;
		case 'h':
		case '?':
			help();
			break;
		}
	}

	return NORMAL;

}


int     getopt(int argc,char *argv[],char *optstring,int *argi)
	/*	Function getopt() scans the command line arguments passed
	 *	via argc and argv for options of the form "-x". It returns
	 *	the letters of the options, one per call, until all the
	 *	options have been returned; it then returns EOF. The argi
	 *	argument is set to the number of the argv string that
	 *	is currently being examined.
	 */
{
	static char *scan = NULL;	/* Scan pointer */
	static int optind = 0;		/* argv index */

	char    c;
	char   *place;
	/*char   *strchr();*/

	if (scan == NULL || *scan == '\0') {
		if (optind == 0)
			optind++;

		if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0') {
			*argi = optind;
			return (EOF);
		}
		if (strcmp(argv[optind], "--") == 0) {
			optind++;
			*argi = optind;
			return (EOF);
		}
		scan = argv[optind] + 1;
		optind++;
	}
	c = *scan++;
	place = strchr(optstring, c);

	if (place == NULL || c == ':') {
		fprintf(STDERR, "Unknown option -%c\n", c);
		*argi = optind;
		return ('?');
	}
	place++;
	if (*place == ':') {
		if (*scan != '\0')
			scan = NULL;
		else
			optind++;
	}
	*argi = optind;
	return c;
}

#endif

/**********************************************************************
 *
 *	Function help() prints out a usage explanation if a bad
 *	option is specified or no filename is given.
 *
 *********************************************************************/

void	help(void)
{
	puts(
		"  Usage: asm68k [-opts] infile.ext\n"
		"    -y   ラベルの大文字小文字を区別する(-y-しない)\n"
		"    -n   S-FORMATファイルを生成(infile.h68)\n"
	    "    -b   バイナリファイルを生成(infile.bin)\n"
		"    -g   バイナリファイルとラベル／アドレス一覧ファイルを生成\n"
		"    -cN  C ソースファイルを生成 (infile.c)\n"
	    "         N= 1:BYTE 2:WORD 4:DWORD\n"
		"    -l   リストファイルを出力(infile.lst)\n"
		"    -d   リスト表示で DCディレクィブの値をすべて表示\n"
		/*"  -i   インテルバイナリを生成\n"*//*失敗*/
	);
	exit(0);
}
