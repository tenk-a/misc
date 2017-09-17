#include <stddef.h>
#include "smp.h"

ChFontTable const* ChFontTable_search(unsigned keyCode) {
    int     high = g_chFontTableInfo.chCount;
    int     low  = 0;
    ChFontTable const* tbl = &g_chFontTable[0];
    while (low < high) {
    	int mid = (low + high - 1) / 2;
    	ChFontTable const* midp    = &tbl[mid];
    	unsigned    	   midCode = midp->code;
    	if (keyCode < midCode)
    	    high = mid;
    	else if (keyCode > midCode)
    	    low  = mid + 1;
    	else
    	    return midp;
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
    rU	  = cx * info.fontW + fnd->x;
    //rV  = cy * info.fontH + fnd->y;
    rV	  = cy * info.fontH;
    rW	  = fnd->w;
    //rH  = fnd->h;
    rH	  = info.fontH;
    return true;
}
