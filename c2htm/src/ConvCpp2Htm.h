/**
 *  @file   ConvCpp2Htm.h
 *  @brief  c/c++ソースをhtml化
 *
 *  @author Masahi KITAMURA (tenka@6809.net)
 *  @date   2003-07-21 ～
 */

#ifndef CONVCPP2HTM_H
#define CONVCPP2HTM_H
#include <string>
#include <map>
#include <assert.h>


/// c/c++ ソース文字列をhtmlに変換する
class ConvCpp2Htm {
public:
    enum tok_t {
    	T_NONE,     ///< 役目なし...だけでなく、前のトークンに同じ、の役として使ってしまってる。
    	T_ZERO,     ///< '\0' だが実質改行コード扱い
    	T_SYM,	    ///< その他の記号
    	T_WQUOT,    ///< " 文字列
    	T_SQUOT,    ///< ' 文字定数
    	T_BSQUOT,   ///< ` 文字定数(for D言語)
    	T_ESC,	    ///< 文字列中の \エスケープ
    	T_ESCX,     ///< ソース中の \エスケープ
    	T_IVAL,     ///< 数値
    	T_OVAL,     ///< 8進数
    	T_CMT,	    ///< //, /* コメント
    	T_CMTJ,     ///< ///, /**, //!, /*! doxygenコメント
    	T_CMTK,     ///< // コメント で//の直後に空白(改行含む)がない場合
    	T_CMTD,     ///< D言語の /+ +/ (ソースのコメントアウト向け)
    	T_CMTEX1,   ///< //x 等のユーザー定義で色違いにする//コメント その1
    	T_CMTEX2,   ///< //* 等のユーザー定義で色違いにする//コメント その2
    	T_SHARP,    ///< #.. だが、とりあえず通常の単語扱いで処理
    	T_WORD1,    ///< 登録単語1
    	T_WORD2,    ///< 登録単語2
    	T_WORD3,    ///< 登録単語3
    	T_WORD4,    ///< 登録単語4
    	T_WORD5,    ///< 登録単語5
    	T_WORD6,    ///< 登録単語6
    	T_WORD7,    ///< 登録単語7
    	T_WORD8,    ///< 登録単語8
    	T_WORD9,    ///< 登録単語9
    	//T_HT,     ///< ハーフトーン扱い

    	T_NAME,     ///< 名前(になりそうな文字)
    	T_MEMB,     ///< メンバ名の前に来る記号 . -> .* ->*
    	T_LF,	    ///< '\n' だが出現しないかも

    	// 解析時の判断用
    	T_SLA,	    ///< '/'
    	T_ASTA,     ///< '*'
    	T_AMP,	    ///< '&'
    	T_LT,	    ///< '<'
    	T_GT,	    ///< '>'
    	T_PERIOD,   ///< '.'
    	T_MINUS,    ///< '-'
    	T_PLUS,     ///< '+'
    	//T_L,
    	T_NUM,	    ///< トークン種別の数( 範囲外を示すとしても利用)
    };

    ConvCpp2Htm();
    ~ConvCpp2Htm() {};

    /// 文字判別用テーブルの初期化. symChrs:記号扱いの文字. nameSymChrs:名前扱いの記号文字.
    void initChTop(const std::string &symChrs, const std::string &nameSymChrs);

    /// 内部状態をクリア
    //void clear() {mode_ = MODE_NORM;}

    /// src のc/c++テキストをhtml に変換してdstに入れて返す
    bool convLine(std::string &dst, const char *src);

    /// SH,W1～W9 の単語登録
    void defWord(const std::string &name, tok_t tok) {wordTbl_[name] = tok;}

    /// トークン種類別の色づけのon/off スイッチ設定
    void useTokGroupSw(tok_t tok, bool sw) {tokSw_[tok] = sw;}

    /// // /* のコメントを廃止し、一行コメントを c で始まるモノに変更
    void setLineCmtChr(int c) { lineCmtChr_ = c;}

    /// 入力文字列として utf8 を使う
    void setUtf8(bool sw) {useUtf8_ = sw;}

    void setExCmtChr(int n, int c) {
    	assert(n >= 0 && n <= 1);
    	cmtExChr_[n] = c;
    }

    void setExCmtChr(int n, const char *s) {
    	assert(s && n >= 0 && n <= 1);
    	cmtExChr_[n] = *s;
    	//x printf("cmtEx%d:%c\n", n+1, *s);
    }

private:
    /// html出力用のテーブルの初期化
    void initMrkBgnEnd();

    /// 予約単語の登録(初期化)
    void initTbl();

    /// 今回のソース行文字列の設定
    void setSrcPtr(const char *p)   {srcPtr_ = (const unsigned char *)p;}

    /// 1文字取得
    int  getC()     	    	    {int c = *srcPtr_; if (c) srcPtr_++; return c;}

    /// 仮で次の一文字を取ってみる。ちょと反則技だが。
    int  peekC()    	    	    {int c = *srcPtr_; return c;}
    int  peekNextC()	    	    {int c = srcPtr_[0] ? srcPtr_[1] : 0; return c;}

    /// 1文字返却
    void ungetC(int c)	    	    {if (c) --srcPtr_;}     // 手抜きなんで、引数は無視

    /// 空白を取得
    void getSpc(std::string &st);

    /// 名前文字列を取得
    void getName(std::string &st);

    /// 数値文字列を取得
    tok_t getVal(std::string &st, tok_t tok);

    /// \エスケープ文字列を取得
    void getEsc(std::string &st);

    /// \ エスケープシーケンスの処理
    void chkGetYenEsc(std::string &st);

    /// モードと名前から、今回のトークンを決定
    tok_t chkName2Tok(const std::string &name, tok_t tok);

    /// '#' 単語のチェック.
    tok_t chkGetSharpName(std::string &st);

    /// トークン文字列を取得
    tok_t getTok(std::string &st);

    /// '/'で始まるコメントの処理
    tok_t chkGetCmt(std::string &st);

    /// 行頭でのtokの辻褄会わせ
    void chkSetMode2CurTok();

    /// 文字が記号かどうか
    bool isSym(char c) {
    	size_t n = symChrs_.find_first_of(c);
    	return n != std::string::npos;
    }

    /// 記号文字用にモード等を考慮して、tokを調整
    tok_t chkSymTok(int c, tok_t tok) {
    	tok  = (isSym(c) && mode_ == MODE_NORM) ? tok : T_NONE;
    	if (c == lineCmtChr_ && tok != T_NONE) {
    	    // 1行コメント文字だった場合
    	    tok      = T_CMT;
    	    mode_    = MODE_CMT1;
    	    cmt_typ_ = CMT_NORM;
    	}
    	return tok;
    }


private:
   #if 0    // コンフィグ未対応時の初期化用
    struct  ini_cword_t {
    	const char *name;
    	tok_t	    tok;
    };
    static ini_cword_t	ini_cwords_[];
   #endif
    struct  mrkTbl_t {
    	tok_t	    tok;
    	const char *bgn;
    	const char *end;
    };
    /// html出力でキーワードの前後に着ける<strong class=...></strong>などの文字列設定
    static mrkTbl_t 	mrkTbl_[];
    static bool     	nextTokNoneOkTbl[T_NUM+1];

    /// 現在処理中のソースの状態.
    enum    mode_t {
    	MODE_NORM,  	///< 通常のcプログラムテキスト
    	MODE_STR,   	///< "文字列"中
    	MODE_CVAL,  	///< '文字'定数中
    	MODE_RSTR,  	///< `文字列`中
    	MODE_STR_R, 	///< r"文字列"中
    	MODE_CMT1,  	///< // １行コメント
    	MODE_BLKCMT,	///< /* ブロックコメント
    	MODE_BLKCMTD,	///< /+ ブロックコメント
    };
    mode_t  	    	mode_;	    	    ///< 状態

    /// コメント・タイプ
    enum cmt_typ_t {
    	CMT_NORM,   	///< 通常のコメント.
    	CMT_DOXY,   	///< doxygen形式のコメント
    	CMT_NOSPC,  	///< "// "や"/* "のようにコメント直後に空白がない場合
    	CMT_D,	    	///< D言語の /+ +/ コメント
    	CMT_EX1,    	///< ユーザー拡張コメント
    	CMT_EX2,    	///< ユーザー拡張コメント
    };
    cmt_typ_t	    	cmt_typ_;   	    ///< コメント・タイプ

    const unsigned char *srcPtr_;   	    ///< 1文字取得でのポインタ
    std::string     	tokStr_;    	    ///< トークン取得用のバッファ
    bool    	    	escEolFlg_; 	    ///< 最後が \ だったかのフラグ
    bool    	    	tokSw_[T_NUM];	    ///< 特定のトークングループの使用のon/off
    tok_t   	    	cur_tok_;   	    ///< 現在のトークン
    tok_t   	    	bak_tok_;
    int     	    	lineCmtChr_;	    ///< 1行コメントにする文字
    tok_t   	    	chTypTbl_[256];     ///< 先頭１文字の判定用テーブル
    std::string     	symChrs_;   	    ///< 記号扱いにする記号文字の集まり
    std::string     	nameSyms_;  	    ///< 名前に含める記号文字
    bool    	    	useUtf8_;   	    ///< 入力が UTF8 か否か
    bool    	    	str_r_mode_;	    ///< r"..." チェック用
    char    	    	cmtExChr_[2];	    ///< ユーザー拡張なコメントの文字

    typedef std::map<std::string, tok_t >   WordTbl;
    WordTbl 	    	    	    wordTbl_;	///< 単語帳
    std::map<tok_t, std::string>    mrkBgn_;	///< html出力での開始文字列
    std::map<tok_t, std::string>    mrkEnd_;	///< html出力での終了文字列
};



#endif	// CONVCPP2HTM_H
