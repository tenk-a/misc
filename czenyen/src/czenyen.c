/* ------------------------------------------------------------------------ */
/* czenyen v1.01  c ソース中の \ を含む全角文字のチェック＆変換     	    */
/*  	    	    	    	    	    	    	    	    	    */
/* 1996/04/26 v1.00 writen by M.Kitamura    	    	    	    	    */
/* 1996/04/27 v1.01 -s -n オプション追加. コメント行末処理付加.     	    */
/* 2000/08/13 v1.10 -na,-nb を追加  	    	    	    	    	    */
/* 2000/09/03 v1.11 -y,-nsを追加. エラー出力はstderrをデフォルトに. 	    */
/* ------------------------------------------------------------------------ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <io.h>

typedef unsigned char  UCHAR;
typedef unsigned       UINT;
typedef unsigned short USHORT;
typedef unsigned long  ULONG;
/*#define ISKANJI(c)	((unsigned)((c)^0x20) - 0xA1 < 0x3C) */
#define ISKANJI(c)  ((UCHAR)(c) >= 0x81 && ( (UCHAR)(c) <= 0x9F || ((UCHAR)(c) >= 0xE0 && (UCHAR)(c) <= 0xFC)))
#define ISKANJI2(c) ((UCHAR)(c) >= 0x40 && (UCHAR)(c) <= 0xfc && (c) != 0x7f)

#ifndef DOS16
#define FIL_NMSZ    4096
#define LBUFSIZE    (8*1024)
#else	// 16ビットの MS-DOS のとき
#define FIL_NMSZ    260
#define LBUFSIZE    (1*1024)
#endif


/* ------------------------------------------------------------------------ */

#define STDERR	    stderr

FILE *err_fp   = NULL;

void eprintf(char *fmt, ...)
{
    va_list app;

    if (err_fp == NULL) {
    	err_fp = STDERR;
    }
    va_start(app, fmt);
    vfprintf(err_fp, fmt, app);
    va_end(app);
}


void err_exit(char *fmt, ...)
{
    va_list app;

    if (err_fp == NULL) {
    	err_fp = STDERR;
    }
    va_start(app, fmt);
    vfprintf(err_fp, fmt, app);
    va_end(app);
    exit(1);
}


void err_init(char *errname)
{
    if (errname) {
    	err_fp = stdout;    // ファイル名がなければエラーを標準出力
    	if (errname[0]) {
    	    err_fp = fopen(errname, "a+");
    	    if (err_fp == NULL) {
    	    	err_exit("エラーファイル %s をオープンできませんでした\n", errname);
    	    }
    	}
    } else {
    	err_fp = STDERR;
    }
}



/* ------------------------------------------------------------------------ */
static char *src_name;
static long src_line;
static char src_lineContinueFlg = 0;


void ErPrintf(char *fmt, ...)
{
    /* 変換時に問題のあった行を表示するためのprintf */
    va_list app;

    va_start(app, fmt);
    fprintf(STDERR, "%s %d : ", src_name, src_line);
    vfprintf(STDERR, fmt, app);
    va_end(app);
}



char *FgetStr(char *buf, long len, FILE *fp)
{
    /* 一行入力. '\0'が混ざっているかのチェックをする */
    char *p;
    int i, c;

  #if 0
    if (len <= 0) {
    	ErPrintf("PRG:programer's error(FgetsStr)\n");
    	exit(1);
    }
  #endif

    --len;
    p = buf;
    i = 0;
    do {
    	if (i++ == len) {
    	    src_lineContinueFlg = 1;
    	    ErPrintf("1行が長すぎる\n");
    	    break;
    	}
    	c = fgetc(fp);
    	if (ferror(fp) || feof(fp)) {
    	    buf[0] = 0;
    	    return NULL;
    	}
    	if (c == '\0') {
    	    ErPrintf("行中に '\\0' が混ざっている\n");
    	    c = ' ';
    	}
    	*p++ = c;
    } while (c != '\n');
    *p = 0;
    return buf;
}



void Conv(char *srcname, char *dstname, int outMode, int cmtFlg, int chkMode, int mszenMode)
{
    static  char buf[LBUFSIZE*4+1024];
    FILE    *fp,*ofp;
    char    *p, *s;
    int     c, k, mode, sptFlg;

    //printf("%s,%s, %d,%d,%d,%d\n", srcname, dstname, outMode,cmtFlg, chkMode, mszenMode);
    sptFlg  = ((outMode & 0x80) != 0);
    outMode = outMode & 0x7f;
    if (srcname) {
    	fp = fopen(srcname, "rt");
    	if (fp == NULL) {
    	    err_exit("ファイル '%s' をオープンできません\n", srcname);
    	}
    } else {
    	srcname = "<stdin>";
    	fp = stdin;
    }
    if (outMode) {
    	if (dstname) {
    	    ofp = fopen(dstname, "wt");
    	    if (ofp == NULL) {
    	    	err_exit("ファイル '%s' を作成できません\n", dstname);
    	    }
    	} else {
    	    dstname = "<stdout>";
    	    ofp = stdout;
    	}
    }
    src_name = srcname;
    src_line = 0;
    src_lineContinueFlg = 0;
    mode = 0;
    for (; ;) {
    	++src_line;
    	if (src_lineContinueFlg) {
    	    src_lineContinueFlg = 0;
    	    --src_line;
    	}
    	if (FgetStr(buf, LBUFSIZE, fp) == NULL)
    	    break;
    	p = buf;
    	for (; ;) {
    	    c = *p++;
    	    if (c == '\0') {
    	    	break;
    	    } else if (c == '"') {
    	    	if (mode == 0)
    	    	    mode = '"';
    	    	else if (mode == '"')
    	    	    mode = 0;
    	    } else if (c == '\'') {
    	    	if (mode == 0)
    	    	    mode = '\'';
    	    	else if (mode == '\'')
    	    	    mode = 0;
    	    } else if (ISKANJI(c) && mszenMode) {
    	    	k = *p++;
    	    	if (ISKANJI2(k) == 0) {
    	    	    ErPrintf("不正な全角がある\n");
    	    	    --p;
    	    	} else /*if (mode)*/ {
    	    	    if (k == '\\') {
    	    	    	if (*p == '\\') {
    	    	    	    s = "\\";
    	    	    	    if (p[1] == 'n') {
    	    	    	    	s = "\\n";
    	    	    	    	if (p[2] == '"')
    	    	    	    	    s = "\\n\"";
    	    	    	    }
    	    	    	    if (chkMode == 0)
    	    	    	    	ErPrintf("<<警告>> \\ を含む全角[%c%c]の直後に %s がある\n", c,k, s);
    	    	    	    else
    	    	    	    	ErPrintf("\\ を含む全角[%c%c]の直後に %s がある\n", c,k, s);
    	    	    	    if ((mode == '\'' || mode == '"') && outMode == 2) { /* 削除 */
    	    	    	    	memmove(p, p+1, strlen(p+1)+1);
    	    	    	    	eprintf("\t\t\tその \\ を削除\n");
    	    	    	    }
    	    	    	} else if (*p == '\n') {
    	    	    	    if (mode == '/' || mode == '*') {
    	    	    	    	ErPrintf("<<警告>> コメント中、\\ を含む全角[%c%c]の直後に改行がある\n", c,k);
    	    	    	    	if (outMode == 1 || outMode == 3) {
    	    	    	    	    eprintf("\t\t\t[%c%c]の後ろに全角空白を挿入\n", c,k);
    	    	    	    	    memmove(p+2, p, strlen(p)+1);
    	    	    	    	    memcpy(p, "　",2);
    	    	    	    	    p += 2;
    	    	    	    	}
    	    	    	    } else {
    	    	    	    	ErPrintf("<<警告>> \\ を含む全角[%c%c]の直後に改行がある\n", c,k);
    	    	    	    }
    	    	    	} else if (*p == '"') {
    	    	    	    if (cmtFlg || mode == '"' || mode == '\'')
    	    	    	    	ErPrintf("<<警告>> \\ を含む全角[%c%c]の直後に \" がある\n", c,k);
    	    	    	} else if (*p == '\'') {
    	    	    	    if (cmtFlg || mode == '"' || mode == '\'')
    	    	    	    	ErPrintf("<<警告>> \\ を含む全角[%c%c]の直後に ' がある\n", c,k);
    	    	    	} else if (*p == '*' && p[1] == '/') {
    	    	    	    	ErPrintf("<<警告>> \\ を含む全角[%c%c]の直後に */ がある\n", c,k);
    	    	    	} else {
    	    	    	    if (cmtFlg || mode == '"' || mode == '\'') {
    	    	    	    	if (chkMode == 0)
    	    	    	    	    ErPrintf("\\ を含む全角[%c%c]がある\n", c,k);
    	    	    	    	else
    	    	    	    	    ErPrintf("<<警告>> \\ を含む全角[%c%c]がある\n", c,k);
    	    	    	    }
    	    	    	}
    	    	    	if (mode == '\'' || mode == '"') {
    	    	    	    if (outMode == 1) { /* 追加 */
    	    	    	    	eprintf("\t\t\t[%c%c]の後ろに \\ を挿入\n", c,k);
    	    	    	    	memmove(p+1, p, strlen(p)+1);
    	    	    	    	*p++ = '\\';
    	    	    	    } else if (outMode == 3 || outMode == 4) {
    	    	    	    	char bf[10];
    	    	    	    	if (outMode == 3)
    	    	    	    	    eprintf("\t\t\t[%c%c]を \\x%02x\\x%02x に変換\n", c,k, (UCHAR)c, (UCHAR)k);
    	    	    	    	if (sptFlg && isxdigit(*p)) {
    	    	    	    	    memmove(p-2+10, p, strlen(p)+1);
    	    	    	    	    sprintf(bf,"\\x%02x\\x%02x\"\"",c&0xff,k&0xff);
    	    	    	    	    memcpy(p-2,bf,10);
    	    	    	    	    p = p -2 + 10;
    	    	    	    	} else {
    	    	    	    	    memmove(p-2+8, p, strlen(p)+1);
    	    	    	    	    sprintf(bf,"\\x%02x\\x%02x",c&0xff,k&0xff);
    	    	    	    	    memcpy(p-2,bf,8);
    	    	    	    	    p = p -2 + 8;
    	    	    	    	}
    	    	    	    }
    	    	    	}
    	    	    } else if (outMode == 4) {	// 全角文字ならば、\x??\x?? の形で出力
    	    	    	char bf[10];
    	    	    	//eprintf("\t\t\t[%c%c]を \\x%02x\\x%02x に変換\n", c,k, (UCHAR)c, (UCHAR)k);
    	    	    	if (sptFlg && isxdigit(*p)) {
    	    	    	    memmove(p-2+10, p, strlen(p)+1);
    	    	    	    sprintf(bf,"\\x%02x\\x%02x\"\"",c&0xff,k&0xff);
    	    	    	    memcpy(p-2,bf,10);
    	    	    	    p = p -2 + 10;
    	    	    	} else {
    	    	    	    memmove(p-2+8, p, strlen(p)+1);
    	    	    	    sprintf(bf,"\\x%02x\\x%02x",c&0xff,k&0xff);
    	    	    	    memcpy(p-2,bf,8);
    	    	    	    p = p -2 + 8;
    	    	    	}
    	    	    }
    	    	}
    	    } else if (c >= 0x80 && outMode == 4) { // 0x80 以上の文字ならば \x?? の形で出力
    	    	char bf[10];
    	    	//eprintf("\t\t\t[%c]を \\x%02x に変換\n", c,k, (UCHAR)c);
    	    	if (sptFlg && isxdigit(*p)) {
    	    	    memmove(p-1+6, p, strlen(p)+1);
    	    	    sprintf(bf,"\\x%02x\"\"",c&0xff);
    	    	    memcpy(p-1,bf,6);
    	    	    p = p -1 + 6;
    	    	} else {
    	    	    memmove(p-1+4, p, strlen(p)+1);
    	    	    sprintf(bf,"\\x%02x",c&0xff);
    	    	    memcpy(p-1,bf,4);
    	    	    p = p -1 + 4;
    	    	}
    	    } else if (c == '\\') {
    	    	c = *p++;
    	    	if (c == '\0')
    	    	    break;
    	    	if (ISKANJI(c))
    	    	    --p;
    	    } else if (mode == '*' && c == '*') {
    	    	if (*p == '/') {
    	    	    p++;
    	    	    mode = 0;
    	    	}
    	    } else if (mode == 0 && c == '/') {
    	    	if (*p == '*') {
    	    	    p++;
    	    	    mode = '*';
    	    	} else if (*p == '/') {
    	    	    p++;
    	    	    mode = '/';
    	    	    break;
    	    	}
    	    }
    	}
    	if (mode == '/')
    	    mode = 0;
    	if (outMode) {
    	    fputs(buf, ofp);
    	    if (ferror(ofp)) {
    	    	err_exit("%s %d : 書込みエラー発生\n", dstname, src_line);
    	    }
    	}
    }
    if (ferror(fp)) {
    	ErPrintf("読込みエラー発生\n");
    	exit(1);
    }
    if (fp != stdin)
    	fclose(fp);
    if (outMode && ofp != stdout) {
    	fclose(ofp);
    }
}



/* ------------------------------------------------------------------------ */

char *FIL_BaseName(char *adr)
{
    /* パス名中のファイル名位置を探す(MS-DOS依存) */
    char *p;

    p = adr;
    while (*p != '\0') {
    	if (*p == ':' || *p == '/' || *p == '\\')
    	    adr = p + 1;
    	if (ISKANJI((*(unsigned char *)p)) && *(p+1) )
    	    p++;
    	p++;
    }
    return adr;
}


char *FIL_ChgExt(char filename[], char *ext)
{
    /* ファイル名の拡張子を付け替える */
    char *p;

  #ifndef DOS16 /* ロングファイル名の時 */
    strcat(filename, ext);
  #else     	/* DOSで 8.3制限があるとき */
    p = FIL_BaseName(filename);
    p = strrchr( p, '.');
    if (p == NULL) {
    	strcat(filename,".");
    	strcat( filename, ext);
    } else {
    	strcpy(p+1, ext);
    }
  #endif
    return filename;
}


#if 0
char *FIL_AddExt(char filename[], char *ext)
{
    /* ファイル名に拡張子がなければ付け足す */
    if (strrchr(FIL_BaseName(filename), '.') == NULL) {
    	strcat(filename,".");
    	strcat(filename, ext);
    }
    return filename;
}
#endif


char *FIL_GetExt(char filename[])
{
    /* 拡張子位置を得る */
    char *p;

    p = FIL_BaseName(filename);
    p = strrchr( p, '.');
    if (p)
    	p++;
    return p;
}



/* ------------------------------------------------------------------------ */

void Usage(void)
{
    eprintf("usage> czenyen [-opts] srcfile [newfile]           //  v1.11 by てんか☆\n"
    	   "        Cソース中 \" や ' で囲まれた中にある \\ を含む全角文字の検索＆変換\n"
    	   "opts:\n"
    	   "  -t       テスト: \\ を含む全角文字がある行を調べる(-a,-nの確認用)\n"
    	   "  -s       テスト: \\ を含む全角文字の直後に \\ があるか調べる(-dの確認用)\n"
    	   "  -a       変換: \\ を含む全角文字の直後に \\ を付加\n"
    	   "  -d       変換: \\ を含む全角文字の直後の \\ を削除\n"
    	   "  -n       変換: \\ を含む全角文字を\\x??\\x5cに変換\n"
    	   "  -na      変換: すべての全角文字と半角カナを \\x?? に変換\n"
    	   "  -nb      変換: 文字コードが 0x80 以上の文字を \\x?? に変換\n"
    	   "  -ns      -n -na -nbで\\x??化の直後に16進数文字が来る場合\"\"で文字列を分割\n"
    	   "  -k       -t -s で \"や 'の範囲だけでなく // /* */コメント中の結果も表示\n"
    	   "  -x[EXT]  指定ファイルは一つで,その名のファイルを読込みその名で出力. 元\n"
    	   "           ファイルは拡張子を .EXT にする(EXT省略時:bak)\n"
    	   "\n"
    	   "  オプション無視定の場合、デフォルトで -t を行う.\n"
    	   "  変換(-a -d -n) を行う場合は、[newfile]を指定するか -x を指定すること.\n"
    	   "  変換は ' か \" で囲まれた範囲のみで // /* */ コメント中は行わないが、\n"
    	   "  改行の直前に \\ を含む全角のある場合は全角空白を追加する(-a -n時).\n"
    );
    exit(1);
}



int main(int argc, char *argv[])
{
    char *dstname = NULL, *srcname = NULL, *ext = NULL, *errname = NULL;
    int i,c, outMode, cmtFlg, chkMode, mszenMode;
    char *p;

    if (argc < 2)
    	Usage();

    cmtFlg = outMode = chkMode = 0;
    mszenMode = 1;
    for (i = 1; i < argc; i++) {
    	p = argv[i];
    	if (*p == '-') {
    	    p++; c = *p++; c = toupper(c);
    	    switch(c) {
    	    case 'A':
    	    	chkMode = 0;
    	    	outMode = 1;
    	    	break;
    	    case 'D':
    	    	chkMode = 1;
    	    	outMode = 2;
    	    	break;
    	    case 'T':
    	    	chkMode = 0;
    	    	outMode = 0;
    	    	break;
    	    case 'S':
    	    	chkMode = 1;
    	    	outMode = 0;
    	    	break;
    	    case 'N':
    	    	if (*p == 'a' || *p == 'A') {
    	    	    chkMode = 0;
    	    	    outMode = 4;
    	    	} else if (*p == 'b' || *p == 'B') {
    	    	    chkMode = 0;
    	    	    outMode = 4;
    	    	    mszenMode = 0;
    	    	} else if (*p == 's' || *p == 'S') {
    	    	    outMode |= 0x80;
    	    	} else {
    	    	    chkMode = 0;
    	    	    outMode = 3;
    	    	}
    	    	break;
    	    case 'K':
    	    	cmtFlg = 1;
    	    	break;
    	    case 'X':
    	    	ext = p;
    	    	if (*p == '\0')
    	    	    ext = "BAK";
    	    	break;
    	    case 'Y':
    	    	errname = p;
    	    	break;
    	    case '\0':
    	    	break;
    	    case 'H':
    	    case '?':
    	    	Usage();
    	    default:
    	    	err_exit("コマンドライン・オプションがおかしい : %s\n", argv[i]);
    	    }
    	} else if (srcname == NULL) {
    	    srcname = p;
    	} else if (dstname == NULL) {
    	    dstname = p;
      #if 0
    	} else if (ext == NULL) {
    	    eprintf("ファイル名が 3つ以上指定されています\n");
      #endif
    	}
    }

    err_init(errname);

    if (outMode == 0 || ext == NULL) {
      #if 0
    	if (srcname == NULL) {
    	    err_exit("\n入力ファイルを指定してください\n\n");
    	}
    	if (outMode && dstname == NULL) {
    	    err_exit("\n出力ファイルを指定してください\n\n");
    	}
      #endif
    	if (outMode && srcname && dstname && stricmp(srcname, dstname) == 0) {	/* ささやかな同名チェック */
    	    err_exit("入力ファイル名と出力ファイル名が同じです\n");
    	}
    	Conv(srcname, dstname, outMode, cmtFlg, chkMode, mszenMode);

    } else {	/* MS-DOS依存なファイル処理... */
    	static char tmpname[FIL_NMSZ];
    	static char bakname[FIL_NMSZ];

    	if (srcname == NULL || dstname) {
    	    err_exit("\n-x指定時は入力ファイルは１つのみ指定してください\n\n");
    	}
    	if (strlen(srcname) >= (sizeof tmpname)-1) {
    	    err_exit("パス名が長すぎます '%s'\n", srcname);
    	}
    	if (stricmp(ext,FIL_GetExt(srcname)) == 0) {
    	    err_exit("入力ファイルの拡張子と出力ファイルの拡張子が同じです\n");
    	}
    	strcpy(tmpname, srcname);
    	strcpy(FIL_BaseName(tmpname), "ccmntss_.tmp");
    	Conv(srcname, tmpname, outMode, cmtFlg, chkMode, mszenMode);
    	FIL_ChgExt(strcpy(bakname,srcname), ext);
    	remove(bakname);
    	if (rename(srcname, bakname) < 0)
    	    eprintf("%s を %s にrenameできませんでした\n",srcname, bakname);
    	else if (rename(tmpname, srcname) < 0)
    	    eprintf("%s を %s にrenameできませんでした\n",srcname, bakname);
    }
    return 0;
}
