/*
 *  BINAT
 *  	    writen by M.Kitamura
 *
 *  91/05/??	bintxt	v1.00	for MS-DOS & os9/09
 *  91/06/09	bintxt	v1.01	��������������
 *  94/06/10	binatxt v1.10	binatxt�ɉ���. WORD���̓��[�h��t��
 *  	    	    	    	���͂�os9/09�͕s��...
 *  95/03/08	binat	v1.11	binat �ɉ���. x68k(xc)�ź��߲ىɂ���
 *  	    	    	    	-am��6809����68K�̋^�����߂ɕύX(-a9��09)
 *  96/03/26	binat	v1.12	DWORD(4�޲�)���̓��[�h��t��
 *  	    	    	    	x68k(xc)�͂��͂�s��...
 *  96/03/27	binat	v1.13	-q�ɂ�鱽��������\���t��
 *  96/04/25	binat	v1.14	-w[i|m] -ww[i|m] �� -m[w|d],-i[w|d]�ɕύX
 *  	    	    	    	�����S�p�\���� $80 �̈����Y��Ă��̂��C��
 *  97/09/13	binat	v1.15	���ڽ�\����4���ɂȂ����̂��C��(%08X --> %08LX)
 *  98/09/11	binat	v1.16	-g �̒ǉ�
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*#include <ctype.h>*/
#include <limits.h>


/*------------------------------------------------*/

#ifdef OS9
  /* OS9/6809 MicrowareC */
  typedef  char     UCHAR;
  typedef  unsigned USHORT;
  typedef  /*unsigned*/ long ULONG;
  #define  REG	    register
  #define  WRTMODE  "r"
/*#define  LONG_MAX 0x7fffffffL */
    	    	/*<limits.h>�̂Ȃ�������LONG_MAX��ݒ肵�Ă�*/
  #define  EP(p)    fprintf(stderr,p)
  #define  NCNT_MAX 512
#else
#ifdef X68K
  /* X68K XC */
  typedef  unsigned char  UCHAR;
  typedef  unsigned short USHORT;
  typedef  unsigned long  ULONG;
  #define  REG
  #define  WRTMODE "rb"
  #define  xtol(x)  strtol((x),NULL,16)
  #define  EP(p)  printf(p)
  #define  NCNT_MAX 0x4000
#else
  /* MS-DOS TC etc */
  typedef  unsigned char UCHAR;
  typedef  unsigned 	 USHORT;
  typedef  unsigned long ULONG;
  #define  REG
  #define  WRTMODE "rb"
  #define  xtol(x)  strtol((x),NULL,16)
 #ifdef LSI_C
  #define  FSETBIN
 #endif
  #define  EP(p)  printf(p)
  #define  NCNT_MAX 0x4000
#endif
#endif

#define isdigit(c)  (((unsigned)(c) - '0') < 10)
#define islower(c)  (((unsigned)(c)-'a') < 26)
#define toupper(c)  (islower(c) ? (c) - 0x20 : (c) )
#define iskanji(c)  ((unsigned)((c)^0x20) - (unsigned)0xa1 < (unsigned)0x3c)
#define iskanji2(c) ((UCHAR)(c) >= (UCHAR)0x40 && (UCHAR)(c) <= (UCHAR)0xfc && (UCHAR)(c) != (UCHAR)0x7f)
/*#define iskanji2(c) (((c)&0xff) >= 0x40 && ((c)&0xff) <= 0xfc && ((c)&0xff) != 0x7f)*/

/*------------------------------------------------*/

UCHAR buf[NCNT_MAX+8];

#define  T_N  0
#define  T_AI 1
#define  T_AM 2
#define  T_B  3
#define  T_C  4
#define  T_A9 5
#define  T_TR 6

void FileErr()
{
    printf("error %d\n",errno);
    exit(0);
}

void Usage()
{
    EP("usage: binat [-opts] [file(s)] ...                     // binat v1.16\n");
    EP("�޲��̧�ق̒��g���w�肳�ꂽ̫�ϯĂ�(1�޲ĒP�ʂ�)÷�Ăɂ��ĕ\�����܂�\n");
    EP(" -?   ���̐���                       -d    10�i�ŕ\��\n");
    EP(" -mw  BYTE�łȂ���۰�WORD�ŏ���      -iw   BYTE�łȂ�����WORD�ŏ���\n");
    EP(" -md  BYTE�łȂ���۰�LONG�ŏ���      -id   BYTE�łȂ�����DWORD�ŏ���\n");
    EP(" -am  ��۰�(68k)�̱����ׂ̕\�L(dc.b) -ai   ����(x86)�̱����ׂ̕\�L(DB)\n");
    EP(" -n   ����dump                       -k    (-n�̂Ƃ�)���ڽ��t��\n");
    EP(" -c   C�ł̕\�L    -g TRON��data     -b    BASIC��DATA��\n");
    EP(" -rN  ���͊J�n�ʒu(10�i)             -rxN  ���͊J�n�ʒu(16�i)\n");
    EP(" -sN  ���͂���ő��޲Đ�(10�i)       -sxN  ���͂���ő��޲Đ�(16�i)\n");
    EP(" -lN  1�s���޲Đ�(1-8192)            -tN   �s���̋�($20)�̐�\n");
    EP(" -yN  N�s�����ɋ�s��}��            -z    �W������(�w��t�@�C������)\n");
    EP(" -qN  �����\���t(1=�S�p 2=7bit 3=�f)\n");
    EP(" -eSTR DB,dc.b,DATA���̑���ɍs���ɒu����������w��\n");
    EP("\n");
    EP("-r(x),-s(x)�ł̒l�͈̔͂͂Ƃ肠����31bits�������ł�\n");
    EP("̧�ق͕����w��\�ł�.\n");
  #ifndef FSETBIN
    EP("-z�̕W�����͂�÷��Ӱ�ނȂ̂Ŗ��ɗ����Ȃ��ł���\n");
  #endif
    EP("��߼�݂͕����w��\�ŁA������ނ̎w��͌��̂��̂��L���ł�\n");
    EP("��߼�݂���̫�ď�Ԃ� -l16 -t8 -r0 -sx7fffffff �ł�\n");
    EP("��߼�ݖ��w�莞�� -n -k -q ���w�肳�ꂽ���ƂɂȂ�܂�\n");
    exit(0);
}


int DmyRead(n,fp)
    long n;
    FILE *fp;
{
  #if 0
    long i;
    for (i = 0; i < n; i++) {
    	fread(buf,1,1,fp);
    	if (feof(fp) || ferror(fp))
    	    return -1;
    }
  #else
    while (n > (sizeof buf)) {
    	fread(buf, 1, (sizeof buf), fp);
    	if (feof(fp) || ferror(fp))
    	    return -1;
    	if (ferror(fp))
    	    return -1;
    	n -= sizeof buf;
    }
    if (n) {
    	fread(buf, 1, (size_t)n, fp);
    	if (feof(fp) || ferror(fp))
    	    return -1;
    }
  #endif
    return 0;
}

void PriLine(buf,linSiz,n,tspcCnt,styl,ditF,wdMd,imF,dc)
    REG UCHAR *buf;
    int  linSiz;
    int  n;
    int  tspcCnt;
    int  styl;
    int  ditF;
    int  wdMd;
    int  imF;
    char *dc;
{
    int i;
    USHORT a;
    char *t;
    static ULONG l;
    static char *dcbuf[3][7] = {
    	 {"","DB ","dc.b ","DATA ","","fcb ",".DATA.B "},
    	 {"","DW ","dc.w ","DATA ","","fdb ",".DATA.W "},
    	 {"","DD ","dc.l ","DATA ","","fdb ",".DATA.L "}    /* 6809���ݸ�ܰ�ނ͐����ł��Ȃ���^^; */
    };
    static char *pat[3/*b|w|d*/][7/*styl*/][2/*h|d*/][3/*0|n| */] = {
    	{
    	    {{"%02x",	" %02x",    "   "}, 	{"%03u",    " %03u",	"    "}},
    	    {{"0%02Xh", ",0%02Xh",  "     "},	{"%03u",    ",%03u",	"    "}},
    	    {{"$%02X",	",$%02X",   "    "},	{"%03u",    ",%03u",	"    "}},
    	    {{"&H%02X", ",&H%02X",  "     "},	{"%03u",    ",%03u",	"    "}},
    	    {{"0x%02x,","0x%02x,",  "     "},	{"%3u,",    "%3u,", 	"    "}},
    	    {{"$%02X",	",$%02X",   "    "},	{"%03u",    ",%03u",	"    "}},
    	    {{"H'%02x", ",H'%02x",  "     "},	{"%03u",    ",%03u",	"    "}}
    	},{
    	    {{"%04x",	" %04x",    "     "},	{"%05u",    " %05u",	"      "}},
    	    {{"0%04Xh", ",0%04Xh",  "       "}, {"%05u",    ",%05u",	"      "}},
    	    {{"$%04X",	",$%04X",   "      "},	{"%05u",    ",%05u",	"      "}},
    	    {{"&H%04X", ",&H%04X",  "       "}, {"%05d",    ",%05d",	"      "}},
    	    {{"0x%04x,","0x%04x,",  "       "}, {"%5uU,",   "%5uU,",	"      "}},
    	    {{"$%04X",	",$%04X",   "      "},	{"%05u",    ",%05u",	"      "}},
    	    {{"H'%04X", ",H'%04X",  "       "}, {"%05d",    ",%05d",	"      "}}
    	},{
    	    {{"%08lx",	" %08lx",   "         "},   {"%010lu",	" %010lu",  "           "}},
    	    {{"0%08lXh",",0%08lXh", "           "}, {"%010lu",	",%010lu",  "           "}},
    	    {{"$%08lX", ",$%08lX",  "          "},  {"%010lu",	",%010lu",  "           "}},
    	    {{"&H%08lX",",&H%08lX", "           "}, {"%010ld",	",%010ld",  "           "}},
    	    {{"0x%08lxL,","0x%08lxL,","            "},{"%10luUL,","%10luUL,","            "}},
    	    {{"$%08lX", ",$%08lX",  "          "},  {"%010u",	",%010u",   "           "}},
    	    {{"H'%08lX",",H'%08lX", "           "}, {"%010ld",	",%010ld",  "           "}}
    	}
    };

    for (i = 0; i < tspcCnt; i++)
    	printf(" ");

    if (dc == NULL)
    	dc = dcbuf[wdMd][styl];

    printf("%s",dc);
    t = pat[wdMd][styl][ditF][1];

    switch (wdMd) {
    case 0: /* BYTE */
    	printf(pat[0][styl][ditF][0],*(buf++));
    	for ( i = 1 ; i < n; i++ )
    	    printf(t,*(buf++));
    	for (i = n; i < linSiz; i+=1)
    	    printf(pat[0][styl][ditF][2]);
    	break;
    case 1: /*WORD*/
    	a = (imF == 0) ? (buf[0]<<8) + buf[1] : (buf[1]<<8) + buf[0];
    	printf(pat[1][styl][ditF][0], a);
    	buf += 2;
    	n = (n+1) & ~1;
    	for (i = 2; i < n; i+= 2, buf += 2) {
    	    a = (imF == 0) ? (buf[0]<<8) + buf[1] : (buf[1]<<8) + buf[0];
    	    printf(t,a);
    	}
    	for (i = n; i < linSiz; i+=2)
    	    printf(pat[1][styl][ditF][2]);
    	break;
    default:/*case 3:*//* DWORD */
    	l = (imF == 0)
    	    ?	((ULONG)buf[0]<<24) + ((ULONG)buf[1]<<16) + (buf[2]<<8) + buf[3]
    	    :	((ULONG)buf[3]<<24) + ((ULONG)buf[2]<<16) + (buf[1]<<8) + buf[0];
    	printf(pat[2][styl][ditF][0], l);
    	buf += 4;
    	n = (n + 3) & ~3;
    	for (i = 4; i < n; i+= 4, buf += 4) {
    	    l = (imF == 0)
    	    	?   ((ULONG)buf[0]<<24) + ((ULONG)buf[1]<<16) + (buf[2]<<8) + buf[3]
    	    	:   ((ULONG)buf[3]<<24) + ((ULONG)buf[2]<<16) + (buf[1]<<8) + buf[0];
    	    printf(t,l);
    	}
    	for (i = n; i < linSiz; i+=4)
    	    printf(pat[2][styl][ditF][2]);
    	break;
    }
}

#if 10

void PriCmt(buf,linSiz,n,styl,cmtM)
    char    *buf;
    int     linSiz;
    int     n;
    int     styl;
    int     cmtM;
{
    static char *cmtMm[7] = {" ","; ","* ","' ","/* ","* ","; "};
    int     i,c;

    printf(" %s",cmtMm[styl]);
    for (i = 0; i < linSiz; i++) {
    	if (i < n) {
    	    c = buf[i];
    	    if (cmtM == 2) {
    	    	c &= 0x7f;
    	    }
    	    if (c <= 0x20) {
    	    	c = '.';
    	    } else if (c <= 0x7E) {
    	    	;
    	    } else if (c == 0x7F) {
    	    	c = '.';
    	    } else if (c <= 0x9f) {
    	    	goto SJIS;
    	    } else if (c <= 0xDF) {
    	    	;
    	    } else if (c <= 0xFC) {
    	  SJIS:
    	    	if (cmtM == 1) {
    	    	    if (i < linSiz - 1 && c != 0x80 && iskanji2(buf[i+1])) {
    	    	    	printf("%c",c);
    	    	    	i++;
    	    	    	c = buf[i];
    	    	    } else {
    	    	    	c = '.';
    	    	    }
    	    	} else if (cmtM == 3) {
    	    	    ;
    	    	} else {
    	    	    c = '.';
    	    	}
    	    } else {
    	    	if (cmtM == 3 && c != 0xff) {
    	    	    ;
    	    	} else {
    	    	    c = '.';
    	    	}
    	    }
    	} else {
    	    c = ' ';
    	}
    	printf("%c",c);
    }
    if (styl == T_C)
    	printf(" */");
}

#endif


int One(fp,linSiz,tspcCnt,styl,wdMd,imF,ditF,adrF,rcnt,sizz,dc,akiY,cmtM)
    FILE    *fp;
    int     linSiz;
    int     tspcCnt;
    int     styl;
    int     wdMd;
    int     imF;
    int     ditF;
    int     adrF;
    long    rcnt;
    long    sizz;
    char    *dc;
    int     akiY;
    int     cmtM;
{
    int     f;
    REG int n;
    ULONG   ofs;
    int     lcnt;

    if (rcnt > 0) {
    	if (DmyRead(rcnt,fp) )
    	    return -1;	/* ERROR */
    }
    ofs = rcnt;

    lcnt = akiY;
    f = 0;
    for ( ; ; ) {
    	memset(buf, 0, linSiz+4);
    	n = fread(buf,1,linSiz,fp);
    	if (n <= 0)
    	    break;
    	if ((sizz -= n) <= 0) {
    	    n += (int)sizz;
    	    f = 1;
    	}
    	if (adrF) {
    	    printf("%08LX  ",ofs);
    	    ofs += n;
    	}
    	PriLine(buf,linSiz,n,tspcCnt,styl,ditF,wdMd,imF,dc);
      #if 10
    	if (cmtM)
    	    PriCmt(buf,linSiz,n,styl,cmtM);
      #endif
    	printf("\n");

    	if (akiY) {
    	    if (--lcnt == 0) {
    	    	lcnt = akiY;
    	    	printf("\n");
    	    }
    	}
    	if (f)
    	    return 0;
    }
    if (ferror(fp))
    	return -1;  /* ERROR */
    return 0;
}

int main(argc,argv)
    int  argc;
    char *argv[];
{
    static FILE *fp;
    static long rcnt = 0;
    static long sizz = LONG_MAX;
    static int	digitFlg = 0;
    static int	linSiz = 16;
    static int	tspcCnt = 8;
    static int	n = 0;
    static UCHAR wdMd = 0;
    static UCHAR imF = 0;
    static UCHAR adrF = 0;
    static UCHAR styl = 0xff;
    static UCHAR stdinFlg = 0;
    static UCHAR cmtM = 0;
    static char dcname[32+2] = "";
    static char *dc = NULL;
    static int	akiY = 0;
    int     	i;
    char    	c;
    REG char	*p;

    /*memset(dcname,'\0', 32);*/
    for (i = 1; i < argc; i++) {
    	p = argv[i];
    	if (*p != '-') {
    	    n++;
    	} else {
    	    ++p;
    	    c = *(p++);
    	    c = (char)toupper(c);
    	    switch (c) {
    	    case 'L':
    	    	linSiz = atoi(p);
    	    	if (linSiz <= 0 || linSiz > NCNT_MAX)
    	    	    goto OPTS_ERR;
    	    	break;
    	    case 'R':
    	    	c = (char)toupper(*p);
    	    	if (c == 'X')
    	    	    rcnt = xtol(++p);
    	    	else
    	    	    rcnt = atol(p);
    	    	if (rcnt < 0)
    	    	    goto OPTS_ERR;
    	    	break;
    	    case 'S':
    	    	c = (char)toupper(*p);
    	    	if (c == 'X')
    	    	    sizz = xtol(++p);
    	    	else
    	    	    sizz = atol(p);
    	    	if (sizz <= 0)
    	    	    sizz = LONG_MAX;
    	    	break;
    	    case 'T':
    	    	tspcCnt = atoi(++p);
    	    	if (tspcCnt < 0 || tspcCnt > 79-7)
    	    	    goto OPTS_ERR;
    	    	break;
    	    case 'A':
    	    	c = (char)toupper(*p);
    	    	if (c == 'M')
    	    	    styl = T_AM;
    	    	else if (c == '9')
    	    	    styl = T_A9;
    	    	else if (c == 'I')
    	    	    styl = T_AI;
    	    	else
    	    	    goto OPTS_ERR;
    	    	break;
    	    case 'B':
    	    	styl = T_B;
    	    	break;
    	    case 'C':
    	    	styl = T_C;
    	    	break;
    	    case 'G':
    	    	styl = T_TR;
    	    	break;
    	    case 'N':
    	    	styl = T_N;
    	    	break;
    	    case 'E':
    	    	strncpy(dcname,p,32);
    	    	strcat(dcname," ");
    	    	dc = dcname;
    	    	break;
    	    case 'D':
    	    	digitFlg = (*p != '-' && *p != '0');
    	    	break;
    	    case 'Q':
    	    	if (*p == 0)
    	    	    cmtM = 1;
    	    	else
    	    	    cmtM = strtoul(p,NULL,10);
    	    	if (cmtM > 3)
    	    	    goto OPTS_ERR;
    	    	break;
    	    case 'K':
    	    	adrF = (*p != '-' && *p != '0');
    	    	break;
    	    case 'Y':
    	    	akiY = atoi(p);
    	    	break;
    	    case 'Z':
    	    	stdinFlg = (*p != '-' && *p != '0');
    	    	break;
    	    case 'M':
    	    	imF = 0;
    	    	wdMd = 0;
    	    	c = (char)toupper(*p);
    	    	if (c == 'B') {
    	    	    ;
    	    	} else if (c == 'W') {
    	    	    wdMd = 1;
    	    	} else if (c == 'D') {
    	    	    wdMd = 2;
    	    	}
    	    	break;
    	    case 'I':
    	    	imF = 1;
    	    	wdMd = 0;
    	    	c = (char)toupper(*p);
    	    	if (c == 'B') {
    	    	    ;
    	    	} else if (c == 'W') {
    	    	    wdMd = 1;
    	    	} else if (c == 'D') {
    	    	    wdMd = 2;
    	    	}
    	    	break;
    	  #if 1 /* �Â��I�v�V����... �����̌Â��o�b�`�΍�^^; */
    	    case 'W':
    	    	wdMd = 1;
    	    	c = (char)toupper(*p);
    	    	if (c == 'I') {
    	    	    imF = 1;
    	    	} else if (c == 'M') {
    	    	    imF = 0;
    	    	} else if (c == 'W') {
    	    	    wdMd = 2;
    	    	    ++p;
    	    	    c = (char)toupper(*p);
    	    	    if (c == 'I') {
    	    	    	imF = 1;
    	    	    } else if (c == 'M') {
    	    	    	imF = 0;
    	    	    }
    	    	} else if (c == '-') {
    	    	    wdMd = 0;
    	    	}
    	    	break;
    	  #endif
    	    case '\0':
    	    case '?':
    	    	Usage();
    	    default:
    	      OPTS_ERR:
    	    	EP("Bad option!\n");
    	    	exit(1);
    	    }
    	}
    }
    if (n == 0)
    	Usage();
    if (styl == 0xff) {
    	styl = T_N;
    	adrF = 1;
    	cmtM = 1;
    }
    if (adrF)
    	tspcCnt = 0;
    if (stdinFlg == 0) {
    	for ( i = 1; i < argc; i++ ) {
    	    p = argv[i];
    	    if (*p == '-')
    	    	continue;
    	    fp = fopen(p, WRTMODE);
    	    if(fp == NULL)
    	    	FileErr();
    	    if (One(fp,linSiz,tspcCnt,styl,wdMd,imF,digitFlg,adrF,rcnt,sizz,dc,akiY,cmtM))
    	    	FileErr();
    	    fclose(fp);
    	}
    } else {
      #ifdef FSETBIN
    	fsetbin(stdin);
      #endif
    	if (One(stdin,linSiz,tspcCnt,styl,wdMd,imF,digitFlg,adrF,rcnt,sizz,dc,akiY,cmtM))
    	    FileErr();
    }
    return 0;
}
