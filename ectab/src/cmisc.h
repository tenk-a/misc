/**
 *  @file   cmisc.h
 *  @brief  ��{�I��C����ŏ����ꂽ�G���ȃ��[�`���S.
 *
 *  @author Masashi Kitamura (tenka@6809.net)
 */


#ifndef CMISC_H
#define CMISC_H

#include <stddef.h>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__BORLANDC__)
#  include <mbstring.h>
#else	    // �V�t�gJIS��p.
#  define _ismbblead(c)     ((unsigned char)(c) >= 0x81 && ((unsigned char)(c) <= 0x9F || ((unsigned char)(c) >= 0xE0 && (unsigned char)(c) <= 0xFC)))
#  define _ismbbtrail(c)    ((unsigned char)(c) >= 0x40 && (unsigned char)(c) <= 0xfc && (c) != 0x7f)
#endif
#define ISKANJI(c)  	_ismbblead(c)
#define ISKANJI2(c) 	_ismbbtrail(c)

#ifdef __cplusplus
#include <string>
#endif

#ifdef __cplusplus
namespace CMISC {
#endif

//-------------------------------------------------------------------------
// C������֌W.

/// �����񖖂ɂ���󔒂��폜����.
char *strTrimSpcR(char str[], int flags);

/// �����񒆂̔��p�̑啶���̏�������,�������̑啶����.
char *strUpLow(char str[], unsigned flags);

/// @fn strTab src�����񒆂�tab���󔒂�,�󔒂̌q�����V����tab�ɕϊ����ĕ�����dst���쐬.
#ifdef __cplusplus
size_t strTab(char *dst, const char *src, int dstTabSz, int srcTabSz, int flags=0, int dstSz=0);
#else
size_t strTab(char *dst, const char *src, int dstTabSz, int srcTabSz, int flags, int dstSz);
#endif

#if __cplusplus
};  	// CMISC
#endif

//-------------------------------------------------------------------------

#endif	// CMISC_H
