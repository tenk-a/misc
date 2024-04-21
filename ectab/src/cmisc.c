/**
 *  @file   cmisc.c
 *  @brief  misc for c
 *  @author Masashi Kitamura (tenka@6809.net)
 *  @license Boost Software Lisence Version 1.0
 */

#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "cmisc.h"
#include "mbc.h"

#if defined(_WIN32)
#include <io.h>
#include <direct.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

char*   fname_base(char const* p)
{
    char const *adr = p;
    while (*p) {
     #if defined(_WIN32) || defined(_DOS)
        unsigned c = *(unsigned char const*)p;
        ++p;
      #if defined(_MBCS) && defined _WIN32
        if (IsDBCSLeadByte(c) && *p) {
            ++p;
            continue;
        }
      #endif
        if (c == ':' || c == '/' || c == '\\')
            adr = p;
     #else
        char c = *p++;
        if (c == ':' || c == '/')
            adr = p;
     #endif
    }
    return (char*)adr;
}

char const* fname_ext(char const* fpath)
{
    char const* p = fname_base(fpath);
    char const* e = strrchr(p, '.');
    return e ? e : "";
}

void str_replace(char str[], char old_c, char new_c)
{
    while (*str) {
        if (*str == old_c)
            *str = new_c;
        ++str;
    }
}


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

//
size_t file_size(const char* fpath)
{
 #if defined(_WIN32) || defined(_DOS)
    struct _stat st;
    int   rc = _stat(fpath, &st);
 #else
    struct stat st;
    int   rc = stat(fpath, &st);
 #endif
    return (rc == 0) ? (size_t)st.st_size : (size_t)-1;
}

int file_exist(char const* fpath)
{
 #if defined(_WIN32) || defined(_DOS)
    return access(fpath, 0) == 0;
 #else
    struct stat st;
    return stat(fpath, &st) == 0;
 #endif
}

size_t file_load(char const* fname, void* dst, size_t bytes, size_t max_bytes)
{
    size_t rbytes;
    if (dst == NULL || bytes == 0)
        return 0;
    if (max_bytes && bytes > max_bytes)
        bytes = max_bytes;

    if (fname) {
     #if defined(_WIN32) || defined(_DOS)
        int fd = _open(fname, _O_RDONLY|_O_BINARY);
        if (fd == -1)
            return 0;
        rbytes = _read(fd, dst, bytes);
        _close(fd);
     #else
        int fd = open(fname, O_RDONLY);
        if (fd == -1)
            return 0;
        rbytes = read(fd, dst, bytes);
        close(fd);
     #endif
    } else {
        rbytes = read(0, dst, bytes);

    }
    return rbytes;
}

void* file_loadMalloc(char const* fname, size_t* pReadSize, size_t max_size)
{
    char*  m;
    size_t bytes, rbytes;
    if (pReadSize)
        *pReadSize = 0;
    bytes = file_size(fname);
    if (bytes == (size_t)(-1))
        return NULL;
    if (max_size && bytes >= max_size)
        bytes = max_size;
    m = (char*)malloc(bytes+1);
    if (m == NULL)
        return NULL;
    m[bytes] = 0;
    rbytes = file_load(fname, m, bytes, max_size);
    if (pReadSize)
        *pReadSize = rbytes;
    return m;
}


#if defined(_WIN32) || defined(_DOS)
 #define FILE_MKDIR(fnm, pmode) _mkdir(fnm)
#else
 #define FILE_MKDIR(fnm, pmode) mkdir((fnm), (pmode))
#endif

static int recursive_mkdir_sub2(char* dir, int pmode)
{
    char const* e  = dir + strlen(dir);
    char*   s;
    do {
        s = fname_base(dir);
        if (s <= dir)
            return -1;
        --s;
        *s = 0;
    } while (FILE_MKDIR(dir, pmode) != 0);
    do {
        *s  = FILE_DIR_SEP;
        s  += strlen(s);
    } while (FILE_MKDIR(dir, pmode) == 0 && s < e);
    return (s >= e) ? 0 : -1;
}

///Extension of mkdir, making directories on the way
int file_recursive_mkdir(char const* dir, int pmode)
{
    char*  buf;
 #if defined(_WIN32) || defined(_DOS)
    struct _stat st;
    int    rc  = _stat(dir, &st);
    int    mod = (rc == 0) ? st.st_mode : -1;
    if (mod != -1)
        return (mod & _S_IFDIR) ? 0 : -1;
 #else
    struct stat st;
    int    rc  = stat(dir, &st);
    int    mod = (rc == 0) ? st.st_mode : -1;
    if (mod != -1)
        return (mod & S_IFDIR) ? 0 : -1;
 #endif
    if (FILE_MKDIR(dir, pmode) == 0)
        return 0;   // ok.

    buf = strdup(dir);
    if (!buf)
        return -1;
    rc = recursive_mkdir_sub2(buf, pmode);
    free(buf);
    return rc;
}

char*   fname_removeDirSep(char path[]) {
    char* p = fname_base(path);
    if (p > path && *p == 0 && FILE_IS_DIR_SEP(p[-1]))
        *--p = 0;
    return path;
}

char*   fname_addDirSep(char* path, size_t capa) {
    char* p = path;
    fname_removeDirSep(p);
    p += strlen(p);
    if (p < path + capa - 1) {
        *p++ = FILE_DIR_SEP;
        *p = 0;
    } else {
        path = NULL;
    }
    return path;
}

//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

/** 文字列末にある空白を削除する.
 *  @param  str 文字列.書き換えられる.
 *  @param  flags bit0=1:最後の'￥ｎ''￥ｒ'は残す                   <br>
 *                bit1=1:C/C++ソース対策で ￥ の直後の' 'は１つ残す.
 */
char *str_trimSpcR(char str[], unsigned flags)
{
    unsigned char *s;
    unsigned char *p;
    size_t n;
    int  cr;
    int  c;

    if (str == NULL)
        return NULL;
    s = (unsigned char *)str;
    n = strlen(str);
    if (n == 0)
        return str;
    p = s + n;

    // 改行文字の状態を設定.
    cr = 0;
    if (flags & 1) {    // 改行を考慮する?
        c = *--p;
        if (c == '\n') {
            if (p-1 >= s && p[-1] == '\r') {
                --p;
                cr = 3;
            } else {
                cr = 1;
            }
            --p;
        } else if (c == '\r') {
            cr = 2;
            --p;
        }
        p++;
        if (p <= s) {
            return str;
        }
    }
    // 行末の空白部分を飛ばしてそうでない部分が現れるまで探す.
    n = 0;
    do {
        c = *--p;
        n++;
    } while (p > s && c && (c <= 0x20 || c == 0x7f));
    if (c > 0x20 && c != 0x7f) {
        --n;
        p++;
    }
    // c/c++を考慮するとき、1文字以上の空白が\ の直後にある状態なら、空白を1文字だけ戻す.
    if ((flags & 2) && n && p > s && p[-1] == '\\') {
        *p++ = ' ';
    }
    // 必要なら改行コードを復元する.
    if (cr) {
        if (cr & 2) {
            *p++ = '\r';
        }
        if (cr & 1) {
            *p++ = '\n';
        }
    }
    *p = '\0';
    return str;
}

#if 1
#define MGETC(mbc, c, pSrc, width) (                                    \
    (width) += mbc_chrWidth((mbc), ((c) = mbc_getChr((mbc), &(pSrc))))  \
)
#define MPUTC(mbc, pDst, dstEnd, c, width) (                            \
    (pDst)   = mbc_strSetChr((mbc), (pDst), (dstEnd), (c)),             \
    (width) += mbc_chrWidth((mbc), (c))                                 \
)
#else
#define MGETC(mbc, c, pSrc, width)          mgetC((mbc), &(c), &(pSrc), &(width))
static inline void mgetC(mbc_enc_t mbc, int* pC, char const** ppSrc, size_t* pWidth)
{
    *pC  = mbc_getChr(mbc, ppSrc);
    *pWidth += mbc_chrWidth(mbc, *pC);
}
#define MPUTC(mbc, pDst, dstEnd, c, width)  mputC((mbc), &(pDst), (dstEnd), (c), &(width))
static inline void mputC(mbc_enc_t mbc, char** ppDst, char* dstEnd, int c, size_t* pWidth)
{
    *ppDst = mbc_strSetChr(mbc, *ppDst, dstEnd, c);
    *pWidth += mbc_chrWidth(mbc, c);
}
#endif

/** src文字列中のtabを空白にして空白の繋がりを新たなtabに変換した文字列をdstに入れる.
 *  @param dst    出力バッファ. NULLのとき出力しない...サイズ計算を行うことになる.
 *  @param flags  bit0=1 空白1文字はtabに変換しない                         <br>
 *                bit1=1 Cの'"ペアを考慮.                                   <br>
 *                bit2=1 Cの￥エスケープを考慮                              <br>
 *                bit3=1 Cの'"情報として前回の結果の続きにする              <br>
 *                bit4=1 タブサイズ丁度のときのみタブに変換する             <br>
 *                bit5=1 4タブ8タブどちらでも見た目が変わらないように変換   <br>
 *                bit6=1 CRのみも改行として扱う                             <br>
 *                bit8=1 行末空白を削除
 *  @param dstSz  出力先サイズ. 0ならサイズチェック無し.
 *  @return       変換後のサイズ.
 */
size_t strConvTab(struct mbc_enc_st const* mbc, char *dst, size_t dstSz
                , const char *src, int dstTabSz, int srcTabSz, unsigned flags)
{
    enum {
        F_SP1NTB = 0x01, F_CPAIR = 0x02, F_CESC = 0x04, F_CPAIRCONT = 0x08,
        F_AJSTAB = 0x10, F_BOTH  = 0x20, F_CR   = 0x40,
        F_TRIMR  = 0x80,
    };
    const char* s  = (const char *)src;
    char*       d  = (char*)dst;
    char*       e  = (char*)dst + dstSz;
    int         jstab= (flags & F_AJSTAB) ? dstTabSz-1 : (flags & F_SP1NTB) ? 1 : 0;
    int         tsn  = -1;
    size_t      sn   = 0;
    size_t      dn   = 0;
    int         k;
    int         c;
    int         c2;
    int         n;
    int         bsn;
    static int  cpairChr;
    static int  cmtMd;

    // '"の続きをするフラグがたっていない場合は初期化.
    if ((flags & F_CPAIRCONT) == 0) {
        cpairChr = 0;
        cmtMd    = 0;
    }
    k = cpairChr;

    // src がNULLなら、たぶん、cpairChrの初期化だ.
    if (src == NULL)
        return 0;

    // サイズが0なら、チェックなしとして、終了を目一杯大きいアドレスにする.
    if (dstSz == 0)
        e = (char*)(-1);

    // 出力先がNULLなら、バイト数のカウントのみにするため、終了アドレスもNULL.
    if (dst == NULL)
        e = NULL;

    // 元tabサイズが0(以下)なら、割り算で破綻しないようにとりあえず1にしとく.
    if (srcTabSz <= 0)
        srcTabSz = 1;

    // 4tab,8tab両用にするならとりあえず4tab扱い.
    if (flags & F_BOTH)
        dstTabSz = 4;

    // 文字列が終わるまでループ.
    while (*s) {
        // 1文字取得.
        bsn = sn;
        MGETC(mbc, c, s, sn);
        if (c == ' ' && k == 0) {           // 空白なら、とりあえずカウント.
            if (tsn < 0)
                tsn = bsn;
        } else if (c == '\t' && k == 0) {   //
            if (tsn < 0)
                tsn = bsn;
            sn = ((bsn+srcTabSz) / srcTabSz) * srcTabSz;
        } else {
            if (tsn >= 0) {         // 空白があった.
                n = bsn - tsn;      // c は必ず 1以上の値.
                if (dstTabSz <= 0) {
                    //空白への変換.
                    do {
                        d = mbc_strSetChr(mbc, d, e, ' ');
                    } while (--n);
                } else if (flags & F_BOTH) {
                    // 4tab,8tab両用変換.
                    int m  = dn/dstTabSz;
                    int tn = (m + 1) * dstTabSz;
                    int l  = tn - dn;
                    dn += n;
                    if (dn >= tn) {
                        if ((l <= jstab && jstab) || (m&1) == 0) {
                            do {
                                d = mbc_strSetChr(mbc, d, e, ' ');
                            } while (--l);
                        } else {
                            d = mbc_strSetChr(mbc, d, e, '\t');
                        }
                        while (dn >= (tn += dstTabSz)) {
                            ++m;
                            if (m & 1) {
                                d = mbc_strSetChr(mbc, d, e, '\t');
                            } else {
                                for (l = 4; --l >= 0;)
                                    d = mbc_strSetChr(mbc, d, e, ' ');
                            }
                        }
                        tn -= dstTabSz;
                        if (dn > tn) {
                            n = dn - tn;
                            do {
                                d = mbc_strSetChr(mbc, d, e, ' ');
                            } while (--n);
                        }
                    } else {
                        do {
                            d = mbc_strSetChr(mbc, d, e, ' ');
                        } while (--n);
                    }
                } else {
                    // 通常のタブ変換.
                    int tn = ((dn / dstTabSz) + 1) * dstTabSz;
                    int l  = tn - dn;
                    dn += n;
                    if (dn >= tn) {
                        if (l <= jstab && jstab) {
                            // フラグ指定によりtabが空白一個、またはタブサイズに満たない場合,
                            // 空白にする指定があったら空白.
                            do {
                                d = mbc_strSetChr(mbc, d, e, ' ');
                            } while (--l);
                        } else {
                            d = mbc_strSetChr(mbc, d, e, '\t');
                        }
                        while (dn >= (tn += dstTabSz)) {
                            d = mbc_strSetChr(mbc, d, e, '\t');
                        }
                        tn -= dstTabSz;
                        if (dn > tn) {
                            n = dn - tn;
                            do {
                                d = mbc_strSetChr(mbc, d, e, ' ');
                            } while (--n);
                        }
                    } else {
                        do {
                            d = mbc_strSetChr(mbc, d, e, ' ');
                        } while (--n);
                    }
                }
                tsn = -1;
            }

            MPUTC(mbc, d, e, c, dn);

            if (flags & (F_CPAIR|F_CESC)) {     // C/C++の " ' を考慮するとき.
                if (c == '\\' && *s && k != '`' && cmtMd == 0) {
                    MGETC(mbc, c2, s, sn);
                    MPUTC(mbc, d, e, c2, dn);
                } else if (c == '"' || c == '\'' || c == '`') { // " ' のチェック.
                    if (cmtMd == 0) {
                        if (k == 0)
                            k = c;
                        else if (k == c)
                            k = 0;
                    }
                } else if (c == '/' && (*s == '/' || *s == '*') && k == 0 && cmtMd == 0) { // // /* のとき.
                    cmtMd = *s;
                    MGETC(mbc, c2, s, sn);
                    MPUTC(mbc, d, e, c2, dn);
                } else if (c == '*' && *s == '/' && k == 0 && cmtMd == '*') { // */のとき.
                    cmtMd = 0;
                    MGETC(mbc, c2, s, sn);
                    MPUTC(mbc, d, e, c2, dn);
                } else if ((flags & F_CESC) && (unsigned)c < 0x20 && (k || (flags & F_CPAIR) == 0)) {
                    static char const xdit[0x10] = {
                        '0','1','2','3','4','5','6','7',
                        '8','9','a','b','c','d','e','f'
                    };
                    static char const escc[0x20] = {    // a:0x07,b:0x08,t:0x09,n:0x0a,v:0x0b,f:0x0c,r:0x0d
                        0  , 0  , 0  , 0  , 0  , 0  , 0  , 'a',
                        'b', 't', 'n', 'v', 'f', 'r', 0  , 0  ,
                    };
                    --d;
                    d = mbc_strSetChr(mbc, d, e, '\\');
                    c2 = escc[c];
                    if (c2) {
                        d = mbc_strSetChr(mbc, d, e, c2);
                        ++dn;
                    } else {
                        d = mbc_strSetChr(mbc, d, e, 'x');
                        d = mbc_strSetChr(mbc, d, e, xdit[c>>4]);
                        d = mbc_strSetChr(mbc, d, e, xdit[c&15]);
                        dn+=3;
                    }
                }
            }
            if (c == '\n') {
                sn = dn = 0;
                if (cmtMd == '/')
                    cmtMd = 0;
            } else if (c == '\r') {
                if (*s == '\n') {
                    s++;
                    d = mbc_strSetChr(mbc, d, e, '\n');
                    sn = dn = 0;
                } if (flags & F_CR) {
                    sn = dn = 0;
                }
                if (cmtMd == '/')
                    cmtMd = 0;
            }
        }
    }
    cpairChr = k;
    if (d < e)
        *d = '\0';
    else if (dst && dstSz > 0)
        dst[dstSz-1] = '\0';

    // 文字列末の空白削除指定があって、"'中でないなら、実行.
    if (dst && (flags & F_TRIMR) && ((k == 0) || (flags & F_CPAIR))) {
        unsigned cf = 1;
        cf |= ((flags & F_CPAIR) != 0) << 1;
        str_trimSpcR(dst, cf);
        return strlen(dst);
    }
    return (char*)d - dst;
}
