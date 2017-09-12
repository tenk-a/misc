/**
 *	@file	cmisc.h
 *	@brief	基本的にC言語で書かれた雑多なルーチン郡
 *
 *	@author	北村雅史<NBB00541@nifty.com>
 */


#ifndef CMISC_H
#define CMISC_H


#define STDERR			stdout
//#define STDERR		stderr
#define CERR			cout
//#define CERR			cerr


#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__BORLANDC__)
#  include <mbstring.h>
#else		// シフトJIS専用
#  define _ismbblead(c)		((unsigned char)(c) >= 0x81 && ((unsigned char)(c) <= 0x9F || ((unsigned char)(c) >= 0xE0 && (unsigned char)(c) <= 0xFC)))
#  define _ismbbtrail(c)	((unsigned char)(c) >= 0x40 && (unsigned char)(c) <= 0xfc && (c) != 0x7f)
#endif
#define ISKANJI(c)		_ismbblead(c)
#define ISKANJI2(c) 	_ismbbtrail(c)

#ifdef __cplusplus
#include <string>
#endif

#ifdef __cplusplus
namespace CMISC {
#endif

//-------------------------------------------------------------------------
// C文字列関係

/// 最後に必ず￥０を置く strncpy
char *strNCpyZ(char *dst, const char *src, unsigned size);

/// 最後に ￥ｎ があればそれを削除
char *strDelLf(char s[]);

/// 簡易に内部バッファでsprintfをしてそのアドレスを返す
char *strTmpF(const char *fmt, ...);

/// 余分に addSz バイトメモリを確保する strdup()
char *strDup(const char *s, int addSz);

#if defined(__cplusplus) || defined(inline)
/// @fn strSkipSpc 空白を読み飛ばす
inline char *strSkipSpc(const char *s) {
	while ((*s && *(const unsigned char *) s <= ' ') || *s == 0x7f) {s++;}
	return (char*)s;
}
#else
char *strSkipSpc(const char *s);
#endif

/// crc table
extern unsigned int memCrc32table[256];

/// crc32 を計算
int memCrc32(void *dat, int siz);


/// 文字列末にある空白を削除する
char *strTrimSpcR(char str[], int flags);

/// 文字列中の半角の大文字の小文字化,小文字の大文字化
char *strUpLow(char str[], unsigned flags);

/// @fn strTab src文字列中のtabを空白に,空白の繋がりを新たなtabに変換して文字列dstを作成.
#ifdef __cplusplus
int strTab(char *dst, const char *src, int dstTabSz, int srcTabSz, int flags=0, int dstSz=0);
#else
int strTab(char *dst, const char *src, int dstTabSz, int srcTabSz, int flags, int dstSz);
#endif



//-------------------------------------------------------------------------
// ファイル名関係

/// fname_ 系は SJISを対処する
#define USE_FNAME_SJIS

/// パス名中のファイル名位置を探す(MS-DOS依存)
char *fname_getBase(const char *adr);

/// 拡張子の位置を返す。なければ名前の最後を返す.
char *fname_getExt(const char *name);

/// 拡張子を付け替える
char *fname_chgExt(char filename[], const char *ext);

/// 拡張子を付け足す
char *fname_addExt(char filename[], const char *ext);

/// path から ./ と ../ のサブディレクトリをルールに従って削除 (MS-DOS依存)
char *fname_delDotDotDir(char *path);

#ifdef __cplusplus
std::string& fname_chgExt(std::string &fname, const char *ext);
#endif

#if __cplusplus
};		// CMISC
#endif


//-------------------------------------------------------------------------



#endif	// CMISC_H
