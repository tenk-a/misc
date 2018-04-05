/**
 *  @file   FontGetter.cpp
 *  @brief  get font images
 *  @author Masashi KITAMURA
 *  @date   2017-09
 */
#include "FontGetter.hpp"
#include <stdio.h>
#include <string.h>
#include <map>
#include <windows.h>
#include <assert.h>
#include "TexChFontInfo.h"



FontGetter::FontGetter(char const* ttfname, unsigned fontW, unsigned cellW, unsigned cellH
						, unsigned mul, unsigned bpp, unsigned weight, bool italic)
    : ttfname_(strdup(ttfname ? ttfname : ""))
    , fontW_(fontW)
    , cellW_(cellW)
    , cellH_(cellH)
    , mul_(mul)
    , bpp_(bpp)
    , tone_(1 << bpp)
    , weight_(weight)	// 0..9
    , italic_(italic)
{
    assert(1 <= bpp && bpp <= 8);
    if (mul_ == 0)
    	mul_ = 1;
}

FontGetter::~FontGetter() {
    free(ttfname_);
}


/** get fonts
 */
bool FontGetter::get(FontVec& rFonts) {
    LOGFONTW	logfont = {0};	    	    	    	    // �t�H���g�f�[�^
    logfont.lfHeight	    	= (fontW_ * mul_) * (-1);   // �t�H���g�̍���
    logfont.lfWidth 	    	= 0;	    	    	    // �t�H���g�̕��i���ρj
    logfont.lfEscapement    	= 0;	    	    	    // ������������̊p�x
    logfont.lfOrientation   	= 0;	    	    	    // �x�[�X���C���̊p�x
    logfont.lfWeight	    	= (weight_) ? weight_*100 : FW_DONTCARE;	// �t�H���g�̑���
    logfont.lfItalic	    	= (italic_) ? TRUE : FALSE; // �Α̂ɂ��邩�ǂ���
    logfont.lfUnderline     	= FALSE;    	    	    // ������t���邩�ǂ���
    logfont.lfStrikeOut     	= FALSE;    	    	    // ����������t���邩�ǂ���
    //logfont.lfCharSet     	= SHIFTJIS_CHARSET; 	    // �����Z�b�g�̎��ʎq
    logfont.lfCharSet	    	= DEFAULT_CHARSET;  	    // �����Z�b�g�̎��ʎq
    logfont.lfOutPrecision  	= OUT_DEFAULT_PRECIS;	    // �o�͐��x
    logfont.lfClipPrecision 	= CLIP_DEFAULT_PRECIS;	    // �N���b�s���O���x
    logfont.lfQuality	    	= ANTIALIASED_QUALITY;	    // �o�͕i��
    logfont.lfPitchAndFamily	= DEFAULT_PITCH;    	    // �s�b�`�ƃt�@�~��
    wchar_t wbuf[0x4000] = {0};
    //mbstowcs(wbuf, ttfname_, strlen(ttfname_));
    ::MultiByteToWideChar(CP_OEMCP,0,ttfname_, int(strlen(ttfname_))+1, wbuf, 0x4000);
    wcsncpy( logfont.lfFaceName, wbuf, 31 );	     // �t�H���g��
    logfont.lfFaceName[31] = 0;

    HFONT   new_hfont	= ::CreateFontIndirectW(&logfont);
    if (new_hfont == 0) {
    	fprintf(stderr, "ERROR: bad font data\n");
    	return false;
    }

    HDC     hdc     	= ::CreateCompatibleDC(NULL);
    HFONT   old_hfont	= (HFONT)::SelectObject( hdc, new_hfont );

    for (unsigned no = 0; no < rFonts.size(); ++no) {
    	rFonts[no].data.resize(cellW_ * cellH_);
    	getFont(hdc, rFonts[no]);
    	adjustFontSize(rFonts[no]);
    }

    ::SelectObject( hdc, old_hfont );
    ::DeleteObject(new_hfont);
    ::DeleteObject(old_hfont);
    ::DeleteDC(hdc);

    return true;
}


/** get font data
 */
bool FontGetter::getFont(void* hdc0, Font& font) {
    HDC     	hdc 	= (HDC)hdc0;
    UINT    	nChar	= font.ch;
    TEXTMETRIC	tm  	= {0};
    ::GetTextMetrics( hdc, &tm );
    static MAT2 const mat2 = {
    	{ 0, 1, }, { 0, 0, },
    	{ 0, 0, }, { 0, 1, }
    };
    GLYPHMETRICS    gm = {0};
    DWORD size	= ::GetGlyphOutline(
    	hdc,	    	    // �f�o�C�X�R���e�L�X�g
    	nChar,	    	    // ���������������̐����l�i�P�����́j
    	GGO_GRAY8_BITMAP,   // �擾����f�[�^�̃t�H�[�}�b�g
    	&gm,	    	    // GLYPHMETRICS�\���̂ւ̃A�h���X
    	0,  	    	    // �擾����o�b�t�@�̃T�C�Y
    	NULL,	    	    // �擾����o�b�t�@�i�̈�쐬�ς݁j
    	&mat2 );    	    // �����ւ̍s��f�[�^
    wkBuf_.clear();
    wkBuf_.resize(size);
    if (!size)
    	return false;
    int rc = ::GetGlyphOutline( hdc, nChar, GGO_GRAY8_BITMAP, &gm, size, (LPVOID)&wkBuf_[0], &mat2 );
    if(rc <= 0) {
    	return false;
    }

    rc = ::GetTextMetrics( hdc, &tm );

    int pitch	    = (gm.gmBlackBoxX + 3) & ~3;

    int dw  	    = gm.gmBlackBoxX;
    int dh  	    = gm.gmBlackBoxY;
    int offset_x    = gm.gmptGlyphOrigin.x;
    int offset_y    = tm.tmAscent - gm.gmptGlyphOrigin.y;
    if (tm.tmInternalLeading != 0) {
    	offset_y    = offset_y - tm.tmDescent;
    }
    int ox = 0;
    int oy = 0;

    //if (offset_y < 0) {
    //	offset_y     = 0;
    //}

    dw	     = (dw+(mul_-1)) / mul_;
    dh	     = (dh+(mul_-1)) / mul_;
    offset_x = (offset_x) / mul_;
    offset_y = (offset_y) / mul_;

    if (offset_x + dw > int(cellW_)) {
		ox       = offset_x + dw - cellW_;
    	offset_x = int(cellW_) - dw;
		if (offset_x < 0)
			offset_x = 0;
    } else if (offset_x < 0) {
		ox = offset_x;
    	offset_x = 0;
    }
    if (offset_y + dh > int(cellH_)) {
		oy       = offset_y + dh - cellH_;
    	offset_y = int(cellH_) - dh;
		if (offset_y < 0)
			offset_y = 0;
    } else if (offset_y < 0) {
		oy = offset_y;
    	offset_y = 0;
    }

	font.ox = ox;
	font.oy = oy;

	unsigned fontH = fontW_;
	unsigned fontW = fontW_;
    if (mul_ == 1) {
		if (fontW < pitch) {
			fontW = pitch;
			if (fontW > cellW_)
				fontW = cellW_;
		}
    	for ( unsigned j = 0 ; j < unsigned(dh) && j < cellH_ && j < fontH; ++j ) {
    	    for ( unsigned i = 0 ; i < unsigned(dw) && i < cellW_ && i < fontW; ++i ) {
    	    	unsigned alp  = wkBuf_[j * pitch + i];
    	    	alp   = (alp * (tone_-1) ) / 64;
    	    	font.data[((j+offset_y) * cellW_) + (i + offset_x)]   = alp;
    	    }
    	}
    }else {
		if (fontW < pitch/mul_) {
			fontW = pitch/mul_;
			if (fontW > cellW_)
				fontW = cellW_;
		}
    	for ( unsigned j = 0 ; j < unsigned(dh) && j < cellH_ && j < fontH; ++j ) {
    	    for ( unsigned i = 0 ; i < unsigned(dw) && i < cellW_ && i < fontW; ++i ) {
    	    	unsigned total = 0;
    	    	for(unsigned y = 0 ; y < mul_ && y+(j*mul_) < gm.gmBlackBoxY ; ++y) {
    	    	    for(unsigned x = 0 ; x < mul_ && x+(i*mul_) < gm.gmBlackBoxX ; ++x) {
    	    	    	uint8_t alp = wkBuf_[ (y + j * mul_) * pitch + (x + i * mul_) ];
    	    	    	total  += alp;
    	    	    }
    	    	}
    	    	font.data[(j + offset_y) * cellW_ +  (i + offset_x)]  = (total * (tone_-1)) / (mul_ * mul_ * 64);
    	    }
    	}
    }
    return true;
}


/** calc font size
 */
bool FontGetter::adjustFontSize(Font& rFont) {
    unsigned x0 = cellW_;
    unsigned y0 = cellH_;
    unsigned x1 = 0;
    unsigned y1 = 0;
    unsigned w	= cellW_;
    unsigned h	= cellH_;
    for (unsigned y = 0; y < h; ++y) {
    	for (unsigned x = 0; x < w; ++x) {
    	    unsigned c = rFont.data[y * w + x];
    	    if (c) {
    	    	if (x0 > x) x0 = x;
    	    	if (y0 > y) y0 = y;
    	    	if (x1 < x) x1 = x;
    	    	if (y1 < y) y1 = y;
    	    }
    	}
    }
    if (x0 == cellW_ && y0 == cellH_ && x1 == 0 && y1 == 0) {
    	rFont.x = 0;
    	rFont.y = 0;
    	rFont.w = cellW_;
    	rFont.h = cellH_;
    	return	false;
    }
    w = 1+x1-x0;
    h = 1+y1-y0;
    rFont.x = x0;
    rFont.y = y0;
    rFont.w = w;
    rFont.h = h;
    return true;
}


#if 1
struct Str {
    enum { SIZE = 260 };
    Str() { memset(str_, 0, SIZE); }
    Str(char const* str) { strncpy(str_,str,SIZE); str_[SIZE-1] = 0; }
    Str(Str const& r) { memcpy(str_, r.str_, SIZE); }
    bool operator<(Str const& r) const { return strcmp(str_, r.str_) < 0; }
    char const* c_str() const { return str_; }
private:
    char    str_[SIZE];
};
typedef std::map<Str, unsigned> FontNames;
#else
#include <string>
typedef std::map<std::string, unsigned> FontNames;
#endif



static int CALLBACK enumFontFamExProc(
  ENUMLOGFONTEXW*   lpelfe, 	// �_���I�ȃt�H���g�f�[�^
  NEWTEXTMETRICEXW* lpntme, 	// �����I�ȃt�H���g�f�[�^
  unsigned/*DWORD*/ FontType,	// �t�H���g�̎��
  LPARAM    	    lParam  	// �A�v���P�[�V������`�̃f�[�^
){
    char buf[0x1000];
    WideCharToMultiByte(0,0,lpelfe->elfLogFont.lfFaceName, 32, buf, 0x1000, 0, 0);
    FontNames* pFontNames = (FontNames*)lParam;
    (*pFontNames)[buf] = 1;
    return 1;
}


/** print font list
 */
void FontGetter::printFontInfo() {
    LOGFONTW	logfont = {0};
    logfont.lfCharSet	= DEFAULT_CHARSET;
    HDC			hdc     = ::CreateCompatibleDC(NULL);

    FontNames	fntNames;
    EnumFontFamiliesExW(
      hdc,  	    	    	    	// �f�o�C�X�R���e�L�X�g�̃n���h��
      &logfont,     	    	    	// �t�H���g���
      (FONTENUMPROCW)enumFontFamExProc, // �R�[���o�b�N�֐�
      (LPARAM)&fntNames,    	    	// �ǉ��f�[�^
      0     	    	    	    	// ���g�p�G�K�� 0 ���w��
    );

    for (FontNames::iterator ite = fntNames.begin(); ite != fntNames.end(); ++ite) {
    	printf("%s\n", ite->first.c_str());
    }
}



void Font::getPara(TexChFontInfo& dst) const
{
	dst.x	= this->x;
	dst.y	= this->y;
	dst.w	= this->w;
	dst.h	= this->h;
	dst.ox	= this->ox;
	dst.oy	= this->oy;
}

void Font::getPara(TexChFontInfo0& dst) const
{
	dst.x	= this->x;
	dst.y	= this->y;
	dst.w	= this->w;
	dst.h	= this->h;
}
