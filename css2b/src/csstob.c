/* ------------------------------------------------------------------------ */
/*  	    	    	    	    	    	    	    	    	    */
/* Cソース中の ／／ コメントを ／＊コメント＊／ に変換する  	    	    */
/* 1996/04/22 v1.00 writen by M.Kitamura    	    	    	    	    */
/* 2000/09/03 v1.01 -y 追加. エラー出力は stderr をデフォルトに。   	    */
/* ------------------------------------------------------------------------ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <stdarg.h>

typedef unsigned char  UCHAR;
typedef unsigned       UINT;
typedef unsigned short USHORT;
typedef unsigned long  ULONG;

#define STREND(p)   ((p)+strlen(p))

/*#define ISKANJI(c)	((unsigned)((c)^0x20) - 0xA1 < 0x3C) */
#define ISKANJI(c)  ((UCHAR)(c) >= 0x81 && ( (UCHAR)(c) <= 0x9F || ((UCHAR)(c) >= 0xE0 && (UCHAR)(c) <= 0xFC)))
#define ISKANJI2(c) ((UCHAR)(c) >= 0x40 && (UCHAR)(c) <= 0xfc && (c) != 0x7f)

#ifndef DOS16	// 32ビット
#define FIL_NMSZ    4096
#define LBUFSIZE    (16*1024)
#else	    	// 16ビットの MS-DOS のとき
#define FIL_NMSZ    260
#define LBUFSIZE    (8*1024)
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
int  opt_sjis = 1;


void RdErrPuts(char *msg)
{
    fprintf(err_fp, "%s %ld : %s", src_name, src_line, msg);
}

char *FgetStr(char *buf, long len, FILE *fp)
{
    char *p;
    int i, c;

  #if 0
    if (len <= 0) {
    	RdErrPuts("PRG:programer's error(FgetsStr)\n");
    	exit(1);
    }
  #endif

    --len;
    p = buf;
    i = 0;
    do {
    	if (i++ == len) {
    	    src_lineContinueFlg = 1;
    	    RdErrPuts("1行が長すぎる\n");
    	    break;
    	}
    	c = fgetc(fp);
    	if (ferror(fp) || feof(fp)) {
    	    buf[0] = 0;
    	    return NULL;
    	}
    	if (c == '\0') {
    	    RdErrPuts("行中に '\\0' が混ざっている\n");
    	    c = ' ';
    	}
    	*p++ = c;
    } while (c != '\n');
    *p = 0;
    return buf;
}

void Conv(char *srcname, char *dstname)
{
    static  char buf[LBUFSIZE+8];
    FILE    *fp,*ofp;
    char    *p;
    int     c, mode;

    if (srcname) {
    	fp = fopen(srcname, "r");
    	if (fp == NULL) {
    	    err_exit("ファイル '%s' をオープンできません\n", srcname);
    	}
    } else {
    	srcname = "<stdin>";
    	fp = stdin;
    }
    if (dstname) {
    	ofp = fopen(dstname, "w");
    	if (ofp == NULL) {
    	    err_exit("ファイル '%s' を作成できません\n", dstname);
    	}
    } else {
    	dstname = "<stdout>";
    	ofp = stdout;
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
    	if (FgetStr(buf, LBUFSIZE, fp) == NULL) {
    	    break;
    	}
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
    	    } else if (opt_sjis && ISKANJI(c)) {
    	    	c = *p++;
    	    	if (ISKANJI2(c) == 0) {
    	    	    RdErrPuts("不正な全角がある\n");
    	    	    --p;
    	    	}
    	    } else if (c == '\\') {
    	    	c = *p++;
    	    	if (c == '\0')
    	    	    break;
    	    	if (opt_sjis && ISKANJI(c))
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
    	    	    char *s;
    	    	    s = STREND(p);
    	    	    if (s[-1] == '\n') {
    	    	    	*p = '*';
    	    	    	if (p[1] == '/') {
    	    	    	    p++;
    	    	    	    *p = ' ';
    	    	    	}
    	    	      #if 10
    	    	    	c = 0;
    	    	    	while (*p) {
    	    	    	    if ((*p == '/' && p[1] == '*')||(*p == '*' && p[1] == '/')) {
    	    	    	    	if (STREND(p)+2 >= buf+(sizeof buf)-6) {
    	    	    	    	    RdErrPuts("//コメント中の /* */ の処理の途中で１行が長くなりすぎた\n");
    	    	    	    	    break;
    	    	    	    	}
    	    	    	    	p++;
    	    	    	    	memmove(p+1,p,strlen(p)+1);
    	    	    	    	*p = ' ';
    	    	    	    	s = STREND(p);
    	    	    	    	c = 1;
    	    	    	    }
    	    	    	    p++;
    	    	    	}
    	    	    	if (c)
    	    	    	    RdErrPuts("//コメント中に /* か */ があった\n");
    	    	      #endif
    	    	    	strcpy(s - 1, " */\n");
    	    	    } else {
    	    	    	RdErrPuts("//があったが行が長すぎるようなので変換できない\n");
    	    	    }
    	    	    break;
    	    	}
    	    }
    	}
    	fputs(buf, ofp);
    	if (ferror(ofp)) {
    	    err_exit("%s %d : 書込みエラー発生\n", dstname, src_line);
    	}
    }
    if (ferror(fp)) {
    	RdErrPuts("読込みエラー発生\n");
    	exit(1);
    }
    if (fp != stdin)
    	fclose(fp);
    if (ofp != stdout)
    	fclose(ofp);
}


/* ------------------------------------------------------------------------ */
char *FIL_BaseName(char *adr)
    /* パス名中のファイル名位置を探す(MS-DOS依存) */
{
    char *p;

    p = adr;
    while (*p != '\0') {
    	if (*p == ':' || *p == '/' || *p == '\\')
    	    adr = p + 1;
    	if (opt_sjis && ISKANJI((*(unsigned char *)p)) && *(p+1) )
    	    p++;
    	p++;
    }
    return adr;
}

char *FIL_ChgExt(char filename[], char *ext)
    /* ファイル名の拡張子を付け替える */
{
    char *p;

  #ifndef DOS16
    strcat(filename, ext);
  #else
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
    /* ファイル名に拡張子がなければ付け足す */
{
    if (strrchr(FIL_BaseName(filename), '.') == NULL) {
    	strcat(filename,".");
    	strcat(filename, ext);
    }
    return filename;
}
#endif

char *FIL_GetExt(char filename[])
    /* 拡張子位置を得る */
{
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
    puts(
    	"\n"
    	"usage> csstob [-opts] srcfile newfile   # v1.01 by てんか☆\n"
    	"        Cソース中の // コメントを /* コメント */ に変換する\n"
    	"opts:\n"
    	"  -x[EXT]  指定ファイルは一つで新しいファイルを元の名で出力\n"
    	"           し、元ファイルは拡張子を .EXT にする\n"
    	"           (EXT省略時:bak)\n"
    	"  -j       指定ファイル名や入力したテキストに関して全角文字\n"
    	"           (シフトJIS)を考慮する（デフォルト）\n"
    	"  -j-      全角を考慮しない\n"
    	"  -y[FILE] エラーメッセージをFILEに出力. -yのみなら標準出力\n"
    	);
    exit(1);
}

int main(int argc, char *argv[])
{
    static char *dstname = NULL, *srcname = NULL, *ext = NULL, *errname = NULL;
    int i,c;
    char *p;

    if (argc < 2)
    	Usage();

    for (i = 1; i < argc; i++) {
    	p = argv[i];
    	if (*p == '-') {
    	    p++, c = *p++, c = toupper(c);
    	    switch(c) {
    	    case 'J':
    	    	opt_sjis = 1;
    	    	if (*p == '-')
    	    	    opt_sjis = 0;
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
    	}
    }

    err_init(errname);

    if (ext == NULL) {
      #if 0
    	if (srcname == NULL || dstname == NULL) {
    	    err_exit("\n入力ファイルと出力ファイルを指定してください\n\n");
    	}
      #endif
    	if (srcname && dstname && stricmp(srcname, dstname) == 0) { /* ささやかな同名チェック */
    	    err_exit("入力ファイル名と出力ファイル名が同じです\n");
    	}
    	Conv(srcname, dstname);

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
    	Conv(srcname, tmpname);
    	FIL_ChgExt(strcpy(bakname,srcname), ext);
    	remove(bakname);
    	if (rename(srcname, bakname) < 0)
    	    eprintf("%s を %s にrenameできませんでした\n",srcname, bakname);
    	else if (rename(tmpname, srcname) < 0)
    	    eprintf("%s を %s にrenameできませんでした\n",srcname, bakname);
    }
    return 0;
}
