/*
	�t�@�C���Ǎ��݃��[�`��
	���@�}�N������
    by tenk* 1996-2000
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "Filn.h"


#define STDERR		stderr
int err_printf(char *fmt, ...);

char 	*appName;

static void Usage(void)
{
	err_printf("\n�g��/����b�v���v���Z�b�T acpp v0.95 (" __DATE__ ")  by tenk*\n");
	err_printf(" usage> %s [-options] filename [-options]\n", appName);
	err_printf("  -d[NAME]  %cdefine NAME  1  ������\n", Filn->mac_chr);
	err_printf("  -u[NAME]  %cundef  NAME ������\n", Filn->mac_chr);
	err_printf(
		   "  -?  -h    �w���v\n"
		   "  -i[DIR]   include ���ɑF������f�B���N�g��\n"
		   "  -o[FILE]  �o�͂� FILE �ɂ���\n"
		   "  -e[FILE]  �G���[�o�͂� FILE �ɂ���\n"
		   "  -s        ���\�[�X���R�����g�Ƃ��ďo�͂��Ȃ�(-pc0 �ɓ���)\n"
		   "  -w        ����\n"
		   "  -p...     -p�Ŏn�܂�I�v�V����\n"
		   "  -p?       -p�Ŏn�܂�I�v�V�����̃w���v\n"
	);
	err_printf("  -c[FILE]  FILE[.CFG] �� %s �̂���f�B���N�g������ǂݍ���\n", appName);
	err_printf("  �� �w��Ȃ��Ƃ� %s �Ɠ����t�H���_�ɂ��� .cfg�t�@�C�� ��Ǎ���\n", appName);
	exit(1);
}



/* ------------------------------------------------------------------------ */
typedef struct SLIST {
	struct SLIST	*link;
	char			*s;
} SLIST;

int				debugflag;

static SLIST	*fileListTop = NULL;
static int  	cmtchr_i=0,cmtcht_i=0;
static char 	*outname = NULL;
static char 	*errname = NULL;


static void ResFileRead(char *name);
static void OptsInit(void);
static int  Opts(char *a);
static char *Pchk(int n);
static void UsageOptP(void);
static char *Pchk(int n);

SLIST *SLIST_Add(SLIST **p0, char *s);
int freeE(void *p);
char *strdupE(char *s);
void *callocE(size_t a, size_t b);
void *mallocE(size_t a);
char *FIL_DelLastDirSep(char *dir);
char *FIL_AddExt(char filename[], char *ext);
char *FIL_ChgExt(char filename[], char *ext);
char *FIL_BaseName(char *adr);
volatile void err_exit(char *fmt, ...);



/* ------------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
	static char name[2100/*FIL_NMSZ*/];
	int i;
	char *p;
	SLIST *fl;
	FILE *fp;

	appName = strdupE(FIL_BaseName(argv[0]));

	if (Filn_Init() == NULL) {		/* �\�[�X���̓��[�`���̏����� */
		err_exit("������������܂���\n");
		return 1;
	}
	OptsInit();

	/* �R���t�B�O�E�t�@�C���̓ǂݍ��� */
	FIL_ChgExt(strcpy(name, argv[0]), "cfg");
	ResFileRead(name);

	/* ������������΃w���v�\�� */
	if (argc < 2)
		Usage();

	/* �R�}���h���C����� */
	for (i = 1; i < argc; i++) {
		p = argv[i];
		if (*p == '-') {
			if (p[1] == 'C' || p[1] == 'c') {	/* �R���t�B�O�t�@�C���ǂݍ��� */
				strcpy(name, argv[0]);
				*FIL_BaseName(name) = '\0';
				strcat(name, p+2);
				FIL_AddExt(name,"CFG");
				ResFileRead(name);
			} else {
				Opts(p);
			}
		} else if ( *p == '@') {
			ResFileRead(p+1);
		} else {
			SLIST_Add(&fileListTop, p);
		}
	}

	if (fileListTop == NULL) {	// �t�@�C�����Ȃ���ΕW�����͂���
		//err_exit("�t�@�C�������w�肵�Ă�������\n");
		SLIST_Add(&fileListTop, NULL);
	}

	/* �G���[�o�͂̏��� */
	if (Filn_ErrReOpen(errname, stderr) == NULL) {
		err_exit("�G���[�o�̓t�@�C���̃I�[�v���Ɏ��s���܂���\n");
	}

	/* �\�[�X�o�̓X�g���[������ */
	if (outname) {
		fp = fopen(outname,"wt");
		if (fp == NULL) {
			err_exit("%s ���I�[�v���ł��܂���\n", outname);
		}
	} else {
		fp = stdout;
	}

	/* �S�Ẵt�@�C�������� */
	for (fl = fileListTop; fl != NULL; fl = fl->link) {
		p = fl->s;
		if (Filn_Open(p) < 0)
			exit(1);
		/* �}�N���W�J��̃\�[�X���o�� */
		for (;;) {
			p = Filn_Gets();
			if (p == NULL)
				break;
			fprintf(fp, "%s", p);
			freeE(p);
		}
	}

	/* �o�͐�̃N���[�Y */
	if (fp != stdout)
		fclose(fp);

	/* �\�[�X���̓��[�`���̏I�� */
	Filn_Term();

	return 0;
}



static void ResFileRead(char *name)
{
	/*���X�|���X�E�t�@�C������t�@�C����,�I�v�V���������o��*/
	FILE *fp;
	char bf[1024*2];
	char *p;
	int n;

	fp = fopen(name,"rt");
	if (fp == NULL)
		return;
	n = 0;
	while (fgets(bf,sizeof bf,fp) != NULL) {
		++n;
		p = strtok(bf, " \t\n");
		while (p && *p && *p != ';') {
			if (*p == '-')
				Opts(p);
			else
				SLIST_Add(&fileListTop, p);
			p = strtok(NULL, " \t\n");
		}
	}
	if (ferror(fp)) {
		printf("%s %d : ���[�h�G���[\n", name, n);
		exit(1);
	}
	fclose(fp);
}



static void OptsInit(void)
{
	Filn->opt_delspc 	= 0;		/* 0�ȊO�Ȃ�΋󔒂̈��k������			*/
	Filn->opt_dellf  	= 1;		/* 0�ȊO�Ȃ�΁����s�ɂ��s�A�����s��	*/
	Filn->opt_sscomment = 1;		/* 0�ȊO�Ȃ��//�R�����g���폜����		*/
	Filn->opt_blkcomment= 1;		/* 0�ȊO�Ȃ�΁^���R�����g���^���폜����*/
	Filn->opt_kanji		= 1;		/* 0�ȊO�Ȃ��MS�S�p�ɑΉ�				*/
	Filn->opt_sq_mode	= 1;		/* ' �� �y�A�ŕ����萔�Ƃ��Ĉ��� */
	Filn->opt_wq_mode	= 1;		/* " �� �y�A�ŕ�����萔�Ƃ��Ĉ��� */
	Filn->opt_mot_doll	= 0;		/* $ �� ���g���[���� 16�i���萔�J�n�����Ƃ��Ĉ��� */
	Filn->opt_oct 		= 1;		/* 1: 0����n�܂鐔���͂W�i��  0:10�i */

	Filn->opt_orgSrc	= 3;		/* 1:���̃\�[�X���R�����g�ɂ��ďo�� 2:TAG JUMP�`�� 3:#line file  0:�o�͂��Ȃ� */

	Filn->orgSrcPre		= ";";		/* ���\�[�X�o�͎��̍s���ɂ��镶����	*/
	Filn->orgSrcPost	= "";		/* ���\�[�X�o�͎��̍s���ɂ��镶����	*/
	Filn->immMode		= 0;		/* 1:�����t10�i 2:������10�i 3:0xFF 4:$FF X(5:0FFh) */
	Filn->cmt_chr[0] 	= 0;		/* �R�����g�J�n�����ɂȂ镶�� */
	Filn->cmt_chr[1] 	= 0;		/* �R�����g�J�n�����ɂȂ镶�� */
	Filn->cmt_chrTop[0] = 0;		/* �s���R�����g�J�n�����ɂȂ镶�� */
	Filn->cmt_chrTop[1] = 0;		/* �s���R�����g�J�n�����ɂȂ镶�� */
	Filn->macErrFlg		= 1;		/* �}�N�����̃G���[�s�ԍ����\�� 1:���� 0:���Ȃ� */
	Filn->mac_chr		= '#';		/* �}�N���s�J�n���� */
	Filn->mac_chr2		= '#';		/* �}�N���̓���W�J�w�蕶��.  */
	Filn->localPrefix	= "_LCL_";
	Filn->opt_yen		= 1;		/* \\������C�̂悤�Ɉ���Ȃ�. 1:���� 2:'"���̂�  3,4:�ϊ��������Ⴄ(����) */

	Filn->sym_chr_doll	= '$';
	Filn->sym_chr_atmk	= '@';
	Filn->sym_chr_qa	= '?';
	Filn->sym_chr_shp	= 0/*'#'*/;
	Filn->sym_chr_prd	= 0/*'.'*/;
}



static int Opts(char *a)
{
	/* �I�v�V�����̏��� */
	char *p;
	int c;

	p = a;
	p++, c = *p++, c = toupper(c);
	switch(c) {
	case 'D':
		if (Filn_SetLabel(p, NULL))
			goto OPT_ERR;
		break;
	case 'U':
		if (Filn_UndefLabel(p))
			goto OPT_ERR;
		break;
	case 'I':
		Filn_AddIncDir(strdupE(p));
		break;
	case 'O':
		outname = strdupE(p);
		break;
	case 'E':
		errname = strdupE(p);
		break;
	case 'S':
		Filn->opt_orgSrc = 0;
		break;
	case 'W':
		break;
	case 'P':
		c = *p++, c = toupper(c);
		switch(c) {
		case 'C':
			if (*p == 'S' || *p == 's') {
				p++;
				Filn->orgSrcPre = strdupE(p);
			} else if (*p == 'E' || *p == 'e') {
				p++;
				Filn->orgSrcPost = strdupE(p);
			} else {
				Filn->opt_orgSrc = strtoul(p,&p,10);
			}
			break;
		case 'D':
			if (*p == 0)
				Filn->immMode = 1;
			else if (*p == '-')
				Filn->immMode = 0;
			else
				Filn->immMode = (int)strtoul(p, NULL, 10);
			if (Filn->immMode > 4)
				goto OPT_ERR;
			break;
		case 'P':
			Filn->localPrefix = strdupE(p);
			break;
		case 'Y':
			Filn->opt_yen = (*p == '1' || *p == 0) ? 1 : (*p == '2') ? 2 : 0;
			if (*p++) {
				if (*p == 't' || *p == 'T') {
					Filn->opt_yen += 2;
				}
			}
			break;
		case 'T':
			Filn->opt_delspc = (*p == '-') ? 0 : 1;	/* 0�ȊO�Ȃ�΋󔒂̈��k������ */
			break;
		case 'F':
			Filn->opt_dellf  = (*p == '-') ? 0 : 1;	/* 0�ȊO�Ȃ�΁����s�ɂ��s�A�����s�� */
			break;
		case 'S':
			Filn->opt_sscomment = (*p == '-') ? 0 : 1;	/* 0�ȊO�Ȃ��//�R�����g���폜���� */
			break;
		case 'B':
			Filn->opt_blkcomment= (*p == '-') ? 0 : 1;	/* 0�ȊO�Ȃ�΁^���R�����g���^���폜���� */
			break;
		case 'J':
			Filn->opt_kanji	= (*p == '-') ? 0 : 1;		/* 0�ȊO�Ȃ��MS�S�p�ɑΉ� */
			break;
		case 'Q':
			if (*p == '0') {
				Filn->opt_sq_mode = 0;
			} else if (*p == '1') {
				Filn->opt_sq_mode = 1;
			} else if (*p == '2') {
				Filn->opt_sq_mode = 2;
			} else if (*p == 'W' || *p == 'w') {
				p++;
				if (*p == '0') {
					Filn->opt_wq_mode = 0;
				} else if (*p == '1') {
					Filn->opt_wq_mode = 1;
				} else if (*p == '2') {
					Filn->opt_wq_mode = 2;
				} else {
					goto OPT_ERR;
				}
			} else {
				goto OPT_ERR;
			}
			break;
		case 'R':
			c = toupper(*p); p++;
			if (c == 'Q') {
				Filn->opt_sq_mode = 2;
				if (*p == '-')
					Filn->opt_sq_mode = 1;
			} else if (c == 'D') {
				Filn->opt_mot_doll = 1;
				if (*p == '-')
					Filn->opt_mot_doll = 0;
			}
			break;
		case 'O':
			if (*p == '-') {
				Filn->opt_oct = 0;
			} else {
				Filn->opt_oct = 1;
			}
			break;
		case 'M':
			if (*p == 'M' || *p == 'm') {
				++p;
				Filn->mac_chr2 = *p;							/* �}�N���s�J�n���� */
			} else {
				Filn->mac_chr = *p;							/* �}�N���s�J�n���� */
				Filn->mac_chr2 = *p;							/* �}�N���s�J�n���� */
			}
			c = 0;
			goto N1;
		case 'L':
			c = *p;
			goto N1;
		case 'N':
			c = 0;
	 N1:
			if (*p == '#' || *p == '@' || *p == '?' || *p == '$' || *p == '.') {
				if (*p == '$')	Filn->sym_chr_doll = c;
				if (*p == '@')	Filn->sym_chr_atmk = c;
				if (*p == '?')	Filn->sym_chr_qa   = c;
				if (*p == '#')	Filn->sym_chr_shp  = c;
				if (*p == '.')	Filn->sym_chr_prd  = c;
			} else {
				err_exit("-pm,-pn,-pl�Ŏw��o���镶���� # @ ? $ . �̂����ꂩ�̂�\n");
			}
			break;
		case 'E':
			if (*p == 'T' || *p == 't') {
				if (cmtcht_i > 1)
					err_exit("-pet �̎w��� 2�܂�\n");
				p++;
				Filn->cmt_chrTop[cmtcht_i++] = *p;
			} else {
				if (cmtchr_i > 1)
					err_exit("-pe �̎w��� 3�܂�\n");
				Filn->cmt_chr[cmtchr_i++] = *p;
			}
			break;
		case 'Z':
			Filn->macErrFlg	= (*p == '-') ? 0 : 1;		/* �}�N�����̃G���[�s�ԍ����\�� 1:���� 0:���Ȃ� */
			break;

		case 'H':
		case '?':
		case '\0':
			UsageOptP();
		default:
			goto OPT_ERR;
		}
		break;

	case 'Z':
		debugflag = (*p == '-') ? 0 : 1;
		break;

	case '\0':	//�������Ȃ��B�I�v�V���������ŁA�W�����͂������Ƃ��p
		break;

	case 'H':
	case '?':
		Usage();
	default:
  OPT_ERR:
		err_exit("Incorrect command line option : %s\n", a);
	}
	return 0;
}



static char *Pchk(int n)
{
	return (n) ? "����" : "���Ȃ�";
}



static void UsageOptP(void)
{
	static char ac[] = "#$@?.", cmc[4] = "   ", cmc2[4] = "   ";
	static char *optYen[] = {"0","1","2","1t","2t",""};
	char *sc[5],*cc[5];
	int i;

	sc[0] = Filn->sym_chr_shp  ? " #":"";
	sc[1] = Filn->sym_chr_doll ? " $":"";
	sc[2] = Filn->sym_chr_atmk ? " @":"";
	sc[3] = Filn->sym_chr_qa   ? " ?":"";
	sc[4] = Filn->sym_chr_prd  ? " .":"";

	cc[0] = !Filn->sym_chr_shp  ? " #":"";
	cc[1] = !Filn->sym_chr_doll ? " $":"";
	cc[2] = !Filn->sym_chr_atmk ? " @":"";
	cc[3] = !Filn->sym_chr_qa   ? " ?":"";
	cc[4] = !Filn->sym_chr_prd  ? " .":"";

	cmc[0] = Filn->cmt_chr[0] ? Filn->cmt_chr[0] : ' ';
	cmc[2] = Filn->cmt_chr[1] ? Filn->cmt_chr[1] : ' ';
	cmc2[0] = Filn->cmt_chrTop[0] ? Filn->cmt_chrTop[0] : ' ';
	cmc2[2] = Filn->cmt_chrTop[1] ? Filn->cmt_chrTop[1] : ' ';

	for (i = 0; i < 5; i++) {
		if (ac[i] == Filn->mac_chr)
			cc[i] = "";
		if (ac[i] == Filn->mac_chr2)
			cc[i] = "";
	}

	printf("-p�Ŏn�܂�I�v�V�����F                                     ���݂̐ݒ�\n");
	printf("  -pm[C]    �}�N���J�n������ C �ɂ���.   C �� # $ @ ? .    %c\n", Filn->mac_chr);
	printf("  -pmm[C]   ##,#���x��,#(��)�ł�#��ύX. C �� # $ @ ? .    %c\n", Filn->mac_chr2);
	printf("  -pl[C]    C �����x���\�������Ƃ���.    C �� # $ @ ? .   %s%s%s%s%s\n",sc[0],sc[1],sc[2],sc[3],sc[4]);
	printf("  -pn[C]    C �����x���\�������Ƃ��Ȃ�.  C �� # $ @ ? .   %s%s%s%s%s\n",cc[0],cc[1],cc[2],cc[3],cc[4]);
	printf("  -pp[NAME] #local �Ő������郉�x���̃v���t�B�b�N�X        %s\n", Filn->localPrefix);
	printf("  -pc[N]    ���\�[�X���R�����g�ɂ��ďo�� 0:���Ȃ� 1:����   %d\n", Filn->opt_orgSrc);
	printf("            2:TAG�`���ł��� 3:# �s�ԍ� �t�@�C���� ���o��   \n");
	printf("  -pcs[STR] -pc1|2 ���A�R�����g�̍s���ɂ��镶����        %s\n", Filn->orgSrcPre);
	printf("  -pce[STR] -pc1|2 ���A�R�����g�̍s���ɂ��镶����        %s\n", Filn->orgSrcPost);
	printf("  -pe[C]    C ���R�����g�J�n�L�����Ƃ���(2��)            %s\n", cmc);
	printf("  -pet[C]   C ���s���݂̂̃R�����g�J�n�����Ƃ���(2��)    %s\n", cmc2);
	printf("  -py[N][t] \\�̓��ꈵ�� 0:�� 1:�L(''\"\"���̂�) t�t:�ϊ���   %s\n", optYen[Filn->opt_yen]);
	printf("  -ps[-]    //�R�����g���폜����           -ps- ���Ȃ�     %s\n", Pchk(Filn->opt_sscomment));
	printf("  -pb[-]    /*�R�����g*/���폜����         -pb- ���Ȃ�     %s\n", Pchk(Filn->opt_blkcomment));
	printf("  -pf[-]    \\���s�R�[�h�ɂ��s�A�������� -pf- ���Ȃ�      %s\n", Pchk(Filn->opt_dellf));
	printf("  -pt[-]    �󔒂̈��k������               -pt- ���Ȃ�     %s\n", Pchk(Filn->opt_delspc));
	printf("  -pj[-]    MS�S�p�ɑΉ�����               -pj- ���Ȃ�     %s\n", Pchk(Filn->opt_kanji));
	printf("  -po[-]    0�Ŏn�܂鐔��8�i���ɂ���       -po- ���Ȃ�     %s\n", Pchk(Filn->opt_oct));
	printf("  -prd[-]   $�̈�����16�i���J�n�����Ƃ���  -prd-���Ȃ�     %s\n", Pchk(Filn->opt_mot_doll));
//	printf("  -prq[-]   '��΂łȂ��A'C �� 68xx�A�Z���u���p�ɂ���      %s\n", Pchk(Filn->opt_sq_mode == 0));
	printf("  -pq[N]    '����'�� 0:���� 1:�y�A'�ŗL�� 2:��'�ŗL��('a)  %d\n", Filn->opt_sq_mode);
	printf("  -pqw[N]   '������'�� 0:���� 1:�L��                       %d\n", Filn->opt_wq_mode);
	printf("  -pd[-]    ���l(31,0x3,0b10,077,'a'��)���\�i���ɕϊ�      %s\n", Pchk(Filn->immMode));
/*	printf("  -pd[N]    1:�����t�� 2:���������\�i  3:C��16�i��         \n");*/
/*	printf("            4:���ق�16�i  5:��۰ׂ�16�i                    \n");*/
/*	printf("�ȗ���: -pm# -pl$ -pl@ -pl? -pj -ps -pb -pf -pt- -pc3 -o-\n");*/
	exit(1);
}



/* ------------------------------------------------------------------------ */
#define ISKANJI(c)	((unsigned char)(c) >= 0x81 && ((unsigned char)(c) <= 0x9F || ((unsigned char)(c) >= 0xE0 && (unsigned char)(c) <= 0xFC)))


volatile void err_exit(char *fmt, ...)
{
	va_list app;

	va_start(app, fmt);
	/*	err_printf("%s %5d : ", src_name, src_line);*/
	vfprintf(STDERR, fmt, app);
	va_end(app);
	exit(1);
}


int err_printf(char *fmt, ...)
{
	va_list app;
	int n;

	va_start(app, fmt);
	n = vfprintf(STDERR, fmt, app);
	va_end(app);
	fflush(STDERR);
	return n;
}


char *FIL_BaseName(char *adr)
{
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
	char *p;

	p = FIL_BaseName(filename);
	p = strrchr( p, '.');
	if (p == NULL) {
		if (ext) {
			strcat(filename,".");
			strcat( filename, ext);
		}
	} else {
		if (ext == NULL)
			*p = 0;
		else
			strcpy(p+1, ext);
	}
	return filename;
}


char *FIL_AddExt(char filename[], char *ext)
{
	if (strrchr(FIL_BaseName(filename), '.') == NULL) {
		strcat(filename,".");
		strcat(filename, ext);
	}
	return filename;
}


char *FIL_DelLastDirSep(char *dir)
{
	char *p;

	if (dir) {
		p = FIL_BaseName(dir);
		if (strlen(p) > 1) {
			p = p+strlen(p);
			if (p[-1] == '\\' || p[-1] == '/')
				p[-1] = 0;
		}
	}
	return dir;
}



void *mallocE(size_t a)
{
	/* �G���[������Α�exit�� malloc() */
	void *p;

	if (a == 0)
		a = 1;
	p = malloc(a);
	if (p == NULL) {
		err_exit("������������Ȃ�(%d byte(s))\n",a);
	}
	return p;
}


void *callocE(size_t a, size_t b)
{
	/* �G���[������Α�exit�� calloc() */
	void *p;

	if (b == 0)
		b = 1;
	p = calloc(a,b);
	if (p == NULL) {
		err_exit("������������Ȃ�(%d*%d byte(s))\n",a,b);
	}
	return p;
}


char *strdupE(char *s)
{
	/* �G���[������Α�exit�� strdup() */
	char *p;
	if (s == NULL)
		return callocE(1,1);
	p = strdup(s);
	if (p == NULL) {
		err_exit("������������Ȃ�(����%d+1)\n",strlen(s));
	}
	return p;
}


int freeE(void *p)
{
	if (p)
		free(p);
	return 0;
}



SLIST *SLIST_Add(SLIST **p0, char *s)
{
	SLIST* p;
	p = *p0;
	if (p == NULL) {
		p = callocE(1, sizeof(SLIST));
		if (s)
			p->s = strdupE(s);
		*p0 = p;
	} else {
		while (p->link != NULL) {
			p = p->link;
		}
		p->link = callocE(1, sizeof(SLIST));
		p = p->link;
		if (s)
			p->s = strdupE(s);
	}
	return p;
}




/* ------------------------------------------------------------------------ */
