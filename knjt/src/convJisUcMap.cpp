#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <vector>
#include <string>
#include <map>


class App {
public:
    App()
    {
		memset(kuten2uc_, 0, sizeof kuten2uc_);
		memset(kuten2ux_, 0, sizeof kuten2ux_);
		larges_.reserve(1024);
		maxUxIdx_ = 0;
    }

    int main(int argc, char* argv[]) {
		if (argc <= 1) {
			usage();
			return 1;
		}
        for (int i = 1; i < argc; ++i) {
            char* p = argv[i];
            if (*p == '-') {
                if (scanOpts(p) == false)
                    return 1;
                continue;
            }
			if (oneFile(p) == false)
				return 1;
        }
        return 0;
    }

private:
	void usage() {
		printf("usage> convJisUcMap kuten-unicode-table.txt\n"
			"'JIS X 0213:2004 vs Unicode mapping table'の区点とunicodeから区点連番でunicode変換するテーブルを生成.\n"
			);
	}

    bool scanOpts(char const* opt) {
		char const* s = opt + 1;
        int c = *s;
        ++s;
        switch (c) {
		case 'h':
			usage();
			return false;
        default:
            fprintf(stderr, "Unkown option : %s\n", opt);
            return false;
        }
        return true;
    }

	bool oneFile(char const* fname) {
		if (readTblFile(fname) == false)
			return false;
		std::string fnm = fname;
		//saveTblBin(fname + ".bin");
		//if (saveTblC(fnm + ".c") == false) return false;
		saveTblC((fnm + ".c").c_str());
		saveTblC2((fnm + "_x.c").c_str());
		saveUtf32Text((fnm + ".utf32.txt").c_str());
		saveMs932Text((fnm + ".ms932.txt").c_str());
		saveSjis2004Text((fnm + ".sjis.txt").c_str());
		return true;
	}

	bool readTblFile(char const* fname) {
		FILE* fp = fopen(fname, "rt");
		if (!fp) {
			fprintf(stderr, "%s : File not open.", fname);
			return false;
		}
		unsigned l = 0;
		char buf[4096] = { 0 };
		while (fgets(buf, sizeof buf, fp) != NULL) {
			++l;
			char const* s = &buf[0];
			s = skipSpc(s);
			if (*s == '#')
				continue;

			unsigned men = strtoul(s, (char**)&s, 10);
			if (men == 0 || *s != '-')
				continue;
			if (men < 1 || 2 < men) {
				fprintf(stderr, "%s (%d) : ERROR: 'MEN': out of range(1..2) (%d)\n", fname, l, men);
				return false;
			}
			unsigned ku = strtoul(s+1, (char**)&s, 10);
			if (ku == 0 || *s != '-')
				continue;
			if (ku < 1 || 120/*94*/ < ku) {
				fprintf(stderr, "%s (%d) : ERROR: 'KU': out of range(1..94) (%d)\n", fname, l, ku);
				return false;
			}
			unsigned ten = strtoul(s+1, (char**)&s, 10);
			if (ten == 0)
				continue;
			if (ten < 1 || 94 < ten) {
				fprintf(stderr, "%s (%d) : ERROR: 'TEN': out of range(1..94) (%d)\n", fname, l, ten);
				return false;
			}

			char const* p = strchr(s, '#');
			s = strstr(s, "U+");
			if (s == NULL)
				continue;
			if (p && p < s)
				continue;

			unsigned uc = strtoul(s + 2, (char**)&s, 16);
			if (uc < 1 || 0x10ffff < uc) {
				fprintf(stderr, "%s (%d) : ERROR: unicode : out of range(1..0x10ffff) (U+%04X)\n", fname, l, uc);
				return false;
			}
			unsigned uc2 = 0;
			if (*s == '+') {
				uc2 = strtoul(s+1, (char**)&s, 16);
				if ((uc < 1 || 0xffff < uc) || (uc2 < 1 || 0x7fff < uc2)) {
					fprintf(stderr, "%s (%d) : ERROR: +unicode : out of range(1..0x7fff) (U+%04X+U+%04X)\n", fname, l, uc, uc2);
					return false;
				}
			}
			if (uc2)
				uc = 0x80000000 | (uc2 << 16) | uc;
			--men;
			--ku;
			--ten;
			unsigned n = men * 94 * 94 + ku * 94 + ten;
			kuten2uc_[n] = uc;

			unsigned jisO = uc2kuten_[uc];
			if (jisO) {
				--jisO;
				fprintf(stderr, "%s (%d) : WARNING: Same unicode (jis: %d-%02d-%02d, %d-%02d-%02d)\n", fname, l
								, jisO / (94 * 94) + 1, (jisO / 94) % 94 + 1, jisO % 94 + 1, men + 1, ku + 1, ten + 1);
			} else {
				uc2kuten_[uc] = n + 1;
			}

			if (men > 0) {			//0 1 2 3  4  5  6 7  8 9 a b c  d   e   f
				static int s_tbl[] = {0,95,0,96,97,98,0,0,98,0,0,0,99,100,101,102 };
				++ku;
				if (ku < 16)
					ku = s_tbl[ku];
				else if (ku >= 78 && ku <= 94)
					ku = 103 + ku - 78;
				else
					ku = 0;
				if (!ku || men != 1 || ten < 0 || ten >= 94) {
					fprintf(stderr, "%s (%d) : ERROR: bad ku-ten code (jis: %d-%02d-%02d)\n", fname, l, men, ku, ten);
					continue;
				}
			}
			n = ku * 94 + ten;
			if (uc > 0xffff) {
				kuten2ux_[n] = larges_.size() + 0xDC00;
				larges_.push_back(uc);
			} else {
				kuten2ux_[n] = uc;
			}
			maxUxIdx_ = n;
		}
		fclose(fp);
		return true;
	}

	char const* skipSpc(char const* s)
	{
		while (*s && *(unsigned char *)s <= ' ')
			s++;
		return s;
	}

	char const* skipNotSpc(char const* s)
	{
		while (*(unsigned char *)s > ' ')
			s++;
		return s;
	}

	bool saveTblC(char const* fname) {
		FILE* fp = fopen(fname, "wt");
		if (!fp) {
			fprintf(stderr, "%s : File not open.", fname);
			return false;
		}
		for (int l = 0; l < 94 * 2; ++l) {
			fprintf(fp, "\t// %d-%02d (%3d)\n", l / 94 + 1, l % 94 + 1, l+1);
			for (int j = 0; j < 94; ++j) {
				if (j % 10 == 0)
					fprintf(fp, "\t");
				fprintf(fp, "0x%08x, ", kuten2uc_[l * 94 + j]);
				if (j % 10 == 9)
					fprintf(fp, "\n");
			}
			fprintf(fp, "\n\n");
		}
		fclose(fp);
		return true;
	}

	bool saveTblC2(char const* fname) {
		FILE* fp = fopen(fname, "wt");
		if (!fp) {
			fprintf(stderr, "%s : File not open.", fname);
			return false;
		}
		fprintf(fp, "// %dbytes + %dbytes = %dbytes\n\n", maxUxIdx_ * 2, larges_.size() * 4, maxUxIdx_ * 2 + larges_.size() * 4);
		fprintf(fp, "unsigned short g_kuten2uni[] = {\n");
		for (size_t l = 0; l < (maxUxIdx_ + 93) / 94; ++l) {
			fprintf(fp, "\t// %d-%02d (%3d)\n", l / 94 + 1, l % 94 + 1, l+1);
			for (int j = 0; j < 94; ++j) {
				if (j % 20 == 0)
					fprintf(fp, "\t");
				fprintf(fp, "0x%04x, ", kuten2ux_[l * 94 + j]);
				if (j % 20 == 19)
					fprintf(fp, "\n");
			}
		fprintf(fp, "\n\n");
		}
		fprintf(fp, "\n};\n\n");
		fprintf(fp, "unsigned short g_kuten2uniAdd[] = {\n");
		for (size_t j = 0; j < larges_.size(); ++j) {
			if (j % 8 == 0)
				fprintf(fp, "\t");
			fprintf(fp, "0x%08x, ", larges_[j]);
			if (j % 8 == 7)
				fprintf(fp, "\n");
		}
		fprintf(fp, "\n};\n\n");
		fclose(fp);
		return true;
	}

	bool saveMs932Text(char const* fname) {
		FILE* fp = fopen(fname, "wt");
		if (!fp) {
			fprintf(stderr, "%s : File not open.", fname);
			return false;
		}
		for (int l = 0; l < 120; ++l) {
			fprintf(fp, "# %d-%02d\n", l / 94 + 1, l % 94 + 1);
			for (int j = 0; j < 94; ++j) {
				unsigned jis =  ((l + 0x21) << 8) | (j + 0x21);
				unsigned sjis = jis2sjis(jis);
				fprintf(fp, "%c%c ", uint8_t(sjis >> 8), uint8_t(sjis) );
				if (j % 20 == 19)
					fprintf(fp, "\n");
			}
			fprintf(fp, "\n\n");
		}
		fclose(fp);
		return true;
	}

	bool saveSjis2004Text(char const* fname) {
		FILE* fp = fopen(fname, "wt");
		if (!fp) {
			fprintf(stderr, "%s : File not open.", fname);
			return false;
		}
		for (int l = 0; l < 94 * 2; ++l) {
			fprintf(fp, "// %d-%02d\n", l / 94 + 1, l % 94 + 1);
			for (int j = 0; j < 94; ++j) {
				unsigned kuten = l * 94 + j;
				unsigned jis = kuten2jis(kuten);
				unsigned sjis = jis2sjis2004(jis);
				fprintf(fp, "%c%c ", uint8_t(sjis >> 8), uint8_t(sjis) );
				if (j % 20 == 19)
					fprintf(fp, "\n");
			}
			fprintf(fp, "\n\n");
		}
		fclose(fp);
		return true;
	}

	bool saveUtf32Text(char const* fname) {
		FILE* fp = fopen(fname, "wb");
		if (!fp) {
			fprintf(stderr, "%s : File not open.", fname);
			return false;
		}
		unsigned spc = ' ';
		unsigned lf = '\n';
		for (int l = 0; l < 94 * 2; ++l) {
			char buf[100];
			sprintf(buf, "# %d-%02d\n", l / 94 + 1, l % 94 + 1);
			for (int i = 0; buf[i]; ++i) {
				unsigned c = buf[i];
				fwrite(&c, 4, 1, fp);
			}
			//fprintf(fp, "# %d-%02d\n", l / 94 + 1, l % 94 + 1);
			for (int j = 0; j < 94; ++j) {
				unsigned uc = kuten2uc_[l * 94 + j];
				if (uc == 0)
					uc = 0x3000;
				if (uc & 0x80000000) {
					unsigned uc2 = (uc >> 16) & 0x7fff;
					uc = uc & 0xffff;
					fwrite(&uc, 4, 1, fp);
					fwrite(&uc2, 4, 1, fp);
				} else {
					fwrite(&uc, 4, 1, fp);
				}
				fwrite(&spc, 4, 1, fp);
				if (j % 20 == 19)
					fwrite(&lf, 4, 1, fp);
			}
			fwrite(&lf, 4, 1, fp);
			fwrite(&lf, 4, 1, fp);
		}
		fclose(fp);
		return true;
	}

	static unsigned kuten2jis(unsigned kuten) {
		unsigned men = kuten / (94 * 94);
		unsigned ku = (kuten / 94) % 94;
		unsigned ten = (kuten) % 94;
		return (men << 16) | ((ku + 0x21) << 8) | (ten + 0x21);

	}

	static int jis2sjis(unsigned c)
	{
		c -= 0x2121;
		if (c & 0x100)
			c += 0x9e;
		else
			c += 0x40;
		if ((uint8_t)c >= 0x7f)
			++c;
		c = (((c >> (8 + 1)) + 0x81) << 8) | ((uint8_t)c);
		if (c >= 0xA000)
			c += 0x4000;
		return c;
	}

	static int jis2sjis2004(unsigned c)
	{
		if (c <= 0xffff) {
			c -= 0x2121;
			if (c & 0x100)
				c += 0x9e;
			else
				c += 0x40;
			if ((uint8_t)c >= 0x7f)
				++c;
			c = (((c >> (8 + 1)) + 0x81) << 8) | ((uint8_t)c);
			if (c >= 0xA000)
				c += 0x4000;
			return c;
		}
		else {	// jis2004
			unsigned a, b;
			b = (uint16_t)c - 0x2121;
			a = b >> 8;
			if (b & 0x100)
				b += 0x9e;
			else
				b += 0x40;
			b = (uint8_t)b;
			if (b >= 0x7f)
				++b;
			if (a < 78 - 1) {	// 1,3,4,5,8,12,15-ku (0,2,3,4,7,11,14)
				a = (a + 1 + 0x1df) / 2 - ((a + 1) / 8) * 3;
			}
			else { // 78..94
				a = (a + 1 + 0x19b) / 2;
			}
			return (a << 8) | b;
		}
	}



private:
	unsigned						kuten2uc_[2 * 94 * 94];
	uint16_t						kuten2ux_[2*94*94];
	std::map<unsigned, unsigned>	uc2kuten_;
	std::vector<uint32_t>			larges_;
	uint32_t						maxUxIdx_;
};


int main(int argc, char* argv[])
{
    return App().main(argc, argv);
}
