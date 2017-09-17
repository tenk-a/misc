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

struct Font {
    Font(uint32_t c=0) : ch(c),x(0),y(0),w(0),h(0) {}

    uint32_t	    	    ch;
    uint32_t	    	    x;
    uint32_t	    	    y;
    uint32_t	    	    w;
    uint32_t	    	    h;
    std::vector<uint8_t>    data;
};

typedef std::vector<Font>   FontVec;

class FontGetter {
public:
    FontGetter(char const* ttfname, unsigned fontW, unsigned cellW, unsigned mul, unsigned bpp);
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
    unsigned	    	    mul_;
    unsigned	    	    bpp_;
    unsigned	    	    tone_;
    std::vector<uint8_t>    wkBuf_;
};

#endif
