/**
 *  @file   misc_cstr.h
 *  @brief  c文字列向けの雑多なルーチン群
 *  @author tenk
 *  @note
 *  ※ int 32ビット専用. long が32か64かはcpu/compilerによる
 */

#ifndef MISC_CSTR_H
#define MISC_CSTR_H

#pragma once

#include "stdafx.h"
//#include "def.h"
//#include "misc_val.h"
#include <assert.h>

// MS全角処理のつじつま合わせよう
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__BORLANDC__)
#include <mbstring.h>
#else	    // シフトJIS専用
static inline int _ismbblead(int c)  { return ( ((uint8_t)(c) >= 0x81) & (((uint8_t)(c) <= 0x9F) | (((uint8_t)(c) >= 0xE0) & ((uint8_t)(c) <= 0xFC))) ); }
static inline int _ismbbtrail(int c) { return ( ((uint8_t)(c) >= 0x40) & ((uint8_t)(c) <= 0xfc) & ((c) != 0x7f) ); }
#endif


#define CALLOC	    calloc

#ifdef __cplusplus
#include <string>
// ===========================================================================
/// 雑多なルーチンのための名前空間。
//namespace MISC {
#endif



// ===========================================================================
// SJIS 関係
// win/dos系なら mbs文字列関数を使えばいいが、ターゲット機ではないので類似品用意
// ===========================================================================

#define USE_FNAME_SJIS

/// SJIS文字の上位バイトか?
static inline int ISKANJI(int c)    { return _ismbblead(c); }

/// SJIS文字の下位バイトか?
static inline int ISKANJI2(int c)  { return _ismbbtrail(c); }

/// 1バイト文字をsjisに変換する
unsigned asc2sjis(unsigned jc);




// ===========================================================================
// c文字列関係
// ===========================================================================

/// strncpy で、サイズいっぱいの場合に最後に'\0'にしておく
static inline char *strNCpyZ(char *dst, const char *src, size_t size) {
    assert(dst != 0 && src != 0 && size > 0);
    strncpy(dst, src, size);
    dst[size - 1] = 0;
    return dst;
}


/// 文字列 s の先頭空白文字をスキップしたアドレスを返す
static inline char *strSkipSpc(const char *s) {
    assert(s != 0);
    while (((*s != 0) & (*(const unsigned char *)s <= ' ')) | (*s == 0x7f))
    	s++;
    return (char*)s;
}


/// 文字列 s の先頭空白文字(改行lfは除く)をスキップしたアドレスを返す
static inline char *strSkipSpcNoLf(const char *s) {
    assert(s != 0);
    while (((*s != 0) && (*(const unsigned char *)s <= ' ' && *s != '\n')) || (*s == 0x7f))
    	s++;
    return (char*)s;
}


/// 文字列 s の空白以外の文字をスキップしたアドレスを返す
static inline char *strSkipNSpc(const char *s) {
    assert(s != 0);
    while ((*s != 0) & (*(const unsigned char *)s > ' ') & (*s != 0x7f))
    	s++;
    return (char*)s;
}


/// c文字列の最後に\nがあればそれを削除
static inline char *strDelLf(char s[]) {
    char *p;
    assert(s != 0);
    p = s + strlen(s);
    if (p != s && p[-1] == '\n')
    	p[-1] = 0;
    return s;
}


/// c文字列の最後に\nがあればそれを削除
static inline char *strTrimR(char s[]) {
    char *p;
    assert(s != 0);
    p = s + strlen(s);
    while (p > s && isspace((unsigned char)p[-1]))
    	*--p = '\0';
    return s;
}


/// 余分に addSz バイトメモリを確保する strdup().
static inline char *strDup(const char *s, unsigned int addSz) {
    char *d = (char*)CALLOC(1, strlen(s) + 1 + addSz);
    return strcpy(d,s);
}


/// stpcpy(d,s) の代用品.
static inline char *stpCpy(char *d, const char *s) {
    while ((*d = *s++) != '\0')
    	d++;
    return d;
}


/// 文字列の小文字を大文字に変換. struprの代用品
static inline char *strUpr(char *src) {
    unsigned char *s = (unsigned char *)src;
    while (*s) {
    	if (islower(*s))
    	    *s = toupper(*s);
    	s++;
    }
    return src;
}


/// 文字列の大文字を小文字に変換. strlwr の代用品
static inline char *strLwr(char *src) {
    unsigned char *s = (unsigned char *)src;
    while (*s) {
    	if (isupper(*s))
    	    *s = tolower(*s);
    	s++;
    }
    return src;
}


/// 大文字小文字を同一視する strcmp. stricmpの代用品
static inline int strICmp(const char *left, const char *right) {
    const unsigned char *l = (const unsigned char *)left;
    const unsigned char *r = (const unsigned char *)right;
    int c;
    while (((c = toupper(*l) - toupper(*r)) == 0) & (*l != '\0')) {
    	r++;
    	l++;
    }
    return c;
}


static inline int strEqu(const char *left, const char *right) {
    const unsigned char *l = (const unsigned char *)left;
    const unsigned char *r = (const unsigned char *)right;
    int c;
    while (((c = *l - *r) == 0) & (*l != '\0')) {
    	r++;
    	l++;
    }
    return c == 0;
}


#ifdef __cplusplus
inline int strEquLong(const char *left, const char *right, char const * *pNewLeft=0);
#endif
inline int strEquLong(const char *left, const char *right, char const * *pNewLeft) {
    const unsigned char *l = (const unsigned char *)left;
    const unsigned char *r = (const unsigned char *)right;
    while ((*l == *r) & (*r != '\0')) {
    	++r;
    	++l;
    }

    bool rc = (*r == '\0');
    if (rc) {	// left が right と同じか余分に文字列を持つならば 真
    	if (pNewLeft)	// 真の時は
    	    *pNewLeft = (const char *)l;
    }
    return rc;
}


/// strからsep の何れかの文字で区切られた文字列をtokに取得. strtok()の類似品
char *strGetTok(char tok[], const char *str, const char *sep);


/// 文字列末にある空白を削除する
char *strTrimSpcR(char str[], int flags);

// /// 文字列中の半角の大文字の小文字化,小文字の大文字化
// char *strUpLow(char str[], unsigned flags);

// /// @fn strTab src文字列中のtabを空白に,空白の繋がりを新たなtabに変換して文字列dstを作成.
// int strTab(char *dst, const char *src, int dstTabSz, int srcTabSz, int flags, int dstSz);



#ifdef __cplusplus
inline std::string strTrim(const std::string &str, const char *skipChrs = " \t\n") {
    if (str.size() == 0)
    	return str;
    std::size_t sn = str.find_first_not_of(skipChrs);
    std::size_t en = str.find_last_not_of(skipChrs);
    if (sn == std::string::npos)
    	return std::string("");
    return str.substr(sn, en+1 - sn);
}
#endif



/// crc32 を計算
unsigned int memCrc32(const void *dat, unsigned int siz);


/// mem から sz バイトのサムを求める
static inline uint64_t memSum64(const uint64_t *mem, uint32_t sz) {
    uint64_t sum = 0;
    sz = sz >> 3;
    if (sz) {
    	do {
    	    sum += *mem++;
    	} while (--sz);
    }
    return sum;
}



// ===========================================================================
// ファイル名関係
// ===========================================================================

/// パス名中のファイル名位置を探す.
static inline char *fname_getBase(const char *adr) {
    const char *p = adr;
    assert( adr != 0 );
    while (*p != '\0') {
    	if ((*p == ':') | (*p == '/') | (*p == '\\'))
    	    adr = p + 1;
      #ifdef USE_FNAME_SJIS
    	if (ISKANJI((*(unsigned char *) p)) & (p[1] != 0))
    	    p++;
      #endif
    	p++;
    }
    return (char*)adr;
}


/// 拡張子の位置を返す。なければ名前の最後を返す.
static inline char *fname_getExt(const char *name) {
    char *p;
    name = fname_getBase(name);
    p = strrchr((char*)name, '.');
    if (p)
    	return p+1;
    return (char *)(name+strlen(name));
}


/// 拡張子を付け足す
static inline char *fname_addExt(char filename[], const char *ext) {
    if (strrchr(fname_getBase(filename), '.') == NULL) {
    	strcat(filename, ".");
    	strcat(filename, ext);
    }
    return filename;
}


/** filenameの拡張子をextに付け替える.
 *  ext=""だと'.'が残るが、ext=NULLなら'.'ごと拡張子を外す.
 */
static inline char *fname_chgExt(char filename[], const char *ext) {
    char *p = (char *) fname_getBase(filename);
    p = strrchr(p, '.');
    if (p == NULL) {
    	if (ext) {
    	    strcat(filename, ".");
    	    strcat(filename, ext);
    	}
    } else {
    	if (ext == NULL)
    	    *p = 0;
    	else
    	    strcpy(p + 1, ext);
    }
    return filename;
}


#ifdef __cplusplus
/// 拡張子を付け替える
// std::string& fname_chgExt(std::string &fname, const char *ext);
#endif

/// path から ./ と ../ のサブディレクトリをルールに従って削除 (MS-DOS依存)
char *fname_delDotDotDir(char *path);

/** 文字列の最後に \ か / があれば削除 */
char *fname_delLastDirSep(char dir[]);



#ifdef __cplusplus
//} // namespace MISC
#endif


#endif	// MISC_CSTR_H
