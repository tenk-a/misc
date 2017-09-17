/**
 *  @file   tga_wrt.h
 *  @brief  tga�摜�o��.
 *  @author Masashi Kitamura
 */
#ifndef TGA_WRT_H
#define TGA_WRT_H

#ifdef __cplusplus
extern "C" {
#endif

/// tga�摜�C���[�W�̐���.
int  tga_write( void *tga_data,     	    	    	    // tga�C���[�W���i�[����o�b�t�@.
    	    	int w, int h, int bpp,	    	    	    // �o�͂̉���,�c��, bpp
    	    	const void *src, int srcWb, int srcBpp,     // ���͂̃s�N�Z��,�����o�C�g��,bpp
    	    	const void *clut, int dir); 	    	    // ���͂� clut, ���̓s�N�Z���̃��C���̏㉺.

/// tga�摜�C���[�W�̐���.
int  tga_writeEx(void *tga_data, int dataSiz,	    	    // tga�C���[�W���i�[����o�b�t�@, �Ƃ��̃T�C�Y,
    	    	int w, int h, int dstBpp,   	    	    // �o�͂̉���,�c��, bpp
    	    	const void* src, int srcWb, int srcBpp,     // ���͂̃s�N�Z��,�����o�C�g��,�c��,bpp
    	    	const void *clut0, int clutBpp,     	    // ���͂� clut �ƁA�o�͂�clut��bpp
    	    	int dir,    	    	    	    	    // ���͂̃s�N�Z�� 0:�ド�C������ 1:�����C������.
    	    	int x0, int y0);    	    	    	    // �o�͂̃w�b�_�ɓ����n�_ x,y. ����0,0.


/// �w��bpp���A����𖞂������ۂ� tga �ŃT�|�[�g����� bpp �ɕϊ�.
int  tga_chkDstBpp(int bpp);

/// w,h,bpp����tga�C���[�W�ɕK�v�ȃo�C�g����Ԃ�(��ڂɕԂ�).
int  tga_encodeWorkSize(int w, int h, int bpp);



#ifdef __cplusplus
}
#endif




// ===========================================================================
// (��L�֐��̎g�p��)

// inline ���w��ł���ꍇ.
#if (defined __cplusplus) || (defined inline) || (__STDC_VERSION__ >= 199901L) || (defined __GNUC__)

// �\�� stdlib.h ��include���Ă���Ƃ��̂ݗ��p�\.
#if (defined _INC_STDLIB/*VC,BCC*/) || (defined __STDLIB_H/*DMC,BCC*/) || (defined _STDLIB_H_/*GCC*/) \
    || (defined _STDLIB_H_INCLUDED/*watcom*/) || (defined _MSL_STDLIB_H/*CW*/)
#include <stdlib.h> 	// calloc,free�̂���.

/** �w�肵���摜���Amalloc������������tga�摜�ɂ��ĕԂ�. ���s�����NULL.
 *  �� srcWidByt�͌��摜�̉����o�C�g���ŁA0 ����w��bpp����W���X�g�̒l�����߂�.
 */
static inline void* tga_writeMalloc(
    	int w, int h, int dstBpp,   	    	    // �o�͂̉���,�c��, bpp
    	const void* src, int srcWidByt, int srcBpp, // ���͂̃s�N�Z��,�����o�C�g��,�c��,bpp
    	const void *clut,   	    	    	    // ���͂� clut ��
    	int dir,    	    	    	    	    // �s�N�Z���� 0:�ド�C������ 1:�����C������.
    	unsigned* pSize)    	    	    	    // ���������T�C�Y
{
    unsigned	dbpp = (dstBpp > 0) ? dstBpp : srcBpp;
    unsigned	bpp  = tga_chkDstBpp(dbpp);
    unsigned	sz   = tga_encodeWorkSize(w, h, bpp);
    void    	*m   = calloc(1, sz);
    if (m == NULL)
    	return NULL;

    sz = tga_write(m, w, h, bpp, src, srcWidByt, srcBpp, clut, dir);
    if (sz == 0) {
    	free(m);
    	m = NULL;
    }
    if (pSize)
    	*pSize = sz;
    return m;
}
#endif


// �\�� stdio.h ��include���Ă���Ƃ��̂ݗ��p�\.
#if (defined _INC_STDIO/*VC,BCC*/) || (defined __STDIO_H/*DMC,BCC*/) || (defined _STDIO_H_/*GCC*/) \
    || (defined _STDIO_H_INCLUDED/*watcom*/) || (defined _MSL_STDIO_H/*CW*/)
#include <stdio.h>

#if defined __cplusplus
static int tga_write_file(const char *fname, int w, int h, int dstBpp,
    	 const void* src, int srcWidByt=0, int srcBpp=0, const void* clut=0, int dir=0);
#endif

/** tga�t�@�C������.
 *  �� srcWidByt�͌��摜�̉����o�C�g���ŁA0 ����w��bpp����W���X�g�̒l�����߂�.
 */
static inline int tga_write_file(
    const char* fname,	    	    	    	// �o�̓t�@�C����
    int w, int h, int dstBpp,	    	    	// �o�͂̉���,�c��, bpp
    const void* src, int srcWidByt, int srcBpp, // ���͂̃s�N�Z��,�����o�C�g��,�c��,bpp
    const void *clut,	    	    	    	// ���͂� clut
    int dir)	    	    	    	    	// �s�N�Z���� 0:�ド�C������ 1:�����C������.
{
    size_t   l	= (unsigned)-1;
    unsigned sz = 0;
    void*    m	= tga_writeMalloc(w,h,dstBpp, src, srcWidByt, srcBpp, clut, dir, &sz);
    if (m) {
    	if (sz) {
    	    FILE* fp = fopen(fname, "wb");
    	    if (fp) {
    	    	l = fwrite(m, 1, sz, fp);
    	    	fclose(fp);
    	    }
    	}
    	free(m);
    }
    return l == (size_t)sz;
}
#endif

#endif	// inline���g����ꍇ.



#endif	// TGA_WRT_H
