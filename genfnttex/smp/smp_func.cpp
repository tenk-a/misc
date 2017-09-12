#include <stddef.h>
#include "smp.h"

ChFontTable const* ChFontTable_search(unsigned key) {
    int     high = g_chFontTableInfo.chCount;
    int     low  = 0;
    ChFontTable const* tbl = &g_chFontTable[0];
    while (low < high) {
        int mid = (low + high - 1) / 2;
        ChFontTable const* tgt = &tbl[mid];
        unsigned           tgtCode = tgt->code;
        if (key < tbl->code)
            high = mid;
        else if (key > tbl->code)
            low  = mid + 1;
        else
            return tgt;
    }
    return NULL;
}


bool ChFontTable_getPageUVWH(unsigned ch, unsigned& rPage, unsigned& rU, unsigned& rV, unsigned& rW, unsigned& rH) {
    ChFontTable const* fnd = ChFontTable_search(ch);
    if (!fnd) {
        return false;
    }
    ChFontTableInfo const& info =  g_chFontTableInfo;
    unsigned pageChSize = info.texChW * info.texChH;
    unsigned idx  = fnd->index;
    unsigned page = idx / pageChSize;
    unsigned ofs  = idx % pageChSize;
    unsigned cx   = ofs % info.texChW;
    unsigned cy   = ofs / info.texChW;

    rPage = page;
    rU    = cx * info.fontW + fnd->x;
    //rV  = cy * info.fontH + fnd->y;
    rV    = cy * info.fontH + fnd->y;
    rW    = fnd->w;
    //rH  = fnd->h;
    rH    = info.fontH;
    return true;
}
