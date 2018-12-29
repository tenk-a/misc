/*
    knjt    	v3.00
    履歴
    198?    os9/09 6809のアセンブラで。一水のみだったような気もする。
    	    どのような仕様だったかもう覚えていない^^;
    199?    ANSI-C(MS-DOS)  行方不明。
    2000/10 v1.00 行方不明ゆえ作り直し。
    	    でもって、いろいろごちゃごちゃ機能つける.
    	    v1.01 -pN のNページごとを付加
    2004/01 v2.00 unicode表示を付加. eucや句点の扱いを変更. -tの仕様変更
    2018/12 v3.00 JIS2004(JIS X 213) ベースにし、オプションで MS-CP932 に対応.(jis-unicode変換はMS依存)

    ライセンスは Boost software license Version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#define USE_WIN_API
#endif


#define ISKANJI(c)  	((uint8_t)(c) >= 0x81 && ((uint8_t)(c) <= 0x9F || ((uint8_t)(c) >= 0xE0 && (uint8_t)(c) <= 0xFC)))
#define ISKANJI2(c) 	((uint8_t)(c) >= 0x40 && (uint8_t)(c) <= 0xfc && (c) != 0x7f)
#define JIS2KUTEN(j)   	(((((j) >> 8) & 0x7f) - 0x21) * 94 + (((j) & 0x7f) - 0x21))


/** 8bit数二つを上下につなげて16ビット数にする */
#define BB(a,b)     	((((uint8_t)(a))<<8)|(uint8_t)(b))

/** 8bit数4つを上位から順につなげて32ビット数にする */
#define BBBB(a,b,c,d)	(((uint32_t)((uint8_t)(a))<<24)|(((uint8_t)(b))<<16)|(((uint8_t)(c))<<8)|((uint8_t)(d)))

/** 8bit数6つを... 64bit整数用 */
#define BBBBBB(a,b,c,d,e,f) (((uint64_t)((uint8_t)(a))<<40)|((uint64_t)((uint8_t)(b))<<32)|(((uint8_t)(c))<<24)|(((uint8_t)(d))<<16)|(((uint8_t)(e))<<8)|((uint8_t)(f)))

static char kuten[94 * 2 + 2];
static int  kutenMen = 0;
static char ms932 = 0;
static char ms932xxxxFlag = 0;
#ifdef USE_WIN_API
static char useWinApi = 0;
#endif

//extern unsigned 		kuten2004_to_msUCS2_tbl[];
//extern unsigned 		kuten_to_msUCS2_tbl[];
//extern unsigned short	ubyte_to_msUCS2_tbl[];
unsigned short ubyte_to_msUCS2_tbl[] = {
    0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,0x0008,0x0009,0x000a,0x000b,0x000c,0x000d,0x000e,0x000f,
    0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,0x0018,0x0019,0x001a,0x001b,0x001c,0x001d,0x001e,0x001f,
    0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,0x0028,0x0029,0x002a,0x002b,0x002c,0x002d,0x002e,0x002f,
    0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003a,0x003b,0x003c,0x003d,0x003e,0x003f,
    0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004a,0x004b,0x004c,0x004d,0x004e,0x004f,
    0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005a,0x005b,0x005c,0x005d,0x005e,0x005f,
    0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,0x0068,0x0069,0x006a,0x006b,0x006c,0x006d,0x006e,0x006f,
    0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,0x007a,0x007b,0x007c,0x007d,0x007e,0x007f,
    0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
    0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
    0xf8f0,0xff61,0xff62,0xff63,0xff64,0xff65,0xff66,0xff67,0xff68,0xff69,0xff6a,0xff6b,0xff6c,0xff6d,0xff6e,0xff6f,
    0xff70,0xff71,0xff72,0xff73,0xff74,0xff75,0xff76,0xff77,0xff78,0xff79,0xff7a,0xff7b,0xff7c,0xff7d,0xff7e,0xff7f,
    0xff80,0xff81,0xff82,0xff83,0xff84,0xff85,0xff86,0xff87,0xff88,0xff89,0xff8a,0xff8b,0xff8c,0xff8d,0xff8e,0xff8f,
    0xff90,0xff91,0xff92,0xff93,0xff94,0xff95,0xff96,0xff97,0xff98,0xff99,0xff9a,0xff9b,0xff9c,0xff9d,0xff9e,0xff9f,
    0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
    0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
};

extern unsigned short const fks_mbc_kutenIdx2uni[];
extern unsigned const 		fks_mbc_kutenIdx2uniAdd[];
static unsigned short const* fks_mbc_cp932kutenIdx2uni;

/** 使い方 */
static void Usage(void)
{
    printf(
    	"usage>knjt [-opts]       // v3.00 " __DATE__ "\n"
    	"    https://github.com/tenk-a/misc/tree/master/knjt\n"
    	"    全角文字一覧のテキストを生成する.\n"
    	"  -opts:  （デフォルトは -l16 -c1 -t0）\n"
    	"    -w[seo]    結果を s:SJIS e:EUC o:UTF8 にして出力\n"
    	"    -t[kjseuo-] 各行の先頭に文字コードを付加. -t- だと付加しない\n"
    	"               k:句点 j:JIS s:SJIS e:EUC u:UTF32 o:UTF8 を付加.(組合わせる)\n"
    	"    -x         SJIS(JIS) を MS-CP932 とし、拡張95-120区も対象.\n"
	 #ifdef USE_WIN_API
		"    -xapi      cp932->unicodeの変換に win-api を用いる\n"
	 #endif
    	"    -X212      JIS X 212 の 2面 2,6,7,9-11,16-77区も対象(SJISは嘘値)\n"
    	"    -k         区の分類の表示(ヘルプ)\n"
    	"    -k[N:M]    表示する区の範囲を指定. 1～94. 複数指定可能\n"
    	"    -kb[N:M]   2面の区指定. 1～94.\n"
    	"    -0         記号や英数カナ等.    -k1:8  に同じ\n"
    	"    -0x        1-15区指定\n"
    	"    -1         漢字第一水準の指定. -k16:47 に同じ\n"
    	"    -2         漢字第ニ水準の指定. -k48:84 に同じ\n"
    	"    -3         JIS X 213 漢字第三水準の指定. -k84:94 に同じ\n"
    	"    -4         JIS X 213 漢字第四水準の指定. 2面の1,3-5,8,12-15,78-94区指定\n"
    	"    -l[N]      1行に表示する文字数. 1～94(デフォルト 16)\n"
    	"               同時に -c0 が指定されたことになる.\n"
    	//"    -cN        JISコード換算での??20,??7fを\n"
    	//"               0:表示しない    1:半角空白２つ'  '(デフォルト)\n"
    	//"               2:全角空白一つ'　'\n"
    	"    -m         目盛りを付加\n"
    	"    -p[N]      区ごとに空行(+目盛り)を挿入.(Nが2以上ならN区ごとに挿入)\n"
    	"    -b         テキストでなく、文字コードのみのバイナリ出力\n"
    	"\n"
    	//"   SJIS は JIS2004ベースだが 1-88-94(JIS:787E,SJIS:ECFC)以下でcp932に\n"
    	//"   存在する(類似だが違う)文字はcp932に合わせている。\n"
    	"   JIS X 213(2004) ベースだが、0x7f以下のコードへの変換はしないよう\n"
    	"   Fullwidth 文字に変更している.\n"
    	"   ※区点1-89-01=JIS:7921=SJIS:ED40 以降についてはMS-CP932と非互換になる.\n"
    	"\n"
    );
    exit(1);
}


/* ----------------------------------------------------------*/


/** JIS を EUC に変換 */
static int jis2eucjp(unsigned c)
{
	if (c < 0x80) {
		return c;
	} else if ( c <= 0xff) {
		return 0x8e00|c;
	} else if (c <= 0xffff) {
		return 0x8080|c;
	} else {
		return 0xf88080 | (uint16_t)c;
	}
}

static int eucjp2jis(int c)
{
	if (c <= 0xff) {
		return c;
	} else if ((c & 0xff00) == 0x8e00) {
		return (uint8_t)c;
	} else if (c <= 0xffff) {
		return c & ~0x8080;
	} else {
		return 0x10000|((uint16_t)c & ~0x8080);
	}
}

/** jis コードを シフトjisに変換 */
static int jis2sjis(int c)
{
	if (c <= 0xffff) {
	    c -= 0x2121;
	    if (c & 0x100)
	    	c += 0x9e;
	    else
	    	c += 0x40;
	    if ((uint8_t)c >= 0x7f)
	    	++c;
	    c = (((c >> (8+1)) + 0x81)<<8) | ((uint8_t)c);
	    if (c >= 0xA000)
	    	c += 0x4000;
	    return c;
	} else {	// jis2004
		unsigned a, b;
		b = (uint16_t)c - 0x2121;
		a = b >> 8;
	    if (b & 0x100)
	    	b += 0x9e;
	    else
	    	b += 0x40;
	    b = (uint8_t)b;
		if (b >= 0x7f)
			++b;
		if (a < 78-1) {	// 1,3,4,5,8,12,15-ku (0,2,3,4,7,11,14)
			a = (a + 1 + 0x1df) / 2 - ((a+1)/8) * 3;
		} else { // 78..94
			a = (a + 1 + 0x19b) / 2;
		}
		return (a << 8) | b;
	}
}


/** シフトjis を jis に変換 */
int sjis2jis(int c)
{
	if (ms932) {
	    if (c >= 0xE000)
	    	c -= 0x4000;
	    c = (((c>>8) - 0x81)<<9) | (uint8_t)c;
	    if ((uint8_t)c >= 0x80)
	    	c -= 1;
	    if ((uint8_t)c >= 0x9e)
	    	c += 0x62;
	    else
	    	c -= 0x40;
	    c += 0x2121;
	    return c;
	} else {
		if (c < 0xf000) {
		    if (c >= 0xE000)
		    	c -= 0x4000;
		    c = (((c>>8) - 0x81)<<9) | (uint8_t)c;
		    if ((uint8_t)c >= 0x80)
		    	c -= 1;
		    if ((uint8_t)c >= 0x9e)
		    	c += 0x62;
		    else
		    	c -= 0x40;
		    c += 0x2121;
		    return c;
		} else {	// jis2004
			unsigned a, b, f;
			b = (uint8_t)c;
			a = (uint8_t)(c >> 8);
			f = (b >= 0x9f);
			if (c < 0xf29f) {
				if (c < 0xf100) {
					a = (f) ? 0x28 : 0x21;
				} else {
					a = (a - 0xf1) * 2 + 0x23 + f;
				}
			} else {
				if (c < 0xf49f) {
					a = (a - 0xf2) * 2 + 0x2c - 1 + f;
				} else {
					a = (a - 0xf4) * 2 + 0x6e - 1 + f;
				}
			}
			if (b <= 0x7e) {
				b  = b - 0x40 + 0x21;
			} else if (b <= 0x9e) {
				b  = b - 0x80 + 0x60;
			} else {
				b  = b - 0x9f + 0x21;
			}
			return 0x10000|(a << 8)|b;
		}
	}
}

/** UCS2 を utf8 に変換.*/
uint64_t utf32toUtf8(unsigned c)
{
    if (c < 0xC0/*0x80*/) {
    	return c;
    } else if (c <= 0x7FF) {
    	return BB(0xC0|(c>>6), 0x80|(c&0x3f));
    } else if (c <= 0xffFF) {
    	return BBBB(0, 0xE0|(c>>12), 0x80|(c>>6)&0x3f, 0x80|(c&0x3f));
    } else if (c <= 0x1FffFF) {
    	return BBBB(0xF0|(c>>18), 0x80|(c>>12)&0x3f, 0x80|(c>>6)&0x3f, 0x80|(c&0x3f));
    } else if (c <= 0x3fFFffFF) {
    	return BBBBBB(00, 0xF8|(c>>24), 0x80|(c>>18)&0x3f, 0x80|(c>>12)&0x3f, 0x80|(c>>6)&0x3f, 0x80|(c&0x3f));
    } else {
    	return BBBBBB(0xFC|(c>>30), 0x80|(c>>24)&0x3f, 0x80|(c>>18)&0x3f, 0x80|(c>>12)&0x3f, 0x80|(c>>6)&0x3f, 0x80|(c&0x3f));
    }
}

#ifdef USE_WIN_API
unsigned ms932ToUtf32winApi(int sjis) {
	char sjisStr[4] = { 0 };
	wchar_t wcs[4] = { 0 };
	int l;
	unsigned c;
	sjisStr[0] = sjis >> 8;
	sjisStr[1] = sjis;
    l = MultiByteToWideChar(932, MB_ERR_INVALID_CHARS, sjisStr, 2, wcs, 3);
	c = (l) ? wcs[0] : 0;
	if (0xD800 <= c && c <= 0xDBFF) {
		unsigned d = wcs[1];
		if (0xDC00 <= d && d <= 0xDFFF) {
			c = ((c & 0x3ff) << 10) | (d & 0x3ff);
			c += 0x10000;
		}
	}
	return c;
}
#endif

static void fks_mbc_init_cp932kutenIdx2uni(void)
{
	static unsigned short const s_repl_cp932[9][2] = {
		{ (1-1)*94+(17-1), 0xFFE3 },		// 203E →	￣	OVERLINE
		{ (1-1)*94+(29-1), 0x2015 },		// 2014 →	―	EM DASH
		{ (1-1)*94+(33-1), 0xFF5E },		// 301C →	～	WAVE DASH
		{ (1-1)*94+(34-1), 0x2225 },		// 2016 →	∥	DOUBLE VERTICAL LINE
		{ (1-1)*94+(61-1), 0xFF0D },		// 2212 →	2	MINUS SIGN
		{ (1-1)*94+(79-1), 0xFFE5 },		// 00A5 →	￥	YEN SIGN
		{ (1-1)*94+(81-1), 0xFFE0 },		// 00A2 →	￠	CENT SIGN
		{ (1-1)*94+(82-1), 0xFFE1 },		// 00A3 →	￡	POUND SIGN
		{ (2-1)*94+(44-1), 0xFFE2 },		// 00AC →	￢	NOT SIGN
	};
	int i, c;
	unsigned short const* s;
	unsigned short* dst = (short*)calloc(1, 120 * 94 * sizeof(unsigned short));
	if (!dst)
		return;
	memcpy(dst, fks_mbc_kutenIdx2uni, 120*94*sizeof(unsigned short));
	s = fks_mbc_kutenIdx2uni + 2 * 94 * 94;
	// 89-92:NEC選定IBM拡張文字.
	for (i = (89-1)*94; i < (93-1) * 94; ++i) {
		c = s[i - (89-1)*94];
		if (c)
			dst[i] = c;
	}
	// ユーザー外字.
	for (i = (95-1) * 94; i < (115-1) * 94; ++i) {
		c = 0xE000 + i - (95-1) * 94;
		dst[i] = c;
	}
	// 115-119:IBM拡張文字.
	for (i = (115-1) * 94; i < (119-1) * 94 + 12; ++i) {
		c = s[i - (115-1)*94 + (93-89)*94];
		if (c)
			dst[i] = c;
	}

	for (i = 0; i < 9; ++i)
		dst[s_repl_cp932[i][0]] = s_repl_cp932[i][1];
	fks_mbc_cp932kutenIdx2uni = dst;
}

unsigned ms932JisToUtf32(unsigned jis)
{
	unsigned ku  = ((jis >> 8) - 0x21);
	unsigned ten = (jis & 0xff) - 0x21;
	unsigned idx = ku * 94 + ten;
	unsigned rc;
	if (!fks_mbc_cp932kutenIdx2uni)
		fks_mbc_init_cp932kutenIdx2uni();
	rc = fks_mbc_cp932kutenIdx2uni[idx];
	if (0xDC00 <= rc && rc <= 0xDFFF)
		rc = fks_mbc_kutenIdx2uniAdd[rc - 0xDC00];
	return rc;
}

unsigned ms932ToUtf32(unsigned sjis)
{
#ifdef USE_WIN_API
	if (useWinApi) {
		return ms932ToUtf32winApi(sjis);
	} else
#endif
	{
		unsigned jis = sjis2jis(sjis);
		return ms932JisToUtf32(jis);
	}
}

/** JIS X 213:2004(with JIS X 212) を UTF32 に変換
 * return unicode (bit31が立っていた場合は 2キャラ合成)
 */
unsigned jis2utf32(unsigned jis)
{
	unsigned men = jis >> 16;
	unsigned ku  = ((jis >> 8) & 0xff) - 0x21;
	unsigned ten = (jis & 0xff) - 0x21;
	unsigned idx = men * 94*94 + ku * 94 + ten;
	unsigned rc;
 #if 1
	rc = fks_mbc_kutenIdx2uni[idx];
	if (0xDC00 <= rc && rc <= 0xDFFF) {
		rc = fks_mbc_kutenIdx2uniAdd[rc - 0xDC00];
	}
 #else
    rc = kuten2004_to_msUCS2_tbl[idx];
 #endif
	//printf("%d-%02d-%02d %04x\n",men+1,ku+1,ten+1,kuten2004_to_msUCS2_tbl[men * 94*94 + ku * 94 + ten]);
    return rc;
}


/* ----------------------------------------------------------*/

/** 表示するコード体系の種類 */
enum {
    F_JIS=1, F_SJIS=2, F_EUC=4, F_UCS2=8, F_UTF8=16, F_KUTEN=32, F_KUTEN94=64,
    F_JIS_B=128,
};

int wrt_flags = 0;
int wrt_clm = 0;

static void wrt_putNbyte(uint64_t c);

/** 半角か全角(SJIS文字)１文字を標準出力 */
void wrt_putCh(unsigned long jis)
{
	uint64_t c = jis;
    if (wrt_flags & F_SJIS) {
		if (jis != 0x2020)
	    	c = jis2sjis(jis);
    } else if (wrt_flags & F_EUC) {
    	//sjis = jis2eucjp( sjis2jis(sjis) );
    	c = jis2eucjp( jis );
    } else if (wrt_flags & F_UTF8) {
		unsigned uc;
		if (ms932)
			uc = ms932ToUtf32( jis2sjis(jis) );
		else
			uc = jis2utf32( jis );
		if (!uc && wrt_clm <= 1) {
    	    c = 0x236e6f6e65LL; /* none */
		} else {
			if (!uc)
				uc = 0x30fb; /* ・ */
			if (uc & 0x80000000) {
				unsigned d = (uint16_t)uc;
				c = utf32toUtf8( d );
				wrt_putNbyte(c);
				uc = (uc >> 16) & 0x7fff;
			}
			c = utf32toUtf8( uc );
		}
	} else {
		if (jis != 0x2020)
	    	c = jis2sjis(jis);
    }
	wrt_putNbyte(c);
}

void wrt_putNbyte(uint64_t c)
{
	if (c >> 40) {
    	fputc(c >> 40, stdout);
    	fputc((c >> 32) & 0xFF, stdout);
    	fputc((c >> 24) & 0xFF, stdout);
    	fputc((c >> 16) & 0xFF, stdout);
    	fputc((c >> 8) & 0xFF, stdout);
    	fputc(c & 0xFF, stdout);
	} else if (c >> 32) {
    	fputc(c >> 32, stdout);
    	fputc((c >> 24) & 0xFF, stdout);
    	fputc((c >> 16) & 0xFF, stdout);
    	fputc((c >> 8) & 0xFF, stdout);
    	fputc(c & 0xFF, stdout);
    } else if (c >> 24) {
    	fputc(c >> 24, stdout);
    	fputc((c >> 16) & 0xFF, stdout);
    	fputc((c >> 8) & 0xFF, stdout);
    	fputc(c & 0xFF, stdout);
    } else if (c >> 16) {
    	fputc((c >> 16) & 0xFF, stdout);
    	fputc((c >> 8) & 0xFF, stdout);
    	fputc(c & 0xFF, stdout);
    } else if (c >> 8) {
    	fputc((c >> 8) & 0xFF, stdout);
    	fputc(c & 0xFF, stdout);
    } else {
    	fputc(c, stdout);
    }
}


/** 指定個の半角空白を出力 */
static void printSpc(int n)
{
    int i;
    for (i = 0; i < n; i++)
    	printf(" ");
}


/* 文字コード名をflagsにしたがって表示 */
static void printFlags(int flags, int add)
{
    if (add > 0) {
    	printSpc(add);
    }
    if (flags & F_KUTEN) {
		printf("KU-TEN\t");
    }
    if (flags & F_JIS) {
    	printf("JIS\t");
    }
    if (flags & F_SJIS) {
    	printf("SJIS\t");
    }
    if (flags & F_EUC) {
    	printf("EUC\t");
    }
    if (flags & F_UCS2) {
    	printf("UCS2\t");
    }
    if (flags & F_UTF8) {
    	printf("UTF-8\t");
    }
}


/** 頁ごとにつける目盛り */
static void printMemori(int flags, int lc)
{
    int i, n = 0;
    if (flags & F_KUTEN)
    	n += 8;
    if (flags & F_JIS)
    	n += 8;
    if (flags & F_SJIS)
    	n += 8;
    if (flags & F_EUC)
    	n += 8;
    if (flags & F_UCS2)
    	n += 8;
    if (flags & F_UTF8)
    	n += 8;
    n = n + (lc == 94)*3;
	if (lc != 94)
	    printf("#");
    if (lc < 10 || (((flags & F_KUTEN) == 0 || (flags & F_JIS)) && lc <= 16)) { /*一行ですむとき */
    	printFlags(flags, (lc == 94)*3);
    	if (lc > 1) {
    	    for (i = 0; i < lc; i++)
    	    	printf("%2x", i);
    	}
    	printf("\n");
    } else if ((flags & F_KUTEN94) || ((flags & F_KUTEN) && (flags & F_JIS) == 0)) {
    	/* 10進数(句点コード)で表示のとき */
    	printSpc(n);
    	for (i = 1; i <= lc; i++)
    	    printf("%2d", i/10);
    	printf("\n");
    	printFlags(flags, (lc == 94)*3);
    	for (i = 1; i <= lc; i++)
    	    printf("%2d", i%10);
    	printf("\n");
    } else if (lc < 94) {   	    /* 通常 */
    	printSpc(n);
    	for (i = 0; i < lc; i++)
    	    printf("%2x", i>>4);
    	printf("\n");
    	printFlags(flags, (lc == 94)*3);
    	for (i = 0; i < lc; i++)
    	    printf("%2x", i&15);
    	printf("\n");
    } else {	    	    	    /* 94桁での16進数表示専用 */
    	printSpc(n);
    	for (i = 0x21; i <= 0x7e; i++)
    	    printf("%2x", i>>4);
    	printf("\n");
    	printSpc(n);
    	for (i = 0x21; i <= 0x7e; i++)
    	    printf("%2x", i&15);
    	printf("\n");
    }
}


/** 文字コードの表示 */
static void printCode(int jis, int sj, int flags)
{
    int euc, ucs2;
	int men = (jis >> 16) + 1;
    if (jis > 0xff) {
    	/* 2バイト文字のとき */
    	euc = jis2eucjp(jis);
		if (ms932)
			ucs2 = ms932ToUtf32( sj );
		else
			ucs2 = jis2utf32( jis );
    } else {
    	/* 1バイト文字のとき */
    	euc  = jis2eucjp(jis);	//(0xA1 <= jis && jis <= 0xFE) ? (0x8E00|jis) : jis;
    	ucs2 = ubyte_to_msUCS2_tbl[jis];
    }

    if (flags & F_KUTEN) {
    	int ku	= ((jis >> 8) & 0xff) - 0x20;
    	int ten = (jis & 0xff) - 0x20;
   	    printf("%d-%02d-%02d\t", men, ku, ten);
    }
    if (flags & F_JIS) {
		if (flags & F_JIS_B)
	    	printf("%d-%04X\t", men+2, jis & 0xffff);
	    else
	    	printf("%04X\t", jis);
    }
    if (flags & F_SJIS) {
		printf("%04X\t", sj);
    }
    if (flags & F_EUC) {
		printf("%04X\t", euc);
    }
    if (flags & F_UCS2) {
    	if (ucs2 == 0 || ucs2 == 0xFFFF) {
    	    printf("\t");
    	} else {
			if (ucs2 & 0x80000000) {
	    	    printf("U+%04X+%04X\t", (uint16_t)ucs2, (ucs2 >> 16) & 0x7fff);
			} else {
	    	    printf("U+%04X\t", ucs2);
	    	}
    	}
    }
    if (flags & F_UTF8) {
    	if (ucs2 == 0 || ucs2 == 0xFFFF) {
    	    printf("\t");
    	} else {
			uint64_t u = utf32toUtf8(ucs2);
			if (u >> 32) {
	    	    printf("%04x%08x\t", (uint32_t)(u >> 32), (uint32_t)u  );
			} else {
	    	    printf("%06x\t", (uint32_t)u);
	    	}
    	}
    }
}


// 自作変換ルーチン向けの cp932 独自コード
static unsigned convMs932xxxx(unsigned c)
{
	unsigned a;
	if (c < 0x7921 || c > 0xFCFC)
		return c;
	if (0x7921 <= c && c <= 0x7C7E) {
		c = 0x2100 + c - 0x7900;
		c |= 0x20000;
		return c;
	} else if (0x9321 <= c && c <= 0x972c) {
		c = 0x2100 + (c - 0x9300) + 0x400;
		c |= 0x20000;
		return c;
	}
	return c;
}


/** 漢字コード表をテキスト出力 */
static void printKnjTbl(int flags, int lc, int patCh, int pgMode, int memoriFlg)
{
    int m, y, x, jis, sj, euc, ucs2, clm, cc, yend;

	wrt_clm = lc;
    clm = 0;
    if (memoriFlg)
    	printMemori(flags, lc);
    yend = (ms932) ? (0x21 + 120) : 0x7f;
    for (m = 0; m < kutenMen; ++m) {
	    for (y = 0x21; y < yend; y++) {
	    	if (kuten[m * 94 + y-0x21] == 0)     	    	/* 表示しない区分は飛ばす */
	    	    continue;
	    	for (x = 0x21-(patCh!=0); x < 0x7f+(patCh!=0); x++) {
	    	    /* JIS */
	    	    jis = (y<<8) | x;

	    	    if (m)
	    	    	jis |= 0x10000;
	    	    /* Shift JIS */
				if (x == 0x20 || x == 0x7f) {
					if (lc <= 1)
						continue;
					cc = jis + 1;
				} else {
					cc = jis;
				}
	    	    sj = jis2sjis(cc);

	    	    if (clm == 0) { 	    	    	/*左端の文字コードの表示 */
					unsigned cc2;
	    	    	if (lc == 94 && memoriFlg) {	/* 区表示 */
	    	    	    if (flags & F_KUTEN94)
	    	    	    	printf("%2d ", ((jis>>8)&0xff)-0x20);
	    	    	    else
	    	    	    	printf("%2x ", (jis>>8)&0xff);
	    	    	}
	    	    	cc2 = cc;
					if (ms932xxxxFlag)
						cc2 = convMs932xxxx(cc2);
    	    		printCode(cc2, sj, flags);
	    	    }
	    	    if (x >= 0x21 && x <= 0x7E) {   	/* 通常の表示 */
	    	    	wrt_putCh(cc/*sj*/);
	    	    	if (++clm == lc) {
	    	    	    printf("\n");
	    	    	    clm = 0;
	    	    	}
	    	    } else if (patCh) {     	    	/*表示位置調整のための ??20, ??7F 対策 */
	    	    	wrt_putCh(patCh);   /* printf("%c%c", (patCh>>8) & 0xFF, patCh & 0xFF); */
	    	    	if (++clm == lc) {
	    	    	    printf("\n");
	    	    	    clm = 0;
	    	    	}
	    	    }
	    	}
	    	if (pgMode && y < yend - 1) {   	    	/* ページ処理 */
	    	    if ((y % pgMode) == 0) {
	    	    	printf("\n");
	    	    	if (memoriFlg)
	    	    	    printMemori(flags, lc);
	    	    	clm = 0;
	    	    }
	    	}
	    }
	}
    if (clm)
    	printf("\n");
}



/** 文字列の、文字ごとに、文字コードを表示 */
static void printSjisLine(const char *str, int flags, int memoriFlg)
{
    const uint8_t *s = (const uint8_t*)str;
    if (memoriFlg)
    	printMemori(flags, 0);
    for (;;) {
    	int jis, sj;
    	int c = *s++;
    	if (c == 0)
    	    break;
    	if (ISKANJI(c)) {
    	    int d = *s++;
    	    if (d == 0)
    	    	break;
    	    if (ISKANJI2(d)) {
    	    	c = c << 8 | d;
    	    }
    	}
    	sj = jis = c;
    	if (c > 0xff) {
    	    jis = sjis2jis(sj);
    	}
    	printCode(jis, sj, flags);

    	if (sj > 0xff)
    	    fputc(sj>>8, stdout);
    	fputc(sj&0xff, stdout);
    	printf("\n");
    }
}


/** バイナリ出力 */
static void outputKnjTblB(void)
{
    /* DOS系ではstdoutはデフォはテキストだけど、 */
    /* 問題になる改行等のコントロールは出力しないので問題なし */
    int x, y,c,j;
	int yy = (ms932) ? 0x21 + 120 : 0x7f;
	int m;
	for (m = 0; m < kutenMen; ++m) {
	    for (y = 0x21; y < yy; y++) {
	    	if (kuten[m * 94 + y - 0x21] == 0)     	    	/* 表示しない区分は飛ばす */
	    	    continue;
	    	for (x = 0x21; x <= 0x7E; x++) {
	    	    c = (m << 16) | (y<<8) | x;
	    	    //j = jis2sjis(c);
	    	    wrt_putCh(c);
	    	}
	    }
	}
}


static void  setJISX213Nimen() {
	unsigned t = 94 - 1;
	int i;

	kuten[t + 1] = 2;
	kuten[t + 3] = 2;
	kuten[t + 4] = 2;
	kuten[t + 5] = 2;
	kuten[t + 8] = 2;
	kuten[t + 12] = 2;
	kuten[t + 13] = 2;
	kuten[t + 14] = 2;
	kuten[t + 15] = 2;

	for (i = 78; i <= 94; ++i)
		kuten[t + i] = 2;
}

static void  setJISX212Nimen() {
	unsigned t = 94 - 1;
	int i;

	kuten[t + 2] = 2;
	kuten[t + 6] = 2;
	kuten[t + 7] = 2;
	kuten[t + 9] = 2;
	kuten[t + 10] = 2;
	kuten[t + 11] = 2;
	for (i = 16; i <= 77; ++i)
		kuten[t + i] = 2;
}

/** 区分に関するヘルプ表示 */
static void ku_help(void)
{
    typedef struct stbl_t {
    	int start, end;
    	char *msg;
    } stbl_t;
    static stbl_t stbl[] = {
    	{0x2121, 0x227E, "各種記号"},
    	{0x2321, 0x237E, "算用数字、(ラテン)アルファベット"},
    	{0x2421, 0x247E, "ひらがな"},
    	{0x2521, 0x257E, "カタカナ"},
    	{0x2621, 0x267E, "ギリシア文字"},
    	{0x2721, 0x277E, "キリル文字"},
    	{0x2821, 0x287E, "罫線素片"},
    	{0x2921, 0x2c7E, "JIS X 213 非漢字"},
    	{0x2d21, 0x2d7E, "JIS X 213 非漢字 | NEC特殊文字"},
    	{0x2e21, 0x2F7E, "JIS X 213 未定義"},
    	{0x3021, 0x4F7E, "第一水準漢字"},
    	{0x5021, 0x7406, "第二水準漢字"},
    	{0x7407, 0x7E7E, "JIS X 213 第三水準"},
    	{0x7f21, 0x987E, "(SJIS機種依存拡張領域)"},
    	{0x12121,0x1217E, "JIS X 213 第四水準"},
    	{0x12321,0x1257E, "JIS X 213 第四水準"},
    	{0x12821,0x1287E, "JIS X 213 第四水準"},
    	{0x12c21,0x12f7E, "JIS X 213 第四水準"},
    	{0x16e21,0x17E7E, "JIS X 213 第四水準"},
    };
    int i, n, c1,c2;

    printf("JIS漢字(JIS X 208|JIS X 213)の区の分類\n");
    for (i = 0; i < sizeof(stbl)/sizeof(stbl[0]); i++) {
    	c1 = stbl[i].start;
    	c2 = stbl[i].end;
    	n  = (c2>>8) - (c1>>8);
		if (c1 < 0x10000) {
	    	if (n == 0) {
	    	    printf("1面%2d    区", (c1>>8)-0x20);
		    	printf("  3-%x～%x  %x～%x   ", c1, c2, jis2sjis(c1), jis2sjis(c2));
	    	} else {
				if ((c2>>8) <= 0x20+99) {
	    	    	printf("1面%2d～%2d区", (c1>>8)-0x20, (c2>>8)-0x20);
		    		printf("  3-%x～%x  %x～%x   ", c1, c2, jis2sjis(c1), jis2sjis(c2));
		    	} else {
	    	    	printf("1面%2d～%2d区", (c1>>8)-0x20, (c2>>8)-0x20);
		    		printf(" 3-%x～%x  %x～%x   ", c1, c2, jis2sjis(c1), jis2sjis(c2));
				}
	    	}
		} else {
	    	if (n == 0) {
	    	    printf("2面%2d    区", ((c1>>8)&0xff)-0x20);
		    	printf("  4-%x～%x  %x～%x   ", (uint16_t)c1, (uint16_t)c2, jis2sjis(c1), jis2sjis(c2));
	    	} else {
    	    	printf("2面%2d～%2d区", ((c1>>8)&0xff)-0x20, ((c2>>8)&0xff)-0x20);
	    		printf("  4-%x～%x  %x～%x   ", (uint16_t)c1, (uint16_t)c2, jis2sjis(c1), jis2sjis(c2));
			}
		}
    	printf("%s\n", stbl[i].msg);
    }
    printf(
    	"※ 終わりは各区で0x7E(94)未満の場合もあるが、一律 ??7Eにしている\n"
    );
    exit(1);
}



int main(int argc, char *argv[])
{
    char    *p;
    int     i,c,d,k;
    int     lc = 16, patCh=0x2020, flags = 0;
    int     binFlg=0, pgMode=0, eucFlg, memoriFlg=0, sj = 0;

    if (argc < 2)
    	Usage();

    memset(kuten, 0, sizeof(kuten));
    for (i = 1; i < argc; i++) {
    	p = argv[i];
    	if (*p == '-') {
    	    p++;
    	    c = *p++;
    	    switch (c) {
    	    case 'b':
    	    	binFlg = (*p != '-' && *p != '0');
    	    	break;
    	    case 'c':
    	    	c = *(uint8_t*)p;
    	    	++p;
    	    	if (c == 0 || c == '0')
    	    	    patCh = 0;
    	    	else if (c == '1')
    	    	    patCh = 0x2020;
    	    	else if (c == '2')
    	    	    patCh = 0x2121; //0x8140;
    	    	//else if (ispunct(c))
    	    	//    patCh = (c<<8)|c;
    	    	//else if (ISKANJI(c))
    	    	//    patCh = (c<<8)|*p;
    	    	else
    	    	    goto OPT_ERR;
    	    	break;
    	    case 'l':
    	    	lc = strtol(p,NULL,10);
    	    	if (lc <= 0 || lc > (0x7f-0x21))
    	    	    goto OPT_ERR;
    	    	patCh = 0;
    	    	break;
    	    case 'm':
    	    	memoriFlg = (*p != '-' && *p != '0');
    	    	break;
    	    case 'p':
    	    	if (*p == 0)
    	    	    pgMode = 1;
    	    	else
    	    	    pgMode = strtol(p,0,10);
    	    	break;
    	    case 't':
    	    	if (*p == '-') {
					flags = 0;
				} else if (isdigit(*p)) {
    	    	    c = strtol(p, 0, 0);
    	    	    if (c < 0 || c > 63)
    	    	    	goto OPT_ERR;
    	    	    if (c == 0)
    	    	    	flags = 0;
    	    	    else
    	    	    	flags |= c;
    	    	} else if (*p) {
    	    	    while (*p) {
    	    	    	c = *p++;
    	    	    	if (c == 0)
    	    	    	    break;
    	    	    	switch (c) {
    	    	    	case 'k':   flags |= F_KUTEN;	break;
    	    	    	case 'j':   flags |= F_JIS; 	break;
    	    	    	case 'J':   flags |= F_JIS|F_JIS_B; 	break;
    	    	    	case 's':   flags |= F_SJIS;	break;
    	    	    	case 'e':   flags |= F_EUC; 	break;
    	    	    	case 'u':   flags |= F_UCS2;	break;
    	    	    	case 'o':   flags |= F_UTF8;	break;
    	    	    	default:
    	    	    	    goto OPT_ERR;
    	    	    	}
    	    	    }
    	    	} else {
    	    	    flags = 0;
    	    	}
    	    	break;
    	    case 'w':
    	    	while (*p) {
    	    	    c = *p++;
    	    	    if (c == 0)
    	    	    	break;
    	    	    switch (c) {
    	    	    /* case 'j':wrt_flags = F_JIS;  break; */
    	    	    case 's':	wrt_flags = F_SJIS; break;
    	    	    case 'e':	wrt_flags = F_EUC;  break;
    	    	    /* case 'u':wrt_flags = F_UCS2; break; */
    	    	    case 'o':	wrt_flags = F_UTF8; break;
    	    	    default:
    	    	    	goto OPT_ERR;
    	    	    }
    	    	}
    	    	break;
    	    case 'k':
    	    	if (*p == 0)
    	    	    ku_help();
    	    	kutenMen = 1;
    	    	if (*p == 'b')
    	    		kutenMen = 2;
    	    	d = c = strtol(p,&p,10);
    	    	if (c < 1 || c > 120/*94*/)
    	    	    goto OPT_ERR;
    	    	if (*p)
    	    	    d = strtol(p+1,0,10);
    	    	if (d < c)
    	    	    goto OPT_ERR;
    	    	for (k = c; k <= d; k++)
    	    	    kuten[(kutenMen-1)*94 + k-1] = 1;
    	    	break;
    	    case '0':
    	    	kutenMen = 1;
    	    	d = (*p == 'x') ? 15 : 8;
    	    	for (k = 1; k <= d; k++)
    	    	    kuten[k-1] = 1;
    	    	break;
    	    case '1':
    	    	kutenMen = 1;
    	    	for (k = 16; k <= 47; k++)
    	    	    kuten[k-1] = 1;
    	    	break;
    	    case '2':
    	    	kutenMen = 1;
    	    	for (k = 48; k <= 84; k++)
    	    	    kuten[k-1] = 1;
    	    	break;
    	    case '3':
    	    	kutenMen = 1;
    	    	for (k = 85; k <= 94; k++)
    	    	    kuten[k-1] = 1;
    	    	break;
    	    case '4':
    	    	kutenMen = 2;
				setJISX213Nimen();
    	    	break;
    	    case 'X':
    	    	//kutenMen = 0;
    	    	if (strcmp(p,"212") == 0) {
	    	    	kutenMen = 2;
					setJISX212Nimen();
				}
				break;
			case 'x':
    	    	ms932 = 1;
				if (*p == 'V') {	// 自作変換処理向けに、区点|JIS表記において、1-89-01以降を2-01-01|5-2121のように2面(group5)に区単位で隙間を詰めて並べて出力.
					ms932xxxxFlag = 1;
				}
			 #ifdef USE_WIN_API
				if (strcmp(p,"api") == 0)
					useWinApi = 1;
			 #endif
    	    	for (k = 95; k <= 120; k++)
    	    	    kuten[k-1] = 1;
    	    	break;
    	    case 'j':
    	    	c = strtol(p,0,16);
    	    	c &= 0x7f7f;
    	    	if (0x2121 <= c && c <= 0x7e7e) {
    	    	    k = jis2sjis(c);
    	    	}
    	    	printf("%x->%x(%c%c)\n", c, k, k>>8,k&0xff);
    	    	return 0;
    	    case '\0':
    	    	break;
    	    case '?':
    	    	Usage();
    	    default:
    	  OPT_ERR:
    	    	printf("Incorrect command line option : %s\n", argv[i]);
    	    	return 1;
    	    }
    	} else {
    	    /* 何かsjis文字列があれば、それの文字コードを表示 */
    	    if (flags == 0)
    	    	flags = 63;
    	    printSjisLine(p, flags, memoriFlg);
    	    sj = 1;
    	}
    }

    if (sj)
    	return 0;

    if (lc >= 94 && (flags & F_KUTEN)) {
    	flags &= ~F_KUTEN;
    	flags |= F_KUTEN94;
    }

    if (kutenMen == 0) {    /* 区の指定がなければすべて表示 */
		if (ms932) {
			kutenMen = 1;
	    	for (k = 1; k <= 120; k++)
	    	    kuten[k-1] = 1;
	   	} else {
			kutenMen = 2;
	    	for (k = 1; k <= 94; k++)
	    	    kuten[k-1] = 1;
			setJISX213Nimen();
		}
    }
    if (binFlg == 0)
    	printKnjTbl(flags, lc, patCh, pgMode, memoriFlg);
    else
    	outputKnjTblB();
    return 0;
}
