/*
	テキストファイルを読みこみ、MS全角 n個を１行として出力

	0.50 で -aN を追加したバージョンと
	0.80 で -s  を追加したバージョンの
	２系統に分かれてしまっていたので、統合。
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "subr.h"

void Usage(void)
{
	printf(
		"usage> chr2lin [-opts] textfile(s)  //v0.90  " __DATE__ "  " __TIME__ "  by tenk\n"
		"テキストファイルを読みこみ、出現順にMS全角 1個を１行として出力\n"
		"半角、および一度出現した文字は無視する\n"
		" -oNAME 出力ファイル名指定\n"
		" -nN    1個で１行なく N個で一行にして出力\n"
		" -eN    N 行ごとに改行を入れる\n"
		" -yN    N 行ごとにファイルを分ける\n"
		"        出力ファイル名は, outfile.000 outfile.001 のような\n"
		"        連番拡張子のファイルを生成\n"
		" -ye    -yと同じだが、名前のつけ方が outfile000.ext のようになる\n"
		" -s     ソートする\n"
		" -cC    テキスト中の文字 C 以降改行までを無視する.\n"
		"        -c のみだと // コメントを無視する.\n"
		" -aN    出力の N行を空白にする\n"
	);
	exit(1);
}

/* ------------------------------------------------------------------------ */
SLIST	*fileList = NULL;
long    opt_clm = 1;
long    opt_len = 0;
int		opt_mltDivFlg = 0;
int		opt_cmtChr = 0;
int		opt_sort = 0;
char    *opt_oname = NULL;
int		addLinNum = 0;


int Opts(char *a)
{
	/* オプションの処理 */
	char *p;
	int c;

	p = a;
	p++, c = *p++, c = toupper(c);
	switch (c) {
	case 'O':
		opt_oname = strdupE(p);
		break;
	case 'N':
		opt_clm = strtol(p,&p,0);
		if (opt_clm <= 0) {
			err_exit("-nでの分割数が 0か負です\n");
		}
		break;
	case 'E':
		opt_len = strtol(p,&p,0);
		if (opt_len < 0) {
			err_exit("-eでの分割数が負です\n");
		}
		break;
	case 'S':
		opt_sort = (*p == '-') ? 0 : 1;
		break;
	case 'Y':
		opt_mltDivFlg = 1;
		if (*p == 'E' || *p == 'e') {
			opt_mltDivFlg = 2;
			p++;
		}
		opt_len = strtol(p,&p,0);
		if (opt_len < 0) {
			err_exit("-yでの分割数が負です\n");
		}
		break;
	case 'C':
		opt_cmtChr = *p;
		if (*p == 0)
			opt_cmtChr = -1;
		break;
	case 'A':
		addLinNum = strtol(p,&p,0);
		break;

	case 'Z':
		debugflag = (*p == '-') ? 0 : 1;
		break;
	case '\0':
	case '?':
		Usage();
	default:
		err_exit("Incorrect command line option : %s\n", a);
	}
	return 0;
}



/*---------------------------------------------------------------------------*/
#define CHR_BUF_MAX 0xFFFF
static Uint16 chrBuf[CHR_BUF_MAX];
static int    chrBuf_num;

int chrBuf_cmp(const void *a, const void *b)
{
	return *(Uint16*)a - *(Uint16*)b;
}


int ChrChkAdd(int c)
{
	int i;

	// 出現順に登録したいので、単純な検索
	for (i = chrBuf_num; --i >= 0;) {
		if (c == chrBuf[i])
			return 0;
	}
	//printf("%c%c", GHB(c), GLB(c));
	chrBuf[chrBuf_num++] = c;
	return 1;
}


void GetFile(SLIST *sl_first)
{
	SLIST *sl;
	char buf[1030], *s;
	int	 c;

	chrBuf_num = 0;
	for (sl = sl_first; sl != NULL; sl = sl->link) {
		TXT1_OpenE(sl->s);
		while (TXT1_GetsE(buf, sizeof buf)) {
			s = buf;
			s = StrSkipSpc(s);
			while (*s) {
				c = (Uint8)*s++;
				if (opt_cmtChr) {
					if (c == opt_cmtChr)
						break;
					else if (opt_cmtChr < 0 && c == '/' && *s == '/')
						break;
				}
				if (ISKANJI(c)) {
					if (ISKANJI2((Uint8)*s)) {
						c = (c << 8) | *(Uint8*)s;
						ChrChkAdd(c);
					}
					if (*s)
						s++;
				}
			}
		}
		TXT1_Close();
	}
	chrBuf[chrBuf_num] = 0;
}



/*-----------------------------------------------------------------------*/
static char	mlt_name[FIL_NMSZ], mlt_basename[FIL_NMSZ], mlt_ext[FIL_NMSZ];
static int  mlt_num;


FILE *MltFileOpen(int md)
{
	if (md)
		sprintf(mlt_name, "%s%03d.%s", mlt_basename, mlt_num, mlt_ext);
	else
		sprintf(mlt_name, "%s.%03d", mlt_basename, mlt_num);
	mlt_num++;
	return fopenE(mlt_name, "wt");
}


void MltFile(char *inputname, char *oname, int w, int h, int md)
{
	FILE		*fp;
	int			c,l,m,n;


	if (oname == NULL)
		oname = inputname;

	mlt_num = 0;
	if (md) {
		strcpy(mlt_ext, FIL_ExtPtr(oname));
	}
	if (oname == NULL)
		oname = inputname;
	strcpy(mlt_basename, oname);
	FIL_ChgExt(mlt_basename, NULL);

	fp = MltFileOpen(md);

	c = 0x8140;
	for (m = l = n = 0; n < addLinNum*w; n++) {
		fprintf(fp, "%c%c", GHB(c), GLB(c));
		if (++m == w) {
			fprintf(fp,"\n");
			m = 0;
			++l;
			if (h && l == h) {
				fprintf(fp,"\n");
				l = 0;
			}
		}
	}
	if (m)
		fprintf(fp,"\n");

	for (l = addLinNum, m = n = 0; n < chrBuf_num; n++) {
		c = chrBuf[n];
		fprintf(fp, "%c%c", GHB(c), GLB(c));
		if (++m == w) {
			fprintf(fp,"\n");
			fflush(fp);
			m = 0;
			++l;
			if (h && l == h) {
				fclose(fp);
				fp = MltFileOpen(md);
				l = 0;
			}
		}
	}
	if (m)
		fprintf(fp,"\n");
	fclose(fp);
}







void OneFile(char *inputname, char *oname, int w, int h)
{
	FILE	*fp;
	int 	c,l,m,n;

	if (oname == NULL) {
		printf("[%s] -> <stdout> %d*%d\n", inputname, w, h);
		fp = stdout;
	} else {
		printf("[%s] -> [%s] %d*%d\n", inputname, oname, w, h);
		fp = fopenE(oname, "wt");
	}

	if (addLinNum == 0) {
		for (l = m = n = 0; n < chrBuf_num; n++) {
			c = chrBuf[n];
			fprintf(fp, "%c%c", GHB(c), GLB(c));
			if (++m == w) {
				fprintf(fp,"\n");
				m = 0;
				++l;
				if (h && l == h) {
					fprintf(fp,"\n");
					l = 0;
				}
			}
		}
	} else {
		c = 0x8140;
		for (m = l = n = 0; n < addLinNum*w; n++) {
			fprintf(fp, "%c%c", GHB(c), GLB(c));
			if (++m == w) {
				fprintf(fp,"\n");
				m = 0;
				++l;
				if (h && l == h) {
					fprintf(fp,"\n");
					l = 0;
				}
			}
		}
		if (m)
			fprintf(fp,"\n");
		for (m = 0, l = addLinNum, n = 0; n < chrBuf_num; n++) {
			c = chrBuf[n];
			fprintf(fp, "%c%c", GHB(c), GLB(c));
			if (++m == w) {
				fprintf(fp,"\n");
				m = 0;
				++l;
				if (h && l == h) {
					fprintf(fp,"\n");
					l = 0;
				}
			}
		}
	}
	if (m)
		fprintf(fp,"\n");

	if (oname)
		fclose(fp);
}



/*---------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
	char	binname[260];
	int i;
	char *p;

	if (argc < 2)
		Usage();

	/* コマンドライン解析 */
	for (i = 1; i < argc; i++) {
		p = argv[i];
		if (*p == '-') {
			Opts(p);
		} else {
			SLIST_Add(&fileList, p);
		}
	}

	if (fileList == NULL) {
		err_exit("ファイル名を指定してください\n");
	}

	/* テキスト入力 */
	GetFile(fileList);

	/* ソートするとき */
	if (opt_sort) {
		qsort(chrBuf, chrBuf_num, 2, chrBuf_cmp);
	}

	/* 1ファイルを処理 */
	if (opt_mltDivFlg)
		MltFile(fileList->s, opt_oname, opt_clm, opt_len, opt_mltDivFlg-1);
	else
		OneFile(fileList->s, opt_oname, opt_clm, opt_len);

	return 0;
}

