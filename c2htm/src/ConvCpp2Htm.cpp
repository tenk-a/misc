/**
 *	@file	ConvCpp2Htm.cpp
 *	@brief	c/c++ソースをhtml化
 *
 *	@author	北村雅史<NBB00541@nifty.com>
 *	@date	2003-07-21 ～
 */

#include <iostream>
#include <fstream>
#include "ConvCpp2htm.h"
#include "cmisc.h"

using namespace std;


/// c/c++ ソースを１行、html で色つきに変換。
/// (ブロックコメントや文字列状態は内部に保持して継続)
bool ConvCpp2Htm::convLine(std::string &dst, const char *src)
{
	// 今回の１行を解析用にセット
	setSrcPtr(src);

	// モードから今回のトークン(というか色付けの種類)を設定
	chkSetMode2CurTok();

	escEolFlg_ = false;			// 最後が \ だったかのフラグをクリア

	string spc("");

	// 行頭のみの処理
	if (mode_ == MODE_NORM && tokSw_[T_SHARP]) {
		int c;
		tokStr_.clear();
		for (;;) {
			c = getC();
			if (c == 0 || c > ' ')
				break;
			spc += char(c);
		}
		if (c == '#' && tokSw_[T_SHARP]) {	// #行だったぜ
			cur_tok_ = chkGetSharpName(tokStr_);
			spc += tokStr_;
		} else {
			ungetC(c);	// 違ったら戻しとけ
		}
	}

	dst += mrkBgn_[cur_tok_];	// とりあえず、出力の文字(色)設定をセット
	dst += spc;					// 行頭空白(あるいは#単語)があれば出力に追加

	// 文字列(行)が終わるまで繰り返す
	for (;;) {
		tokStr_.clear();
		tok_t tok = getTok(tokStr_);
		if (tok == T_ZERO)		// 文字列の終わり(行末)だった
			break;
		bak_tok_ = tok;
		if (tok == T_MEMB)
			tok = T_SYM;
		// 現在のタイプと違うタイプだった？
		if (tok != cur_tok_) {
			// 今回のがNONE でないか、NONEでも前回のがNONEを継続対象にしない種類の場合
			// 出力設定を変更
			if (tok != T_NONE || nextTokNoneOkTbl[cur_tok_] == false) {
				if (cur_tok_ != T_NONE) {
					dst += mrkEnd_[cur_tok_];
				}
				cur_tok_ = tok;
				if (cur_tok_ != T_NONE) {
					dst += mrkBgn_[cur_tok_];
				}
			}
		}
		getSpc(tokStr_);
		dst += tokStr_;
	}

	dst += mrkEnd_[cur_tok_];	// 出力の文字(色)設定の終了をセット
	return true;
}


/// モードから今回のトークン(というか色付けの種類)を設定
///
void ConvCpp2Htm::chkSetMode2CurTok()
{
	// 現在のモードにあわせて、今行初っ端の色のために
	// cur_tok_ に対応するトークンを設定。
	switch (mode_) {
	case MODE_STR_R:
	case MODE_STR:		// " 文字列
		cur_tok_ = T_WQUOT;
		break;

	case MODE_CVAL:		// ' 文字定数
		cur_tok_ = T_SQUOT;
		break;

	case MODE_RSTR:		// ` 文字列
		cur_tok_ = T_BSQUOT;
		break;

	case MODE_BLKCMT:	// /*  コメント
		cur_tok_ = (cmt_typ_ == CMT_DOXY) ? T_CMTJ
				 : (cmt_typ_ == CMT_NOSPC)  ? T_CMTK
				 : T_CMT;
		break;

	case MODE_BLKCMTD:	// /+  コメント
		cur_tok_ = (cmt_typ_ == CMT_DOXY) ? T_CMTJ
				 : T_CMTD;
		break;

	case MODE_CMT1:		// //  コメント
		if (escEolFlg_) {
			// //コメントの行末が \だった場合... 今回も1行コメント:-<
			cur_tok_ = (cmt_typ_ == CMT_DOXY) ? T_CMTJ
					 : (cmt_typ_ == CMT_NOSPC)  ? T_CMTK
					 : T_CMT;
		} else {
			// 通常は、コメント終了
			mode_    = MODE_NORM;
			cur_tok_ = T_NUM;
		}
		break;

	//case MODE_NORM:
	default:
		// 絶対本編に現れないタイプにする
		cur_tok_ = T_NUM;
		break;
	}
}


/// 記号,単語,コメントを出力向けに変換して取得
/// @param st	取得した文字列を入れて返す
/// @return		トークン
ConvCpp2Htm::tok_t ConvCpp2Htm::getTok(string &st)
{
	int k;
	int c = getC();
	tok_t tok = chTypTbl_[c];

	switch (tok) {
	case T_LF:		// '\n' だが、たぶん出現しないかも...万が一
		//st += char(c);			// 下手に出力すると面倒だろうで破棄
		// 継続

	case T_ZERO:	// '\0'
		//if (mode_ == MODE_CMT1)	// 1行コメントの初期化は次行先頭の判断
			//mode_ = MODE_NORM;	// でするので、ここは破棄
		break;

	case T_NONE:	// その他
		st += char(c);
		break;

	case T_SYM:		// 記号
		st += char(c);
		tok   = chkSymTok(c, T_SYM);
		break;

	case T_AMP:		// &
		st += "&amp;";
		tok   = chkSymTok(c, T_SYM);
		break;

	case T_LT:		// <
		st += "&lt;";
		tok   = chkSymTok(c, T_SYM);
		break;

	case T_GT:		// >
		st += "&gt;";
		tok   = chkSymTok(c, T_SYM);
		break;

	case T_SHARP:	// # 行頭チェックは別にやっているのでここでは、単なる記号 #,## のため
		st += char(c);
		tok   = chkSymTok(c, ((tokSw_[T_SHARP]) ? T_SHARP : T_SYM));
		break;

	case T_WQUOT:	// " 文字列
		st += "&quot;";
		if (isSym(c)) {
			if (tokSw_[tok]) {
				if (mode_ == MODE_NORM) {
					if (str_r_mode_ == 0)
						mode_ = MODE_STR;
					else
						mode_ = MODE_STR_R;
				} else if (mode_ == MODE_STR) {
					mode_ = MODE_NORM;
				} else if (mode_ == MODE_STR_R) {
					mode_ = MODE_NORM;
				} else {
					tok = T_NONE;
				}
			} else {
				tok   = chkSymTok(c, T_SYM);
			}
		} else {
			tok = T_NONE;
		}
		break;

	case T_SQUOT:	// ' 文字定数
		st += char(c);
		if (isSym(c)) {
			if (tokSw_[tok]) {
				if (mode_ == MODE_NORM) {
					mode_ = MODE_CVAL;
				} else if (mode_ == MODE_CVAL) {
					mode_ = MODE_NORM;
				} else {
					tok = T_NONE;
				}
			} else {
				tok   = chkSymTok(c, T_SYM);
			}
		} else {
			tok = T_NONE;
		}
		break;

	case T_BSQUOT:	// ` 文字列
		st += char(c);
		if (isSym(c)) {
			if (tokSw_[tok]) {
				if (mode_ == MODE_NORM) {
					mode_ = MODE_RSTR;
				} else if (mode_ == MODE_RSTR) {
					mode_ = MODE_NORM;
				} else {
					tok = T_NONE;
				}
			} else {
				tok   = chkSymTok(c, T_SYM);
			}
		} else {
			tok = T_NONE;
		}
		break;

	case T_ESC:	//	\ エスケープ
		if (tokSw_[T_ESC] && isSym(c)) {			// 設定がある場合
			chkGetYenEsc(st);
			// ここで処理しちゃったので、他に影響でないようにNONE
			tok = T_NONE;
		} else {	// 色設定がなけりゃ、通常の記号扱い
			tok   = chkSymTok(c, T_SYM);
		}
		break;

	case T_OVAL:		// 0で始まる
		k = peekC();				// 仮に次の一文字を取ってみる
		if (k < '0' || '9' < k)		// 0に続いて'0'～'9'以外ならば、普通の数値扱い
			tok = T_IVAL;			//   0のみや 0x 0. で始まる数値を想定
		// ケイゾク

	case	T_IVAL:		// 数値
		ungetC(c);
		tok = getVal(st, tok);
		tok   = (mode_ == MODE_NORM) ? tok : T_NONE;
		break;

	case	T_PERIOD:	// '.'
		k = peekC();			// 仮に次の一文字を取ってみる
		if (isSym(c)) {
			if (isdigit(k) && tokSw_[T_IVAL]) {
				ungetC(c);
				//st += char(c);
				tok = getVal(st, T_IVAL);
				tok = (mode_ == MODE_NORM) ? tok : T_NONE;
			} else {
				st += char(c);
				if (k == '*') {		// .*
					c = getC();
					st += char(c);
				}
				tok   = (mode_ == MODE_NORM) ? T_MEMB : T_NONE;
			}
		} else {
			st += char(c);
			tok = T_NONE;
		}
		break;

	case	T_MINUS:	// '-'
		k = peekC();			// 仮に次の一文字を取ってみる
		st += char(c);
		if (isSym(c)) {
			if (k == '>' && isSym(k)) {			// ->
				st += getC();
				k = peekC();					// 仮に次の一文字を取ってみる
				if (k == '*' && isSym(k)) {		// ->*
					c = getC();
					st += char(c);
				}
				tok   = (mode_ == MODE_NORM) ? T_MEMB : T_NONE;
			} else {
				tok   = chkSymTok(c, T_SYM);
			}
		} else {
			tok = T_NONE;
		}
		break;

	case	T_NAME:		// そのた、トークン等
		k = peekC();			// 仮に次の一文字を取ってみる
		if (c == 'L'
			 && ((k == '\'' && tokSw_[T_SQUOT])
			   || (k == '"'  && tokSw_[T_WQUOT]))
			 && mode_ == MODE_NORM)
		{
			// Lの直後に'か"があればwchar_t用文字列/定数として処理。
			// ※Lとの間に空白は置けない文法なんで気にしない^^;
			st += char(c);
			return getTok(st);
		}
		if (c == 'r'
			 && (/*(k == '\'' && tokSw_[T_BSQUOT])||*/
			     (k == '"' && tokSw_[T_WQUOT] && tokSw_[T_BSQUOT]))
			 && mode_ == MODE_NORM)
		{
			// rの直後に'か"があれば D言語のr"" r''として処理。
			// ※rとの間に空白は置けない文法なんで気にしない^^;
			st += char(c);
			str_r_mode_ = 1;
			tok_t tt = getTok(st);
			str_r_mode_ = 0;
			return tt;
		}
		ungetC(c);
		getName(st);				// 名前を取得
		//cout << "[" << st << "]" << endl;
		tok = chkName2Tok(st, tok);	// 名前から種別を取得
		break;

	case	T_SLA:		// / コメント等
		if (isSym(c) && lineCmtChr_ == 0) {
			tok = chkGetCmt(st);
		} else {
			st += char(c);
			tok   = chkSymTok(c, T_SYM);
		}
		break;

	case T_ASTA:		// *
		st += char(c);
		if (mode_ == MODE_BLKCMT && isSym(c)) {
			tok = T_NONE;
			c = getC();
			if (c == '/') {
				// */ でブロックコメントの終わりだった
				st += char(c);
				mode_ = MODE_NORM;
			} else {
				ungetC(c);
			}
		} else {
			tok   = chkSymTok(c, T_SYM);
		}
		break;

	case T_PLUS:		// +
		st += char(c);
		if (mode_ == MODE_BLKCMTD && isSym(c)) {
			tok = T_NONE;
			c = getC();
			if (c == '/') {
				// +/ でブロックコメントの終わりだった
				st += char(c);
				mode_ = MODE_NORM;
			} else {
				ungetC(c);
			}
		} else {
			tok   = chkSymTok(c, T_SYM);
		}
		break;

	default:
		//CERR << "[PRGERR]getTok\n";
		;
	}
	// 記号の、色設定がなかったら、通常文字扱い
	if (tok == T_SYM && tokSw_[tok] == false)
		tok = T_NONE;
	if (tok == T_MEMB && tokSw_[tok] == false)
		tok = T_NONE;
	// ８進数の、色設定がなかったら、通常の数値扱い
	if (tok == T_OVAL && tokSw_[tok] == false)
		tok = T_IVAL;
	// 数値の、色設定がなかったら、通常文字扱い
	if (tok == T_IVAL && tokSw_[tok] == false)
		tok = T_NONE;
	return tok;
}


/// \ エスケープシーケンスの処理
///
void ConvCpp2Htm::chkGetYenEsc(string &st)
{
	// ちょっと特別なんで、予め、色づけしちゃう方針
	int c = getC();
	ungetC(c);
	tok_t k = T_ESCX;
	if (tokSw_[k] == false)	// 設定されてなかったら通常と同じように同一視
		k = T_ESC;
	if (c == '\0' || c == '\n') {
		// 行末の\はちと特殊なんで色付け
		st += mrkBgn_[k];
		st += char('\\');
		st += mrkEnd_[k];
		escEolFlg_ = true;		// 行末が \ だったことを次行に教える
	} else if (mode_ == MODE_NORM) {
		// ソース中に現れた \ だった場合
		st += mrkBgn_[k];
		st += char('\\');
		getEsc(st);
		st += mrkEnd_[k];
	} else if (mode_ == MODE_STR || mode_ == MODE_CVAL /*|| mode_ == MODE_RSTR*/) {
		// 文字列, 文字定数中だった場合
		st += mrkBgn_[T_ESC];
		st += char('\\');
		getEsc(st);
		st += mrkEnd_[T_ESC];
	} else {
		// コメント中だった場合... 色つけんでええか、と
		// D言語での`文字列中`もあえて色つける必要なし、としとこう
		st += char('\\');
	}
}


/// '#' 単語のチェック.
/// @param name	取得した文字列を入れて返す
/// @return		トークン
ConvCpp2Htm::tok_t ConvCpp2Htm::chkGetSharpName(string &st)
{
	string	spc("");

	// #と単語の間には空白があっていいので、それを取得
	for (;;) {
		int c = getC();
		if (c == 0 || c > ' ') {
			ungetC(c);
			break;
		}
		spc += char(c);
	}
	getName(st);

	string nm('#'+st);
	st  = '#' + spc + st;
	return chkName2Tok(nm, T_NAME);
}


/// モードと名前から、今回のトークンを決定
/// @param name	チェックする名前
/// @return		トークン
ConvCpp2Htm::tok_t ConvCpp2Htm::chkName2Tok(const string &name, ConvCpp2Htm::tok_t tok)
{
	if (mode_ == MODE_NORM) {
	  #if 0	// . -> .* ->* の直後の名前は色づけ対象にしない場合は、この判定を復活させる
	  		// ……が、色がついたほうが結局問題把握のためにはよいんではないかと。
		if (bak_tok_ != T_MEMB)
	  #endif
		{
			WordTbl::iterator wd = wordTbl_.find(name);
			if (wd != wordTbl_.end())
				tok = wd->second;
		}
	} else {
		tok = T_NONE;
	}
	tok   = (mode_ == MODE_NORM) ? tok : T_NONE;
	return tok;
}


/// '/'で始まるコメントの処理
/// @param st	取得した文字列を入れて返す
/// @return		トークン
ConvCpp2Htm::tok_t ConvCpp2Htm::chkGetCmt(string &st)
{
	tok_t	tok;	//=T_SLA;
	int		c;

	st += char('/');
	if (mode_ == MODE_NORM) {
		c = getC();
		if (c == '/') {
			// //コメントだった
			st      += char(c);
			tok      = T_CMT;
			mode_    = MODE_CMT1;
			cmt_typ_ = CMT_NORM;
			c = getC();
			if (c == '/' && tokSw_[T_CMTJ]) {
				st += char(c);
				c = getC();
				if (c != '/') {
					// /// で doxygen(jdoc形式) としとく
					tok      = T_CMTJ;
					cmt_typ_ = CMT_DOXY;
				}
				ungetC(c);
			} else if (c == '!' && tokSw_[T_CMTJ]) {
				// //! で doxygen(qt形式) としとく
				st 	    += char(c);
				tok      = T_CMTJ;
				cmt_typ_ = CMT_DOXY;
			} else if (tokSw_[T_CMTEX1] && c == cmtExChr_[0] && c) {
				if (c)
					st  += char(c);
				tok      = T_CMTEX1;
				cmt_typ_ = CMT_EX1;
			} else if (tokSw_[T_CMTEX2] && c == cmtExChr_[1] && c) {
				if (c)
					st  += char(c);
				tok      = T_CMTEX2;
				cmt_typ_ = CMT_EX2;
			} else if ((c != ' ' && c != '\t' && c != '\0' && c != '\n') && tokSw_[T_CMTK]) {
				// // の直後に空白がない場合
				if (c)
					st += char(c);
				tok      = T_CMTK;
				cmt_typ_ = CMT_NOSPC;
			} else {
				ungetC(c);
			}
		} else if (c == '*') {
			// /* コメントだった
			st += char(c);
			tok      = T_CMT;
			mode_    = MODE_BLKCMT;
			cmt_typ_ = CMT_NORM;
			c = getC();
			if (c == '*' && tokSw_[T_CMTJ]) {
				st += char(c);
				c = getC();
				if (c != '*') {
					// /** で doxygen(jdoc形式) としとく
					tok      = T_CMTJ;
					cmt_typ_ = CMT_DOXY;
				}
				ungetC(c);
			} else if (c == '!' && tokSw_[T_CMTJ]) {
				// /*! で doxygen(qt形式) としとく
				st += char(c);
				tok      = T_CMTJ;
				cmt_typ_ = CMT_DOXY;
			} else if ((c != ' ' && c != '\t' && c != '\0' && c != '\n') && tokSw_[T_CMTK]) {
				// /* の直後に空白がない場合
				if (c)
					st += char(c);
				tok      = T_CMTK;
				cmt_typ_ = CMT_NOSPC;
			} else {
				ungetC(c);
			}
		} else if (c == '+' && tokSw_[T_CMTD]) {
			// /+ コメントだった
			st += char(c);
			tok      = T_CMTD;
			mode_    = MODE_BLKCMTD;
			cmt_typ_ = CMT_D;
			c = getC();
			if (c == '+' && tokSw_[T_CMTJ]) {
				st += char(c);
				c = getC();
				if (c != '+') {
					// /++ で嘘だけど doxygen扱いにする
					tok      = T_CMTJ;
					cmt_typ_ = CMT_DOXY;
				}
				ungetC(c);
			} else if (c == '!' && tokSw_[T_CMTJ]) {
				// /+! で嘘だけど doxygen扱いにする
				st += char(c);
				tok      = T_CMTJ;
				cmt_typ_ = CMT_DOXY;
			} else {
				ungetC(c);
			}
		} else {
			ungetC(c);
			tok = T_SYM;
		}
	} else {
		tok = T_NONE;
	}
	return tok;
}


/// 単語の取得(全角文字列とかもまとめて)
/// @param st	取得した文字列を入れて返す
void ConvCpp2Htm::getName(std::string &st)
{
	for (;;) {
		int c = getC();
		tok_t tok = chTypTbl_[c];
		if (tok != T_NAME && tok != T_IVAL && tok != T_OVAL /*&& tok != T_SHARP*/) {
			ungetC(c);
			break;
		}
	  #if 1
		if (useUtf8_) {
			if (c < 0x80) {
				st += char(c);
			} else {
				int n = (c < 0xC0) ? 1-1
					  : (c < 0xE0) ? 2-1
					  : (c < 0xF0) ? 3-1
					  : (c < 0xF8) ? 4-1
					  : (c < 0xFC) ? 5-1
					  :              6-1;
				st += char(c);
				for (int i = 0; i < n; i++) {
					c = getC();
					if (c == 0) {
						//cout << "bad utf8!\n";
						break;
					}
					st += char(c);
				}
			}
		} else {
			if (ISKANJI(c)) {
				int c2 = getC();
				if (c2 == 0) {
					break;
				}
				st += char(c);
				c   = c2;
			}
			st += char(c);
		}
	  #endif
	}
}


/// 数値を処理.
/// 判定はわざと甘くしてある。数字で始まり名前文字が続く間はそれと見做す。
/// @param st	取得した文字列を入れて返す
/// @param tok	現状のtok
/// @return		解析結果、変更となったtok
ConvCpp2Htm::tok_t ConvCpp2Htm::getVal(std::string &st, ConvCpp2Htm::tok_t tok)
{
	int b = 0;

	for (;;) {
		int c = getC();
		tok_t t = chTypTbl_[c];
		if (c == '.') {
			t = T_IVAL;
			tok = t;
		}
		if (c == 'x' || c == 'X')
			tok = T_IVAL;
		if ((b == 'e' || b == 'E' || b == 'p' || b == 'P') && (c == '+' || c == '-')) {
			t = T_IVAL;
			tok = t;
		}
		if (c >= 0x7F || (t != T_NAME && t != T_IVAL && t != T_OVAL)) {
			ungetC(c);
			break;
		}
		st += char(c);
		b = c;
	}
	return tok;
}


/// \エスケープ文字の処理
/// @param st	取得した文字列を入れて返す
void ConvCpp2Htm::getEsc(std::string &st)
{
	int l, i;
	int c = getC();
	switch (c) {
	case 'a':		// abvntfr\'"
	case 'b':
	case 'v':
	case 'n':
	case 't':
	case 'f':
	case 'r':
	case '\\':
	case '\'':
	case '"':
	case '?':
		st += char(c);
		break;

	case 'x':
	case 'u':
	case 'U':
		l = (c == 'x') ? 2 : (c == 'u') ? 4 : 8;
		for (i = 0; i < l; i++) {
			c = getC();
			if (isxdigit(c)) {
				st += char(c);
			} else {
				ungetC(c);
				break;
			}
		}
		break;

  #if 0	// perl関係
	case 'e':
	case 'l':
	case 'u':
	case 'L':
	case 'U':
	case 'E':
	//case 'c':
		st += char(c);
		break;
  #endif

	default:
		if (isdigit(c)) {
			st += char(c);
			for (i = 0; i < 2; i++) {
				c = getC();
				if (isdigit(c)) {
					st += char(c);
				} else {
					ungetC(c);
					break;
				}
			}
		} else {
			// アンノウンは、色つけない、より、影響を受けることを
			// 示すために、色をつけたほうがよさげ
			//ungetC(c);
			st += char(c);
		}
		break;
	}
}



/// 空白を取得
///
void ConvCpp2Htm::getSpc(string &st)
{
	int c;
	for (;;) {
		c = getC();
		if (c == 0 || c > ' ')
			break;
		st += char(c);
	}
	ungetC(c);
}


// ------------------------------------------------------------
// 初期化関係

/// コンストラクタ
///
ConvCpp2Htm::ConvCpp2Htm()
{
	srcPtr_     = 0;
	mode_		= MODE_NORM;
	cur_tok_	= T_NONE;
	cmt_typ_	= CMT_NORM;
	useUtf8_	= false;
	escEolFlg_	= false;
	str_r_mode_ = false;
	lineCmtChr_ = 0;
	tokStr_.clear();
	tokStr_.resize(0x1000);
	initChTop(std::string(""), std::string(""));
	//initTbl();
	initMrkBgnEnd();
	for (int i = 0; i < T_NUM;i++)
		tokSw_[i] = false;
}


/// 文字判別用テーブルの初期化
/// @param	symChrs		記号扱いの文字
/// @pram 	nameSymChrs	名前扱いする記号文字.
void ConvCpp2Htm::initChTop(const std::string &symChrs, const std::string &nameSyms)
{
	int i;

	symChrs_  = symChrs;
	nameSyms_ = nameSyms;

	chTypTbl_[0] = T_ZERO;
	for (i = 1; i < 0x100; i++)
		chTypTbl_[i] = T_NONE;

	for (i = '0'; i <= '9'; i++)
		chTypTbl_[i] = T_IVAL;
	chTypTbl_['0']   = T_OVAL;
	//
	for (i = 'A'; i <= 'Z'; i++)
		chTypTbl_[i] = T_NAME;

	for (i = 'a'; i <= 'z'; i++)
		chTypTbl_[i] = T_NAME;

	//if (useUtf8_) {
		for (i = 0x80; i <= 0xFF; i++)	// utf8の場合...
			chTypTbl_[i] = T_NAME;
	//} else {
		//for (i = 0x81; i <= 0xFC; i++)	// sjisの場合...
			//chTypTbl_[i] = T_NAME;
	//}
	//chTypTbl_['_']  = T_NAME;
	//chTypTbl_['$']  = T_NAME;
	//chTypTbl_['@']  = T_NAME;
	for (i = 0; i < int(nameSyms.size()); i++) {
		chTypTbl_[nameSyms[i]] = T_NAME;
	}
	// 指定された記号文字を登録
	for (i = 0; i < int(symChrs.size()); i++) {
		chTypTbl_[symChrs[i]] = T_SYM;
	}
	//
	chTypTbl_['&']  = T_AMP;
	chTypTbl_['<']  = T_LT;
	chTypTbl_['>']  = T_GT;
	chTypTbl_['"']  = T_WQUOT;
	chTypTbl_['\''] = T_SQUOT;
	chTypTbl_['`']  = T_BSQUOT;
	chTypTbl_['\\'] = T_ESC;
	chTypTbl_['/']  = T_SLA;
	chTypTbl_['*']  = T_ASTA;
	chTypTbl_['#']  = T_SHARP;
	chTypTbl_['.']  = T_PERIOD;
	chTypTbl_['-']  = T_MINUS;
	chTypTbl_['+']  = T_PLUS;
}



/// HTML での色付けのためのテーブルを準備
///
void ConvCpp2Htm::initMrkBgnEnd()
{
	mrkBgn_.clear();
	mrkEnd_.clear();
	for (int i = T_NONE; i <= T_NUM; i++) {
		mrkBgn_[tok_t(i)] = string("");
		mrkEnd_[tok_t(i)] = string("");
	}
	for (mrkTbl_t *m = mrkTbl_; m->bgn; m++) {
		mrkBgn_[m->tok] = m->bgn;
		mrkEnd_[m->tok] = m->end;
	}
}


/// 色/フォントを設定するHTML文字列
ConvCpp2Htm::mrkTbl_t	ConvCpp2Htm::mrkTbl_[] = {
	{T_SYM,		"<span class=C_SY>",		"</span>"},
	{T_WQUOT,	"<span class=C_STR>",		"</span>"},
	{T_SQUOT,	"<span class=C_CH>",		"</span>"},
	{T_BSQUOT,	"<span class=C_RSTR>",		"</span>"},
	{T_ESC,		"<span class=C_ESC>",		"</span>"},
	{T_ESCX,	"<span class=C_ESCX>",		"</span>"},
	{T_IVAL,	"<span class=C_VAL>",		"</span>"},
	{T_OVAL,	"<span class=C_OCT>",		"</span>"},
	{T_CMT,		"<span class=C_CMT>",		"</span>"},
	{T_CMTJ,	"<span class=C_CMTJ>",		"</span>"},
	{T_CMTK,	"<span class=C_CMTK>",		"</span>"},
	{T_CMTD,	"<span class=C_CMTD>",		"</span>"},
	{T_CMTEX1,	"<span class=C_CMTEX1>",	"</span>"},
	{T_CMTEX2,	"<span class=C_CMTEX2>",	"</span>"},
	{T_SHARP,	"<strong class=C_SH>",		"</strong>"},
	{T_WORD1,	"<strong class=C_W1>",		"</strong>"},
	{T_WORD2,	"<strong class=C_W2>",		"</strong>"},
	{T_WORD3,	"<strong class=C_W3>",		"</strong>"},
	{T_WORD4,	"<strong class=C_W4>",		"</strong>"},
	{T_WORD5,	"<span class=C_W5>",		"</span>"},
	{T_WORD6,	"<span class=C_W6>",		"</span>"},
	{T_WORD7,	"<span class=C_W7>",		"</span>"},
	{T_WORD8,	"<span class=C_W8>",		"</span>"},
	{T_WORD9,	"<span class=C_W9>",		"</span>"},
	{T_NONE,	0,	0},
};



/// 直後のtokが、NONEのとき、同一視するか否かのチェックテーブル
bool ConvCpp2Htm::nextTokNoneOkTbl[T_NUM+1] = {
	true,		// T_NONE
	false,		// T_ZERO
	false,		// T_SYM
	true,		// T_WQUOT
	true,		// T_SQUOT
	true,		// T_BSQUOT
	false,		// T_ESC
	false,		// T_ESCX
	false,		// T_IVAL
	false,		// T_OVAL
	true,		// T_CMT
	true,		// T_CMTJ
	true,		// T_CMTK
	true,		// T_CMTD
	true,		// T_CMTEX1
	true,		// T_CMTEX2
	false,		// T_SHARP
	false,		// T_WORD1
	false,		// T_WORD2
	false,		// T_WORD3
	false,		// T_WORD4
	false,		// T_WORD5
	false,		// T_WORD6
	false,		// T_WORD7
	false,		// T_WORD8
	false,		// T_WORD9
	false,		// T_NAME
	false,		// T_MEMB
	false,		// T_LF
	false,		// T_SLA
	false,		// T_ASTA
	false,		// T_AMP
	false,		// T_LT
	false,		// T_GT
	false,		// T_PERIOD
	false,		// T_MINUS
	false,		// T_PLUS
	false,		// T_NUM
};


/* ---------------------------------------------------------------------------
 *  メモ：
 *  	2003-07-23 一応、それなりなものが完成。結構気分よし。
 *		でソース整理してて、リコンパイルしたらこのファイルがないとエラー.
 *		.bak やらテストで使ったこのファイルのコピーなども一律削除した直後で、
 *		ファイル名中のhtmの部分を拡張子と見間違えて削除した模様(T T)
 *		全く片鱗も残ってなかった...で、結局、思い出して作り直し。
 *		出社直前一時間で大筋書き出して(結構覚えてるもんだ)帰宅後デバッグで復元.
 *		2003-08-08 \0を含む \+8進数において、最初の数字を、出力し忘れてたのを修正
 *		2003-08-09 記号文字の設定を外部からの設定に変更.
 *		トークン種別の判定のon/off を一通り可能に。
 *		c形式以外の一行コメントの設定を可能に(手抜き版)
 *		2003-08-18 '.'を記号文字にしていないとき、出力から欠けてしまったのを修正
 */
