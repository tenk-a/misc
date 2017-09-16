/*
    caj     半角<->全角変換ﾌﾟﾛｸﾞﾗﾑ
    C言語版 Ver. 1.00  1991.06.20  os9/09 & MS-DOS    Writen by M.kitamura
            Ver. 1.01  1991.08.23  jstrcnv虫取り
            Ver. 1.02  1992.10.25  jstrcnv虫取り,indent(5->4),-y[-]追加 etc
            Ver. 1.03  1995.12.30  os9関係を削除. ANSI-Cｽﾀｲﾙに修正. -cy追加
            Ver. 1.04  1999.12.04  -ck追加
            Ver. 1.04b 2017.09     コンパイル警告修正
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "jstr.h"

#define STDERR          stdout

#ifdef C16  // for 16bit CPU
#define NAM_SIZE        (260*2)
#define BUF_SIZE        (1024*5)
#else
#define NAM_SIZE        (1030)
#define BUF_SIZE        (0x8000)
#endif

typedef unsigned char   UCHAR;
#define iskanji(c)      ((UCHAR)(c) > 0x80U && ((UCHAR)(c) <= 0x9FU || ((UCHAR)(c) >= 0xE0U && (UCHAR)(c) <= 0xFCU)))
#define iskanji2(c)     ((UCHAR)(c) >= 0x40U && (UCHAR)(c) <= 0xfcU && (UCHAR)(c) != 0x7fU)

#define eputs_exit(s)  (fputs((s),STDERR),exit(1))

void usage(void)
{
    fputs(
        "usage: caj [-opts] file...                     // CAJ v1.04\n"
        " -h -? ヘルプ\n"
        " -e  半角を全角に変換(-epdakn) -j  全角を半角に変換(-jpdak)\n"
        " -ep 半角記号を全角化(空白除)  -jp 全角記号を半角化(空白含)\n"
        " -ed 半角数字を全角化          -jd 全角数字を半角化\n"
        " -ea 半角ｱﾙﾌｧﾍﾞｯﾄを全角化      -ja 全角ｱﾙﾌｧﾍﾞｯﾄを半角化\n"
        " -eh 半角ｶﾅを全角ひらがな化    -jh 全角ひらがなを半角ｶﾅ化\n"
        " -ek 半角ｶﾅを全角カタカナ化    -jk 全角カタカナを半角ｶﾅ化\n"
        " -en 半角ｶﾅ記号を全角化        -jn 全角カナ記号を半角化\n"
        " -es 半角空白を全角化          -js 全角空白を半角空白2字\n"
        " -eo 半角の全角化で'ﾞ''ﾟ'をたえず1文字とする\n"
        " -jt ひらがなをカタカナ化      -jr カタカナをひらがな化\n"
        " -u  半角ｱﾙﾌｧﾍﾞｯﾄの大文字化    -ju 全角ｱﾙﾌｧﾍﾞｯﾄの大文字化\n"
        " -l  半角ｱﾙﾌｧﾍﾞｯﾄの小文字化    -jl 全角ｱﾙﾌｧﾍﾞｯﾄの小文字化\n"
        " -y  \\ に対応する全角を￥に    -y- \\に対応する全角を＼に\n"
        " -cy 全角2ﾊﾞｲﾄ目が\\の行を探す  -ck 全角文字の有る行を探す\n"
        " -o  各ﾌｧｲﾙの.bakﾌｧｲﾙを作り元のﾌｧｲﾙ名で出力\n"
        " -o=<path> 出力を<path>にする\n"
        , STDERR);
    exit(0);
}

void opts_err(void)
{
    eputs_exit("ｵﾌﾟｼｮﾝ指定がおかしい\n");
}

void rename_e(char const* oldname, char const* newname)
{
    if (rename(oldname,newname)) {
        fprintf(STDERR,"%s を %s に rename できません\n",oldname,newname);
        exit(errno);
    }
}

FILE *fopen_e(char const* name, char const* mode)
{
    FILE *fp = fopen(name,mode);
    if (fp == NULL) {
        fprintf(STDERR,"\n %s を open できません\n",name);
        exit(errno);
    }
    return fp;
}


char *fil_basename(char const* adr)
{
    char const* p = adr;
    while (*p != '\0') {
        if (*p == ':' || *p == '/' || *p == '\\')
            adr = p + 1;
        if (iskanji((*(UCHAR *)p)) && *(p+1) )/*全角交じりFILE名への対応*/
            p++;
        p++;
    }
    return (char*)adr;
}

char *fil_chgext(char filename[], char const* ext)
{
    char* p = fil_basename(filename);
    p = strrchr( p, '.');
    if (p == NULL) {
        strcat(filename,".");
        strcat( filename, ext);
    } else {
        strcpy(p+1, ext);
    }
    return filename;
}


static char ibuf[BUF_SIZE+4];
static char obuf[BUF_SIZE * 2 + 256];


void chkzenyen(FILE *fp, char const* fnam)
{
    UCHAR*          p;
    unsigned long   cnt = 0L;

    for (; ;) {
        ++cnt;
        memset(ibuf,0,BUF_SIZE+2);
        if (fgets(ibuf,BUF_SIZE,fp) == NULL)
            break;
        for (p = (UCHAR*)ibuf; *p != '\n' && *p != '\0' && p < (UCHAR*)ibuf+BUF_SIZE; p++) {
            if (iskanji(*p)) {
                p++;
                if (*p == '\\') {
                    fprintf(STDERR,"%s %ld : %c%c の２バイト目は '\\'.\n",fnam,cnt,*(p-1),*p);
                } else if (*p == '\0') {
                    fprintf(STDERR,"%s %ld : 警告>不正全角がある(2ﾊﾞｲﾄ目が 0)\n",fnam,cnt);
                } else if (!iskanji2(*p)) {
                    fprintf(STDERR,"%s %ld : 警告>不正全角がある\n",fnam,cnt);
                }
            }
        }
      #if 10
        if (*p != '\n') {
            fprintf(STDERR,
                    "%s %ld : 警告>この行に改行がない(1行が長すぎたか,NUL文字が交じってたか,ﾌｧｲﾙの終わりか)\n"
                    ,fnam,cnt);
        }
      #endif
    }
    if (ferror(fp))
        eputs_exit("入力でエラーが発生した\n");
}


void findZen(FILE* fp, char const* fnam)
{
    UCHAR*          p;
    unsigned long   cnt = 0L;

    for (; ;) {
        ++cnt;
        memset(ibuf,0,BUF_SIZE+2);
        if (fgets(ibuf,BUF_SIZE,fp) == NULL)
            break;
        for (p = (UCHAR*)ibuf; *p != '\n' && *p != '\0' && p < (UCHAR*)ibuf+BUF_SIZE; p++) {
            if (iskanji(*p)) {
                printf("%s %ld : %s", fnam, cnt, ibuf);
                break;
            }
        }
    }
}


void one(FILE *fp, FILE *ofp, unsigned opt, unsigned opt2, char const* fnam)
{
    int  k;
    unsigned long cnt = 0L;

    while (fgets(ibuf,BUF_SIZE,fp)) {
        ++cnt;
        if (jstrcnv(obuf,ibuf,opt) == NULL)
            fprintf(STDERR,"\n%s %ld : 不正な文字がある\n", fnam, cnt);
        if (opt2 > 0)
            jstruplow(obuf,opt2);
        k = fputs(obuf,ofp);
        if (k < 0)
            eputs_exit("出力でエラーが発生した\n");
    }
    if (ferror(fp))
        eputs_exit("入力でエラーが発生した\n");
}


int main(int argc, char *argv[])
{
    static char bakname[NAM_SIZE];
    static char tmpname[NAM_SIZE];
    FILE        *fp,*ofp;
    char        *oname = NULL;
    unsigned    mode,mode2,chk2yen,chk2kan;
    unsigned    i;
    char        *p;
    int         c,n;

    oname = NULL;
    chk2kan = chk2yen = mode = mode2 = 0;
    for (n = i = 1; i < (unsigned)argc; i++) {
        p = argv[i];
        if (*p != '-') {
            n = 0;
        } else {
            c = *++p;
            switch (toupper(c)) {
            case 'E':
                if (*(p + 1) == '\0') {
                    mode |= TOJPUN|TOJALPH|TOJDGT|TOJKATA|TOJKPUN;
                    break;
                }
                while ((c = *(++p)) != '\0') {
                    switch (toupper(c)) {
                    case 'P':
                        mode |= TOJPUN;
                        break;
                    case 'S':
                        mode |= TOJSPC;
                        break;
                    case 'A':
                        mode |= TOJALPH;
                        break;
                    case 'D':
                        mode |= TOJDGT;
                        break;
                    case 'H':
                        mode |= TOJHIRA;
                        break;
                    case 'K':
                        mode |= TOJKATA;
                        break;
                    case 'N':
                        mode |= TOJKPUN;
                        break;
                    case 'O':
                        mode |= DAKUOFF;
                        break;
                    default:
                        opts_err();
                    }
                }
                break;
            case 'J':
                if (*(p + 1) == '\0') {
                    mode |=JTOPUNS|JTOALPH|JTODGT|JTOKATA;
                    break;
                }
                while ((c = *(++p)) != '\0') {
                    switch (toupper(c)) {
                    case 'P':
                        mode |= JTOPUNS;
                        break;
                    case 'S':
                        mode |= JSPC2SPC;
                        break;
                    case 'A':
                        mode |= JTOALPH;
                        break;
                    case 'D':
                        mode |= JTODGT;
                        break;
                    case 'H':
                        mode |= JTOHIRA;
                        break;
                    case 'K':
                        mode |= JTOKATA;
                        break;
                    case 'N':
                        mode |= JTOKPUN;
                        break;
                    case 'U':
                        mode2 |= J2UPPER;
                        break;
                    case 'L':
                        mode2 |= J2LOWER;
                        break;
                    case 'T':
                        mode2 |= J2KATA;
                        break;
                    case 'R':
                        mode2 |= J2HIRA;
                        break;
                    default:
                        opts_err();
                    }
                }
                break;
            case 'U':
                mode2 |= A2UPPER;
                break;
            case 'L':
                mode2 |= A2LOWER;
                break;
            case 'Y':
                mode &= ~YENOFF;
                if (*(++p) == '-')
                    mode |= YENOFF;
                break;
            case 'C':
                c = *(++p);
                if (c == 'Y'||c == 'y') {
                    chk2yen = 1;
                }
                if (c == 'k'||c == 'K') {
                    chk2kan = 1;
                }
                break;
            case 'O':
                if (*(++p) == '=')
                    ++p;
                oname = p;
                break;
            case 'H':
            case '?':
            case '\0':
                usage();
            default:
                opts_err();
            }
        }
    }

    ofp = stdout;
    c = 0;
    if (oname == NULL) {
        c = 2;
    } else if (*oname != '\0') {
        c++; /* c = 1; */
        ofp = fopen_e(oname,"w");
    }
    if (n) {
        if (chk2yen)
            chkzenyen(stdin,"<stdin>");
        else if (chk2kan)
            findZen(stdin,"<stdin>");
        else
            one(stdin,ofp,mode,mode2, "<stdin>");
    } else {
        for (i = 1; i < (unsigned)argc; i++) {
            p = argv[i];
            if (*p == '-')
                continue;
            fp = fopen_e(p,"r");
            if (c == 0) {
                fil_chgext(strcpy(tmpname,p),"TMP");
                ofp = fopen_e(tmpname,"w");
            }
            if (chk2yen)
                chkzenyen(fp,p);
            else if (chk2kan)
                findZen(fp,p);
            else
                one(fp,ofp,mode,mode2,p);
            fclose(fp);

            if (c == 0) {
                fclose(ofp);
                fil_chgext(strcpy(bakname,p),"bak");
                unlink(bakname);
                rename_e(p,bakname);
                rename_e(tmpname,p);
            }
        }
    }
    if (c == 1)
        fclose(ofp);
    return 0;
}
