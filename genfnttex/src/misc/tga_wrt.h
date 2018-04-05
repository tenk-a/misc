/**
 *  @file   tga_wrt.h
 *  @brief  tga画像出力.
 *  @author Masashi Kitamura
 */
#ifndef TGA_WRT_H
#define TGA_WRT_H

#ifdef __cplusplus
extern "C" {
#endif

/// tga画像イメージの生成.
int  tga_write( void *tga_data,     	    	    	    // tgaイメージを格納するバッファ.
    	    	int w, int h, int bpp,	    	    	    // 出力の横幅,縦幅, bpp
    	    	const void *src, int srcWb, int srcBpp,     // 入力のピクセル,横幅バイト数,bpp
    	    	const void *clut, int dir); 	    	    // 入力の clut, 入力ピクセルのラインの上下.

/// tga画像イメージの生成.
int  tga_writeEx(void *tga_data, int dataSiz,	    	    // tgaイメージを格納するバッファ, とそのサイズ,
    	    	int w, int h, int dstBpp,   	    	    // 出力の横幅,縦幅, bpp
    	    	const void* src, int srcWb, int srcBpp,     // 入力のピクセル,横幅バイト数,縦幅,bpp
    	    	const void *clut0, int clutBpp,     	    // 入力の clut と、出力のclutのbpp
    	    	int dir,    	    	    	    	    // 入力のピクセル 0:上ラインから 1:下ラインから.
    	    	int x0, int y0);    	    	    	    // 出力のヘッダに入れる始点 x,y. 普通0,0.


/// 指定bppを、それを満たす実際に tga でサポートされる bpp に変換.
int  tga_chkDstBpp(int bpp);

/// w,h,bppからtgaイメージに必要なバイト数を返す(大目に返す).
int  tga_encodeWorkSize(int w, int h, int bpp);



#ifdef __cplusplus
}
#endif




// ===========================================================================
// (上記関数の使用例)

// inline が指定できる場合.
#if (defined __cplusplus) || (defined inline) || (__STDC_VERSION__ >= 199901L) || (defined __GNUC__)

// 予め stdlib.h をincludeしているときのみ利用可能.
#if (defined _INC_STDLIB/*VC,BCC*/) || (defined __STDLIB_H/*DMC,BCC*/) || (defined _STDLIB_H_/*GCC*/) \
    || (defined _STDLIB_H_INCLUDED/*watcom*/) || (defined _MSL_STDLIB_H/*CW*/)
#include <stdlib.h> 	// calloc,freeのため.

/** 指定した画像を、mallocしたメモリにtga画像にして返す. 失敗するとNULL.
 *  ※ srcWidBytは元画像の横幅バイト数で、0 だとwとbppからジャストの値を求める.
 */
static inline void* tga_writeMalloc(
    	int w, int h, int dstBpp,   	    	    // 出力の横幅,縦幅, bpp
    	const void* src, int srcWidByt, int srcBpp, // 入力のピクセル,横幅バイト数,縦幅,bpp
    	const void *clut,   	    	    	    // 入力の clut と
    	int dir,    	    	    	    	    // ピクセルは 0:上ラインから 1:下ラインから.
    	unsigned* pSize)    	    	    	    // 生成したサイズ
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


// 予め stdio.h をincludeしているときのみ利用可能.
#if (defined _INC_STDIO/*VC,BCC*/) || (defined __STDIO_H/*DMC,BCC*/) || (defined _STDIO_H_/*GCC*/) \
    || (defined _STDIO_H_INCLUDED/*watcom*/) || (defined _MSL_STDIO_H/*CW*/)
#include <stdio.h>

#if defined __cplusplus
static int tga_write_file(const char *fname, int w, int h, int dstBpp,
    	 const void* src, int srcWidByt=0, int srcBpp=0, const void* clut=0, int dir=0);
#endif

/** tgaファイル生成.
 *  ※ srcWidBytは元画像の横幅バイト数で、0 だとwとbppからジャストの値を求める.
 */
static inline int tga_write_file(
    const char* fname,	    	    	    	// 出力ファイル名
    int w, int h, int dstBpp,	    	    	// 出力の横幅,縦幅, bpp
    const void* src, int srcWidByt, int srcBpp, // 入力のピクセル,横幅バイト数,縦幅,bpp
    const void *clut,	    	    	    	// 入力の clut
    int dir)	    	    	    	    	// ピクセルは 0:上ラインから 1:下ラインから.
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

#endif	// inlineが使える場合.



#endif	// TGA_WRT_H
