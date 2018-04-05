/**
 *  @file   genfnttex.cpp
 *  @brief  generate font texture(.tga) for Win32
 *  @author Masashi KITAMURA (tenka@6809.net)
 *  @date   2017-09-09
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <map>
#include "misc/mbc.h"
#include "misc/fks_fname.h"
#include "misc/tga_wrt.h"
#include "FontGetter.hpp"
#include "TexChFontInfo.h"

class App {
    typedef 	std::map<uint32_t, uint32_t>	    Cmap;
    typedef 	std::vector< std::vector<uint8_t> > TexBuf;

public:
    App()
    	: appname_(NULL)
    	, ttfname_("MS UI Gothic")
    	, oname_(NULL)
    	, tblname_(NULL)
    	, texW_(2048)
    	, texH_(2048)
    	, fontW_(0)
    	, cellW_(0)
    	, cellH_(0)
    	, texChW_(0)
    	, texChH_(0)
    	, mul_(1)
    	, bpp_(4)
    	, addascii_(false)
    	, addspc_(false)
    	, addcr_(false)
    	, oldTable_(false)
    	, weight_(0)
    	, italic_(false)
    {
    	makeClut();
    	//mbs_setEnv("ja_JP.UTF-8");
    }

    int main(int argc, char* argv[]) {

    	appname_ = strrchr(argv[0], '\\');
    	if (appname_)
    	    ++appname_;
    	else
    	    appname_ = argv[0];

    	if (argc < 2)
    	    return usage();
    	int fno = 0;
    	for (int i = 1; i < argc; ++i) {
    	    char* p = argv[i];
    	    if (*p == '-') {
    	    	if (scanOpts(p) == false)
    	    	    return 1;
    	    } else {
    	    	++fno;
    	    }
    	}
    	if (fno == 0)
    	    return 0;

    	for (int i = 1; i < argc; ++i) {
    	    char* p = argv[i];
    	    if (*p != '-') {
    	    	if (!oname_) {
    	    	    oname_ = getOutName(p);
    	    	}
    	    	if (text2chr(p) == false)
    	    	    return 1;
    	    }
    	}
    	if (oname_ == NULL)
    	    return 0;
    	if (optsSetting() == false)
    	    return 1;
    	if (getFonts() == false)
    	    return 1;
    	if (makeTex() == false)
    	    return 1;
    	if (saveTga() == false)
    	    return 1;

    	//if (addcr_) {
   	    //	cmap_[0x0a] = 0x0a;
   	    //	cmap_[0x0d] = 0x0d;
		//}

    	if (oldTable_) {
    	    if (saveOldChFontTableHeader() == false)
    	    	return 1;
    	    if (saveCTable() == false)
    	    	return 1;
    	} else {
    	    if (saveTexChFontInfoHeader() == false)
    	    	return 1;
    	    if (saveTexChFontInfoExtern() == false)
    	    	return 1;
    	    if (saveCTable() == false)
    	    	return 1;
    	    if (saveBinFile() == false)
    	    	return 1;
    	}
    	return 0;
    }

private:
    enum { TEX_FONT_INFO_ID = ('T') | ('F' << 8) | ('I' << 16) | ('\0' << 24) };    // little endian

    int usage() {
    	fprintf(stderr, "usage>%s [-opts] file(s)\n", appname_);
    	fprintf(stderr,
    	   "       https://github.com/tenk-a/genfnttex\n"
    	   " -ttf=[TTFNAME]  ttf font name\n"
    	   " -o=[OUTPUT]     output base name\n"
    	   " -tblname=[NAME] c table var name\n"
    	   " -ts[W:H]        texture size W*H (2^N)\n"
    	   " -fs[N]          input-font size (N pixels)\n"
    	   " -cs[W:H]        tex-cell(char)/font size (W*H pixels)\n"
    	   " -mul[N]         input-font-size*N(/N)\n"
    	   " -bpp[N]         bit per pixel. N=1..8\n"
    	   " -addascii       generate 0x21..0x7E\n"
    	   " -addspc         genetate space(0x20)\n"
		   " -weight=[N]     1-9:weight(5:standard) 0:default\n"
		   " -italic         italic\n"
    	   " -fontlist       output font name list\n"
    	   " (-oldtable      use old table)\n"
    	);
    	return 1;
    }

    bool scanOpts(char* arg) {
    	char* p = arg;
    	if (paramEquLong(p, "-ttf", p)) {
    	    if (*p == '=')
    	    	++p;
    	    ttfname_ = p;
    	} else if (paramEquLong(p, "-oldtable", p)) {
    	    oldTable_ = (*p != '-');
    	} else if (paramEquLong(p, "-o", p)) {
    	    if (*p == '=')
    	    	++p;
    	    oname_ = p;
    	} else if (paramEquLong(p, "-tblname", p)) {
    	    if (*p == '=')
    	    	++p;
    	    tblname_ = p;
    	} else if (paramEquLong(p, "-ts", p)) {
    	    texW_ = (int)strtoul(p, (char**)&p, 0);
    	    if (rangeCheck(texW_, 64, 0x10000, arg) == false)
    	    	return false;
    	    if (*p) {
    	    	++p;
    	    	texH_ = (int)strtoul(p, (char**)&p, 0);
    	    	if (rangeCheck(texH_, 64, 0x10000, "-ts?:") == false)
    	    	    return false;
    	    }
    	} else if (paramEquLong(p, "-fs", p)) {
    	    fontW_ = (int)strtoul(p, (char**)&p, 0);
    	    if (rangeCheck(fontW_, 4, 2048, arg) == false)
    	    	return false;
    	} else if (paramEquLong(p, "-cs", p)) {
    	    cellW_ = (int)strtoul(p, (char**)&p, 0);
    	    if (rangeCheck(cellW_, 4, 2048, arg) == false)
    	    	return false;
			if (*p != '\0') {
				++p;
				cellH_ = (int)strtoul(p, (char**)&p, 0);
	    	    if (rangeCheck(cellH_, 4, 2048, arg) == false)
	    	    	return false;
			}
    	    if (cellH_ == 0)
    	    	cellH_ = cellW_;
    	} else if (paramEquLong(p, "-mul", p)) {
    	    mul_ = (int)strtoul(p, (char**)&p, 0);
    	    if (rangeCheck(cellW_, 1, 256, arg) == false)
    	    	return false;
    	} else if (paramEquLong(p, "-bpp=", p) || paramEquLong(p, "-bpp" , p) ) {
    	    bpp_ = (int)strtoul(p, (char**)&p, 0);
    	    if (rangeCheck(bpp_, 1, 8, arg) == false)
    	    	return false;
    	} else if (paramEquLong(p, "-addascii", p)) {
    	    addascii_	= (*p != '-');
    	} else if (paramEquLong(p, "-addspc", p)) {
    	    addspc_ 	= (*p != '-');
    	} else if (paramEquLong(p, "-addcr", p)) {
    	    addcr_  	= (*p != '-');
    	} else if (paramEquLong(p, "-fontlist", p)) {
    	    FontGetter::printFontInfo();
    	} else if (paramEquLong(p, "-weight", p)) {
    	    if (*p == '=')
    	    	++p;
    	    weight_	= (unsigned)strtoul(p, (char**)&p, 0);
    	    if (rangeCheck(weight_, 0, 9, arg) == false)
    	    	return false;
    	} else if (paramEquLong(p, "-italic", p)) {
    	    italic_	= (*p != '-');
    	} else {
    	    fprintf(stderr, "unkown option : %s\n", arg);
    	    return false;
    	}
    	return true;
    }

    bool paramEquLong(char const* s, char const* t, char* &result_s) {
    	return paramEquLong(s, t, (char const* &)result_s);
    }

    bool paramEquLong(char const* s, char const* t, char const* &result_s) {
    	size_t tlen = strlen(t);
    	if (strncmp(s, t, tlen) == 0) {
    	    s += tlen;
    	    result_s = s;
    	    return true;
    	}
    	return false;
    }

    bool rangeCheck(int val, int mi, int ma, char const* nm) {
    	if (mi <= val && val <= ma)
    	    return true;
    	fprintf(stderr, "option %s%d out of range(%d .. %d)\n", nm, val, mi, ma);
    	return false;
    }

    char* getOutName(char const* nm) {
    	fks_fnameGetBaseNameNoExt(onameBuf_, sizeof(onameBuf_), nm);
    	//fks_fnameCat(onameBuf_, sizeof(onameBuf_), ".tga");
    	return onameBuf_;
    }

    bool optsSetting() {
    	if (fontW_ == 0 && cellW_ == 0) {
    	    fontW_ = cellW_ = 32;
    	} else if (fontW_ == 0) {
    	    fontW_ = cellW_;
    	} else if (cellW_ == 0) {
    	    cellW_ = fontW_;
    	} else if (fontW_ > cellW_) {
    	    fprintf(stderr, "ERROR: need font-size < cell-size (%d,%d)\n", fontW_, cellW_);
    	    return false;
    	}
		if (cellH_ == 0)
			cellH_ = cellW_;

    	if (!tblname_) {
    	    _snprintf(tblNameBuf_, sizeof(tblNameBuf_), "g_chFontTable_%s", oname_);
    	    tblname_ = tblNameBuf_;
    	}

    	if (addspc_ || addcr_) {
   	    	cmap_[0x20] = 0x20;
		}
    	//if (addcr_) {
   	    //	cmap_[0x0a] = 0x0a;
   	    //	cmap_[0x0d] = 0x0d;
		//}
    	if (addascii_) {
    	    for (int i = 0x21; i < 0x7F; ++i) {
    	    	cmap_[i] = i;
    	    }
    	}
    	return true;
    }


    bool text2chr(char const* fname) {
    	FILE* fp = fopen(fname, "rt");
    	if (!fp) {
    	    fprintf(stderr, "%s : file open error\n", fname);
    	    return false;
    	}
    	char buf[0x8000] = {0};
    	while (fgets(buf, sizeof buf, fp) != NULL) {
    	    char const* s = buf;
    	    for (;;) {
    	    	s = strSkipSpc(s);
    	    	uint32_t c = mbc_getc(mbc_utf8, &s);
    	    	if (c == 0)
    	    	    break;
    	    	cmap_[c] = c;
    	    }
    	}
    	fclose(fp);
    	return true;
    }

    char* strSkipSpc(char const* s) {
    	while ((*s && *(unsigned char *)s <= ' ') || *s == 0x7f) {
    	    s++;
    	}
    	return (char*)s;
    }

    bool getFonts() {
    	fonts_.clear();
    	fonts_.resize( cmap_.size() );
    	int no = 0;
    	for (Cmap::iterator ite = cmap_.begin(); ite != cmap_.end(); ++ite) {
    	    fonts_[no].ch = ite->second;
    	    ++no;
    	}

    	FontGetter fontGetter(ttfname_, fontW_, cellW_, cellH_, mul_, bpp_, weight_, italic_);
    	fontGetter.get(fonts_);
    	return true;
    }

    bool makeTex() {
    	unsigned nw = texW_ / cellW_;
    	unsigned nh = texH_ / cellH_;
    	texChW_ = nw;
    	texChH_ = nh;
    	if (nw == 0 || nh == 0 || fontW_ > cellW_ || fontW_ > cellH_) {
    	    fprintf(stderr, "ERROR: bad size ... tex:%d*%d font:%d*%d  cell:%d*%d\n", texW_, texH_, fontW_, fontW_, cellW_, cellH_);
    	    return false;
    	}
    	unsigned cnum	   = unsigned(fonts_.size());
    	unsigned numOfPage = nw * nh;
    	unsigned pageSize  = (cnum + numOfPage - 1) / numOfPage;

    	texs_.resize(pageSize);
    	unsigned no = 0;
    	for (unsigned npage = 0; npage < pageSize; ++npage) {
    	    texs_[npage].resize(texW_ * texH_);
    	    uint8_t* tex = &texs_[npage][0];
    	    for (unsigned ny = 0; ny < nh; ++ny) {
    	    	for (unsigned nx = 0; nx < nw; ++nx) {
    	    	    copyFont(tex, nx, ny, &fonts_[no].data[0]);
    	    	    ++no;
    	    	    if (no >= cnum)
    	    	    	return true;
    	    	}
    	    }
    	}
    	return true;
    }

    void copyFont(uint8_t* tex, unsigned nx, unsigned ny, uint8_t const* font) {
    	for (unsigned y = 0; y < cellH_; ++y) {
    	    for (unsigned x = 0; x < cellW_; ++x) {
    	    	int xx = nx * cellW_ + x;
    	    	int yy = ny * cellH_ + y;
    	    	tex[yy * texW_ + xx] = font[y * cellW_ + x];
    	    }
    	}
    }


    bool saveOldChFontTableHeader() {
    	char	name[FKS_FNAME_MAX_PATH];
    	sprintf(name, "%s.h", oname_);
    	FILE* fp = fopen(name, "wt");
    	if (!fp) {
    	    fprintf(stderr, "%s : file open error\n", name);
    	    return false;
    	}
    	fprintf(fp, "// generated by %s\n", appname_);
    	fprintf(fp, "#ifndef %s_h_included\n", oname_);
    	fprintf(fp, "#define %s_h_included\n", oname_);
    	fprintf(fp, "%s",
    	    "\n"
    	    "#ifdef __cplusplus\n"
    	    "extern \"C\" {\n"
    	    "#endif\n"
    	    "\n"
    	    "typedef struct ChFontTableInfo {\n"
    	    "\tunsigned      \tchCount;\n"
    	    "\tunsigned short\ttexW;\n"
    	    "\tunsigned short\ttexH;\n"
    	    "\tunsigned short\tfontW;\n"
    	    "\tunsigned short\tfontH;\n"
    	    "\tunsigned short\ttexChW;\n"
    	    "\tunsigned short\ttexChH;\n"
    	    "\tunsigned short\ttexPage;\n"
    	    "} ChFontTableInfo;\n"
    	    "\n"
    	    "typedef struct ChFontTable {\n"
    	    "\tunsigned      \tcode;\n"
    	    "\tunsigned      \tindex;\n"
    	    "\tunsigned short\tx;\n"
    	    "\tunsigned short\ty;\n"
    	    "\tunsigned short\tw;\n"
    	    "\tunsigned short\th;\n"
    	    "} ChFontTable;\n"
    	    "\n"
    	    "extern ChFontTableInfo const\tg_chFontTableInfo;\n"
    	    "extern ChFontTable const\t\tg_chFontTable[];\n"
    	    "\n"
    	    "#ifdef __cplusplus\n"
    	    "} //extern \"C\"\n"
    	    "#endif\n"
    	    "\n"
    	    "#endif\n"
    	);
    	return true;
    }

    bool saveTexChFontInfoHeader() {
    	char const* name = "TexChFontInfo.h";
    	FILE* fp = fopen(name, "wt");
    	if (!fp) {
    	    fprintf(stderr, "%s : file open error\n", name);
    	    return false;
    	}
    	fprintf(fp, "// generated by %s\n", appname_);
    	fprintf(fp, "%s",
    	    #include "TexChFontInfo_h_cstr.hh"
    	);
    	fclose(fp);
    	return true;
    }

    bool saveTexChFontInfoExtern() {
    	char	name[FKS_FNAME_MAX_PATH];
    	sprintf(name, "%s.h", oname_);
    	FILE* fp = fopen(name, "wt");
    	if (!fp) {
    	    fprintf(stderr, "%s : file open error\n", name);
    	    return false;
    	}
    	fprintf(fp, "// generated by %s\n", appname_);
    	fprintf(fp, "#ifndef %s_h_included\n", oname_);
    	fprintf(fp, "#define %s_h_included\n", oname_);
    	fprintf(fp, "\n");
    	fprintf(fp, "#include \"TexChFontInfo.h\"\n");
    	fprintf(fp, "\n");
    	fprintf(fp, "#ifdef __cplusplus\n");
    	fprintf(fp, "extern \"C\" {\n");
    	fprintf(fp, "#endif\n");
    	fprintf(fp, "\n");
    	fprintf(fp, "extern TexChFontInfo const\t\t%s[];\t// [0]=id,n,tw,th,fw,fh [1..]=c,i,x,y,w,h\n", tblname_);
    	fprintf(fp, "\n");
    	fprintf(fp, "#ifdef __cplusplus\n");
    	fprintf(fp, "} //extern \"C\"\n");
    	fprintf(fp, "#endif\n");
    	fprintf(fp, "\n");
    	fprintf(fp, "#endif\n");
    	fclose(fp);
    	return true;
    }

    bool saveCTable() {
    	char	name[FKS_FNAME_MAX_PATH];
    	sprintf(name, "%s.c", oname_);
    	FILE* fp = fopen(name, "wt");
    	if (!fp) {
    	    fprintf(stderr, "%s : file open error\n", name);
    	    return false;
    	}

    	fprintf(fp, "// generated by %s\n", appname_);
    	if (oldTable_) {
    	    fprintf(fp, "#include \"%s.h\"\n", oname_);
    	    fprintf(fp, "\n");
    	    fprintf(fp, "ChFontTableInfo const\tg_chFontTableInfo = {\n");
    	    fprintf(fp, "\t%u,\t// chCount\n", unsigned(fonts_.size()));
    	    fprintf(fp, "\t%u,\t// texW\n", texW_);
    	    fprintf(fp, "\t%u,\t// texH\n", texH_);
    	    fprintf(fp, "\t%u,\t// fontW\n", cellW_);
    	    fprintf(fp, "\t%u,\t// fontH\n", cellH_);
    	    fprintf(fp, "\t%u,\t// texChW\n", texChW_);
    	    fprintf(fp, "\t%u,\t// texChH\n", texChH_);
    	    fprintf(fp, "\t%u,\t// texPage\n", unsigned(texs_.size()));
    	    fprintf(fp, "};\n");
    	    fprintf(fp, "\n");
    	    fprintf(fp, "\n");
    	    fprintf(fp, "ChFontTable const\tg_chFontTable[] = {\n");
    	} else {
    	    fprintf(fp, "#include \"TexChFontInfo.h\"\n");
    	    fprintf(fp, "#include \"%s.h\"\n", oname_);
    	    fprintf(fp, "\n");
    	    fprintf(fp, "TexChFontInfo const\t%s[] = {\n", tblname_);
    	    fprintf(fp, "\t{\n");
    	    fprintf(fp, "\t\t0x%08x,\t// id\n", TEX_FONT_INFO_ID);
    	    fprintf(fp, "\t\t%10u,\t// chCount\n", unsigned(fonts_.size()));
    	    fprintf(fp, "\t\t%10u,\t// texW\n", texW_);
    	    fprintf(fp, "\t\t%10u,\t// texH\n", texH_);
    	    fprintf(fp, "\t\t%10u,\t// fontW\n", cellW_);
    	    fprintf(fp, "\t\t%10u,\t// fontH\n", cellH_);
    	    fprintf(fp, "\t},\n");
    	    fprintf(fp, "\n");
    	    fprintf(fp, "\t// code     , index, texW, texH,fontW,fontH,\n");
    	}
    	char buf[2048] = {0};
    	for (unsigned i = 0; i < fonts_.size(); ++i) {
    	    Font const& f = fonts_[i];
    	    mbc_setc(mbc_utf8, buf, f.ch);
    	    fprintf(fp, "\t{ 0x%08x, %5d, %4d, %4d, %4d, %4d },\t// %s \n", f.ch, i, f.x, f.y, f.w, f.h, buf);
    	}
    	fprintf(fp, "};\n");
    	fclose(fp);
    	return true;
    }

    bool saveBinFile() {
    	if (fonts_.empty())
    	    return false;
    	char	name[FKS_FNAME_MAX_PATH];
    	sprintf(name, "%s.bin", oname_);
    	FILE* fp = fopen(name, "wb");
    	if (!fp) {
    	    fprintf(stderr, "%s : file open error\n", name);
    	    return false;
    	}
    	std::vector<uint8_t>	buf(sizeof(TexChFontInfoHeader) + sizeof(TexChFontInfo) * fonts_.size());
    	// for little endian
    	TexChFontInfoHeader*	hdr = reinterpret_cast<TexChFontInfoHeader*>(&buf[0]);
    	hdr->id      = TEX_FONT_INFO_ID;
    	hdr->chCount = fonts_.size();
    	hdr->texW    = texW_;
    	hdr->texH    = texH_;
    	hdr->fontW   = cellW_;
    	hdr->fontH   = cellH_;
    	TexChFontInfo*	fontInfo = reinterpret_cast<TexChFontInfo*>( &buf[ sizeof(TexChFontInfoHeader) ] );
    	for (unsigned i = 0; i < fonts_.size(); ++i) {
    	    Font const&    s = fonts_[i];
    	    TexChFontInfo& d = fontInfo[i];
    	    d.code  = s.ch;
    	    d.index = i;
    	    d.x     = s.x;
    	    d.y     = s.y;
    	    d.w     = s.w;
    	    d.h     = s.h;
    	}
    	fwrite(&buf[0], 1, buf.size(), fp);
    	fclose(fp);
    	return true;
    }

    bool saveTga() {
    	char	name[FKS_FNAME_MAX_PATH];
    	int wksize = tga_encodeWorkSize(texW_, texH_, 8);
    	std::vector<uint8_t>	tga(wksize * 2);
    	for (unsigned n = 0; n < texs_.size(); ++n) {
    	    sprintf(name, "%s_%04d.tga", oname_, n);
    	    int  sz = tga_writeEx(&tga[0], wksize, texW_, texH_, 8, &texs_[n][0], int(texW_), 8, clut_, 32, 0, 0, 0);
    	    if (sz < 0) {
    	    	fprintf(stderr, "%s : tga encode error (%d*%d)\n", name, texW_, texH_);
    	    	return false;
    	    }
    	    if (fileSave(name, &tga[0], unsigned(sz)) == false) {
    	    	return false;
    	    }
    	}
    	return true;
    }

    bool    fileSave(char const* name, uint8_t const* src, unsigned sz) {
    	FILE* fp = fopen(name, "wb");
    	if (!fp) {
    	    fprintf(stderr, "%s : file open error\n", name);
    	    return false;
    	}
    	size_t l = fwrite(src, 1, sz, fp);
    	if (l != sz) {
    	    fprintf(stderr, "%s : file write error\n", name);
    	    fclose(fp);
    	    return false;
    	}
    	fclose(fp);
    	return true;
    }

    void    makeClut() {
    	memset(clut_, 0, sizeof clut_);
    	unsigned tone = 1 << bpp_;
    	for (unsigned i = 1; i < tone; ++i) {
    	    unsigned a = i * 255 / (tone-1);
    	    clut_[i] = (a << 24) | 0xffffff;
    	}
    }

private:
    FontVec 	fonts_;
    Cmap    	cmap_;
    char const* appname_;
    char const* ttfname_;
    char const* oname_;
    char const* tblname_;
    unsigned	texW_;
    unsigned	texH_;
    unsigned	fontW_;
    //int   	fontH_;
    unsigned	cellW_;
 	unsigned   	cellH_;
    unsigned	texChW_;
    unsigned	texChH_;
    unsigned	mul_;
    unsigned	bpp_;
    bool    	addascii_;
	bool		addspc_;
    bool    	oldTable_;
    unsigned	weight_;
    bool		italic_;
    TexBuf  	texs_;
    uint32_t	clut_[256];

    char    	onameBuf_[FKS_FNAME_MAX_PATH];
    char    	tblNameBuf_[FKS_FNAME_MAX_PATH];
    char    	tblInfoNameBuf_[FKS_FNAME_MAX_PATH];
};



int main(int argc, char* argv[]) {
    return App().main(argc, argv);
}
