#include "FontGetter.hpp"
#include <windows.h>



bool FontGetter::get(FontVec& rFonts) {


    // �t�H���g�̐ݒ�
    LOGFONT     logfont = {0};                  // �t�H���g�f�[�^
    logfont.lfHeight            = (fontW_ * mul_) * (-1);   // �t�H���g�̍���
    logfont.lfWidth             = 0;                        // �t�H���g�̕��i���ρj
    //logfont.lfWidth           = fontW_ / 2;               // �t�H���g�̕��i���ρj
    logfont.lfEscapement        = 0;                        // ������������̊p�x
    logfont.lfOrientation       = 0;                        // �x�[�X���C���̊p�x
    logfont.lfWeight            = FW_DONTCARE;              // �t�H���g�̑���
    logfont.lfItalic            = FALSE;                    // �Α̂ɂ��邩�ǂ���
    logfont.lfUnderline         = FALSE;                    // ������t���邩�ǂ���
    logfont.lfStrikeOut         = FALSE;                    // ����������t���邩�ǂ���
    //logfont.lfCharSet         = SHIFTJIS_CHARSET;         // �����Z�b�g�̎��ʎq
    logfont.lfCharSet           = DEFAULT_CHARSET;          // �����Z�b�g�̎��ʎq
	logfont.lfOutPrecision      = OUT_DEFAULT_PRECIS;       // �o�͐��x
    logfont.lfClipPrecision     = CLIP_DEFAULT_PRECIS;      // �N���b�s���O���x
    logfont.lfQuality           = ANTIALIASED_QUALITY;      // �o�͕i��
    logfont.lfPitchAndFamily    = DEFAULT_PITCH;            // �s�b�`�ƃt�@�~��
	wchar_t wbuf[0x4000] = {0};
	//mbstowcs(wbuf, ttfname_.c_str(), ttfname_.size());
	::MultiByteToWideChar(CP_OEMCP,0,ttfname_.c_str(), ttfname_.size()+1, wbuf, 0x4000);
    wcsncpy( logfont.lfFaceName, wbuf, 31 );         // �t�H���g��
	logfont.lfFaceName[31] = 0;

    HFONT   new_hfont   = ::CreateFontIndirect(&logfont);
    if (new_hfont == 0) {
        fprintf(stderr, "ERROR: bad font data\n");
        return false;
    }

    HDC     hdc         = ::CreateCompatibleDC(NULL);
    HFONT   old_hfont   = (HFONT)::SelectObject( hdc, new_hfont );  // �t�H���g�֘A�i���̃t�H���g�j

    for (unsigned no = 0; no < rFonts.size(); ++no) {
        rFonts[no].data.resize(cellW_ * cellW_);
        getFont(hdc, rFonts[no]);
        adjustFontSize(rFonts[no]);
    }

    ::SelectObject( hdc, old_hfont );
    ::DeleteObject(new_hfont);
    ::DeleteObject(old_hfont);
    ::DeleteDC(hdc);

    return true;
}




/** �����̃t�H���g���f�[�^�Ƃ��ēf���o��
 */
bool FontGetter::getFont(void* hdc0, Font& font) {
    HDC     hdc     = (HDC)hdc0;
    UINT    nChar   = font.ch;
    // ���ݑI������Ă���t�H���g�̎擾
    TEXTMETRIC  tm = {0};               // ���݂̃t�H���g���
    GetTextMetrics( hdc, &tm );

    static MAT2 const mat2 = {          // �����`��p�s��
        { 0, 1, }, { 0, 0, },
        { 0, 0, }, { 0, 1, }
    };
    GLYPHMETRICS    gm = {0};           // ���
    DWORD size  = ::GetGlyphOutline(
        hdc,                // �f�o�C�X�R���e�L�X�g
        nChar,              // ���������������̐����l�i�P�����́j
        GGO_GRAY8_BITMAP,   // �擾����f�[�^�̃t�H�[�}�b�g
        &gm,                // GLYPHMETRICS�\���̂ւ̃A�h���X
        0,                  // �擾����o�b�t�@�̃T�C�Y
        NULL,               // �擾����o�b�t�@�i�̈�쐬�ς݁j
        &mat2 );            // �����ւ̍s��f�[�^
    // �o�b�t�@���m��
    wkBuf_.clear();
    wkBuf_.resize(size);
	if (!size)
		return false;
    int rc = ::GetGlyphOutline( hdc, nChar, GGO_GRAY8_BITMAP, &gm, size, (LPVOID)&wkBuf_[0], &mat2 );
    if(rc <= 0) {
        // �o�b�t�@���J��
        return false;
    }

    // ���ݑI������Ă���t�H���g�̎擾
    rc = GetTextMetrics( hdc, &tm );

    // �s�b�`
    int pitch       = (gm.gmBlackBoxX + 3) & ~3;

    int dw          = gm.gmBlackBoxX;
    int dh          = gm.gmBlackBoxY;
    int ofset_x     = gm.gmptGlyphOrigin.x;
    int ofset_y     = tm.tmAscent - gm.gmptGlyphOrigin.y;
    if (tm.tmInternalLeading != 0) {
        ofset_y     = ofset_y - tm.tmDescent;
    }
    //ofset_y       = font_H_ - gm.gmptGlyphOrigin.y;
    //ofset_y       = tm.tmAscent - gm.gmptGlyphOrigin.y;
    if (ofset_y < 0) {
        ofset_y     = 0;
    }

    dw      = (dw+(mul_-1)) / mul_;
    dh      = (dh+(mul_-1)) / mul_;
    ofset_x = (ofset_x) / mul_;
    ofset_y = (ofset_y) / mul_;

    if (mul_ == 1) {
        for ( int j = 0 ; j < dh && j < cellW_; ++j ) {
            for ( int i = 0 ; i < dw && i < cellW_; ++i ) {
                // �F�̎擾
                unsigned alpha  = wkBuf_[j * pitch + i];
                alpha   = (alpha * 15 ) / 64;
                font.data[((j+ofset_y) * cellW_) + (i + ofset_x)]   = alpha;
            }
        }
    }else {
        for ( int j = 0 ; j < dh && j < cellW_; ++j ) {
            for ( int i = 0 ; i < dw && i < cellW_; ++i ) {
                unsigned alpha2     = 0;
                for(int y = 0 ; y < mul_ && y+(j*mul_) < gm.gmBlackBoxY ; ++y) {
                    for(int x = 0 ; x < mul_ && x+(i*mul_) < gm.gmBlackBoxX ; ++x) {
                        uint8_t alpha = wkBuf_[ (y + j * mul_) * pitch + (x + i * mul_) ];
                        alpha2  += alpha;
                    }
                }
                //printf("%x",(alpha2 * 15) / (mul_ * mul_ * 64));
                font.data[(j + ofset_y) * cellW_ +  (i + ofset_x)]  = (alpha2 * 15) / (mul_ * mul_ * 64);
            }
            //printf("\n");
        }
    }
    return true;
}


bool FontGetter::adjustFontSize(Font& rFont) {
    unsigned x0 = cellW_;
    unsigned y0 = cellW_;
    unsigned x1 = 0;
    unsigned y1 = 0;
    unsigned w  = cellW_;
    unsigned h  = cellW_;
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
    if (x0 == cellW_ && y0 == cellW_ && x1 == 0 && y1 == 0) {
        rFont.x = 0;
        rFont.y = 0;
        rFont.w = cellW_;
        rFont.h = cellW_;
        return  false;
    }
    w = 1+x1-x0;
    h = 1+y1-y0;
    rFont.x = x0;
    rFont.y = y0;
    rFont.w = w;
    rFont.h = h;
    return true;
}

//static int fontEnumProcW(CONST LOGFONTW *logfont, CONST VOID *, DWORD, LPARAM)
//static int fontEnumProcW(CONST LOGFONTW *logfont, CONST TEXTMETRICW *, DWORD, LPARAM) 
static int count = 0;
static int CALLBACK enumFontFamExProc(
  ENUMLOGFONTEXW *lpelfe,    // �_���I�ȃt�H���g�f�[�^
  NEWTEXTMETRICEXW *lpntme,  // �����I�ȃt�H���g�f�[�^
  DWORD FontType,           // �t�H���g�̎��
  LPARAM lParam             // �A�v���P�[�V������`�̃f�[�^
){
	char buf[0x1000];
	WideCharToMultiByte(0,0,lpelfe->elfLogFont.lfFaceName, 32, buf, 0x1000, 0, 0);
	++count;
	printf("%s\n", buf);
	return 1;
}

void FontGetter::printFontInfo() {
    // �t�H���g�̐ݒ�
    LOGFONT     logfont = {0};                  // �t�H���g�f�[�^
    logfont.lfCharSet   = DEFAULT_CHARSET;         // �����Z�b�g�̎��ʎq
    HDC     hdc         = ::CreateCompatibleDC(NULL);


	int rc = EnumFontFamiliesExW(
	  hdc,					// �f�o�C�X�R���e�L�X�g�̃n���h��
	  &logfont,				// �t�H���g���
	  (FONTENUMPROCW)enumFontFamExProc,	// �R�[���o�b�N�֐�
	  NULL,					// �ǉ��f�[�^
	  0						// ���g�p�G�K�� 0 ���w��
	);
}
