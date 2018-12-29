#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <vector>
#include <string>
#include <map>

#define ISKANJI(c)  	((uint8_t)(c) >= 0x81 && ((uint8_t)(c) <= 0x9F || ((uint8_t)(c) >= 0xE0 && (uint8_t)(c) <= 0xFC)))
#define ISKANJI2(c) 	((uint8_t)(c) >= 0x40 && (uint8_t)(c) <= 0xfc && (c) != 0x7f)
#define JIS2KUTEN(j)   	(((((j) >> 8) & 0x7f) - 0x21) * 94 + (((j) & 0x7f) - 0x21))


/** 8bit数二つを上下につなげて16ビット数にする */
#define BB(a,b)     	((((uint8_t)(a))<<8)|(uint8_t)(b))

/** 8bit数4つを上位から順につなげて32ビット数にする */
#define BBBB(a,b,c,d)	(((uint32_t)((uint8_t)(a))<<24)|(((uint8_t)(b))<<16)|(((uint8_t)(c))<<8)|((uint8_t)(d)))

/** 8bit数6つを... 64bit整数用 */
#define BBBBBB(a,b,c,d,e,f) (((uint64_t)((uint8_t)(a))<<40)|((uint64_t)((uint8_t)(b))<<32)|(((uint8_t)(c))<<24)|(((uint8_t)(d))<<16)|(((uint8_t)(e))<<8)|((uint8_t)(f)))


extern "C" unsigned 		kutenIdx_to_utf32_tbl_jisx213with212[];
extern "C" unsigned 		kutenIdx_to_utf32_tbl_cp932[];


class App {
public:
	typedef std::map<unsigned,unsigned>	MapUU;

    App()
    {
    }

    int main(int argc, char* argv[]) {
		MapUU uniToSjisX;
		for (int i = 1; i <= 2*94; ++i) {
			int ku = kuTbl_[i-1];
			for (int j = 1; j <= 94; ++j) {
				int n = ku*94 + (j-1);
				unsigned uc  = kutenIdx_to_utf32_tbl_jisx213with212[n];
				unsigned jis = ((ku+0x21)<<8) + (j+0x20);
				if (!uniToSjisX[uc])
					uniToSjisX[uc] = jis;
			}
		}
		MapUU cp932toSjisX;
		MapUU sjisXToCp932;
		MapUU cp932toUc;
		// 89-92:NEC選定IBM拡張文字  115-119:IBM拡張文字
		for (int i = 89; i <= 119/*120*/; ++i) {
			if (92 < i && i < 115)
				continue;
			for (int j = 1; j <= 94; ++j) {
				int n = (i-1)*94 + (j-1);
				unsigned uc = kutenIdx_to_utf32_tbl_cp932[n];
				if (uc == 0)
					continue;
				unsigned jis = ((i+0x20)<<8) + (j+0x20);
				unsigned xjis = uniToSjisX[uc];
				if (xjis == 0) {
					char buf[100] = {0};
					utf8_setC(buf, buf + sizeof buf, uc);
					fprintf(stderr, "WARNING: JIS:%04x U+%04X(%s) not found in JIS-X-213(with 212)\n", jis, uc, buf);
				} else {
					cp932toSjisX[jis] = xjis;
					sjisXToCp932[xjis] = jis;
					cp932toUc[jis] = uc;
				}
			}
		}

		printf("\t// cp932idx,\tjis2004idx,\t// unicode\t932jis(kuten)\tjis2004(kuten)\n");
		for (MapUU::iterator ite = cp932toSjisX.begin();
				ite != cp932toSjisX.end();
				++ite)
		{
			char buf[100] = {0};
			unsigned uc = cp932toUc[ite->first];
			utf8_setC(buf, buf + sizeof buf, uc);
			unsigned f = ite->first;
			unsigned s = ite->second;
			f = ((f >> 8) - 0x21) * 94 + ((f&0xff) - 0x21);
			s = ((s >> 8) - 0x21) * 94 + ((s&0xff) - 0x21);
			printf("\t{%#06x,\t%#06x},\t// U+%04X %s\t%04x(%3d*94+%2d)\t%04x(%3d*94+%2d)\n"
				, f, s, uc, buf
				, ite->first, (f / 94)+1, f % 94 + 1
				, ite->second, (s / 94)+1, s % 94 + 1
			);
		}

        return 0;
    }


	/** Set character.
	 */
	static char*    utf8_setC(char*  dst, char* e, unsigned c) {
	    char* d = dst;
	    if (c < 0xC0/*0x80*/) {	// 0x80-xBF bad code
			if (d >= e) goto ERR;
	        *d++ = c;
	    } else if (c <= 0x7FF) {
			if (d+2 > e) goto ERR;
	        *d++ = 0xC0|(c>>6);
	        *d++ = 0x80|(c&0x3f);
	    } else if (c <= 0xFFFF) {
			if (d+3 > e) goto ERR;
	        *d++ = 0xE0|(c>>12);
	        *d++ = 0x80|((c>>6)&0x3f);
	        *d++ = 0x80|(c&0x3f);
	        //if (c >= 0xff60 && c <= 0xff9f) {--(*adn); }	// hankaku-kana
	    } else if (c <= 0x1fFFFF) {
			if (d+4 > e) goto ERR;
	        *d++ = 0xF0|(c>>18);
	        *d++ = 0x80|((c>>12)&0x3f);
	        *d++ = 0x80|((c>>6)&0x3f);
	        *d++ = 0x80|(c&0x3f);
	    } else if (c <= 0x3fffFFFF) {
			if (d+5 > e) goto ERR;
	        *d++ = 0xF8|(c>>24);
	        *d++ = 0x80|((c>>18)&0x3f);
	        *d++ = 0x80|((c>>12)&0x3f);
	        *d++ = 0x80|((c>>6)&0x3f);
	        *d++ = 0x80|(c&0x3f);
	    } else {
			if (d+6 > e) goto ERR;
	        *d++ = 0xFC|(c>>30);
	        *d++ = 0x80|((c>>24)&0x3f);
	        *d++ = 0x80|((c>>18)&0x3f);
	        *d++ = 0x80|((c>>12)&0x3f);
	        *d++ = 0x80|((c>>6)&0x3f);
	        *d++ = 0x80|(c&0x3f);
	    }
	    return d;

	ERR:
		while (d < e)
			*d++ = 0;
		return e;
	}

	void initKuTbl() {
		uint8_t* p = kuTbl_;
		for (int i = 0; i < 94; ++i)
			*p++ = i;
		// jis x 213
		*p++ = 94 +  1 - 1;
		*p++ = 94 +  3 - 1;
		*p++ = 94 +  4 - 1;
		*p++ = 94 +  5 - 1;
		*p++ = 94 +  8 - 1;
		*p++ = 94 + 12 - 1;
		*p++ = 94 + 13 - 1;
		*p++ = 94 + 14 - 1;
		*p++ = 94 + 15 - 1;
		for (int i = 78-1; i <= 94 - 1; ++i)
			*p++ = 94 + i;
		// jis x 212
		*p++ = 94 +  2 - 1;
		*p++ = 94 +  6 - 1;
		*p++ = 94 +  7 - 1;
		*p++ = 94 +  9 - 1;
		*p++ = 94 + 10 - 1;
		*p++ = 94 + 11 - 1;
		for (int i = 16-1; i <= 77 - 1; ++i)
			*p++ = 94 + i;
	}

private:
	uint8_t		kuTbl_[2 * 94];
};


int main(int argc, char* argv[])
{
    return App().main(argc, argv);
}
