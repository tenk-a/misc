/*
    knjt    	v3.00
    ����
    198?    os9/09 6809�̃A�Z���u���ŁB�ꐅ�݂̂������悤�ȋC������B
    	    �ǂ̂悤�Ȏd�l�������������o���Ă��Ȃ�^^;
    199?    ANSI-C(MS-DOS)  �s���s���B
    2000/10 v1.00 �s���s���䂦��蒼���B
    	    �ł����āA���낢�낲���Ⴒ����@�\����.
    	    v1.01 -pN ��N�y�[�W���Ƃ�t��
    2004/01 v2.00 unicode�\����t��. euc���_�̈�����ύX. -t�̎d�l�ύX
    2018/12 v3.00 JIS2004(JIS X 213) �x�[�X�ɂ��A�I�v�V������ MS-CP932 �ɑΉ�.(jis-unicode�ϊ���MS�ˑ�)

    ���C�Z���X�� Boost software license Version 1.0
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


/** 8bit������㉺�ɂȂ���16�r�b�g���ɂ��� */
#define BB(a,b)     	((((uint8_t)(a))<<8)|(uint8_t)(b))

/** 8bit��4����ʂ��珇�ɂȂ���32�r�b�g���ɂ��� */
#define BBBB(a,b,c,d)	((((uint8_t)(a))<<24)|(((uint8_t)(b))<<16)|(((uint8_t)(c))<<8)|((uint8_t)(d)))

/** 8bit��6��... 64bit�����p */
#define BBBBBB(a,b,c,d,e,f) (((uint64_t)((uint8_t)(a))<<40)|((uint64_t)((uint8_t)(b))<<32)|(((uint8_t)(c))<<24)|(((uint8_t)(d))<<16)|(((uint8_t)(e))<<8)|((uint8_t)(f)))

static char kubun[94 * 2 + 2];
static int  kubunMen = 0;
static char ms932 = 0;

extern unsigned 		kuten2004_to_msUCS2_tbl[];
extern unsigned 		kuten_to_msUCS2_tbl[];
extern unsigned short	ubyte_to_msUCS2_tbl[];


/** �g���� */
static void Usage(void)
{
    printf(
    	"usage>knjt [-opts]       // v3.00 " __DATE__ "\n"
    	"    https://github.com/tenk-a/misc/tree/master/knjt\n"
    	"    �S�p�����ꗗ�̃e�L�X�g�𐶐�����.\n"
    	"  -opts:  �i�f�t�H���g�� -l16 -c1 -t0�j\n"
    	"    -w[seo]    ���ʂ� s:SJIS e:EUC o:UTF8 �ɂ��ďo��\n"
    	"    -t[kjseuo] �e�s�̐擪�ɕ����R�[�h��t��. -t0���ƕt�����Ȃ�\n"
    	"               k:��_ j:JIS s:SJIS e:EUC u:UTF32 o:UTF8 ��t��.(�g���킹��)\n"
    	"    -x         SJIS �� cp932 �Ƃ��A�g��95-120����Ώ�.\n"
    	"    -k         ��̕��ނ̕\��(�w���v)\n"
    	"    -k[N:M]    �\�������͈̔͂��w��. 1�`94. �����w��\\n"
    	"    -kb[N:M]   2�ʂ̋�w��. 1�`94.\n"
    	"    -0         �L����p���J�i��.    -k1:8  �ɓ���\n"
    	"    -0x        1-15��w��\n"
    	"    -1         ������ꐅ���̎w��. -k16:47 �ɓ���\n"
    	"    -2         ������j�����̎w��. -k48:84 �ɓ���\n"
    	"    -3         JIS X 213 ������O�����̎w��. -k84:94 �ɓ���\n"
    	"    -4         JIS X 213 ������l�����̎w��. 2�ʂ�1,3-5,8,12-15,78-94��w��\n"
    	"    -l[N]      1�s�ɕ\�����镶����. 1�`94(�f�t�H���g 16)\n"
    	"               ������ -c0 ���w�肳�ꂽ���ƂɂȂ�.\n"
    	//"    -cN        JIS�R�[�h���Z�ł�??20,??7f��\n"
    	//"               0:�\�����Ȃ�    1:���p�󔒂Q��'  '(�f�t�H���g)\n"
    	//"               2:�S�p�󔒈��'�@'\n"
    	"    -m         �ڐ����t��\n"
    	"    -p[N]      �悲�Ƃɋ�s(+�ڐ���)��}��.(N��2�ȏ�Ȃ�N�悲�Ƃɑ}��)\n"
    	"    -b         �e�L�X�g�łȂ��A�����R�[�h�݂̂̃o�C�i���o��\n"
    	"\n"
    	"   SJIS �� JIS2004�x�[�X���� 1-88-94(JIS:787E,SJIS:ECFC)�ȉ���cp932��\n"
    	"   ���݂���(�ގ������Ⴄ)������cp932�ɍ��킹�Ă���B\n"
    	"   1-89-01(JIS:7921,SJIS:ED40)�ȍ~�ɂ��Ă�MS-CP932�Ɣ�݊��ɂȂ�.\n"
    	"�ϊ���:\n"
    	" > knjt -tkjsu -l1 -wo -m\n"
    	"   SJIS(2004)�̕\��'Unicode mapping table'�ɋ߂��`���� utf8 �e�L�X�g�ŏo��\n"
    	" > knjt -tkjsu -l1 -wo -m -x\n"
    	"   MS-CP932�̕\��'Unicode mapping table'�ɋ߂��`���� utf8 �e�L�X�g�ŏo��\n"
    	"\n"
    );
    exit(1);
}


/* ----------------------------------------------------------*/


/** JIS �� EUC �ɕϊ� */
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

/** jis �R�[�h�� �V�t�gjis�ɕϊ� */
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


/** �V�t�gjis �� jis �ɕϊ� */
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

/** UCS2 �� utf8 �ɕϊ�.*/
unsigned utf32toUtf8(unsigned c)
{
    if (c < 0x80) {
    	return c;
    } else if (c <= 0x7FF) {
    	return BB(0xC0|(c>>6), 0x80|(c&0x3f));
    } else if (c <= 0xFFFF) {
    	return BBBB(0, 0xE0|(c>>12), 0x80|(c>>6)&0x3f, 0x80|(c&0x3f));
    } else if (c <= 0x1fFFFF) {
    	return BBBB(0xF0|(c>>18), 0x80|(c>>12)&0x3f, 0x80|(c>>6)&0x3f, 0x80|(c&0x3f));
    } else if (c <= 0x3fffFFFF) {
    	return BBBBBB(00, 0xF8|(c>>24), 0x80|(c>>18)&0x3f, 0x80|(c>>12)&0x3f,0x80|(c>>6)&0x3f,0x80|(c&0x3f));
    } else {
    	return BBBBBB(0xFC|(c>>30), 0x80|(c>>24)&0x3f, 0x80|(c>>18)&0x3f, 0x80|(c>>12)&0x3f, 0x80|(c>>6)&0x3f, 0x80|(c&0x3f));
    }
}

#ifdef USE_WIN_API
unsigned ms932ToUtf32(int sjis) {
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
#else
unsigned ms932ToUtf32(int jis)
{
	unsigned ku  = ((jis >> 8) - 0x21);
	unsigned ten = (jis & 0xff) - 0x21;
    return kuten_to_msUCS2_tbl[ku * 94 + ten];
}
#endif

/** JIS2004 �� MS-UCS2 �ɕϊ� */
unsigned jis2utf32(int jis)
{
	unsigned men = jis >> 16;
	unsigned ku  = ((jis >> 8) & 0xff) - 0x21;
	unsigned ten = (jis & 0xff) - 0x21;
	//printf("%d-%02d-%02d %04x\n",men+1,ku+1,ten+1,kuten2004_to_msUCS2_tbl[men * 94*94 + ku * 94 + ten]);
    return kuten2004_to_msUCS2_tbl[men * 94*94 + ku * 94 + ten];
}


/* ----------------------------------------------------------*/

/** �\������R�[�h�̌n�̎�� */
enum {
    F_JIS=1, F_SJIS=2, F_EUC=4, F_UCS2=8, F_UTF8=16, F_KUTEN=32, F_KUTEN94=64,
};

int wrt_flags = 0;
int wrt_clm = 0;

static void wrt_putNbyte(uint64_t c);

/** ���p���S�p(SJIS����)�P������W���o�� */
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
				uc = 0x30fb; /* �E */
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


/** �w��̔��p�󔒂��o�� */
static void printSpc(int n)
{
    int i;
    for (i = 0; i < n; i++)
    	printf(" ");
}


/* �����R�[�h����flags�ɂ��������ĕ\�� */
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


/** �ł��Ƃɂ���ڐ��� */
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
    printf("#");
    if (lc < 10 || (((flags & F_KUTEN) == 0 || (flags & F_JIS)) && lc <= 16)) { /*��s�ł��ނƂ� */
    	printFlags(flags, (lc == 94)*3);
    	if (lc > 1) {
    	    for (i = 0; i < lc; i++)
    	    	printf("%2x", i);
    	}
    	printf("\n");
    } else if ((flags & F_KUTEN94) || ((flags & F_KUTEN) && (flags & F_JIS) == 0)) {
    	/* 10�i��(��_�R�[�h)�ŕ\���̂Ƃ� */
    	printSpc(n);
    	for (i = 1; i <= lc; i++)
    	    printf("%2d", i/10);
    	printf("\n");
    	printFlags(flags, (lc == 94)*3);
    	for (i = 1; i <= lc; i++)
    	    printf("%2d", i%10);
    	printf("\n");
    } else if (lc < 94) {   	    /* �ʏ� */
    	printSpc(n);
    	for (i = 0; i < lc; i++)
    	    printf("%2x", i>>4);
    	printf("\n");
    	printFlags(flags, (lc == 94)*3);
    	for (i = 0; i < lc; i++)
    	    printf("%2x", i&15);
    	printf("\n");
    } else {	    	    	    /* 94���ł�16�i���\����p */
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


/** �����R�[�h�̕\�� */
static void printCode(int jis, int sj, int flags)
{
    int euc, ucs2;
	int men = (jis >> 16) + 1;
    if (jis > 0xff) {
    	/* 2�o�C�g�����̂Ƃ� */
    	euc = jis2eucjp(jis);
		if (ms932)
			ucs2 = ms932ToUtf32( sj );
		else
			ucs2 = jis2utf32( jis );
    } else {
    	/* 1�o�C�g�����̂Ƃ� */
    	euc  = jis2eucjp(jis);	//(0xA1 <= jis && jis <= 0xFE) ? (0x8E00|jis) : jis;
    	ucs2 = ubyte_to_msUCS2_tbl[jis];
    }

    if (flags & F_KUTEN) {
    	int ku	= ((jis >> 8) & 0xff) - 0x20;
    	int ten = (jis & 0xff) - 0x20;
   	    printf("%d-%02d-%02d\t", men, ku, ten);
    }
    if (flags & F_JIS) {
    	printf("%d-%04X\t", men+2, jis & 0xffff);
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



/** �����R�[�h�\���e�L�X�g�o�� */
static void printKnjTbl(int flags, int lc, int patCh, int pgMode, int memoriFlg)
{
    int m, y, x, jis, sj, euc, ucs2, clm, cc, yend;

	wrt_clm = lc;
    clm = 0;
    if (memoriFlg)
    	printMemori(flags, lc);
    yend = (ms932) ? (0x21 + 120) : 0x7f;
    for (m = 0; m < kubunMen; ++m) {
	    for (y = 0x21; y < yend; y++) {
	    	if (kubun[m * 94 + y-0x21] == 0)     	    	/* �\�����Ȃ��敪�͔�΂� */
	    	    continue;
	    	for (x = 0x21-(patCh!=0); x < 0x7f+(patCh!=0); x++) {
	    	    /* JIS */
	    	    jis = (y<<8) | x;

	    	    if (m)
	    	    	jis |= 0x10000;

	    	    /* Shift JIS */
	    	    if (x == 0x20)
	    	    	cc = jis + 1;
	    	    else if (x == 0x7f)
	    	    	cc = jis + 1;
	    	    else
	    	    	cc = jis;
	    	    sj = jis2sjis(cc);

	    	    if (clm == 0) { 	    	    	/*���[�̕����R�[�h�̕\�� */
	    	    	if (lc == 94 && memoriFlg) {	/* ��\�� */
	    	    	    if (flags & F_KUTEN94)
	    	    	    	printf("%2d ", ((jis>>8)&0xff)-0x20);
	    	    	    else
	    	    	    	printf("%2x ", (jis>>8)&0xff);
	    	    	}
	    	    	printCode(jis, sj, flags);
	    	    }
	    	    if (x >= 0x21 && x <= 0x7E) {   	/* �ʏ�̕\�� */
	    	    	wrt_putCh(cc/*sj*/);
	    	    	if (++clm == lc) {
	    	    	    printf("\n");
	    	    	    clm = 0;
	    	    	}
	    	    } else if (patCh) {     	    	/*�\���ʒu�����̂��߂� ??20, ??7F �΍� */
	    	    	wrt_putCh(patCh);   /* printf("%c%c", (patCh>>8) & 0xFF, patCh & 0xFF); */
	    	    	if (++clm == lc) {
	    	    	    printf("\n");
	    	    	    clm = 0;
	    	    	}
	    	    }
	    	}
	    	if (pgMode && y < yend - 1) {   	    	/* �y�[�W���� */
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



/** ������́A�������ƂɁA�����R�[�h��\�� */
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




/** �o�C�i���o�� */
static void outputKnjTblB(void)
{
    /* DOS�n�ł�stdout�̓f�t�H�̓e�L�X�g�����ǁA */
    /* ���ɂȂ���s���̃R���g���[���͏o�͂��Ȃ��̂Ŗ��Ȃ� */
    int x, y,c,j;
	int yy = (ms932) ? 0x21 + 120 : 0x7f;
	int m;
	for (m = 0; m < kubunMen; ++m) {
	    for (y = 0x21; y < yy; y++) {
	    	if (kubun[m * 94 + y - 0x21] == 0)     	    	/* �\�����Ȃ��敪�͔�΂� */
	    	    continue;
	    	for (x = 0x21; x <= 0x7E; x++) {
	    	    c = (m << 16) | (y<<8) | x;
	    	    //j = jis2sjis(c);
	    	    wrt_putCh(c);
	    	}
	    }
	}
}


static void  setNimen() {
	unsigned t = 94 - 1;
	int i;

	kubun[t + 1] = 2;
	kubun[t + 3] = 2;
	kubun[t + 4] = 2;
	kubun[t + 5] = 2;
	kubun[t + 8] = 2;
	kubun[t + 12] = 2;
	kubun[t + 13] = 2;
	kubun[t + 14] = 2;
	kubun[t + 15] = 2;

	for (i = 78; i <= 94; ++i)
		kubun[t + i] = 2;
}

/** �敪�Ɋւ���w���v�\�� */
static void ku_help(void)
{
    typedef struct stbl_t {
    	int start, end;
    	char *msg;
    } stbl_t;
    static stbl_t stbl[] = {
    	{0x2121, 0x227E, "�e��L��"},
    	{0x2321, 0x237E, "�Z�p�����A(���e��)�A���t�@�x�b�g"},
    	{0x2421, 0x247E, "�Ђ炪��"},
    	{0x2521, 0x257E, "�J�^�J�i"},
    	{0x2621, 0x267E, "�M���V�A����"},
    	{0x2721, 0x277E, "�L��������"},
    	{0x2821, 0x287E, "�r���f��"},
    	{0x2921, 0x2c7E, "JIS X 213 �񊿎�"},
    	{0x2d21, 0x2d7E, "JIS X 213 �񊿎� | NEC���ꕶ��"},
    	{0x2e21, 0x2F7E, "JIS X 213 ����`"},
    	{0x3021, 0x4F7E, "��ꐅ������"},
    	{0x5021, 0x7406, "��񐅏�����"},
    	{0x7407, 0x7E7E, "JIS X 213 ��O����"},
    	{0x7f21, 0x987E, "(SJIS�@��ˑ��g���̈�)"},
    	{0x12121,0x1217E, "JIS X 213 ��l����"},
    	{0x12321,0x1257E, "JIS X 213 ��l����"},
    	{0x12821,0x1287E, "JIS X 213 ��l����"},
    	{0x12c21,0x12f7E, "JIS X 213 ��l����"},
    	{0x16e21,0x17E7E, "JIS X 213 ��l����"},
    };
    int i, n, c1,c2;

    printf("JIS����(JIS X 208|JIS X 213)�̋�̕���\n");
    for (i = 0; i < sizeof(stbl)/sizeof(stbl[0]); i++) {
    	c1 = stbl[i].start;
    	c2 = stbl[i].end;
    	n  = (c2>>8) - (c1>>8);
		if (c1 < 0x10000) {
	    	if (n == 0) {
	    	    printf("1��%2d    ��", (c1>>8)-0x20);
		    	printf("  3-%x�`%x  %x�`%x   ", c1, c2, jis2sjis(c1), jis2sjis(c2));
	    	} else {
				if ((c2>>8) <= 0x20+99) {
	    	    	printf("1��%2d�`%2d��", (c1>>8)-0x20, (c2>>8)-0x20);
		    		printf("  3-%x�`%x  %x�`%x   ", c1, c2, jis2sjis(c1), jis2sjis(c2));
		    	} else {
	    	    	printf("1��%2d�`%2d��", (c1>>8)-0x20, (c2>>8)-0x20);
		    		printf(" 3-%x�`%x  %x�`%x   ", c1, c2, jis2sjis(c1), jis2sjis(c2));
				}
	    	}
		} else {
	    	if (n == 0) {
	    	    printf("2��%2d    ��", ((c1>>8)&0xff)-0x20);
		    	printf("  4-%x�`%x  %x�`%x   ", (uint16_t)c1, (uint16_t)c2, jis2sjis(c1), jis2sjis(c2));
	    	} else {
    	    	printf("2��%2d�`%2d��", ((c1>>8)&0xff)-0x20, ((c2>>8)&0xff)-0x20);
	    		printf("  4-%x�`%x  %x�`%x   ", (uint16_t)c1, (uint16_t)c2, jis2sjis(c1), jis2sjis(c2));
			}
		}
    	printf("%s\n", stbl[i].msg);
    }
    printf(
    	"�� �I���͊e���0x7E(94)�����̏ꍇ�����邪�A�ꗥ ??7E�ɂ��Ă���\n"
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

    memset(kubun, 0, sizeof(kubun));
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
    	    	    	switch (c) {
    	    	    	case 'k':   flags |= F_KUTEN;	break;
    	    	    	case 'j':   flags |= F_JIS; 	break;
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
    	    	    /* case 'k':wrt_flags =F_KUTEN; break; */
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
    	    	kubunMen = 1;
    	    	if (*p == 'b')
    	    		kubunMen = 2;
    	    	d = c = strtol(p,&p,10);
    	    	if (c < 1 || c > 120/*94*/)
    	    	    goto OPT_ERR;
    	    	if (*p)
    	    	    d = strtol(p+1,0,10);
    	    	if (d < c)
    	    	    goto OPT_ERR;
    	    	for (k = c; k <= d; k++)
    	    	    kubun[(kubunMen-1)*94 + k-1] = 1;
    	    	break;
    	    case '0':
    	    	kubunMen = 1;
    	    	d = (*p == 'x') ? 15 : 8;
    	    	for (k = 1; k <= d; k++)
    	    	    kubun[k-1] = 1;
    	    	break;
    	    case '1':
    	    	kubunMen = 1;
    	    	for (k = 16; k <= 47; k++)
    	    	    kubun[k-1] = 1;
    	    	break;
    	    case '2':
    	    	kubunMen = 1;
    	    	for (k = 48; k <= 84; k++)
    	    	    kubun[k-1] = 1;
    	    	break;
    	    case '3':
    	    	kubunMen = 1;
    	    	for (k = 85; k <= 94; k++)
    	    	    kubun[k-1] = 1;
    	    	break;
    	    case '4':
    	    	kubunMen = 2;
				setNimen();
    	    	break;
    	    case 'x':
    	    	//kubunMen = 0;
    	    	ms932 = 1;
    	    	for (k = 95; k <= 120; k++)
    	    	    kubun[k-1] = 1;
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
    	    /* ����sjis�����񂪂���΁A����̕����R�[�h��\�� */
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

    if (kubunMen == 0) {    /* ��̎w�肪�Ȃ���΂��ׂĕ\�� */
		if (ms932) {
			kubunMen = 1;
	    	for (k = 1; k <= 120; k++)
	    	    kubun[k-1] = 1;
	   	} else {
			kubunMen = 2;
	    	for (k = 1; k <= 94; k++)
	    	    kubun[k-1] = 1;
			setNimen();
		}
    }
    if (binFlg == 0)
    	printKnjTbl(flags, lc, patCh, pgMode, memoriFlg);
    else
    	outputKnjTblB();
    return 0;
}
