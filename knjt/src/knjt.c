/*
    knjt    	出直し版 v2.00
    履歴
    198?    os9/09 6809のアセンブラで。一水のみだったような気もする。
    	    どのような仕様だったかもう覚えていない^^;
    199?    ANSI-C(MS-DOS)  行方不明。
    2000/10 v1.00 行方不明ゆえ作り直し。
    	    でもって、いろいろごちゃごちゃ機能つける.
    	    v1.01 -pN のNページごとを付加
    2004/01 v2.00 unicode表示を付加. eucや句点の扱いを変更. -tの仕様変更
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef unsigned char	UCHAR;

#define ISKANJI(c)  	((UCHAR)(c) >= 0x81 && ((UCHAR)(c) <= 0x9F || ((UCHAR)(c) >= 0xE0 && (UCHAR)(c) <= 0xFC)))
#define ISKANJI2(c) 	((UCHAR)(c) >= 0x40 && (UCHAR)(c) <= 0xfc && (c) != 0x7f)
#define JIS2NO(j)   	(((((j) >> 8) & 0x7f) - 0x21) * 94 + (((j) & 0x7f) - 0x21))


/** 8bit数二つを上下につなげて16ビット数にする */
#define BB(a,b)     	((((UCHAR)(a))<<8)|(UCHAR)(b))

/** 8bit数4つを上位から順につなげて32ビット数にする */
#define BBBB(a,b,c,d)	((((UCHAR)(a))<<24)|(((UCHAR)(b))<<16)|(((UCHAR)(c))<<8)|((UCHAR)(d)))

/** 8bit数6つを... 64bit整数用となるので、とりあえず、現状はダミー */
#define BBBBBB(a,b,c,d,e,f) 	(0xFFFFFFFF)

static char kubun[94];
extern unsigned short jisno_to_msUCS2_tbl[];
extern unsigned short ubyte_to_msUCS2_tbl[];


/** 使い方 */
static void Usage(void)
{
    printf(
    	"usage>knjt [-opts]       // v2.00 " __DATE__ "  by tenk*\n"
    	"    シフトJIS文字で全角文字一覧のテキストを生成する\n"
    	"  -opts:  （デフォルトは -l16 -c1 -t0）\n"
    	"    -w[seo]    結果を s:SJIS e:EUC o:UTF8 にして出力\n"
    	"    -t[kjseuo] 各行の先頭に文字コードを付加. -t0だと付加しない\n"
    	"               k:句点 j:JIS s:SJIS e:EUC u:UCS2 o:UTF8 を付加.(組合わせる)\n"
    	"    -k         区の分類の表示(ヘルプ)\n"
    	"    -k[N:M]    表示する区の範囲を指定. 1～94. 複数指定可能\n"
    	"    -0         記号や英数カナ等.    -k1:8  に同じ\n"
    	"    -1         漢字第一水準の指定. -k16:47 に同じ\n"
    	"    -2         漢字第ニ水準の指定. -k48:84 に同じ\n"
    	"    -l[N]      1行に表示する文字数. 1～94(デフォルト 16)\n"
    	"               同時に -c0 が指定されたことになる.\n"
    	"    -cN        JISコード換算での??20,??7fを\n"
    	"               0:表示しない    1:半角空白２つ'  '(デフォルト)\n"
    	"               2:全角空白一つ'　'\n"
    	"               半角記号:それを二つ  全角文字:それを一つ\n"
    	"    -m         目盛りを付加\n"
    	"    -p[N]      区ごとに空行(+目盛り)を挿入.(Nが2以上ならN区ごとに挿入)\n"
    	"    -b         テキストでなく、文字コードのみのバイナリ出力\n"
    	" ※ ??20, ??7F のシフトJIS表示の値はウソになります(次の文字の値です)\n"
    	/*
    	//  "	 -t[N]	    各行の先頭に文字コードを付加. 0:何も追加しない\n"
    	//  "	    	    1:JIS 2:SJIS 4:EUC 8:Unicode(UCS2) 16:UTF8	32:句点\n"
    	//  "	    	    足し算して組み合わせて指定可.\n"
    	*/
    );
    exit(1);
}


/* ----------------------------------------------------------*/


/** JIS を EUC に変換 */
#define jis2euc(jis)	((jis) | 0x8080)


/** jis コードを シフトjisに変換 */
static int jis2sjis(int c)
{
    c -= 0x2121;
    if (c & 0x100)
    	c += 0x9e;
    else
    	c += 0x40;
    if ((UCHAR)c >= 0x7f)
    	c++;
    c = (((c >> (8+1)) + 0x81)<<8) | ((UCHAR)c);
    if (c >= 0xA000)
    	c += 0x4000;
    return c;
}


/** シフトjis を jis に変換 */
int sjis2jis(int c)
{
    if (c >= 0xE000)
    	c -= 0x4000;
    c = (((c>>8) - 0x81)<<9) | (UCHAR)c;
    if ((UCHAR)c >= 0x80)
    	c -= 1;
    if ((UCHAR)c >= 0x9e)
    	c += 0x62;
    else
    	c -= 0x40;
    c += 0x2121;
    return c;
}


/** JIS を MS-UCS2 に変換 */
unsigned jis2ucs2(int jis)
{
    return jisno_to_msUCS2_tbl[JIS2NO(jis)];
}


/** UCS2 を utf8 に変換. ※utf16には未対応 */
unsigned ucs2utf8(int c)
{
    if (c <= 0x7F) {
    	return c;
    } else if (c <= 0x7FF) {
    	return BB(0xC0|(c>>6), 0x80|(c&0x3f));
    } else if (c <= 0xFFFF) {
    	return BBBB(0, 0xE0|(c>>12), 0x80|(c>>6)&0x3f, 0x80|(c&0x3f));
    } else if (c <= 0x1fFFFF) {
    	return BBBB(0xF0|(c>>18), 0x80|(c>>12)&0x3f, 0x80|(c>>6)&0x3f, 0x80|(c&0x3f));
    } else if (c <= 0x3fffFFFF) {
    	/* JIS範囲文字は3バイトで収まるので、このあたりは使わない...のでBBBBBBダミーにして、この値も偽者 */
    	return BBBBBB(00, 0xF8|(c>>24), 0x80|(c>>18)&0x3f, 0x80|(c>>12)&0x3f,0x80|(c>>6)&0x3f,0x80|(c&0x3f));
    } else {
    	/* JIS範囲文字は3バイトで収まるので、このあたりは使わない...のでBBBBBBダミーにして、この値も偽者 */
    	return BBBBBB(0xFC|(c>>30), 0x80|(c>>24)&0x3f, 0x80|(c>>18)&0x3f, 0x80|(c>>12)&0x3f, 0x80|(c>>6)&0x3f, 0x80|(c&0x3f));
    }
}



/* ----------------------------------------------------------*/

/** 表示するコード体系の種類 */
enum {
    F_JIS=1, F_SJIS=2, F_EUC=4, F_UCS2=8, F_UTF8=16, F_KUTEN=32, F_KUTEN94=64,
};

int wrt_flags = 0;


/** 半角か全角(SJIS文字)１文字を標準出力 */
void wrt_putCh(unsigned sjis)
{
    if (wrt_flags & F_SJIS) {
    	;
    } else if (wrt_flags & F_EUC) {
    	sjis = jis2euc( sjis2jis(sjis) );
  #if 0
    } else if (wrt_flags & F_UCS16) {
    	unsigned ucs2 = jis2ucs2( sjis2jis(sjis) );
    	if (ucs2 == 0xffff)
    	    ucs2 = 0x30FB;  /* ・ のUCS2コード. 仮文字として */
    	fputc(ucs2 & 0xFF, stdout);
    	fputc((ucs2 >> 8) & 0xFF, stdout);
    	return;
  #endif
    } else if (wrt_flags & F_UTF8) {
    	unsigned ucs2 = jis2ucs2( sjis2jis(sjis) );
    	if (ucs2 == 0xffff)
    	    ucs2 = 0x30FB;  /* ・ のUCS2コード. 仮文字として */
    	sjis = ucs2utf8(ucs2);
    }

    if (sjis >> 24) {
    	fputc(sjis >> 24, stdout);
    	fputc((sjis >> 16) & 0xFF, stdout);
    	fputc((sjis >> 8) & 0xFF, stdout);
    	fputc(sjis & 0xFF, stdout);
    } else if (sjis >> 16) {
    	fputc((sjis >> 16) & 0xFF, stdout);
    	fputc((sjis >> 8) & 0xFF, stdout);
    	fputc(sjis & 0xFF, stdout);
    } else if (sjis >> 8) {
    	fputc((sjis >> 8) & 0xFF, stdout);
    	fputc(sjis & 0xFF, stdout);
    } else {
    	fputc(sjis, stdout);
    }
}


/** 指定個の半角空白を出力 */
static void printSpc(int n)
{
  #if 0
    printf("%*c", n, ' ');
  #else /* printfが古い仕様のままのとき */
    int i;
    for (i = 0; i < n; i++)
    	printf(" ");
  #endif
}


/* 文字コード名をflagsにしたがって表示 */
static void printFlags(int flags, int add)
{
    if (add > 0) {
    	printSpc(add);
    }
    if (flags & F_KUTEN) {
    	/* printf("句 点 "); */
    	wrt_putCh(0x8be5);
    	wrt_putCh(' ');
    	wrt_putCh(0x935f);
    	wrt_putCh(' ');
    }
    if (flags & F_JIS) {
    	printf("JIS  ");
    }
    if (flags & F_SJIS) {
    	printf("SJIS ");
    }
    if (flags & F_EUC) {
    	printf("EUC  ");
    }
    if (flags & F_UCS2) {
    	printf("UCS2 ");
    }
    if (flags & F_UTF8) {
    	printf("UTF-8  ");
    }
}


/** 頁ごとにつける目盛り */
static void printMemori(int flags, int lc)
{
    int i, n = 0;
    if (flags & F_KUTEN)
    	n += 6;
    if (flags & F_JIS)
    	n += 5;
    if (flags & F_SJIS)
    	n += 5;
    if (flags & F_EUC)
    	n += 5;
    if (flags & F_UCS2)
    	n += 5;
    if (flags & F_UTF8)
    	n += 7;
    n = n + (lc == 94)*3;
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
    if (jis > 0xff) {
    	/* 2バイト文字のとき */
    	euc = jis2euc(jis);
    	ucs2 = jisno_to_msUCS2_tbl[JIS2NO(jis)];
    } else {
    	/* 1バイト文字のとき */
    	euc  = (0xA1 <= jis && jis <= 0xFE) ? (0x8E00|jis) : jis;
    	ucs2 = ubyte_to_msUCS2_tbl[jis];
    }

    if (flags & F_KUTEN) {
    	int ku	= (jis >> 8  ) - 0x20;
    	int ten = (jis & 0xff) - 0x20;
    	if (ku < 0) {
    	    printf("%5d ", jis);
    	} else {
    	    printf("%2d,%2d ", ku, ten);
    	}
    }
    if (flags & F_JIS) {
    	printf("%04x ", jis);
    }
    if (flags & F_SJIS) {
    	printf("%04x ", sj);
    }
    if (flags & F_EUC) {
    	printf("%04x ", euc);
    }
    if (flags & F_UCS2) {
    	if (ucs2 == 0xFFFF)
    	    printf(".... ");
    	else
    	    printf("%04x ", ucs2);
    }
    if (flags & F_UTF8) {
    	if (ucs2 == 0xFFFF)
    	    printf("...... ");
    	else
    	    printf("%06x ", (unsigned)ucs2utf8(ucs2));
    }
}



/** 漢字コード表をテキスト出力 */
static void printKnjTbl(int flags, int lc, int patCh, int pgMode, int memoriFlg)
{
    int y, x, jis, sj, euc, ucs2, clm, cc;

    clm = 0;
    if (memoriFlg)
    	printMemori(flags, lc);
    for (y = 0x21; y < 0x7F; y++) {
    	if (kubun[y-0x21] == 0)     	    	/* 表示しない区分は飛ばす */
    	    continue;
    	for (x = 0x21-(patCh!=0); x < 0x7f+(patCh!=0); x++) {
    	    /* JIS */
    	    jis = (y<<8) | x;

    	    /* Shift JIS */
    	    if (x == 0x20)
    	    	cc = jis + 1;
    	    else if (x == 0x7f)
    	    	cc = jis + 0x100;
    	    else
    	    	cc = jis;
    	    sj = jis2sjis(cc);

    	    if (clm == 0) { 	    	    	/*左端の文字コードの表示 */
    	    	if (lc == 94 && memoriFlg) {	/* 区表示 */
    	    	    if (flags & F_KUTEN94)
    	    	    	printf("%2d ", (jis>>8)-0x20);
    	    	    else
    	    	    	printf("%2x ", jis>>8);
    	    	}
    	    	printCode(jis, sj, flags);
    	    }
    	    if (x >= 0x21 && x <= 0x7E) {   	/* 通常の表示 */
    	    	wrt_putCh(sj);	    /* fputc(sj>>8, stdout);fputc(sj&0xff, stdout); */
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
    	if (pgMode && y < 0x7e) {   	    	/* ページ処理 */
    	    if ((y % pgMode) == 0) {
    	    	printf("\n");
    	    	if (memoriFlg)
    	    	    printMemori(flags, lc);
    	    	clm = 0;
    	    }
    	}
    }
    if (clm)
    	printf("\n");
}



/** 文字列の、文字ごとに、文字コードを表示 */
static void printSjisLine(const char *str, int flags, int memoriFlg)
{
    const UCHAR *s = (const UCHAR*)str;
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

    for (y = 0x21; y < 0x7F; y++) {
    	if (kubun[y-0x21] == 0)     	    	/* 表示しない区分は飛ばす */
    	    continue;
    	for (x = 0x21; x <= 0x7E; x++) {
    	    c = (y<<8) | x;
    	    j = jis2sjis(c);
    	    wrt_putCh(j);
    	}
    }
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
    	{0x2921, 0x2F7E, "JIS X 208 で未定義"},
    	{0x3021, 0x4F7E, "第一水準漢字"},
    	{0x5021, 0x747E, "第二水準漢字"},
    	{0x7521, 0x7E7E, "JIS X 208 で未定義"},
    };
    int i, n, c1,c2;

    printf("JIS漢字(JIS X 208)の区の分類\n");
    for (i = 0; i < sizeof(stbl)/sizeof(stbl[0]); i++) {
    	c1 = stbl[i].start;
    	c2 = stbl[i].end;
    	n  = (c2>>8) - (c1>>8);
    	if (n == 0) {
    	    printf("%2d    区", (c1>>8)-0x20);
    	} else {
    	    printf("%2d～%2d区", (c1>>8)-0x20, (c2>>8)-0x20);
    	}
    	printf("  %x～%x  %x～%x   ", c1, c2, jis2sjis(c1), jis2sjis(c2));
    	printf("%s\n", stbl[i].msg);
    }
    printf(
    	"※ 終わりは各区で0x7E(94)未満の場合もあるが、一律 ??7Eにしている\n"
    	"※ JIS X 208 での未定義エリアには機種依存文字や\n"
    	"   その後の規格で拡張された文字が入っている場合がある。\n"
    );
    exit(1);
}



int main(int argc, char *argv[])
{
    char    *p;
    int     i,c,d,k;
    int     lc = 16, patCh=0x2020, flags = 0, kubunFlg=0;
    int     binFlg=0, pgMode=0, eucFlg, memoriFlg=0, sj = 0;

    if (argc < 2)
    	Usage();

    memset(kubun, 0, sizeof(kubun));
    for (i = 1; i < argc; i++) {
    	p = argv[i];
    	if (*p == '-') {
    	    p++;
    	    c = *p++;
    	    c = toupper(c);
    	    switch (c) {
    	    case 'B':
    	    	binFlg = (*p != '-' && *p != '0');
    	    	break;
    	    case 'C':
    	    	c = (UCHAR)*p++;
    	    	if (c == 0 || c == '0')
    	    	    patCh = 0;
    	    	else if (c == '1')
    	    	    patCh = 0x2020;
    	    	else if (c == '2')
    	    	    patCh = 0x8140;
    	    	else if (ispunct(c))
    	    	    patCh = (c<<8)|c;
    	    	else if (ISKANJI(c))
    	    	    patCh = (c<<8)|*p;
    	    	else
    	    	    goto OPT_ERR;
    	    	break;
    	    case 'L':
    	    	lc = strtol(p,NULL,10);
    	    	if (lc <= 0 || lc > (0x7f-0x21))
    	    	    goto OPT_ERR;
    	    	patCh = 0;
    	    	break;
    	    case 'M':
    	    	memoriFlg = (*p != '-' && *p != '0');
    	    	break;
    	    case 'P':
    	    	if (*p == 0)
    	    	    pgMode = 1;
    	    	else
    	    	    pgMode = strtol(p,0,10);
    	    	break;
    	    case 'T':
    	    	if (isdigit(*p)) {
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
    	    	    	c = toupper(c);
    	    	    	switch (c) {
    	    	    	case 'K':   flags |= F_KUTEN;	break;
    	    	    	case 'J':   flags |= F_JIS; 	break;
    	    	    	case 'S':   flags |= F_SJIS;	break;
    	    	    	case 'E':   flags |= F_EUC; 	break;
    	    	    	case 'U':   flags |= F_UCS2;	break;
    	    	    	case 'O':   flags |= F_UTF8;	break;
    	    	    	default:
    	    	    	    goto OPT_ERR;
    	    	    	}
    	    	    }
    	    	} else {
    	    	    flags = 0;
    	    	}
    	    	break;
    	    case 'W':
    	    	while (*p) {
    	    	    c = *p++;
    	    	    if (c == 0)
    	    	    	break;
    	    	    c = toupper(c);
    	    	    switch (c) {
    	    	    /* case 'K':wrt_flags =F_KUTEN; break; */
    	    	    /* case 'J':wrt_flags = F_JIS;  break; */
    	    	    case 'S':	wrt_flags = F_SJIS; break;
    	    	    case 'E':	wrt_flags = F_EUC;  break;
    	    	    /* case 'U':wrt_flags = F_UCS2; break; */
    	    	    case 'O':	wrt_flags = F_UTF8; break;
    	    	    default:
    	    	    	goto OPT_ERR;
    	    	    }
    	    	}
    	    	break;
    	    case 'K':
    	    	if (*p == 0)
    	    	    ku_help();
    	    	kubunFlg = 1;
    	    	d = c = strtol(p,&p,10);
    	    	if (c < 1 || c > 94)
    	    	    goto OPT_ERR;
    	    	if (*p)
    	    	    d = strtol(p+1,0,10);
    	    	if (d < c)
    	    	    goto OPT_ERR;
    	    	for (k = c; k <= d; k++)
    	    	    kubun[k-1] = 1;
    	    	break;
    	    case '0':
    	    	kubunFlg = 1;
    	    	for (k = 1; k <= 8; k++)
    	    	    kubun[k-1] = 1;
    	    	break;
    	    case '1':
    	    	kubunFlg = 1;
    	    	for (k = 16; k <= 47; k++)
    	    	    kubun[k-1] = 1;
    	    	break;
    	    case '2':
    	    	kubunFlg = 1;
    	    	for (k = 48; k <= 84; k++)
    	    	    kubun[k-1] = 1;
    	    	break;
    	    case 'J':
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

    if (kubunFlg == 0) {    /* 区の指定がなければすべて表示 */
    	for (k = 1; k <= 94; k++)
    	    kubun[k-1] = 1;
    }
    if (binFlg == 0)
    	printKnjTbl(flags, lc, patCh, pgMode, memoriFlg);
    else
    	outputKnjTblB();
    return 0;
}

