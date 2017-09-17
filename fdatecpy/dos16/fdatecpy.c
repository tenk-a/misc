/* ファイルの日付のコピー for MS-DOS */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <io.h>
#include <ctype.h>



/* ------------------------------------------------------------------------ */
typedef struct ftime ftime_t;


void Usage(void)
{
    printf( "fdatecpy  v1.11                                      by tenk*\n"
    	    ">fdatecpy SRC_FILE DST_FILE\n"
    	    "  SRC_FILE のファイル日付を DST_FILE にコピーします\n"
    	    "\n"
    	    ">fdatecpy -uDST_FILE SRC_FILE(s)\n"
    	    "  SRC_FILE(s)のうち一番日付の新しいものを DST_FILEにコピーします\n"
    	    "\n"
    	    "  @file指定でfileより、オプションやファイル名を入力\n"
    	    "\n"
    	    );
    exit(0);
}

/* ------------------------------------------------------------------------ */

volatile void printfE(char *fmt, ...)
{
    va_list app;

    va_start(app, fmt);
/*  fprintf(stdout, "%s %5d : ", src_name, src_line);*/
    vfprintf(stdout, fmt, app);
    va_end(app);
    exit(1);
}



FILE *fopenE(char *name, char *mod)
    /* エラーがあれば即exitの fopen() */
{
    FILE *fp;

  #if 0
    if (name == NULL || name[0] == 0) {
    	if (mod[0] == 'w')
    	    return stdout;
    	else if (mod[1] == 'r')
    	    return stdin;
    }
  #endif
    fp = fopen(name,mod);
    if (fp == NULL) {
    	printfE("ファイル %s をオープンできません\n",name);
    }
    setvbuf(fp, NULL, _IOFBF, 1*1024*1024);
    return fp;
}



/* ------------------------------------------------------------------------ */

void *callocE(size_t a, size_t b)
    /* エラーがあれば即exitの calloc() */
{
    void *p;

#if 1
    if (a== 0 || b == 0)
    	a = b = 1;
#endif
    p = calloc(a,b);
    if (p == NULL) {
    	printfE("メモリが足りない(%d*%d byte(s))\n",a,b);
    }
    return p;
}

char *strdupE(char *s)
    /* エラーがあれば即exitの strdup() */
{
    char *p;
    if (s == NULL)
    	return callocE(1,1);
  #if 1
    p = calloc(1,strlen(s)+8);
    if (p)
    	strcpy(p, s);
  #else
    p = strdup(s);
  #endif
    if (p == NULL) {
    	printfE("メモリが足りない(長さ%d+1)\n",strlen(s));
    }
    return p;
}



/* ------------------------------------------------------------------------ */

typedef struct SLIST {
    struct SLIST    *link;
    char    	    *s;
} SLIST;


SLIST *SLIST_Add(SLIST **root, char *s);

SLIST *SLIST_Add(SLIST **p0, char *s)
{
    SLIST* p;

    p = *p0;
    if (p == NULL) {
    	p = callocE(1, sizeof(SLIST));
    	p->s = strdupE(s);
    	*p0 = p;
    } else {
    	while (p->link != NULL) {
    	    p = p->link;
    	}
    	p->link = callocE(1, sizeof(SLIST));
    	p = p->link;
    	p->s = strdupE(s);
    }
    return p;
}



/* ------------------------------------------------------------------------ */
#define FIL_NMSZ    1030
long	TXT1_line;
char	TXT1_name[FIL_NMSZ];

void TXT1_Error(char *fmt, ...)
{
    va_list app;

    va_start(app, fmt);
    fprintf(stdout, "%-12s %5d : ", TXT1_name, TXT1_line);
    vfprintf(stdout, fmt, app);
    va_end(app);
    fflush(stdout);
    return;
}

void TXT1_ErrorE(char *fmt, ...)
{
    va_list app;

    va_start(app, fmt);
    fprintf(stdout, "%-12s %5d : ", TXT1_name, TXT1_line);
    vfprintf(stdout, fmt, app);
    va_end(app);
    fflush(stdout);
    exit(1);
}

FILE	*TXT1_fp;

int TXT1_Open(char *name)
{
    TXT1_fp = fopen(name,"rt");
    if (TXT1_fp == 0)
    	return -1;
    strcpy(TXT1_name, name);
    TXT1_line = 0;
    return 0;
}

void TXT1_OpenE(char *name)
{
    TXT1_fp = fopenE(name,"rt");
    strcpy(TXT1_name, name);
    TXT1_line = 0;
}


char *TXT1_GetsE(char *buf, int sz)
{
    char *p;

    p = fgets(buf, sz, TXT1_fp);
    if (ferror(TXT1_fp)) {
    	TXT1_Error("file read error\n");
    	exit(1);
    }
    TXT1_line++;
    return p;
}

void TXT1_Close(void)
{
    fclose(TXT1_fp);
}



/* ------------------------------------------------------------------------ */

ftime_t GetFileTime(char *srcname)
{
    FILE *fp;
    ftime_t fdt;
    int    f;

    fp = fopen(srcname, "rb");
    if (fp == NULL) {
    	printfE("%s がオープンできません\n", srcname);
    }
    f = getftime(fileno(fp), &fdt);
    fclose(fp);
    if (f < 0) {
    	printfE("%s のアクセスに失敗しました\n", srcname);
    }
    return fdt;
}


void SetFileTime(char *dstname, ftime_t fdt)
{
    FILE *fp;
    int    f;

    fp = fopen(dstname, "ab+");
    if (fp == NULL) {
    	printfE("%s がオープンできません\n", dstname);
    }
    f = setftime(fileno(fp), &fdt);
    fclose(fp);
    if (f < 0) {
    	printfE("%s のアクセスに失敗しました\n", dstname);
    }
}

int CmpFileTime(ftime_t t1, ftime_t t2)
{
    int f;

    f = t1.ft_year  - t2.ft_year;   if (f)  return f;
    f = t1.ft_month - t2.ft_month;  if (f)  return f;
    f = t1.ft_day   - t2.ft_day;    if (f)  return f;
    f = t1.ft_hour  - t2.ft_hour;   if (f)  return f;
    f = t1.ft_min   - t2.ft_min;    if (f)  return f;
    f = t1.ft_tsec  - t2.ft_tsec;
    return f;
}



/* ------------------------------------------------------------------------ */
SLIST	*fileListTop = NULL;
char	*dstname = NULL;
int 	multiInpMode = 0;


int Opts(char *a)
    /* オプションの処理 */
{
    char *p;
    int c;

    p = a;
    p++, c = *p++, c = toupper(c);
    switch (c) {
    case 'U':
    	multiInpMode = 1;
    	dstname = strdupE(p);
    	break;
    case '\0':
    case '?':
    	Usage();
    default:
    	printfE("Incorrect command line option : %s\n", a);
    }
    return 0;
}


void GetResFile(char *name)
{
    static char buf[1024*16];
    char *p;

    TXT1_OpenE(name);
    while (TXT1_GetsE(buf, sizeof buf)) {
    	p = strtok(buf, " \t\n");
    	do {
    	    if (*p == '-') {
    	    	Opts(p);
    	    } else if (multiInpMode) {
    	    	SLIST_Add(&fileListTop, p);
    	    } else if (fileListTop == NULL) {
    	    	SLIST_Add(&fileListTop, p);
    	    } else if (dstname == NULL) {
    	    	dstname = strdupE(p);
    	    }
    	    p = strtok(NULL, " \t\n");
    	} while (p);
    }
    TXT1_Close();
}



/* ------------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
    int i;
    SLIST *s,*d;
    char *p;
    ftime_t tim, t;

    if (argc < 2)
    	Usage();

    for (i = 1; i < argc; i++) {
    	p = argv[i];
    	if (*p == '-') {
    	    Opts(p);
    	} else if (*p == '@') {
    	    GetResFile(p+1);
    	} else if (multiInpMode) {
    	    SLIST_Add(&fileListTop, p);
    	} else if (fileListTop == NULL) {
    	    SLIST_Add(&fileListTop, p);
    	} else if (dstname == NULL) {
    	    dstname = p;
    	}
    }

    if (dstname == NULL)
    	Usage();

    memset(&tim, 0, sizeof tim);
    for (s = fileListTop; s; s = s->link) {
    	t = GetFileTime(s->s);
    	if (CmpFileTime(tim,t) < 0)
    	    tim = t;
    };
    SetFileTime(dstname, tim);

    return 0;
}
