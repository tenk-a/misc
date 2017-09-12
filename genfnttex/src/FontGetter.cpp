#include "FontGetter.hpp"
#include <windows.h>



bool FontGetter::get(FontVec& rFonts) {


    // フォントの設定
    LOGFONT     logfont = {0};                  // フォントデータ
    logfont.lfHeight            = (fontW_ * mul_) * (-1);   // フォントの高さ
    logfont.lfWidth             = 0;                        // フォントの幅（平均）
    //logfont.lfWidth           = fontW_ / 2;               // フォントの幅（平均）
    logfont.lfEscapement        = 0;                        // 文字送り方向の角度
    logfont.lfOrientation       = 0;                        // ベースラインの角度
    logfont.lfWeight            = FW_DONTCARE;              // フォントの太さ
    logfont.lfItalic            = FALSE;                    // 斜体にするかどうか
    logfont.lfUnderline         = FALSE;                    // 下線を付けるかどうか
    logfont.lfStrikeOut         = FALSE;                    // 取り消し線を付けるかどうか
    //logfont.lfCharSet         = SHIFTJIS_CHARSET;         // 文字セットの識別子
    logfont.lfCharSet           = DEFAULT_CHARSET;          // 文字セットの識別子
	logfont.lfOutPrecision      = OUT_DEFAULT_PRECIS;       // 出力精度
    logfont.lfClipPrecision     = CLIP_DEFAULT_PRECIS;      // クリッピング精度
    logfont.lfQuality           = ANTIALIASED_QUALITY;      // 出力品質
    logfont.lfPitchAndFamily    = DEFAULT_PITCH;            // ピッチとファミリ
	wchar_t wbuf[0x4000] = {0};
	//mbstowcs(wbuf, ttfname_.c_str(), ttfname_.size());
	::MultiByteToWideChar(CP_OEMCP,0,ttfname_.c_str(), ttfname_.size()+1, wbuf, 0x4000);
    wcsncpy( logfont.lfFaceName, wbuf, 31 );         // フォント名
	logfont.lfFaceName[31] = 0;

    HFONT   new_hfont   = ::CreateFontIndirect(&logfont);
    if (new_hfont == 0) {
        fprintf(stderr, "ERROR: bad font data\n");
        return false;
    }

    HDC     hdc         = ::CreateCompatibleDC(NULL);
    HFONT   old_hfont   = (HFONT)::SelectObject( hdc, new_hfont );  // フォント関連（今のフォント）

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




/** 文字のフォントをデータとして吐き出す
 */
bool FontGetter::getFont(void* hdc0, Font& font) {
    HDC     hdc     = (HDC)hdc0;
    UINT    nChar   = font.ch;
    // 現在選択されているフォントの取得
    TEXTMETRIC  tm = {0};               // 現在のフォント情報
    GetTextMetrics( hdc, &tm );

    static MAT2 const mat2 = {          // 文字描画用行列
        { 0, 1, }, { 0, 0, },
        { 0, 0, }, { 0, 1, }
    };
    GLYPHMETRICS    gm = {0};           // 情報
    DWORD size  = ::GetGlyphOutline(
        hdc,                // デバイスコンテキスト
        nChar,              // 処理したい文字の整数値（１文字の）
        GGO_GRAY8_BITMAP,   // 取得するデータのフォーマット
        &gm,                // GLYPHMETRICS構造体へのアドレス
        0,                  // 取得するバッファのサイズ
        NULL,               // 取得するバッファ（領域作成済み）
        &mat2 );            // 文字への行列データ
    // バッファを確保
    wkBuf_.clear();
    wkBuf_.resize(size);
	if (!size)
		return false;
    int rc = ::GetGlyphOutline( hdc, nChar, GGO_GRAY8_BITMAP, &gm, size, (LPVOID)&wkBuf_[0], &mat2 );
    if(rc <= 0) {
        // バッファを開放
        return false;
    }

    // 現在選択されているフォントの取得
    rc = GetTextMetrics( hdc, &tm );

    // ピッチ
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
                // 色の取得
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
  ENUMLOGFONTEXW *lpelfe,    // 論理的なフォントデータ
  NEWTEXTMETRICEXW *lpntme,  // 物理的なフォントデータ
  DWORD FontType,           // フォントの種類
  LPARAM lParam             // アプリケーション定義のデータ
){
	char buf[0x1000];
	WideCharToMultiByte(0,0,lpelfe->elfLogFont.lfFaceName, 32, buf, 0x1000, 0, 0);
	++count;
	printf("%s\n", buf);
	return 1;
}

void FontGetter::printFontInfo() {
    // フォントの設定
    LOGFONT     logfont = {0};                  // フォントデータ
    logfont.lfCharSet   = DEFAULT_CHARSET;         // 文字セットの識別子
    HDC     hdc         = ::CreateCompatibleDC(NULL);


	int rc = EnumFontFamiliesExW(
	  hdc,					// デバイスコンテキストのハンドル
	  &logfont,				// フォント情報
	  (FONTENUMPROCW)enumFontFamExProc,	// コールバック関数
	  NULL,					// 追加データ
	  0						// 未使用；必ず 0 を指定
	);
}
