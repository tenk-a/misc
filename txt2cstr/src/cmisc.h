/**
 *  @file   cmisc.h
 *  @brief  ��{�I��C����ŏ����ꂽ�G���ȃ��[�`���S
 *
 *  @author �k����j<NBB00541@nifty.com>
 */


#ifndef CMISC_H
#define CMISC_H


#define STDERR	    	stdout
//#define STDERR    	stderr
#define CERR	    	cout
//#define CERR	    	cerr


#define ISKANJI(c)  	((unsigned char)(c) >= 0x81 && ((unsigned char)(c) <= 0x9F || ((unsigned char)(c) >= 0xE0 && (unsigned char)(c) <= 0xFC)))
#define ISKANJI2(c) 	((unsigned char)(c) >= 0x40 && (unsigned char)(c) <= 0xfc && (c) != 0x7f)

#ifdef __cplusplus
#include <string>
#endif

#ifdef __cplusplus
namespace CMISC {
#endif

//-------------------------------------------------------------------------
// C������֌W

/// �Ō�ɕK��\0 ��u�� strncpy
char *strNCpyZ(char *dst, const char *src, unsigned size);

/// �Ō��\n������΂�����폜
char *strDelLf(char s[]);

/// �ȈՂɓ����o�b�t�@��sprintf�����Ă��̃A�h���X��Ԃ�
char *strTmpF(const char *fmt, ...);

/// �]���� addSz �o�C�g���������m�ۂ��� strdup()
char *strDup(const char *s, int addSz);

#if defined(__cplusplus) || defined(inline)
/// @fn strSkipSpc �󔒂�ǂݔ�΂�
inline char *strSkipSpc(const char *s) {
    while ((*s && *(const unsigned char *) s <= ' ') || *s == 0x7f) {s++;}
    return (char*)s;
}
#else
char *strSkipSpc(const char *s);
#endif

/// crc table
extern unsigned int memCrc32table[256];

/// crc32 ���v�Z
int memCrc32(void *dat, int siz);


/// �����񖖂ɂ���󔒂��폜����
char *strTrimSpcR(char str[], int flags);

/// �����񒆂̔��p�̑啶���̏�������,�������̑啶����
char *strUpLow(char str[], unsigned flags);

/// @fn strTab src�����񒆂�tab���󔒂�,�󔒂̌q�����V����tab�ɕϊ����ĕ�����dst���쐬.
#ifdef __cplusplus
int strTab(char *dst, const char *src, int dstTabSz, int srcTabSz, int flags=0, int dstSz=0);
#else
int strTab(char *dst, const char *src, int dstTabSz, int srcTabSz, int flags, int dstSz);
#endif



//-------------------------------------------------------------------------
// �t�@�C�����֌W

/// fname_ �n�� SJIS��Ώ�����
#define USE_FNAME_SJIS

/// �p�X�����̃t�@�C�����ʒu��T��(MS-DOS�ˑ�)
char *fname_getBase(const char *adr);

/// �g���q�̈ʒu��Ԃ��B�Ȃ���Ζ��O�̍Ō��Ԃ�.
char *fname_getExt(const char *name);

/// �g���q��t���ւ���
char *fname_chgExt(char filename[], const char *ext);

/// �g���q��t������
char *fname_addExt(char filename[], const char *ext);

/// path ���� ./ �� ../ �̃T�u�f�B���N�g�������[���ɏ]���č폜 (MS-DOS�ˑ�)
char *fname_delDotDotDir(char *path);

#ifdef __cplusplus
std::string& fname_chgExt(std::string &fname, const char *ext);
#endif

#if __cplusplus
};  	// CMISC
#endif


//-------------------------------------------------------------------------



#endif	// CMISC_H
