/**
    @file   acpp.c
    @brief  cpp風マクロ・プリプロセッサ
    @author Masashi KITAMURA (tenka@6809.net)
    @date   1996-2025
    @license Boost Software License Version 1.0
    @note
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "Filn.h"

#if defined(_WIN32)
 #include <windows.h>
 #if !defined(USE_SJIS)
  #define USE_JAPANESE_CONSOLE()    SetConsoleOutputCP(65001)
  char   s_jpFlag  = 0;
 #endif
#endif


#define STDERR      stderr
int     err_printf(char *fmt, ...);

char*   g_appName = NULL;

static void Usage(void)
{
 #if defined(USE_JAPANESE_CONSOLE)
    if (s_jpFlag) {
        USE_JAPANESE_CONSOLE();
        err_printf("\nacpp v0.98 (" __DATE__ ")  by tenk*\n");
        err_printf("  cpp風マクロ・プリプロセッサ\n");
        err_printf(" usage> %s [-options] filename [-options]\n", g_appName);
        err_printf("  -d[NAME]  %cdefine NAME  1  をする\n", Filn->mac_chr);
        err_printf("  -u[NAME]  %cundef  NAME をする\n", Filn->mac_chr);
        err_printf(
               "  -?  -h    ヘルプ\n"
               "  -i[DIR]   include 時に詮索するディレクトリ\n"
               "  -o[FILE]  出力を FILE にする\n"
               "  -e[FILE]  エラー出力を FILE にする\n"
               "  -s        元ソースをコメントとして出力しない(-pc0 に同じ)\n"
               "  -w        無視\n"
               "  -p...     -pで始まるオプション\n"
               "  -p?       -pで始まるオプションのヘルプ\n"
        );
        err_printf("  -c[FILE]  FILE[.CFG] を %s のあるディレクトリから読み込む\n", g_appName);
        err_printf("  ※ 指定なくとも %s と同じフォルダにある .cfgファイル を読込む\n", g_appName);
    } else
 #endif
    {
        err_printf("acpp v0.98 (" __DATE__ ")  by tenk*\n");
        err_printf(" usage> %s [-options] filename [-options]\n", g_appName);
        err_printf("  -d[NAME]  %cdefine NAME 1\n", Filn->mac_chr);
        err_printf("  -u[NAME]  %cundef NAME\n", Filn->mac_chr);
        err_printf(
               "  -?  -h    Help\n"
               "  -i[DIR]   Directory to search on include\n"
               "  -o[FILE]  Output to FILE\n"
               "  -e[FILE]  Output errors to FILE\n"
               "  -s        Do not output the original source as comments (same as -pc0)\n"
               "  -w        Ignore\n"
               "  -p...     Option starting with -p\n"
               "  -p?       Help for options starting with -p\n"
        );
        err_printf("  -c[FILE]  Load FILE[.CFG] from the directory where %s is located\n", g_appName);

        err_printf("\n Even without specifying, .cfg file in the same folder as %s will be loaded\n", g_appName);
    }
    exit(1);
}



/* ------------------------------------------------------------------------ */
typedef struct SLIST {
    struct SLIST    *link;
    char            *s;
} SLIST;

int             debugflag;

static SLIST    *fileListTop = NULL;
static int      cmtchr_i=0,cmtcht_i=0;
static char     *outname = NULL;
static char     *errname = NULL;


static void ResFileRead(char *name);
static void OptsInit(void);
static int  Opts(char *a);
static char *Pchk(int n);
static void UsageOptP(void);
static char *Pchk(int n);

SLIST *SLIST_Add(SLIST **p0, char *s);
int freeE(void *p);
char *strdupE(char *s);
void *callocE(size_t a, size_t b);
void *mallocE(size_t a);
char *FIL_DelLastDirSep(char *dir);
char *FIL_AddExt(char filename[], char *ext);
char *FIL_ChgExt(char filename[], char *ext);
char *FIL_BaseName(char *adr);
volatile void err_exit(char *fmt, ...);



/* ------------------------------------------------------------------------ */

int Main(int argc, char *argv[])
{
    static char name[2100/*FIL_NMSZ*/];
    int i;
    char *p;
    SLIST *fl;
    FILE *fp;

    g_appName = strdupE(FIL_BaseName(argv[0]));

    if (Filn_Init() == NULL) {      /* ソース入力ルーチンの初期化 */
        err_exit("Initialize error.\n");
        return 1;
    }
    OptsInit();

    /* コンフィグ・ファイルの読み込み */
    FIL_ChgExt(strcpy(name, argv[0]), "cfg");
    ResFileRead(name);

    /* 引数が無ければヘルプ表示 */
    if (argc < 2)
        Usage();

    /* コマンドライン解析 */
    for (i = 1; i < argc; i++) {
        p = argv[i];
        if (*p == '-') {
            if (p[1] == 'C' || p[1] == 'c') {   /* コンフィグファイル読み込み */
                strcpy(name, argv[0]);
                *FIL_BaseName(name) = '\0';
                strcat(name, p+2);
                FIL_AddExt(name,"CFG");
                ResFileRead(name);
            } else {
                Opts(p);
            }
        } else if ( *p == '@') {
            ResFileRead(p+1);
        } else {
            SLIST_Add(&fileListTop, p);
        }
    }

    if (fileListTop == NULL) {  // ファイルがなければ標準入力する
        //err_exit("ファイル名を指定してください\n");
        SLIST_Add(&fileListTop, NULL);
    }

    /* エラー出力の準備 */
    if (Filn_ErrReOpen(errname, stderr) == NULL) {
        err_exit("Error-output-file open error.\n");
    }

    /* ソース出力ストリーム準備 */
    if (outname) {
        fp = fopen(outname,"wt");
        if (fp == NULL) {
            err_exit("%s : File open error.\n", outname);
        }
    } else {
        fp = stdout;
    }

    /* 全てのファイルを処理 */
    for (fl = fileListTop; fl != NULL; fl = fl->link) {
        p = fl->s;
        if (Filn_Open(p) < 0)
            exit(1);
        /* マクロ展開後のソースを出力 */
        for (;;) {
            p = Filn_Gets();
            if (p == NULL)
                break;
            fprintf(fp, "%s", p);
            freeE(p);
        }
    }

    /* 出力先のクローズ */
    if (fp != stdout)
        fclose(fp);

    /* ソース入力ルーチンの終了 */
    Filn_Term();

    return 0;
}



/**レスポンス・ファイルからファイル名,オプションを取り出す
 */
static void ResFileRead(char *name)
{
    FILE *fp;
    char bf[1024*2];
    char *p;
    int n;

    fp = fopen(name,"rt");
    if (fp == NULL)
        return;
    n = 0;
    while (fgets(bf,sizeof bf,fp) != NULL) {
        ++n;
        p = strtok(bf, " \t\n");
        while (p && *p && *p != ';') {
            if (*p == '-')
                Opts(p);
            else
                SLIST_Add(&fileListTop, p);
            p = strtok(NULL, " \t\n");
        }
    }
    if (ferror(fp)) {
        printf("%s %d : Read error.\n", name, n);
        exit(1);
    }
    fclose(fp);
}



static void OptsInit(void)
{
    Filn->opt_delspc    = 0;        /* 0以外ならば空白の圧縮を許す          */
    Filn->opt_dellf     = 1;        /* 0以外ならば￥改行による行連結を行う  */
    Filn->opt_sscomment = 1;        /* 0以外ならば//コメントを削除する      */
    Filn->opt_blkcomment= 1;        /* 0以外ならば／＊コメント＊／を削除する*/
    Filn->opt_kanji     = 1;        /* 0以外ならばMS全角に対応              */
    Filn->opt_sq_mode   = 1;        /* ' を ペアで文字定数として扱う        */
    Filn->opt_wq_mode   = 1;        /* " を ペアで文字列定数として扱う      */
    Filn->opt_mot_doll  = 0;        /* $ を モトローラな 16進数定数開始文字として扱う */
    Filn->opt_oct       = 1;        /* 1: 0から始まる数字は８進数  0:10進   */

    Filn->opt_orgSrc    = 3;        /* 1:元のソースもコメントにして出力 2:TAG JUMP形式 3:#line file  0:出力しない */

    Filn->orgSrcPre     = ";";      /* 元ソース出力時の行頭につける文字列   */
    Filn->orgSrcPost    = "";       /* 元ソース出力時の行末につける文字列   */
    Filn->immMode       = 0;        /* 1:符号付10進 2:符号無10進 3:0xFF 4:$FF X(5:0FFh) */
    Filn->cmt_chr[0]    = 0;        /* コメント開始文字になる文字           */
    Filn->cmt_chr[1]    = 0;        /* コメント開始文字になる文字           */
    Filn->cmt_chrTop[0] = 0;        /* 行頭コメント開始文字になる文字       */
    Filn->cmt_chrTop[1] = 0;        /* 行頭コメント開始文字になる文字       */
    Filn->macErrFlg     = 1;        /* マクロ中のエラー行番号も表示 1:する 0:しない */
    Filn->mac_chr       = '#';      /* マクロ行開始文字                     */
    Filn->mac_chr2      = '#';      /* マクロの特殊展開指定文字.            */
    Filn->localPrefix   = "_LCL_";
    Filn->opt_yen       = 1;        /* \\文字をCのように扱わない. 1:する 2:'"中のみ  3,4:変換もしちゃう(実験) */

    Filn->sym_chr_doll  = '$';
    Filn->sym_chr_atmk  = '@';
    Filn->sym_chr_qa    = '?';
    Filn->sym_chr_shp   = 0/*'#'*/;
    Filn->sym_chr_prd   = 0/*'.'*/;
}



/** オプションの処理
 */
static int Opts(char *a)
{
    char *p;
    int c;

    p = a;
    p++, c = *p++, c = toupper(c);
    switch(c) {
    case 'D':
        if (Filn_SetLabel(p, NULL))
            goto OPT_ERR;
        break;
    case 'U':
        if (Filn_UndefLabel(p))
            goto OPT_ERR;
        break;
    case 'I':
        Filn_AddIncDir(strdupE(p));
        break;
    case 'O':
        outname = strdupE(p);
        break;
    case 'E':
        errname = strdupE(p);
        break;
    case 'S':
        Filn->opt_orgSrc = 0;
        break;
    case 'W':
        break;
    case 'P':
        c = *p++, c = toupper(c);
        switch(c) {
        case 'C':
            if (*p == 'S' || *p == 's') {
                p++;
                Filn->orgSrcPre = strdupE(p);
            } else if (*p == 'E' || *p == 'e') {
                p++;
                Filn->orgSrcPost = strdupE(p);
            } else {
                Filn->opt_orgSrc = strtoul(p,&p,10);
            }
            break;
        case 'D':
            if (*p == 0)
                Filn->immMode = 1;
            else if (*p == '-')
                Filn->immMode = 0;
            else
                Filn->immMode = (int)strtoul(p, NULL, 10);
            if (Filn->immMode > 4)
                goto OPT_ERR;
            break;
        case 'P':
            Filn->localPrefix = strdupE(p);
            break;
        case 'Y':
            Filn->opt_yen = (*p == '1' || *p == 0) ? 1 : (*p == '2') ? 2 : 0;
            if (*p++) {
                if (*p == 't' || *p == 'T') {
                    Filn->opt_yen += 2;
                }
            }
            break;
        case 'T':
            Filn->opt_delspc = (*p == '-') ? 0 : 1; /* 0以外ならば空白の圧縮を許す */
            break;
        case 'F':
            Filn->opt_dellf  = (*p == '-') ? 0 : 1; /* 0以外ならば￥改行による行連結を行う */
            break;
        case 'S':
            Filn->opt_sscomment = (*p == '-') ? 0 : 1;  /* 0以外ならば//コメントを削除する */
            break;
        case 'B':
            Filn->opt_blkcomment= (*p == '-') ? 0 : 1;  /* 0以外ならば／＊コメント＊／を削除する */
            break;
        case 'J':
            Filn->opt_kanji = (*p == '-') ? 0 : 1;      /* 0以外ならばMS全角に対応 */
            break;
        case 'Q':
            if (*p == '0') {
                Filn->opt_sq_mode = 0;
            } else if (*p == '1') {
                Filn->opt_sq_mode = 1;
            } else if (*p == '2') {
                Filn->opt_sq_mode = 2;
            } else if (*p == 'W' || *p == 'w') {
                p++;
                if (*p == '0') {
                    Filn->opt_wq_mode = 0;
                } else if (*p == '1') {
                    Filn->opt_wq_mode = 1;
                } else if (*p == '2') {
                    Filn->opt_wq_mode = 2;
                } else {
                    goto OPT_ERR;
                }
            } else {
                goto OPT_ERR;
            }
            break;
        case 'R':
            c = toupper(*p); p++;
            if (c == 'Q') {
                Filn->opt_sq_mode = 2;
                if (*p == '-')
                    Filn->opt_sq_mode = 1;
            } else if (c == 'D') {
                Filn->opt_mot_doll = 1;
                if (*p == '-')
                    Filn->opt_mot_doll = 0;
            }
            break;
        case 'O':
            if (*p == '-') {
                Filn->opt_oct = 0;
            } else {
                Filn->opt_oct = 1;
            }
            break;
        case 'M':
            if (*p == 'M' || *p == 'm') {
                ++p;
                Filn->mac_chr2 = *p;                /* マクロ行開始文字 */
            } else {
                Filn->mac_chr  = *p;                /* マクロ行開始文字 */
                Filn->mac_chr2 = *p;                /* マクロ行開始文字 */
            }
            c = 0;
            goto N1;
        case 'L':
            c = *p;
            goto N1;
        case 'N':
            c = 0;
     N1:
            if (*p == '#' || *p == '@' || *p == '?' || *p == '$' || *p == '.') {
                if (*p == '$')  Filn->sym_chr_doll = c;
                if (*p == '@')  Filn->sym_chr_atmk = c;
                if (*p == '?')  Filn->sym_chr_qa   = c;
                if (*p == '#')  Filn->sym_chr_shp  = c;
                if (*p == '.')  Filn->sym_chr_prd  = c;
            } else {
                //err_exit("-pm,-pn,-plで指定出来る文字は # @ ? $ . のいずれかのみ\n");
                err_exit("Only # @ ? $ . can be specified for -pm, -pn, -pl\n");
            }
            break;
        case 'E':
            if (*p == 'T' || *p == 't') {
                if (cmtcht_i > 1) {
                    //err_exit("-pet の指定は 2個まで\n");
                    err_exit("The -pet option can be specified up to 2 times\n");
                }
                p++;
                Filn->cmt_chrTop[cmtcht_i++] = *p;
            } else {
                if (cmtchr_i > 1) {
                    //err_exit("-pe の指定は 3個まで\n");
                    err_exit("The -pe option can be specified up to 3 times\n");
                }
                Filn->cmt_chr[cmtchr_i++] = *p;
            }
            break;
        case 'Z':
            Filn->macErrFlg = (*p == '-') ? 0 : 1;      /* マクロ中のエラー行番号も表示 1:する 0:しない */
            break;

        case 'H':
        case '?':
        case '\0':
            UsageOptP();
        default:
            goto OPT_ERR;
        }
        break;

    case 'Z':
        debugflag = (*p == '-') ? 0 : 1;
        break;

    case '\0':  //何もしない。オプション無しで、標準入力したいとき用
        break;

    case 'H':
    case '?':
        Usage();
    default:
  OPT_ERR:
        err_exit("Incorrect command line option : %s\n", a);
    }
    return 0;
}



static char *Pchk(int n)
{
    //return (n) ? "する" : "しない";
    return (n) ? "On " : "Off";
}



static void UsageOptP(void)
{
    static char ac[] = "#$@?.", cmc[4] = "   ", cmc2[4] = "   ";
    static char *optYen[] = {"0","1","2","1t","2t",""};
    char *sc[5],*cc[5];
    int i;

    sc[0] = Filn->sym_chr_shp  ? " #":"";
    sc[1] = Filn->sym_chr_doll ? " $":"";
    sc[2] = Filn->sym_chr_atmk ? " @":"";
    sc[3] = Filn->sym_chr_qa   ? " ?":"";
    sc[4] = Filn->sym_chr_prd  ? " .":"";

    cc[0] = !Filn->sym_chr_shp  ? " #":"";
    cc[1] = !Filn->sym_chr_doll ? " $":"";
    cc[2] = !Filn->sym_chr_atmk ? " @":"";
    cc[3] = !Filn->sym_chr_qa   ? " ?":"";
    cc[4] = !Filn->sym_chr_prd  ? " .":"";

    cmc[0] = Filn->cmt_chr[0] ? Filn->cmt_chr[0] : ' ';
    cmc[2] = Filn->cmt_chr[1] ? Filn->cmt_chr[1] : ' ';
    cmc2[0] = Filn->cmt_chrTop[0] ? Filn->cmt_chrTop[0] : ' ';
    cmc2[2] = Filn->cmt_chrTop[1] ? Filn->cmt_chrTop[1] : ' ';

    for (i = 0; i < 5; i++) {
        if (ac[i] == Filn->mac_chr)
            cc[i] = "";
        if (ac[i] == Filn->mac_chr2)
            cc[i] = "";
    }

 #if defined(USE_JAPANESE_CONSOLE)
    if (s_jpFlag) {
        USE_JAPANESE_CONSOLE();
        printf("-pで始まるオプション：                                     現在の設定\n");
        printf("  -pm[C]    マクロ開始文字を C にする.   C は # $ @ ? .    %c\n", Filn->mac_chr);
        printf("  -pmm[C]   ##,#ラベル,#(式)での#を変更. C は # $ @ ? .    %c\n", Filn->mac_chr2);
        printf("  -pl[C]    C をラベル構成文字とする.    C は # $ @ ? .   %s%s%s%s%s\n",sc[0],sc[1],sc[2],sc[3],sc[4]);
        printf("  -pn[C]    C をラベル構成文字としない.  C は # $ @ ? .   %s%s%s%s%s\n",cc[0],cc[1],cc[2],cc[3],cc[4]);
        printf("  -pp[NAME] #local で生成するラベルのプレフィックス        %s\n", Filn->localPrefix);
        printf("  -pc[N]    元ソースをコメントにして出力 0:しない 1:する   %d\n", Filn->opt_orgSrc);
        printf("            2:TAG形式でする 3:# 行番号 ファイル名 を出力   \n");
        printf("  -pcs[STR] -pc1|2 時、コメントの行頭につける文字列        %s\n", Filn->orgSrcPre);
        printf("  -pce[STR] -pc1|2 時、コメントの行末につける文字列        %s\n", Filn->orgSrcPost);
        printf("  -pe[C]    C をコメント開始キャラとする(2個可)            %s\n", cmc);
        printf("  -pet[C]   C を行頭のみのコメント開始文字とする(2個可)    %s\n", cmc2);
        printf("  -py[N][t] \\の特殊扱い 0:無 1:有(''\"\"中のみ) t付:変換も   %s\n", optYen[Filn->opt_yen]);
        printf("  -ps[-]    //コメントを削除する           -ps- しない     %s\n", Pchk(Filn->opt_sscomment));
        printf("  -pb[-]    /*コメント*/を削除する         -pb- しない     %s\n", Pchk(Filn->opt_blkcomment));
        printf("  -pf[-]    \\改行コードによる行連結をする -pf- しない      %s\n", Pchk(Filn->opt_dellf));
        printf("  -pt[-]    空白の圧縮をする               -pt- しない     %s\n", Pchk(Filn->opt_delspc));
        printf("  -pj[-]    MS全角に対応する               -pj- しない     %s\n", Pchk(Filn->opt_kanji));
        printf("  -po[-]    0で始まる数を8進数にする       -po- しない     %s\n", Pchk(Filn->opt_oct));
        printf("  -prd[-]   $の扱いを16進数開始文字とする  -prd-しない     %s\n", Pchk(Filn->opt_mot_doll));
    //  printf("  -prq[-]   'を対でなく、'C で 68xxアセンブラ用にする      %s\n", Pchk(Filn->opt_sq_mode == 0));
        printf("  -pq[N]    '文字'を 0:無効 1:ペア'で有効 2:片'で有効('a)  %d\n", Filn->opt_sq_mode);
        printf("  -pqw[N]   '文字列'を 0:無効 1:有効                       %d\n", Filn->opt_wq_mode);
        printf("  -pd[-]    数値(31,0x3,0b10,077,'a'等)を十進数に変換      %s\n", Pchk(Filn->immMode));
    /*  printf("  -pd[N]    1:符号付き 2:符号無し十進  3:Cの16進数         \n");*/
    /*  printf("            4:ｲﾝﾃﾙな16進  5:ﾓﾄﾛｰﾗな16進                    \n");*/
    /*  printf("省略時: -pm# -pl$ -pl@ -pl? -pj -ps -pb -pf -pt- -pc3 -o-\n");*/
    } else
 #endif
    {
        printf("-Options starting with -p:                                    Current settings\n");
        printf("  -pm[C]    Set macro start character to C.     C is #$@?.    %c\n", Filn->mac_chr);
        printf("  -pmm[C]   Change # in ##, #label, #(expr).    C is #$@?.    %c\n", Filn->mac_chr2);
        printf("  -pl[C]    Make C a valid label character.     C is #$@?.   %s%s%s%s%s\n",sc[0],sc[1],sc[2],sc[3],sc[4]);
        printf("  -pn[C]    Make C not a valid label character. C is #$@?.   %s%s%s%s%s\n",cc[0],cc[1],cc[2],cc[3],cc[4]);
        printf("  -pp[NAME] Prefix for labels generated by #local             %s\n", Filn->localPrefix);
        printf("  -pc[N]    Output original source as comments 0:No 1:Yes     %d\n", Filn->opt_orgSrc);
        printf("            2:TAG format  3:# line-number file-name output\n");
        printf("  -pcs[STR] String to add at start of comment in -pc1|2       %s\n", Filn->orgSrcPre);
        printf("  -pce[STR] String to add at end of comment in -pc1|2         %s\n", Filn->orgSrcPost);
        printf("  -pe[C]    Use C as comment start character (up to 2)        %s\n", cmc);
        printf("  -pet[C]   Use C as line-head-only comment start char        %s\n", cmc2);
        printf("  -py[N][t] Special treatment for \\                           %s\n", optYen[Filn->opt_yen]);
        printf("            0:None 1:Yes (only in ''/\"\") t:convert as well\n");
        printf("  -ps[-]    Remove // comments           -ps- Do not remove   %s\n", Pchk(Filn->opt_sscomment));
        printf("  -pb[-]    Remove /* comments */        -pb- Do not remove   %s\n", Pchk(Filn->opt_blkcomment));
        printf("  -pf[-]    Join lines with backslash-newline  -pf- Do not    %s\n", Pchk(Filn->opt_dellf));
        printf("  -pt[-]    Compress spaces              -pt- Do not compress %s\n", Pchk(Filn->opt_delspc));
        printf("  -pj[-]    Support MS double-byte chars -pj- Do not support  %s\n", Pchk(Filn->opt_kanji));
        printf("  -po[-]    Numbers starting with 0 are octal   -po- Do not   %s\n", Pchk(Filn->opt_oct));
        printf("  -prd[-]   Treat $ as hex prefix        -prd- Do not treat   %s\n", Pchk(Filn->opt_mot_doll));
    //  printf("  -prq[-]   Use ' not as a pair, but as 'C for 68xx assembler %s\n", Pchk(Filn->opt_sq_mode == 0));
        printf("  -pq[N]    'char' is 0:invalid 1:pair 2:single 'a            %d\n", Filn->opt_sq_mode);
        printf("  -pqw[N]   'string' is 0:invalid 1:valid                     %d\n", Filn->opt_wq_mode);
        printf("  -pd[-]    Convert values (31,0b10,'a', etc.) to decimal     %s\n", Pchk(Filn->immMode));
    /*  printf("  -pd[N]    1:signed 2:unsigned decimal  3:C hex             \n");*/
    /*  printf("            4:intel hex   5:motorola hex                     \n");*/
    /*  printf("Default: -pm# -pl$ -pl@ -pl? -pj -ps -pb -pf -pt- -pc3 -o-\n");*/
    }
    exit(1);
}



/* ------------------------------------------------------------------------ */
#define ISKANJI(c)  ((unsigned char)(c) >= 0x81 && ((unsigned char)(c) <= 0x9F || ((unsigned char)(c) >= 0xE0 && (unsigned char)(c) <= 0xFC)))


volatile void err_exit(char *fmt, ...)
{
    va_list app;

    va_start(app, fmt);
    /*  err_printf("%s %5d : ", src_name, src_line);*/
    vfprintf(STDERR, fmt, app);
    va_end(app);
    exit(1);
}


int err_printf(char *fmt, ...)
{
    va_list app;
    int n;

    va_start(app, fmt);
    n = vfprintf(STDERR, fmt, app);
    va_end(app);
    fflush(STDERR);
    return n;
}


char *FIL_BaseName(char *adr)
{
    char *p;

    p = adr;
    while (*p != '\0') {
        if (*p == ':' || *p == '/' || *p == '\\')
            adr = p + 1;
      #ifdef USE_SJIS
        if (ISKANJI((*(unsigned char *)p)) && *(p+1) )
            p++;
      #endif
        p++;
    }
    return adr;
}


char *FIL_ChgExt(char filename[], char *ext)
{
    char *p;

    p = FIL_BaseName(filename);
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


char *FIL_AddExt(char filename[], char *ext)
{
    if (strrchr(FIL_BaseName(filename), '.') == NULL) {
        strcat(filename,".");
        strcat(filename, ext);
    }
    return filename;
}


char *FIL_DelLastDirSep(char *dir)
{
    char *p;

    if (dir) {
        p = FIL_BaseName(dir);
        if (strlen(p) > 1) {
            p = p+strlen(p);
            if (p[-1] == '\\' || p[-1] == '/')
                p[-1] = 0;
        }
    }
    return dir;
}



/** エラーがあれば即exitの malloc()
 */
void *mallocE(size_t a)
{
    void *p;

    if (a == 0)
        a = 1;
    p = malloc(a);
    if (p == NULL) {
        err_exit("Out of memory(%d byte(s))\n",a);
    }
    return p;
}


/** エラーがあれば即exitの calloc()
 */
void *callocE(size_t a, size_t b)
{
    void *p;

    if (b == 0)
        b = 1;
    p = calloc(a,b);
    if (p == NULL) {
        err_exit("Out of memory(%d*%d byte(s))\n",a,b);
    }
    return p;
}


/** エラーがあれば即exitの strdup()
 */
char *strdupE(char *s)
{
    char *p;
    if (s == NULL)
        return callocE(1,1);
    p = strdup(s);
    if (p == NULL) {
        //err_exit("メモリが足りない(長さ%d+1)\n",strlen(s));
        err_exit("Out of memory(%d+1)\n",strlen(s));
    }
    return p;
}


int freeE(void *p)
{
    if (p)
        free(p);
    return 0;
}



SLIST *SLIST_Add(SLIST **p0, char *s)
{
    SLIST* p;
    p = *p0;
    if (p == NULL) {
        p = callocE(1, sizeof(SLIST));
        if (s)
            p->s = strdupE(s);
        *p0 = p;
    } else {
        while (p->link != NULL) {
            p = p->link;
        }
        p->link = callocE(1, sizeof(SLIST));
        p = p->link;
        if (s)
            p->s = strdupE(s);
    }
    return p;
}


/* ------------------------------------------------------------------------ */
#if !defined(_WIN32)
int main(int argc, char *argv[])
{
    return Main(argc, argv);
}
#else
int main(int argc, char *argv[])
{
    int rc;
    int savCP = GetConsoleOutputCP();
 #if defined(USE_JAPANESE_CONSOLE)
    //SetConsoleOutputCP(65001);
    s_jpFlag  = GetSystemDefaultLangID() == 0x0411;
 #endif
    rc = Main(argc, argv);
    SetConsoleOutputCP(savCP);
    return rc;
}
#endif
