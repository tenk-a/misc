/**
 *  @file   cfg.cpp
 *  @brief  �R���t�B�O�t�@�C���������֐�
 *
 *  @author Masahi KITAMURA (tenka@6809.net)
 *  @note
 *      �ʃc�[������ʂ������Ăł�������
 */

#include "cfgfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>


// �񂹏W�߃\�[�X���l�[���X�y�[�X�ŉB��

namespace CfgFile_Tmp {

typedef unsigned char Uint8;

/** ������ s �̐擪�󔒕������X�L�b�v�����A�h���X��Ԃ� */
static inline const char *strSkipSpc(const char *s)
{
    while ((*s && *(const unsigned char *)s <= ' ') || *s == 0x7f) {
        s++;
    }
    return s;
}

/** �t�@�C���ꊇ�ǂݍ���(���[�h)
 *  @param  name    �ǂ݂��ރt�@�C��
 *  @param  buf     �ǂ݂��ރ������BNULL���w�肳���� malloc���A16�o�C�g�]���Ɋm�ۂ���
 *  @param  bufsz   buf�̃T�C�Y�B0���w�肳���� �t�@�C���T�C�Y�ƂȂ�
 *  @param  rdszp   NULL�łȂ���΁A�ǂ݂��񂾃t�@�C���T�C�Y�����ĕԂ�
 *  @return         buf�̃A�h���X��malloc���ꂽ�A�h���X. �G���[����NULL��Ԃ�
 */
static void *file_loadAbs(const char *name, void *buf, int bufsz, int *rdszp)
{
    FILE *fp;
    int  l;

    fp = fopen(name, "rb");
    if (fp == NULL)
        return NULL;
    l = filelength(fileno(fp));
    if (rdszp)
        *rdszp = l;
    if (bufsz == 0)
        bufsz = l;
    if (l > bufsz)
        l = bufsz;
    if (buf == NULL) {
        bufsz = (bufsz + 15 + 16) & ~15;
        buf = calloc(1, bufsz);
        if (buf == NULL)
            return NULL;
    }
    fread(buf, 1, l, fp);
    if (ferror(fp)) {
        fclose(fp);
        buf = NULL;
    }
    fclose(fp);
    return buf;
}


/* ------------------------------------------------------------------------ */
/* �e�L�X�g�t�@�C���s����                                                   */
/* ------------------------------------------------------------------------ */
#define FILE_NMSZ   1024

/// �e�L�X�g�s���̓��[�`���p
typedef struct txtf_t {
    char        txtName[FILE_NMSZ];
    const char  *txtTop;
    const char  *txtEnd;
    const char  *txtPtr;
    int         txtLine;
    int         txtMallocFlg;
} txtf_t;

static txtf_t txtf;

/** �e�L�X�g�t�@�C���I�[�v��(�s���͗p). �ꊇ�Ńt�@�C�����������[�Ƀ��[�h */
void txtf_open(const char *fname)
{
    int         sz = 0;

    strncpy(txtf.txtName, fname, FILE_NMSZ);
    txtf.txtName[FILE_NMSZ-1] = '\0';
    txtf.txtTop         = (const char *)file_loadAbs(fname, NULL, 0, &sz);  // �t�@�C���T�C�Y��Malloc���ă��[�h
    txtf.txtMallocFlg   = 1;                                    // Malloc�������������g��
    txtf.txtPtr         = txtf.txtTop;
    txtf.txtEnd         = txtf.txtTop + sz;
    txtf.txtLine        = 0;
}


/** �������̃e�L�X�g�f�[�^��Ώۂɂ���BMalloc���Ȃ��I */
void txtf_openMem(const void *mem, int sz)
{
    txtf.txtTop         = (const char *)mem;
    txtf.txtMallocFlg   = 0;                        // Malloc���ĂȂ���
    txtf.txtPtr         = txtf.txtTop;
    txtf.txtEnd         = txtf.txtTop + sz;
    txtf.txtLine        = 0;
}


/** �e�L�X�g�s���͂̏I�� */
void txtf_close(void)
{
    if (txtf.txtTop && txtf.txtMallocFlg) {     // Malloc�����������Ȃ���
        free((void*)txtf.txtTop);
    }
    txtf.txtMallocFlg   = 0;
    txtf.txtTop         = (const char *)NULL;
    txtf.txtPtr         = (const char *)NULL;
    txtf.txtLine        = 0;
}



/** �e�L�X�g�s���� */
char *txtf_gets(char *buf, int sz)
{
    const char *s;
    char       *d;

    if (sz <= 0)
        return buf;
    --sz;
    d = buf;
    s = txtf.txtPtr;
    if (s == 0 || *s == 0 || s >= txtf.txtEnd)
        return (char*)NULL;
    while (*s && s < txtf.txtEnd) {
        if (*s == '\r' && s[1] == '\n') {
            s++;
            continue;
        }
        if (sz == 0)
            break;
        *d++ = *s++;
        --sz;
        if (s[-1] == '\n')
            break;
    }
    txtf.txtPtr = s;
    *d = 0;
    txtf.txtLine++;
    return buf;
}



/* ------------------------------------------------------------------------ */
/* �R���t�B�O�E�t�@�C���ǂݍ���                                             */
/* ------------------------------------------------------------------------ */

//#define CMT_CHR           ';'
//#define CMT_CHR           '%'
#define CMT_CHR             '/'
// 2���ׂ�Ƃ��͒�`
#define CMT_2

static char *cfg_buf;       // �ǂݍ���cfg�t�@�C���̓��e
static int  cfg_size;       // ���̃o�C�g��

/** �R���t�B�O���(�t�@�C��)�̏��� */
int cfg_init(const char *fn)
{
    cfg_buf = (char *)file_loadAbs(fn, NULL, 0, &cfg_size);
    return cfg_buf != NULL;
}


/** �R���t�B�O���̏����B�I���������� */
int cfg_init4mem(const char *mem, int memSz)
{
    //if (mem == NULL || memSz < 0) {ERR_ROUTE(); return 0;}
    cfg_buf = (char *)calloc(1, memSz);
    if (cfg_buf)
        memcpy(cfg_buf, mem, memSz);
    cfg_size = memSz;
    return cfg_buf != NULL;
}


/** �R���t�B�O���̎g�p���I�� */
void cfg_term(void)
{
    if (cfg_buf)
        free(cfg_buf);
    cfg_buf = (char *)NULL;
}


/** tn�ɑΉ����镶������擾 */
char *cfg_getStr(char *dst, int len, const char *tn)
{
    static char buf[0x1000];
    char *s;
    char *p;
    int  tnlen,rc=0;

    if (cfg_buf == NULL || tn == NULL)
        return NULL;

    //DBG_ASSERT(tn != NULL);
    tnlen = strlen(tn);

    if (dst == NULL) {  // �w�肪�Ȃ���΁A�����o�b�t�@�Ɏ擾���ăA�h���X��Ԃ�
        dst = buf;
        len = sizeof(buf);
    }
    txtf_openMem(cfg_buf, cfg_size);

    for (;;) {
        // ��s�擾
        dst[0] = 0;
        s = txtf_gets(dst, len);
        if (s == NULL) {    // �t�@�C���̏I���Ȃ�I��
            dst[0] = 0;
            break;
        }
        // �s���󔒂��X�L�b�v
        s = (char*)strSkipSpc(s);
        if (memcmp(s, tn, tnlen) == 0) {    // �w�肵��TAGNAME�Ɠ������H
            rc = 1;
            // = ������Δ�΂�
            s = (char*)strSkipSpc(s+tnlen);
            // { ������΁A�����s��`�A�Ȃ���Έ�s��`
            if (*s == '=') {    // ��s��`
                s = (char*)strSkipSpc(s+1);
                // �o�b�t�@�K�[�h
                if (strlen(s) >= (size_t)len) {
                    s[len-1] = 0;
                }
                int md = 0;
                for (p = s; *(Uint8*)p >= 0x20 && *p != 0x7f; p++) {
                    if (*p == '"')
                        md ^= 1;
                  #ifdef CMT_2
                    if (md == 0 && p > s && p[-1] <= ' ' && *p == CMT_CHR && p[1] == CMT_CHR)       // ���O���󔒂� ; �̓R�����g����
                        break;
                  #else
                    if (md == 0 && p > s && p[-1] <= ' ' && *p == CMT_CHR)      // ���O���󔒂� ; �̓R�����g����
                        break;
                  #endif
                }
                while (p > s && p[-1] == ' ') --p;      // �P�c�ɂ���󔒂͍폜
                *p = 0;
                if (s < p-1 && *s == '"' && p[-1] == '"') {
                    *s++ = 0;
                    *--p = 0;
                }
                if (s <= p)
                    memmove(dst, s, strlen(s)+1);
                break;
            } else if (*s == '{') {     // {}�ɂ�镡���s��`
                char buf2[0x1000];
                int l, n = 0;
                dst[0] = 0;
                for (;;) {
                    s = txtf_gets(buf2, sizeof buf2);
                    if (s == NULL)
                        break;
                    s = (char*)strSkipSpc(s);
                    if (*s == '}')
                        break;
                    if (*s == '\0')
                        continue;
                  #ifdef CMT_2
                    if (*s == CMT_CHR && s[1] == CMT_CHR)
                        continue;
                    for (p = s; *(Uint8*)p >= 0x20 && *p != 0x7f; p++) {
                        // ���O���󔒂� // �̓R�����g����
                        if (p > s && p[-1] <= ' ' && *p == CMT_CHR && p[1] == CMT_CHR)
                            break;
                    }
                  #else
                    if (*s == CMT_CHR)
                        continue;
                    for (p = s; *(Uint8*)p >= 0x20 && *p != 0x7f; p++) {
                        if (p > s && p[-1] <= ' ' && *p == CMT_CHR)     // ���O���󔒂� ; �̓R�����g����
                            break;
                    }
                  #endif
                    while (p > s && p[-1] == ' ') --p;      // �P�c�ɂ���󔒂͍폜
                    *p = 0;
                    l = strlen(s);
                    if (n + l + 2 > len) {
                        l = len - n - 2;
                        if (l <= 0)
                            break;
                    }
                    if (l <= 0)
                        continue;
                    memcpy(dst+n, s, l);
                    n += l;
                    dst[n] = '\n';
                    n++;
                    dst[n] = '\0';
                }
                break;
            }
        }
    }
    return rc ? dst : NULL;
}


/** tn �̒l���擾 */
int     cfg_getVal(const char *tn)
{
    char buf[256], *rp, *s;
    int  rc = 0;

    txtf_openMem(cfg_buf, cfg_size);

    for (;;) {
        rp = txtf_gets(buf, 256);
        if (rp == NULL)
            break;
        rp = (char*)strSkipSpc(rp);
        if (memcmp(rp, tn, strlen(tn)) == 0) {  // �w�肵��TAGNAME�Ɠ������H
            s = (char*)strSkipSpc(rp+strlen(tn));
            if (*s == '=')
                s = (char*)strSkipSpc(s+1);
            rc = strtol(s, (char**)NULL, 0);        //rc = cfg_getval0(rp + strlen(tn));
            //DBG_F(("cfg[%d=%s]>%s",rc,s,rp));
            break;
        }
    }
    txtf_close();
    return rc;
}

};      // CfgFile_Tmp


// �񂹏W�߂��g���̂͂��������[
using namespace CfgFile_Tmp;

using namespace std;


/** ������ */
bool CfgFile::init(
    const char  *name,      ///< �t�@�C����
    int         memSw)      ///< 0:�t�@�C���ǂݍ��� !0:������
{
    int rc;
    if (memSw == 0) // �t�@�C����ǂݍ���
        rc = cfg_init(name);
    else            // �������ɂ����ꂽ��������t�@�C�������ɂ���
        rc = cfg_init4mem(name, strlen(name));
    return rc != 0;
}

void CfgFile::term()
{
    cfg_term();
}


/** tagName �̎��l���擾 */
int  CfgFile::getVal(const char *tagName)
{
    int val = cfg_getVal(tagName);
    return val;
}

/** tagName �̕ێ����镶������擾 */
bool CfgFile::getStr(string &dst, const char *tagName)
{
    char    buf[0x10000];
    buf[0] = 0;
    char *p = cfg_getStr(buf, sizeof buf, tagName);
    if (p)
        dst = p;
    else
        dst = "";
    return p != NULL && buf[0] != 0;
}


/** �t�@�C����path���X�g��vector lst�ɕ������� */
bool CfgFile::getStrVec(vector<string> &lst, const char *tagName)
{
    string wk("");
    int rc = getStr(wk, tagName);
    if (rc) {
        rc = str2vec(lst, wk.c_str());
    }
    return rc != 0;
}


/** �������'����'��'����'�ŋ�؂��āA������̃��X�g�����
 *  �s����%�����邩�A�󔒂̒����%������ꍇ�A���s�܂ł��R�����g�Ƃ��ēǂݔ�΂�
 */
int CfgFile::str2vec(vector<string> &lst, const char *src)
{
    const char  *s = src;

    lst.clear();
    while (*s) {
        const char *nm = s = strSkipSpc(s);
      #ifdef CMT_2
        if (*s == CMT_CHR && s[1] == CMT_CHR)
      #else
        if (*s == CMT_CHR)
      #endif
        {       // �R�����g����������A�X�L�b�v
            s = strchr(s, '\n');
            if (s == NULL) {
                break;
            }
            s++;
            continue;
        }
        if (*s == '\0')
            break;
        for (s = nm; *(Uint8*)s >= 0x20 && *s != 0x7f; s++) {
          #ifdef CMT_2
            // ���O���󔒂� // �̓R�����g����
            if (s > nm && s[-1] <= ' ' && *s == CMT_CHR && s[1] == CMT_CHR)
                break;
          #else
            if (s > nm && s[-1] <= ' ' && *s == CMT_CHR)        // ���O���󔒂� ; �̓R�����g����
                break;
          #endif
        }
        while (s > nm && s[-1] == ' ') --s;     // �P�c�ɂ���󔒂͍폜
        int    len = s - nm;
        string tmp("");
        tmp.assign(nm, len);
        lst.push_back(tmp);
    }
    return lst.size();
}


#if 0
#include <iostream>

int main()
{
    CfgFile cfg;

    cfg.init("c2htm.cfg");
    //cout << cfg_buf << endl;
    string st("");
    cfg.getStr(st, "C_SY");
    cout << "C_SY =" <<  st << endl;

    st.clear();
    cfg.getStr(st, "HEADER");
    cout << "HEADER =" <<  st << endl;

    st.clear();
    cfg.getStr(st, "WORD1");
    cout << "WORD1 =" <<  st << endl;

    st.clear();
    cfg.getStr(st, "SHARP");
    cout << "SHARP =" <<  st << endl;

    st.clear();
    cfg.getStr(st, "WORD4");
    cout << "WORD4 =" <<  st << endl;

    return 0;
}

#endif

