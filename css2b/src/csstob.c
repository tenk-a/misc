/* ------------------------------------------------------------------------ */
/*  	    	    	    	    	    	    	    	    	    */
/* C�\�[�X���� �^�^ �R�����g�� �^���R�����g���^ �ɕϊ�����  	    	    */
/* 1996/04/22 v1.00 writen by M.Kitamura    	    	    	    	    */
/* 2000/09/03 v1.01 -y �ǉ�. �G���[�o�͂� stderr ���f�t�H���g�ɁB   	    */
/* ------------------------------------------------------------------------ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <stdarg.h>

typedef unsigned char  UCHAR;
typedef unsigned       UINT;
typedef unsigned short USHORT;
typedef unsigned long  ULONG;

#define STREND(p)   ((p)+strlen(p))

/*#define ISKANJI(c)	((unsigned)((c)^0x20) - 0xA1 < 0x3C) */
#define ISKANJI(c)  ((UCHAR)(c) >= 0x81 && ( (UCHAR)(c) <= 0x9F || ((UCHAR)(c) >= 0xE0 && (UCHAR)(c) <= 0xFC)))
#define ISKANJI2(c) ((UCHAR)(c) >= 0x40 && (UCHAR)(c) <= 0xfc && (c) != 0x7f)

#ifndef DOS16	// 32�r�b�g
#define FIL_NMSZ    4096
#define LBUFSIZE    (16*1024)
#else	    	// 16�r�b�g�� MS-DOS �̂Ƃ�
#define FIL_NMSZ    260
#define LBUFSIZE    (8*1024)
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
int  opt_sjis = 1;


void RdErrPuts(char *msg)
{
    fprintf(err_fp, "%s %ld : %s", src_name, src_line, msg);
}

char *FgetStr(char *buf, long len, FILE *fp)
{
    char *p;
    int i, c;

  #if 0
    if (len <= 0) {
    	RdErrPuts("PRG:programer's error(FgetsStr)\n");
    	exit(1);
    }
  #endif

    --len;
    p = buf;
    i = 0;
    do {
    	if (i++ == len) {
    	    src_lineContinueFlg = 1;
    	    RdErrPuts("1�s����������\n");
    	    break;
    	}
    	c = fgetc(fp);
    	if (ferror(fp) || feof(fp)) {
    	    buf[0] = 0;
    	    return NULL;
    	}
    	if (c == '\0') {
    	    RdErrPuts("�s���� '\\0' ���������Ă���\n");
    	    c = ' ';
    	}
    	*p++ = c;
    } while (c != '\n');
    *p = 0;
    return buf;
}

void Conv(char *srcname, char *dstname)
{
    static  char buf[LBUFSIZE+8];
    FILE    *fp,*ofp;
    char    *p;
    int     c, mode;

    if (srcname) {
    	fp = fopen(srcname, "r");
    	if (fp == NULL) {
    	    err_exit("�t�@�C�� '%s' ���I�[�v���ł��܂���\n", srcname);
    	}
    } else {
    	srcname = "<stdin>";
    	fp = stdin;
    }
    if (dstname) {
    	ofp = fopen(dstname, "w");
    	if (ofp == NULL) {
    	    err_exit("�t�@�C�� '%s' ���쐬�ł��܂���\n", dstname);
    	}
    } else {
    	dstname = "<stdout>";
    	ofp = stdout;
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
    	if (FgetStr(buf, LBUFSIZE, fp) == NULL) {
    	    break;
    	}
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
    	    } else if (opt_sjis && ISKANJI(c)) {
    	    	c = *p++;
    	    	if (ISKANJI2(c) == 0) {
    	    	    RdErrPuts("�s���ȑS�p������\n");
    	    	    --p;
    	    	}
    	    } else if (c == '\\') {
    	    	c = *p++;
    	    	if (c == '\0')
    	    	    break;
    	    	if (opt_sjis && ISKANJI(c))
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
    	    	    char *s;
    	    	    s = STREND(p);
    	    	    if (s[-1] == '\n') {
    	    	    	*p = '*';
    	    	    	if (p[1] == '/') {
    	    	    	    p++;
    	    	    	    *p = ' ';
    	    	    	}
    	    	      #if 10
    	    	    	c = 0;
    	    	    	while (*p) {
    	    	    	    if ((*p == '/' && p[1] == '*')||(*p == '*' && p[1] == '/')) {
    	    	    	    	if (STREND(p)+2 >= buf+(sizeof buf)-6) {
    	    	    	    	    RdErrPuts("//�R�����g���� /* */ �̏����̓r���łP�s�������Ȃ肷����\n");
    	    	    	    	    break;
    	    	    	    	}
    	    	    	    	p++;
    	    	    	    	memmove(p+1,p,strlen(p)+1);
    	    	    	    	*p = ' ';
    	    	    	    	s = STREND(p);
    	    	    	    	c = 1;
    	    	    	    }
    	    	    	    p++;
    	    	    	}
    	    	    	if (c)
    	    	    	    RdErrPuts("//�R�����g���� /* �� */ ��������\n");
    	    	      #endif
    	    	    	strcpy(s - 1, " */\n");
    	    	    } else {
    	    	    	RdErrPuts("//�����������s����������悤�Ȃ̂ŕϊ��ł��Ȃ�\n");
    	    	    }
    	    	    break;
    	    	}
    	    }
    	}
    	fputs(buf, ofp);
    	if (ferror(ofp)) {
    	    err_exit("%s %d : �����݃G���[����\n", dstname, src_line);
    	}
    }
    if (ferror(fp)) {
    	RdErrPuts("�Ǎ��݃G���[����\n");
    	exit(1);
    }
    if (fp != stdin)
    	fclose(fp);
    if (ofp != stdout)
    	fclose(ofp);
}


/* ------------------------------------------------------------------------ */
char *FIL_BaseName(char *adr)
    /* �p�X�����̃t�@�C�����ʒu��T��(MS-DOS�ˑ�) */
{
    char *p;

    p = adr;
    while (*p != '\0') {
    	if (*p == ':' || *p == '/' || *p == '\\')
    	    adr = p + 1;
    	if (opt_sjis && ISKANJI((*(unsigned char *)p)) && *(p+1) )
    	    p++;
    	p++;
    }
    return adr;
}

char *FIL_ChgExt(char filename[], char *ext)
    /* �t�@�C�����̊g���q��t���ւ��� */
{
    char *p;

  #ifndef DOS16
    strcat(filename, ext);
  #else
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
    /* �t�@�C�����Ɋg���q���Ȃ���Εt������ */
{
    if (strrchr(FIL_BaseName(filename), '.') == NULL) {
    	strcat(filename,".");
    	strcat(filename, ext);
    }
    return filename;
}
#endif

char *FIL_GetExt(char filename[])
    /* �g���q�ʒu�𓾂� */
{
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
    puts(
    	"\n"
    	"usage> csstob [-opts] srcfile newfile   # v1.01 by �Ă񂩁�\n"
    	"        C�\�[�X���� // �R�����g�� /* �R�����g */ �ɕϊ�����\n"
    	"opts:\n"
    	"  -x[EXT]  �w��t�@�C���͈�ŐV�����t�@�C�������̖��ŏo��\n"
    	"           ���A���t�@�C���͊g���q�� .EXT �ɂ���\n"
    	"           (EXT�ȗ���:bak)\n"
    	"  -j       �w��t�@�C��������͂����e�L�X�g�Ɋւ��đS�p����\n"
    	"           (�V�t�gJIS)���l������i�f�t�H���g�j\n"
    	"  -j-      �S�p���l�����Ȃ�\n"
    	"  -y[FILE] �G���[���b�Z�[�W��FILE�ɏo��. -y�݂̂Ȃ�W���o��\n"
    	);
    exit(1);
}

int main(int argc, char *argv[])
{
    static char *dstname = NULL, *srcname = NULL, *ext = NULL, *errname = NULL;
    int i,c;
    char *p;

    if (argc < 2)
    	Usage();

    for (i = 1; i < argc; i++) {
    	p = argv[i];
    	if (*p == '-') {
    	    p++, c = *p++, c = toupper(c);
    	    switch(c) {
    	    case 'J':
    	    	opt_sjis = 1;
    	    	if (*p == '-')
    	    	    opt_sjis = 0;
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
    	}
    }

    err_init(errname);

    if (ext == NULL) {
      #if 0
    	if (srcname == NULL || dstname == NULL) {
    	    err_exit("\n���̓t�@�C���Əo�̓t�@�C�����w�肵�Ă�������\n\n");
    	}
      #endif
    	if (srcname && dstname && stricmp(srcname, dstname) == 0) { /* �����₩�ȓ����`�F�b�N */
    	    err_exit("���̓t�@�C�����Əo�̓t�@�C�����������ł�\n");
    	}
    	Conv(srcname, dstname);

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
    	Conv(srcname, tmpname);
    	FIL_ChgExt(strcpy(bakname,srcname), ext);
    	remove(bakname);
    	if (rename(srcname, bakname) < 0)
    	    eprintf("%s �� %s ��rename�ł��܂���ł���\n",srcname, bakname);
    	else if (rename(tmpname, srcname) < 0)
    	    eprintf("%s �� %s ��rename�ł��܂���ł���\n",srcname, bakname);
    }
    return 0;
}
