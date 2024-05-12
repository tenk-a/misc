/**
 *  @file   ExArgv.h
 *  @brief  argc,argvの拡張処理(ワイルドカード,レスポンスファイル).
 *  @author Masashi KITAMURA
 *  @date   2006-2024
 *  @license Boost Software Lisence Version 1.0
 *  @note
 *  -   主にWin/Dos系(のコマンドラインツール)での利用を想定.
 *      mac,linux gcc/clang でも利用可.
 *
 *  -   main(int argc,char* argv[]) のargc,argvに対し,
 *      ワイルドカード指定やレスポンスファイル指定等を展開したargc,argvに変換.
 *      main()の初っ端ぐらいで,
 *          ExArgv_conv(&argc, &argv);
 *      のように呼び出す.
 *      WC変換を後で行いたい場合は
 *          ExArgv_convEx(&argc, &argv, 0); // ResFile等WC展開以外を先に行う.
 *          (なにか処理)
 *          ExArgv_convEx(&argc, &argv, 1); // WC展開を行う.
 *      結果の argv の各文字列は、必ず malloc されたものになる.
 *      明示的に argv を開放する場合は ExArgv_Free(&argv) する.
 *
 *  -   WinMain() で使う場合は EXARGV_FOR_WINMAIN を定義し、
 *          ExArgv_cmdLineToArgv(cmdl, &argc, &argv);
 *          ExArgv_conv(&argc, &argv);
 *      のように呼び出す. 旧版互換で,
 *          ExArgv_forWinMain(cmdl, &argc, &argv);
 *      も可.
 *
 *  -   ExArgv.h は、ヘッダだが、ExArgv.c の設定ファイルでもある.
 *      アプリごとに ExArgv.h ExArgv.c をコピーして、ExArgv.h
 *      をカスタムして使うのを想定.
 *      - あるいは ExArgv_conf.h を用意, (EXARGV_UE_CONF_Hを定義して)
 *        そちらに設定する.
 *
 *  -   設定できる要素は,
 *      - ワイルドカード (on/off)
 *      - ワイルドカード時の再帰指定(**)の有無 (on/off)
 *      - @レスポンスファイル (on/off)
 *      - .exe連動 .cfg ファイル 読込 (on/off)
 *      - オプション環境変数名の利用.
 *      等.
 *
 *  -   引数文字列の先頭が'-'ならばオプションだろうで、その文字列中に
 *      ワイルドカード文字があっても展開しない.
 *  -   _WIN32 が定義されていれば win用、でなければ unix系を想定.
 *  -   マクロ UNICODE か EXARGV_USE_WCHAR を定義で wchar_t用、なければchar用.
 *  -   UTF8 が普及したので、EXARGV_USE_MBC 定義時のみMBCの2バイト目'\'対処.
 *  -   EXARGV_USE_WCHAR_TO_UTF8 を定義すれば、Win10 1903以前の環境で UTF8名を
 *      扱うための変換関数を用意.
 *      ex) int wmain(int argc, wchar_t* wargv[]) {
 *              char** argv = ExArgv_convExToUtf8(&argc, wargv, 1);
 */

#ifndef EXARGV_H_INCLUDED__
#define EXARGV_H_INCLUDED__

// ---------------------------------------------------------------------------
// 設定.

//[] 定義すると、WinMain 用に ExArgv_forWinMain を生成.
//#define EXARGV_FOR_WINMAIN

//[] 定義すると、wchar_t 用として生成. UNICODE 定義時は自動で定義される.
//#define EXARGV_USE_WCHAR

//[] 定義すると、EXARGV_USE_WCHAR を有効、ExArgv_wargvToU8等を利用可能に.
//#define EXARGV_USE_WCHAR_TO_UTF8

//[] 定義され かつ UNICODE 未定義なら MBCS として2バイト目\文字対処を行う.
//#define EXARGV_USE_MBC

//[] ワイルドカード指定を 1=有効  0=無効  未定義=デフォルト設定(1)
//#define EXARGV_USE_WC         1

//[] ワイルドカードon時に、ワイルドカード文字 ** があれば再帰検索に,
//      1=する 0=しない 未定義=デフォルト設定(1)
//#define EXARGV_USE_WC_REC     1

//[] @レスポンスファイルを,
//      1=有効   0=無効  未定義=デフォルト設定(0)
//#define EXARGV_USE_RESFILE    0
#define EXARGV_USE_RESFILE    1

//[] 簡易コンフィグ(レスポンス)ファイル入力を,
//      1=有効  0=無効  未定義=デフォルト(0)
//   有効時は、win/dosなら .exe を .cfg に置換したパス名.
//             以外なら unix 系だろうで ~/.実行ファイル名.cfg
//#define EXARGV_USE_CONFIG     0

//[] コンフィグファイル入力有効のとき、これを定義すれば,
//      コンフィグファイルの拡張子をこれにする.
//#define EXARGV_CONFIG_EXT     ".ini"

//[] 定義すると、この名前の環境変数をコマンドライン文字列として利用.
//#define EXARGV_ENVNAME    "your_app_env_name"


//[] win環境のみ. argv[0] の実行ファイル名をフルパス化.
//      1=有効  0=無効      未定義=デフォルト(0)
//   ※bcc,dmc,watcomは元からフルパスなので何もしません. のでvc,gcc向.
//#define EXARGV_USE_FULLPATH_ARGV0

//[] 定義すれば、filePath中の \ を / に置換.
//#define EXARGV_TOSLASH


//[] 定義すれば、filePath中の / を \ に置換.
//#define EXARGV_TOBACKSLASH

//[] 定義すれば、/ もオプション開始文字とみなす.
//#define EXARGV_USE_SLASH_OPT


// ---------------------------------------------------------------------------
// 上記で設定せず、ExArg_conf.h で設定の場合.

//#if defined(EXARGV_USE_CONF_H) || (defined(__has_include) && __has_include("ExArgv_conf.h"))
#if defined(EXARGV_USE_CONF_H) || defined(__has_include) && __has_include("ExArgv_conf.h")
 #include "ExArgv_conf.h"
#endif


// ---------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

#define ExArgv_RC	int

#if !defined EXARGV_USE_WCHAR \
	&& (defined UNICODE || defined EXARGV_USE_WCHAR_TO_UTF8)
 #define EXARGV_USE_WCHAR
#endif

#if defined(EXARGV_USE_WCHAR)
 #define EXARGV_CHAR_T	wchar_t
#else
 #define EXARGV_CHAR_T	char
#endif

#if defined(EXARGV_USE_WCHAR_TO_UTF8) == 0

  ExArgv_RC	ExArgv_conv(int* pArgc, EXARGV_CHAR_T*** pppArgv);
  ExArgv_RC	ExArgv_convEx(int* pArgc, EXARGV_CHAR_T*** pppArgv, unsigned wcFlags);
  void 		ExArgv_Free(EXARGV_CHAR_T*** pppArgv);

 #if defined EXARGV_FOR_WINMAIN	// win-gui用.
   ExArgv_RC ExArgv_cmdLineToArgv(EXARGV_CHAR_T const* pCmdLine, int* pArgc, EXARGV_CHAR_T*** pppArgv);
   ExArgv_RC ExArgv_forWinMain(EXARGV_CHAR_T const* pCmdLine, int* pArgc, EXARGV_CHAR_T*** pppArgv);
 #endif

 #if EXARGV_USE_RESFILE || EXARGV_USE_CONFIG
  void*  ExArgv_fileLoadMalloc(EXARGV_CHAR_T const* fpath, size_t* pSize);
  size_t ExArgv_fileSize(EXARGV_CHAR_T const* fpath);
 #endif

#else	// defined(EXARGV_USE_WCHAR_TO_UTF8)

  ExArgv_RC	ExArgv_conv( int* pArgc, char*** pppArgv);
  ExArgv_RC	ExArgv_convW(int* pArgc, wchar_t*** pppArgv);

  ExArgv_RC	ExArgv_convEx( int* pArgc, char*** pppArgv, unsigned wcFlags);
  ExArgv_RC	ExArgv_convExW(int* pArgc, wchar_t*** pppArgv, unsigned wcFlags);

  char** 	ExArgv_convExToUtf8(int* pArgc, wchar_t** pppArgv, unsigned wcFlags);

  void 		ExArgv_Free( char*** pppArgv);
  void 		ExArgv_FreeW(wchar_t*** pppArgv);

 #if defined EXARGV_FOR_WINMAIN	// win-gui用.
   ExArgv_RC ExArgv_cmdLineToArgv( wchar_t const* pCmdLine, int* pArgc, char*** pppArgv);
   ExArgv_RC ExArgv_cmdLineToArgvW(wchar_t const* pCmdLine, int* pArgc, wchar_t*** pppArgv);
   ExArgv_RC ExArgv_forWinMain( wchar_t const* pCmdLine, int* pArgc, char*** pppArgv);
   ExArgv_RC ExArgv_forWinMainW(wchar_t const* pCmdLine, int* pArgc, wchar_t*** pppArgv);
 #endif

 #if EXARGV_USE_RESFILE || EXARGV_USE_CONFIG
  void*  	ExArgv_fileLoadMalloc(char const* fpath, size_t* pSize);
  size_t 	ExArgv_fileSize(char const* fpath);
 #endif

  char**    ExArgv_wargvToUtf8(int argc, wchar_t* ppWargv[]);
  char*     ExArgv_u8strdupFromWcs(wchar_t const* wcs);
  wchar_t** ExArgv_u8argvToWcs(int argc, char* ppArgv[]);
  wchar_t*  ExArgv_wcsdupFromUtf8(char const* u8s);

#endif	// EXARGV_USE_WCHAR_TO_UTF8

#ifdef __cplusplus
}
#endif
// ---------------------------------------------------------------------------
#endif  // EXARGV_H_INCLUDED__
