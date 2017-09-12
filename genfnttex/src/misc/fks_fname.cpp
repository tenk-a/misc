/**
 *  @file   fks_fname.cpp
 *  @brief  ファイル名の処理.
 *  @author Masashi Kitamura
 *  @note
 *  -   c/c++ 用.
 *  -   ara_lib に入れるに際し、拡張子を .cpp にしているが 内容は .cの範囲.
 *  -   win / linux(unix) 用.
 *      winでは\ /が、以外は/がセパレータ.
 *      一応dos判定しているが、int=2byteを考慮していないので注意.
 *  -   文字の0x80未満はascii系であること前提.
 *  -   SJIS等のマルチバイト文字対応.
 *      - Win環境ではマルチバイト文字はCharNextでポインタを進めている.
 *        (CharNextがUTF8対応してもいいように.最もwin自体が現状MBCはDBCのみ
 *        でutf8対応していない)
 *      - Win以外の環境では、環境変数LANGをみて, SJIS,BIG5,GBKを考慮.
 *        (おそらく全角2バイト目に0x5cがあるのは、これくらい?と想定)
 *      - 比較関係は、ロケール対応不十分につき誤判定ありえる?
 *  -   pathの最大長を考慮して、size_t でなく unsigned でサイズ指定.
 */

#include "fks_fname.h"

#ifdef __cplusplus
#include <cstdlib>
#include <cstring>
#else
#include <stdlib.h>
#include <string.h>
#endif
#ifndef assert
#include <assert.h>
#endif

#ifndef FKS_LIB_DECL
#define FKS_LIB_DECL
#endif

#undef FKS_STD
#define FKS_STD
#undef FKS_W32_NS
#define FKS_W32_NS

// os の違い関係.
#if defined _WIN32
 //#include "../win/ara_w32.h"
 #include <windows.h>
 #include <malloc.h>
 #if defined _MSC_VER
  #pragma comment(lib, "User32.lib")            // CharNext()で必要...
 #endif
#else   // linux
 #include <alloca.h>
 // #include <dirent.h>
 // #include <sys/stat.h>
#endif

// fullpath 化でallocaを使わない場合定義.
//#define FKS_USE_FNAME_ALLOCA

// unicode対応. ※ mb系を使われたくないため tchar.h を使わず自前で対処.
#ifdef FKS_FNAME_WCS_COMPILE    // wchar_t 対応.
 #define FKS_FNAME_C(x)             L##x
 #define FKS_FNAME_CHAR             wchar_t
 #define FKS_FNAME_R_STR(s,c)       FKS_STD wcsrchr((s),(c))
 #if defined FKS_FNAME_WINDOS       // 大小文字同一視.
  #define FKS_FNAME_CMP(l,r)        _wcsicmp((l),(r))
  #define FKS_FNAME_N_CMP(l,r,n)    _wcsnicmp((l),(r),(n))
 #else                              // 大小区別.
  #define FKS_FNAME_CMP(l,r)        FKS_STD wcscmp((l),(r))
  #define FKS_FNAME_N_CMP(l,r,n)    FKS_STD wcsncmp((l),(r),(n))
 #endif
  #define FKS_FNAME_STRTOL(s,t,r)   FKS_STD wcstol((s),(t),(r))
#else           // char ベース.
 #define FKS_FNAME_C(x)             x
 #define FKS_FNAME_CHAR             char
 #define FKS_FNAME_R_STR(s,c)       FKS_STD strrchr((s),(c))
 #if defined FKS_FNAME_WINDOS       // 大小文字同一視.
  #define FKS_FNAME_CMP(l,r)        _stricmp((l),(r))
  #define FKS_FNAME_N_CMP(l,r,n)    _strnicmp((l),(r),(n))
 #else                              // 大小区別.
  #define FKS_FNAME_CMP(l,r)        FKS_STD strcmp((l),(r))
  #define FKS_FNAME_N_CMP(l,r,n)    FKS_STD strncmp((l),(r),(n))
 #endif
  #define FKS_FNAME_STRTOL(s,t,r)   FKS_STD strtol((s),(t),(r))
#endif
#define FKS_FNAME_IS_DIGIT(c)       (('0') <= (c) && (c) <= ('9'))

// c/c++ 対策.
#ifdef __cplusplus                  // c++の場合、ポインタ操作のみの関数はconst版,非const版をつくる.
 #define FKS_FNAME_const_CHAR       FKS_FNAME_CHAR      // そのため、基本は、非const関数にする.
#else                               // cの場合は標準ライブラリにあわせ 引数constで戻り値 非const にする.
 #define FKS_FNAME_const_CHAR       const FKS_FNAME_CHAR
#endif


// ============================================================================
// マルチバイト文字の0x5c 対策関係.

/// 文字 C が MS全角の１バイト目か否か. (utf8やeucは \ 問題は無いので 0が帰ればok)
#if defined FKS_FNAME_WCS_COMPILE
 #define FKS_FNAME_ISMBBLEAD(c)     (0)
#elif defined _WIN32
 #define FKS_FNAME_ISMBBLEAD(c)     FKS_W32_NS IsDBCSLeadByte(c)
//#elif defined HAVE_MBCTYPE_H
// #define FKS_FNAME_ISMBBLEAD(c)   _ismbblead(c)
#elif defined FKS_USE_FNAME_MBC
 #define FKS_FNAME_ISMBBLEAD(c)     ((unsigned)(c) >= 0x80 && fks_fnameIsZenkaku1((c)))
#else
 #define FKS_FNAME_ISMBBLEAD(c)     (0)
#endif

/// 次の文字へポインタを進める. ※CharNext()がサロゲートペアやutf8対応してくれてたらいいなと期待(駄目かもだけど)
#if  defined _WIN32
 #ifdef FKS_FNAME_WCS_COMPILE
  #define FKS_FNAME_CHARNEXT(p)     (FKS_FNAME_CHAR*)FKS_W32_NS CharNextW((FKS_FNAME_CHAR*)(p))
 #else
  #define FKS_FNAME_CHARNEXT(p)     (FKS_FNAME_CHAR*)FKS_W32_NS CharNextA((FKS_FNAME_CHAR*)(p))
 #endif
#else
 #if defined FKS_FNAME_WCS_COMPILE || !defined(FKS_USE_FNAME_MBC)
  #define FKS_FNAME_CHARNEXT(p)     ((p) + 1)
 #else
  #define FKS_FNAME_CHARNEXT(p)     ((p) + 1 + (FKS_FNAME_ISMBBLEAD(*(unsigned char*)(p)) && (p)[1]))
 #endif
#endif


// ----------------------------------
// win 以外で、sjis対応する場合の処理.

#if defined(FKS_USE_FNAME_MBC) && !defined(_WIN32) && !defined(FKS_FNAME_WCS_COMPILE)   //&& !defined(HAVE_MBCTYPE_H)   // win32以外で, LANG対応する場合.

#ifdef __cplusplus
extern "C" {
#endif

/** とりあえず、0x5c関係の対処用.
 */
static FKS_LIB_DECL int fks_fnameMbcInit(void)
{
    const char*         lang = FKS_STD getenv("LANG");
    const char*         p;
    if (lang == 0)
        return 0;
    //s_fname_locale_ctype = strdup(lang);
    // ja_JP.SJIS のような形式であることを前提にSJIS,big5,gbkかをチェック.
    p = FKS_STD strrchr(lang, '.');
    if (p) {
        const char* enc = p + 1;
        // 0x5c対策が必要なencodingかをチェック.
        if (strncasecmp(enc, "sjis", 4) == 0) {
            return 1;
        } else if (strncasecmp(enc, "gbk", 3) == 0 || strncasecmp(enc, "gb2312", 6) == 0) {
            return 2;
        } else if (strncasecmp(enc, "big5", 4) == 0) {
            return 3;
        }
    }
    return 0;
}

static FKS_LIB_DECL int fks_fnameIsZenkaku1(unsigned c) {
    static int    s_fks_fnameShift_char_mode = -1;
    if (s_fks_fnameShift_char_mode < 0)
        s_fks_fnameShift_char_mode = fks_fnameMbcInit();
    switch (s_fks_fnameShift_char_mode) {
    case 1 /* sjis */: return ((c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC));
    case 2 /* GBK  */: return  (c >= 0x81 && c <= 0xFE);
    case 3 /* BIG5 */: return ((c >= 0xA1 && c <= 0xC6) || (c >= 0xC9 && c <= 0xF9));
    default:           return 0;
    }
}

#ifdef __cplusplus
}
#endif

#endif  // !defined(_WIN32) && !defined(FKS_FNAME_WCS_COMPILE)



// ############################################################################

// ============================================================================

#define FKS_FNAME_TO_UPPER(c)       (((c) >= FKS_FNAME_C('a') && (c) <= FKS_FNAME_C('z')) ? (c) - FKS_FNAME_C('a') + FKS_FNAME_C('A') : (c))
#define FKS_FNAME_TO_LOWER(c)       (((c) >= FKS_FNAME_C('A') && (c) <= FKS_FNAME_C('Z')) ? (c) - FKS_FNAME_C('A') + FKS_FNAME_C('a') : (c))


#if defined __cplusplus && !defined FKS_FNAME_WCS_COMPILE
extern "C" {
#endif

/** 絶対パスか否か(ドライブ名の有無は関係なし)
 */
FKS_LIB_DECL int    fks_fnameIsAbs(const FKS_FNAME_CHAR* path)
{
    if (path == 0)
        return 0;
    path = fks_fnameSkipDrive(path);
    return fks_fnameIsSep(path[0]);
}


/** ドライブ名がついているか.
 */
FKS_LIB_DECL int        fks_fnameHasDrive(const FKS_FNAME_CHAR* path)
{
  #if 1 // 先頭の"文字列:"をドライブ名とみなす.
    const FKS_FNAME_CHAR* s = path;
    if (s == 0)
        return 0;
    if (*s && *s != FKS_FNAME_C(':')) {
        while (*s && !fks_fnameIsSep(*s)) {
            if (*s == FKS_FNAME_C(':')) {
                return 1;
            }
            ++s;
        }
    }
    return 0;
  #else // 一字ドライブ名のみ対応する場合.
   #if defined FKS_FNAME_WINDOS
    if (path == 0)
        return 0;
    return (path[0] && path[1] == FKS_FNAME_C(':'));
   #else
    return 0;
   #endif
  #endif
}


// ============================================================================

/** sizeに収まる文字列の文字数を返す. \0を含まない.
 *  (win環境ではなるべくマルチバイト文字の途中で終わらないようにする.
 *   けど、用途的には切れる以上あまり意味ない...)
 */
FKS_LIB_DECL unsigned   fks_fnameAdjustSize(const FKS_FNAME_CHAR* str, unsigned size)
{
    const FKS_FNAME_CHAR* s = str;
    const FKS_FNAME_CHAR* b = s;
    const FKS_FNAME_CHAR* e = s + size;
    assert(str != 0 && size > 0);
    while (s < e) {
        if (*s == 0)
            return s - str;
        b = s;
        s = FKS_FNAME_CHARNEXT((FKS_FNAME_CHAR*)s);
    }
    return b - str;
}


/** ファイル名のコピー. mbcの時は文字が壊れない部分まで. dst == src もok.
 */
FKS_LIB_DECL FKS_FNAME_CHAR*    fks_fnameCpy(FKS_FNAME_CHAR dst[], unsigned dstSz, const FKS_FNAME_CHAR* src)
{
    unsigned    l;
    assert(dst != NULL && dstSz > 0);

    if (src == NULL)
        return NULL;
    l = fks_fnameAdjustSize(src, dstSz);

    // アドレスが同じなら、長さをあわせるのみ.
    if (dst == src) {
        dst[l] = 0;
        return dst;
    }

    // コピー.
    {
        const FKS_FNAME_CHAR*   s = src;
        const FKS_FNAME_CHAR*   e = s + l;
        FKS_FNAME_CHAR*         d = dst;
        while (s < e)
            *d++ = *s++;
        *d = 0;
    }

    return dst;
}


/** ファイル名文字列の連結.
 */
FKS_LIB_DECL FKS_FNAME_CHAR*    fks_fnameCat(FKS_FNAME_CHAR dst[], unsigned dstSz, const FKS_FNAME_CHAR* src)
{
    unsigned l;
    assert(dst != NULL && dstSz > 0);
    if (src == 0)
        return NULL;
    assert(src != 0 && dst != src);
    l = fks_fnameLen(dst);
    if (l >= dstSz) {   // そもそも転送先が満杯ならサイズ調整のみ.
        return fks_fnameCpy(dst, dstSz, dst);
    }
    fks_fnameCpy(dst+l, dstSz - l, src);
    return dst;
}


// ============================================================================

/** ドライブ名があればそれをスキップしたポインタを返す.
 *   ※ c:等だけでなくhttp:もスキップするため "文字列:" をスキップ.
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameSkipDrive(FKS_FNAME_const_CHAR* path)
{
  #if 1
    const FKS_FNAME_CHAR* s = path;
    if (*s && *s != FKS_FNAME_C(':')) {
        while (*s && !fks_fnameIsSep(*s)) {
            if (*s == FKS_FNAME_C(':')) {
                return (FKS_FNAME_CHAR*)s+1;
            }
            ++s;
        }
    }
 #else  // 1字ドライブ名のみの対応ならこちら.
  #if defined FKS_FNAME_WINDOS
    if (path[0] && path[1] == FKS_FNAME_C(':'))     // ドライブ名付きだった
        return (FKS_FNAME_CHAR*) path + 2;
    #endif
 #endif
    return (FKS_FNAME_CHAR*)path;
}


/** 文字列先頭のドライブ名,ルート指定をスキップしたポインタを返す.
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameSkipDriveRoot(FKS_FNAME_const_CHAR* path)
{
    FKS_FNAME_CHAR* p = fks_fnameSkipDrive(path);
    while (fks_fnameIsSep(*p))
        ++p;
    return p;
}


/** ファイルパス名中のディレクトリを除いたファイル名の位置を返す.
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameBaseName(FKS_FNAME_const_CHAR *adr)
{
    const FKS_FNAME_CHAR *p = adr;
    assert(adr != 0);
    while (*p) {
        if (*p == FKS_FNAME_C(':') || fks_fnameIsSep(*p))
            adr = (FKS_FNAME_CHAR*)p + 1;
        p = FKS_FNAME_CHARNEXT(p);
    }
    return (FKS_FNAME_CHAR*)adr;
}


/** 拡張子の位置を返す. '.'は含む. なければ文字列の最後を返す.
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameExt(FKS_FNAME_const_CHAR *name)
{
    const FKS_FNAME_CHAR *p;
    assert(name != 0);
    name = fks_fnameBaseName(name);
    p    = FKS_FNAME_R_STR(name, FKS_FNAME_C('.'));
    if (p) {
        return (FKS_FNAME_CHAR*)(p);
    }
    return (FKS_FNAME_CHAR*)name + fks_fnameLen(name);
}


// ============================================================================

/** ファイルパス名中の拡張子を削除する.
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameDelExt(FKS_FNAME_CHAR buf[])
{
    FKS_FNAME_CHAR *t;
    FKS_FNAME_CHAR *p;
    assert(buf != 0);
    t = fks_fnameBaseName(buf);
    p = FKS_FNAME_R_STR(t, FKS_FNAME_C('.'));
    if (p == 0) {
        p = t + fks_fnameLen(t);
    }
    *p = 0;
    return buf;
}


/** ファイルパス名中の拡張子を除いた部分の取得.
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameGetNoExt(FKS_FNAME_CHAR dst[], unsigned size, const FKS_FNAME_CHAR *src)
{
    const FKS_FNAME_CHAR *s;
    const FKS_FNAME_CHAR *e;
    unsigned         l = 0;
    assert(dst != 0 && size > 1 && src != 0);
    //if (dst == 0 || size == 0 || src == 0) return 0;
    s = fks_fnameBaseName(src);
    e = FKS_FNAME_R_STR(s, FKS_FNAME_C('.'));
    if (e == 0) {
        e = s + fks_fnameLen(s);
    }
    l = e - src + 1;
    if (l > size)
        l = size;
    fks_fnameCpy(dst, l, src);
    return dst;
}


/** ファイルパス名中のディレクトリと拡張子を除いたファイル名の取得.
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameGetBaseNameNoExt(FKS_FNAME_CHAR dst[], unsigned size, const FKS_FNAME_CHAR *src)
{
    const FKS_FNAME_CHAR *s;
    const FKS_FNAME_CHAR *e;
    unsigned         l = 0;
    assert(dst != 0 && size > 1 && src != 0);
    //if (dst == 0 || size == 0 || src == 0) return 0;
    s = fks_fnameBaseName(src);
    e = FKS_FNAME_R_STR(s, FKS_FNAME_C('.'));
    if (e == 0) {
        e = s + fks_fnameLen(s);
    }
    l = e - s + 1;
    if (l > size)
        l = size;
    fks_fnameCpy(dst, l, s);
    return dst;
}


/** 拡張子を、ext に変更する. dst == src でもよい.
 *  ext = NULL or "" のときは 拡張子削除.
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameSetExt(FKS_FNAME_CHAR dst[], unsigned size, const FKS_FNAME_CHAR* src, const FKS_FNAME_CHAR *ext)
{
    assert(dst != 0 && size > 0 && src != 0);
    fks_fnameGetNoExt(dst, size, src);
    if (ext && ext[0]) {
        if (ext[0] != FKS_FNAME_C('.'))
            fks_fnameCat(dst, size, FKS_FNAME_C("."));
        fks_fnameCat(dst, size, ext);
    }
    return dst;
}


/** 拡張子がない場合、拡張子を追加する.(あれば何もしない). dst == src でもよい.
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameSetDefaultExt(FKS_FNAME_CHAR dst[], unsigned size, const FKS_FNAME_CHAR* src, const FKS_FNAME_CHAR *ext)
{
    FKS_FNAME_CHAR* p;
    assert(dst != 0 && size > 0 && src != 0);

    fks_fnameCpy(dst, size, src);
    if (ext == 0)
        return dst;
    p = fks_fnameBaseName(dst);
    p = FKS_FNAME_R_STR(p, FKS_FNAME_C('.'));
    if (p) {
        if (p[1])
            return dst;
        *p = 0;
    }
    if (ext[0]) {
        if (ext[0] != FKS_FNAME_C('.'))
            fks_fnameCat(dst, size, FKS_FNAME_C("."));
        fks_fnameCat(dst, size, ext);
    }
    return dst;
}


// ============================================================================

/** ドライブ名部分を取得. :つき. ※ file:等の対処のため"文字列:"をドライブ扱い
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameGetDrive(FKS_FNAME_CHAR drive[], unsigned size, const FKS_FNAME_CHAR *name)
{
    const FKS_FNAME_CHAR*   s;
    unsigned                l;
    assert(drive && size > 0 && name);
    drive[0] = 0;
    s = fks_fnameSkipDrive(name);
    l = s - name;
    if (l > 0) {
        ++l;
        if (l > size)
            l = size;
        fks_fnameCpy(drive, l, name);
    }
    return drive;
}


/** ドライブ名とルート指定部分を取得.
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameGetDriveRoot(FKS_FNAME_CHAR dr[], unsigned size, const FKS_FNAME_CHAR *name)
{
    const FKS_FNAME_CHAR*   s;
    unsigned                l;
    assert(dr && size > 0 && name);
    dr[0] = 0;
    s = fks_fnameSkipDriveRoot(name);
    l = s - name;
    if (l > 0) {
        ++l;
        if (l > size)
            l = size;
        fks_fnameCpy(dr, l, name);
    }
    return dr;
}


/** ディレクトリ部分を返す. 最後のディレクトリセパレータは外す.
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameGetDir(FKS_FNAME_CHAR dir[], unsigned size, const FKS_FNAME_CHAR *name)
{
    const FKS_FNAME_CHAR*   p;
    size_t                  l;

    assert(dir  != 0 && size > 0 && name != 0);

    p = fks_fnameBaseName(name);
    l = p - name + 1;
    if (l > size) {
        l = size;
    }
    if (l && dir != name)
        fks_fnameCpy(dir, l, name);
    dir[l] = 0;
    if (l > 0)
        fks_fnameDelLastSep(dir);
    return dir;
}



// ============================================================================

/** 文字列の最後に \ か / があればその位置を返し、なければNULLを返す.
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameCheckPosSep(FKS_FNAME_const_CHAR* dir, int ofs)
{
    assert(dir != 0);
    if (dir) {
        const FKS_FNAME_CHAR*   s   = dir;
        if (ofs >= 0) {
            const FKS_FNAME_CHAR* p = s + ofs;
            if (*p == FKS_FNAME_C('/')) {
                return (FKS_FNAME_CHAR *)p;
            }
          #if (defined FKS_FNAME_WINDOS)
            else if (*p == FKS_FNAME_C('\\')) {
              #ifdef FKS_FNAME_WCS_COMPILE
                return (FKS_FNAME_CHAR *)p;
              #else     // adjust_sizeの結果がofs未満になってたら*pはマルチバイト文字の一部.
                if (fks_fnameAdjustSize(s, ofs+1) == ofs)
                    return (FKS_FNAME_CHAR *)p;
              #endif
            }
          #endif
        }
    }
    return NULL;
}


/** 文字列の最後に \ か / があればその位置を返し、なければNULLを返す.
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameCheckLastSep(FKS_FNAME_const_CHAR* dir)
{
    unsigned l = fks_fnameLen(dir);
    if (l == 0) return 0;
    return fks_fnameCheckPosSep(dir, l - 1);
}


/** 文字列の最後に \ か / があれば削除
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameDelLastSep(FKS_FNAME_CHAR dir[])
{
    FKS_FNAME_CHAR* p = fks_fnameCheckLastSep(dir);
    if (p) {
        *p = 0;
    }
    return dir;
}


/** 文字列の最後に ディレクトリセパレータ文字がなければ追加.
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameAddSep(FKS_FNAME_CHAR dir[], unsigned size)
{
    FKS_FNAME_CHAR* e = dir + size;
    FKS_FNAME_CHAR* p = fks_fnameCheckLastSep(dir);
    if (p == 0) {
        p = dir + fks_fnameLen(dir);
        if (p+1 < e) {
            *p++ = FKS_FNAME_SEP_CHR;
            *p = 0;
        }
    }
    return dir;
}


/** ディレクトリ名とファイル名をくっつける. fks_fnameCat と違い、\ / を間に付加.
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameJoin(FKS_FNAME_CHAR buf[], unsigned bufSz, const FKS_FNAME_CHAR *dir, const FKS_FNAME_CHAR *name)
{
    fks_fnameCpy(buf, bufSz, dir);
    if (buf[0])
        fks_fnameAddSep(buf, bufSz);
    fks_fnameCat(buf, bufSz, name);
    return buf;
}


// ============================================================================

/** 全角２バイト目を考慮した strupr
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameToUpper(FKS_FNAME_CHAR name[])
{
    FKS_FNAME_CHAR *p = name;
    assert(name != NULL);
    while (*p) {
        unsigned c = *p;
        *p = FKS_FNAME_TO_UPPER(c);
        p  = FKS_FNAME_CHARNEXT(p);
    }
    return name;
}


/** 全角２バイト目を考慮した strlwr
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameToLower(FKS_FNAME_CHAR name[])
{
    FKS_FNAME_CHAR *p = name;
    assert(name != NULL);

    while (*p) {
        unsigned c = *p;
        *p = FKS_FNAME_TO_LOWER(c);
        p  = FKS_FNAME_CHARNEXT(p);
    }
    return name;
}


/** filePath中の \ を / に置換.
 */
FKS_LIB_DECL FKS_FNAME_CHAR     *fks_fnameBackslashToSlash(FKS_FNAME_CHAR filePath[])
{
    FKS_FNAME_CHAR *p = filePath;
    assert(filePath != NULL);
    while (*p != FKS_FNAME_C('\0')) {
        if (*p == FKS_FNAME_C('\\')) {
            *p = FKS_FNAME_C('/');
        }
        p = FKS_FNAME_CHARNEXT(p);
    }
    return filePath;
}


/** filePath中の / を \ に置換.
 */
FKS_LIB_DECL FKS_FNAME_CHAR     *fks_fnameSlashToBackslash(FKS_FNAME_CHAR filePath[])
{
    FKS_FNAME_CHAR *p;
    assert(filePath != NULL);
    for (p = filePath; *p; ++p) {
        if (*p == FKS_FNAME_C('/')) {
            *p = FKS_FNAME_C('\\');
        }
    }
    return filePath;
}


// ============================================================================

FKS_LIB_DECL FKS_FNAME_CHAR*    fks_fnameFullpath(FKS_FNAME_CHAR dst[], unsigned size, const FKS_FNAME_CHAR* path, const FKS_FNAME_CHAR* currentDir) {
  #if defined FKS_FNAME_WINDOS
    return fks_fnameFullpathBS(dst, size, path, currentDir);
  #else
    return fks_fnameFullpathSL(dst, size, path, currentDir);
  #endif
}


/** フルパス生成. ディレクトリセパレータを\\にして返すバージョン.
 */
FKS_LIB_DECL FKS_FNAME_CHAR*    fks_fnameFullpathBS(FKS_FNAME_CHAR dst[], unsigned size, const FKS_FNAME_CHAR* path, const FKS_FNAME_CHAR* currentDir)
{
    fks_fnameFullpathSL(dst, size, path, currentDir);
    fks_fnameSlashToBackslash(dst);
    return dst;
}


/** フルパス生成. 文字列操作のみ. カレントパスは引数で渡す.
 *  currentDir は絶対パスであること. そうでない場合の挙動は不定.
 *  '\'文字対策で、セパレータは'/'に置き換ている.
 */
FKS_LIB_DECL FKS_FNAME_CHAR*    fks_fnameFullpathSL(FKS_FNAME_CHAR dst[], unsigned size, const FKS_FNAME_CHAR* path, const FKS_FNAME_CHAR* currentDir)
{
    FKS_FNAME_CHAR*     wk;
    unsigned            wkSz;

    assert(dst != 0 && size > 2 && path != 0);
    if (dst == 0 || size <= 2 || path == 0)
        return 0;
    if (currentDir == 0)
        currentDir = FKS_FNAME_C("/");  // DIRSEP_STR;
    assert(fks_fnameIsAbs(currentDir));

  #if !(defined FKS_USE_FNAME_ALLOCA)
    // 書き込み先サイズが十分でなければ作業用メモリを確保.
    {
        unsigned pl = fks_fnameLen(path);
        unsigned cl = fks_fnameLen(currentDir);
        wkSz = pl + cl + 4;
        if (wkSz >= size) {     // dstサイズよりも、元が多いならワークを用意.
            assert( wkSz <= FKS_FNAME_MAX_URL * sizeof(FKS_FNAME_CHAR) );
            wk = (FKS_FNAME_CHAR*)alloca(wkSz*sizeof(FKS_FNAME_CHAR));
            if (wk == 0) {
                wk   = dst;
                wkSz = size;
            }
        } else {
            wkSz = size;
            wk   = dst;
        }
    }
  #else // allocaを全く使わない場合は出力先を直接使うだけ.
    wkSz = size;
    wk   = dst;
  #endif

    // 作業用の絶対パスを作る.
    {
        unsigned hasDrive = fks_fnameHasDrive(path);
        unsigned isAbs    = fks_fnameIsAbs(path);
        wk[0] = 0;
        if (isAbs && hasDrive) {    // ドライブ付き絶対パスなら、そのまま.
            fks_fnameCpy(wk, wkSz, path);
        } else if (isAbs) {
            if (fks_fnameHasDrive(currentDir)) {   // 絶対パスだけどドライブがない場合はcurrentDirからドライブだけいただく.
                fks_fnameGetDrive(wk, wkSz, currentDir);
            }
            fks_fnameCat(wk, wkSz, path);
        } else {
            if (hasDrive) {         // ドライブ付き相対パスで、
                if (!fks_fnameHasDrive(currentDir))    // カレント側にドライブがなければ
                    fks_fnameGetDrive(wk, wkSz, path); // pathのドライブ名を設定. ちがえばカレント側のドライブ名になる.
            }
            fks_fnameCat(wk, wkSz, currentDir);
            fks_fnameAddSep(wk, wkSz);
            fks_fnameCat(wk, wkSz, fks_fnameSkipDrive(path));
        }
    }

  #if defined FKS_FNAME_WINDOS
    // 処理を簡単にするため、パスの区切りを一旦 / に変換.
    fks_fnameBackslashToSlash(wk);
  #endif

    // "." や ".." を取り除く.
    {
        // この時点でwkは必ず絶対パスになっている.(currentDirが違反してた場合の挙動は不定扱い).
        FKS_FNAME_CHAR*     s     = fks_fnameSkipDrive(wk);    // ドライブ名は弄らないのでスキップ.
        FKS_FNAME_CHAR*     d     = s;
        FKS_FNAME_CHAR*     top   = d;
        unsigned            first = 1;
        while (*s) {
            unsigned c = *s++;
            if (c == FKS_FNAME_C('/')) {
                if (first) {    // 初回の / は "//" "///" を許す... あとで*d++=cするのでここでは2回まで.
                    unsigned i;
                    for (i = 0; i < 2 && *s == FKS_FNAME_C('/'); ++i) {
                        *d++ = *s++;
                    }
                }
                first = 0;
                // '/'の連続は一つの'/'扱い.
              RETRY:
                while (*s == FKS_FNAME_C('/'))
                    ++s;
                if (*s == FKS_FNAME_C('.')) {
                    if (s[1] == 0) {                    // "." のみは無視.
                        s += 1;
                        goto RETRY;
                    } else if (s[1] == FKS_FNAME_C('/')) {      // "./" は無視.
                        s += 2;
                        goto RETRY;
                    } else if (s[1] == FKS_FNAME_C('.') && (s[2] == FKS_FNAME_C('/') || s[2] == 0)) {   // "../" ".." のとき.
                        s += 2 + (s[2] != 0);
                        while (d > top && *--d != FKS_FNAME_C('/')) {   // 出力先のディレクトリ階層を１つ減らす.
                            ;
                        }
                        goto RETRY;
                    }
                }
            }
            *d++ = c;
        }
        *d = 0;
    }

  #if !(defined FKS_USE_FNAME_ALLOCA)
    if (wk != dst) {    // ワークをallocaしてたのなら、コピー.
        fks_fnameCpy(dst, size, wk);
    }
  #endif

    return dst;
}


// ============================================================================
#if 1   // 本来は別モジュールだがついで.
/** コマンドライン引数や、;区切りの複数パス指定を、分解するのに使う.
 *  ""はwinコマンドラインにあわせた扱い.
 *  sepChrで区切られた文字列(ファイル名)を取得. 0x20以外の空白は無視か0x20扱い.
 *  @return スキャン更新後のアドレスを返す。strがEOSだったらNULLを返す.
 */
FKS_LIB_DECL FKS_FNAME_CHAR *fks_fnameScanArgStr(FKS_FNAME_CHAR arg[], unsigned argSz, const FKS_FNAME_CHAR *str, unsigned sepChr)
{
  #ifdef FKS_FNAME_WCS_COMPILE
    const FKS_FNAME_CHAR*   s = str;
  #else
    const unsigned char*    s = (const unsigned char*)str;
  #endif
    FKS_FNAME_CHAR*         d = arg;
    FKS_FNAME_CHAR*         e = d + argSz;
    unsigned                f = 0;
    unsigned                c;

    assert( str != 0 && arg != 0 && argSz > 1 );

    // 0x20以外の空白とセパレータをスキップ.
    while ( *s == sepChr || (*s > 0U && *s < 0x20U) || *s == 0x7fU) {
        ++s;
    }
    if (*s == 0) {  // EOSだったら、見つからなかったとしてNULLを返す.
        arg[0] = 0;
        return NULL;
    }

    do {
        c = *s++;
        if (c == FKS_FNAME_C('"')) {
            f ^= 1;                         // "の対の間は空白をファイル名に許す.ためのフラグ.

            // ちょっと気持ち悪いが、Win(XP)のcmd.exeの挙動に合わせてみる.
            // (ほんとにあってるか、十分には調べてない)
            if (*s == FKS_FNAME_C('"') && f == 0)   // 閉じ"の直後にさらに"があれば、それはそのまま表示する.
                ++s;
            else
                continue;                   // 通常は " は省いてしまう.
        }
        if ((c > 0 && c < 0x20) || c == 0x7f)
            c = FKS_FNAME_C(' ');
        if (d < e) {
            *d++ = (FKS_FNAME_CHAR)c;
        }
    } while (c >= 0x20 && c != 0x7f && (f || (c != sepChr)));
    *--d  = FKS_FNAME_C('\0');
    --s;
    return (FKS_FNAME_CHAR *)s;
}
#endif


// ============================================================================

// ファイル名文字列のポインタpから1文字取得してcにいれるマクロ.
// osがwinなら2バイト文字対応で小文字化. utf8は破綻.
#ifdef FKS_FNAME_WCS_COMPILE
 #if defined FKS_FNAME_WINDOS
  #define FKS_FNAME_GET_C(c, p)     ((c) = *((p)++), (c) = FKS_FNAME_TO_LOWER(c))
 #else
  #define FKS_FNAME_GET_C(c, p)     ((c) = *((p)++))
 #endif
#else
 #if defined FKS_FNAME_WINDOS
  #define FKS_FNAME_GET_C(c, p) do {                        \
        (c) = *(unsigned char*)((p)++);                     \
        if (FKS_FNAME_ISMBBLEAD(c) && *(p))                 \
            (c) = ((c) << 8) | *(unsigned char*)((p)++);    \
        else                                                \
            (c) = FKS_FNAME_TO_LOWER(c);                    \
    } while (0)
 #else
  #define FKS_FNAME_GET_C(c, p) do {                        \
        (c) = *(unsigned char*)((p)++);                     \
        if (FKS_FNAME_ISMBBLEAD(c) && *(p))                 \
            (c) = ((c) << 8) | *(unsigned char*)((p)++);    \
    } while (0)
 #endif
#endif


/** ファイル名の大小比較.
 *  win/dos系は大小同一視. ディレクトリセパレータ \ / も同一視.
 *  以外は単純に文字列比較.
 */
FKS_LIB_DECL int fks_fnameCmp(const FKS_FNAME_CHAR* l, const FKS_FNAME_CHAR* r)
{
    return fks_fnameNCmp(l, r, (unsigned)-1);
}


/** ファイル名の大小比較.
 *  win/dos系は大小同一視. ディレクトリセパレータ \ / も同一視.
 *  以外は単純に文字列比較.
 */
FKS_LIB_DECL int   fks_fnameNCmp(const FKS_FNAME_CHAR* l, const FKS_FNAME_CHAR* r, unsigned len)
{
 #if defined FKS_FNAME_WINDOS
    const FKS_FNAME_CHAR* e = l + len;
    assert( l != 0 && r != 0 );
    if (e < l)
        e = (const FKS_FNAME_CHAR*)-1;
    while (l < e) {
        int      n;
        unsigned lc;
        unsigned rc;

        FKS_FNAME_GET_C(lc, l);
        FKS_FNAME_GET_C(rc, r);

	 #ifdef FKS_FNAME_WINDOS
        if ((lc == FKS_FNAME_C('/') && rc == FKS_FNAME_C('\\')) || (lc == FKS_FNAME_C('\\') && rc == FKS_FNAME_C('/'))) {
            continue;
        }

		lc = FKS_FNAME_TO_LOWER(lc);
		rc = FKS_FNAME_TO_LOWER(rc);
	 #endif

        n  = (int)(lc - rc);
        if (n == 0) {
            if (lc == 0)
                return 0;
            continue;
        }

        return n;
    }
    return 0;
 #else
    int i;
    assert( l != 0 && r != 0 );
  #if 1
    return FKS_FNAME_N_CMP(l, r, len);
  #else
    char*   orig = setlocale(LC_CTYPE, s_fname_locale_ctype);
    i = FKS_FNAME_CMP(l, r);
    setlocale(orig);
    return i;
  #endif
 #endif
}


/** ファイル名の大小比較. 数値があった場合、桁数違いの数値同士の大小を反映
 *  win/dos系は大小同一視. ディレクトリセパレータ \ / も同一視.
 *  以外は単純に文字列比較.
 */
FKS_LIB_DECL int   fks_fnameDigitCmp(const FKS_FNAME_CHAR* l, const FKS_FNAME_CHAR* r)
{
    assert( l != 0 && r != 0 );
    for (;;) {
        ptrdiff_t   n;
        unsigned    lc;
        unsigned    rc;

        FKS_FNAME_GET_C(lc, l);
        FKS_FNAME_GET_C(rc, r);

      #ifdef FKS_FNAME_WINDOS
        if ((lc == FKS_FNAME_C('/') && rc == FKS_FNAME_C('\\')) || (lc == FKS_FNAME_C('\\') && rc == FKS_FNAME_C('/'))) {
            continue;
        }

		lc = FKS_FNAME_TO_LOWER(lc);
		rc = FKS_FNAME_TO_LOWER(rc);
	  #endif

        if (lc <= 0x80 && FKS_FNAME_IS_DIGIT(lc) && rc <= 0x80 && FKS_FNAME_IS_DIGIT(rc)) {
            ptrdiff_t   lv = FKS_FNAME_STRTOL(l-1, (char**)&l, 10);
            ptrdiff_t   rv = FKS_FNAME_STRTOL(r-1, (char**)&r, 10);
            n = lv - rv;
            if (n == 0)
                continue;
        }

        n  = (ptrdiff_t)(lc - rc);
        if (n == 0) {
            if (lc == 0)
                return 0;
            continue;
        }

        return (n < 0) ? -1 : 1;
    }
    return 0;
}


/** fnameがbaseNameで始まっていれば、fnameの余分の先頭のアドレスを返す.
 *  マッチしていなければNULLを返す.
 */
FKS_LIB_DECL FKS_FNAME_CHAR*     fks_fnameEquLong(FKS_FNAME_const_CHAR* fname, const FKS_FNAME_CHAR* baseName)
{
    unsigned l;
    assert(fname && baseName);
    l = fks_fnameLen(baseName);
    if (l == 0)
        return (FKS_FNAME_CHAR*)fname;
    return (fks_fnameNCmp(fname, baseName, l) == 0) ? (FKS_FNAME_CHAR*)fname+l : 0;
}


/** ?,* のみの(dos/winな)ワイルドカード文字列比較.
 * *,? はセパレータにはマッチしない.
 * ただし拡張して ** はセパレータにもマッチする.
 * ※unDonutのソースよりコピペ改造.
 *   その元はhttp://www.hidecnet.ne.jp/~sinzan/tips/main.htmらしいがリンク切.
 *  @param ptn  パターン(*?指定可能)
 *  @param tgt  ターゲット文字列.
 */
FKS_LIB_DECL int fks_fnameMatchWildCard(const FKS_FNAME_CHAR* ptn, const FKS_FNAME_CHAR* tgt)
{
    unsigned                tc;
    const FKS_FNAME_CHAR*   tgt2 = tgt;
    FKS_FNAME_GET_C(tc, tgt2);  // 1字取得& tgtポインタ更新.
    switch (*ptn) {
    case FKS_FNAME_C('\0'):
        return tc == FKS_FNAME_C('\0');

  #if defined FKS_FNAME_WINDOWS
    case FKS_FNAME_C('\\'):
    case FKS_FNAME_C('/'):
        return (tc == FKS_FNAME_C('/') || tc == FKS_FNAME_C('\\')) && fks_fnameMatchWildCard(ptn+1, tgt2);
  #endif

    case FKS_FNAME_C('?'):
        return tc && !fks_fnameIsSep(tc) && fks_fnameMatchWildCard( ptn+1, tgt2 );

    case FKS_FNAME_C('*'):
        if (ptn[1] == FKS_FNAME_C('*')) // ** ならセパレータにもマッチ.
            return fks_fnameMatchWildCard(ptn+2, tgt) || (tc && fks_fnameMatchWildCard( ptn, tgt2 ));
        return fks_fnameMatchWildCard(ptn+1, tgt) || (tc && !fks_fnameIsSep(tc) && fks_fnameMatchWildCard( ptn, tgt2 ));

    default:
        {
            unsigned pc;
            FKS_FNAME_GET_C(pc, ptn);   // 1字取得& ptnポインタ更新.
            return (pc == tc) && fks_fnameMatchWildCard(ptn, tgt2);
        }
    }
}


// ============================================================================

FKS_LIB_DECL FKS_FNAME_CHAR*    fks_fnameRelativePath(FKS_FNAME_CHAR dst[], unsigned size, const FKS_FNAME_CHAR* path, const FKS_FNAME_CHAR* currentDir) {
  #if defined FKS_FNAME_WINDOS
    return fks_fnameRelativePathBS(dst, size, path, currentDir);
  #else
    return fks_fnameRelativePathSL(dst, size, path, currentDir);
  #endif
}


/** 相対パス生成. ディレクトリセパレータを\\にして返すバージョン.
 */
FKS_LIB_DECL FKS_FNAME_CHAR*    fks_fnameRelativePathBS(FKS_FNAME_CHAR dst[], unsigned size, const FKS_FNAME_CHAR* path, const FKS_FNAME_CHAR* currentDir)
{
    fks_fnameRelativePathSL(dst, size, path, currentDir);
    fks_fnameSlashToBackslash(dst);
    return dst;
}


/** currentDirからの相対パス生成.
 *  currentDir は絶対パスであること. そうでない場合の挙動は不定.
 *  '\'文字対策で、セパレータは'/'に置き換ている.
 *  カレントディレクトリから始まる場合、"./"はつけない.
 */
FKS_LIB_DECL FKS_FNAME_CHAR*    fks_fnameRelativePathSL(FKS_FNAME_CHAR dst[], unsigned size, const FKS_FNAME_CHAR* path, const FKS_FNAME_CHAR* currentDir)
{
    enum { CHECK_MAX_PATH = sizeof(char[FKS_FNAME_MAX_PATH >= 16 ? 1 : -1]) };  // コンパイル時のサイズチェック.
    enum { CHECK_MAX_URL  = sizeof(char[FKS_FNAME_MAX_URL  >= 16 ? 1 : -1]) };  // コンパイル時のサイズチェック.
    FKS_FNAME_CHAR      curDir  [ FKS_FNAME_MAX_URL + 1 ];
    FKS_FNAME_CHAR      fullName[ FKS_FNAME_MAX_URL + 1 ];
    FKS_FNAME_CHAR*     cp;
    FKS_FNAME_CHAR*     cpSav;
    FKS_FNAME_CHAR*     fp;
    unsigned            cl;
    unsigned            fl;

    assert(dst != 0 && size > 2 && path != 0);
    if (dst == 0 || size <= 2 || path == 0)
        return 0;

    // まず、カレントディレクトリをフルパス化(/) & 最後に/を付加.
    assert(fks_fnameIsAbs(currentDir));
    fks_fnameFullpathSL(curDir, FKS_FNAME_MAX_URL, currentDir, FKS_FNAME_C("/"));
    cp = fks_fnameCheckLastSep(curDir);
    if (cp == 0) {
        cp = curDir + fks_fnameLen(curDir);
        if (cp+1 < curDir+FKS_FNAME_MAX_URL) {
            *cp++ = FKS_FNAME_C('/');
            *cp = 0;
        }
    }

    // 対象を path をフルパス化. \\は面倒なので/化した状態.
    fks_fnameFullpathSL(fullName, FKS_FNAME_MAX_URL, path, curDir);

    // マッチしているディレクトリ部分をスキップ.
    cp    = fks_fnameSkipDriveRoot(curDir);
    fp    = fks_fnameSkipDriveRoot(fullName);

    // ドライブ名が違っていたら相対パス化しない.
    cl    = cp - curDir;
    fl    = fp - fullName;
    if (cl != fl || fks_fnameNCmp(curDir, fullName, fl) != 0) {
        fks_fnameCpy(dst, size, fullName);
        return dst;
    }

    // 同じ部分をチェック.
    cpSav = cp;
    while (*cp && *fp) {
        unsigned cc;
        unsigned fc;
        FKS_FNAME_GET_C(cc, cp);
        FKS_FNAME_GET_C(fc, fp);
        if (cc != fc)
            break;
        if (*cp == FKS_FNAME_C('/')) {
            cpSav = (FKS_FNAME_CHAR*)cp + 1;
        }
    }
    fp      = fp - (cp - cpSav);
    cp      = cpSav;

    // カレント位置から上への移動数分../を生成.
    {
        FKS_FNAME_CHAR* d = dst;
        FKS_FNAME_CHAR* e = dst + size - 1;
        while (*cp) {
            if (*cp == FKS_FNAME_C('/')) {
                if (d < e) *d++ = FKS_FNAME_C('.');
                if (d < e) *d++ = FKS_FNAME_C('.');
                if (d < e) *d++ = FKS_FNAME_C('/');
            }
            ++cp;
        }
        *d = FKS_FNAME_C('\0');
    }

    // カレント位置以下の部分をコピー
    fks_fnameCat(dst, size, fp);

    return dst;
}


#if defined __cplusplus && !defined FKS_FNAME_WCS_COMPILE
}
#endif
// ============================================================================
