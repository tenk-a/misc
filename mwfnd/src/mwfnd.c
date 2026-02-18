/*
    mwfnd
    1995/??/??  v1.00   作成
    1996/??/??  v1.01   -m モードを付加
    1997/08/17  v1.02   usage修正. bcc32で再コンパイルするための修正.
    2007/04/28  v1.03   -l追加. ソース整形.
    200?/??/??  v1.04   -l0対応. -i追加. -a追加.
    2022/11/25  v1.05   -l拡張.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <limits.h>
#include "tree.h"


/** 使い方表示.
 */
void usage(void)
{
    puts("[mwfnd] 複数のファイルから複数の単語を検索し、各々の見つかった位置を出力\n"
         "usage mwfnd [-option(s)] <file(s)>\n"
         "  -oNAME    出力ファイル指定\n"
         "  --        これより後ろの名前はファイル名\n"
         "  ++        これより後ろの名前は検索単語名\n"
         "  -q        参照の無い単語は出力しない\n"
         "  -n        参照は出力せず見つかった単語のみ出力\n"
         "  -lN       参照がN以下の数のときのみ出力\n"
         "  -lN:M     参照が [N,M] 範囲の数のときのみ出力\n"
         "  -m[-]     見つかった行の内容を保持(デフォルト)  -m- しない\n"
         "  -i        名前の大文字小文字を無視\n"
         "  -aCHARS   英数 _ 以外の文字を名前に使えるようにする\n"
         "  -xN       出力モード\n"
         "  -v-       メッセージ出力off\n"
         "  @RESFILE  レスポンス・ファイル入力\n");
    exit(0);
}



/*------------------------------------------------------------------------*/
typedef unsigned char UCHAR;

#ifdef UNIX                 /** utf8|euc-jpを想定. */
#define FNAME_ISDIRSEP(c)   ((c) == '/' || (c) == ':')
#define ISKANJI(c)          (0)
#define STRCASECMP(a,b)     strcasecmp(a,b)
#else
#define FNAME_ISDIRSEP(c)   ((c) == ':' || (c) == '/' || (c) == '\\')
#define ISKANJI(c)          ((UCHAR)(c) >= 0x81 && ( (UCHAR)(c) <= 0x9F || ((UCHAR)(c) >= 0xE0 && (UCHAR)(c) <= 0xFC)))
/*#define ISKANJI(c)        ((unsigned)((c)^0x20) - 0xA1 < 0x3C) */
/*#define ISKANJI2(c)       ((UCHAR)(c) >= 0x40 && (UCHAR)(c) <= 0xfc && (c) != 0x7f) */
#define STRCASECMP(a,b)     stricmp(a,b)
#endif


#ifdef DOS16    // 16bit MS-DOS の場合
#define MAX_PATH        (264)
#define LINE_SIZE       (1024)
#define TOKENNAME_SIZE  (260)
#define MLINE           0
#else           // 32ビット環境の場合.
#define MAX_PATH        (0x2000)
#define LINE_SIZE       (0x8000)
#define TOKENNAME_SIZE  (0x1000)
#define MLINE           1
#endif



int     opt_qk          = 0;    /**< onなら 参照の無い単語は表示しない.     */
int     opt_msgFlg      = 1;    /**< 経過メッセージを表示する/しない        */
int     opt_nameOnly    = 0;    /**< 名前しか表示しない.                    */
int     opt_ex          = 0;    /**< file line : word で表示                */
int     opt_miNum       = -1;   /**< 非負の指定数未満の参照数なら表示無  .  */
int     opt_maNum       = -1;   /**< 非負の指定数以上参照があるなら表示無.  */
int     opt_ignorecase  = 0;    /**< 大文字小文字の区別をしない             */
int     opt_dq          = 0;
char    opt_outname[MAX_PATH];  /**< 出力ファイル名.                        */
char*   opt_addnamechr  = "";   /**< 英数_以外の(記号)文字を名前に含める    */

#ifdef MLINE
int     opt_linmemFlg   =MLINE; /**< 見つかった行の内容を保持.              */
#endif



/** オプション解析
 */
void options(char *s)
{
    char    *p;
    int     c;

    p   = s;
    p++;
    c   = *p++;
    c   = toupper(c);

    switch (c) {
    case 'Q':       /* 参照の無い単語は表示しない.      */
        opt_qk = 1;
        break;

    case 'V':       /* 経過メッセージを表示する/しない  */
        opt_msgFlg = 1;
        if (*p == '-')
            opt_msgFlg = 0;
        break;

    case 'X':       /* 表示モード. */
        if (*p == 0)
            opt_ex = 1;
        else
            opt_ex = strtoul(p, NULL, 10);
        break;

    case 'N':       /* 参照は表示せず見つかった単語のみ出力. */
        opt_nameOnly = 1;
        break;

    case 'L':       /* 参照がN以下の数のときのみ出力        */
        opt_maNum    = strtoul(p, &p, 0);
        if (*p == ':' || *p == '~' || *p == '-' || *p == ',') {
            ++p;
            opt_miNum = opt_maNum;
            opt_maNum = strtoul(p, &p, 0);
            if (opt_maNum < opt_miNum)
                opt_maNum = INT_MAX;
        }
        break;

    case 'I':
        opt_ignorecase = (*p != '-');
        break;

    case 'A':
        opt_addnamechr = strdup(p);
        break;

  #ifdef MLINE
    case 'M':       /* 見つかった行の内容を保持 */
        opt_linmemFlg     = 1;
        if (*p == '-')
            opt_linmemFlg = 0;
        break;
  #endif

    case 'O':       /* 出力ファイル指定         */
        strncpy(opt_outname, p, MAX_PATH-1);
        opt_outname[MAX_PATH-1] = '\0';
        break;

    case 'D':
        if (*p == 'q' || *p == 'Q')
            opt_dq = 1;
        break;

    /* case '\0': */
    case 'H':
    case '?':       /* ヘルプ                   */
        usage();

    default:
        printf("Incorrect command line option : %s\n", s);
        exit(1);
    }
}



/*---------- エラー処理付きの標準関数-----------------------*/

void       *mallocE(size_t a)
{
    void       *p;

    p = malloc(a);
    if (p == NULL) {
        printf("メモリが足りない\n");
        exit(1);
    }
    return p;
}



void       *callocE(size_t a, size_t b)
{
    void       *p;

    p = calloc(a, b);
    if (p == NULL) {
        printf("メモリが足りない\n");
        exit(1);
    }
    return p;
}



char       *strdupE(char *p)
{
    p = strdup(p);
    if (p == NULL) {
        printf("メモリが足りない\n");
        exit(1);
    }
    return p;
}



FILE       *fopenE(char *name, char *mod)
{
    FILE       *fp;

    fp = fopen(name, mod);
    if (fp == NULL) {
        printf("ファイル %s をオープンできません\n", name);
        exit(1);
    }
    return fp;
}



/*------------------------------------------------------------------------*/
typedef unsigned short LINENUM_T;

typedef struct LBLINK_T {
    struct LBLINK_T *next;
    char            *fname;
    LINENUM_T       line;
  #ifdef MLINE
    char            *mlin;
  #endif
} LBLINK_T;


typedef struct LBL_T {
    LBLINK_T   *next;                           /**< ラベルの参照リスト */
    char       *name;                           /**< 定義ラベル名       */
} LBL_T;

TREE        *LBL_tree;                          /**< ラベルを登録する木 */

char        *LBL_fname = NULL;                  /**< 現在のファイル名   */
LINENUM_T   LBL_line;                           /**< 現在の行番号       */



/** TREE ルーチンで、新しい要素を造るときに呼ばれる.
 */
static void *LBL_new(LBL_T * t)
{
    LBL_T      *p;

    p = callocE(1, sizeof(LBL_T));
    memcpy(p, t, sizeof(LBL_T));
    p->name = strdupE(t->name);
    return p;
}



/** TREE ルーチンで、メモリ開放のときに呼ばれる.
 */
static void LBL_del(void *ff)
{
    free(ff);
}



/** TREE ルーチンで、用いられる比較条件.
 */
static int LBL_cmp(LBL_T * f1, LBL_T * f2)
{
    if (opt_ignorecase)
        return STRCASECMP(f1->name, f2->name);
    else
        return strcmp(f1->name, f2->name);
}



/** TREE を初期化.
 */
void LBL_init(void)
{
    LBL_tree = TREE_Make((TREE_NEW) LBL_new, (TREE_DEL) LBL_del, (TREE_CMP) LBL_cmp, (TREE_MALLOC) mallocE, (TREE_FREE) free);
}



/** TREE を開放.
 */
void LBL_term(void)
{
    TREE_Clear(LBL_tree);
}



/** 現在の名前が木に登録されたラベルかどうか探す.
 */
LBL_T      *LBL_search(char *lbl_name)
{
    LBL_T       t;

    memset(&t, 0, sizeof(LBL_T));
    t.name = lbl_name;
    if (t.name == NULL) {
        printf("メモリがたりないぜよ\n");
        exit(1);
    }
    t.next = NULL;
    return TREE_Search(LBL_tree, &t);
}



/** ラベル(名前)を木に登録する.
 */
void LBL_add(char *lbl_name)
{
    LBL_T       t;

    memset(&t, 0, sizeof(LBL_T));
    t.name = lbl_name;
    t.next = NULL;
    TREE_Insert(LBL_tree, &t);
    if (LBL_tree->flag == 0) {                  /* 新規登録でなかった */
        printf("%-12s\t%6ld : %s が多重定義かも\n", LBL_fname, (long)LBL_line, lbl_name);
    }
}



/*---------------------------------------------------------------------*/

#define CHK_EOS(c)      ((c) == '\n' || (c) == '\0')
#define CHK_LBLKIGO(c)  ((c) == '_' /*|| (c) == '@' || (c) == '$'*/)



char        tokenName[TOKENNAME_SIZE + 2];      /**< 今回取得したラベル名 */



/** スペースのスキップ.
 * @return 次の文字位置.
 */
char    *skipSpc(char *s)
{
    while (*(unsigned char *) s <= 0x20 && !CHK_EOS(*s))
        s++;
    return s;
}



/** tokenNameにラベルをコピーする.
 *  @return 次の文字位置
 */
char    *getName(char *s)
{
    int     i;
    int     dq;

    i = 0;
    dq = 0;
    s = skipSpc(s);
    if (opt_dq) {
        if (*s == '"') {
            ++s;
            dq |= 1;
        }
        while (isalnum(*s) || CHK_LBLKIGO(*s) || strchr(opt_addnamechr, *s)) {
            if (i < TOKENNAME_SIZE - 1) {
                tokenName[i] = *s;
                i++;
            }
            s++;
        }
    } else {
        while (isalnum(*s) || CHK_LBLKIGO(*s) || strchr(opt_addnamechr, *s)) {
            if (i < TOKENNAME_SIZE - 1) {
                tokenName[i] = *s;
                i++;
            }
            s++;
        }
    }
    tokenName[i] = '\0';
    if (opt_dq) {
        if (*s == '"') {
            ++s;
            dq |= 2;
        }
        if (dq != 3) {
            tokenName[0] = '\0';
        }
    }

    /* 先頭が数字の時 */
    if (isdigit(tokenName[0])) {
        tokenName[0] = '\0';
    }
    return s;
}



/** ラベルを構成する文字以外をスキップする.
 *  @return 次の文字位置
 */
char    *skipKigo(char *s)
{
    s = skipSpc(s);
    while (!(isalnum(*s) || CHK_LBLKIGO(*s) || CHK_EOS(*s))) {
        if (opt_dq && *s == '"')
            break;
        s++;
    }
    return s;
}



/** 参照リストに追加する
 */
void    addRefList(LBL_T *p, char *linbuf)
{
    LBLINK_T   *k;
    LBLINK_T   *s;

    /* 登録 */
    k        = callocE(1, sizeof(LBLINK_T));
    k->next  = NULL;
    k->fname = LBL_fname;
    k->line  = LBL_line;
  #ifdef MLINE
    k->mlin  = strdupE(linbuf);
  #endif
    if (p->next == NULL) {
        p->next = k;
    } else {
        for (s = p->next; s->next != NULL; s = s->next);
        s->next = k;
    }
}



/** 定義ラベル各々の使われる(参照)位置を探しリストを作成
 */
void ref(char *name)
{
    char    buf[LINE_SIZE];
    FILE    *fp;
    char    *p;
    LBL_T   *link;

    LBL_fname = name;
    LBL_line  = 0;
    fp        = fopenE(name, "r");
    while (fgets(buf, sizeof buf, fp) != NULL) {
        LBL_line++;
        p = buf;
        for (;;) {
            p = skipKigo(p);
            if (CHK_EOS(*p))
                break;
            p = getName(p);
            if (tokenName[0] != '\0') {
                link = LBL_search(tokenName);
                if (link != NULL)
                    addRefList(link, buf);
            }
        }
    }

    if (ferror(fp)) {
        printf("%s %ld : リードエラーが起きました。\n", name, (long) LBL_line);
        exit(1);
    }

    fclose(fp);
}



/*-------------------------------------------------------------------------*/
FILE        *outFp;                             /* 出力ファイル */
int         fname_len = 4/*12 */;               /* 表示を揃えるための桁数 */

char       *Fname_baseName(char *adr);


/** １つの定義ラベル名と、その参照リストを表示
 */
void disp(void *p0)
{
    LBL_T      *p;
    LBLINK_T   *q;

    p = p0;

    if (opt_maNum >= 0) {           /* 数をチェックする場合 */
        int         n = 0;

        for (q = p->next; q != NULL; q = q->next) {
            ++n;
            if (n > opt_maNum)      /* 指定個数以上の参照があれば、それは無視 */
                return;
        }
        if (n < opt_miNum)
            return;
    }
    if (opt_nameOnly) {
        if (p->next)
            fprintf(outFp, "%s\n", p->name);
    } else {
        if (opt_qk && p->next == NULL)
            return;
        if (opt_ex) {
            if (opt_ex != 2) {      /* タグジャンプ形式 */
                if (opt_ex & 16) {
                    char const* base;
                  #ifdef MLINE
                    if (opt_linmemFlg) {
                        for (q = p->next; q != NULL; q = q->next) {
                            char* d = q->mlin;
                            while (*d) {
                                if (*d == '\t')
                                    *d = ' ';
                                ++d;
                            }
                            base = Fname_baseName(q->fname);
                            fprintf(outFp, "%-*s\t%6ld\t%s\t%s\t%s", fname_len, q->fname, (long) q->line, base, p->name, skipSpc(q->mlin));
                        }
                    } else
                  #endif
                    {
                        for (q = p->next; q != NULL; q = q->next) {
                            base = Fname_baseName(q->fname);
                            fprintf(outFp, "%-*s\t%6ld\t%s\t%s\n", fname_len, q->fname, (long) q->line, base, p->name);
                        }
                    }
                } else if (opt_ex & 8) {
                  #ifdef MLINE
                    if (opt_linmemFlg) {
                        for (q = p->next; q != NULL; q = q->next) {
                            char* d = q->mlin;
                            while (*d) {
                                if (*d == '\t')
                                    *d = ' ';
                                ++d;
                            }
                            fprintf(outFp, "%-*s\t%6ld\t%s\t%s", fname_len, q->fname, (long) q->line, p->name, skipSpc(q->mlin));
                        }
                    } else
                  #endif
                    {
                        for (q = p->next; q != NULL; q = q->next)
                            fprintf(outFp, "%-*s\t%6ld\t%s\n", fname_len, q->fname, (long) q->line, p->name);
                    }
                } else {
                  #ifdef MLINE
                    if (opt_linmemFlg) {
                        for (q = p->next; q != NULL; q = q->next)
                            fprintf(outFp, "%-*s\t%6ld [%s] : %s", fname_len, q->fname, (long) q->line, p->name, q->mlin);
                    } else
                  #endif
                    {
                        for (q = p->next; q != NULL; q = q->next)
                            fprintf(outFp, "%-*s\t%6ld : %s\n", fname_len, q->fname, (long) q->line, p->name);
                    }
                }
            } else {                /* ファイル名 : 単語 */
                for (q = p->next; q != NULL; q = q->next)
                    fprintf(outFp, "%-*s : %s\n", fname_len, q->fname, p->name);
            }
        } else {
            fprintf(outFp, "; %s\n", p->name);
            /* 参照行を表示 */
          #ifdef MLINE
            if (opt_linmemFlg) {
                for (q = p->next; q != NULL; q = q->next)
                    fprintf(outFp, "\t%-*s\t%6ld : %s", fname_len, q->fname, (long) q->line, q->mlin);
            } else
          #endif
            {
                for (q = p->next; q != NULL; q = q->next)
                    fprintf(outFp, "\t%-*s\t%6ld\n", fname_len, q->fname, (long) q->line);
            }
        }
    }
}



/*------------------------------------------------------------------------*/
char       *dspSameFile_name;

/**
 * １つの定義ラベル名と、その参照リストを表示
 */
void dspSameFile(void *p0)
{
    LBL_T      *p;
    LBLINK_T   *q;

    p = p0;
    for (q = p->next; q != NULL; q = q->next) {
        if (dspSameFile_name == q->fname) {
            fprintf(outFp, "\t%s\n", p->name);
            break;
        }
    }
}



/*------------------------------------------------------------------------*/

char       *Fname_baseName(char *adr)
{
    char       *p;

    p = adr;
    while (*p != '\0') {
        if (FNAME_ISDIRSEP(*p))
            adr = p + 1;
        if (ISKANJI(((unsigned char) *p)) && *(p + 1))
            p++;
        p++;
    }
    return adr;
}



char       *Fname_chgExt(char filename[], char *ext)
{
    char       *p;

    p = Fname_baseName(filename);
    p = strrchr(p, '.');
    if (p == NULL) {
        strcat(filename, ".");
        strcat(filename, ext);
    } else {
        strcpy(p + 1, ext);
    }
    return filename;
}



/*------------------------------------------------------------------------*/

typedef struct FLIST_T {
    struct FLIST_T *next;
    char       *fname;
    short       glb;
} FLIST_T;

FLIST_T    *FLIST_top = NULL;



/** ファイル名リストに追加.
 */
void FLIST_link(char *fname)
{
    FLIST_T    *lk;

    lk      = FLIST_top;
    if (lk == NULL) {
        lk  = FLIST_top = callocE(1, sizeof(FLIST_T));
    } else {
        while (lk->next != NULL) {
            lk   = lk->next;
        }
        lk->next = callocE(1, sizeof(FLIST_T));
        lk       = lk->next;
    }
    lk->fname    = strdupE(fname);
    lk->next     = NULL;
}



/*------------------------------------------------------------------------*/
int         flg_sep = 0;


/** コマンドライン引数の、1語の処理.
 */
void getWord(char *p)
{
    if (strcmp(p, "--") == 0) {
        flg_sep = 1;
    } else if (strcmp(p, "++") == 0) {
        flg_sep = 0;
    } else if (flg_sep == 0 && *p == '-') {
        options(p);
    } else {
        if (flg_sep == 0)   /* 単語を取得       */
            LBL_add(p);
        else                /* ファイル名を取得 */
            FLIST_link(p);
    }
}



/**
 *レスポンス・ファイルからファイル名を取り出す
 */
void readResFile(char *name)
{
    FILE    *fp;
    char    bf[LINE_SIZE];
    char    fnam[MAX_PATH];
    char    *p;
    int     i;
    int     n;

    fp = fopenE(name, "r");
    n  = 0;
    while (fgets(bf, sizeof bf, fp) != NULL) {
        ++n;
        p = bf;
        for (;;) {
            p = skipSpc(p);
            if (CHK_EOS(*p) || *p == ';')
                break;
            i = 0;
            while ((unsigned char) *p > 0x20)
                fnam[i++] = *p++;
            fnam[i] = '\0';
            getWord(fnam);
        }
    }
    if (ferror(fp)) {
        printf("%s %d : リードエラー\n", name, n);
        exit(1);
    }
    fclose(fp);
}



int main(int argc, char *argv[])
{
    int         i;
    char        *p;
    FLIST_T     *lk;

    if (argc < 2)
        usage();

    /* ラベルのツリー初期化 */
    LBL_init();

    /*-- オプション／ファイル名の取得 --*/
    for (i = 1; i < argc; i++) {
        p = argv[i];
        if (*p == '@')
            readResFile(p + 1);
        else
            getWord(p);
    }

    if (FLIST_top == NULL) {
        printf("ファイル名を指定してください\n");
        return 0;
    }

    /* ファイル名の文字列長を求める...表示での桁揃えのため */
    for (lk = FLIST_top; lk != NULL; lk = lk->next) {
        i = (int)strlen(lk->fname);
        if (i > fname_len)
            fname_len = i;
    }

    /*-- 参照処理 --*/
    for (lk = FLIST_top; lk != NULL; lk = lk->next) {
        if (opt_msgFlg)
            printf("[%s]\n", lk->fname);
        ref(lk->fname);
    }

    /*-- 結果出力 --*/
    if (opt_ex == 4) {
        for (lk = FLIST_top; lk != NULL; lk = lk->next) {
            dspSameFile_name  = lk->fname;
            strcpy(opt_outname, lk->fname);
            Fname_chgExt(opt_outname, "___");
            outFp = fopenE(opt_outname, "wt");

            /*fprintf(outFp, "%s\n", lk->fname); */
            TREE_DoAll(LBL_tree, dspSameFile);
            fclose(outFp);
        }
    } else {
        if (opt_outname[0] == 0)
            outFp = stdout;
        else
            outFp = fopenE(opt_outname, "wt");
        setvbuf(outFp, NULL, _IOFBF, 0xC000U);

        if (opt_ex != 3) {
            TREE_DoAll(LBL_tree, disp);
        } else {
            for (lk = FLIST_top; lk != NULL; lk = lk->next) {
                dspSameFile_name     = lk->fname;
                fprintf(outFp, "%s\n", lk->fname);
                TREE_DoAll(LBL_tree, dspSameFile);
            }
        }
        if (opt_outname[0])
            fclose(outFp);
    }

    /* ラベル・ツリー終了 */
    LBL_term();

    return 0;
}
