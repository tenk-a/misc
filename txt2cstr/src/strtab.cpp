/**
 *	@file	strtab.cpp
 *	@brief	文字列中の空白タブ変換を行う
 *
 *	@author 北村雅史<NBB00541@nifty.com>
 *	@date	2001～2003-07-27
 */

#include <stdlib.h>
#include <string.h>
#include "cmisc.h"

#ifdef __cplusplus
using namespace std;
#endif

#ifdef __cplusplus	// c++のときは、ネームスペースCMISC に放り込む
namespace CMISC {
#endif

// --------------------------------------------------------------------------

#undef	CHR_CODE_TYPE
#define CHR_CODE_TYPE		1

#undef MGET1
#undef MGETC
#undef MPUT1
#undef MPUTC

#if CHR_CODE_TYPE == 1			// シフトJIS文字列として処理
#define IsKANJI_(c) 		((unsigned char)(c) >= 0x81 && ((unsigned char)(c) <= 0x9F || ((unsigned char)(c) >= 0xE0 && (unsigned char)(c) <= 0xFC)))
#define MGET1(ac, as)		((*(ac) = **(as)), ++*(as))
#define MGETC(ac, as, asn)	do { MGET1((ac),(as)); ++*(asn); if (IsKANJI_(*(ac) && **(as))) {int c_;MGET1(&c_,(as));*(ac)=(*(ac)<<8)|c_; ++*(asn);} } while (0)
#define MPUT1(ad,e,c)		do { if (*(ad) < (e)) {**(ad) = (c);} ; (*(ad))++; } while (0)
#define MPUTC(ad,e,c, adn)	do { if ((c) <= 0xff) {MPUT1((ad),(e),(c)); ++*(adn);} else {MPUT1((ad),(e),(c)>>8); MPUT1((ad),(e),(c)); *(adn) += 2;} } while (0)
#elif CHR_CODE_TYPE == 2		// EUCとして処理(..未チェック)
#define IsKANJI_(c) 		((unsigned char)(c) >= 0x80)
#define MGET1(ac, as)		((*(ac) = **(as)), ++*(as))
#define MGETC(ac, as, asn)	do { MGET1((ac),(as)); ++*(asn); if (IsKANJI_(*(ac) && **(as))) {int c_;if (*(ac) != 0x81) {++*(asn);} MGET1(&c_,(as));*(ac)=(*(ac)<<8)|c_;} } while (0)
#define MPUT1(ad,e,c)		do { if (*(ad) < (e)) {**(ad) = (c);} ; (*(ad))++; } while (0)
#define MPUTC(ad,e,c, adn)	do { if ((c) <= 0xff) {MPUT1((ad),(e),(c)); ++*(adn);} else {MPUT1((ad),(e),(c)>>8); MPUT1((ad),(e),(c)); *(adn) += 2;} } while (0)
#else	// CHR_CODE_TYPE == 0	// シフトJISを考慮しない
#define MGET1(ac, as)		((*(ac) = **(as)), ++*(as))
#define MGETC(ac, as, asn)	do { MGET1(ac,as); ++*(asn); } while (0)
#define MPUT1(ad,e,c)		do { if (*(ad) < (e)) {**(ad) = (c);} ; (*(ad))++; } while (0)
#define MPUTC(ad,e,c, adn)	do { MPUT1((ad),(e),(c)); ++*(adn); } while (0)
#endif

/// src文字列中のtabを空白にして空白の繋がりを新たなtabに変換した文字列をdstに入れる
/// @param dst	  出力バッファ. NULLのとき出力しない...サイズ計算を行うことになる
/// @param flags  bit0=1 空白1文字はtabに変換しない
/// 			  bit1=1 Cの'"ペアを考慮.
///               bit2=1 Cの\エスケープを考慮
///               bit3=1 Cの'"情報として前回の結果の続きにする
///               bit4=1 タブサイズ丁度のときのみタブに変換する
///               bit5=1 4タブ8タブどちらでも見た目が変わらないように変換
///               bit6=1 CRのみも改行として扱う
///               bit7=1 シフトJIS文字を考慮
///               bit9=8 行末空白を削除
/// @param dstSz  出力先サイズ. 0ならサイズチェック無し
/// @return       変換後のサイズ
int strTab(char *dst, const char *src, int dstTabSz, int srcTabSz, int flags, int dstSz)
{
	enum {F_SP1NTB = 0x01, F_CPAIR = 0x02, F_CESC = 0x04, F_CPAIRCONT = 0x08,
		  F_AJSTAB = 0x10, F_BOTH  = 0x20, F_CR   = 0x40, F_SJIS      = 0x80, F_TRIMR= 0x100,};
	typedef char CHAR_T;
	const CHAR_T *s = (const CHAR_T *)src;
	CHAR_T		*d = dst;
	CHAR_T		*e = dst + dstSz;
	int			jstab = (flags & F_AJSTAB) ? dstTabSz-1 : (flags & F_SP1NTB) ? 1 : 0;
	int			tsn = -1;
	int			sn = 0;
	int			dn = 0;
	int			k, c, c2, n, bsn;
	static int  cpairChr;

	// '"の続きをするフラグがたっていない場合は初期化
	if ((flags & F_CPAIRCONT) == 0)
		cpairChr = 0;
	k = cpairChr;

	// src がNULLなら、たぶん、cpairChrの初期化だ
	if (src == NULL)
		return 0;

	// サイズが0なら、チェックなしとして、終了を目一杯大きいアドレスにする
	if (dstSz == 0)
		e = (CHAR_T*)(~0);

	// 出力先がNULLなら、バイト数のカウントのみにするため、終了アドレスもNULL
	if (dst == NULL)
		e = NULL;

	// 元tabサイズが0(以下)なら、割り算で破綻しないようにとりあえず1にしとく
	if (srcTabSz <= 0)
		srcTabSz = 1;

	// 4tab,8tab両用にするならとりあえず4tab扱い
	if (flags & F_BOTH)
		dstTabSz = 4;

	// 文字列が終わるまでループ
	while (*s) {
		// 1文字取得
		bsn = sn;
		MGETC(&c, &s, &sn);
		if (c == ' ' && k == 0) {			// 空白なら、とりあえずカウント
			if (tsn < 0)
				tsn = bsn;
		} else if (c == '\t' && k == 0) {	//
			if (tsn < 0)
				tsn = bsn;
			sn = ((bsn+srcTabSz) / srcTabSz) * srcTabSz;
		} else {
			if (tsn >= 0) { 		// 空白があった
				n = bsn - tsn;		// c は必ず 1以上の値
				if (dstTabSz <= 0) {
					//空白への変換
					do {
						MPUT1(&d, e, ' ');
					} while (--n);
				} else if (flags & F_BOTH) {
					// 4tab,8tab両用変換
					int m  = dn/dstTabSz;
					int tn = (m + 1) * dstTabSz;
					int l  = tn - dn;
					dn += n;
					if (dn >= tn) {
						if ((l <= jstab && jstab) || (m&1) == 0) {
							do {
								MPUT1(&d, e, ' ');
							} while (--l);
						} else {
							MPUT1(&d, e, '\t');
						}
						while (dn >= (tn += dstTabSz)) {
							++m;
							if (m & 1) {
								MPUT1(&d, e, '\t');
							} else {
								for (l = 4; --l >= 0;)
									MPUT1(&d, e, ' ');
							}
						}
						tn -= dstTabSz;
						if (dn > tn) {
							n = dn - tn;
							do {
								MPUT1(&d, e, ' ');
							} while (--n);
						}
					} else {
						do {
							MPUT1(&d, e, ' ');
						} while (--n);
					}
				} else {
					// 通常のタブ変換
					int tn = ((dn / dstTabSz) + 1) * dstTabSz;
					int l  = tn - dn;
					dn += n;
					if (dn >= tn) {
						if (l <= jstab && jstab) {
							// フラグ指定によりtabが空白一個、またはタブサイズに満たない場合、
							// 空白にする指定があったら空白
							do {
								MPUT1(&d, e, ' ');
							} while (--l);
						} else {
							MPUT1(&d, e, '\t');
						}
						while (dn >= (tn += dstTabSz)) {
							MPUT1(&d, e, '\t');
						}
						tn -= dstTabSz;
						if (dn > tn) {
							n = dn - tn;
							do {
								MPUT1(&d, e, ' ');
							} while (--n);
						}
					} else {
						do {
							MPUT1(&d, e, ' ');
						} while (--n);
					}
				}
				tsn = -1;
			}
			MPUTC(&d, e, c, &dn);
			if (flags & (F_CPAIR|F_CESC)) { 	// C/C++の " ' を考慮するとき
				if (c == '\\' && *s) {
					MGETC(&c2, &s, &sn);
					MPUTC(&d, e, c2, &dn);
				} else if (c == '"' || c == '\'') {	// " ' のチェック
					if (k == 0)
						k = c;
					else if (k == c)
						k = 0;
				} else if ((flags & F_CESC) && (unsigned)c < 0x20 && (k || (flags & F_CPAIR) == 0)) {
					static char xdit[0x10] = {
						'0','1','2','3','4','5','6','7',
						'8','9','a','b','c','d','e','f'
					};
					static char escc[0x20] = {	// a:0x07,b:0x08,t:0x09,n:0x0a,v:0x0b,f:0x0c,r:0x0d
						0  , 0	, 0  , 0  , 0  , 0	, 0  , 'a',
						'b', 't', 'n', 'v', 'f', 'r', 0  , 0  ,
					};
					--d;
					MPUT1(&d, e, '\\');
					c2 = escc[c];
					if (c2) {
						MPUT1(&d, e, c2);
						dn++;
					} else {
						MPUT1(&d, e, 'x');
						MPUT1(&d, e, xdit[c>>4]);
						MPUT1(&d, e, xdit[c&15]);
						dn+=3;
					}
				}
			}
			if (c == '\n') {
				sn = dn = 0;
			} else if (c == '\r') {
				if (*s == '\n') {
					s++;
					MPUT1(&d, e, '\n');
					sn = dn = 0;
				} if (flags & F_CR) {
					sn = dn = 0;
				}
			}
		}
	}
	cpairChr = k;
	if (d < e)
		*d = '\0';
	else if (dst && dstSz > 0)
		dst[dstSz-1] = '\0';

	// 文字列末の空白削除指定があって、"'中でないなら、実行
	if ((flags & F_TRIMR) && k == 0) {
		int cf = 1;
		cf |= ((flags & F_CPAIR) != 0) << 1;
		cf |= ((flags & F_SJIS)  != 0) << 7;
		strTrimSpcR(dst, cf);
		return strlen(dst);
	} else {
		return d - dst;
	}
}


#undef MGET1
#undef MGETC
#undef MPUT1
#undef MPUTC


// --------------------------------------------------------------------------

/// 文字列末にある空白を削除する
/// @param	str 文字列.書き換えられる
/// @param	flags	bit0=1:最後の'\n''\r'は残す
/// 				bit1=1:C/C++ソース対策で \ の直後の' 'は１つ残す
char *strTrimSpcR(char str[], int flags)
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

	// 改行文字の状態を設定
	cr = 0;
	if (flags & 1) {	// 改行を考慮する？
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
	// 行末の空白部分を飛ばしてそうでない部分が現れるまで探す
	n = 0;
	do {
		c = *--p;
		n++;
	} while (p > s && c && (c <= 0x20 || c == 0x7f));
	--n;
	p++;
	// c/c++を考慮するとき、1文字以上の空白が\ の直後にある状態なら、空白を1文字だけ戻す
	if ((flags & 2) && n && p > s && p[-1] == '\\') {
		*p++ = ' ';
	}
	// 必要なら改行コードを復元する
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


// --------------------------------------------------------------------------

/// 文字列中の半角の大文字の小文字化,小文字の大文字化
/// @param str	対象文字列。書き換える
/// @param flags	bit0=1:小文字の大文字化
/// 				bit1=1:大文字の小文字化
/// 				bit7=1:シフトJISを考慮する
char *strUpLow(char str[], unsigned flags)
{
	unsigned char *p = (unsigned char *)str;

	if ((flags&3) == 0 || str == NULL)		// 変換指定がなかったり文字列が無かったら帰る
		return str;
	while (*p) {
		int c = *p++;
		if (ISKANJI(c) && *p && (flags & 0x80)) {
			p++;
		} else {
			if (c < 'A') {
				;
			} else if (c <= 'Z') {
				if (flags & 2)
					*(p - 1) = c + 0x20;
			} else if (c < 'a') {
				;
			} else if (c <= 'z') {
				if (flags & 1)
					*(p - 1) = c - 0x20;
			}
		}
	}
	return str;
}


// --------------------------------------------------------------------------

#ifdef __cplusplus	// c++のときは、ネームスペースCMISC に放り込む
};
#endif

