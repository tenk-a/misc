/**
 *  @file   strtab.c
 *  @brief  文字列中の空白タブ変換を行う
 *
 *  @author 北村雅史<NBB00541@nifty.com>
 *  @date   2001～2004-01
 */

#include <stdlib.h>
#include <string.h>
#include "cmisc.h"

#ifdef __cplusplus
using namespace std;
#endif

#ifdef __cplusplus  // c++のときは、ネームスペースCMISC に放り込む
namespace CMISC {
#endif

// --------------------------------------------------------------------------

#undef	CHR_CODE_TYPE
#define CHR_CODE_TYPE	    3	// mb, utf8対応

#undef MGET1
#undef MGETC
#undef MPUT1
#undef MPUTC

#if CHR_CODE_TYPE == 1	    	// シフトJIS文字列として処理
#define IsKANJI_(c) 	    	    ((unsigned char)(c) >= 0x81 && ((unsigned char)(c) <= 0x9F || ((unsigned char)(c) >= 0xE0 && (unsigned char)(c) <= 0xFC)))
#define MGET1(ac, as)	    	    ((*(ac) = **(as)), ++*(as))
#define MGETC(ac, as, asn, utf8)    do { MGET1((ac),(as)); ++*(asn); if (IsKANJI_(*(ac)) && **(as)) {int c_;MGET1(&c_,(as));*(ac)=(*(ac)<<8)|c_; ++*(asn);} } while (0)
#define MPUT1(ad,e,c)	    	    do { if (*(ad) < (e)) {**(ad) = (c);} ; (*(ad))++; } while (0)
#define MPUTC(ad,e,c, adn, utf8)    do { if ((c) <= 0xff) {MPUT1((ad),(e),(c)); ++*(adn);} else {MPUT1((ad),(e),(c)>>8); MPUT1((ad),(e),(c)); *(adn) += 2;} } while (0)
#elif CHR_CODE_TYPE == 2    	    // EUCとして処理(..未チェック)
#define IsKANJI_(c) 	    	    ((unsigned char)(c) >= 0x80)
#define MGET1(ac, as)	    	    ((*(ac) = **(as)), ++*(as))
#define MGETC(ac, as, asn, utf8)    do { MGET1((ac),(as)); ++*(asn); if (IsKANJI_(*(ac)) && **(as)) {int c_;if (*(ac) != 0x81) {++*(asn);} MGET1(&c_,(as));*(ac)=(*(ac)<<8)|c_;} } while (0)
#define MPUT1(ad,e,c)	    	    do { if (*(ad) < (e)) {**(ad) = (c);} ; (*(ad))++; } while (0)
#define MPUTC(ad,e,c, adn, utf8)    do { if ((c) <= 0xff) {MPUT1((ad),(e),(c)); ++*(adn);} else {MPUT1((ad),(e),(c)>>8); MPUT1((ad),(e),(c)); *(adn) += 2;} } while (0)
#elif CHR_CODE_TYPE == 0    	    // シフトJISを考慮しない
#define MGET1(ac, as)	    	    ((*(ac) = **(as)), ++*(as))
#define MGETC(ac, as, asn, utf8)    do { MGET1(ac,as); ++*(asn); } while (0)
#define MPUT1(ad,e,c)	    	    do { if (*(ad) < (e)) {**(ad) = (c);} ; (*(ad))++; } while (0)
#define MPUTC(ad,e,c, adn, utf8)    do { MPUT1((ad),(e),(c)); ++*(adn); } while (0)
#else
// 強引に UTF8 対応.
// タブ位置の計算のため、0x7F以下と半角カナを半角1文字、以外を
// 全角1文字(半角2文字)で計算しています。日本語環境前提:-)

/// 1バイト取得
#define MGET1(ac, as)	    	    ((*(ac) = **(as)), ++*(as))
/// 1バイト書込
#define MPUT1(ad,e,c)	    	    do { if (*(ad) < (e)) {**(ad) = (c);} ; (*(ad))++; } while (0)

/** 1文字読み込み */
void MGETC(int *ac, const unsigned char **as, int *asn, int utf8)
{
    if (utf8 == 0) {
    	MGET1(ac, as);
    	++(*asn);
    	if (ISKANJI(*ac) && **as) {
    	    int c_;
    	    MGET1(&c_, as);
    	    *ac = (*ac << 8) | c_;
    	    ++(*asn);
    	}
    	return;
    } else {
    	int c;
    	MGET1(&c, as);
    	++(*asn);
    	if (c < 0x80) {
    	    ;
    	} else if (**as) {
    	    int c2;
    	    ++(*asn);
    	    MGET1(&c2, as);
    	    c2 &= 0x3F;
    	    if (c < 0xE0) {
    	    	c = ((c & 0x1F) << 6) | c2;
    	    } else if (**as) {
    	    	int c3;
    	    	MGET1(&c3, as);
    	    	c3 &= 0x3F;
    	    	if (c < 0xF0) {
    	    	    c = ((c & 0xF) << 12) | (c2 << 6) | c3;
    	    	    // 半角カナなら、半角文字扱い
    	    	    if (c >= 0xff60 && c <= 0xff9f) {
    	    	    	--(*asn);
    	    	    }
    	    	} else if (**as) {
    	    	    int c4;
    	    	    MGET1(&c4, as);
    	    	    c4 &= 0x3F;
    	    	    if (c < 0xF8) {
    	    	    	c = ((c&7)<<18) | (c2<<12) | (c3<<6) | c4;
    	    	    } else if (**as) {
    	    	    	int c5;
    	    	    	MGET1(&c5, as);
    	    	    	c5 &= 0x3F;
    	    	    	if (c < 0xFC) {
    	    	    	    c = ((c&3)<<24) | (c2<<18) | (c3<<12) | (c4<<6) | c5;
    	    	    	} else if (**as) {
    	    	    	    int c6;
    	    	    	    MGET1(&c6, as);
    	    	    	    c6 &= 0x3F;
    	    	    	    c = ((c&1)<<30) |(c2<<24) | (c3<<18) | (c4<<12) | (c5<<6) | c6;
    	    	    	}
    	    	    }
    	    	}
    	    }
    	}
    	*ac = c;
    }
}


/** 1文字書込 */
void MPUTC(unsigned char **ad, unsigned char *e, int c, int *adn, int utf8)
{
    if (utf8 == 0) {
    	if ((c) <= 0xff) {
    	    MPUT1((ad),(e),(c));
    	    ++*(adn);
    	} else {
    	    MPUT1((ad),(e),(c)>>8);
    	    MPUT1((ad),(e),(c));
    	    *(adn) += 2;
    	}
    	return;
    } else {
    	if (c < 0x80) {
    	    MPUT1(ad, e, c);
    	    ++*(adn);
    	} else {
    	    *(adn) += 2;
    	    if (c <= 0x7FF) {
    	    	MPUT1(ad, e, 0xC0|(c>>6));
    	    	MPUT1(ad, e, 0x80|(c&0x3f));
    	    } else if (c <= 0xFFFF) {
    	    	MPUT1(ad, e, 0xE0|(c>>12));
    	    	MPUT1(ad, e, 0x80|(c>>6)&0x3f);
    	    	MPUT1(ad, e, 0x80|(c&0x3f));
    	    	// 半角カナなら、半角文字扱い
    	    	if (c >= 0xff60 && c <= 0xff9f) {
    	    	    --(*adn);
    	    	}
    	    } else if (c <= 0x1fFFFF) {
    	    	MPUT1(ad, e, 0xF0|(c>>18));
    	    	MPUT1(ad, e, 0x80|(c>>12)&0x3f);
    	    	MPUT1(ad, e, 0x80|(c>>6)&0x3f);
    	    	MPUT1(ad, e, 0x80|(c&0x3f));
    	    } else if (c <= 0x3fffFFFF) {
    	    	MPUT1(ad, e, 0xF8|(c>>24));
    	    	MPUT1(ad, e, 0x80|(c>>18)&0x3f);
    	    	MPUT1(ad, e, 0x80|(c>>12)&0x3f);
    	    	MPUT1(ad, e, 0x80|(c>>6)&0x3f);
    	    	MPUT1(ad, e, 0x80|(c&0x3f));
    	    } else {
    	    	MPUT1(ad, e, 0xFC|(c>>30));
    	    	MPUT1(ad, e, 0x80|(c>>24)&0x3f);
    	    	MPUT1(ad, e, 0x80|(c>>18)&0x3f);
    	    	MPUT1(ad, e, 0x80|(c>>12)&0x3f);
    	    	MPUT1(ad, e, 0x80|(c>>6)&0x3f);
    	    	MPUT1(ad, e, 0x80|(c&0x3f));
    	    }
    	}
    }
}

#endif



/** src文字列中のtabを空白にして空白の繋がりを新たなtabに変換した文字列をdstに入れる
 *  @param dst	  出力バッファ. NULLのとき出力しない...サイズ計算を行うことになる
 *  @param flags  bit0=1 空白1文字はtabに変換しない 	    	    	    <br>
 *  	    	  bit1=1 Cの'"ペアを考慮.   	    	    	    	    <br>
 *  	    	  bit2=1 Cの￥エスケープを考慮	    	    	    	    <br>
 *  	    	  bit3=1 Cの'"情報として前回の結果の続きにする	    	    <br>
 *  	    	  bit4=1 タブサイズ丁度のときのみタブに変換する     	    <br>
 *  	    	  bit5=1 4タブ8タブどちらでも見た目が変わらないように変換   <br>
 *  	    	  bit6=1 CRのみも改行として扱う     	    	    	    <br>
 *  	    	  bit7=1 シフトJIS文字を考慮	    	    	    	    <br>
 *  	    	  bit8=1 行末空白を削除     	    	    	    	    <br>
 *  	    	  bit9=1 入力が UTF8 だ
 *  @param dstSz  出力先サイズ. 0ならサイズチェック無し
 *  @return 	  変換後のサイズ
 */
int strTab(char *dst, const char *src, int dstTabSz, int srcTabSz, int flags, int dstSz)
{
    enum {
    	F_SP1NTB = 0x01, F_CPAIR = 0x02, F_CESC = 0x04, F_CPAIRCONT = 0x08,
    	F_AJSTAB = 0x10, F_BOTH  = 0x20, F_CR	= 0x40, F_SJIS	    = 0x80,
    	F_TRIMR  = 0x100,F_UTF8  = 0x200
    };
    typedef unsigned char CHAR_T;
    const CHAR_T *s = (const CHAR_T *)src;
    CHAR_T  	*d = (CHAR_T*)dst;
    CHAR_T  	*e = (CHAR_T*)dst + dstSz;
    int     	jstab = (flags & F_AJSTAB) ? dstTabSz-1 : (flags & F_SP1NTB) ? 1 : 0;
    int     	tsn = -1;
    int     	sn = 0;
    int     	dn = 0;
    int     	utf8 = flags & F_UTF8;
    int     	k;
    int     	c;
    int     	c2;
    int     	n;
    int     	bsn;
    static int	cpairChr;
    static int	cmtMd;

    // '"の続きをするフラグがたっていない場合は初期化
    if ((flags & F_CPAIRCONT) == 0) {
    	cpairChr = 0;
    	cmtMd	 = 0;
    }
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
    	MGETC(&c, &s, &sn, utf8);
    	if (c == ' ' && k == 0) {   	    // 空白なら、とりあえずカウント
    	    if (tsn < 0)
    	    	tsn = bsn;
    	} else if (c == '\t' && k == 0) {   //
    	    if (tsn < 0)
    	    	tsn = bsn;
    	    sn = ((bsn+srcTabSz) / srcTabSz) * srcTabSz;
    	} else {
    	    if (tsn >= 0) { 	    // 空白があった
    	    	n = bsn - tsn;	    // c は必ず 1以上の値
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

    	    MPUTC(&d, e, c, &dn, utf8);

    	    if (flags & (F_CPAIR|F_CESC)) { 	// C/C++の " ' を考慮するとき
    	    	if (c == '\\' && *s && k != '`' && cmtMd == 0) {
    	    	    MGETC(&c2, &s, &sn, utf8);
    	    	    MPUTC(&d, e, c2, &dn, utf8);
    	    	} else if (c == '"' || c == '\'' || c == '`') { // " ' のチェック
    	    	    if (cmtMd == 0) {
    	    	    	if (k == 0)
    	    	    	    k = c;
    	    	    	else if (k == c)
    	    	    	    k = 0;
    	    	    }
    	    	} else if (c == '/' && (*s == '/' || *s == '*') && k == 0 && cmtMd == 0) { // // /* のとき
    	    	    cmtMd = *s;
    	    	    MGETC(&c2, &s, &sn, utf8);
    	    	    MPUTC(&d, e, c2, &dn, utf8);
    	    	} else if (c == '*' && *s == '/' && k == 0 && cmtMd == '*') { // */のとき
    	    	    cmtMd = 0;
    	    	    MGETC(&c2, &s, &sn, utf8);
    	    	    MPUTC(&d, e, c2, &dn, utf8);
    	    	} else if ((flags & F_CESC) && (unsigned)c < 0x20 && (k || (flags & F_CPAIR) == 0)) {
    	    	    static char xdit[0x10] = {
    	    	    	'0','1','2','3','4','5','6','7',
    	    	    	'8','9','a','b','c','d','e','f'
    	    	    };
    	    	    static char escc[0x20] = {	// a:0x07,b:0x08,t:0x09,n:0x0a,v:0x0b,f:0x0c,r:0x0d
    	    	    	0  , 0	, 0  , 0  , 0  , 0  , 0  , 'a',
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
    	    	if (cmtMd == '/')
    	    	    cmtMd = 0;
    	    } else if (c == '\r') {
    	    	if (*s == '\n') {
    	    	    s++;
    	    	    MPUT1(&d, e, '\n');
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

    // 文字列末の空白削除指定があって、"'中でないなら、実行
    if (dst && (flags & F_TRIMR) && ((k == 0) || (flags & F_CPAIR))) {
    	int cf = 1;
    	cf |= ((flags & F_CPAIR) != 0) << 1;
    	cf |= ((flags & F_SJIS)  != 0) << 7;
    	strTrimSpcR(dst, cf);
    	return strlen(dst);
    }
    return (char*)d - dst;
}


#undef MGET1
#undef MGETC
#undef MPUT1
#undef MPUTC


// --------------------------------------------------------------------------

/** 文字列末にある空白を削除する
 *  @param  str 文字列.書き換えられる
 *  @param  flags bit0=1:最後の'￥ｎ''￥ｒ'は残す   	    	    <br>
 *  	    	  bit1=1:C/C++ソース対策で ￥ の直後の' 'は１つ残す
 */
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
    if (c > 0x20 && c != 0x7f) {
    	--n;
    	p++;
    }
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

/** 文字列中の半角の大文字の小文字化,小文字の大文字化
 *  @param str	 対象文字列。書き換える
 *  @param flags bit0=1:小文字の大文字化    <br>
 *  	    	 bit1=1:大文字の小文字化    <br>
 *  	    	 bit7=1:シフトJISを考慮する
 */
char *strUpLow(char str[], unsigned flags)
{
    unsigned char *p = (unsigned char *)str;

    if ((flags&3) == 0 || str == NULL)	    // 変換指定がなかったり文字列が無かったら帰る
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

#ifdef __cplusplus  // c++のときは、ネームスペースCMISC に放り込む
};
#endif

