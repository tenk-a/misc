/*
    caj     ���p<->�S�p�ϊ���۸���
    C����� Ver. 1.00  1991.06.20  os9/09 & MS-DOS    Writen by M.kitamura
            Ver. 1.01  1991.08.23  jstrcnv�����
            Ver. 1.02  1992.10.25  jstrcnv�����,indent(5->4),-y[-]�ǉ� etc
            Ver. 1.03  1995.12.30  os9�֌W���폜. ANSI-C���قɏC��. -cy�ǉ�
            Ver. 1.04  1999.12.04  -ck�ǉ�
            Ver. 1.04b 2017.09     �R���p�C���x���C��
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "jstr.h"

#define STDERR          stdout

#ifdef C16  // for 16bit CPU
#define NAM_SIZE        (260*2)
#define BUF_SIZE        (1024*5)
#else
#define NAM_SIZE        (1030)
#define BUF_SIZE        (0x8000)
#endif

typedef unsigned char   UCHAR;
#define iskanji(c)      ((UCHAR)(c) > 0x80U && ((UCHAR)(c) <= 0x9FU || ((UCHAR)(c) >= 0xE0U && (UCHAR)(c) <= 0xFCU)))
#define iskanji2(c)     ((UCHAR)(c) >= 0x40U && (UCHAR)(c) <= 0xfcU && (UCHAR)(c) != 0x7fU)

#define eputs_exit(s)  (fputs((s),STDERR),exit(1))

void usage(void)
{
    fputs(
        "usage: caj [-opts] file...                     // CAJ v1.04\n"
        " -h -? �w���v\n"
        " -e  ���p��S�p�ɕϊ�(-epdakn) -j  �S�p�𔼊p�ɕϊ�(-jpdak)\n"
        " -ep ���p�L����S�p��(�󔒏�)  -jp �S�p�L���𔼊p��(�󔒊�)\n"
        " -ed ���p������S�p��          -jd �S�p�����𔼊p��\n"
        " -ea ���p��̧�ޯĂ�S�p��      -ja �S�p��̧�ޯĂ𔼊p��\n"
        " -eh ���p�ł�S�p�Ђ炪�ȉ�    -jh �S�p�Ђ炪�Ȃ𔼊p�ŉ�\n"
        " -ek ���p�ł�S�p�J�^�J�i��    -jk �S�p�J�^�J�i�𔼊p�ŉ�\n"
        " -en ���p�ŋL����S�p��        -jn �S�p�J�i�L���𔼊p��\n"
        " -es ���p�󔒂�S�p��          -js �S�p�󔒂𔼊p��2��\n"
        " -eo ���p�̑S�p����'�''�'��������1�����Ƃ���\n"
        " -jt �Ђ炪�Ȃ��J�^�J�i��      -jr �J�^�J�i���Ђ炪�ȉ�\n"
        " -u  ���p��̧�ޯĂ̑啶����    -ju �S�p��̧�ޯĂ̑啶����\n"
        " -l  ���p��̧�ޯĂ̏�������    -jl �S�p��̧�ޯĂ̏�������\n"
        " -y  \\ �ɑΉ�����S�p������    -y- \\�ɑΉ�����S�p���_��\n"
        " -cy �S�p2�޲Ėڂ�\\�̍s��T��  -ck �S�p�����̗L��s��T��\n"
        " -o  �ȩ�ق�.baķ�ق���茳��̧�ٖ��ŏo��\n"
        " -o=<path> �o�͂�<path>�ɂ���\n"
        , STDERR);
    exit(0);
}

void opts_err(void)
{
    eputs_exit("��߼�ݎw�肪��������\n");
}

void rename_e(char const* oldname, char const* newname)
{
    if (rename(oldname,newname)) {
        fprintf(STDERR,"%s �� %s �� rename �ł��܂���\n",oldname,newname);
        exit(errno);
    }
}

FILE *fopen_e(char const* name, char const* mode)
{
    FILE *fp = fopen(name,mode);
    if (fp == NULL) {
        fprintf(STDERR,"\n %s �� open �ł��܂���\n",name);
        exit(errno);
    }
    return fp;
}


char *fil_basename(char const* adr)
{
    char const* p = adr;
    while (*p != '\0') {
        if (*p == ':' || *p == '/' || *p == '\\')
            adr = p + 1;
        if (iskanji((*(UCHAR *)p)) && *(p+1) )/*�S�p������FILE���ւ̑Ή�*/
            p++;
        p++;
    }
    return (char*)adr;
}

char *fil_chgext(char filename[], char const* ext)
{
    char* p = fil_basename(filename);
    p = strrchr( p, '.');
    if (p == NULL) {
        strcat(filename,".");
        strcat( filename, ext);
    } else {
        strcpy(p+1, ext);
    }
    return filename;
}


static char ibuf[BUF_SIZE+4];
static char obuf[BUF_SIZE * 2 + 256];


void chkzenyen(FILE *fp, char const* fnam)
{
    UCHAR*          p;
    unsigned long   cnt = 0L;

    for (; ;) {
        ++cnt;
        memset(ibuf,0,BUF_SIZE+2);
        if (fgets(ibuf,BUF_SIZE,fp) == NULL)
            break;
        for (p = (UCHAR*)ibuf; *p != '\n' && *p != '\0' && p < (UCHAR*)ibuf+BUF_SIZE; p++) {
            if (iskanji(*p)) {
                p++;
                if (*p == '\\') {
                    fprintf(STDERR,"%s %ld : %c%c �̂Q�o�C�g�ڂ� '\\'.\n",fnam,cnt,*(p-1),*p);
                } else if (*p == '\0') {
                    fprintf(STDERR,"%s %ld : �x��>�s���S�p������(2�޲Ėڂ� 0)\n",fnam,cnt);
                } else if (!iskanji2(*p)) {
                    fprintf(STDERR,"%s %ld : �x��>�s���S�p������\n",fnam,cnt);
                }
            }
        }
      #if 10
        if (*p != '\n') {
            fprintf(STDERR,
                    "%s %ld : �x��>���̍s�ɉ��s���Ȃ�(1�s������������,NUL�������������Ă���,̧�ق̏I��肩)\n"
                    ,fnam,cnt);
        }
      #endif
    }
    if (ferror(fp))
        eputs_exit("���͂ŃG���[����������\n");
}


void findZen(FILE* fp, char const* fnam)
{
    UCHAR*          p;
    unsigned long   cnt = 0L;

    for (; ;) {
        ++cnt;
        memset(ibuf,0,BUF_SIZE+2);
        if (fgets(ibuf,BUF_SIZE,fp) == NULL)
            break;
        for (p = (UCHAR*)ibuf; *p != '\n' && *p != '\0' && p < (UCHAR*)ibuf+BUF_SIZE; p++) {
            if (iskanji(*p)) {
                printf("%s %ld : %s", fnam, cnt, ibuf);
                break;
            }
        }
    }
}


void one(FILE *fp, FILE *ofp, unsigned opt, unsigned opt2, char const* fnam)
{
    int  k;
    unsigned long cnt = 0L;

    while (fgets(ibuf,BUF_SIZE,fp)) {
        ++cnt;
        if (jstrcnv(obuf,ibuf,opt) == NULL)
            fprintf(STDERR,"\n%s %ld : �s���ȕ���������\n", fnam, cnt);
        if (opt2 > 0)
            jstruplow(obuf,opt2);
        k = fputs(obuf,ofp);
        if (k < 0)
            eputs_exit("�o�͂ŃG���[����������\n");
    }
    if (ferror(fp))
        eputs_exit("���͂ŃG���[����������\n");
}


int main(int argc, char *argv[])
{
    static char bakname[NAM_SIZE];
    static char tmpname[NAM_SIZE];
    FILE        *fp,*ofp;
    char        *oname = NULL;
    unsigned    mode,mode2,chk2yen,chk2kan;
    unsigned    i;
    char        *p;
    int         c,n;

    oname = NULL;
    chk2kan = chk2yen = mode = mode2 = 0;
    for (n = i = 1; i < (unsigned)argc; i++) {
        p = argv[i];
        if (*p != '-') {
            n = 0;
        } else {
            c = *++p;
            switch (toupper(c)) {
            case 'E':
                if (*(p + 1) == '\0') {
                    mode |= TOJPUN|TOJALPH|TOJDGT|TOJKATA|TOJKPUN;
                    break;
                }
                while ((c = *(++p)) != '\0') {
                    switch (toupper(c)) {
                    case 'P':
                        mode |= TOJPUN;
                        break;
                    case 'S':
                        mode |= TOJSPC;
                        break;
                    case 'A':
                        mode |= TOJALPH;
                        break;
                    case 'D':
                        mode |= TOJDGT;
                        break;
                    case 'H':
                        mode |= TOJHIRA;
                        break;
                    case 'K':
                        mode |= TOJKATA;
                        break;
                    case 'N':
                        mode |= TOJKPUN;
                        break;
                    case 'O':
                        mode |= DAKUOFF;
                        break;
                    default:
                        opts_err();
                    }
                }
                break;
            case 'J':
                if (*(p + 1) == '\0') {
                    mode |=JTOPUNS|JTOALPH|JTODGT|JTOKATA;
                    break;
                }
                while ((c = *(++p)) != '\0') {
                    switch (toupper(c)) {
                    case 'P':
                        mode |= JTOPUNS;
                        break;
                    case 'S':
                        mode |= JSPC2SPC;
                        break;
                    case 'A':
                        mode |= JTOALPH;
                        break;
                    case 'D':
                        mode |= JTODGT;
                        break;
                    case 'H':
                        mode |= JTOHIRA;
                        break;
                    case 'K':
                        mode |= JTOKATA;
                        break;
                    case 'N':
                        mode |= JTOKPUN;
                        break;
                    case 'U':
                        mode2 |= J2UPPER;
                        break;
                    case 'L':
                        mode2 |= J2LOWER;
                        break;
                    case 'T':
                        mode2 |= J2KATA;
                        break;
                    case 'R':
                        mode2 |= J2HIRA;
                        break;
                    default:
                        opts_err();
                    }
                }
                break;
            case 'U':
                mode2 |= A2UPPER;
                break;
            case 'L':
                mode2 |= A2LOWER;
                break;
            case 'Y':
                mode &= ~YENOFF;
                if (*(++p) == '-')
                    mode |= YENOFF;
                break;
            case 'C':
                c = *(++p);
                if (c == 'Y'||c == 'y') {
                    chk2yen = 1;
                }
                if (c == 'k'||c == 'K') {
                    chk2kan = 1;
                }
                break;
            case 'O':
                if (*(++p) == '=')
                    ++p;
                oname = p;
                break;
            case 'H':
            case '?':
            case '\0':
                usage();
            default:
                opts_err();
            }
        }
    }

    ofp = stdout;
    c = 0;
    if (oname == NULL) {
        c = 2;
    } else if (*oname != '\0') {
        c++; /* c = 1; */
        ofp = fopen_e(oname,"w");
    }
    if (n) {
        if (chk2yen)
            chkzenyen(stdin,"<stdin>");
        else if (chk2kan)
            findZen(stdin,"<stdin>");
        else
            one(stdin,ofp,mode,mode2, "<stdin>");
    } else {
        for (i = 1; i < (unsigned)argc; i++) {
            p = argv[i];
            if (*p == '-')
                continue;
            fp = fopen_e(p,"r");
            if (c == 0) {
                fil_chgext(strcpy(tmpname,p),"TMP");
                ofp = fopen_e(tmpname,"w");
            }
            if (chk2yen)
                chkzenyen(fp,p);
            else if (chk2kan)
                findZen(fp,p);
            else
                one(fp,ofp,mode,mode2,p);
            fclose(fp);

            if (c == 0) {
                fclose(ofp);
                fil_chgext(strcpy(bakname,p),"bak");
                unlink(bakname);
                rename_e(p,bakname);
                rename_e(tmpname,p);
            }
        }
    }
    if (c == 1)
        fclose(ofp);
    return 0;
}
