/*
    knjt    	�o������ v2.00
    ����
    198?    os9/09 6809�̃A�Z���u���ŁB�ꐅ�݂̂������悤�ȋC������B
    	    �ǂ̂悤�Ȏd�l�������������o���Ă��Ȃ�^^;
    199?    ANSI-C(MS-DOS)  �s���s���B
    2000/10 v1.00 �s���s���䂦��蒼���B
    	    �ł����āA���낢�낲���Ⴒ����@�\����.
    	    v1.01 -pN ��N�y�[�W���Ƃ�t��
    2004/01 v2.00 unicode�\����t��. euc���_�̈�����ύX. -t�̎d�l�ύX
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef unsigned char	UCHAR;

#define ISKANJI(c)  	((UCHAR)(c) >= 0x81 && ((UCHAR)(c) <= 0x9F || ((UCHAR)(c) >= 0xE0 && (UCHAR)(c) <= 0xFC)))
#define ISKANJI2(c) 	((UCHAR)(c) >= 0x40 && (UCHAR)(c) <= 0xfc && (c) != 0x7f)
#define JIS2NO(j)   	(((((j) >> 8) & 0x7f) - 0x21) * 94 + (((j) & 0x7f) - 0x21))


/** 8bit������㉺�ɂȂ���16�r�b�g���ɂ��� */
#define BB(a,b)     	((((UCHAR)(a))<<8)|(UCHAR)(b))

/** 8bit��4����ʂ��珇�ɂȂ���32�r�b�g���ɂ��� */
#define BBBB(a,b,c,d)	((((UCHAR)(a))<<24)|(((UCHAR)(b))<<16)|(((UCHAR)(c))<<8)|((UCHAR)(d)))

/** 8bit��6��... 64bit�����p�ƂȂ�̂ŁA�Ƃ肠�����A����̓_�~�[ */
#define BBBBBB(a,b,c,d,e,f) 	(0xFFFFFFFF)

static char kubun[94];
extern unsigned short jisno_to_msUCS2_tbl[];
extern unsigned short ubyte_to_msUCS2_tbl[];


/** �g���� */
static void Usage(void)
{
    printf(
    	"usage>knjt [-opts]       // v2.00 " __DATE__ "  by tenk*\n"
    	"    �V�t�gJIS�����őS�p�����ꗗ�̃e�L�X�g�𐶐�����\n"
    	"  -opts:  �i�f�t�H���g�� -l16 -c1 -t0�j\n"
    	"    -w[seo]    ���ʂ� s:SJIS e:EUC o:UTF8 �ɂ��ďo��\n"
    	"    -t[kjseuo] �e�s�̐擪�ɕ����R�[�h��t��. -t0���ƕt�����Ȃ�\n"
    	"               k:��_ j:JIS s:SJIS e:EUC u:UCS2 o:UTF8 ��t��.(�g���킹��)\n"
    	"    -k         ��̕��ނ̕\��(�w���v)\n"
    	"    -k[N:M]    �\�������͈̔͂��w��. 1�`94. �����w��\\n"
    	"    -0         �L����p���J�i��.    -k1:8  �ɓ���\n"
    	"    -1         ������ꐅ���̎w��. -k16:47 �ɓ���\n"
    	"    -2         ������j�����̎w��. -k48:84 �ɓ���\n"
    	"    -l[N]      1�s�ɕ\�����镶����. 1�`94(�f�t�H���g 16)\n"
    	"               ������ -c0 ���w�肳�ꂽ���ƂɂȂ�.\n"
    	"    -cN        JIS�R�[�h���Z�ł�??20,??7f��\n"
    	"               0:�\�����Ȃ�    1:���p�󔒂Q��'  '(�f�t�H���g)\n"
    	"               2:�S�p�󔒈��'�@'\n"
    	"               ���p�L��:�������  �S�p����:��������\n"
    	"    -m         �ڐ����t��\n"
    	"    -p[N]      �悲�Ƃɋ�s(+�ڐ���)��}��.(N��2�ȏ�Ȃ�N�悲�Ƃɑ}��)\n"
    	"    -b         �e�L�X�g�łȂ��A�����R�[�h�݂̂̃o�C�i���o��\n"
    	" �� ??20, ??7F �̃V�t�gJIS�\���̒l�̓E�\�ɂȂ�܂�(���̕����̒l�ł�)\n"
    	/*
    	//  "	 -t[N]	    �e�s�̐擪�ɕ����R�[�h��t��. 0:�����ǉ����Ȃ�\n"
    	//  "	    	    1:JIS 2:SJIS 4:EUC 8:Unicode(UCS2) 16:UTF8	32:��_\n"
    	//  "	    	    �����Z���đg�ݍ��킹�Ďw���.\n"
    	*/
    );
    exit(1);
}


/* ----------------------------------------------------------*/


/** JIS �� EUC �ɕϊ� */
#define jis2euc(jis)	((jis) | 0x8080)


/** jis �R�[�h�� �V�t�gjis�ɕϊ� */
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


/** �V�t�gjis �� jis �ɕϊ� */
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


/** JIS �� MS-UCS2 �ɕϊ� */
unsigned jis2ucs2(int jis)
{
    return jisno_to_msUCS2_tbl[JIS2NO(jis)];
}


/** UCS2 �� utf8 �ɕϊ�. ��utf16�ɂ͖��Ή� */
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
    	/* JIS�͈͕�����3�o�C�g�Ŏ��܂�̂ŁA���̂�����͎g��Ȃ�...�̂�BBBBBB�_�~�[�ɂ��āA���̒l���U�� */
    	return BBBBBB(00, 0xF8|(c>>24), 0x80|(c>>18)&0x3f, 0x80|(c>>12)&0x3f,0x80|(c>>6)&0x3f,0x80|(c&0x3f));
    } else {
    	/* JIS�͈͕�����3�o�C�g�Ŏ��܂�̂ŁA���̂�����͎g��Ȃ�...�̂�BBBBBB�_�~�[�ɂ��āA���̒l���U�� */
    	return BBBBBB(0xFC|(c>>30), 0x80|(c>>24)&0x3f, 0x80|(c>>18)&0x3f, 0x80|(c>>12)&0x3f, 0x80|(c>>6)&0x3f, 0x80|(c&0x3f));
    }
}



/* ----------------------------------------------------------*/

/** �\������R�[�h�̌n�̎�� */
enum {
    F_JIS=1, F_SJIS=2, F_EUC=4, F_UCS2=8, F_UTF8=16, F_KUTEN=32, F_KUTEN94=64,
};

int wrt_flags = 0;


/** ���p���S�p(SJIS����)�P������W���o�� */
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
    	    ucs2 = 0x30FB;  /* �E ��UCS2�R�[�h. �������Ƃ��� */
    	fputc(ucs2 & 0xFF, stdout);
    	fputc((ucs2 >> 8) & 0xFF, stdout);
    	return;
  #endif
    } else if (wrt_flags & F_UTF8) {
    	unsigned ucs2 = jis2ucs2( sjis2jis(sjis) );
    	if (ucs2 == 0xffff)
    	    ucs2 = 0x30FB;  /* �E ��UCS2�R�[�h. �������Ƃ��� */
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


/** �w��̔��p�󔒂��o�� */
static void printSpc(int n)
{
  #if 0
    printf("%*c", n, ' ');
  #else /* printf���Â��d�l�̂܂܂̂Ƃ� */
    int i;
    for (i = 0; i < n; i++)
    	printf(" ");
  #endif
}


/* �����R�[�h����flags�ɂ��������ĕ\�� */
static void printFlags(int flags, int add)
{
    if (add > 0) {
    	printSpc(add);
    }
    if (flags & F_KUTEN) {
    	/* printf("�� �_ "); */
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


/** �ł��Ƃɂ���ڐ��� */
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
    if (jis > 0xff) {
    	/* 2�o�C�g�����̂Ƃ� */
    	euc = jis2euc(jis);
    	ucs2 = jisno_to_msUCS2_tbl[JIS2NO(jis)];
    } else {
    	/* 1�o�C�g�����̂Ƃ� */
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



/** �����R�[�h�\���e�L�X�g�o�� */
static void printKnjTbl(int flags, int lc, int patCh, int pgMode, int memoriFlg)
{
    int y, x, jis, sj, euc, ucs2, clm, cc;

    clm = 0;
    if (memoriFlg)
    	printMemori(flags, lc);
    for (y = 0x21; y < 0x7F; y++) {
    	if (kubun[y-0x21] == 0)     	    	/* �\�����Ȃ��敪�͔�΂� */
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

    	    if (clm == 0) { 	    	    	/*���[�̕����R�[�h�̕\�� */
    	    	if (lc == 94 && memoriFlg) {	/* ��\�� */
    	    	    if (flags & F_KUTEN94)
    	    	    	printf("%2d ", (jis>>8)-0x20);
    	    	    else
    	    	    	printf("%2x ", jis>>8);
    	    	}
    	    	printCode(jis, sj, flags);
    	    }
    	    if (x >= 0x21 && x <= 0x7E) {   	/* �ʏ�̕\�� */
    	    	wrt_putCh(sj);	    /* fputc(sj>>8, stdout);fputc(sj&0xff, stdout); */
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
    	if (pgMode && y < 0x7e) {   	    	/* �y�[�W���� */
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



/** ������́A�������ƂɁA�����R�[�h��\�� */
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




/** �o�C�i���o�� */
static void outputKnjTblB(void)
{
    /* DOS�n�ł�stdout�̓f�t�H�̓e�L�X�g�����ǁA */
    /* ���ɂȂ���s���̃R���g���[���͏o�͂��Ȃ��̂Ŗ��Ȃ� */
    int x, y,c,j;

    for (y = 0x21; y < 0x7F; y++) {
    	if (kubun[y-0x21] == 0)     	    	/* �\�����Ȃ��敪�͔�΂� */
    	    continue;
    	for (x = 0x21; x <= 0x7E; x++) {
    	    c = (y<<8) | x;
    	    j = jis2sjis(c);
    	    wrt_putCh(j);
    	}
    }
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
    	{0x2921, 0x2F7E, "JIS X 208 �Ŗ���`"},
    	{0x3021, 0x4F7E, "��ꐅ������"},
    	{0x5021, 0x747E, "��񐅏�����"},
    	{0x7521, 0x7E7E, "JIS X 208 �Ŗ���`"},
    };
    int i, n, c1,c2;

    printf("JIS����(JIS X 208)�̋�̕���\n");
    for (i = 0; i < sizeof(stbl)/sizeof(stbl[0]); i++) {
    	c1 = stbl[i].start;
    	c2 = stbl[i].end;
    	n  = (c2>>8) - (c1>>8);
    	if (n == 0) {
    	    printf("%2d    ��", (c1>>8)-0x20);
    	} else {
    	    printf("%2d�`%2d��", (c1>>8)-0x20, (c2>>8)-0x20);
    	}
    	printf("  %x�`%x  %x�`%x   ", c1, c2, jis2sjis(c1), jis2sjis(c2));
    	printf("%s\n", stbl[i].msg);
    }
    printf(
    	"�� �I���͊e���0x7E(94)�����̏ꍇ�����邪�A�ꗥ ??7E�ɂ��Ă���\n"
    	"�� JIS X 208 �ł̖���`�G���A�ɂ͋@��ˑ�������\n"
    	"   ���̌�̋K�i�Ŋg�����ꂽ�����������Ă���ꍇ������B\n"
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

    if (kubunFlg == 0) {    /* ��̎w�肪�Ȃ���΂��ׂĕ\�� */
    	for (k = 1; k <= 94; k++)
    	    kubun[k-1] = 1;
    }
    if (binFlg == 0)
    	printKnjTbl(flags, lc, patCh, pgMode, memoriFlg);
    else
    	outputKnjTblB();
    return 0;
}

