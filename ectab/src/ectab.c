/**
 *  @file   ectab.c
 *  @brief  空白タブ変換ツール.
 *
 *  @author Masashi Kitamura (tenka@6809.net)
 *  @date   2001(?)～ 2004-01-29,2024
 *  @license Boost Software Lisence Version 1.0
 *  @note
 *      アセンブラで書かれたmsdos-16bit-exe版をリメイク.
 *      Boost software license Version 1.0.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>
#if defined(_WIN32)
 #include <windows.h>
#endif
#ifndef NO_USE_EXARGV
#include "ExArgv.h"
#endif
#include "cmisc.h"
#include "ujfile.h"
#include "mbc.h"

#ifndef FNAME_MAX_PATH
#define FNAME_MAX_PATH  4100
#endif

#undef  BOOL
#define BOOL int


/** 説明表示＆終了.
 */
static int usage(void)
{
    printf("usage> ectab [-opts] file(s)    // v3.01 " __DATE__ "  writen by M.Kitamura\n"
           "            https://github.com/tenk-a/misc/tree/master/ectab\n"
           "  UTF8,SJIS変換やタブ変換,行末空白削除等を行う. デフォルトは標準出力.\n"
           "  -I[DIR]   DIRから入力.\n"
           "  -O[DIR]   DIRに出力.\n"
           "  -o[FILE]  FILEに出力. -o のみは元を.bakにして入力ファイル名で出力.\n"
           "  -x[EXT]   出力ファイル名に拡張子 EXT を付加.\n"
           "  -m        行末の空白を削除.\n"
           "  -r[0-3]   改行を 0:入力のまま 1:'\\n' 2:'\\r' 3:'\\r\\n' に変換(file出力時のみ)\n"
           "  -s[N]     出力のタブサイズを N にする(空白->タブ)\n"
           "  -t[N]     入力のタブサイズを N にする(タブ->空白)\n"
           "  -z        出力のタブサイズが空白1文字にしかならない場合は空白で出力.\n"
           "  -j        出力のタブサイズ丁度の空白のみタブに変換.\n"
           "  -q        出力を4タブ8タブで見た目が違わないようにする.\n"
           "  -b[0-2]   C/C++の \" '対を考慮 0:しない 1:する 2:対中のctrl文字を\\文字化.\n"
           "  -u        半角小文字の大文字化.\n"
           "  -l        半角大文字の小文字化.\n"
           "  -a        EOFとして0x1aを出力.\n"
           "  -k[ENC]   入力文字コード.[ENC]= AUTO,UTF8,SJIS,EUCJP.(SJIS,EUCJP はMS形式)\n"
           "            デフォルト:AUTO. (旧版互換で 0=ASC,1=SJIS,2=UTF8)\n"
           "  -p[ENC]   出力文字コード.[ENC]=UTF8,UTF8N,UTF8BOM,SJIS,EUCJP\n"
           "  -v        経過ログ出力.\n"
           "  -n[N:M:L:STR]  行番号を付加. N:桁数,M:スキップ数,L:0行目の行番号,STR:区切.\n"
    );
    return 1;
}


/// オプション設定.
typedef struct opts_t {
    int         stab;
    int         dtab;
    int         cmode;
    BOOL        both48;
    BOOL        sp1ntb;
    BOOL        ajstab;
    unsigned    crlfMd;
    BOOL        trimSw;
    mbc_cp_t    srcCP;
    mbc_cp_t    dstCP;
    signed char bomMode;
    BOOL        mbcSw;
    BOOL        utf8Sw;
    BOOL        eofSw;
    BOOL        uprSw;
    BOOL        lwrSw;
    BOOL        verbose;
    unsigned    numbering;
    unsigned    numbStart;
    int         numbSkip;
    char       *numbSep;

    char*       outname;
    char*       extname;
    char*       srcdir;
    char*       dstdir;
} opts_t;


typedef struct CharBuf {
    char*   ptr;
    size_t  capa;
} CharBuf;

void charbuf_create(CharBuf* b, size_t capa) {
    assert(b && capa > 0);
    b->ptr  = NULL;
    b->capa = capa;
    if (capa)
        b->ptr  = malloc(capa+1);
}

void charbuf_release(CharBuf* b) {
    free(b->ptr);
    b->ptr  = 0;
    b->capa = 0;
}

void charbuf_set(CharBuf* b, char const* src, size_t capa) {
    if (capa > b->capa || b->ptr == NULL) {
        char* a = (char*)realloc(b->ptr, capa+4);
        b->ptr  = a;
    }
    if (src && b->ptr) {
        if (capa)
            memcpy(b->ptr, src, capa);
        b->ptr[capa] = 0;
        b->ptr[capa+1] = 0;
        b->ptr[capa+2] = 0;
        b->ptr[capa+3] = 0;
    }
}

static unsigned checkLine(char* s, char* e) {
    unsigned flags = 0;
    while (s < e) {
        unsigned c = *s++;
        if (c < 0x20) {
            switch (c) {
            case '\a':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
            case '\v':
            case '\x1a':
            case '\x1b':
                break;
            case '\0':
                flags |= 1;
                s[-1] = ' ';    // '\0' to space.
                break;
            default:
                flags |= 2;
                break;
            }
        //} else if (c == 0xff) {
        //  flags |= 2;
        }
    }
    return flags;
}


#if _WIN32
static int s_console_codepage = 0;

void setConsoleCodePage(int cp)
{
    if (cp != s_console_codepage) {
        SetConsoleOutputCP(cp);
        s_console_codepage = cp;
    }
}
#endif

int err_printf(char const* fmt, ...)
{
    va_list ap;
 #if _WIN32
    setConsoleCodePage(65001);
 #endif
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    return 0;
}

static char const* strCodePage(int cp)
{
    switch (cp) {
    case MBC_CP_UTF8:       return "UTF8";
    case MBC_CP_UTF16LE:    return "UTF16(LE)";
    case MBC_CP_UTF16BE:    return "UTF16(BE)";
    case MBC_CP_UTF32LE:    return "UTF32(LE)";
    case MBC_CP_UTF32BE:    return "UTF32(BE)";
    case MBC_CP_SJIS:       return "SJIS(CP932)";
    case MBC_CP_EUCJP:      return "EUCjp(MS)";
    case MBC_CP_1BYTE:      return "ASCII";
    default: break;
    }
    return "";
}

/** iname のテキストをoの変換指定にしたがって変換し oname に出力.
 *  @param  iname   入力ファイル名. NULLなら標準入力.
 *  @param  oname   出力ファイル名. NULLなら標準出力.
 *  @return         0:失敗 1:成功.
 */
static int convFile(const char *iname, const char *oname, opts_t *o)
{
    enum {SBUF_SZ = 16*1024};
    CharBuf         tbuf = {0};
    CharBuf         sbuf = {0};
    FILE*           ofp;
    unsigned        lno = 0, numb;
    int             er = 0, rc = 1;
    int             crAsLf = 1; //(o->crlfMd & 4) != 0;
    int             crlfMd = o->crlfMd & 3;
    int             noLineBreak = crlfMd != 0;
    int             tabFlags, trimFlags, upLwrFlags;
    int             dstBom;
    char*           p;
    mbc_cp_t        curCP;
    mbc_enc_t       curEnc;
    ujfile_t*       uj = NULL;
    ujfile_opts_t   opts;
    opts.src_cp = o->srcCP;
    opts.dst_cp = o->dstCP;
    opts.remove_bom = 1;
    opts.crlf_to_lf = 0;
    opts.opt_getline= (crAsLf << 1) | noLineBreak;

    uj =ujfile_open(iname, &opts);
    if (uj == NULL) {
        if (iname)
            err_printf("%s : [ERROR] Read open error.\n", iname);
        return 0;
    }
    if (!iname)
        iname = "<stdin>";

    if (ujfile_unkownEnc(uj)) {
        err_printf("%s : [ERROR] Character encoding unknown.\n", iname);
        ujfile_close(&uj);
        return 0;
    }

    curCP  = (mbc_cp_t)ujfile_curCP(uj);
    curEnc = mbc_cpToEnc(curCP);
    dstBom = (curCP == MBC_CP_UTF8) && (o->bomMode > 0 || ujfile_hasBOM(uj));
    if (o->bomMode < 0)
        dstBom = 0;

    if (o->verbose) {
        int srcCP = ujfile_srcCP(uj);
        err_printf("%s : ", iname);
        err_printf("[%s%s]", strCodePage(srcCP), ujfile_hasBOM(uj) ? "-BOM" : "");
        if (srcCP != curCP)
            err_printf(" -> [%s%s]", strCodePage(curCP), dstBom ? "-BOM" : "");
        err_printf("\n");
    }

    if (oname) {
        ofp = fopen(oname, "wb");       // 改行を自前で管理したいのでバイナリでオープン.
        if (ofp == NULL) {
            err_printf("%s : [ERROR] Write open error.\n", oname);
            ujfile_close(&uj);
            return 0;
        }
    } else {
        oname = "<stdout>";
        ofp   = stdout;
    }

    // 行番号表示関係の設定.
    numb = o->numbStart;
    if (o->numbSkip == 0)
        o->numbSkip = 1;
    if (o->numbSep == NULL)
        o->numbSep = " ";

    // タブ変換関係の準備.
    tabFlags =  (o->cmode << 1) | (o->ajstab << 4) | (o->both48 << 5);
    if (o->cmode)
        tabFlags |= o->trimSw << 7;     // cモードの時は'"ペアのチェックの都合, strTabからstr_trimSpcRを呼ぶ.
    if (tabFlags && o->stab == 0) {     // 変換指定があるのに、ソースタブサイズが無い場合は、強制設定.
        if (o->cmode)   o->stab = 4;    // cモードなら4.
        else            o->stab = 8;    // 以外は 9.
    }
    if (o->stab || o->dtab || tabFlags)
        tabFlags |= (1<<6) | (o->sp1ntb);

    // タブ変換しないけど、行末空白削除しない場合は専用に呼び出す.
    trimFlags = 0;
    if (tabFlags == 0 && o->trimSw) {
        // bit0=1:行末の'\n''\r'は外さない. bit1=1:C/C++での\文字を考慮(行連結に化けないように)
        trimFlags = /*(o->cmode << 1) |*/ 1;
    }

    // 大文字小文字変換の設定.
    upLwrFlags = (o->lwrSw<<1) | o->uprSw;
    if (upLwrFlags)
        upLwrFlags |= (o->mbcSw << 7);

    // c/c++向け' " ペアチェックの初期化.
    strConvTab((mbc_enc_t)NULL, NULL, 0, NULL, 0,0,0);

 #if defined(_WIN32)
    if (ofp == stdout)
        setConsoleCodePage(curCP);
 #endif
    if (curCP == MBC_CP_UTF8 && dstBom)
        fputs("\xEF\xBB\xBF", ofp);

    charbuf_create(&sbuf, SBUF_SZ);
    charbuf_create(&tbuf, SBUF_SZ*4);

    // 変換本編.
    for (;;) {
        size_t  size = 0, crlfSize = 0;
        char const* curline = ujfile_getCurrentLine(uj, &size, &crlfSize);
        if (size == 0)
            break;
        ++lno;

        charbuf_set(&sbuf, curline, size);
        charbuf_set(&tbuf, NULL, size*4);

        // 壊れた全角文字がないかチェック.
        if (o->dstCP > 0 || o->srcCP > 0) {
            if (mbc_checkEnc(curEnc, curline, size, 0) == 0) {
                err_printf("%s (%d): There are broken multi-byte characters.\n", iname, lno);
            }
        }
        er = checkLine(sbuf.ptr, sbuf.ptr + size);
        if (er) {
            if (er & 0x01)
                err_printf("%s (%d): There was '\\0'.\n", iname, lno);
            if (er & 0x02)
                err_printf("%s (%d): Binary code ?\n", iname, lno);
        }

        numb += o->numbSkip;
        p = sbuf.ptr;
        // タブ空白変換.
        if (o->stab || o->dtab || tabFlags) {
            strConvTab(curEnc, tbuf.ptr, tbuf.capa, sbuf.ptr, o->dtab, o->stab, tabFlags);
            p = tbuf.ptr;
        }
        // 行末空白の削除.
        if (trimFlags) {
            str_trimSpcR(p, trimFlags);
        }
        // 文字列の大文字/小文字化.
        if (upLwrFlags) {
            mbc_strUpLow(curEnc, p, upLwrFlags);
        }

     #if defined(_WIN32)
        if (ofp == stdout)
            setConsoleCodePage(curCP);
     #endif
        // 行を出力.
        if (o->numbering) { // 行番号付き.
            fprintf(ofp, "%*d%s%s", o->numbering, numb, o->numbSep, p);
        } else {            // そのまま.
            fprintf(ofp, "%s", p);
        }
        if (crlfMd) {
            static char const* const s_Crlf[] = { "", "\n", "\r", "\r\n" };
            fputs(s_Crlf[crlfMd], ofp);
        }
        if (ferror(ofp))
            break;
    }

    // eofをつかる場合.
    if (o->eofSw) {
     #if defined(_WIN32)
        if (ofp == stdout)
            setConsoleCodePage(curCP);
     #endif
        fputs("\x1a", ofp);
    }
    // 後処理.
    if (ferror(ofp)) {
        err_printf("%s (%d): [ERROR] Write error.\n", oname, lno);
        rc = 0;
    }
    ujfile_close(&uj);
    if (ofp != stdout)
        fclose(ofp);
    charbuf_release(&sbuf);
    charbuf_release(&tbuf);
    return rc;
}


/** 1ファイルの変換. ファイル名の辻褄会わせ.
 *  @param  iname   入力パス. NULL なら標準入力.
 *  @param  oname   出力パス. NULL なら標準出力.
 *  @param  o       オプション.
 */
static int oneFile(const char *iname, const char *oname, opts_t *o)
{
    // ファイル名生成やバックアップの処理.
    char nameBuf[FNAME_MAX_PATH+8];
    char tmpname[FNAME_MAX_PATH+8];
    char const* orig_oname = NULL;
    int  rc;

    nameBuf[0] = tmpname[0] = 0;
    if (iname) {    // 入力ファイルがあるとき.
        // 出力ディレクトリ指定があるとき.
        if (o->dstdir && o->dstdir[0]) {
            char const* b = iname;
            if (fname_startsWith(b, o->srcdir)) {   // 入力パスが、-I入力ディレクトリ下なら,
                size_t ln = strlen(o->srcdir);
                b += ln;                            // 入力ディレクトリ下の相対パスを取得.
            } else if (fname_isAbsolutePath(b)) {   // 入力が絶対パスなら,
                b  = fname_baseName(b);             // ファイル名だけ取得.
            }
            size_t      bsz = strlen(b);
            size_t      l   = strlen(o->dstdir);
            size_t      extlen = (o->extname && o->extname[0]) ? strlen(o->extname)+1 : 0;
            if (l + bsz + extlen >= FNAME_MAX_PATH - 1) {
                err_printf("[ERROR] Destination path too long. : %s/%s\n", o->dstdir, b);
                return 1;
            }
            snprintf(nameBuf, FNAME_MAX_PATH, "%s%s%s", o->dstdir, b, extlen ? o->extname : "");
            fname_backslashToSlash(nameBuf);        // win/dos なら \ を / に置換.

            // 出力先ディレクトリが存在しなければ作成.
            {
                char* p = fname_baseName(nameBuf);
                if (nameBuf < p && p[-1] == '/') {
                    p[-1] = 0;
                    if (!file_exist(nameBuf))
                        file_recursive_mkdir(nameBuf, 755);
                    p[-1] = '/';
                }
            }
            oname = nameBuf;
        } else if (oname) {
            if (o->extname && o->extname[0]) {
                snprintf(nameBuf, FNAME_MAX_PATH, "%s%s", (oname[0] ? oname : iname), o->extname);
                oname = nameBuf;
            } else if (oname[0] == 0) {     //
                oname = iname;
            }
        }
    }

    if (oname && (oname == iname || file_exist(oname))) {
        orig_oname = oname;
        snprintf(tmpname, FNAME_MAX_PATH, "%s.~tmp", oname);
        oname   = tmpname;
    }

    rc = convFile(iname, oname, o);

    if (rc && orig_oname) {     // 入力ファイル自身の出力の場合.
        char bakname[FNAME_MAX_PATH+8];
        snprintf(bakname, FNAME_MAX_PATH, "%s.bak", orig_oname);
        remove(bakname);
        (void)rename(orig_oname, bakname);
        (void)rename(tmpname, orig_oname);
    }
    return 0;
}

/** オプション文字の次に'-'か'0'なら偽, 以外なら真にする為のマクロ.
 */
static int opts_getVal(const char *arg, char **pp, int dfltVal, int maxVal)
{
    char *p = *pp;
    int val;

    if (*p == '=')
        ++p;
    if (*p == '-') {
        *pp = p + 1;
        val = 0;
    } else if (isdigit(*p)) {
        val = strtol(p, pp, 0);
    } else {
        val = dfltVal;
    }
    if (val > maxVal) {
        err_printf("[ERROR] Option %s : Out of range. (%d > %d)\n", arg, val, maxVal);
        return -1;
    }
    return val;
}


static inline int striEqu(char const* l, char const* r)
{
    return strcasecmp(l,r) == 0;
}

static char* opts_pathDup(char const*path, char const* arg)
{
    char* dst;
    if (path && *path == '=')
        ++path;
    if (arg && (path == NULL || *path == 0)) {
        err_printf("[ERROR] Option %s : Not enough arguments.\n", arg);
        return NULL;
    }
    dst = strdupAddCapa(path, 4);   // 4bytes余分にメモリ確保.
    fname_backslashToSlash(dst);
    return dst;
}

/** オプション解析.
 */
static int opts_get(char *arg, opts_t *o)
{
    char *p = arg + 1;

    while (*p) {
        char* a = p;
        int   c = *p++;
        //c = toupper(c);
        switch (c) {
        case 'm': o->trimSw = opts_getVal(arg, &p, 1, 1); break;
        case 'r': o->crlfMd = opts_getVal(arg, &p, 0, 7); break;
        case 'a': o->eofSw  = opts_getVal(arg, &p, 1, 1); break;
        case 'u': o->uprSw  = opts_getVal(arg, &p, 1, 1); break;
        case 'l': o->lwrSw  = opts_getVal(arg, &p, 1, 1); break;
        case 'z': o->sp1ntb = opts_getVal(arg, &p, 1, 9) ? 1 : 0; break;
        case 'j': o->ajstab = opts_getVal(arg, &p, 1, 1); break;
        case 'q': o->both48 = opts_getVal(arg, &p, 1, 1); break;
        case 's': o->dtab   = opts_getVal(arg, &p, 4,256); break;
        case 't': o->stab   = opts_getVal(arg, &p, 4, 16); break;
        case 'v': o->verbose= opts_getVal(arg, &p, 1, 1); break;
        case 'b':
        case 'c':
            o->cmode  = opts_getVal(arg, &p, 1, 2);
            if (o->cmode == 2)
                o->cmode = 4|2|1;
            else if (o->cmode == 1)
                o->cmode = 4|1;
            break;
        case 'k':
        case 'p':
            {
                static mbc_cp_t const tbl[] = {
                    MBC_CP_1BYTE,
                    MBC_CP_SJIS,
                    MBC_CP_UTF8,
                    MBC_CP_EUCJP,
                    (mbc_cp_t)0,
                };
                int idx;
                if (*p == '=')
                    ++p;
                idx = striEqu(p,"ascii") || striEqu(p,"asc")   ? 0
                    : striEqu(p,"sjis" ) || striEqu(p,"cp932") ? 1
                    : striEqu(p,"utf8") || striEqu(p,"utf")   || striEqu(p,"unicode")
                    || striEqu(p,"nobom") || striEqu(p,"utf8n") || striEqu(p,"utf8nobom")
                    || striEqu(p,"bom")  || striEqu(p,"utf8bom") ? 2
                    : striEqu(p,"eucjpms") || striEqu(p,"eucjp") ? 3
                    : striEqu(p, "auto")                       ? 4
                    : -1;
                if (idx >= 0) {
                    if (c == 'K') {
                        o->srcCP = tbl[idx];
                    } else {
                        if (striEqu(p,"bom") || striEqu(p,"utf8bom")) {
                            o->bomMode = 1;
                        } else if ( striEqu(p,"nobom") || striEqu(p,"utf8n") || striEqu(p,"utf8nobom") ) {
                            o->bomMode = -1;
                        }
                        o->dstCP = tbl[idx];
                    }
                    return 0;
                } else {
                    idx = opts_getVal(arg, &p, 3, 3);
                }
            }
            break;
        case 'n':
            o->numbering = (*p == 0) ? 7 : strtoul(p,&p,0);
            if (*p == ':' || *p == ',') {
                p++;
                o->numbSkip = strtol(p,&p,0);
                if (*p == ':' || *p == ',') {
                    p++;
                    o->numbStart = strtoul(p, &p, 0);
                    if (*p == ':' || *p == ',') {
                        p++;
                        o->numbSep = strdup(p);
                        return 0;
                    }
                }
            }
            break;
        case 'o':
            o->outname = opts_pathDup(p, NULL);
            return o->outname == NULL;
        case 'x':
            o->extname = p = opts_pathDup(p, a);
            if (p && *p != '.') {
                memmove(p+1, p, strlen(p));
                *p = '.';
            }
            return p == NULL;
        case 'I':
            o->srcdir  = opts_pathDup(p, a);
            if (o->srcdir == NULL || o->srcdir[0] == 0)
                return 1;
            fname_addDirSep(o->srcdir, strlen(o->srcdir)+2);
            return 0;
        case 'O':
        case 'd':
            o->dstdir  = opts_pathDup(p, a);
            if (o->dstdir == NULL || o->dstdir[0] == 0)
                return 1;
            fname_removeDirSep(o->dstdir);
            file_recursive_mkdir(o->dstdir,755);
            fname_addDirSep(o->dstdir, strlen(o->dstdir)+2);
            return 0;
        case 'h':
        case '?':
            return usage();
        default:
            err_printf("[ERROR] Unkown option: %s\n", a);
            return 1;
        }
    }
    return 0;
}


static int Main(int argc, char *argv[])
{
    enum { Ok = 0, Er = 1 };
    static  opts_t opt;
    opts_t* o = &opt;
    int     optCk = 1;
    int     n;
    int     i;

    memset(o, 0, sizeof *o);
    o->srcCP = (mbc_cp_t)0;
    o->dstCP = (mbc_cp_t)0;

    if (argc < 2)
        return usage();

    if (ExArgv_convEx(&argc, &argv, 0) == 0)
        return Er;

    // オプション解析 & オプション引数は開放して詰める.
    for (n = i = 1; i < argc; ++i) {
        char*  a = argv[i];
        if (optCk && *a == '-') {   // オプション解析.
            if (a[1] == '\0') {     // - だけなら標準入力.
            } else if (a[1] == '-' && a[2] == '\0') {
                optCk = 0;
            } else if (opts_get(a, o) != 0) {
                return Er;
            }
            free(a);
            argv[i] = NULL;
        } else {
            argv[n++] = a;
        }
    }
    argv[n] = NULL;
    argc = n;

    if (o->srcdir) {    // 入力ディレクトリ指定があれば相対パスに付加.
        for (i = 1; i < argc; ++i) {
            char*  a = argv[i];
            if (!fname_isAbsolutePath(a)) {
                char* dup = fname_appendDup(o->srcdir, a);
                if (!dup)
                    return Er;
                free(argv[i]);
                argv[i] = dup;
            }
        }
    }

    // ワイルドカード展開.
    if (ExArgv_convEx(&argc, &argv, 3) == 0)
        return Er;

    // ファイルごとの処理.
    for (i = 1; i < argc; ++i) {
        char*  a = argv[i];
        if (oneFile(a, o->outname, o) != 0)
            return Er;

        if (o->outname && o->outname[0])    // 1ファイル出力指定だったら抜ける.
            break;
    }

    ExArgv_Free(&argv);

    return Ok;
}


int main(int argc, char* argv[])
{
    int rc;
 #if defined(_WIN32)
    int savCP = GetConsoleOutputCP();
    setConsoleCodePage(65001);
 #endif
    rc = Main(argc, argv);
 #if defined(_WIN32)
    SetConsoleOutputCP(savCP);
 #endif
    return rc;
}
