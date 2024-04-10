/**
 *  @file   cmisc.h
 *  @brief  基本的にC言語で書かれた雑多なルーチン郡.
 *
 *  @author Masashi Kitamura (tenka@6809.net)
 */


#ifndef CMISC_H
#define CMISC_H

#include <stddef.h>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__BORLANDC__)
#  include <mbstring.h>
#else	    // シフトJIS専用.
#  define _ismbblead(c)     ((unsigned char)(c) >= 0x81 && ((unsigned char)(c) <= 0x9F || ((unsigned char)(c) >= 0xE0 && (unsigned char)(c) <= 0xFC)))
#  define _ismbbtrail(c)    ((unsigned char)(c) >= 0x40 && (unsigned char)(c) <= 0xfc && (c) != 0x7f)
#endif
#define ISKANJI(c)  	_ismbblead(c)
#define ISKANJI2(c) 	_ismbbtrail(c)

#ifdef __cplusplus
#include <string>
#endif

#ifdef __cplusplus
namespace CMISC {
#endif

//-------------------------------------------------------------------------
// C文字列関係.

/// 文字列末にある空白を削除する.
char *strTrimSpcR(char str[], int flags);

/// 文字列中の半角の大文字の小文字化,小文字の大文字化.
char *strUpLow(char str[], unsigned flags);

/// @fn strTab src文字列中のtabを空白に,空白の繋がりを新たなtabに変換して文字列dstを作成.
#ifdef __cplusplus
size_t strTab(char *dst, const char *src, int dstTabSz, int srcTabSz, int flags=0, int dstSz=0);
#else
size_t strTab(char *dst, const char *src, int dstTabSz, int srcTabSz, int flags, int dstSz);
#endif

#if __cplusplus
};  	// CMISC
#endif

//-------------------------------------------------------------------------

#endif	// CMISC_H
