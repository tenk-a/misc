#include "FontGetter.hpp"
#include <windows.h>
#include <map>


bool FontGetter::get(FontVec& rFonts) {
    LOGFONT     logfont = {0};                              // フォントデータ
    logfont.lfHeight            = (fontW_ * mul_) * (-1);   // フォントの高さ
    logfont.lfWidth             = 0;                        // フォントの幅（平均）
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
    HFONT   old_hfont   = (HFONT)::SelectObject( hdc, new_hfont );

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
    HDC         hdc     = (HDC)hdc0;
    UINT        nChar   = font.ch;
    TEXTMETRIC  tm      = {0};
    GetTextMetrics( hdc, &tm );
    static MAT2 const mat2 = {
        { 0, 1, }, { 0, 0, },
        { 0, 0, }, { 0, 1, }
    };
    GLYPHMETRICS    gm = {0};
    DWORD size  = ::GetGlyphOutline(
        hdc,                // デバイスコンテキスト
        nChar,              // 処理したい文字の整数値（１文字の）
        GGO_GRAY8_BITMAP,   // 取得するデータのフォーマット
        &gm,                // GLYPHMETRICS構造体へのアドレス
        0,                  // 取得するバッファのサイズ
        NULL,               // 取得するバッファ（領域作成済み）
        &mat2 );            // 文字への行列データ
    wkBuf_.clear();
    wkBuf_.resize(size);
    if (!size)
        return false;
    int rc = ::GetGlyphOutline( hdc, nChar, GGO_GRAY8_BITMAP, &gm, size, (LPVOID)&wkBuf_[0], &mat2 );
    if(rc <= 0) {
        return false;
    }

    rc = GetTextMetrics( hdc, &tm );

    int pitch       = (gm.gmBlackBoxX + 3) & ~3;

    int dw          = gm.gmBlackBoxX;
    int dh          = gm.gmBlackBoxY;
    int offset_x    = gm.gmptGlyphOrigin.x;
    int offset_y    = tm.tmAscent - gm.gmptGlyphOrigin.y;
    if (tm.tmInternalLeading != 0) {
        offset_y    = offset_y - tm.tmDescent;
    }
    if (offset_y < 0) {
        offset_y     = 0;
    }

    dw       = (dw+(mul_-1)) / mul_;
    dh       = (dh+(mul_-1)) / mul_;
    offset_x = (offset_x) / mul_;
    offset_y = (offset_y) / mul_;
    if (offset_x < 0)
        offset_x = 0;

    if (mul_ == 1) {
        for ( int j = 0 ; j < dh && j < cellW_ && j < fontW_; ++j ) {
            for ( int i = 0 ; i < dw && i < cellW_ && i < fontW_; ++i ) {
                // 色の取得
                unsigned alp  = wkBuf_[j * pitch + i];
                alp   = (alp * 15 ) / 64;
                font.data[((j+offset_y) * cellW_) + (i + offset_x)]   = alp;
            }
        }
    }else {
        for ( int j = 0 ; j < dh && j < cellW_ && j < fontW_; ++j ) {
            for ( int i = 0 ; i < dw && i < cellW_ && i < fontW_; ++i ) {
                unsigned total = 0;
                for(int y = 0 ; y < mul_ && y+(j*mul_) < gm.gmBlackBoxY ; ++y) {
                    for(int x = 0 ; x < mul_ && x+(i*mul_) < gm.gmBlackBoxX ; ++x) {
                        uint8_t alp = wkBuf_[ (y + j * mul_) * pitch + (x + i * mul_) ];
                        total  += alp;
                    }
                }
                font.data[(j + offset_y) * cellW_ +  (i + offset_x)]  = (total * 15) / (mul_ * mul_ * 64);
            }
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

typedef std::map<std::string, unsigned> FontNames;

static int count = 0;
static int CALLBACK enumFontFamExProc(
  ENUMLOGFONTEXW *lpelfe,   // 論理的なフォントデータ
  NEWTEXTMETRICEXW *lpntme, // 物理的なフォントデータ
  DWORD FontType,           // フォントの種類
  LPARAM lParam             // アプリケーション定義のデータ
){
    ++count;
    char buf[0x1000];
    WideCharToMultiByte(0,0,lpelfe->elfLogFont.lfFaceName, 32, buf, 0x1000, 0, 0);
    FontNames* pFontNames = (FontNames*)lParam;
    (*pFontNames)[buf] = 1;
    return 1;
}

void FontGetter::printFontInfo() {
    LOGFONT     logfont = {0};
    logfont.lfCharSet   = DEFAULT_CHARSET;
    HDC     hdc         = ::CreateCompatibleDC(NULL);

    FontNames   fntNames;
    int rc = EnumFontFamiliesExW(
      hdc,                              // デバイスコンテキストのハンドル
      &logfont,                         // フォント情報
      (FONTENUMPROCW)enumFontFamExProc, // コールバック関数
      (LPARAM)&fntNames,                // 追加データ
      0                                 // 未使用；必ず 0 を指定
    );

    for (FontNames::iterator ite = fntNames.begin(); ite != fntNames.end(); ++ite) {
        printf("%s\n", ite->first.c_str());
    }
}
