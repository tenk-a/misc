/**
 *  @file   ExArgv.h
 *  @brief  argc,argvの拡張処理(ワイルドカード,レスポンスファイル).
 *  @author Masashi KITAMURA
 *  @date   2006-2010,2023
 *  @note
 *  -   main(int argc,char* argv[]) のargc,argvに対し、
 *      ワイルドカード指定やレスポンスファイル指定等を展開したargc,argvに変換.
 *      main()の初っ端ぐらいで
 *          ExArgv_conv(&argc, &argv);
 *      のように呼び出す.
 *  -   WinMain() で使う場合は EXARGV_FOR_WINMAIN を定義し、
 *          ExArgv_forWinMain(cmdl, &argc, &argv);
 *      のように呼び出す.
 *
 *  -   主にWin/Dos系(のコマンドラインツール)での利用を想定.
 *      一応 mac,linux gcc/clang でのコンパイル可.
 *      (unix系だとワイルドカードはシェル任せだろうで、メリット少なく)
 *
 *  -   ExArgv.hは、一応ヘッダだが、ExArgv.c の設定ファイルでもある.
 *      アプリごとに ExArgv.h ExArgv.c をコピーして、ExArgv.hを
 *      カスタムして使うのを想定.
 *  -   設定できる要素は、
 *          - ワイルドカード (on/off)
 *          - ワイルドカード時の再帰指定(**)の有無 (on/off)
 *          - @レスポンスファイル (on/off)
 *          - .exe連動 .cfg ファイル 読込 (on/off)
 *          - オプション環境変数名の利用
 *          等
 *
 *  -   引数文字列の先頭が'-'ならばオプションだろうで、その文字列中に
 *      ワイルドカード文字があっても展開しない.
 *  -   マクロ UNICODE か EXARGV_USE_WCHAR を定義で wchar_t用、なければchar用.
 *  -   UTF8 が普及したので、EXARGV_USE_MBC 定義時のみMBCの2バイト目'\'対処.
 *  -   _WIN32 が定義されていれば win用、でなければ unix系を想定.
 *
 *  - Public Domain Software
 */

#ifndef EXARGV_INCLUDED__
#define EXARGV_INCLUDED__

// ---------------------------------------------------------------------------
// 設定.

//[] 定義すると、WinMain 用に ExArgv_forWinMain を生成.(ExArgv_conv は無)
//#define EXARGV_FOR_WINMAIN

//[] 定義され かつ UNICODE 未定義なら MBCS として2バイト目\文字対処を行う
//#define EXARGV_USE_MBC

//[] 定義すると、wchar_t 用として生成. UNICODE 定義時は自動で定義される.
//#define EXARGV_USE_WCHAR

//[] EXARGV_USE_WCHAR時に定義すると、ExArgv_conv でなく ExArgv_conv_to_utf8 を生成.
//#define EXARGV_USE_WCHAR_TO_UTF8

//[] ワイルドカード指定を 1=有効  0=無効  未定義=デフォルト設定(1)
//#define EXARGV_USE_WC         1

//[] ワイルドカードon時に、ワイルドカード文字 ** があれば再帰検索に
//      1=する 0=しない 未定義=デフォルト設定(1)
#define EXARGV_USE_WC_REC     1


//[] @レスポンスファイルを
//      1=有効   0=無効  未定義=デフォルト設定(0)
//#define EXARGV_USE_RESFILE    0


//[] 簡易コンフィグ(レスポンス)ファイル入力を
//      1=有効  0=無効  未定義=デフォルト(0)
//   有効時は、win/dosなら .exe を .cfg に置換したパス名.
//             以外なら unix 系だろうで ~/.実行ファイル名.cfg
//#define EXARGV_USE_CONFIG     0


//[] コンフィグファイル入力有効のとき、これを定義すれば、
//      コンフィグファイルの拡張子をこれにする.
//#define EXARGV_CONFIG_EXT     ".cfg"  // .conf


//[] 定義すると、この名前の環境変数をコマンドライン文字列として利用.
//#define EXARGV_ENVNAME    "your_app_env_name"


//[] win環境のみ. argv[0] の実行ファイル名をフルパス化
//      1=有効  0=無効      未定義=デフォルト(0)
//   ※bcc,dmc,watcomは元からフルパスなので何もしません. のでvc,gcc向.
//#define EXARGV_USE_FULLPATH_ARGV0

//[] 定義すれば、filePath中の \ を / に置換.
//#define EXARGV_TOSLASH


//[] 定義すれば、filePath中の / を \ に置換.
//#define EXARGV_TOBACKSLASH


//[] 定義すれば、/ もオプション開始文字とみなす.
//#define EXARGV_USE_SLASH_OPT

//[] 実験. VCのみ. 定義すると、setargvの代用品としてコンパイル(ExArgv_getは無)
//   現状 setargv.obj のリンクも必要.
// #define EXARGV_USE_SETARGV



// ---------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif


#if defined EXARGV_USE_SETARGV  // VCの暗黙処理の置き換え用.

#elif defined EXARGV_FOR_WINMAIN // win-gui用. _WINDOWS はCONSOLE用でも定義されることもあり廃止.
 #if defined UNICODE || defined EXARGV_USE_WCHAR
  #if defined(EXARGV_USE_WCHAR_TO_UTF8)
   void ExArgv_to_utf8_forWinMain(const wchar_t* pCmdLine, int* pArgc, wchar_t*** pppArgv, char*** pppUtf8s);
  #else
   void ExArgv_forWinMain(const wchar_t* pCmdLine, int* pArgc, wchar_t*** pppArgv);
  #endif
 #else
   void ExArgv_forWinMain(const char*    pCmdLine, int* pArgc, char***    pppArgv);
 #endif
#else                       // コマンドラインツール用. mainの初っ端くらいに呼ぶのを想定.
 #if defined UNICODE || defined EXARGV_USE_WCHAR
  #if defined(EXARGV_USE_WCHAR_TO_UTF8)
   char** ExArgv_conv_to_utf8(int* pArgc, wchar_t*** ppArgv);
  #else
   void** ExArgv_conv(int* pArgc, wchar_t*** pppArgv);
  #endif
 #else
   void** ExArgv_conv(int* pArgc, char*** pppArgv);
 #endif
#endif
void ExArgv_Free(void*** pppArgv);
void ExArgv_FreeA(char*** pppArgv);
void ExArgv_FreeW(wchar_t*** pppArgv);


#ifdef __cplusplus
}
#endif
// ---------------------------------------------------------------------------
#endif  // EXARGV_INCLUDED__
