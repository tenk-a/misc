/*
    テキストファイルを読みこみ、文字 n個を１行として出力

 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ExArgv.h"
#include "mbc.h"
#include "ujfile.h"

#if defined(_WIN32)
#include <windows.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX    1024
#endif

typedef unsigned char_t;

void Usage(void)
{
    printf(
        "usage> chr2lin [-opts] textfile(s)  //v1.10  " __DATE__ "  " __TIME__ "\n"
        "テキストファイルを読みこみ、出現順に１文字１行として出力\n"
        "一度出現した文字は無視\n"
        " -oNAME 出力ファイル名指定\n"
        " -nN    1文字で１行なく N文字で一行にして出力\n"
        " -eN    N 行ごとに改行を入れる\n"
        " -yN    N 行ごとにファイルを分ける\n"
        "        出力ファイル名は, outfile.000 outfile.001 のような\n"
        "        連番拡張子のファイルを生成\n"
        " -ye    -yと同じだが、名前のつけ方が outfile000.ext のようになる\n"
        " -s     ソートする\n"
        " -cC    テキスト中の文字 C 以降改行までを無視する.\n"
        "        -c のみだと // コメントを無視する.\n"
        " -aN    出力の N行を空白にする\n"
        " -b     半角を無視\n"
        " -l[S]  入出力の文字コード S:utf8,mbc,sjis,eucjp (default: 自動)\n"
    );
    exit(1);
}


/*---------------------------------------------------------------------------*/

#if defined(_WIN32)
#ifndef strcasecmp
#define strcasecmp(a,b)         _stricmp((a),(b))
#endif
#endif

char *StrSkipSpc(char *s)
{
    while ((*s && *(unsigned char *)s <= ' ') || *s == 0x7f) {
        s++;
    }
    return s;
}

char *fpath_baseName(char *adr)
{
    char *p = adr;
    while (*p != '\0') {
        if (*p == ':' || *p == '/' || *p == '\\')
            adr = p + 1;
        //if (ISKANJI((*(unsigned char *)p)) && *(p+1) ) p++;
        ++p;
    }
    return adr;
}

char *fpath_getExt(char *name)
{
    char *p;
    name = fpath_baseName(name);
    p = strrchr(name, '.');
    if (p) {
        return p+1;
    }
    return name + strlen(name);
}

char *fpath_chgExt(char filename[], char *ext)
{
    char *p;

    p = fpath_baseName(filename);
    p = strrchr( p, '.');
    if (p == NULL) {
        if (ext) {
            strcat(filename,".");
            strcat( filename, ext);
        }
    } else {
        if (ext == NULL)
            *p = 0;
        else
            strcpy(p+1, ext);
    }
    return filename;
}


volatile void err_exit(char *fmt, ...)
{
    va_list app;
    va_start(app, fmt);
    // fprintf(stdout, "%s %5d : ", src_name, src_line);
    vfprintf(stdout, fmt, app);
    va_end(app);
    exit(1);
}

void *callocE(size_t a, size_t b)
{
    void *p;
    if (a== 0 || b == 0)
        a = b = 1;
    p = calloc(a,b);
    if (p == NULL)
        err_exit("Not enough memory.(%d*%d byte(s))\n",a,b);
    return p;
}

char *strdupE(char *s)
{
    char *p = callocE(1, strlen(s) + 8);
    return p ? strcpy(p, s) : NULL;
}


typedef struct SLIST {
    struct SLIST    *link;
    char            *s;
} SLIST;

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

void SLIST_Free(SLIST **p0)
{
    SLIST *p, *q;

    for (p = *p0; p; p = q) {
        q = p->link;
        free(p->s);
        free(p);
    }
}


/* ------------------------------------------------------------------------ */
SLIST*      fileList    = NULL;
long        opt_clm     = 1;
long        opt_len     = 0;
int         opt_mltDivFlg = 0;
int         opt_cmtChr  = 0;
int         opt_sort    = 0;
int         opt_noAscii = 0;
char*       opt_oname   = NULL;
int         addLinNum   = 0;
int         cur_cp      = 0;
mbc_enc_t   cur_enc     = NULL;

/** オプションの処理
 */
int Opts(char *a)
{
    char *p = a + 1;
    int  c  = *p++;
    c = toupper(c);
    switch (c) {
    case 'O':
        opt_oname = strdupE(p);
        break;
    case 'N':
        opt_clm = strtol(p,&p,0);
        if (opt_clm <= 0) {
            err_exit("bad -n param(%d)\n", opt_clm);
        }
        break;
    case 'E':
        opt_len = strtol(p,&p,0);
        if (opt_len < 0) {
            err_exit("bad -e param(%d)\n", opt_len);
        }
        break;
    case 'S':
        opt_sort = (*p != '-');
        break;
    case 'Y':
        opt_mltDivFlg = 1;
        if (*p == 'E' || *p == 'e') {
            opt_mltDivFlg = 2;
            p++;
        }
        opt_len = strtol(p,&p,0);
        if (opt_len < 0) {
            err_exit("bad -y param(%d)\n", opt_len);
        }
        break;
    case 'C':
        opt_cmtChr = *p;
        if (*p == 0)
            opt_cmtChr = -1;
        break;
    case 'A':
        addLinNum = strtol(p,&p,0);
        break;
    case 'B':
        opt_noAscii = (*p != '-');
        break;
    case 'L':
        if (strcasecmp(p, "utf8")==0 || strcasecmp(p, "utf-8") == 0) {
            cur_cp = MBC_CP_UTF8;   //mbs_setEnv("ja_JP.UTF-8");
        } else if (strcasecmp(p, "mbc")==0 || strcasecmp(p, "mbs") == 0 || strcasecmp(p, "mb") == 0) {
            cur_cp = MBC_CP_NONE;   //mbs_setEnv("win");
        } else if (strcasecmp(p, "sjis") == 0 || strcasecmp(p,"932") == 0) {
            cur_cp = MBC_CP_SJIS;   //mbs_setEnv("ja_JP.SJIS");
        } else if (strcasecmp(p, "euc-jp") == 0 || strcasecmp(p, "eucjp") == 0 || strcasecmp(p, "euc") == 0) {
            cur_cp = MBC_CP_EUCJP;  //mbs_setEnv("ja_JP.EUC");
        }
        break;

    case '\0':
    case '?':
        Usage();
    default:
        err_exit("Incorrect command line option : %s\n", a);
    }
    return 0;
}


/*---------------------------------------------------------------------------*/
#define CHR_BUF_MAX 0xFFFF
static char_t s_chrBuf[CHR_BUF_MAX+1];
static int    s_chrBuf_size;

int chrBuf_cmp(const void *a, const void *b)
{
    return *(char_t*)a - *(char_t*)b;
}


static int ChrChkAdd(int c)
{
    int i;

    // 出現順に登録したいので、単純な検索
    for (i = s_chrBuf_size; --i >= 0;) {
        if (c == s_chrBuf[i])
            return 0;
    }
    //printf("%c%c", GHB(c), GLB(c));
    s_chrBuf[s_chrBuf_size++] = c;
    return 1;
}


void GetFile(SLIST *sl_first)
{
    SLIST*      sl;
    char        buf[0x2000];
    unsigned    c;

    s_chrBuf_size = 0;
    for (sl = sl_first; sl != NULL; sl = sl->link) {
        int         cp;
        ujfile_t*   uj = ujfile_fopen(sl->s, "rt");
        if (uj == NULL) {
            err_exit("%s : File open error.\n", sl->s);
        }
        cp  = ujfile_curCP(uj);
        if (cur_cp && cur_cp != cp && cur_cp != MBC_CP_1BYTE)
            err_exit("%s : Bad Codepage.\n", sl->s);
        cur_cp  = cp;
        cur_enc = mbc_cpToEnc(cur_cp);
        while (ujfile_fgets(buf, sizeof buf, uj)) {
            char* s = buf;
            for (;;) {
                s = StrSkipSpc(s);
                c = mbc_getChr(cur_enc, (char const**)&s);
                if (c == 0)
                    break;
                if (opt_noAscii && c < 0x80) {
                    continue;
                }
                if (opt_cmtChr) {
                    if (c == opt_cmtChr)
                        break;
                    else if (opt_cmtChr < 0 && c == '/' && *s == '/')
                        break;
                }
                ChrChkAdd(c);
            }
        }
        ujfile_fclose(uj);
    }
    s_chrBuf[s_chrBuf_size] = 0;
}



/*-----------------------------------------------------------------------*/
static char mlt_name[PATH_MAX], mlt_basename[PATH_MAX], mlt_ext[PATH_MAX];
static int  mlt_num;


static char const* chrToStr(char_t c)
{
    static char buf[16];
    char* p = mbc_strSetChr(cur_enc, buf, buf+sizeof(buf), c);
    *p = 0;
    return buf;
}


FILE *MltFileOpen(int md)
{
    FILE* fp;
    if (md)
        sprintf(mlt_name, "%s%03d.%s", mlt_basename, mlt_num, mlt_ext);
    else
        sprintf(mlt_name, "%s.%03d", mlt_basename, mlt_num);
    mlt_num++;
    fp = fopen(mlt_name, "wt");
    if (fp == NULL)
        err_exit("%s : File open error.\n", mlt_name);
    return fp;
}


void MltFile(char *inputname, char *oname, int w, int h, int md)
{
    FILE        *fp;
    char        buf[16];
    int         c,l,m,n;

    if (oname == NULL)
        oname = inputname;

    mlt_num = 0;
    if (md) {
        strcpy(mlt_ext, fpath_getExt(oname));
    }
    if (oname == NULL)
        oname = inputname;
    strcpy(mlt_basename, oname);
    fpath_chgExt(mlt_basename, NULL);

    fp = MltFileOpen(md);

    //c = 0x8140;
    c = ' ';
    for (m = l = n = 0; n < addLinNum*w; n++) {
        fprintf(fp, "%s", chrToStr(c));
        if (++m == w) {
            fprintf(fp,"\n");
            m = 0;
            ++l;
            if (h && l == h) {
                fprintf(fp,"\n");
                l = 0;
            }
        }
    }
    if (m)
        fprintf(fp,"\n");

    for (l = addLinNum, m = n = 0; n < s_chrBuf_size; n++) {
        c = s_chrBuf[n];
        fprintf(fp, "%s", chrToStr(c));
        if (++m == w) {
            fprintf(fp,"\n");
            fflush(fp);
            m = 0;
            ++l;
            if (h && l == h) {
                fclose(fp);
                fp = MltFileOpen(md);
                l = 0;
            }
        }
    }
    if (m)
        fprintf(fp,"\n");
    fclose(fp);
}



void OneFile(char *inputname, char *oname, int w, int h)
{
    FILE    *fp;
    char    buf[16];
    int     c,l,m,n;

    if (h == 0 && w) {
        h = (s_chrBuf_size + w - 1) / w;
    }

    if (oname == NULL) {
        printf("[%s] -> <stdout> %d*%d\n", inputname, w, h);
        fp = stdout;
    } else {
        printf("[%s] -> [%s] %d*%d\n", inputname, oname, w, h);
        fp = fopen(oname, "wt");
        if (fp == NULL)
            err_exit("%s : File open error.\n", oname);
    }

    if (addLinNum == 0) {
        for (l = m = n = 0; n < s_chrBuf_size; n++) {
            c = s_chrBuf[n];
            fprintf(fp, "%s", chrToStr(c));
            if (++m == w) {
                fprintf(fp,"\n");
                m = 0;
                ++l;
                if (h && l == h) {
                    fprintf(fp,"\n");
                    l = 0;
                }
            }
        }
    } else {
        //c = 0x8140;
        c = ' ';
        for (m = l = n = 0; n < addLinNum*w; n++) {
            fprintf(fp, "%s", chrToStr(c));
            if (++m == w) {
                fprintf(fp,"\n");
                m = 0;
                ++l;
                if (h && l == h) {
                    fprintf(fp,"\n");
                    l = 0;
                }
            }
        }
        if (m)
            fprintf(fp,"\n");
        for (m = 0, l = addLinNum, n = 0; n < s_chrBuf_size; n++) {
            c = s_chrBuf[n];
            fprintf(fp, "%s", chrToStr(c));
            if (++m == w) {
                fprintf(fp,"\n");
                m = 0;
                ++l;
                if (h && l == h) {
                    fprintf(fp,"\n");
                    l = 0;
                }
            }
        }
    }
    if (m)
        fprintf(fp,"\n");

    if (oname)
        fclose(fp);
}



/*---------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
    char    binname[260];
    int i;
    char *p;
 #if defined(_WIN32)
    int savCP = GetConsoleOutputCP();
    SetConsoleOutputCP(65001);
 #endif

    if (argc < 2)
        Usage();

 #ifdef EXARGV_INCLUDED
    ExArgv_conv(&argc, &argv);
 #endif

    /* コマンドライン解析 */
    for (i = 1; i < argc; i++) {
        p = argv[i];
        if (*p == '-') {
            Opts(p);
        } else {
            SLIST_Add(&fileList, p);
        }
    }

    if (fileList == NULL) {
        err_exit("no file\n");
    }

    cur_enc = mbc_cpToEnc(cur_cp);

    /* テキスト入力 */
    GetFile(fileList);

    /* ソートするとき */
    if (opt_sort) {
        qsort(s_chrBuf, s_chrBuf_size, sizeof(s_chrBuf[0]), chrBuf_cmp);
    }

    /* 1ファイルを処理 */
    if (opt_mltDivFlg)
        MltFile(fileList->s, opt_oname, opt_clm, opt_len, opt_mltDivFlg-1);
    else
        OneFile(fileList->s, opt_oname, opt_clm, opt_len);

    SLIST_Free(&fileList);

 #if defined(_WIN32)
    SetConsoleOutputCP(savCP);
 #endif
    return 0;
}
