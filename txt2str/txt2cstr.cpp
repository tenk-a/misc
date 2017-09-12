/**
 *	@file	txt2cstr.cpp
 *	@brief	テキストファイルをc/c++の文字列に変換するコマンドラインツール
 *
 *	@author	北村雅史<NBB00541@nifty.com>
 *	@date	2003-07-26
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <set>

#include "cmisc.h"

using namespace std;
using namespace CMISC;

//#define CERR	cerr
#define CERR	cout


/// 説明表示＆終了
void usage(void)
{
	CERR<< "txt2cstr [-opts] file(s)\n"
		   " SJISテキストファイルをc/c++ソース用の\"文字列\"に変換する\n"
		   "  -o[FILE]  出力ファイル名\n"
		   "  -s        入力ファイルがなければ標準入力に、出力指定が無ければ標準出力にする\n"
		   "  -c        行毎の最後に , を置く\n"
		   "  -fSTR     出力文字列をprintf形式で指定。%sが入力行に置き換わる。\n"
		   "            例えば -fputs(%s); のように指定\n"
	;
	exit(1);
}


// ---------------------------------------------------------------------------

/// オプション要素の管理
class Opts {
  public:
	string	outName_;		///< 出力名(1回きり)
	string	appName_;		///< アプリケーション名
	string	fmt_;			///< 出力フォーマットの指定
	int		stdio_;			///< 1:"-s"で標準入出力可能にする. 0:標準入出力しない

	Opts() : outName_(""), appName_(""), fmt_("%s"), stdio_(0) {}
	~Opts() {}
	/// このプログラム名を
	void setAppName(const char *appNm) {appName_ = string(appNm);}
    void get(const char *p);
};


/// -で始まるコマンドラインオプションの解析
void Opts::get(const char *arg)
{
	const char *p = arg + 1;
	int c = *p++;
	c = toupper(c);
	switch (c) {
	case 'O':
		outName_ = string(p);
		break;
	case 'S':
		stdio_   = (*p != '-');
		break;
	case 'C':
		fmt_ = "%s,";
		break;
	case 'F':
		fmt_ = p;
		break;
	case '?':
		::usage();
		break;
	default:
		CERR << arg << " は知らないオプションです\n";
		exit(1);
	}
}



// ---------------------------------------------------------------------------

/// テキスト -> cソース 変換
class Conv {
	string fmt_;
	static char *chTbl[256];
	int  convLine(vector<char> &st, const char *src);
  public:
	Conv() {
		fmt_ = "%s";
	}
	~Conv() {}
	/// 出力フォーマットを設定する
	bool setFmt(string &fmt);
	/// 変換を実行する
	int  run(const char *name, const char *outName);
};


/// 変換本体
///
int Conv::run(const char *name, const char *outName)
{
	FILE *ifp;
	if (name && name[0]) {
		ifp = fopen(name, "rt");
		if (ifp == NULL) {
			CERR << name << " をopenできない\n";
			return false;
		}
	} else {
		ifp = stdin;
	}

	FILE *ofp;
	if (outName && outName[0]) {
		ofp = fopen(outName, "wt");
		if (ofp == NULL) {
			CERR << outName << " をopenできない\n";
			return false;
		}
	} else {
		ofp = stdout;
	}

	vector<char> st(0x10000);	// なんとなく、stringでなくvector<char>でお試し
	//int lineNum = 0;
	while (feof(ifp) == 0) {
		//lineNum++;
		char buf[0x10000];
		if (fgets(buf, sizeof buf, ifp) == NULL)
			break;

		st.clear();
		st.push_back('"');
		convLine(st, buf);
		st.push_back('"');
		st.push_back('\0');
		fprintf(ofp, "\t");
		fprintf(ofp, fmt_.c_str(), &st[0]);
		fprintf(ofp, "\n");
	}

	if (ofp != stdout)
		fclose(ofp);
	if (ifp != stdin)
		fclose(ifp);

	return true;
}


/// 特殊な文字コードを変換するためのテーブル
char *Conv::chTbl[256] = {				// a:0x07,b:0x08,t:0x09,n:0x0a,v:0x0b,f:0x0c,r:0x0d,
	"\\x00", "\\x01", "\\x02", "\\x03", "\\x04", "\\x05", "\\x06", "\\a"  ,
	"\\b"  , "\\t"  , "\\n"  , "\\v"  , "\\f"  , "\\r"  , "\\x0e", "\\x0f",
	"\\x10", "\\x11", "\\x12", "\\x13", "\\x14", "\\x15", "\\x16", "\\x17",
	"\\x18", "\\x19", "\\x1a", "\\x1b", "\\x1c", "\\x1d", "\\x1e", "\\x1f",
	0/* */ , 0/*!*/ , "\\\"" , 0/*#*/ , 0/*$*/ , 0/*%*/ , 0/*&*/ , 0/*'*/ ,
	0/*(*/ , 0/*)*/ , 0/***/ , 0/*+*/ , 0/*,*/ , 0/*-*/ , 0/*.*/ , 0/* / */,
	0/*0*/ , 0/*1*/ , 0/*2*/ , 0/*3*/ , 0/*4*/ , 0/*5*/ , 0/*6*/ , 0/*7*/ ,
	0/*8*/ , 0/*9*/ , 0/*:*/ , 0/*;*/ , 0/*<*/ , 0/*=*/ , 0/*>*/ , 0/*?*/ ,
	0/*@*/ , 0/*A*/ , 0/*B*/ , 0/*C*/ , 0/*D*/ , 0/*E*/ , 0/*F*/ , 0/*G*/ ,
	0/*H*/ , 0/*I*/ , 0/*J*/ , 0/*K*/ , 0/*L*/ , 0/*M*/ , 0/*N*/ , 0/*O*/ ,
	0/*P*/ , 0/*Q*/ , 0/*R*/ , 0/*S*/ , 0/*T*/ , 0/*U*/ , 0/*V*/ , 0/*W*/ ,
	0/*X*/ , 0/*Y*/ , 0/*Z*/ , 0/*[*/ , "\\\\" , 0/*]*/ , 0/*^*/ , 0/*_*/ ,
	0/*`*/ , 0/*a*/ , 0/*b*/ , 0/*c*/ , 0/*d*/ , 0/*e*/ , 0/*f*/ , 0/*g*/ ,
	0/*h*/ , 0/*i*/ , 0/*j*/ , 0/*k*/ , 0/*l*/ , 0/*m*/ , 0/*n*/ , 0/*o*/ ,
	0/*p*/ , 0/*q*/ , 0/*r*/ , 0/*s*/ , 0/*t*/ , 0/*u*/ , 0/*v*/ , 0/*w*/ ,
	0/*x*/ , 0/*y*/ , 0/*z*/ , 0/*{*/ , 0/*|*/ , 0/*}*/ , 0/*~*/ , "\\x7f",
	// 0x80〜0xFF はとりあえず、全て0
};


/// １行変換
int Conv::convLine(vector<char> &st, const char *src)
{
	const unsigned char *s = (const unsigned char *)src;
	while (*s) {
		int c = *s++;
		if (ISKANJI(c) && *s) {
			st.push_back(c);
			st.push_back(*s);
			s++;
		} else {
			const char *p = chTbl[c];
			if (p) {
				while (*p)
					st.push_back(*p++);
			} else {
				st.push_back(c);
			}
		}
	}
	return 1;
}


/// 出力フォーマットを設定する
bool Conv::setFmt(string &fmt)
{
	int n  = 0;
	int sf = 0;

	// チェック
	for (;;) {
		n = fmt.find_first_of('%', n);
		if (size_t(n) == string::npos)
			break;
		++n;
		if (fmt[n] == '%') {
			++n;
		} else if (fmt[n] == 's') {
			++n;
			++sf;
			if (sf > 1) {
				CERR << "-f指定中 %s が複数ある\n";
				return false;
			}
		} else {
			CERR << "%% %s 以外の%指定があるようだ\n";
			return false;
		}
	}
	fmt_ = fmt;
	return true;
}


// ---------------------------------------------------------------------------


class App {
	Opts	opts_;
	Conv	conv_;
  public:
	App() {}
	~App() {}
	int main(int argc, char *argv[]);
	int oneFile(const char *name);
};


int App::main(int argc, char *argv[])
{
	if (argc < 2)
		::usage();

	opts_.setAppName(argv[0]);
	int rc = 0, n = 0;
	for (int i = 1; i < argc; i++) {
		char *p = argv[i];
		if (*p == '-') {
			opts_.get(p);
		} else {
			rc = oneFile(p);
			n++;
		}
	}
	if (n == 0 && opts_.stdio_) {
		rc = oneFile("");
	}
	rc = !rc;		// main()の復帰値は 0:正常 0以外:エラー なんで、そのように変換
	return rc;
}


int App::oneFile(const char *name)
{
	string inm(name);
	string onm(name);

	if (opts_.outName_.empty()) {	// オプション-o がない場合
		if (opts_.stdio_)			// 標準出力指定があれば、ファイル名を無くす
			onm = "";
		else						// 通常は .c にしたファイルに出力
			onm += ".c";
	} else {						// オプション-oがあった場合
		onm = opts_.outName_;
		opts_.outName_ = "";		// -oは一回こっきりなんで、次回向けに初期化
	}
	if (conv_.setFmt(opts_.fmt_) == false)
		return 0;
	return conv_.run(inm.c_str(), onm.c_str());
}



// ---------------------------------------------------------------------------

/// ここより始まる
int main(int argc, char *argv[])
{
	int rc;
	App app;
	try {
	  #ifdef _MSC_VER
		// VC(7)でコンパイルしたら、argv[0]にフルパス化していない、コマンドライン
		// でタイプしたプログラム名が入ってたりするので、強引対処
		argv[0] = _pgmptr;
	  #endif
		rc = app.main(argc, argv);
	}
	catch (const exception &ex) {
		CERR << ex.what() << endl;
		rc = 1;
	}
	return rc;
}



