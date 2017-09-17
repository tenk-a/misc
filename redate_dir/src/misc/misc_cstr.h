/**
 *  @file   misc_cstr.h
 *  @brief  c����������̎G���ȃ��[�`���Q
 *  @author tenk
 *  @note
 *  �� int 32�r�b�g��p. long ��32��64����cpu/compiler�ɂ��
 */

#ifndef MISC_CSTR_H
#define MISC_CSTR_H

#pragma once

#include "stdafx.h"
//#include "def.h"
//#include "misc_val.h"
#include <assert.h>

// MS�S�p�����̂��܍��킹�悤
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__BORLANDC__)
#include <mbstring.h>
#else	    // �V�t�gJIS��p
static inline int _ismbblead(int c)  { return ( ((uint8_t)(c) >= 0x81) & (((uint8_t)(c) <= 0x9F) | (((uint8_t)(c) >= 0xE0) & ((uint8_t)(c) <= 0xFC))) ); }
static inline int _ismbbtrail(int c) { return ( ((uint8_t)(c) >= 0x40) & ((uint8_t)(c) <= 0xfc) & ((c) != 0x7f) ); }
#endif


#define CALLOC	    calloc

#ifdef __cplusplus
#include <string>
// ===========================================================================
/// �G���ȃ��[�`���̂��߂̖��O��ԁB
//namespace MISC {
#endif



// ===========================================================================
// SJIS �֌W
// win/dos�n�Ȃ� mbs������֐����g���΂������A�^�[�Q�b�g�@�ł͂Ȃ��̂ŗގ��i�p��
// ===========================================================================

#define USE_FNAME_SJIS

/// SJIS�����̏�ʃo�C�g��?
static inline int ISKANJI(int c)    { return _ismbblead(c); }

/// SJIS�����̉��ʃo�C�g��?
static inline int ISKANJI2(int c)  { return _ismbbtrail(c); }

/// 1�o�C�g������sjis�ɕϊ�����
unsigned asc2sjis(unsigned jc);




// ===========================================================================
// c������֌W
// ===========================================================================

/// strncpy �ŁA�T�C�Y�����ς��̏ꍇ�ɍŌ��'\0'�ɂ��Ă���
static inline char *strNCpyZ(char *dst, const char *src, size_t size) {
    assert(dst != 0 && src != 0 && size > 0);
    strncpy(dst, src, size);
    dst[size - 1] = 0;
    return dst;
}


/// ������ s �̐擪�󔒕������X�L�b�v�����A�h���X��Ԃ�
static inline char *strSkipSpc(const char *s) {
    assert(s != 0);
    while (((*s != 0) & (*(const unsigned char *)s <= ' ')) | (*s == 0x7f))
    	s++;
    return (char*)s;
}


/// ������ s �̐擪�󔒕���(���slf�͏���)���X�L�b�v�����A�h���X��Ԃ�
static inline char *strSkipSpcNoLf(const char *s) {
    assert(s != 0);
    while (((*s != 0) && (*(const unsigned char *)s <= ' ' && *s != '\n')) || (*s == 0x7f))
    	s++;
    return (char*)s;
}


/// ������ s �̋󔒈ȊO�̕������X�L�b�v�����A�h���X��Ԃ�
static inline char *strSkipNSpc(const char *s) {
    assert(s != 0);
    while ((*s != 0) & (*(const unsigned char *)s > ' ') & (*s != 0x7f))
    	s++;
    return (char*)s;
}


/// c������̍Ō��\n������΂�����폜
static inline char *strDelLf(char s[]) {
    char *p;
    assert(s != 0);
    p = s + strlen(s);
    if (p != s && p[-1] == '\n')
    	p[-1] = 0;
    return s;
}


/// c������̍Ō��\n������΂�����폜
static inline char *strTrimR(char s[]) {
    char *p;
    assert(s != 0);
    p = s + strlen(s);
    while (p > s && isspace((unsigned char)p[-1]))
    	*--p = '\0';
    return s;
}


/// �]���� addSz �o�C�g���������m�ۂ��� strdup().
static inline char *strDup(const char *s, unsigned int addSz) {
    char *d = (char*)CALLOC(1, strlen(s) + 1 + addSz);
    return strcpy(d,s);
}


/// stpcpy(d,s) �̑�p�i.
static inline char *stpCpy(char *d, const char *s) {
    while ((*d = *s++) != '\0')
    	d++;
    return d;
}


/// ������̏�������啶���ɕϊ�. strupr�̑�p�i
static inline char *strUpr(char *src) {
    unsigned char *s = (unsigned char *)src;
    while (*s) {
    	if (islower(*s))
    	    *s = toupper(*s);
    	s++;
    }
    return src;
}


/// ������̑啶�����������ɕϊ�. strlwr �̑�p�i
static inline char *strLwr(char *src) {
    unsigned char *s = (unsigned char *)src;
    while (*s) {
    	if (isupper(*s))
    	    *s = tolower(*s);
    	s++;
    }
    return src;
}


/// �啶���������𓯈ꎋ���� strcmp. stricmp�̑�p�i
static inline int strICmp(const char *left, const char *right) {
    const unsigned char *l = (const unsigned char *)left;
    const unsigned char *r = (const unsigned char *)right;
    int c;
    while (((c = toupper(*l) - toupper(*r)) == 0) & (*l != '\0')) {
    	r++;
    	l++;
    }
    return c;
}


static inline int strEqu(const char *left, const char *right) {
    const unsigned char *l = (const unsigned char *)left;
    const unsigned char *r = (const unsigned char *)right;
    int c;
    while (((c = *l - *r) == 0) & (*l != '\0')) {
    	r++;
    	l++;
    }
    return c == 0;
}


#ifdef __cplusplus
inline int strEquLong(const char *left, const char *right, char const * *pNewLeft=0);
#endif
inline int strEquLong(const char *left, const char *right, char const * *pNewLeft) {
    const unsigned char *l = (const unsigned char *)left;
    const unsigned char *r = (const unsigned char *)right;
    while ((*l == *r) & (*r != '\0')) {
    	++r;
    	++l;
    }

    bool rc = (*r == '\0');
    if (rc) {	// left �� right �Ɠ������]���ɕ���������Ȃ�� �^
    	if (pNewLeft)	// �^�̎���
    	    *pNewLeft = (const char *)l;
    }
    return rc;
}


/// str����sep �̉��ꂩ�̕����ŋ�؂�ꂽ�������tok�Ɏ擾. strtok()�̗ގ��i
char *strGetTok(char tok[], const char *str, const char *sep);


/// �����񖖂ɂ���󔒂��폜����
char *strTrimSpcR(char str[], int flags);

// /// �����񒆂̔��p�̑啶���̏�������,�������̑啶����
// char *strUpLow(char str[], unsigned flags);

// /// @fn strTab src�����񒆂�tab���󔒂�,�󔒂̌q�����V����tab�ɕϊ����ĕ�����dst���쐬.
// int strTab(char *dst, const char *src, int dstTabSz, int srcTabSz, int flags, int dstSz);



#ifdef __cplusplus
inline std::string strTrim(const std::string &str, const char *skipChrs = " \t\n") {
    if (str.size() == 0)
    	return str;
    std::size_t sn = str.find_first_not_of(skipChrs);
    std::size_t en = str.find_last_not_of(skipChrs);
    if (sn == std::string::npos)
    	return std::string("");
    return str.substr(sn, en+1 - sn);
}
#endif



/// crc32 ���v�Z
unsigned int memCrc32(const void *dat, unsigned int siz);


/// mem ���� sz �o�C�g�̃T�������߂�
static inline uint64_t memSum64(const uint64_t *mem, uint32_t sz) {
    uint64_t sum = 0;
    sz = sz >> 3;
    if (sz) {
    	do {
    	    sum += *mem++;
    	} while (--sz);
    }
    return sum;
}



// ===========================================================================
// �t�@�C�����֌W
// ===========================================================================

/// �p�X�����̃t�@�C�����ʒu��T��.
static inline char *fname_getBase(const char *adr) {
    const char *p = adr;
    assert( adr != 0 );
    while (*p != '\0') {
    	if ((*p == ':') | (*p == '/') | (*p == '\\'))
    	    adr = p + 1;
      #ifdef USE_FNAME_SJIS
    	if (ISKANJI((*(unsigned char *) p)) & (p[1] != 0))
    	    p++;
      #endif
    	p++;
    }
    return (char*)adr;
}


/// �g���q�̈ʒu��Ԃ��B�Ȃ���Ζ��O�̍Ō��Ԃ�.
static inline char *fname_getExt(const char *name) {
    char *p;
    name = fname_getBase(name);
    p = strrchr((char*)name, '.');
    if (p)
    	return p+1;
    return (char *)(name+strlen(name));
}


/// �g���q��t������
static inline char *fname_addExt(char filename[], const char *ext) {
    if (strrchr(fname_getBase(filename), '.') == NULL) {
    	strcat(filename, ".");
    	strcat(filename, ext);
    }
    return filename;
}


/** filename�̊g���q��ext�ɕt���ւ���.
 *  ext=""����'.'���c�邪�Aext=NULL�Ȃ�'.'���Ɗg���q���O��.
 */
static inline char *fname_chgExt(char filename[], const char *ext) {
    char *p = (char *) fname_getBase(filename);
    p = strrchr(p, '.');
    if (p == NULL) {
    	if (ext) {
    	    strcat(filename, ".");
    	    strcat(filename, ext);
    	}
    } else {
    	if (ext == NULL)
    	    *p = 0;
    	else
    	    strcpy(p + 1, ext);
    }
    return filename;
}


#ifdef __cplusplus
/// �g���q��t���ւ���
// std::string& fname_chgExt(std::string &fname, const char *ext);
#endif

/// path ���� ./ �� ../ �̃T�u�f�B���N�g�������[���ɏ]���č폜 (MS-DOS�ˑ�)
char *fname_delDotDotDir(char *path);

/** ������̍Ō�� \ �� / ������΍폜 */
char *fname_delLastDirSep(char dir[]);



#ifdef __cplusplus
//} // namespace MISC
#endif


#endif	// MISC_CSTR_H
