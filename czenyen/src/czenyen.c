/* ------------------------------------------------------------------------ */
/* czenyen v1.01  c �\�[�X���� \ ���܂ޑS�p�����̃`�F�b�N���ϊ�     	    */
/*  	    	    	    	    	    	    	    	    	    */
/* 1996/04/26 v1.00 writen by M.Kitamura    	    	    	    	    */
/* 1996/04/27 v1.01 -s -n �I�v�V�����ǉ�. �R�����g�s�������t��.     	    */
/* 2000/08/13 v1.10 -na,-nb ��ǉ�  	    	    	    	    	    */
/* 2000/09/03 v1.11 -y,-ns��ǉ�. �G���[�o�͂�stderr���f�t�H���g��. 	    */
/* ------------------------------------------------------------------------ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <io.h>

typedef unsigned char  UCHAR;
typedef unsigned       UINT;
typedef unsigned short USHORT;
typedef unsigned long  ULONG;
/*#define ISKANJI(c)	((unsigned)((c)^0x20) - 0xA1 < 0x3C) */
#define ISKANJI(c)  ((UCHAR)(c) >= 0x81 && ( (UCHAR)(c) <= 0x9F || ((UCHAR)(c) >= 0xE0 && (UCHAR)(c) <= 0xFC)))
#define ISKANJI2(c) ((UCHAR)(c) >= 0x40 && (UCHAR)(c) <= 0xfc && (c) != 0x7f)

#ifndef DOS16
#define FIL_NMSZ    4096
#define LBUFSIZE    (8*1024)
#else	// 16�r�b�g�� MS-DOS �̂Ƃ�
#define FIL_NMSZ    260
#define LBUFSIZE    (1*1024)
#endif


/* ------------------------------------------------------------------------ */

#define STDERR	    stderr

FILE *err_fp   = NULL;

void eprintf(char *fmt, ...)
{
    va_list app;

    if (err_fp == NULL) {
    	err_fp = STDERR;
    }
    va_start(app, fmt);
    vfprintf(err_fp, fmt, app);
    va_end(app);
}


void err_exit(char *fmt, ...)
{
    va_list app;

    if (err_fp == NULL) {
    	err_fp = STDERR;
    }
    va_start(app, fmt);
    vfprintf(err_fp, fmt, app);
    va_end(app);
    exit(1);
}


void err_init(char *errname)
{
    if (errname) {
    	err_fp = stdout;    // �t�@�C�������Ȃ���΃G���[��W���o��
    	if (errname[0]) {
    	    err_fp = fopen(errname, "a+");
    	    if (err_fp == NULL) {
    	    	err_exit("�G���[�t�@�C�� %s ���I�[�v���ł��܂���ł���\n", errname);
    	    }
    	}
    } else {
    	err_fp = STDERR;
    }
}



/* ------------------------------------------------------------------------ */
static char *src_name;
static long src_line;
static char src_lineContinueFlg = 0;


void ErPrintf(char *fmt, ...)
{
    /* �ϊ����ɖ��̂������s��\�����邽�߂�printf */
    va_list app;

    va_start(app, fmt);
    fprintf(STDERR, "%s %d : ", src_name, src_line);
    vfprintf(STDERR, fmt, app);
    va_end(app);
}



char *FgetStr(char *buf, long len, FILE *fp)
{
    /* ��s����. '\0'���������Ă��邩�̃`�F�b�N������ */
    char *p;
    int i, c;

  #if 0
    if (len <= 0) {
    	ErPrintf("PRG:programer's error(FgetsStr)\n");
    	exit(1);
    }
  #endif

    --len;
    p = buf;
    i = 0;
    do {
    	if (i++ == len) {
    	    src_lineContinueFlg = 1;
    	    ErPrintf("1�s����������\n");
    	    break;
    	}
    	c = fgetc(fp);
    	if (ferror(fp) || feof(fp)) {
    	    buf[0] = 0;
    	    return NULL;
    	}
    	if (c == '\0') {
    	    ErPrintf("�s���� '\\0' ���������Ă���\n");
    	    c = ' ';
    	}
    	*p++ = c;
    } while (c != '\n');
    *p = 0;
    return buf;
}



void Conv(char *srcname, char *dstname, int outMode, int cmtFlg, int chkMode, int mszenMode)
{
    static  char buf[LBUFSIZE*4+1024];
    FILE    *fp,*ofp;
    char    *p, *s;
    int     c, k, mode, sptFlg;

    //printf("%s,%s, %d,%d,%d,%d\n", srcname, dstname, outMode,cmtFlg, chkMode, mszenMode);
    sptFlg  = ((outMode & 0x80) != 0);
    outMode = outMode & 0x7f;
    if (srcname) {
    	fp = fopen(srcname, "rt");
    	if (fp == NULL) {
    	    err_exit("�t�@�C�� '%s' ���I�[�v���ł��܂���\n", srcname);
    	}
    } else {
    	srcname = "<stdin>";
    	fp = stdin;
    }
    if (outMode) {
    	if (dstname) {
    	    ofp = fopen(dstname, "wt");
    	    if (ofp == NULL) {
    	    	err_exit("�t�@�C�� '%s' ���쐬�ł��܂���\n", dstname);
    	    }
    	} else {
    	    dstname = "<stdout>";
    	    ofp = stdout;
    	}
    }
    src_name = srcname;
    src_line = 0;
    src_lineContinueFlg = 0;
    mode = 0;
    for (; ;) {
    	++src_line;
    	if (src_lineContinueFlg) {
    	    src_lineContinueFlg = 0;
    	    --src_line;
    	}
    	if (FgetStr(buf, LBUFSIZE, fp) == NULL)
    	    break;
    	p = buf;
    	for (; ;) {
    	    c = *p++;
    	    if (c == '\0') {
    	    	break;
    	    } else if (c == '"') {
    	    	if (mode == 0)
    	    	    mode = '"';
    	    	else if (mode == '"')
    	    	    mode = 0;
    	    } else if (c == '\'') {
    	    	if (mode == 0)
    	    	    mode = '\'';
    	    	else if (mode == '\'')
    	    	    mode = 0;
    	    } else if (ISKANJI(c) && mszenMode) {
    	    	k = *p++;
    	    	if (ISKANJI2(k) == 0) {
    	    	    ErPrintf("�s���ȑS�p������\n");
    	    	    --p;
    	    	} else /*if (mode)*/ {
    	    	    if (k == '\\') {
    	    	    	if (*p == '\\') {
    	    	    	    s = "\\";
    	    	    	    if (p[1] == 'n') {
    	    	    	    	s = "\\n";
    	    	    	    	if (p[2] == '"')
    	    	    	    	    s = "\\n\"";
    	    	    	    }
    	    	    	    if (chkMode == 0)
    	    	    	    	ErPrintf("<<�x��>> \\ ���܂ޑS�p[%c%c]�̒���� %s ������\n", c,k, s);
    	    	    	    else
    	    	    	    	ErPrintf("\\ ���܂ޑS�p[%c%c]�̒���� %s ������\n", c,k, s);
    	    	    	    if ((mode == '\'' || mode == '"') && outMode == 2) { /* �폜 */
    	    	    	    	memmove(p, p+1, strlen(p+1)+1);
    	    	    	    	eprintf("\t\t\t���� \\ ���폜\n");
    	    	    	    }
    	    	    	} else if (*p == '\n') {
    	    	    	    if (mode == '/' || mode == '*') {
    	    	    	    	ErPrintf("<<�x��>> �R�����g���A\\ ���܂ޑS�p[%c%c]�̒���ɉ��s������\n", c,k);
    	    	    	    	if (outMode == 1 || outMode == 3) {
    	    	    	    	    eprintf("\t\t\t[%c%c]�̌��ɑS�p�󔒂�}��\n", c,k);
    	    	    	    	    memmove(p+2, p, strlen(p)+1);
    	    	    	    	    memcpy(p, "�@",2);
    	    	    	    	    p += 2;
    	    	    	    	}
    	    	    	    } else {
    	    	    	    	ErPrintf("<<�x��>> \\ ���܂ޑS�p[%c%c]�̒���ɉ��s������\n", c,k);
    	    	    	    }
    	    	    	} else if (*p == '"') {
    	    	    	    if (cmtFlg || mode == '"' || mode == '\'')
    	    	    	    	ErPrintf("<<�x��>> \\ ���܂ޑS�p[%c%c]�̒���� \" ������\n", c,k);
    	    	    	} else if (*p == '\'') {
    	    	    	    if (cmtFlg || mode == '"' || mode == '\'')
    	    	    	    	ErPrintf("<<�x��>> \\ ���܂ޑS�p[%c%c]�̒���� ' ������\n", c,k);
    	    	    	} else if (*p == '*' && p[1] == '/') {
    	    	    	    	ErPrintf("<<�x��>> \\ ���܂ޑS�p[%c%c]�̒���� */ ������\n", c,k);
    	    	    	} else {
    	    	    	    if (cmtFlg || mode == '"' || mode == '\'') {
    	    	    	    	if (chkMode == 0)
    	    	    	    	    ErPrintf("\\ ���܂ޑS�p[%c%c]������\n", c,k);
    	    	    	    	else
    	    	    	    	    ErPrintf("<<�x��>> \\ ���܂ޑS�p[%c%c]������\n", c,k);
    	    	    	    }
    	    	    	}
    	    	    	if (mode == '\'' || mode == '"') {
    	    	    	    if (outMode == 1) { /* �ǉ� */
    	    	    	    	eprintf("\t\t\t[%c%c]�̌��� \\ ��}��\n", c,k);
    	    	    	    	memmove(p+1, p, strlen(p)+1);
    	    	    	    	*p++ = '\\';
    	    	    	    } else if (outMode == 3 || outMode == 4) {
    	    	    	    	char bf[10];
    	    	    	    	if (outMode == 3)
    	    	    	    	    eprintf("\t\t\t[%c%c]�� \\x%02x\\x%02x �ɕϊ�\n", c,k, (UCHAR)c, (UCHAR)k);
    	    	    	    	if (sptFlg && isxdigit(*p)) {
    	    	    	    	    memmove(p-2+10, p, strlen(p)+1);
    	    	    	    	    sprintf(bf,"\\x%02x\\x%02x\"\"",c&0xff,k&0xff);
    	    	    	    	    memcpy(p-2,bf,10);
    	    	    	    	    p = p -2 + 10;
    	    	    	    	} else {
    	    	    	    	    memmove(p-2+8, p, strlen(p)+1);
    	    	    	    	    sprintf(bf,"\\x%02x\\x%02x",c&0xff,k&0xff);
    	    	    	    	    memcpy(p-2,bf,8);
    	    	    	    	    p = p -2 + 8;
    	    	    	    	}
    	    	    	    }
    	    	    	}
    	    	    } else if (outMode == 4) {	// �S�p�����Ȃ�΁A\x??\x?? �̌`�ŏo��
    	    	    	char bf[10];
    	    	    	//eprintf("\t\t\t[%c%c]�� \\x%02x\\x%02x �ɕϊ�\n", c,k, (UCHAR)c, (UCHAR)k);
    	    	    	if (sptFlg && isxdigit(*p)) {
    	    	    	    memmove(p-2+10, p, strlen(p)+1);
    	    	    	    sprintf(bf,"\\x%02x\\x%02x\"\"",c&0xff,k&0xff);
    	    	    	    memcpy(p-2,bf,10);
    	    	    	    p = p -2 + 10;
    	    	    	} else {
    	    	    	    memmove(p-2+8, p, strlen(p)+1);
    	    	    	    sprintf(bf,"\\x%02x\\x%02x",c&0xff,k&0xff);
    	    	    	    memcpy(p-2,bf,8);
    	    	    	    p = p -2 + 8;
    	    	    	}
    	    	    }
    	    	}
    	    } else if (c >= 0x80 && outMode == 4) { // 0x80 �ȏ�̕����Ȃ�� \x?? �̌`�ŏo��
    	    	char bf[10];
    	    	//eprintf("\t\t\t[%c]�� \\x%02x �ɕϊ�\n", c,k, (UCHAR)c);
    	    	if (sptFlg && isxdigit(*p)) {
    	    	    memmove(p-1+6, p, strlen(p)+1);
    	    	    sprintf(bf,"\\x%02x\"\"",c&0xff);
    	    	    memcpy(p-1,bf,6);
    	    	    p = p -1 + 6;
    	    	} else {
    	    	    memmove(p-1+4, p, strlen(p)+1);
    	    	    sprintf(bf,"\\x%02x",c&0xff);
    	    	    memcpy(p-1,bf,4);
    	    	    p = p -1 + 4;
    	    	}
    	    } else if (c == '\\') {
    	    	c = *p++;
    	    	if (c == '\0')
    	    	    break;
    	    	if (ISKANJI(c))
    	    	    --p;
    	    } else if (mode == '*' && c == '*') {
    	    	if (*p == '/') {
    	    	    p++;
    	    	    mode = 0;
    	    	}
    	    } else if (mode == 0 && c == '/') {
    	    	if (*p == '*') {
    	    	    p++;
    	    	    mode = '*';
    	    	} else if (*p == '/') {
    	    	    p++;
    	    	    mode = '/';
    	    	    break;
    	    	}
    	    }
    	}
    	if (mode == '/')
    	    mode = 0;
    	if (outMode) {
    	    fputs(buf, ofp);
    	    if (ferror(ofp)) {
    	    	err_exit("%s %d : �����݃G���[����\n", dstname, src_line);
    	    }
    	}
    }
    if (ferror(fp)) {
    	ErPrintf("�Ǎ��݃G���[����\n");
    	exit(1);
    }
    if (fp != stdin)
    	fclose(fp);
    if (outMode && ofp != stdout) {
    	fclose(ofp);
    }
}



/* ------------------------------------------------------------------------ */

char *FIL_BaseName(char *adr)
{
    /* �p�X�����̃t�@�C�����ʒu��T��(MS-DOS�ˑ�) */
    char *p;

    p = adr;
    while (*p != '\0') {
    	if (*p == ':' || *p == '/' || *p == '\\')
    	    adr = p + 1;
    	if (ISKANJI((*(unsigned char *)p)) && *(p+1) )
    	    p++;
    	p++;
    }
    return adr;
}


char *FIL_ChgExt(char filename[], char *ext)
{
    /* �t�@�C�����̊g���q��t���ւ��� */
    char *p;

  #ifndef DOS16 /* �����O�t�@�C�����̎� */
    strcat(filename, ext);
  #else     	/* DOS�� 8.3����������Ƃ� */
    p = FIL_BaseName(filename);
    p = strrchr( p, '.');
    if (p == NULL) {
    	strcat(filename,".");
    	strcat( filename, ext);
    } else {
    	strcpy(p+1, ext);
    }
  #endif
    return filename;
}


#if 0
char *FIL_AddExt(char filename[], char *ext)
{
    /* �t�@�C�����Ɋg���q���Ȃ���Εt������ */
    if (strrchr(FIL_BaseName(filename), '.') == NULL) {
    	strcat(filename,".");
    	strcat(filename, ext);
    }
    return filename;
}
#endif


char *FIL_GetExt(char filename[])
{
    /* �g���q�ʒu�𓾂� */
    char *p;

    p = FIL_BaseName(filename);
    p = strrchr( p, '.');
    if (p)
    	p++;
    return p;
}



/* ------------------------------------------------------------------------ */

void Usage(void)
{
    eprintf("usage> czenyen [-opts] srcfile [newfile]           //  v1.11 by �Ă񂩁�\n"
    	   "        C�\�[�X�� \" �� ' �ň͂܂ꂽ���ɂ��� \\ ���܂ޑS�p�����̌������ϊ�\n"
    	   "opts:\n"
    	   "  -t       �e�X�g: \\ ���܂ޑS�p����������s�𒲂ׂ�(-a,-n�̊m�F�p)\n"
    	   "  -s       �e�X�g: \\ ���܂ޑS�p�����̒���� \\ �����邩���ׂ�(-d�̊m�F�p)\n"
    	   "  -a       �ϊ�: \\ ���܂ޑS�p�����̒���� \\ ��t��\n"
    	   "  -d       �ϊ�: \\ ���܂ޑS�p�����̒���� \\ ���폜\n"
    	   "  -n       �ϊ�: \\ ���܂ޑS�p������\\x??\\x5c�ɕϊ�\n"
    	   "  -na      �ϊ�: ���ׂĂ̑S�p�����Ɣ��p�J�i�� \\x?? �ɕϊ�\n"
    	   "  -nb      �ϊ�: �����R�[�h�� 0x80 �ȏ�̕����� \\x?? �ɕϊ�\n"
    	   "  -ns      -n -na -nb��\\x??���̒����16�i������������ꍇ\"\"�ŕ�����𕪊�\n"
    	   "  -k       -t -s �� \"�� '�͈̔͂����łȂ� // /* */�R�����g���̌��ʂ��\��\n"
    	   "  -x[EXT]  �w��t�@�C���͈��,���̖��̃t�@�C����Ǎ��݂��̖��ŏo��. ��\n"
    	   "           �t�@�C���͊g���q�� .EXT �ɂ���(EXT�ȗ���:bak)\n"
    	   "\n"
    	   "  �I�v�V����������̏ꍇ�A�f�t�H���g�� -t ���s��.\n"
    	   "  �ϊ�(-a -d -n) ���s���ꍇ�́A[newfile]���w�肷�邩 -x ���w�肷�邱��.\n"
    	   "  �ϊ��� ' �� \" �ň͂܂ꂽ�͈݂͂̂� // /* */ �R�����g���͍s��Ȃ����A\n"
    	   "  ���s�̒��O�� \\ ���܂ޑS�p�̂���ꍇ�͑S�p�󔒂�ǉ�����(-a -n��).\n"
    );
    exit(1);
}



int main(int argc, char *argv[])
{
    char *dstname = NULL, *srcname = NULL, *ext = NULL, *errname = NULL;
    int i,c, outMode, cmtFlg, chkMode, mszenMode;
    char *p;

    if (argc < 2)
    	Usage();

    cmtFlg = outMode = chkMode = 0;
    mszenMode = 1;
    for (i = 1; i < argc; i++) {
    	p = argv[i];
    	if (*p == '-') {
    	    p++; c = *p++; c = toupper(c);
    	    switch(c) {
    	    case 'A':
    	    	chkMode = 0;
    	    	outMode = 1;
    	    	break;
    	    case 'D':
    	    	chkMode = 1;
    	    	outMode = 2;
    	    	break;
    	    case 'T':
    	    	chkMode = 0;
    	    	outMode = 0;
    	    	break;
    	    case 'S':
    	    	chkMode = 1;
    	    	outMode = 0;
    	    	break;
    	    case 'N':
    	    	if (*p == 'a' || *p == 'A') {
    	    	    chkMode = 0;
    	    	    outMode = 4;
    	    	} else if (*p == 'b' || *p == 'B') {
    	    	    chkMode = 0;
    	    	    outMode = 4;
    	    	    mszenMode = 0;
    	    	} else if (*p == 's' || *p == 'S') {
    	    	    outMode |= 0x80;
    	    	} else {
    	    	    chkMode = 0;
    	    	    outMode = 3;
    	    	}
    	    	break;
    	    case 'K':
    	    	cmtFlg = 1;
    	    	break;
    	    case 'X':
    	    	ext = p;
    	    	if (*p == '\0')
    	    	    ext = "BAK";
    	    	break;
    	    case 'Y':
    	    	errname = p;
    	    	break;
    	    case '\0':
    	    	break;
    	    case 'H':
    	    case '?':
    	    	Usage();
    	    default:
    	    	err_exit("�R�}���h���C���E�I�v�V�������������� : %s\n", argv[i]);
    	    }
    	} else if (srcname == NULL) {
    	    srcname = p;
    	} else if (dstname == NULL) {
    	    dstname = p;
      #if 0
    	} else if (ext == NULL) {
    	    eprintf("�t�@�C������ 3�ȏ�w�肳��Ă��܂�\n");
      #endif
    	}
    }

    err_init(errname);

    if (outMode == 0 || ext == NULL) {
      #if 0
    	if (srcname == NULL) {
    	    err_exit("\n���̓t�@�C�����w�肵�Ă�������\n\n");
    	}
    	if (outMode && dstname == NULL) {
    	    err_exit("\n�o�̓t�@�C�����w�肵�Ă�������\n\n");
    	}
      #endif
    	if (outMode && srcname && dstname && stricmp(srcname, dstname) == 0) {	/* �����₩�ȓ����`�F�b�N */
    	    err_exit("���̓t�@�C�����Əo�̓t�@�C�����������ł�\n");
    	}
    	Conv(srcname, dstname, outMode, cmtFlg, chkMode, mszenMode);

    } else {	/* MS-DOS�ˑ��ȃt�@�C������... */
    	static char tmpname[FIL_NMSZ];
    	static char bakname[FIL_NMSZ];

    	if (srcname == NULL || dstname) {
    	    err_exit("\n-x�w�莞�͓��̓t�@�C���͂P�̂ݎw�肵�Ă�������\n\n");
    	}
    	if (strlen(srcname) >= (sizeof tmpname)-1) {
    	    err_exit("�p�X�����������܂� '%s'\n", srcname);
    	}
    	if (stricmp(ext,FIL_GetExt(srcname)) == 0) {
    	    err_exit("���̓t�@�C���̊g���q�Əo�̓t�@�C���̊g���q�������ł�\n");
    	}
    	strcpy(tmpname, srcname);
    	strcpy(FIL_BaseName(tmpname), "ccmntss_.tmp");
    	Conv(srcname, tmpname, outMode, cmtFlg, chkMode, mszenMode);
    	FIL_ChgExt(strcpy(bakname,srcname), ext);
    	remove(bakname);
    	if (rename(srcname, bakname) < 0)
    	    eprintf("%s �� %s ��rename�ł��܂���ł���\n",srcname, bakname);
    	else if (rename(tmpname, srcname) < 0)
    	    eprintf("%s �� %s ��rename�ł��܂���ł���\n",srcname, bakname);
    }
    return 0;
}
