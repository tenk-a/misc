/**
 *	@file	ectab.c
 *	@brief	空白タブ変換ツール
 *
 *	@author 北村雅史<NBB00541@nifty.com>
 *	@date	2001(?)～ 2004-01-29
 *	@note
 *		アセンブラで書かれたmsdos-16bit-exe版をリメイク。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "cmisc.h"

#undef	BOOL
#define BOOL int

#undef	STDERR
#define STDERR	stderr

#define DBG_S(s)				printf("%-14s %5d : %s", __FILE__, __LINE__, (s), 0)
#define DBG_F(s)				printf s
#define DBG_M() 				printf("%-14s %5d :\n", __FILE__, __LINE__)


/** 説明表示＆終了 */
static void usage(void)
{
	printf("usage> ectab [-opts] file(s)    // v2.10 " __DATE__ "  writen by M.Kitamura\n"
		   "  タブ変換,行末空白削除等を行う. デフォルトでは標準出力\n"
		   "  改行として LF(\\n un*x) CR(\\r mac) CRLF(\\r\\n win/dos) を認識\n"
		   "  -o        出力を元ファイル名にする。元々のファイルは.bakにする\n"
		   "  -o[FILE]  FILEに出力.\n"
		   "  -m        行末の空白を削除.\n"
		   "  -r[0-3]   改行を 0:入力のまま 1:'\\n' 2:'\\r' 3:'\\r\\n' に変換(file出力時のみ)\n"
		   "  -s[N]     出力のタブサイズを N にする(空白->タブ)\n"
		   "  -t[N]     入力のタブサイズを N にする(タブ->空白)\n"
		   "  -z        出力のタブサイズが空白1文字にしかならない場合は空白で出力\n"
		   "  -j        出力のタブサイズ丁度の空白のみタブに変換\n"
		   "  -q        出力を4タブ8タブで見た目が違わないようにする\n"
		   "  -b[0-2]   C/C++の \" ' 対を考慮 0:しない 1:する 2:対中のctrl文字を\\文字化\n"
		   "  -u        半角小文字の大文字化\n"
		   "  -l        半角大文字の小文字化\n"
		   "  -a        EOFとして0x1aを出力\n"
		   "  -k[0-2]   0:1バイト系  1:シフトJIS(MBC)  2:UTF8  (デフォルト -k1)\n"
		   "  -n[N:M:L:STR]  行番号を付加. N:桁数,M:スキップ数,L:0行目の行番号,STR:区切\n"
	);
	exit(1);
}


#define stpcpy(d,s) 			((d) + sprintf((d), "%s", (s)))


/** オプション設定 */
typedef struct opts_t {
	int  stab;
	int  dtab;
	int  cmode;
	BOOL both48;
	BOOL sp1ntb;
	BOOL ajstab;
	BOOL crlfMd;
	BOOL trimSw;
	BOOL sjisSw;
	BOOL utf8Sw;
	BOOL eofSw;
	BOOL uprSw;
	BOOL lwrSw;
	unsigned  numbering;
	unsigned  numbStart;
	int       numbSkip;
	char      *numbSep;
	// 本来入れたくなかったが、面倒なんで
	const char *outname;
	const char *extname;
} opts_t;


/** 文字列中に壊れた全角文字がないかチェック
 *  @return  0:壊れてなかった 1:壊れていた
 */
static int isJstrBroken(char buf[])
{
	unsigned char *s = (unsigned char *)buf;
	int rc = 0;

	for (;;) {
		int c = *s++;
		if (c == 0)
			break;
		if (ISKANJI(c)) {
			c = *s++;
			if (c == 0) {
				return 1;
			}
			if (ISKANJI2(c) == 0) {
				rc = 1;
			}
		}
	}
	return rc;
}



/** '\n','\r','\r\n'の何れかを改行とする１行入力
 *	@param	buf 	読み込むバッファ
 *	@param	len 	バッファサイズ.
 *	@param	crlfMd	改行を 0:変換しない 1:\nに変換 2:\rに変換 3:\r\nに変換
 *	@return 0:正常読込	bit0=1:eof bit1=1:読込エラー bit2:1行が長すぎる
 *			bit3:'\0'在り bit4:バイナリコードがある
 */
static int getLine(char *buf, size_t bufSz, int crlfMd, FILE *fp)
{
	char *p   = buf;
	//char *e = buf + bufSz - 1;
	size_t i  = 0;
	int    rc = 0;
	int    c;

	assert(buf);
	assert(bufSz > 0);
	--bufSz;
	buf[0] = 0;
	for (;;) {
		if (i == bufSz) {
			rc |= 1<<2; //("1行が長すぎる\n");
			break;
		}
		c = fgetc(fp);
		if (feof(fp)) {
			if (i > 0) {
				break;
			}
			return 1<<0;
		}
		if (ferror(fp)) {
			rc |= 1<<1; //("リードエラーが起きました.\n");
		}
		if (c < 0x20 || c == 0x7f) {
			if (c == '\0') {
				rc |= 1<<3; //("行中に '\\0' が混ざっている\n");
				c = ' ';
			} else if (c == '\n') {
				if (crlfMd & 3) {
					if (crlfMd & 2) *p++ = '\r';
					if (crlfMd & 1) *p++ = '\n';
				} else {
					*p++ = '\n';
				}
				break;
			} else if (c == '\r') {
				c = fgetc(fp);
				if (c == '\n') {
					if (crlfMd & 3) {
						if (crlfMd & 2) *p++ = '\r';
						if (crlfMd & 1) *p++ = '\n';
					} else {
						*p++ = '\r';
						*p++ = '\n';
					}
				} else {
					if (feof(fp) == 0) {
						ungetc(c,fp);
					}
					if (crlfMd & 3) {
						if (crlfMd & 2) *p++ = '\r';
						if (crlfMd & 1) *p++ = '\n';
					} else {
						*p++ = '\r';
					}
				}
				break;
			} else if (c == '\a' || c == '\b' || c == '\t'
					|| c == '\v' || c == '\f' || c == '\r'
					|| c == 0x1a || c == 0x1b) {
				;
			} else {
				rc |= 1 << 4;	// コントロールコードが混ざっている
				c = ' ';
			}
		}
		*p++ = c;
		i++;
	}
	*p = 0;
	return rc;
}


/** iname のテキストをoの変換指定にしたがって変換し oname に出力
 *  @param	iname	入力ファイル名. NULLなら標準入力
 *  @param	oname	出力ファイル名. NULLなら標準出力
 *  @return 		0:失敗 1:成功
 */
static int convFile(const char *iname, const char *oname, opts_t *o)
{
	enum {SBUF_SZ = 16*1024};
	char buf[16*16*1024];
	char sbuf[16*1024];
	char *p;
	FILE *ofp, *ifp;
	unsigned lno = 0, numb;
	int  er, rc = 1, crlfMd = o->crlfMd;
	int  tabFlags, trimFlags, upLwrFlags;

	if (iname) {
		const char *md = "rb";			// 改行を自前で管理したいのでバイナリでオープン
		if (oname == NULL)				// 出力が標準出力のときは、\r\nの扱いが面倒になるので、
			md = "rt";					// いっそテキストモードにして\r\nの入力をos/ライブラリに任す^^;
		ifp = fopen(iname, md);
		if (ifp == NULL) {
			fprintf(STDERR, "%s がopenできない\n", iname);
			return 0;
		}
	} else {
		iname = "<stdin>";
		ifp = stdin;
	}
	if (oname) {
		ofp = fopen(oname, "wb");		// 改行を自前で管理したいのでバイナリでオープン
		if (ofp == NULL) {
			fprintf(STDERR, "%s がopenできない\n", oname);
			exit(1);
		}
	} else {
		oname = "<stdout>";
		ofp   = stdout;
	}

	// 行番号表示関係の設定
	numb = o->numbStart;
	if (o->numbSkip == 0)
		o->numbSkip = 1;
	if (o->numbSep == NULL)
		o->numbSep = " ";

	// タブ変換関係の準備
	tabFlags =  (o->cmode << 1) | (o->ajstab << 4) | (o->both48 << 5);
	if (o->cmode)
		tabFlags |= o->trimSw << 8;		// cモードの時は'"ペアのチェックの都合, strTabからstrTrimSpcRを呼ぶ
	if (tabFlags && o->stab == 0) {		// 変換指定があるのに、ソースタブサイズが無い場合は、強制設定
		if (o->cmode)	o->stab = 4;	// cモードなら4
		else			o->stab = 8;	// 以外は 9
	}
	if (o->stab || o->dtab || tabFlags)
		tabFlags |= (o->utf8Sw << 9) | (o->sjisSw << 7) | (1<<6) | (o->sp1ntb);

	// タブ変換しないけど、行末空白削除しない場合は専用に呼び出す
	trimFlags = 0;
	if (tabFlags == 0 && o->trimSw) {
		// bit0=1:行末の'\n''\r'は外さない. bit1=1:C/C++での\文字を考慮(行連結に化けないように)
		trimFlags = (o->sjisSw << 7) /*|(o->cmode << 1)*/ | 1;
	}

	// 大文字小文字変換の設定
	upLwrFlags = (o->lwrSw<<1) | o->uprSw;
	if (upLwrFlags)
		upLwrFlags |= (o->sjisSw << 7);

	// c/c++向け' " ペアチェックの初期化
	strTab(NULL, NULL, 0,0,0,0);

	// 変換本編
	for (;;) {
		er = getLine(sbuf, sizeof(sbuf), crlfMd, ifp);
		if (er) {
			if (er & 0x10)
				fprintf(STDERR, "%s %d : 入力の１行が長すぎる\n", iname, lno);
			if (er & 0x08)
				fprintf(STDERR, "%s %d : バイナリコードがあった\n", iname, lno);
			if (er & 0x04)
				fprintf(STDERR, "%s %d : '\0'が行中にあった\n", iname, lno);
			if (er & 0x02)
				break;		// リードエラーだった
			if (er & 0x01)
				break;		// EOFだった
		}
		++lno;
		numb += o->numbSkip;
		p = sbuf;
		// タブ空白変換
		if (o->stab || o->dtab || tabFlags) {
			strTab(buf, sbuf, o->dtab, o->stab, tabFlags, sizeof(buf));
			p = buf;
		}
		// 行末空白の削除
		if (trimFlags) {
			strTrimSpcR(p, trimFlags);
		}
		// 文字列の大文字/小文字化
		if (upLwrFlags) {
			strUpLow(p, upLwrFlags);
		}
		// 壊れた全角文字がないかチェック
		if (o->sjisSw && isJstrBroken(p)) {
			fprintf(STDERR, "%s %d : 壊れた全角文字があります\n", iname, lno);
		}
		// 行を出力
		if (o->numbering) {	// 行番号付き
			fprintf(ofp, "%*d%s%s", o->numbering, numb, o->numbSep, p);
		} else {			// そのまま
			fprintf(ofp, "%s", p);
		}
		if (ferror(ofp))
			break;
	}
	// eofをつかる場合
	if (o->eofSw) {
		fputs("\x1a", ofp);
	}
	// 後処理
	if (er & 2) {
		fprintf(STDERR, "%s (%d) 読み込みでエラーがあった\n", iname, lno);
		rc = 0;
	}
	if (ferror(ofp)) {
		fprintf(STDERR, "%s (%d) 書き込みでエラーがあった\n", oname, lno);
		rc = 0;
	}
	if (ifp != stdin)
		fclose(ifp);
	if (ofp != stdout)
		fclose(ofp);
	return rc;
}


/** 1ファイルの変換. ファイル名の辻褄会わせ */
static void oneFile(const char *iname, const char *oname, const char *extname, opts_t *o)
{
	// ファイル名生成やバックアップの処理
	char nameBuf[_MAX_PATH+8];
	char *tmpname = NULL;
	int rc;

	if (iname) {
		if (iname && oname && oname[0] == 0) {	// 入力ファイル名自身に出力
			sprintf(nameBuf, "%s.~tmp", iname);
			oname = nameBuf;
			tmpname = nameBuf;
			fprintf(STDERR, "[%s]\n", iname);
		} else if (extname && extname[0]) {
			sprintf(nameBuf, "%s.%s", iname, extname);
			oname = nameBuf;
		}
	}

	rc = convFile(iname, oname, o);

	if (rc && iname && iname[0] && tmpname) {		// 入力ファイル自身の出力の場合
		char bakname[_MAX_PATH+8];
		sprintf(bakname, "%s.bak", iname);
		remove(bakname);
		rename(iname, bakname);
		rename(tmpname, iname);
	}
}

/** オプション文字の次に'-'か'0'なら偽, 以外なら真にする為のマクロ */
static int opts_getVal(const char *arg, char **pp, int dfltVal, int maxVal)
{
	char *p = *pp;
	int val;

	if (*p == '-') {
		*pp = p + 1;
		val = 0;
	} else if (isdigit(*p)) {
		val = strtol(p, pp, 0);
	} else {
		val = dfltVal;
	}
	if (val > maxVal) {
		fprintf(STDERR, "オプション引数%s中の値 %d が大きすぎる(>%d)\n", arg, val, maxVal);
		exit(1);
	}
	return val;
}


/** オプション解析 */
static void opts_get(char *arg, opts_t *o)
{
	char *p = arg + 1;

	if (*p == '\0') {	// - だけなら標準入力
		oneFile(NULL, o->outname, o->extname, o);
		return;
	}
	while (*p) {
		int   c = *p++;
		c = toupper(c);
		switch (c) {
		case 'O':
			o->outname = strdup(p);
			return;
		case 'X':
			o->extname = strdup(p);
			return;
		case 'K':
			o->sjisSw = opts_getVal(arg, &p, 1, 2);
			o->utf8Sw = o->sjisSw == 2;
			o->sjisSw &= 1;
			break;
		case 'M':
			o->trimSw = opts_getVal(arg, &p, 1, 1);
			break;
		case 'R':
			o->crlfMd = opts_getVal(arg, &p, 0, 3);
			break;
		case 'A':
			o->eofSw  = opts_getVal(arg, &p, 1, 1);
			break;
		case 'U':
			o->uprSw  = opts_getVal(arg, &p, 1, 1);
			break;
		case 'L':
			o->lwrSw  = opts_getVal(arg, &p, 1, 1);
			break;
		case 'B':
		case 'C':
			o->cmode  = opts_getVal(arg, &p, 1, 2);
			if (o->cmode == 2)
				o->cmode = 4|2|1;
			else if (o->cmode == 1)
				o->cmode = 4|1;
			break;
		case 'Z':
			o->sp1ntb = opts_getVal(arg, &p, 1, 9) ? 1 : 0;
			break;
		case 'J':
			o->ajstab = opts_getVal(arg, &p, 1, 1);
			break;
		case 'Q':
			o->both48 = opts_getVal(arg, &p, 1, 1);
			break;
		case 'S':
			o->dtab = opts_getVal(arg, &p, 4, 256);
			break;
		case 'T':
			o->stab = opts_getVal(arg, &p, 4, 16);
			break;
		case 'N':
			o->numbering = (*p == 0) ? 7 : strtoul(p,&p,0);
			if (*p == ':' || *p == ',') {
				p++;
				o->numbSkip = strtol(p,&p,0);
				if (*p == ':' || *p == ',') {
					p++;
					o->numbStart = strtoul(p, &p, 0);
					if (*p == ':' || *p == ',') {
						p++;
						o->numbSep = strdup(p);
						return;
					}
				}
			}
			break;
		case 'H':
		case '?':
			usage();
			return;
		default:
	  //ERR_OPTS:
			fprintf(STDERR, "知らないオプション(%s)\n", arg);
			exit(1);
			return;
		}
	}
}


/** ここより始まる */
int main(int argc, char *argv[])
{
	int  i;
	char *p;
	static opts_t opt;
	opts_t *o = &opt;

	memset(o, 0, sizeof *o);
	o->sjisSw = 1;

	if (argc < 2)
		usage();

	for (i = 1; i < argc; i++) {
		p = argv[i];
		if (*p == '-') {	// オプション解析
			opts_get(p, o);
		} else {			// ファイル実行
			oneFile(p, o->outname, o->extname, o);
		}
	}
	return 0;
}


