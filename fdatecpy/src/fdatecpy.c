/* ファイルの日付のコピー for MS-DOS */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <io.h>
#include <ctype.h>
#include <sys/utime.h>
#include <sys/stat.h>



/* ------------------------------------------------------------------------ */

int usage(void)
{
    printf( "fdatecpy  v1.12\n"
            ">fdatecpy SRC_FILE DST_FILE\n"
            "  SRC_FILE のファイル日付を DST_FILE にコピーします\n"
            "\n"
            ">fdatecpy -uDST_FILE SRC_FILE(s)\n"
            "  SRC_FILE(s)のうち一番日付の新しいものを DST_FILEにコピーします\n"
            "\n"
            "  @file指定でfileより、オプションやファイル名を入力\n"
            "\n"
            );
    return 1;
}




/* ------------------------------------------------------------------------ */

/** エラーがあれば即exitの calloc() */
void *callocE(size_t a, size_t b)
{
    void *p;

    if (a== 0 || b == 0)
        a = b = 1;
    p = calloc(a,b);
    if (p == NULL) {
        fprintf(stderr, "メモリが足りない(%d*%d byte(s))\n",a,b);
        exit(1);
    }
    return p;
}



/** エラーがあれば即exitの strdup() */
char *strdupE(char *s)
{
    char *p;
    if (s == NULL)
        return callocE(1,1);
    p = calloc(1,strlen(s)+8);
    if (p)
        strcpy(p, s);
    if (p == NULL) {
        fprintf(stderr, "メモリが足りない(長さ%d+1)\n",strlen(s));
        exit(1);
    }
    return p;
}



/* ------------------------------------------------------------------------ */

typedef struct SLIST {
    struct SLIST    *link;
    char            *s;
} SLIST;


SLIST *SLIST_add(SLIST **root, char *s);

SLIST *SLIST_add(SLIST **p0, char *s)
{
    SLIST* p;

    p = *p0;
    if (p == NULL) {
        p    = callocE(1, sizeof(SLIST));
        p->s = strdupE(s);
        *p0  = p;
    } else {
        while (p->link != NULL) {
            p   = p->link;
        }
        p->link = callocE(1, sizeof(SLIST));
        p       = p->link;
        p->s    = strdupE(s);
    }
    return p;
}



/* ------------------------------------------------------------------------ */
#define FIL_NMSZ    1030

long    TXT1_line;
char    TXT1_name[FIL_NMSZ];
FILE    *TXT1_fp;



int TXT1_Open(char *name)
{
    TXT1_fp = fopen(name,"rt");
    if (TXT1_fp == 0)
        return -1;
    strcpy(TXT1_name, name);
    TXT1_line = 0;
    return 0;
}



void TXT1_openE(char *name)
{
    TXT1_fp = fopen(name,"rt");
    if (TXT1_fp == NULL) {
        fprintf(stderr, "%s : open error\n", name);
        exit(1);
    }
    strcpy(TXT1_name, name);
    TXT1_line = 0;
}




char *TXT1_getsE(char *buf, int sz)
{
    char *p;

    p = fgets(buf, sz, TXT1_fp);
    if (ferror(TXT1_fp)) {
        fprintf(stderr, "%-12s %5d : ", TXT1_name, TXT1_line);
        fprintf(stderr, "file read error\n");
        exit(1);
    }
    TXT1_line++;
    return p;
}



void TXT1_close(void)
{
    fclose(TXT1_fp);
}



/* ------------------------------------------------------------------------ */

typedef time_t      ftime_t;


ftime_t file_getTime(const char *fname)
{
    struct stat st;
    ftime_t     t;
    int rc = stat(fname, &st);
    if (rc != 0)
        return 0;
    t = st.st_mtime;
    return t;
}



int file_setTime(const char *fname, ftime_t t)
{
    struct utimbuf  utb;
    int             rc;
    utb.actime  = t;
    utb.modtime = t;
    rc = utime( fname, &utb );
    return rc == 0;
}


#if 0
/// ファイルの日付時間を設定. yy:西暦(1900以降) mm:1-12 dd:1-31 hh:0-23 min:0-59 sec:0-61
int file_setTime(const char *fname, int yy, int mm, int dd, int hh, int min, int sec)
{
    struct utimbuf  utb;
    struct tm       tms;
    ftime_t         t;

    assert( fname != NULL && fname[0] );
    assert(yy >= 1900);

    if (mm < 1 || mm > 12)  mm  = 1;
    if (dd < 1 || dd > 31)  dd  = 1;
    if ((Uint32)hh  >= 24)  hh  = 0;
    if ((Uint32)min >= 60)  min = 0;
    if ((Uint32)sec >= 62)  sec = 0;

    tms.tm_sec   = sec;
    tms.tm_min   = min;
    tms.tm_hour  = hh;
    tms.tm_mday  = dd;
    tms.tm_mon   = mm - 1;          // なんで 0オリジンやねん(T T)
    tms.tm_year  = yy - 1900;
    tms.tm_wday  = -1;
    tms.tm_yday  = -1;
    tms.tm_isdst = -1;
    
    t = mktime(&tms);
    utb.actime  = t;
    utb.modtime = t;
    int rc = utime( fname, &utb );
    return rc == 0;
}
#endif


int file_cmpTime(ftime_t t1, ftime_t t2)
{
    ftime_t f = t1 - t2;
    return f;
}



/* ------------------------------------------------------------------------ */
SLIST   *fileListTop = NULL;
char    *dstname     = NULL;
int     multiInpMode = 0;


int options(char *a)
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
        usage();
        break;
    default:
        printf("Incorrect command line option : %s\n", a);
        return 1;
    }
    return 0;
}


void getResFile(char *name)
{
    static char buf[1024*16];
    char *p;

    TXT1_openE(name);
    while (TXT1_getsE(buf, sizeof buf)) {
        p = strtok(buf, " \t\n");
        do {
            if (*p == '-') {
                options(p);
            } else if (multiInpMode) {
                SLIST_add(&fileListTop, p);
            } else if (fileListTop == NULL) {
                SLIST_add(&fileListTop, p);
            } else if (dstname == NULL) {
                dstname = strdupE(p);
            }
            p = strtok(NULL, " \t\n");
        } while (p);
    }
    TXT1_close();
}



/* ------------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
    int    i;
    SLIST  *s;
    char   *p;
    ftime_t tgtTim, t;

    if (argc < 2)
        return usage();

    for (i = 1; i < argc; ++i) {
        p = argv[i];
        if (*p == '-') {
            if (options(p))
                return 1;
        } else if (*p == '@') {
            getResFile(p+1);
        } else if (multiInpMode) {
            SLIST_add(&fileListTop, p);
        } else if (fileListTop == NULL) {
            SLIST_add(&fileListTop, p);
        } else if (dstname == NULL) {
            dstname = p;
        }
    }

    if (dstname == NULL)
        return usage();

    tgtTim = 0; // memset(&tgtTim, 0, sizeof tgtTim);
    for (s = fileListTop; s; s = s->link) {
        t = file_getTime(s->s);
        //x printf("%s %d\n", s->s, t);
        if (file_cmpTime(tgtTim, t) < 0)
            tgtTim = t;
    };
    //x printf("set %s %d\n", dstname, tgtTim);
    file_setTime(dstname, tgtTim);

    return 0;
}
