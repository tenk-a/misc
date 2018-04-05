/**
 *  @file   FontGetter.hpp
 *  @brief  get font images
 *  @author Masashi KITAMURA
 *  @date   2017-09
 */
#ifndef FONTGETTER_HPP
#define FONTGETTER_HPP

#pragma once

#include <stdint.h>
#include <vector>

struct TexChFontInfo;
struct TexChFontInfo0;

struct Font {
    Font(uint32_t c=0) : ch(c),x(0),y(0),w(0),h(0),ox(0),oy(0) {}

	void getPara(TexChFontInfo& dst) const;
	void getPara(TexChFontInfo0& dst) const;

public:
    uint32_t	    	    ch;
    uint32_t	    	    x;
    uint32_t	    	    y;
    uint32_t	    	    w;
    uint32_t	    	    h;
    int						ox;
    int						oy;
    std::vector<uint8_t>    data;
};

typedef std::vector<Font>   FontVec;

class FontGetter {
public:
    FontGetter(char const* ttfname, unsigned fontW, unsigned cellW, unsigned cellH, unsigned mul, unsigned bpp, unsigned weight, bool italic);
    ~FontGetter();

    bool get(FontVec& fonts);

    static void printFontInfo();

private:
    bool getFont(void* hdc0, Font& font);
    bool adjustFontSize(Font& rFont);

private:
    char*   	    	    ttfname_;
    unsigned	    	    fontW_;
    unsigned	    	    cellW_;
    unsigned	    	    cellH_;
    unsigned	    	    mul_;
    unsigned	    	    bpp_;
    unsigned	    	    tone_;
    unsigned				weight_;
    bool					italic_;
    std::vector<uint8_t>    wkBuf_;
};

#endif
