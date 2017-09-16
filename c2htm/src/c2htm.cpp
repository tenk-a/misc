/**
 *  @file   c2htm.cpp
 *  @brief  c/c++ソースを色つきhtmlに変換するコマンドラインツール
 *
 *  @author Masahi KITAMURA (tenka@6809.net)
 *  @date   2003-07-21 .. 2017
 */

#include <cstring>

#include <fstream>
#include <iostream>
#include <Iomanip>
#include <string>
#include <vector>
#include <set>
#include <stdlib.h>

#include "cmisc.h"
#include "ConvCpp2Htm.h"
#include "cfgfile.h"

#define CERR    cout
using namespace CMISC;
using namespace std;


/** 説明表示＆終了
 */
void usage(void)
{
    cout<< "c2htm [-opts] file(s)    // v1.73  " __DATE__  "  writen by M.Kitamura\n"
           "     https://github.com/tenk-a/misc/tree/master/c2htm\n"
           " c/c++ source text を色つきhtmlに変換(html4.0)\n"
           " 定義ファイルとして この .exe パス名を .cfg にしたものを読込む\n"
           " オプション:\n"
           "  :NAME         exeと同じフォルダにある c2htm-NAME.cfg を読込む\n"
           "  -c[CFGFILE]   デフォルト以外の定義ファイルを読込む\n"
           "  -o[FILE]      出力ファイル名\n"
           "  -n[N[:S]]     行番号を付加. N=桁数(省略:5).S:行番号と本文の敷居文字列(" ")\n"
           "  -n-           行番号を外す\n"
           "  -t[N]         タブサイズの設定(省略:4)\n"
           "  -m[N]         ヘッダ＆フッダ組の番号\n"
           "  -ql<STR>      行末にSTRのある行のみ色変換、以外はハーフトーン扱い\n"
           "  -qr<STR>      -ql とほぼ同様だが、STR自体は削除して出力\n"
           "  -u[-]         UTF8 で入力する / -u- しない(Multi Byte Char)\n"
           "  -s            標準入出力を行う\n"
           "  -gencfg       デフォルトcfg定義を標準出力\n" ;
    exit(1);
}


// ---------------------------------------------------------------------------

/** オプション要素の管理 */
class Opts {
public:
    Opts(const char *appNm="")
        : outName_(), cfgName_(), tabSz_(-1), lnoClm_(-1), lnoSep_()
        , stdio_(0), cfgOut_(0), hdrFtrMd_(0), markStr_(), markMd_(0), useUtf8_(-1)
    {
        cfgName_ = string(appNm);
        fname_chgExt(cfgName_, "cfg");
    }
    ~Opts() {}
    void get(const char *p);

public:
    string  outName_;       ///< 出力名(1回きり)
    string  cfgName_;       ///< コンフィグファイル名.
    int     tabSz_;         ///< タブ・サイズ.
    int     lnoClm_;        ///< 0以上:行番号をつける.値は桁数 -1:つけない.
    string  lnoSep_;        ///< 行番号がつく時の、番号とソース行との間の文字.普通" "
    int     stdio_;         ///< 1:"-s"で標準入出力可能にする. 0:標準入出力しない.
    int     cfgOut_;        ///< デフォルトのcfgの内容を出力.
    int     hdrFtrMd_;      ///< 1:ヘッダフッダの出力を抑止.
    string  markStr_;       ///< 行末にこの文字列があれば色変換、無かればハーフトーン.
    int     markMd_;        ///< markStr_ を 0:未使用 1:そのまま出力 2:削除して出力.
    int     useUtf8_;       ///< 入力ソースが utf8か否か.
};



/** '-'で始まるコマンドラインオプションの解析.
 */
void Opts::get(const char *arg)
{
    const char *p = arg + 1;
    int c = *p++;
    c = toupper(c);
    switch (c) {
    case 'O':
        outName_ = string(p);
        break;
    case 'C':
        cfgName_ = string(p);
        break;
    case 'T':
        tabSz_ = strtol(p,0,0);
        break;
    case 'S':
        stdio_   = (*p != '-');
        break;
    case 'G':
        cfgOut_ = stricmp(p-1, "gencfg") == 0;
        break;
    case 'M':
        hdrFtrMd_ = strtol(p,0,0);
        break;
    case 'Q':
        c = *p++;
        c = toupper(c);
        if (c == 'L') {
            markMd_  = 1;
            markStr_ = string(p);
            if (markStr_.empty())
                goto OPTS_ERR;
        } else if (c == 'R') {
            markMd_  = 2;
            markStr_ = string(p);
            if (markStr_.empty())
                goto OPTS_ERR;
        } else {
            goto OPTS_ERR;
        }
        break;
    case 'N':
        lnoClm_ = 5;
        if (*p == 0)
            break;
        if (*p == '-') {
            lnoClm_ = 0;
            p++;
        } else {
            lnoClm_ = strtol(p,(char **)&p,0);
        }
        if (*p == ':') {
            p++;
            if (strlen(p) > 0)
                lnoSep_ = string(p);
        }
        break;
    case 'U':
        useUtf8_ = (*p != '-');
        break;
    case '?':
        ::usage();
        break;

    default:
  OPTS_ERR:
        CERR << arg << " は知らないオプションです\n";
        exit(1);
    }
}



// ---------------------------------------------------------------------------

/** c/c++ソース -> html 変換の管理 */
class Conv {
public:
    Conv()  : hdrFtrMd_(0)
            , tabSz_(0)
            , useUtf8_(false)
            , lnoClm_(0)
            , lnoSep_()
            , css_()
            , cfgName_()
            , dfltCfgName_()
            , symChrs_()
            , nameSyms_()
            , markStr_()
            , markMd_(0)
            , cpp2htm_()
            , cpp2txt_()
    {
        for (int i = 0; i < HF_NUM; i++) {
            header_[i] = "";
            footer_[i] = "";
        }
        for (int j = 0; j < int(ConvCpp2Htm::T_NUM); j++) {
            cssStr_[j] = "";
            cssStrSw_[j] = false;
        }
    }
    ~Conv() {}

    /// デフォルトのコンフィグファイル名を設定.
    void setDfltCfgName(string &cfgname) {dfltCfgName_ = cfgname;}

    /// コンフィグファイルをロード済か否か.
    bool isCfgLoaded() const {return cfgName_.empty() == false;}

    /// コンフィグファイルを読み込む.
    bool cfgLoad(const string &cfgFname);

    /// ヘッダフッダを出力するか否かを設定.
    void setHdrFtrMode(int md) {
        if (md < 0 || md >= HF_NUM) {
            CERR << "-m[N]の N は 0～" << (HF_NUM-1) << " の値にしてください\n";
            md = 0;
        }
        hdrFtrMd_ = md;
    }

    /// タブサイズを設定する.
    void setTabSz(int sz)   {tabSz_  = sz;}

    /// 行番号の桁数を設定する.
    void setLnoClm(int clm) {lnoClm_ = clm;}

    /// 行番号とテキストの区切り文字列を設定する.
    void setLnoSep(const string &sep) {lnoSep_ = sep;}

    /// マーク行の扱いを設定.
    void setMarkLine(int mode, string &str) {markMd_ = mode; markStr_ = str;}

    /// ソースがutf8か否か.
    void setUtf8(int sw) { useUtf8_ = sw != 0; cpp2htm_.setUtf8(sw != 0);}

    /// 変換を実行する.
    int  run(const char *name, const char *outName);

    static const char dfltCfgData_[];       ///< デフォルトのコンフィグデータ.


private:
    typedef ConvCpp2Htm::tok_t  tok_t;

    /// tokに対応したcss文字列が設定されているか否か.
    bool    isCssStr(int tok) { return (cssStr_[tok].empty() == false);}

    /// コンフィグで、tokグループに属する単語を登録.
    void    defWord(CfgFile &cf, const char *tag, tok_t tok);

    /// ヘッダやフッタ中の "*fn*" 文字列を実際のファイル名に置き換えるのに使う.
    void    replaceFn(const char *name, string &hdr);

    /// マーク行のチェック
    bool    checkMarkLine(char *buf);


private:
    enum {      HF_NUM = 4 };
    static const char *c_tag_[];                ///< コンフィグ中の名前.
    int         hdrFtrMd_;                      ///< ヘッダフッダの出力の有無.
    int         tabSz_;                         ///< タブサイズ.
    bool        useUtf8_;                       ///< ソースがUTF8か否か.
    int         lnoClm_;                        ///< 1以上:行番号を付加.値は桁数 0以下:つけない.
    string      lnoSep_;                        ///< 行番号をつける場合の、番号とテキストの間の文字列.普通" "
    string      header_[HF_NUM];                ///< html として吐き出す先頭.
    string      footer_[HF_NUM];                ///< html として吐き出す最後.
    string      css_;                           ///< html として吐き出す色付けの設定.
    string      cfgName_;                       ///< コンフィグファイルの名前.
    string      dfltCfgName_;                   ///< デフォルトのコンフィグファイルの名前.
    bool        cssStrSw_[int(ConvCpp2Htm::T_NUM)]; ///< C_定義があるかどうか.
    string      cssStr_[int(ConvCpp2Htm::T_NUM)];
    string      symChrs_;
    string      nameSyms_;
    string      markStr_;                       ///< 行末にこの文字列があれば色変換、無かればハーフトーン.
    int         markMd_;                        ///< markStr_ を 0:未使用 1:そのまま出力 2:削除して出力.
    ConvCpp2Htm cpp2htm_;                       ///< c/c++ -> html の1行変換.
    ConvCpp2Htm cpp2txt_;                       ///< マーク行外をハーフトーンにする場合の１行変換.
};



/** 色付けする種類の名前 */
const char *Conv::c_tag_[] = {
    "",
    "C_LNO", "C_SY",  "C_STR", "C_CH",   "C_RSTR",
    "C_ESC", "C_ESCX","C_VAL", "C_OCT",
    "C_CMT", "C_CMTJ", "C_CMTK","C_CMTD", "C_CMTEX1", "C_CMTEX2",
    "C_SH",  "C_W1",  "C_W2",  "C_W3",   "C_W4",
    "C_W5",  "C_W6",  "C_W7",  "C_W8",   "C_W9",
    "C_HT",
};


/** デフォルトのコンフィグデータ */
const char Conv::dfltCfgData_[] = {
    #include "c2htm.cfg.cc"
};


/** コンフィグファイルを読み込む
 *  読み込みは１度のみしか、しちゃダメ
 */
bool Conv::cfgLoad(const string &cfgFname)
{
    CfgFile cf;

    if (!cfgName_.empty()) {
        CERR << "cfgが複数指定されています" << endl;
        return false;
    }

    int rc = cf.init(cfgFname);
    if (rc == 0) {
        //
        CERR << "コンフィグファイル " << cfgFname << " が見つかりません。\n";
        if (cfgFname == dfltCfgName_) {
            CERR << "最低限 exeと同じフォルダに " << fname_getBase(dfltCfgName_.c_str())
                 << " を置いてください。\n";
        }
        CERR << "今回は内臓のデフォルト設定で変換します。\n"
                "(内臓デフォルトは c2htm -gencfg とすれば確認できます)\n";
        // コンフィグファイルが無かったら、デフォルトのテーブルを使う.
        rc = cf.init(dfltCfgData_, 1);
        if (rc == 0) {
            return false;
        }
    }

    // オプション要素の取得.
    tabSz_  = cf.getVal("TAB");
    lnoClm_ = cf.getVal("LNO");
    cf.getStr(lnoSep_, "LNOSEP");

    setUtf8(cf.getVal("UTF8"));

    // 記号文字の扱いに関する取得.
    cf.getStr(symChrs_, "SYM");
    cf.getStr(nameSyms_, "NAMESYM");
    cpp2htm_.initChTop(symChrs_, nameSyms_);
    {   // // /* コメントをやめ、別の1行コメントを行う場合.
        string tmpStr;
        rc = cf.getStr(tmpStr, "LINECMT");
        if (rc) {
            cpp2htm_.setLineCmtChr(tmpStr[0]);
        }
    }

    // ユーザー拡張１行コメントの文字.
    string cmtExStr;
    cf.getStr(cmtExStr, "CMTEX1");
    cpp2htm_.setExCmtChr(0, cmtExStr.c_str());
    cmtExStr.clear();
    cf.getStr(cmtExStr, "CMTEX2");
    cpp2htm_.setExCmtChr(1, cmtExStr.c_str());

    // 種類ごとの設定.
    css_.clear();
    for (int i = ConvCpp2Htm::T_ZERO; i <= ConvCpp2Htm::T_WORD9; i++) {
        tok_t   tok = tok_t(i);

        // 設定の有無(on/off)と内容を取得.
        cssStr_[tok].clear();
        cssStrSw_[tok] = cf.getStr(cssStr_[tok], c_tag_[i]);
        if (cssStrSw_[tok] && cssStr_[tok].empty())
            cssStrSw_[tok] = false;
        cpp2htm_.useTokGroupSw(tok, cssStrSw_[tok]);

        // html出力用の定義を作成.
        if (isCssStr(tok)) {
            css_ += ".";
            css_ += c_tag_[tok];
            css_ += "{";
            css_ += cssStr_[tok];
            css_ += "}\n";
        }
    }
    {   // ハーフトーン設定は別枠だが、ここでやっちゃう.
        string htStr("");
        if (cf.getStr(htStr, "C_HT") && !htStr.empty())
            css_ += ".C_HT{" + htStr + "}\n";
    }

    // 色をつける単語の取得＆登録.
    defWord(cf, "SHARP", ConvCpp2Htm::T_SHARP);
    defWord(cf, "WORD1", ConvCpp2Htm::T_WORD1);
    defWord(cf, "WORD2", ConvCpp2Htm::T_WORD2);
    defWord(cf, "WORD3", ConvCpp2Htm::T_WORD3);
    defWord(cf, "WORD4", ConvCpp2Htm::T_WORD4);
    defWord(cf, "WORD5", ConvCpp2Htm::T_WORD5);
    defWord(cf, "WORD6", ConvCpp2Htm::T_WORD6);
    defWord(cf, "WORD7", ConvCpp2Htm::T_WORD7);
    defWord(cf, "WORD8", ConvCpp2Htm::T_WORD8);
    defWord(cf, "WORD9", ConvCpp2Htm::T_WORD9);

    // ヘッダフッタの取得.
    cf.getStr(header_[0], "HEADER");
    cf.getStr(footer_[0], "FOOTER");
    cf.getStr(header_[1], "HEADER1");
    cf.getStr(footer_[1], "FOOTER1");
    cf.getStr(header_[2], "HEADER2");
    cf.getStr(footer_[2], "FOOTER2");
    cf.getStr(header_[3], "HEADER3");
    cf.getStr(footer_[3], "FOOTER3");
    cf.term();

    // ヘッダフッダ中の *css* を実際の設定に置換.
    for (int n = 0; n < HF_NUM; n++) {
        for (;;) {
            rc = header_[n].find("*css*");
            if (rc == int(string::npos))
                break;
            header_[n].replace(rc, 5, css_);
        }
        for (;;) {
            rc = footer_[n].find("*css*");
            if (rc == int(string::npos))
                break;
            footer_[n].replace(rc, 5, css_);
        }
    }
    return true;
}



/** コンフィグで、tokグループに属する単語を登録
 *  @param cf   処理中のコンフィグファイルの内容
 *  @param tag  コンフィグファイル中の定義名
 *  @param tok  登録する単語の種類(T_SHARP,T_WORD1～T_WORD9)
 */
void Conv::defWord(CfgFile &cf, const char *tag, tok_t tok)
{
    vector<string> lst;
    lst.clear();
    if (cssStrSw_[tok]) {
        if (cf.getStrVec(lst, tag)) {
            for (int i=0; i < int(lst.size()); i++) {
                cpp2htm_.defWord(lst[i], tok);
            }
        }
    }
}



/** nameのc/c++ファイルをoutNameにhtmlに変換して出力
 */
int Conv::run(const char *name, const char *outName)
{
    // 入力ファイル用意.
    ifstream inFstrm;
    if (name && name[0]) {
        inFstrm.open(name);
        if (!inFstrm) {
            CERR << name << " をopenできない\n";
            return false;
        }
    }
    istream &istrm = (name && name[0]) ? static_cast<istream&>(inFstrm) : cin;

    // 出力ファイル用意.
    ofstream outFstrm;
    if (outName && outName[0]) {
        outFstrm.open(outName);
        if (!outFstrm) {
            CERR << outName << " をopenできない\n";
            return false;
        }
    }
    ostream &ostrm = (outName && outName[0]) ? static_cast<ostream&>(outFstrm) : cout;

    // ヘッダとフッダの準備.
    string hdr(header_[hdrFtrMd_]), ftr(footer_[hdrFtrMd_]);
    replaceFn(name, hdr);
    replaceFn(name, ftr);

    // ヘッダ出力.
    ostrm << hdr;

    // ソース変換.
    int lineNum = 0;
    string lnoSp(lnoSep_);
    if (lnoSp.empty())
        lnoSp = " ";
    string st;
    while (!istrm.eof()) {
        lineNum++;
        char buf[0x4000];                               // c/c++ソース１行のバッファ.
        char buf2[0x10000];                             // tab展開後用のバッファ.
        st.clear();                                     // 文字バッファを初期化.
        buf[0] = 0;
        istrm.getline(buf, sizeof buf);                 // 1行入力.
        if (istrm.eof() && buf[0] == 0)
            break;
        strTab(buf2, buf, 0, tabSz_, useUtf8_<<9, sizeof buf2); // tab変換.
        if (markMd_ == 0 || checkMarkLine(buf2)) {      // 通常の変換、または、マーク行なら.
            cpp2htm_.convLine(st, buf2);                // c/c++ 行を html に変換.
            if (lnoClm_ > 0)                            // 桁数があれば行番号を付ける.
                ostrm << "<span class=C_LNO>" << setw(lnoClm_) << lineNum << lnoSp << "</span>";
        } else {                                        // マーク行をチェックする場合.
            st += "<span class=C_HT>";
            cpp2txt_.convLine(st, buf2);
            st += "</span>";
            if (lnoClm_ > 0)                            // 桁数があれば行番号を付ける.
                ostrm << "<span class=C_HT>" << setw(lnoClm_) << lineNum << lnoSp << "</span>";
        }
        ostrm << st << endl;
    }

    // フッタ出力
    ostrm << ftr;
    return true;
}



/** ヘッダやフッダ文字列中 *fn* があれば、ファイル名に置き換える
 */
void Conv::replaceFn(const char *name, string &hdr)
{
    //x hdr = header_;
    //x ftr = footer_;
    int i;
    for (;;) {
        i = hdr.find("*fn*");
        if (i == int(string::npos))
            break;
        hdr.replace(i, 4, string(name));
    }
}



/** マーク行？ ※モード２ならbuf2を編集
 */
bool Conv::checkMarkLine(char *buf)
{
    size_t  l = strlen(buf);
    if (l < markStr_.size())
        return false;
    char *p = buf + l - markStr_.size();
    if (p != markStr_)
        return false;
    if (markMd_ == 2)
        *p = '\0';
    return true;
}




// ---------------------------------------------------------------------------

/// メインの処理
class App {
public:
    App() {}
    ~App() {}
    int main(int argc, char *argv[]);
    int oneFile(const char *name, Conv &conv, Opts &opts);
};



/** 実際のメイン処理。オプション解析＆ファイルコンバート呼び出し
 */
int App::main(int argc, char *argv[])
{
    if (argc < 2)   // コマンドライン引数が無かったらヘルプ表示.
        ::usage();

    Opts    opts(argv[0]);
    Conv    conv;
    conv.setDfltCfgName(opts.cfgName_);
    int rc = 0, n = 0;
    for (int i = 1; i < argc; i++) {
        char *p = argv[i];
        if (*p == '-') {
            opts.get(p);
            // デフォルトcfgファイルの吐き出し.
            if (opts.cfgOut_) {
                cout << conv.dfltCfgData_;
                opts.cfgOut_ = 0;
            }
        } else if (*p == ':') {
            string nm(argv[0]);
            fname_chgExt(nm, NULL);
            nm += "-";
            nm += p+1;
            nm += ".cfg";
            opts.cfgName_ = nm;
        } else {
            rc = oneFile(p, conv, opts);
            n++;
        }
    }
    if (n == 0 && opts.stdio_) {
        rc = oneFile("", conv, opts);
    }
    rc = !rc;
    return rc;
}


/** 1ファイル変換の準備＆go
 */
int App::oneFile(const char *name, Conv &conv, Opts &o)
{
    string inm(name);
    string onm(name);

    if (o.outName_.empty()) {
        if (o.stdio_)
            onm = "";
        else
            onm += ".htm";
    } else {
        onm = string(o.outName_);
        o.outName_ = "";    // -oは一回こっきりなんで、次回向けに初期化.
    }
    if (conv.isCfgLoaded() == false) {
        int rc = conv.cfgLoad(o.cfgName_);
        if (rc == 0) {
            CERR << "コンフィグファイル " << o.cfgName_ << " をロードできなかった\n";
            return 0;
        }
    }
    // オプション設定を変換側へ通知.
    conv.setHdrFtrMode(o.hdrFtrMd_);
    if (o.tabSz_ > -1)
        conv.setTabSz(o.tabSz_);
    if (o.lnoClm_ >= 0)
        conv.setLnoClm(o.lnoClm_);
    if (!o.lnoSep_.empty())
        conv.setLnoSep(o.lnoSep_);
    if (o.useUtf8_ >= 0)
        conv.setUtf8(o.useUtf8_);
    conv.setMarkLine(o.markMd_, o.markStr_);

    // inmのファイルをonmに変換を実行.
    return conv.run(inm.c_str(), onm.c_str());
}




// ---------------------------------------------------------------------------

/** ここより始まる */
int main(int argc, char *argv[])
{
    int rc;
    App app;
    try {
      #ifdef _MSC_VER
        // VC(7)でコンパイルしたら、argv[0]にフルパスでなく、コマンドライン.
        // でタイプしたプログラム名しか入っていなかったので、強引対処.
        argv[0] = _pgmptr;
      #endif
        rc = app.main(argc, argv);

    } catch (const exception &ex) {
        CERR << ex.what() << endl;
        rc = 1;
    }
    return rc;
}



// ---------------------------------------------------------------------------
// c2htm 作業メモ
// 2003-07-21   作成開始.
// 2003-07-22   とりあえず完成.コンフィグ無し.
// 2003-07-23   convCpp2htm.cppソースを誤削除してしまい、同等品を復元しなおし....
// 2003-07-24   コンフィグ対応。-sで標準入出力対応(fstreamを諦めFILEにT T)。
//              コンフィグ設定(有無)で、項目の一部機能のon/off可能に.
// 2003-07-25   デバッグもろもろ.
// 2003-07-27   デフォルトcfgファイルを内臓するようにした.
// 2003-07-27   v1.00 として公開.
// 2003-07-28   v1.01 行番号が外せなくなってたので修正. てかConvに初期化抜けがあった.
// 2003-08-08   v1.02 \0 や \123 等8進数で最初の一文字を出力し忘れるのを修正.
//              行番号offを、-1桁でなく0桁で判断するように修正.
// 2003-08-09   v1.50
//              ・配布パッケージから cfgfile.cpp,cfgfile.h,c2htm.cfg.c
//              　ソースファイルが抜けていました m(_ _)m
//              ・標準入出力の-sをusageに書き忘れてたのを追加.
//              ・fopen/fprintf等をやめてofstream等に変更.
//              ・cfg解析ルーチンの不具合修正.
//              ・cfg内で、全てのC_設定を、削除/コメント化、で機能offできるように修正.
//              ・cfg内で、SYM=,NAMESYM=にて、記号文字の種別を設定可能に.
//              ・cfg内で、LINECMT=にて、// /* を止め記号一文字での１行コメントを可能に.
// 2003-08-10   v1.51  c/c++ソースとして１行余分に空行を出力していたのを修正.
// 2003-08-11   v1.52 実は -t でのタブ指定が実装されていなかった(T T)
// 2003-08-18   v1.53 '.'を記号文字にしていないとき、出力から欠けてしまったのを修正.
// 2003-08-21   v1.60 行末にマークのある行のみ色つき変換、以外はハーフトーン扱い にするオプションを追加.
// 2004-01-03   v1.61 strtab.cのバグ修正.
// 2004-01-03   v1.61 strtab.cのバグ修正.
// 2004-01-29   v1.70 D言語向けの修正。 utf8の入力。 /+コメント+/
//              ・\エスケープしない文字列として r"文字列" `文字列` に対応.
// 2004-02-07   v1.71 数値やエスケープ文字対応の(D向けの)抜け対処.
// 2005-11-??   v1.72 CMTEX1,CMTEX2の追加.
// 2017-09-16   v1.73 c++11以降の予約語を追加. L""同様な u"",U"",u8"" 対応.
