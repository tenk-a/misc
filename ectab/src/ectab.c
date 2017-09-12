/**
 *	@file	ectab.c
 *	@brief	�󔒃^�u�ϊ��c�[��
 *
 *	@author �k����j<NBB00541@nifty.com>
 *	@date	2001(?)�` 2004-01-29
 *	@note
 *		�A�Z���u���ŏ����ꂽmsdos-16bit-exe�ł������C�N�B
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "cmisc.h"

#undef	BOOL
#define BOOL int

#undef	STDERR
#define STDERR	stderr

#define DBG_S(s)				printf("%-14s %5d : %s", __FILE__, __LINE__, (s), 0)
#define DBG_F(s)				printf s
#define DBG_M() 				printf("%-14s %5d :\n", __FILE__, __LINE__)


/** �����\�����I�� */
static void usage(void)
{
	printf("usage> ectab [-opts] file(s)    // v2.10 " __DATE__ "  writen by M.Kitamura\n"
		   "  �^�u�ϊ�,�s���󔒍폜�����s��. �f�t�H���g�ł͕W���o��\n"
		   "  ���s�Ƃ��� LF(\\n un*x) CR(\\r mac) CRLF(\\r\\n win/dos) ��F��\n"
		   "  -o        �o�͂����t�@�C�����ɂ���B���X�̃t�@�C����.bak�ɂ���\n"
		   "  -o[FILE]  FILE�ɏo��.\n"
		   "  -m        �s���̋󔒂��폜.\n"
		   "  -r[0-3]   ���s�� 0:���͂̂܂� 1:'\\n' 2:'\\r' 3:'\\r\\n' �ɕϊ�(file�o�͎��̂�)\n"
		   "  -s[N]     �o�͂̃^�u�T�C�Y�� N �ɂ���(��->�^�u)\n"
		   "  -t[N]     ���͂̃^�u�T�C�Y�� N �ɂ���(�^�u->��)\n"
		   "  -z        �o�͂̃^�u�T�C�Y����1�����ɂ����Ȃ�Ȃ��ꍇ�͋󔒂ŏo��\n"
		   "  -j        �o�͂̃^�u�T�C�Y���x�̋󔒂̂݃^�u�ɕϊ�\n"
		   "  -q        �o�͂�4�^�u8�^�u�Ō����ڂ����Ȃ��悤�ɂ���\n"
		   "  -b[0-2]   C/C++�� \" ' �΂��l�� 0:���Ȃ� 1:���� 2:�Β���ctrl������\\������\n"
		   "  -u        ���p�������̑啶����\n"
		   "  -l        ���p�啶���̏�������\n"
		   "  -a        EOF�Ƃ���0x1a���o��\n"
		   "  -k[0-2]   0:1�o�C�g�n  1:�V�t�gJIS(MBC)  2:UTF8  (�f�t�H���g -k1)\n"
		   "  -n[N:M:L:STR]  �s�ԍ���t��. N:����,M:�X�L�b�v��,L:0�s�ڂ̍s�ԍ�,STR:���\n"
	);
	exit(1);
}


#define stpcpy(d,s) 			((d) + sprintf((d), "%s", (s)))


/** �I�v�V�����ݒ� */
typedef struct opts_t {
	int  stab;
	int  dtab;
	int  cmode;
	BOOL both48;
	BOOL sp1ntb;
	BOOL ajstab;
	BOOL crlfMd;
	BOOL trimSw;
	BOOL sjisSw;
	BOOL utf8Sw;
	BOOL eofSw;
	BOOL uprSw;
	BOOL lwrSw;
	unsigned  numbering;
	unsigned  numbStart;
	int       numbSkip;
	char      *numbSep;
	// �{�����ꂽ���Ȃ��������A�ʓ|�Ȃ��
	const char *outname;
	const char *extname;
} opts_t;


/** �����񒆂ɉ�ꂽ�S�p�������Ȃ����`�F�b�N
 *  @return  0:���ĂȂ����� 1:���Ă���
 */
static int isJstrBroken(char buf[])
{
	unsigned char *s = (unsigned char *)buf;
	int rc = 0;

	for (;;) {
		int c = *s++;
		if (c == 0)
			break;
		if (ISKANJI(c)) {
			c = *s++;
			if (c == 0) {
				return 1;
			}
			if (ISKANJI2(c) == 0) {
				rc = 1;
			}
		}
	}
	return rc;
}



/** '\n','\r','\r\n'�̉��ꂩ�����s�Ƃ���P�s����
 *	@param	buf 	�ǂݍ��ރo�b�t�@
 *	@param	len 	�o�b�t�@�T�C�Y.
 *	@param	crlfMd	���s�� 0:�ϊ����Ȃ� 1:\n�ɕϊ� 2:\r�ɕϊ� 3:\r\n�ɕϊ�
 *	@return 0:����Ǎ�	bit0=1:eof bit1=1:�Ǎ��G���[ bit2:1�s����������
 *			bit3:'\0'�݂� bit4:�o�C�i���R�[�h������
 */
static int getLine(char *buf, size_t bufSz, int crlfMd, FILE *fp)
{
	char *p   = buf;
	//char *e = buf + bufSz - 1;
	size_t i  = 0;
	int    rc = 0;
	int    c;

	assert(buf);
	assert(bufSz > 0);
	--bufSz;
	buf[0] = 0;
	for (;;) {
		if (i == bufSz) {
			rc |= 1<<2; //("1�s����������\n");
			break;
		}
		c = fgetc(fp);
		if (feof(fp)) {
			if (i > 0) {
				break;
			}
			return 1<<0;
		}
		if (ferror(fp)) {
			rc |= 1<<1; //("���[�h�G���[���N���܂���.\n");
		}
		if (c < 0x20 || c == 0x7f) {
			if (c == '\0') {
				rc |= 1<<3; //("�s���� '\\0' ���������Ă���\n");
				c = ' ';
			} else if (c == '\n') {
				if (crlfMd & 3) {
					if (crlfMd & 2) *p++ = '\r';
					if (crlfMd & 1) *p++ = '\n';
				} else {
					*p++ = '\n';
				}
				break;
			} else if (c == '\r') {
				c = fgetc(fp);
				if (c == '\n') {
					if (crlfMd & 3) {
						if (crlfMd & 2) *p++ = '\r';
						if (crlfMd & 1) *p++ = '\n';
					} else {
						*p++ = '\r';
						*p++ = '\n';
					}
				} else {
					if (feof(fp) == 0) {
						ungetc(c,fp);
					}
					if (crlfMd & 3) {
						if (crlfMd & 2) *p++ = '\r';
						if (crlfMd & 1) *p++ = '\n';
					} else {
						*p++ = '\r';
					}
				}
				break;
			} else if (c == '\a' || c == '\b' || c == '\t'
					|| c == '\v' || c == '\f' || c == '\r'
					|| c == 0x1a || c == 0x1b) {
				;
			} else {
				rc |= 1 << 4;	// �R���g���[���R�[�h���������Ă���
				c = ' ';
			}
		}
		*p++ = c;
		i++;
	}
	*p = 0;
	return rc;
}


/** iname �̃e�L�X�g��o�̕ϊ��w��ɂ��������ĕϊ��� oname �ɏo��
 *  @param	iname	���̓t�@�C����. NULL�Ȃ�W������
 *  @param	oname	�o�̓t�@�C����. NULL�Ȃ�W���o��
 *  @return 		0:���s 1:����
 */
static int convFile(const char *iname, const char *oname, opts_t *o)
{
	enum {SBUF_SZ = 16*1024};
	char buf[16*16*1024];
	char sbuf[16*1024];
	char *p;
	FILE *ofp, *ifp;
	unsigned lno = 0, numb;
	int  er, rc = 1, crlfMd = o->crlfMd;
	int  tabFlags, trimFlags, upLwrFlags;

	if (iname) {
		const char *md = "rb";			// ���s�����O�ŊǗ��������̂Ńo�C�i���ŃI�[�v��
		if (oname == NULL)				// �o�͂��W���o�͂̂Ƃ��́A\r\n�̈������ʓ|�ɂȂ�̂ŁA
			md = "rt";					// �������e�L�X�g���[�h�ɂ���\r\n�̓��͂�os/���C�u�����ɔC��^^;
		ifp = fopen(iname, md);
		if (ifp == NULL) {
			fprintf(STDERR, "%s ��open�ł��Ȃ�\n", iname);
			return 0;
		}
	} else {
		iname = "<stdin>";
		ifp = stdin;
	}
	if (oname) {
		ofp = fopen(oname, "wb");		// ���s�����O�ŊǗ��������̂Ńo�C�i���ŃI�[�v��
		if (ofp == NULL) {
			fprintf(STDERR, "%s ��open�ł��Ȃ�\n", oname);
			exit(1);
		}
	} else {
		oname = "<stdout>";
		ofp   = stdout;
	}

	// �s�ԍ��\���֌W�̐ݒ�
	numb = o->numbStart;
	if (o->numbSkip == 0)
		o->numbSkip = 1;
	if (o->numbSep == NULL)
		o->numbSep = " ";

	// �^�u�ϊ��֌W�̏���
	tabFlags =  (o->cmode << 1) | (o->ajstab << 4) | (o->both48 << 5);
	if (o->cmode)
		tabFlags |= o->trimSw << 8;		// c���[�h�̎���'"�y�A�̃`�F�b�N�̓s��, strTab����strTrimSpcR���Ă�
	if (tabFlags && o->stab == 0) {		// �ϊ��w�肪����̂ɁA�\�[�X�^�u�T�C�Y�������ꍇ�́A�����ݒ�
		if (o->cmode)	o->stab = 4;	// c���[�h�Ȃ�4
		else			o->stab = 8;	// �ȊO�� 9
	}
	if (o->stab || o->dtab || tabFlags)
		tabFlags |= (o->utf8Sw << 9) | (o->sjisSw << 7) | (1<<6) | (o->sp1ntb);

	// �^�u�ϊ����Ȃ����ǁA�s���󔒍폜���Ȃ��ꍇ�͐�p�ɌĂяo��
	trimFlags = 0;
	if (tabFlags == 0 && o->trimSw) {
		// bit0=1:�s����'\n''\r'�͊O���Ȃ�. bit1=1:C/C++�ł�\�������l��(�s�A���ɉ����Ȃ��悤��)
		trimFlags = (o->sjisSw << 7) /*|(o->cmode << 1)*/ | 1;
	}

	// �啶���������ϊ��̐ݒ�
	upLwrFlags = (o->lwrSw<<1) | o->uprSw;
	if (upLwrFlags)
		upLwrFlags |= (o->sjisSw << 7);

	// c/c++����' " �y�A�`�F�b�N�̏�����
	strTab(NULL, NULL, 0,0,0,0);

	// �ϊ��{��
	for (;;) {
		er = getLine(sbuf, sizeof(sbuf), crlfMd, ifp);
		if (er) {
			if (er & 0x10)
				fprintf(STDERR, "%s %d : ���͂̂P�s����������\n", iname, lno);
			if (er & 0x08)
				fprintf(STDERR, "%s %d : �o�C�i���R�[�h��������\n", iname, lno);
			if (er & 0x04)
				fprintf(STDERR, "%s %d : '\0'���s���ɂ�����\n", iname, lno);
			if (er & 0x02)
				break;		// ���[�h�G���[������
			if (er & 0x01)
				break;		// EOF������
		}
		++lno;
		numb += o->numbSkip;
		p = sbuf;
		// �^�u�󔒕ϊ�
		if (o->stab || o->dtab || tabFlags) {
			strTab(buf, sbuf, o->dtab, o->stab, tabFlags, sizeof(buf));
			p = buf;
		}
		// �s���󔒂̍폜
		if (trimFlags) {
			strTrimSpcR(p, trimFlags);
		}
		// ������̑啶��/��������
		if (upLwrFlags) {
			strUpLow(p, upLwrFlags);
		}
		// ��ꂽ�S�p�������Ȃ����`�F�b�N
		if (o->sjisSw && isJstrBroken(p)) {
			fprintf(STDERR, "%s %d : ��ꂽ�S�p����������܂�\n", iname, lno);
		}
		// �s���o��
		if (o->numbering) {	// �s�ԍ��t��
			fprintf(ofp, "%*d%s%s", o->numbering, numb, o->numbSep, p);
		} else {			// ���̂܂�
			fprintf(ofp, "%s", p);
		}
		if (ferror(ofp))
			break;
	}
	// eof������ꍇ
	if (o->eofSw) {
		fputs("\x1a", ofp);
	}
	// �㏈��
	if (er & 2) {
		fprintf(STDERR, "%s (%d) �ǂݍ��݂ŃG���[��������\n", iname, lno);
		rc = 0;
	}
	if (ferror(ofp)) {
		fprintf(STDERR, "%s (%d) �������݂ŃG���[��������\n", oname, lno);
		rc = 0;
	}
	if (ifp != stdin)
		fclose(ifp);
	if (ofp != stdout)
		fclose(ofp);
	return rc;
}


/** 1�t�@�C���̕ϊ�. �t�@�C�����̒����킹 */
static void oneFile(const char *iname, const char *oname, const char *extname, opts_t *o)
{
	// �t�@�C����������o�b�N�A�b�v�̏���
	char nameBuf[_MAX_PATH+8];
	char *tmpname = NULL;
	int rc;

	if (iname) {
		if (iname && oname && oname[0] == 0) {	// ���̓t�@�C�������g�ɏo��
			sprintf(nameBuf, "%s.~tmp", iname);
			oname = nameBuf;
			tmpname = nameBuf;
			fprintf(STDERR, "[%s]\n", iname);
		} else if (extname && extname[0]) {
			sprintf(nameBuf, "%s.%s", iname, extname);
			oname = nameBuf;
		}
	}

	rc = convFile(iname, oname, o);

	if (rc && iname && iname[0] && tmpname) {		// ���̓t�@�C�����g�̏o�͂̏ꍇ
		char bakname[_MAX_PATH+8];
		sprintf(bakname, "%s.bak", iname);
		remove(bakname);
		rename(iname, bakname);
		rename(tmpname, iname);
	}
}

/** �I�v�V���������̎���'-'��'0'�Ȃ�U, �ȊO�Ȃ�^�ɂ���ׂ̃}�N�� */
static int opts_getVal(const char *arg, char **pp, int dfltVal, int maxVal)
{
	char *p = *pp;
	int val;

	if (*p == '-') {
		*pp = p + 1;
		val = 0;
	} else if (isdigit(*p)) {
		val = strtol(p, pp, 0);
	} else {
		val = dfltVal;
	}
	if (val > maxVal) {
		fprintf(STDERR, "�I�v�V��������%s���̒l %d ���傫������(>%d)\n", arg, val, maxVal);
		exit(1);
	}
	return val;
}


/** �I�v�V������� */
static void opts_get(char *arg, opts_t *o)
{
	char *p = arg + 1;

	if (*p == '\0') {	// - �����Ȃ�W������
		oneFile(NULL, o->outname, o->extname, o);
		return;
	}
	while (*p) {
		int   c = *p++;
		c = toupper(c);
		switch (c) {
		case 'O':
			o->outname = strdup(p);
			return;
		case 'X':
			o->extname = strdup(p);
			return;
		case 'K':
			o->sjisSw = opts_getVal(arg, &p, 1, 2);
			o->utf8Sw = o->sjisSw == 2;
			o->sjisSw &= 1;
			break;
		case 'M':
			o->trimSw = opts_getVal(arg, &p, 1, 1);
			break;
		case 'R':
			o->crlfMd = opts_getVal(arg, &p, 0, 3);
			break;
		case 'A':
			o->eofSw  = opts_getVal(arg, &p, 1, 1);
			break;
		case 'U':
			o->uprSw  = opts_getVal(arg, &p, 1, 1);
			break;
		case 'L':
			o->lwrSw  = opts_getVal(arg, &p, 1, 1);
			break;
		case 'B':
		case 'C':
			o->cmode  = opts_getVal(arg, &p, 1, 2);
			if (o->cmode == 2)
				o->cmode = 4|2|1;
			else if (o->cmode == 1)
				o->cmode = 4|1;
			break;
		case 'Z':
			o->sp1ntb = opts_getVal(arg, &p, 1, 9) ? 1 : 0;
			break;
		case 'J':
			o->ajstab = opts_getVal(arg, &p, 1, 1);
			break;
		case 'Q':
			o->both48 = opts_getVal(arg, &p, 1, 1);
			break;
		case 'S':
			o->dtab = opts_getVal(arg, &p, 4, 256);
			break;
		case 'T':
			o->stab = opts_getVal(arg, &p, 4, 16);
			break;
		case 'N':
			o->numbering = (*p == 0) ? 7 : strtoul(p,&p,0);
			if (*p == ':' || *p == ',') {
				p++;
				o->numbSkip = strtol(p,&p,0);
				if (*p == ':' || *p == ',') {
					p++;
					o->numbStart = strtoul(p, &p, 0);
					if (*p == ':' || *p == ',') {
						p++;
						o->numbSep = strdup(p);
						return;
					}
				}
			}
			break;
		case 'H':
		case '?':
			usage();
			return;
		default:
	  //ERR_OPTS:
			fprintf(STDERR, "�m��Ȃ��I�v�V����(%s)\n", arg);
			exit(1);
			return;
		}
	}
}


/** �������n�܂� */
int main(int argc, char *argv[])
{
	int  i;
	char *p;
	static opts_t opt;
	opts_t *o = &opt;

	memset(o, 0, sizeof *o);
	o->sjisSw = 1;

	if (argc < 2)
		usage();

	for (i = 1; i < argc; i++) {
		p = argv[i];
		if (*p == '-') {	// �I�v�V�������
			opts_get(p, o);
		} else {			// �t�@�C�����s
			oneFile(p, o->outname, o->extname, o);
		}
	}
	return 0;
}


