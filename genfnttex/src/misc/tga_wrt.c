/**
 *  @file   tga_wrt.c
 *  @brief  tga�摜�o��.
 *  @author Masashi Kitamura
 */

#include "tga_wrt.h"
#include <stddef.h>
#include <string.h>


/*--------------------------------------------------------------------------*/
/* �R���p�C�����̒��날�킹. */


#if (defined _MSC_VER) || (defined __BORLANDC__ && __BORLANDC__ <= 0x0551)
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned       uint32_t;
#else
#include <stdint.h>
#endif


#if !defined(inline) && !(defined __GNUC__) && (__STDC_VERSION__ < 199901L)
#define inline	    	__inline
#endif



#ifdef _WINDOWS     	// win�A�v���ŁA�Ȃ�ׂ� C���C�u�����������N�������Ȃ��ꍇ�p.
#include <windows.h>
#define MALLOC(sz)  	    LocalAlloc(LMEM_FIXED, (sz))
#define FREE(p)     	    LocalFree(p)
#else
#include <stdlib.h> 	/* NULL, malloc, free ���g�p */
#define MALLOC(sz)  	    malloc(sz)
#define FREE(p)     	    free(p)
#endif



/*--------------------------------------------------------------------------*/
//x #define MY_EX   	    /* ������`����ƁA�Ǝ���bpp=12(A4R4G4B4)�t�H�[�}�b�g�ɑΉ� */

#define BBBB(a,b,c,d)	((((uint8_t)(a))<<24)+(((uint8_t)(b))<<16)+(((uint8_t)(c))<<8)+((uint8_t)(d)))

#define GLB(a)	    	((uint8_t)(a))
#define GHB(a)	    	GLB(((uint16_t)(a))>>8)

#define PEEKW(a)    	(*(uint16_t *)(a))
#define PEEKD(a)    	(*(uint32_t *)(a))
#define POKEB(a,b)  	(*(uint8_t  *)(a) = (b))
#define POKEW(a,b)  	(*(uint16_t *)(a) = (b))
#define POKED(a,b)  	(*(uint32_t *)(a) = (b))

#if defined _M_IX86 || defined _X86_ || defined _M_AMD64 || defined __amd64__	// X86 �́A�A���C�����g���C�ɂ���K�v���Ȃ��̂Œ��ڃA�N�Z�X
 // DM-C v8.41�ł�-j0�̃o�O�΍��, �}�N�����Ƀ}�N���������Ȃ��悤�ɏC��.
 #define POKEiW(a,b)	(*(uint16_t *)(a) = (b))
#else
 #define POKEiW(a,b)	(POKEB((a),GLB(b)), POKEB((ptrdiff_t)(a)+1,GHB(b)))
#endif

#define BPP2BYT(bpp)	(((bpp) > 24) ? 4 : ((bpp) > 16) ? 3 : ((bpp) > 8) ? 2 : 1)
#define WID2BYT(w,bpp)	(((w) * "\1\2\4\4\10\10\10\10\20\20\20\20\20\20\20\20\30\30\30\30\30\30\30\30\40\40\40\40\40\40\40\40"[(bpp)-1] + 7) >> 3)
#define WID2BYT4(w,bpp) ((WID2BYT(w,bpp) + 3) & ~3)
#define BYT2WID(w,bpp)	(((bpp) > 24) ? (w)>>2 : ((bpp) > 16) ? (w)/3 : ((bpp) > 8) ? (w)>>1 : ((bpp) > 4) ? (w) : ((bpp) > 2) ? ((w)<<1) : ((bpp) > 1) ? ((w)<<2) : ((w)<<3))



#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------*/
/* �o�͂̂��߂̏������ݏ���(�G���f�B�A���΍�)	    	    	    	    */


#if defined _M_IX86 || defined _X86_ || defined _M_AMD64 || defined __amd64__	// X86 �́A�A���C�����g���C�ɂ���K�v���Ȃ��̂Œ��ڃA�N�Z�X
/* �ėp�ł͂Ȃ��A���̃t�@�C���݂̂ŗL���ȃ}�N���B */
/* ���� a �͕K��uint8_t*�^�ϐ��ւ̃|�C���^�ł��邱��! */
/* mput_b3()�� c �͕���p�̂Ȃ��l�ł��邱�ƁI */

#define mput_b1(a, c)	    (*((*a)++) = (c))
#define mput_b2(a, c)	    (*(uint16_t*)(*a) = (c),  (*a) += 2)
#define mput_b3(a, c)	    (*(uint16_t*)(*a) = (c),  (*a)[2] = (c)>>16, (*a) += 3)
#define mput_b4(a, c)	    (*(uint32_t*)(*a) = (c),  (*a) += 4)
#define mput_cpy(d,s,l,n)   memcpy((d),(s),(l)*(n))

#else

#define mput_b1(a, c)	    (*((*a)++) = (c))

/** 2�o�C�g�������������� */
static inline void mput_b2(uint8_t **a, int c)
{
    uint8_t *d = *a;
    /*DBG_F(("b2:%x  %04x\n", d, c)); */
    *d++ = (uint8_t)c;
    *d++ = (uint8_t)(c>>8);
    *a	 = d;
}

/** 3�o�C�g�������������� */
static inline void mput_b3(uint8_t **a, int c)
{
    uint8_t *d = *a;
    /*DBG_F(("b3:%x  %06x\n", d, c)); */
    *d++ = (uint8_t)c;
    *d++ = (uint8_t)(c>>8);
    *d++ = (uint8_t)(c>>16);
    *a	 = d;
}

/** 4�o�C�g�������������� */
static inline void mput_b4(uint8_t **a, int c)
{
    uint8_t *d = *a;
    /*DBG_F(("b4:%x  %08x\n", d, c)); */
    *d++ = (uint8_t)c;
    *d++ = (uint8_t)(c>>8);
    *d++ = (uint8_t)(c>>16);
    *d++ = (uint8_t)(c>>24);
    *a	 = d;
}

#if 1
#define mput_cpy(d,s,l,n)   memcpy((d),(s),(l)*(n))
#else
static inline void mput_cpy(void *dst, void const* src, int len, int n)
{
  #if 0
    uint8_t *d = (uint8_t*)dst;
    if (n == 4) {
    	uint32_t *s = (uint32_t const*)src;
    	do {
    	    mput_b4(&d, *s++);
    	} while (--len);
    } else if (n == 2) {
    	uint16_t *s = (uint16_t*)src;
    	do {
    	    mput_b2(&d, *s++);
    	} while (--len);
    } else
  #endif
    {
    	memcpy(dst, src, len * n);
    }
}
#endif

#endif


/*--------------------------------------------------------------------------*/

static inline void   PutPx1(uint8_t **dp, int c, int dstBpp)
{
    uint8_t *d = *dp;
    int   r, g, b, a;

    if (dstBpp == 8) {
    	r = (uint8_t)(c >> 16);
    	g = (uint8_t)(c >>  8);
    	b = (uint8_t)(c >>  0);
    	mput_b1(&d, ((g >> 5)<<5) | ((r >> 5)<<2) | (b >> 6));
  #if 0 //def MY_EX
    } else if (dstBpp == 12) {
    	a = ((uint32_t)c >> 24);
    	a = (a + 15) >> 4; if (a > 15) a = 15;
    	r = (uint8_t)(c >> 16);
    	g = (uint8_t)(c >>  8);
    	b = (uint8_t)(c >>  0);
    	c = (a << 12) | ((r >> 4)<<8) | ((g >> 4)<<4) | (b >> 4);
    	mput_b2(&d, c);
  #endif
  #if 0
    } else if (dstBpp == 15) {
    	r = (uint8_t)(c >> 16);
    	g = (uint8_t)(c >>  8);
    	b = (uint8_t)(c >>  0);
    	c = ((r >> 3)<<10) | ((g >> 3)<<5) | (b >> 3);
    	mput_b2(&d, c);
  #endif
    } else if (dstBpp == 16 || dstBpp == 15) {
    	a = ((uint32_t)c >> 24) ? 0x8000 : 0;
    	r = (uint8_t)(c >> 16);
    	g = (uint8_t)(c >>  8);
    	b = (uint8_t)(c >>  0);
    	c = a | ((r >> 3)<<10) | ((g >> 3)<<5) | (b >> 3);
    	mput_b2(&d, c);
    } else if (dstBpp == 24) {
    	mput_b3(&d, c);
    } else {
    	mput_b4(&d, c);
    }
    *dp = d;
}




/*--------------------------------------------------------------------------*/
static int  tga_wrt_putPixs(uint8_t *dst, int dstSiz, int w, int h, int bpp, const uint8_t *src, int src_wb, int srcBpp, uint32_t *clut, int dir, int bpp4Cnv);
static ptrdiff_t tga_encode(uint8_t *d, const uint8_t *s, int w, int h, int bypp);

enum { F_BPP4CNV = 1 << 3, };

/** targa �摜�̐��� (���ʈ�����) */
int  tga_write(
    void    	*tga_data,  ///< ������
    int     	w,  	    ///< �����摜�̉���
    int     	h,  	    ///< �����摜�̏c��
    int     	bpp,	    ///< �����摜�� BPP
    const void	*src,	    ///< ���摜
    int     	srcWb,	    ///< ���摜�̉��o�C�g��
    int     	srcBpp,     ///< ���摜�� BPP. 8,16,24,32�̂���. (4���Ή��Ȃ̂Œ���)
    const void	*clut0,     ///< �F�p���b�g
    int     	dir)	    ///< tga_writeEx() ���Q��
{
    int clutBpp = 24;
    if (dir & 0x40)
    	clutBpp = 32;
    return tga_writeEx(tga_data, 0x7FFFFFFF, w, h, bpp, src, srcWb, srcBpp, clut0, clutBpp, dir, 0,0);
}


/** targa �摜�̐���
 *  @return ���������T�C�Y��Ԃ��B
 */
int  tga_writeEx(
    void    	*tga_data,  /**< ������     	    	    	    */
    int     	dataSiz,    /**< ������T�C�Y	    	    	    */
    int     	w,  	    /**< �����摜�̉���     	    	    */
    int     	h,  	    /**< �����摜�̏c��     	    	    */
    int     	dstBpp,     /**< �����摜�� BPP     	    	    */
    const void	*src0,	    /**< ���摜     	    	    	    */
    int     	src_wb,     /**< ���摜�̉��o�C�g�� 	    	    */
    int     	srcBpp,     /**< ���摜�� BPP. 4,8,16,24,32 	    */
    const void	*clut0,     /**< �F�p���b�g 	    	    	    */
    int     	clutBpp,    /**< �F�p���b�g�� BPP   	    	    */
    int     	flags,	    /**< bit 0: ���͂����摜�̊i�[���� 0:�ʏ� 1:�c�t(bmp�΍�)	    	    <br>
    	    	    	     *	 bit 3: ���͂����摜��bpp=4�̂Ƃ��̏㉺�̋l�ߏ��𔽓]	    	    <br>
    	    	    	     *	 bit 4: �����ł̊i�[����(tga�ł�) 0:���� 1:���� (2,3�͖��T�|�[�g)   <br>
    	    	    	     *	 bit 7: on �Ȃ爳�k����
    	    	    	     */
    int     	x0, 	    /**< tga�w�b�_�̐��̎n�_x�B�ʏ� 0���w�� */
    int     	y0) 	    /**< tga�w�b�_�̐��̎n�_y�B�ʏ� 0���w�� */
{
    uint32_t	clut1[256];
    uint8_t 	*d  	= (uint8_t*)tga_data;
    uint32_t	*clut	= (uint32_t*)clut0;
    uint8_t 	*src	= (uint8_t*)src0;
    int     	cm  	= (flags & 0x80) ? 8 : 0;
    int     	dn  	= BPP2BYT(dstBpp);
//x int     	sn  	= BPP2BYT(srcBpp);
    int     	cn  	= BPP2BYT(clutBpp);
    int     	dr  	= (flags & 0x10) | 0x0;     /* �����͂��ƂłȂ�Ƃ�����B */
    int     	i;
    int     	n;
    int     	dir 	= (flags & 1) ^ ((dr>>4)^1);

    if (dstBpp == 0)
    	dstBpp = srcBpp;

    flags &= F_BPP4CNV;
    src_wb = (src_wb) ? src_wb : WID2BYT(w, srcBpp);

    /*DBG_F(("dstBpp = %d dir=%x dr=%x\n",dstBpp,dir,dr)); */

    /* �w�b�_�[�쐬 */
    if (dstBpp == 8) {
    	dataSiz -= 0x12 + 0x100*cn;
    	if (dataSiz < w * h)
    	    return 0;
    	mput_b2(&d, 0x100); 	    /* ?? */
    	mput_b1(&d,  1+cm); 	    /* id */
    	mput_b2(&d, 	0); 	    /* clutTop */
    	mput_b2(&d, 0x100); 	    /* clutNum */
    	mput_b1(&d, clutBpp);	    /* clutBpp. */
    	mput_b2(&d,    x0); 	    /* x0 */
    	mput_b2(&d,    y0); 	    /* y0 */
    	mput_b2(&d, 	w); 	    /* w */
    	mput_b2(&d, 	h); 	    /* h */
    	mput_b1(&d, 	8); 	    /* dstBpp */
    	n =  (clutBpp == 32) ? 8 : (clutBpp == 16) ? 1 : 0; 	/* �����͂��ƂłȂ�Ƃ����� */
    	mput_b1(&d,  dr|n);

    	if (srcBpp > 8) {
    	    int r,g,b;
    	    clut = clut1;
    	    for (i = 0; i < 256; ++i) {
    	    	r = ((i & 0x1c) << 3);
    	    	r = r | (r >> 3) | (r >> 6);
    	    	g = i & 0xE0;
    	    	g = g | (g >> 3) | (g >> 6);
    	    	b = i & 3;
    	    	b = (b << 6) | (b << 4) | (b << 2) | b;
    	    	clut[i] = BBBB(0xff, g,r,b);
    	    }
    	} else if (srcBpp == 4) {
    	    for (i = 0; i < 256; ++i)
    	    	clut1[i] = BBBB(0xFF,0,0,0);
    	    memcpy(clut1, clut, 16*4);
    	    clut = clut1;
    	}
    	for (i = 0; i < 256; i++) {
    	    /*DBG_F(("\t%3d %08x\n", i, clut[i])); */
    	  #if 1
    	    PutPx1(&d, clut[i], clutBpp);
    	  #else
    	    switch (cn) {
    	    case 2: mput_b2(&d, clut[i]); break;
    	    case 3: mput_b3(&d, clut[i]); break;
    	    default:mput_b4(&d, clut[i]); break;
    	    }
    	  #endif
    	}
    } else {
    	dataSiz -= 0x12;
    	if (dataSiz < w * h * dn)
    	    return 0;
    	mput_b2(&d, 0x000); 	    /* ?? */
    	mput_b1(&d,  2+cm); 	    /* id */
    	mput_b2(&d, 	0); 	    /* clutTop */
    	mput_b2(&d, 	0); 	    /* clutNum */
    	mput_b1(&d, 	0); 	    /* clutBpp.  32�r�b�g�Œ� */
    	mput_b2(&d,    x0); 	    /* x0 */
    	mput_b2(&d,    y0); 	    /* y0 */
    	mput_b2(&d, 	w); 	    /* w */
    	mput_b2(&d, 	h); 	    /* h */
    	n = (dstBpp == 15) ? 16 : dstBpp;
    	mput_b1(&d,   n);   	    /* dstBpp */
      #if 0 //def MY_EX
    	n =  (dstBpp == 32) ? 8 : (dstBpp == 16||dstBpp == 12) ? 1 : 0; /* �����͂��ƂłȂ�Ƃ����� */
      #else
    	n =  (dstBpp == 32) ? 8 : (dstBpp == 16) ? 1 : 0;   	    	/* �����͂��ƂłȂ�Ƃ����� */
      #endif
    	mput_b1(&d,  dr|n);
    }
    if (cm == 0) {  /*�����k */
    	tga_wrt_putPixs(d, dataSiz, w, h, dstBpp, src, src_wb, srcBpp, clut, dir, flags);
    	d += w * h * dn;
    } else {
    	if (dstBpp == srcBpp && w * dn == src_wb && dir == 0) {
    	    /* ���o�͂̐F���A�������ꏏ�ŁA�������������̂Ƃ� */
    	    d += tga_encode(d, src, w, h, BPP2BYT(dstBpp));
    	} else {
    	    uint8_t *m = (uint8_t*)MALLOC(w * h * dn);
    	    if (m == NULL)
    	    	return 0;
    	    tga_wrt_putPixs(m, w*h*dn, w, h, dstBpp, src, src_wb, srcBpp, clut, dir, flags);
    	    d += tga_encode(d, m, w, h, BPP2BYT(dstBpp));
    	    FREE(m);
    	}
    }

  #if 0 //def MY_EX
    if (clutBpp != 12)	    //clut��12�r�b�g�F�͎����p�������[�`���Ȃ̂Ńt�b�^��t�����Ȃ��ł��܂�
  #endif
    {	/* �ʏ�� �t�@�C���t�b�^��t�� */
    	mput_b4(&d, 0); /* �g���̈�̃t�@�C���ʒu */
    	mput_b4(&d, 0); /* �f�x���b�p�f�B���N�g���̃t�@�C���ʒu */
    	mput_cpy(d, "TRUEVISION-XFILE.", 18, 1);
    	/*mput_cpy(d, "TRUEVISION-TARGA.", 18, 1); */
    	d += 18;
    }
    return d - (uint8_t*)tga_data;
}



static inline int    GetPx1(uint8_t const **sp, int srcBpp, uint32_t *clut)
{
    const uint8_t   *s = *sp;
    int     	    c;
    int     	    r,g,b,a;

    if (srcBpp <= 8) {
    	c = clut[*s++];
  #if 0 //def MY_EX
    } else if (srcBpp <= 12) {
    	c  = PEEKW(s);
    	b  = ((c      ) & 0xf); b  |= (b << 4);
    	g  = ((c >>  4) & 0xf); g  |= (g << 4);
    	r  = ((c >>  8) & 0xf); r  |= (r << 4);
    	a  = (c >>  12);    	a  |= (a << 4);
    	c = BBBB(a, r, g, b);
    	s += 2;
  #endif
  #if 0
    } else if (srcBpp <= 15) {
    	c  = PEEKW(s);
    	b  = ((c      ) & 0x1f) << 3;	b |= b>>5;
    	g  = ((c >>  5) & 0x1f) << 3;	g |= g>>5;
    	r  = ((c >> 10) & 0x1f) << 3;	r |= r>>5;
    	c = BBBB(0, r, g, b);
    	s += 2;
  #endif
    } else if (srcBpp <= 16) {
    	c  = PEEKW(s);
    	b  = ((c      ) & 0x1f) << 3;	b |= b>>5;
    	g  = ((c >>  5) & 0x1f) << 3;	g |= g>>5;
    	r  = ((c >> 10) & 0x1f) << 3;	r |= r>>5;
    	a  = (c & 0x8000) ? 0xFF : 0;
    	c = BBBB(a, r, g, b);
    	s += 2;
    } else if (srcBpp <= 24) {
    	c = BBBB(0, s[2], s[1], s[0]);
    	s += 3;
    } else {
    	c = PEEKD(s);
    	s += 4;
    }
    *sp = s;
    return c;
}


static int  tga_wrt_putPixs(
    uint8_t 	    *dst,
    int     	    dstSiz,
    int     	    w,
    int     	    h,
    int     	    dstBpp,
    const uint8_t   *src,
    int     	    src_wb,
    int     	    srcBpp,
    uint32_t	    *clut,
    int     	    dir,
    int     	    bpp4cnv)
{
    int     	    c;
    int     	    spat;
    int     	    dpat    = 0;
    int     	    dn	    = BPP2BYT(dstBpp);
    uint8_t 	    *d	    = dst;
    const uint8_t   *s	    = src;
    int     	    dst_wb  = WID2BYT(w, dstBpp);
    int     	    srcN    = WID2BYT(w, srcBpp);
    int     	    x;
    int     	    y,y0,y1,yd;

    src_wb = (src_wb) ? src_wb : ((srcN+3) & ~3);

    if (dstSiz < dst_wb * h)
    	return 0;

    spat = src_wb - srcN;
    if (spat < 0) {
    	c    = BYT2WID(src_wb, srcBpp);
    	spat = src_wb - WID2BYT(c, srcBpp);
    	dpat = (w - c) * dn;
    	w    = c;
    }

    if (dstBpp == srcBpp) {
    	/* ���o�͂������F���̂Ƃ� */
    	if (dir == 0 && dst_wb == src_wb) {
    	    /*���������Ő������Ȃ�� */
    	    mput_cpy(d, s, w*h, dn);
    	} else {
    	    /*spat =  src_wb; */
    	    dpat =  dst_wb;
    	    y0	= 0, y1 = h, yd = +1;
    	    if (dir & 1) {
    	    	y0  = h-1, y1 = -1, yd = -1;
    	    	d += (h - 1) * dst_wb;
    	    	dpat = -dpat;
    	    }
    	  #if 0
    	    if (((w * dn)&3) == 0) {	/* 4�Ŋ���؂��T�C�Y�̂Ƃ� */
    	    	for (y = y0; y != y1; y += yd) {
    	    	    MEMCPY4(d, s, w * dn);
    	    	    s += src_wb;
    	    	    d += dpat;
    	    	}
    	    } else
    	  #endif
    	    {
    	    	for (y = y0; y != y1; y += yd) {
    	    	    mput_cpy(d, s, w, dn);
    	    	    s += src_wb;
    	    	    d += dpat;
    	    	}
    	    }
    	}

  #if 1 // 2007-06  16�F��̓��͑Ή�
    } else if (srcBpp == 4) {
    	y0  = 0, y1 = h, yd = +1;
    	if (dir & 1) {
    	    y0	= h-1, y1 = -1, yd = -1;
    	    d += (h - 1) * dst_wb;
    	    dpat = dpat - (dst_wb * 2);
    	}
    	for (y = y0; y != y1; y += yd) {
    	    for (x = 0; x < w; x+=2) {
    	    	int c;
    	    	if (dstBpp == 8) {
    	    	    if (bpp4cnv) {
    	    	    	c = *s & 0x0f;
    	    	    	*d++ = c;
    	    	    	c = *s >> 4;
    	    	    	*d++ = c;
    	    	    } else {
    	    	    	c = *s >> 4;
    	    	    	*d++ = c;
    	    	    	c = *s & 0x0f;
    	    	    	*d++ = c;
    	    	    }
    	    	} else {
    	    	    if (bpp4cnv) {
    	    	    	c = *s & 0x0f;
    	    	    	PutPx1(&d, c, dstBpp);
    	    	    	c = *s >> 4;
    	    	    	PutPx1(&d, c, dstBpp);
    	    	    } else {
    	    	    	c = *s >> 4;
    	    	    	PutPx1(&d, c, dstBpp);
    	    	    	c = *s & 0x0f;
    	    	    	PutPx1(&d, c, dstBpp);
    	    	    }
    	    	}
    	    	++s;
    	    }
    	    s += spat;
    	    d += dpat;
    	}
  #endif
    } else {
    	y0  = 0, y1 = h, yd = +1;
    	if (dir & 1) {
    	    y0	= h-1, y1 = -1, yd = -1;
    	    d += (h - 1) * dst_wb;
    	    dpat = dpat - (dst_wb * 2);
    	}
    	/*DBG_F(("@y0=%d,y1=%d,yd=%d %x s:%d,%d,%d d:%d*%d->%d\n", y0,y1,yd,d,src_wb,sn,spat, w,dn,dpat)); */
    	for (y = y0; y != y1; y += yd) {
    	    for (x = 0; x < w; x++) {
    	    	c = GetPx1(&s, srcBpp, clut);
    	    	PutPx1(&d, c, dstBpp);
    	    }
    	    s += spat;
    	    d += dpat;
    	}
    }
    return 1;
}



static inline int getCn(uint8_t const **s0, int bypp)
{
    const uint8_t *s = *s0;
    int c;

    if (bypp == 1) {
    	c = *s++;
    } else if (bypp == 2) {
    	c = PEEKW(s);
    	s += 2;
    } else if (bypp == 3) {
    	c = BBBB(0,s[2],s[1],s[0]);
    	s += 3;
    } else {
    	c = PEEKD(s);
    	s += 4;
    }
    *s0 = s;
    return c;
}


static inline void putCn(uint8_t **d0, int c, int bypp)
{
    uint8_t *d = *d0;

    if (bypp == 1) {
    	*d++ = c;
    } else if (bypp == 2) {
    	POKEW(d, c);
    	d += 2;
    } else if (bypp == 3) {
    	POKEiW(d, c);
    	d[2] = (uint8_t)(c >> 16);
    	d += 3;
    } else {
    	POKED(d, c);
    	d += 4;
    }
    *d0 = d;
}



#if 0

static int tga_encode(uint8_t *dst, const uint8_t *s, int w, int h, int bypp)
{
    int sz = w * h;
    uint8_t  *d = dst;
    uint8_t  *p, *e = s + sz*bypp;
    int    c, l, c2;
    /*DBG_F(("@ %x %x %x\n", s,e, d)); */
    while (s < e) {
    	p = s;
    	c = getCn(&s, bypp);
    	c2 = getCn(&s, bypp);
    	if (s < e && c == c2) {
    	    l = 2;
    	    while (s < e && l < 128) {
    	    	c2 = getCn(&s,bypp);
    	    	if (c != c2) {
    	    	    s -= bypp;
    	    	    break;
    	    	}
    	    	l++;
    	    }
    	    *d++ = 0x80 | (l - 1);
    	    putCn(&d, c, bypp);
    	} else {
    	    s -= bypp;
    	    l = 1;
    	    while (s < e && l < 128) {
    	    	c2 = getCn(&s, bypp);
    	    	if (c == c2) {
    	    	    /*s -= bypp; */
    	    	    break;
    	    	}
    	    	c = c2;
    	    	l++;
    	    }
    	    *d++ = (l - 1);
    	    do {
    	    	c = getCn(&p, bypp);
    	    	putCn(&d, c, bypp);
    	    } while (--l);
    	    s = p;
    	}
    }
    return d - dst;
}

#else

/* bcc55��while,for,switch���܂�inline�֐���inline�W�J�ł��Ȃ� */
#define TGA_ENCODE0(dst, s, w, h, bypp, a_n) do {\
    int sz = w * h;\
    uint8_t  *d = dst;\
    const uint8_t  *p;\
    const uint8_t  *e = s + sz*bypp;\
    int    c, l, c2;\
    while (s < e) {\
    	p = s;\
    	c = getCn(&s, bypp);\
    	c2 = getCn(&s, bypp);\
    	if (s < e && c == c2) {\
    	    l = 2;\
    	    while (s < e && l < 128) {\
    	    	c2 = getCn(&s,bypp);\
    	    	if (c != c2) {\
    	    	    s -= bypp;\
    	    	    break;\
    	    	}\
    	    	l++;\
    	    }\
    	    *d++ = 0x80 | (l - 1);\
    	    putCn(&d, c, bypp);\
    	} else {\
    	    s -= bypp;\
    	    l = 1;\
    	    while (s < e && l < 128) {\
    	    	c2 = getCn(&s, bypp);\
    	    	if (c == c2) {\
    	    	    break;\
    	    	}\
    	    	c = c2;\
    	    	l++;\
    	    }\
    	    *d++ = (l - 1);\
    	    do {\
    	    	c = getCn(&p, bypp);\
    	    	putCn(&d, c, bypp);\
    	    } while (--l);\
    	    s = p;\
    	}\
    }\
    *(a_n) = d - dst;\
} while (0)


/** ���������O�X���k�����f�[�^�̍쐬
 */
static ptrdiff_t tga_encode(uint8_t *dst, const uint8_t *s, int w, int h, int bypp)
{
    ptrdiff_t rc;

    if (bypp <= 1) {
    	TGA_ENCODE0(dst, s, w, h, 1, &rc);
    	return rc;
    } else if (bypp <= 2) {
    	TGA_ENCODE0(dst, s, w, h, 2, &rc);
    	return rc;
    } else if (bypp <= 3) {
    	TGA_ENCODE0(dst, s, w, h, 3, &rc);
    	return rc;
    } else {
    	TGA_ENCODE0(dst, s, w, h, 4, &rc);
    	return rc;
    }
}

#endif



//----------------------------------------------------------------------
// �����⏕
//----------------------------------------------------------------------

/** ���� bpp �̒���
 */
int tga_chkDstBpp(int bpp)
{
    if (bpp <= 8)   	return 8;
  #if 0 //def MY_EX
    else if (bpp <= 12) return 12;  //���{���ɂ͑��݂��Ȃ��B�������[�`���p
  #endif
    else if (bpp <= 16) return 16;
    else if (bpp <= 24) return 24;
    else    	    	return 32;
}


/// �摜�t�@�C���f�[�^�����̂ɕK�v�ȃ������T�C�Y��Ԃ�
int tga_encodeWorkSize(int w, int h, int bpp)
{
    int wb = WID2BYT4(w, bpp);
    return 0x8000 + w * h + wb * h;
}




#ifdef __cplusplus
}
#endif
