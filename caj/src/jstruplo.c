/*
  unsigned char *jstruplow(unsigned char *jstr,unsigned short opts);
    unsigned char *jstr; �ϊ������镶����. ���ڏ��������܂�
    unsigned short opts; �ϊ����@

  �w�肳�ꂽ�ϊ��𕶎���jstr �ɑ΂��s���܂�. ���A�l��jstr�̱��ڽ�ł��B
  jstr�𒼐ڏ��������܂��̂ŋC�����Ă��������B
  �ϊ����@�� J2KATA|J2UPPER|J2LOWER �̂悤��'|'���g���ē����Ɏw��ł��܂��B

  opts �̒l
    A2UPPER  (0x01)   ���p��̧�ޯď�������啶���ɕϊ����܂�
    A2LOWER  (0x02)   ���p��̧�ޯđ啶�����������ɕϊ����܂�
    J2UPPER  (0x04)   �S�p�A���t�@�x�b�g��������啶���ɕϊ����܂�
    J2LOWER  (0x08)   �S�p�A���t�@�x�b�g�啶�����������ɕϊ����܂�
    J2KATA   (0x10)   �S�p�Ђ炪�Ȃ��J�^�J�i�ɕϊ����܂�
    J2HIRA   (0x20)   �S�p�J�^�J�i���Ђ炪�Ȃɕϊ����܂�
    JSPC2SPC (0x40)   �S�p�󔒂𔼊p�󔒂Q�޲Ăɕϊ����܂�

    * 1991/06/20 Writen by M.Kitamura
    * 1995/12/30 ANSI-C�X�^�C���ɏC��
 */

#include "jstr.h"

typedef unsigned char UCHAR;

char *jstruplow(char *jstr, unsigned mode)
{
    UCHAR up_f;
    UCHAR low_f;
    UCHAR jup_f;
    UCHAR jlow_f;
    UCHAR kata_f;
    UCHAR hira_f;
    UCHAR jspc_f;
    UCHAR d;
    UCHAR c;
    UCHAR *js;

    js = (UCHAR*)jstr;
    up_f = mode & A2UPPER;
    low_f = mode & A2LOWER;
    jup_f = mode & J2UPPER;
    jlow_f = mode & J2LOWER;
    kata_f = mode & J2KATA;
    hira_f = mode & J2HIRA;
    jspc_f = mode & JSPC2SPC;

    while ((c = *js++) != '\0') {
    	if (c <= 0x80) {
    	    if (c < 'A') {
    	    	;
    	    } else if (c <= 'Z') {
    	    	if (low_f)
    	    	    *(js - 1) = c + 0x20;
    	    } else if (c < 'a') {
    	    	;
    	    } else if (c <= 'z') {
    	    	if (up_f)
    	    	    *(js - 1) = c - 0x20;
    	    }
    	} else if (c <= 0x9f || (c >= 0xe0 && c <= 0xfc)) { /* ���JIS */
    	    d = *(js);
    	    if (d == 0)
    	    	*(--js) = '\0';
    	    if (c == 0x81) {
    	    	if (d == 0x40 && jspc_f) {
    	    	    *(js - 1) = 0x20;
    	    	    *(js) = 0x20;
    	    	}
    	    } else if (c == 0x82) {
    	    	if (d >= 0x60 && d <= 0x79) {
    	    	    if (jlow_f) {
    	    	    	d += 0x81 - 0x60;
    	    	    	*(js - 1) = 0x82;
    	    	    	*(js) = d;
    	    	    }
    	    	} else if (d >= 0x81 && d <= 0x9a) {
    	    	    if (jup_f) {
    	    	    	d -= 0x81 - 0x60;
    	    	    	*(js - 1) = 0x82;
    	    	    	*(js) = d;
    	    	    }
    	    	} else if (d >= 0x9f && d <= 0xf1) {
    	    	    if (kata_f) {
    	    	    	d -= 0x9f - 0x40;
    	    	    	if (d > 0x7e)
    	    	    	    ++d;
    	    	    	*(js - 1) = 0x83;
    	    	    	*(js) = d;
    	    	    }
    	    	}
    	    } else if (c == 0x83 && d >= 0x40 && d <= 0x93) {
    	    	if (hira_f) {
    	    	    if (d > 0x7e)
    	    	    	--d;
    	    	    d += 0x9f - 0x40;
    	    	    *(js - 1) = 0x82;
    	    	    *(js) = d;
    	    	}
    	    }
    	    js++;
    	}
    }/* end of while */

    return jstr;
}
