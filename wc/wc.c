/**
 *	@file	wc.c
 *	@brief	行数、単語数カウンター
 *	@author	tenk*
 *	@note
 *	-	普通の wc.
 *	-	file_hdl.h のサンプル...
 *	-	行として\nしか数えないのは(\n無し最終行を含まないのは)、wc の仕様.
 */


#include "file_hdl.h"
#include "ExArgv.h"			// コマンドラインのワイルドカード展開等を行う.
#include <ctype.h>

typedef file_size64_t		size_type;

#if defined _WIN32
#define STDOUT_PRINTF(...)	do { char bUf[1030]; wsprintfA(bUf, __VA_ARGS__); file_write(FILE_HDL_STDOUT(),bUf,strlen(bUf)); } while (0)
#define STR_PER_LLU(d)		"%" #d "I64d"
#define STR_LF				"\r\n"
#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#pragma warning(disable: 4127)
#endif
#else
#include <stdint.h>
#include <stdio.h>
typedef uint64_t			size_type;
#define STDOUT_PRINTF		printf
#define STR_LF				"\n"
#define STR_PER_LLU(d)		"%" #d "lld"
#endif



typedef struct CounterLWC {
	size_type	line_;
	size_type	word_;
	size_type	chr_;
	unsigned	wordFlag_;
} CounterLWC;


enum { BUF_SIZE = 0x100000 };
static char	g_buf[ BUF_SIZE ];




int countLWC(CounterLWC* pLWC, const char* pBegin, const char* pEnd)
{
	const char* s  = pBegin;
	unsigned	c  = 0;
	pLWC->chr_	  += pEnd - pBegin;
	while (s != pEnd) {
		c = *(unsigned char*)s++;
		if (isspace(c)) {
			pLWC->wordFlag_ = 0;
			if (c == '\n') {
				++pLWC->line_;
			}
		} else {
			if (!pLWC->wordFlag_) {
				pLWC->wordFlag_ = 1;
				++pLWC->word_;
			}
		}
	}
	return c == '\n';
}



void oneFile(const char* fname, CounterLWC* pTotalLWC)
{
	CounterLWC	lwc = {0};
	FILE_HDL	hdl = file_open_r(fname);
	if (hdl == FILE_ERR_HDL) {
		file_puts(fname, FILE_HDL_STDERR());
		file_puts(" : Can't open.\n", FILE_HDL_STDERR());
	} else {
		size_t		rdsz;
		do {
			rdsz = file_read(hdl, g_buf, BUF_SIZE);
			if (rdsz == 0)
				break;
			countLWC(&lwc, g_buf, g_buf+rdsz);
		} while (rdsz == BUF_SIZE);
		file_close(hdl);
		STDOUT_PRINTF(STR_PER_LLU(7) " " STR_PER_LLU(8) " " STR_PER_LLU(9) " %s" STR_LF, lwc.line_, lwc.word_, lwc.chr_, fname);
		pTotalLWC->line_ += lwc.line_;
		pTotalLWC->word_ += lwc.word_;
		pTotalLWC->chr_  += lwc.chr_;
	}
}



int main(int argc, char* argv[])
{
	CounterLWC	total	= {0};
	int			i;
  #ifdef EXARGV_INCLUDED
	ExArgv_conv(&argc, &argv);
  #endif
	if (argc <= 1) {
		size_t	rdsz;
		do {
			rdsz = file_read(FILE_HDL_STDIN(), g_buf, BUF_SIZE);
			if (rdsz == 0)
				break;
			countLWC(&total, g_buf, g_buf+rdsz);
		} while (rdsz == BUF_SIZE);
		STDOUT_PRINTF(STR_PER_LLU(7) " " STR_PER_LLU(8) " " STR_PER_LLU(9) STR_LF, total.line_, total.word_, total.chr_);
	} else {
		for (i = 1; i < argc; ++i) {
			const char* p = argv[i];
			if (*p != '-')
				oneFile(p, &total);
		}
		STDOUT_PRINTF(STR_PER_LLU(7) " " STR_PER_LLU(8) " " STR_PER_LLU(9) " total" STR_LF, total.line_, total.word_, total.chr_);
	}
	return 0;
}
