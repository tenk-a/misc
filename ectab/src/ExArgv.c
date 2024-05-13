/**
 *  @file   ExArgv.c
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
 *              char** argv = NULL;
 *              if (ExArgv_conv(&argc, &wargv))
 *                  argv = ExArgv_wargvToUtf8(argc, argv);
 */
 // 2009 再帰指定を**にすることで、仕様を単純化.
 // 2023 UTF-8 対処. vcpkgがWin用に必ず_WINDOWSを定義するためWinMain指定変更.
 // 2024 UTF-8 の対応方法を変更. exit(1)で終了せずエラーreturnするように変更.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef assert
#include <assert.h>
#endif

#if defined _WIN32 && defined UNICODE && defined EXARGV_USE_WCHAR == 0
#define EXARGV_USE_WCHAR
#endif

#ifdef EXARGV_USE_WCHAR
#include <wchar.h>
#endif

#include "ExArgv.h"

#if defined EXARGV_USE_WCHAR_TO_UTF8 && defined EXARGV_USE_WCHAR == 0
#define EXARGV_USE_WCHAR
#endif

#if !defined _WIN32 && defined EXARGV_USE_WCHAR
#error EXARGV_USE_WCHAR is only available when _WIN32 is defined.
#endif

#ifdef _MSC_VER
#pragma warning(disable:4996)                   // MSのお馬鹿なセキュリティ関数使えを無視.
#endif

/// 定義するとこの名前の環境変数の中身をコマンドライン文字列として使えるようにする.
//#define EXARGV_ENVNAME    "your_app_env_name"

#ifndef EXARGV_USE_WC
#define EXARGV_USE_WC       1                   ///< ワイルドカード指定があればファイル名に展開する.
#endif

#ifndef EXARGV_USE_WC_REC
#define EXARGV_USE_WC_REC   1                   ///< EXARGV_USE_WC時に、**があれば再帰ワイルドカードにする.
#endif

#ifndef EXARGV_USE_RESFILE
#define EXARGV_USE_RESFILE  0                   ///< @レスポンスファイルを有効にする.
#endif

#ifndef EXARGV_USE_CONFIG
#define EXARGV_USE_CONFIG   0                   ///< .exeを.cfgに置換したパス名から読込.
#endif

#ifndef EXARGV_CONFIG_EXT
//#define EXARGV_CONFIG_EXT ".ini"              ///< コンフィグファイル入力有の時の拡張子. 拡張子は4文字以内のこと.
#endif

#if 0 //ndef EXARGV_USE_FULLPATH_ARGV0
#define EXARGV_USE_FULLPATH_ARGV0   1           ///< argv[0] の実行ファイル名をフルパスにする/しない. win環境のみ.
#endif

//#define EXARGV_TOSLASH                        ///< 定義すれば、filePath中の \ を / に置換.
//#define EXARGV_TOBACKSLASH                    ///< 定義すれば、filePath中の / を \ に置換.
//#define EXARGV_USE_SLASH_OPT                  ///< 定義すれば、/ もオプション開始文字とみなす.

// ===========================================================================
// 辻褄あわせ.

#if !defined DOSWIN32 && (defined _WIN32 || defined MSODS)
 #define DOSWIN32   1                           // DOS/WIN系なら定義.
#endif


#if defined _WIN32
 //#if !defined(EXARGV_USE_MBC) && !defined(UNICODE)
 // #define UNICODE
 //#endif
 #include <windows.h>
 #if defined _MSC_VER && defined EXARGV_USE_MBC // CharNext()で必要.
  #pragma comment(lib, "User32.lib")
 #endif
#elif defined MSDOS
 #include <mbctype.h>
 #include <dirent.h>
 #include <sys/stat.h>
#else   // linux
 #include <dirent.h>
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <fnmatch.h>
#endif


#if defined __cplusplus
 #define BOOL           bool
 #define EXTERN_C       extern "C"
#else
 #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
  #define BOOL          _Bool
 #else
  #define BOOL          char
 #endif
 #define EXTERN_C       extern
 #if defined DOSWIN32
  #define inline        __inline
 #endif
#endif


#ifndef MAX_PATH
 #ifdef _MAX_PATH
  #define MAX_PATH      _MAX_PATH
 #else
  #if defined MSDOS //DOSWIN32
   #define MAX_PATH     260
  #else
   #define MAX_PATH     4096
  #endif
 #endif
#endif


#if defined(EXARGV_USE_WCHAR)
#define MAC_TO_STR(x)   MAC_TO_STR_2(x)
#define MAC_TO_STR_2(x) L##x
#else
#define MAC_TO_STR(x)   x
#endif


// ===========================================================================
// char,wchar_t 切り替えの辻褄合わせ関係.

#ifdef EXARGV_USE_WCHAR
#undef  _pgmptr
#define _pgmptr         _wpgmptr
#define T(x)            L##x
typedef wchar_t         char_t;
typedef wchar_t         uchar_t;
#define STR_LEN(a)      wcslen(a)
#define STR_CMP(a,b)    wcscmp(a,b)
#define STR_R_CHR(a,b)  wcsrchr(a,b)
#define GET_ENV(s)      _wgetenv(s)
//#define STDERR        stderr
//#define FPRINTF       fwprintf
#define ERR_PUTS(s)     fprintf(stderr, "%s", s)
#else
#define T(x)            x
typedef char            char_t;
typedef unsigned char   uchar_t;
#define STR_LEN(a)      strlen(a)
#define STR_CMP(a,b)    strcmp(a,b)
#define STR_R_CHR(a,b)  strrchr(a,b)
#define GET_ENV(s)      getenv(s)
//#define STDERR        stderr
//#define FPRINTF       fprintf
#define ERR_PUTS(s)     fprintf(stderr, "%s", s)
#endif



// ===========================================================================

enum { FILEPATH_SZ              = (MAX_PATH*2 > 0x4000) ? MAX_PATH*2 : 0x4000 };
enum { EXARGV_VECTOR_CAPA_BASE  = 4096 };


#ifdef EXARGV_TOBACKSLASH
#define DIRSEP_STR          T("\\")
#else
#define DIRSEP_STR          T("/")
#endif

//#if (EXARGV_USE_WC || EXARGV_USE_RESFILE) && !EXARGV_USE_CONFIG && !defined(EXARGV_ENVNAME) \
//        && !defined(EXARGV_FOR_WINMAIN) \
//        && !defined EXARGV_TOSLASH && !defined EXARGV_TOBACKSLASH
//    #define EXARGV_USE_CHK_CHR
//#endif

#if EXARGV_USE_WC
static BOOL             s_ExArgv_wildMode; ///< ワイルドカード文字列の有無.
#endif


// ===========================================================================

typedef struct ExArgv_Vector {
    char_t**        buf;
    unsigned        size;
    unsigned        capa;
} ExArgv_Vector;

static ExArgv_Vector *ExArgv_Vector_create(unsigned size);
static char_t**     ExArgv_Vector_release(ExArgv_Vector* pVec);
static BOOL         ExArgv_Vector_push(ExArgv_Vector* pVec, char_t const* pStr);
static char_t**     ExArgv_VectorToArgv(ExArgv_Vector** pVec, int* pArgc, char_t*** pppArgv);
static void*        ExArgv_alloc(unsigned size);
static char_t*      ExArgv_strdup(char_t const* s);
static void         ExArgv_free(void* s);

#define EXARGV_ALLOC(T,size)    ((T*)ExArgv_alloc((size) * sizeof(T)))

#if EXARGV_USE_WC
static int          ExArgv_Vector_findFname(ExArgv_Vector* pVec, char_t const* pPathName, int recFlag);
static BOOL         ExArgv_wildCard(ExArgv_Vector* pVec);
#endif
#if defined EXARGV_FOR_WINMAIN
static int          ExArgv_forCmdLine1(char_t const* pCmdLine, int* pArgc, char_t*** pppArgv);
#endif
#if 0 //defined EXARGV_USE_CHK_CHR
static unsigned     ExArgv_checkWcResfile(int argc, char_t** argv);
#endif
#ifdef EXARGV_ENVNAME
static BOOL         ExArgv_getAppEnv(char_t const* envName, ExArgv_Vector* pVec);
#endif
#if EXARGV_USE_CONFIG
static BOOL         ExArgv_getCfgFile(char_t const* exeName, ExArgv_Vector* pVec);
#endif
#if EXARGV_USE_RESFILE || EXARGV_USE_CONFIG
static BOOL         ExArgv_getResFile(char_t const* fname, ExArgv_Vector* pVec, BOOL notFoundOk);
#endif

#if (defined EXARGV_TOSLASH) || (defined EXARGV_TOBACKSLASH)
static void         ExArgv_convBackSlash(ExArgv_Vector* pVec);
#endif
#if (defined EXARGV_TOBACKSLASH)
static char_t*      ExArgv_fname_slashToBackslash(char_t filePath[]);
#endif
#if (defined EXARGV_TOSLASH)
static char_t*      ExArgv_fname_backslashToSlash(char_t filePath[]);
#endif

#if EXARGV_USE_WC || EXARGV_USE_CONFIG
static char_t*      ExArgv_fname_baseName(char_t const* adr);
#endif
#if defined EXARGV_ENVNAME || defined EXARGV_FOR_WINMAIN
static char_t*      ExArgv_fname_scanArgStr(char_t const* str, char_t arg[], int argSz);
#endif
#if EXARGV_USE_WC
static unsigned     ExArgv_fname_isWildCard(char_t const* s);
#endif

/**
 */
static inline int ExArgv_isOpt(int c)
{
  #ifdef EXARGV_USE_SLASH_OPT
    return c == T('-') || c == T('/');
  #else
    return c == T('-');
  #endif
}


// ===========================================================================

/** argc,argv からレスポンスファイルやワイルドカード展開して,
 *  argc, argvを更新して返す. 
 *  argvや各文字列はmallocしたメモリ. また argv[argc] はNULLになる.
 *  @param  pArgc       argcのアドレス.(argvの数)
 *  @param  pppArgv     argvのアドレス.
 *  @param  wcFlags     bit0: ワイルドカード展開の有無
 *                      bit1: 入力が ExArgv_conv した argv としてメモリ解放.
 */
#if defined(EXARGV_USE_WCHAR)
ExArgv_RC ExArgv_convExW(int* pArgc, char_t*** pppArgv, unsigned wcFlags)
#else
ExArgv_RC ExArgv_convEx(int* pArgc, char_t*** pppArgv, unsigned wcFlags)
#endif
{
    int             argc;
    char_t**        ppArgv;
    ExArgv_Vector*  pVec;
    int             i;

    assert( pArgc != 0 && pppArgv != 0 );
    if (pArgc == 0 || pppArgv == 0)
        return 0;

    ppArgv = *pppArgv;
    argc   = *pArgc;
    assert(argc > 0 && ppArgv != 0);
    if (argc == 0 || ppArgv == 0)
        return 0;

  #if defined EXARGV_USE_FULLPATH_ARGV0 && defined _WIN32       // 古いソース用に、exeのフルパスを設定.
   #if defined _MSC_VER     // vcならすでにあるのでそれを流用.
    ppArgv[0] = _pgmptr;
   #elif 1 //defined __GNUC__   // わからないのでモジュール名取得.
    {
        static char_t nm[264];
        if (GetModuleFileName(NULL, nm, 264) > 0)
            ppArgv[0] = nm;
    }
   #endif
  #endif

    if (argc < 1)
        return 0;

  #if !EXARGV_USE_CONFIG && !defined(EXARGV_ENVNAME)    \
        && !defined(EXARGV_TOSLASH) && !defined(EXARGV_TOBACKSLASH)
   #if 0 //!EXARGV_USE_WC && !EXARGV_USE_RESFILE
    return 1;   //(void**)*ppArgv;     // ほぼ変換無し...
   #elif 0 //defined EXARGV_USE_CHK_CHR
    if (ExArgv_checkWcResfile(argc, ppArgv) == 0)   // 現状のargc,argvを弄る必要があるか?
        return 1;
   #endif
  #endif

    pVec = ExArgv_Vector_create(argc+1);            // argvが増減するので、作業用のリストを用意.

    //x printf("@4 %d %p(%p)\n", argc, ppArgv, *ppArgv);
    //x printf("   %p: %p %d %d\n", pVec, pVec->buf, pVec->capa, pVec->size);

    // 実行ファイル名の取得.
    if (argc > 0) {
        if (ExArgv_Vector_push( pVec, ppArgv[0] ) == 0)      // Vecに登録.
            return 0;
    }

    // 環境変数の取得.
  #ifdef EXARGV_ENVNAME
    assert(STR_LEN(EXARGV_ENVNAME) > 0);
    if (ExArgv_getAppEnv(EXARGV_ENVNAME, pVec) == 0)
        return 0;
  #endif

    // コンフィグファイルの読込.
  #if EXARGV_USE_CONFIG
    if (ExArgv_getCfgFile( ppArgv[0], pVec ) == 0)
        return 0;
  #endif

    //x printf("%p %x %#x %p\n",pVec, pVec->capa, pVec->size, pVec->buf);

  #if EXARGV_USE_WC
    s_ExArgv_wildMode  = 0;
  #endif

    // 引数の処理.
    for (i = 1; i < argc; ++i) {
        char_t const* p = ppArgv[i];
      #if EXARGV_USE_RESFILE
        if (i > 0 && *p == T('@')) {
            if (ExArgv_getResFile(p+1, pVec, 0) == 0)   // レスポンスファイル読込.
                return 0;
        } else
      #endif
        {
          #if EXARGV_USE_WC
            s_ExArgv_wildMode |= ExArgv_fname_isWildCard(p);
          #endif
            if (ExArgv_Vector_push( pVec, p ) == 0) // Vecに登録.
                return 0;
        }
    }

  #if EXARGV_USE_WC
    if ((wcFlags & 1) && s_ExArgv_wildMode) {
        if (ExArgv_wildCard(pVec) == 0) {           // ワイルドカードやディレクトリ再帰してパスを取得.
            return 0;
        }
    }
  #endif

  #if (defined EXARGV_TOSLASH) || (defined EXARGV_TOBACKSLASH)
    ExArgv_convBackSlash(pVec);                     // define設定に従って、\ と / の変換. (基本的には何もしない)
  #endif

    // 作業リストを argc,argv に変換し、作業リスト自体は開放.
    {
        int rc = ExArgv_VectorToArgv( &pVec, pArgc, pppArgv ) != 0;
        if (wcFlags & 2)
            ExArgv_Free(&ppArgv);
        return rc;
    }
}



#if defined(EXARGV_USE_WCHAR)
#if !defined(EXARGV_USE_WCHAR_TO_UTF8)

ExArgv_RC ExArgv_convEx(int* pArgc, char_t*** pppArgv, unsigned wcFlags)
{
    return ExArgv_convExW(pArgc, pppArgv, wcFlags);
}

#else

ExArgv_RC ExArgv_convEx(int* pArgc, char*** pppArgv, unsigned wcFlags)
{
    wchar_t** ppWargv = ExArgv_u8argvToWcs(*pArgc, *pppArgv);
    ExArgv_RC rc      = ExArgv_convExW(pArgc, &ppWargv, wcFlags);
    if (rc) {
        *pppArgv      = ExArgv_wargvToUtf8(*pArgc, ppWargv);
        ExArgv_FreeW(&ppWargv);
    } else {
        *pppArgv = NULL;
    }
    return *pppArgv != 0;
}

char** ExArgv_convExToUtf8(int* pArgc, wchar_t** ppWargv, unsigned wcFlags)
{
    char**    ppArgv = NULL;
    ExArgv_RC rc     = ExArgv_convExW(pArgc, &ppWargv, wcFlags);
    if (rc) {
        ppArgv       = ExArgv_wargvToUtf8(*pArgc, ppWargv);
        ExArgv_FreeW(&ppWargv);
    }
    return ppArgv;
}

#endif
#endif  // EXARGV_USE_WCHAR



#if defined(EXARGV_USE_WCHAR) == 0
ExArgv_RC ExArgv_conv(int* pArgc, char_t*** pppArgv)
{
    return ExArgv_convEx(pArgc, pppArgv, 1);
}
#else   // EXARGV_USE_WCHAR

#if defined(EXARGV_USE_WCHAR_TO_UTF8) == 0
ExArgv_RC ExArgv_conv(int* pArgc, char_t*** pppArgv)
{
    return ExArgv_convEx(pArgc, pppArgv, 1);
}
#else   // EXARGV_USE_WCHAR_TO_UTF8
ExArgv_RC ExArgv_conv(int* pArgc, char*** pppArgv)
{
    return ExArgv_convEx(pArgc, pppArgv, 1);
}
#endif  // EXARGV_USE_WCHAR_TO_UTF8

ExArgv_RC ExArgv_convW(int* pArgc, char_t*** pppArgv)
{
    return ExArgv_convExW(pArgc, pppArgv, 1);
}

#endif  // EXARGV_USE_WCHAR


// ===========================================================================

#if defined EXARGV_FOR_WINMAIN

#if defined(EXARGV_USE_WCHAR)
ExArgv_RC ExArgv_cmdLineToArgvW(char_t const* pCmdLine, int* pArgc, char_t*** pppArgv)
#else
ExArgv_RC ExArgv_cmdLineToArgv( char_t const* pCmdLine, int* pArgc, char_t*** pppArgv)
#endif
{
    ExArgv_Vector*  pVec;
    char_t*         arg;
    char_t const*   s;

    assert(pArgc != 0 && pppArgv != 0);
    if (pArgc == 0 || pppArgv == 0)
        return 0;

    arg = EXARGV_ALLOC(char_t, FILEPATH_SZ + 4);
    if (arg == NULL)
        return 0;

    if (GetModuleFileName(NULL, arg, FILEPATH_SZ) == 0)
        return 0;

    pVec = ExArgv_Vector_create(1);                 // 作業用のリストを用意.
    if (pVec == 0) {
        ExArgv_free(arg);
        return 0;
    }

    if (ExArgv_Vector_push(pVec, arg) == 0) {
        ExArgv_free(arg);
        return 0;
    }

    // 1行で渡されるコマンドラインを分割.
    s = pCmdLine;
    while ( (s = ExArgv_fname_scanArgStr(s, arg, FILEPATH_SZ)) != NULL ) {
        if (ExArgv_Vector_push( pVec, arg ) == 0) {
            ExArgv_free(arg);
            return 0;
        }
    }
    ExArgv_free(arg);

    return ExArgv_VectorToArgv( &pVec, pArgc, pppArgv );   // 作業リストを argc,argv に変換し、作業リスト自体は開放.
}


#if defined(EXARGV_USE_WCHAR)
#if !defined(EXARGV_USE_WCHAR_TO_UTF8)

ExArgv_RC ExArgv_cmdLineToArgv(char_t const* pCmdLine, int* pArgc, char_t*** pppArgv)
{
    return (ExArgv_cmdLineToArgvW( pCmdLine, pArgc, &ppWargv ;
}

#else

ExArgv_RC ExArgv_cmdLineToArgv(const wchar_t* pCmdLine, int* pArgc, char*** pppArgv)
{
    char_t**        ppWargv = NULL;
    if (ExArgv_cmdLineToArgvW( pCmdLine, pArgc, &ppWargv ) != NULL)
        *ppArgv = ExArgv_wargvToUtf8(*pArgc, ppWargv);
    return *ppArgv != 0;
}

#endif
#endif

/** WinMain から呼び出す用.
 */
#if !defined(EXARGV_USE_WCHAR)

ExArgv_RC ExArgv_forWinMain(char_t const* pCmdLine, int* pArgc, char_t*** pppArgv)
{
    if (ExArgv_VectorToArgv( &pVec, pArgc, pppArgv ))
        return ExArgv_conv(pArgc, pppArgv);
    return 0;
}

#else   // EXARGV_USE_WCHAR

ExArgv_RC ExArgv_forWinMainW(char_t const* pCmdLine, int* pArgc, char_t*** pppArgv)
{
    if (ExArgv_VectorToArgvW( &pVec, pArgc, pppArgv ))
        return ExArgv_convW(pArgc, pppArgv);
    return 0;
}

#if !defined(EXARGV_USE_WCHAR_TO_UTF8)

ExArgv_RC ExArgv_forWinMain(char_t const* pCmdLine, int* pArgc, char_t*** pppArgv)
{
    return ExArgv_forWinMainW( pCmdLine, pArgc, &ppWargv );
}

#else   // EXARGV_USE_WCHAR_TO_UTF8

ExArgv_RC ExArgv_forWinMain(const wchar_t* pCmdLine, int* pArgc, char*** pppArgv)
{
    char_t**    ppWargv = NULL;
    if (ExArgv_forWinMainW( pCmdLine, pArgc, &ppWargv ) != NULL) {
        *pppArgv = ExArgv_wargvToUtf8(*pArgc, ppWargv);
        ExArgv_FreeW(ppWargv);
    }
    return *pppArgv != NULL;
}

#endif  // EXARGV_USE_WCHAR_TO_UTF8
#endif  // EXARGV_USE_WCHAR

#endif  // EXARGV_FOR_WINMAIN


// ===========================================================================

#if defined(EXARGV_USE_WCHAR_TO_UTF8)   \
    || defined(EXARGV_USE_WCHAR) && (EXARGV_USE_RESFILE || EXARGV_USE_CONFIG)

#define U8F_WCS_FROM_MBS(d,dl,s,sl) MultiByteToWideChar(65001,0,(s),(int)(sl),(d),(int)(dl))
#define U8F_MBS_FROM_WCS(d,dl,s,sl) WideCharToMultiByte(65001,0,(s),(int)(sl),(d),(int)(dl),0,0)

/** whar_t 文字列の argv を utf8　文字列に変換.
 *  argc が 0 の場合は終端 NULL ポがあるものとして処理.
 */
char** ExArgv_wargvToUtf8(int argc, wchar_t* ppWargv[])
{
    char**  av;
    int     i;
    assert( ppWargv != 0 );

    if (argc == 0) {
        while (ppWargv[argc])
            ++argc;
    }
    av  = EXARGV_ALLOC(char*, argc + 2);
    if (av == NULL)
        return NULL;
    for (i = 0; i < argc; ++i)
        av[i] = ExArgv_u8strdupFromWcs( ppWargv[i] );
    av[argc]   = NULL;
    av[argc+1] = NULL;
    return av;
}

/**
 */
char*  ExArgv_u8strdupFromWcs(wchar_t const* wcs) {
    size_t  len, wlen;
    char* u8s;
    if (!wcs)
        wcs = L"";
    wlen = wcslen(wcs);
    len  = U8F_MBS_FROM_WCS(NULL,0, wcs, wlen);
    u8s  = EXARGV_ALLOC(char, len + 2);
    if (u8s)
        U8F_MBS_FROM_WCS(u8s, len + 1, wcs, wlen + 1);
    return u8s;
}

/** utf8 文字列の argv を wchar_t　文字列に変換.
 *  argc が 0 の場合は終端 NULL ポがあるものとして処理.
 */
wchar_t** ExArgv_u8argvToWcs(int argc, char* ppArgv[])
{
    wchar_t** av;
    int       i;
    assert( ppArgv != 0 );

    if (argc == 0) {
        while (ppArgv[argc])
            ++argc;
    }
    av = EXARGV_ALLOC(wchar_t*, argc + 2);
    if (av == NULL)
        return NULL;
    for (i = 0; i < argc; ++i)
        av[i] = ExArgv_wcsdupFromUtf8( ppArgv[i] );
    av[argc]   = NULL;
    av[argc+1] = NULL;
    return av;
}

/**
 */
wchar_t*  ExArgv_wcsdupFromUtf8(char const* u8s) {
    size_t  len, wlen;
    wchar_t* wcs;
    if (!u8s)
        u8s = "";
    len  = strlen(u8s);
    wlen = U8F_WCS_FROM_MBS(NULL,0, u8s, len);
    wcs  = EXARGV_ALLOC(wchar_t, wlen + 1);
    if (wcs)
        U8F_WCS_FROM_MBS(wcs, wlen + 1, u8s, len + 1);
    return wcs;
}


#endif  // EXARGV_USE_WCHAR_TO_UTF8



// ===========================================================================

/**
 */
static inline int ExArgv_isDirSep(int c)
{
  #if defined DOSWIN32
    return c == T('/') || c == T('\\');
  #else
    return c == T('/');
  #endif
}


/**
 */
static void str_l_cpy(char_t* d, char_t const* s, size_t l)
{
    char_t const*   e = d + l - 1;
    while (d < e && *s) {
        *d++ = *s++;
    }
    *d = T('\0');
}


/**
 */
static void str_l_cat(char_t* d, char_t const* s, size_t l)
{
    char_t const*   e = d + l - 1;
    while (d < e && *d) {
        ++d;
    }
    while (d < e && *s) {
        *d++ = *s++;
    }
    *d = T('\0');
}

#if EXARGV_USE_WC

/** ワイルドカード文字が混じっているか?
 */
static unsigned  ExArgv_fname_isWildCard(char_t const* s) {
  #if defined DOSWIN32
    unsigned    rc = 0;
    unsigned    c;
    while ((c = *s++) != 0) {
        if (c == T('*')) {
            if (*s == T('*')) {
                return 2;
            }
            rc = 1;
        } else if (c == T('?')) {
            rc = 1;
        }
    }
    return rc;
  #else // linux(fnmatch)
    //return strpbrk(s, "*?[]\\") != 0;
    unsigned    rc = 0;
    unsigned    bc = 0;
    unsigned    c;
    while ((c = *s++) != 0) {
        if (bc == 0) {
            if (c == T('*')) {
                if (*s == T('*')) {
                    return 2;
                }
                rc = 1;
            } else if (c == T('?')) {
                rc = 1;
            } else if (c == T('[')) {
                rc = 1;
                bc = 1;
                if (*s == T(']'))
                    ++s;
            }
          #if 0
            else if (c == T('\\') && *s) {
                ++s;
            }
          #endif
        } else if (c == T(']')) {
            bc = 0;
        }
    }
    return rc;
  #endif
}


/** リカーシブ指定の**があれば、*一つにする.
 */
static inline void  ExArgv_fname_removeRecChr(char_t* d, char_t const* s)
{
    char_t  c;
    while ((c = *s++) != 0) {
        if (c == T('*') && *s == T('*')) {
            ++s;
        }
        *d++ = c;
    }
    *d = 0;
}

#endif  // EXARGV_USE_WC


#if 0 //defined EXARGV_USE_CHK_CHR
/** 引数文字列配列に、レスポンスファイル指定、ワイルドカード指定、リカーシブ指定があるかチェック.
 */
static unsigned ExArgv_checkWcResfile(int argc, char_t** argv)
{
    int         i;
    unsigned    rc    = 0;
 #if EXARGV_USE_WC
    BOOL        optCk = 1;
 #endif

    for (i = 1; i < argc; ++i) {
        char_t const* p = argv[i];
      #if EXARGV_USE_RESFILE
        if (*p == T('@')) {
            rc |= 4;    // レスポンスファイル指定.
        } else
      #endif
        {
          #if EXARGV_USE_WC
            if (!optCk || ExArgv_isOpt(*p) == 0) {
                int mode = ExArgv_fname_isWildCard(p);
                s_ExArgv_wildMode |= mode;
                if (mode > 0) {
                    rc |= 1;    // ワイルドカード指定.
                    if (mode == 2)
                        rc |= 2;
                }
            } else if (*p == '-' && p[1] == '-' && p[2] == 0) {
                optCk = 0;
            }
          #endif
        }
    }
    return rc;
}
#endif


// -    -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

#ifdef EXARGV_ENVNAME
/** 環境変数があれば、登録.
 */
static BOOL ExArgv_getAppEnv(char_t const* envName, ExArgv_Vector* pVec)
{
    BOOL          rc = 1;
    char_t const* env;
    if (envName == 0 || envName[0] == 0)
        return 1;
    env = GET_ENV(envName);
    if (env && env[0]) {
        char_t* arg = EXARGV_ALLOC(char_t, FILEPATH_SZ + 4);
        if (arg == NULL)
            return 0;
        while ( (env = ExArgv_fname_scanArgStr(env, arg, FILEPATH_SZ)) != NULL ) {
            char_t const* p = arg;
          #if EXARGV_USE_WC
            s_ExArgv_wildMode |= ExArgv_fname_isWildCard(p);
          #endif
            if (ExArgv_Vector_push( pVec, p ) == 0) {
                rc = 0;
                break;
            }
        }
        ExArgv_free(arg);
    }
    return rc;
}
#endif


#if EXARGV_USE_CONFIG
/** コンフィグファイルの読み込み.
 */
static BOOL ExArgv_getCfgFile(char_t const* exeName, ExArgv_Vector* pVec)
{
    char_t  name[ MAX_PATH + 4 ];
    char_t* p = name;

    // 実行ファイル名からコンフィグパス名を生成.
  #ifdef DOSWIN32
    str_l_cpy(p, exeName, MAX_PATH-10);
  #else
    *p++ = T('~'), *p++ = T('/'), *p++ = T('.');
    str_l_cpy(p, ExArgv_fname_baseName(exeName), MAX_PATH - 3 - 10);
  #endif

    p = STR_R_CHR(p, T('.'));
    if (p)
        str_l_cpy(p, MAC_TO_STR(EXARGV_CONFIG_EXT), 10);
    else
        str_l_cpy(name+STR_LEN(name), MAC_TO_STR(EXARGV_CONFIG_EXT), 10);
    return ExArgv_getResFile(name, pVec, 1);
}
#endif


#if EXARGV_USE_RESFILE || EXARGV_USE_CONFIG

static size_t ExArgv_fileSizeAW(char_t const* fpath);
static void*  ExArgv_fileLoadMallocAW(char_t const* fpath, size_t* pSize);

/** レスポンスファイルを(argc,argv)を pVec に変換.
 * レスポンスファイルやワイルドカード、リカーシブ等の処理も行う.
 */
static BOOL ExArgv_getResFile(char_t const* fpath, ExArgv_Vector* pVec, BOOL notFoundOk)
{
    unsigned char*  s;
    unsigned char*  src;
    unsigned char*  src_e;
    unsigned char*  dst;
    unsigned char*  dst_e;
    size_t          bytes = 0;
    unsigned        lno = 1;
    BOOL            rc = 1;

    src = (unsigned char*)ExArgv_fileLoadMallocAW(fpath, &bytes);
    if (src == NULL) {
        if (notFoundOk) {
            return 1;
        } else {
         #if defined(EXARGV_USE_WCHAR)
            char* fnm = ExArgv_u8strdupFromWcs(fpath);
            ERR_PUTS( fnm );
            ExArgv_free(fnm);
         #else
            ERR_PUTS( fpath );
         #endif
            ERR_PUTS(" : Response-file is not opened.\n");
            return 0;
        }
    }

    dst   = EXARGV_ALLOC(unsigned char, FILEPATH_SZ*2+2);
    if (dst == NULL)
        return 0;
    dst_e = dst + FILEPATH_SZ*2;

    src_e = src + bytes;
    s     = src;

    while (s < src_e) {
        unsigned char*  d;
        unsigned char*  p;
        BOOL            dst_ovr = 0;
        unsigned        f = 0;
        unsigned        c;

        while (s < src_e && *s != '\n' && (*s <= 0x20 || *s == 0x7f))
            ++s;
        if (*s == '\n') {
            ++lno;
            continue;
        }
        if (*s == '#' /*|| *s == ';'*/) {
            while (s < src_e && *s != '\n')
                ++s;
            ++lno;
            continue;
        }
        d = dst;
        p = s;
        do {
            c = *p++;
            if (c == '\0' || c == '\n')
                break;
            if (c == '"') {
                f ^= 1;                     // "の対の間は空白をファイル名に許す.ためのフラグ.
                // ちょっと気持ち悪いが、Win(XP)のcmd.exeの挙動に合わせてみる.
                // (ほんとにあってるか、十分には調べてない)
                if (*p == '"' && f == 0) // 閉じ"の直後にさらに"があれば、それはそのまま表示する.
                    ++p;
                else
                    continue;               // 通常は " は省いてしまう.
            }
            if (d < dst_e)
                *d++ = c;
            else
                dst_ovr = 1;
        } while (c > 0x20 || (f && c == ' '));
        --p;
        if (d > dst)
            *--d = 0;

        if (dst_ovr) {
         #if defined(EXARGV_USE_WCHAR)
            char* fnm = ExArgv_u8strdupFromWcs(fpath);
         #else
            char const* fnm = fpath;
         #endif
            fprintf(stderr, "%s (%u): Argument too long.\n", fnm, lno);
         #if defined(EXARGV_USE_WCHAR)
            ExArgv_free(fnm);
         #endif
        }

        {
          #if defined(EXARGV_USE_WCHAR)
            char_t* arg = ExArgv_wcsdupFromUtf8((char*)dst);
          #else
            char*   arg = (char*)dst;
          #endif
            // 再帰検索指定,ワイルドカードの有無をチェック.
          #if EXARGV_USE_WC
            s_ExArgv_wildMode |= ExArgv_fname_isWildCard(arg);
          #endif
            rc = ExArgv_Vector_push(pVec, arg);
          #if defined(EXARGV_USE_WCHAR)
            ExArgv_free(arg);
          #endif
            if (rc == 0)
                break;
        }
    }
    ExArgv_free(dst);
    return rc;
}

//
static size_t ExArgv_fileSizeAW(char_t const* fpath)
{
 #if defined(_WIN32)
    if (fpath) {
        WIN32_FIND_DATA d;
        HANDLE h = FindFirstFile(fpath, &d);
        if (h != INVALID_HANDLE_VALUE) {
            FindClose(h);
            #if defined(_WIN64)
                return (((size_t)d.nFileSizeHigh<<32) | (size_t)d.nFileSizeLow);
            #else
                return (d.nFileSizeHigh) ? (size_t)-1 : d.nFileSizeLow;
            #endif
        }
    }
    return (size_t)(-1);
 #else
    struct stat st;
    int   rc = stat(fpath, &st);
    return (rc == 0) ? (size_t)st.st_size : (size_t)-1;
 #endif
}

#if defined(EXARGV_USE_WCHAR_TO_UTF8)
size_t ExArgv_fileSize(char const* fpath)
{
    size_t   len = (size_t)-1;
    wchar_t* wpath = ExArgv_wcsdupFromUtf8(fpath);
    if (wpath) {
        len = ExArgv_fileSizeAW(wpath);
        ExArgv_free(wpath);
    }
    return len;
}
#else
size_t ExArgv_fileSize(char_t const* fpath)
{
    return ExArgv_fileSizeAW(fpath);
}
#endif

/** Load the file, add '\0'*4 to the end and return malloced memory.
 */
static void* ExArgv_fileLoadMallocAW(char_t const* fpath, size_t* pSize)
{
    char*  buf;
    size_t rbytes;
    size_t bytes = ExArgv_fileSizeAW(fpath);
    if (bytes == (size_t)(-1))
        return NULL;

    buf    = EXARGV_ALLOC(char, bytes + 4);
    if (buf == NULL)
        return NULL;

 #if defined(_WIN32)
    {
        DWORD  r   = 0;
        HANDLE hdl = CreateFile(fpath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (!hdl || hdl == INVALID_HANDLE_VALUE) {
            ExArgv_free(buf);
            return NULL;
        }
        if (!ReadFile(hdl, buf, (DWORD)bytes, &r, 0))
            r = 0;
        rbytes = r;
        CloseHandle(hdl);
    }
 #elif 1
    {
        FILE* fp = fopen(fpath, "rb");
        if (fp == NULL) {
            ExArgv_free(buf);
            return NULL;
        }
        rbytes = fread(buf, 1, bytes, fp);
        fclose(fp);
    }
 #else
    {
        int fd = open(fpath, O_RDONLY);
        if (fd == -1) {
            ExArgv_free(buf);
            return NULL;
        }
        rbytes = read(fd, buf, bytes);
        close(fd);
    }
 #endif
    if (rbytes == bytes) {
        buf[bytes] = buf[bytes+1] = buf[bytes+2] = buf[bytes+3] = 0;
    } else {
        ExArgv_free(buf);
        buf   = NULL;
        pSize = NULL;
    }
    if (pSize)
        *pSize = bytes;
    return buf;
}

#if defined(EXARGV_USE_WCHAR_TO_UTF8)
void* ExArgv_fileLoadMalloc(char const* fpath, size_t* pSize)
{
    void*    p     = NULL;
    wchar_t* wpath = ExArgv_wcsdupFromUtf8(fpath);
    if (wpath) {
        p = ExArgv_fileLoadMallocAW(wpath, pSize);
        ExArgv_free(wpath);
    }
    return p;
}
#else
void* ExArgv_fileLoadMalloc(char_t const* fpath, size_t* pSize)
{
    return ExArgv_fileLoadMallocAW(fpath, pSize);
}
#endif

#endif


// ===========================================================================

#if EXARGV_USE_WC
/** ワイルドカード、再帰処理.
 */
static BOOL ExArgv_wildCard(ExArgv_Vector* pVec)
{
    char_t**        pp;
    char_t**        ee;
    int             mode;
    BOOL            optCk = 1;
    char_t*         name;
    ExArgv_Vector*  wk;

    wk = ExArgv_Vector_create( pVec->size+1 );
    if (wk == NULL)
        return 0;
    name = EXARGV_ALLOC(char_t, FILEPATH_SZ+4);
    if (name == NULL)
        return 0;
    ee = pVec->buf + pVec->size;
    for (pp = pVec->buf; pp != ee; ++pp) {
        char_t const* s = *pp;
      #if EXARGV_USE_WC
        if (optCk && ExArgv_isOpt(*s)) {
            if (*s == '-' && s[1] == '-' && s[2] == 0)
                optCk = 0;
            continue;
        }
        if ( (pp != pVec->buf)                              // 初っ端以外([0]は実行ファイル名なので検索させない)のときで,
            && ((mode = ExArgv_fname_isWildCard( s )) != 0) // ワイルドカード指定のありのとき.
         ){
            int recFlag = (mode >> 1) & 1;
          #if EXARGV_USE_WC_REC
            if (s[0] == T('*') && s[1] == T('*') && ExArgv_isDirSep(s[2])) {
                recFlag = 1;
                s += 3;
            } else
          #endif
            if (recFlag) {
                ExArgv_fname_removeRecChr(name, s);
                s = name;
            }
            if (ExArgv_Vector_findFname(wk, s, recFlag) < 0) {
                wk = NULL;
                break;
            }

        } else {
            if (ExArgv_Vector_push( wk, s ) == 0) {
                wk = NULL;
                break;
            }
        }
      #else
        if (ExArgv_Vector_push( wk, s ) == 0) {
            wk = NULL;
            break;
        }
      #endif
    }

    ExArgv_free(name);

    // 今回生成したものを、pVecに設定.
    if (wk) {
        // 元のリストを開放.
        for (pp = pVec->buf; pp != ee; ++pp) {
            char_t* p = *pp;
            if (p)
                ExArgv_free(p);
        }
        ExArgv_free(pVec->buf);

        pVec->buf  = wk->buf;
        pVec->size = wk->size;
        pVec->capa = wk->capa;
        // 作業に使ったメモリを開放.
        ExArgv_free(wk);
    }
    return 1;
}
#endif


#if (defined EXARGV_TOSLASH) || (defined EXARGV_TOBACKSLASH)
/** ファイル(パス)名中の \ / の変換. -で始まるオプション文字列は対象外.
 *  最近のwin環境ではどちらの指定でもokなので、無理に変換する必要なし.
 *  (オプション中にファイル名があると結局自前で変換せざるえないので、ここでやらないほうが無難かも)
 */
static void ExArgv_convBackSlash(ExArgv_Vector* pVec)
{
    char_t**    pp;
    char_t**    ee = pVec->buf + pVec->size;
    BOOL        optCk = 1;

    for (pp = pVec->buf; pp != ee; ++pp) {
        char_t* s = *pp;
        if (optCk && ExArgv_isOpt(*s)) {    // オプションなら、変換しない.
            if (*s == '-' && s[1] == '-' && s[2] == 0)
                optCk = 0;
            continue;
        }
        // オプション以外の文字列で,
      #if (defined EXARGV_TOSLASH)
        ExArgv_fname_backslashToSlash(s);       // \ を / に置換.
      #else
        ExArgv_fname_slashToBackslash(s);       // / を \ に置換.
      #endif
    }
}
#endif


/** pVecから、(argc,argv)を生成. ppVecは開放する.
 */
static char_t** ExArgv_VectorToArgv(ExArgv_Vector** ppVec, int* pArgc, char_t*** pppArgv)
{
    ExArgv_Vector*  pVec;
    char_t**        av;
    int             ac;

    assert( pppArgv != 0 && pArgc != 0 && ppVec != 0 );

    *pppArgv = NULL;
    *pArgc   = 0;

    pVec     = *ppVec;
    if (pVec == NULL)
        return NULL;

    ac       = (int)pVec->size;
    if (ac == 0)
        return NULL;

    // char_t*配列のためのメモリを取得.
    *pArgc   = ac;
    av       = EXARGV_ALLOC(char_t*, ac + 2);
    if (!av)
        return NULL;
    *pppArgv = av;

    memcpy(av, pVec->buf, sizeof(char_t*) * ac);

    av[ac]   = NULL;
    av[ac+1] = NULL;

    // 作業に使ったメモリを開放.
    ExArgv_free(pVec->buf);
    ExArgv_free(pVec);
    *ppVec   = NULL;

    return (char_t**)av;
}

/**
 */
static char_t** ExArgv_Vector_release(ExArgv_Vector* pVec)
{
    if (pVec) {
        if (pVec->buf) {
            size_t i;
            for (i = 0; i < pVec->size; ++i) {
                ExArgv_free(pVec->buf[i]);
            }
            ExArgv_free(pVec->buf);
        }
        ExArgv_free(pVec);
    }
    return NULL;
}

#if defined(EXARGV_USE_WCHAR)
/**
 */
void ExArgv_FreeW(wchar_t*** pppArgv)
{
    ExArgv_Free((char***)pppArgv);
}
#endif

/**
 */
void ExArgv_Free(char*** pppArgv)
{
    void** pp = (void**)*pppArgv;
    while (*pp) {
        void* p = *pp;
        if (p)
            free(p);
        ++pp;
    }
    free(*pppArgv);
    *pppArgv = 0;
}


// ===========================================================================

#if defined DOSWIN32 == 0 && defined EXARGV_USE_WCHAR == 0   // 環境変数 LANG=ja_JP.SJIS のような状態を前提.

static unsigned char        s_shift_char_type = 0;

/**
 */
static void  ExArgv_fname_check_locale()
{
    const char*         lang = getenv("LANG");
    s_shift_char_type  = 1;
    if (lang) {
        // ja_JP.SJIS のような形式であることを前提にSJIS,big5,gbkかをチェック.
        const char*     p    = strrchr(lang, '.');
        if (p) {
            ++p;
            // 0x5c対策が必要なencodingかをチェック. (sjis以外は未確認)
            if (strncasecmp(p, "sjis", 4) == 0) {
                s_shift_char_type   = 2;
            } else if (strncasecmp(p, "big5", 4) == 0) {
                s_shift_char_type   = 3;
            } else if (strncasecmp(p, "gbk", 3) == 0 || strncasecmp(p, "gb18030", 7) == 0) {
                s_shift_char_type   = 4;
            }
        }
    }
}

/**
 */
static int ExArgv_fname_is_mbblead(unsigned c) {
  RETRY:
    switch (s_shift_char_type) {
    case 0 /* INIT */: ExArgv_fname_check_locale(); goto RETRY;
    case 2 /* SJIS */: return ((c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC));
    case 3 /* BIG5 */: return ((c >= 0xA1 && c <= 0xC6) || (c >= 0xC9 && c <= 0xF9));
    case 4 /* GBK  */: return (c >= 0x81 && c <= 0xFE);
    default:           return 0;
    }
}
#endif


/// 文字 C が MS全角の１バイト目か否か. (utf8やeucは \ 問題は無いので 0が帰ればok)
#if defined EXARGV_USE_WCHAR || defined EXARGV_USE_MBC == 0
//#define ExArgv_FNAME_ISMBBLEAD(c)     (0)
#elif defined _WIN32
 #define ExArgv_FNAME_ISMBBLEAD(c)      IsDBCSLeadByte((unsigned)c)
#elif defined HAVE_MBCTYPE_H
 #define ExArgv_FNAME_ISMBBLEAD(c)      _ismbblead(c)
#else
 #define ExArgv_FNAME_ISMBBLEAD(c)      ((c) >= 0x80 && ExArgv_fname_is_mbblead(c))
#endif


/// 次の文字へポインタを進める.
#if defined EXARGV_USE_WCHAR || defined EXARGV_USE_MBC == 0
#define ExArgv_FNAME_CHARNEXT(p)        ((p) + 1)
#elif  defined _WIN32
#define ExArgv_FNAME_CHARNEXT(p)        (TCHAR*)CharNext((TCHAR*)(p))
#else
#define ExArgv_FNAME_CHARNEXT(p)        ((p) + 1 + (ExArgv_FNAME_ISMBBLEAD(*(unsigned char*)(p)) && (p)[1]))
#endif


#if defined EXARGV_TOSLASH
/** filePath中の \ を / に置換.
 */
static char_t   *ExArgv_fname_backslashToSlash(char_t filePath[])
{
    char_t *p = filePath;
    while (*p != T('\0')) {
        if (*p == T('\\')) {
            *p = T('/');
        }
        p = ExArgv_FNAME_CHARNEXT(p);
    }
    return filePath;
}
#endif


#if defined EXARGV_TOBACKSLASH
/** filePath中の / を \ に置換.
 */
static char_t   *ExArgv_fname_slashToBackslash(char_t filePath[])
{
    char_t *p;
    for (p = filePath; *p != T('\0'); ++p) {
        if (*p == T('/')) {
            *p = T('\\');
        }
    }
    return filePath;
}
#endif


#if defined EXARGV_ENVNAME || defined EXARGV_FOR_WINMAIN
/** コマンドラインで指定されたファイル名として、""を考慮して,
 *  空白で区切られた文字列(ファイル名)を取得.
 *  @return スキャン更新後のアドレスを返す。strがEOSだったらNULLを返す.
 */
static char_t *ExArgv_fname_scanArgStr(char_t const *str, char_t arg[], int argSz)
{
    const uchar_t*  s = (const uchar_t *)str;
    char_t*         d = arg;
    char_t*         e = d + argSz;
    unsigned        f = 0;
    int             c;

    if (s == 0)
        return NULL;

    assert( str != 0 && arg != 0 && argSz > 1 );

    // 空白をスキップ.
    // while ( *s < 0x7f && isspace(*s) )
    while ((0 < *s && *s <= 0x20) || *s == 0x7f)    // ascii,sjis,utf8,utf16 ならこれでいい...
        ++s;

    if (*s == T('\0'))  // EOSだったら、見つからなかったとしてNULLを返す.
        return NULL;

    do {
        c = *s++;
        if (c == T('"')) {
            f ^= 1;                     // "の対の間は空白をファイル名に許す.ためのフラグ.

            // ちょっと気持ち悪いが、Win(XP)のcmd.exeの挙動に合わせてみる.
            // (ほんとにあってるか、十分には調べてない)
            if (*s == T('"') && f == 0) // 閉じ"の直後にさらに"があれば、それはそのまま表示する.
                ++s;
            else
                continue;               // 通常は " は省いてしまう.
        }
        if (d < e) {
            *d++ = (char_t)c;
        }
    } while (c >= 0x20 && (c != T(' ') || f != 0));
    *--d  = T('\0');
    --s;
    return (char_t *)s;
}
#endif


#if EXARGV_USE_WC || EXARGV_USE_CONFIG
/** ファイルパス名中のディレクトリを除いたファイル名の位置を返す.
 */
static char_t*  ExArgv_fname_baseName(char_t const* adr)
{
    char_t const *p = adr;
    while (*p != T('\0')) {
        if (*p == T(':') || ExArgv_isDirSep(*p)) {
            adr = p + 1;
        }
        p = ExArgv_FNAME_CHARNEXT(p);
    }
    return (char_t*)adr;
}
#endif


#if EXARGV_USE_WC

/** srchNameで指定されたパス名(ワイルドカード文字対応) にマッチするパス名を全て pVec に入れて返す.
 *  recFlag が真なら再帰検索を行う.
 */
static int  ExArgv_Vector_findFname(ExArgv_Vector* pVec, char_t const* srchName, int recFlag)
{
  #if defined _WIN32        // ※dos未対応(同様に作成可能だけど)
    unsigned            num         = 0;
    WIN32_FIND_DATA*    pFindData   = EXARGV_ALLOC(WIN32_FIND_DATA, 1);
    HANDLE              hdl         = FindFirstFile(srchName, pFindData);
    char_t*             pathBuf;
    char_t*             baseName;
    size_t              baseNameSz;

    if (hdl == INVALID_HANDLE_VALUE)
        return 0;

    pathBuf  = EXARGV_ALLOC(char_t, FILEPATH_SZ);
    if (pathBuf == NULL)
        return -1;

    str_l_cpy(pathBuf, srchName, FILEPATH_SZ);

    baseName    = ExArgv_fname_baseName(pathBuf);
    *baseName   = T('\0');
    baseNameSz  = FILEPATH_SZ - STR_LEN(pathBuf);
    assert(baseNameSz >= MAX_PATH);

    // ファイル名を取得. ※ 隠しファイルは対象外にしておく.
    do {
        str_l_cpy(baseName, pFindData->cFileName, baseNameSz);
        if ((pFindData->dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_HIDDEN)) == 0) {
            if (ExArgv_Vector_push( pVec, pathBuf ) == 0) {
                pVec = NULL;
                num  = -1;
                break;
            }
            ++num;
        }
    } while (FindNextFile(hdl, pFindData) != 0);
    FindClose(hdl);

   #if EXARGV_USE_WC_REC
    // ディレクトリ再帰でファイル名を取得.
    if (num >= 0 && recFlag && baseNameSz >= 16) {
        char_t const* srch = ExArgv_fname_baseName(srchName);
        str_l_cpy(baseName, T("*"), 4);
        hdl = FindFirstFile(pathBuf, pFindData);
        if (hdl != INVALID_HANDLE_VALUE) {
            do {
                str_l_cpy(baseName, pFindData->cFileName, baseNameSz);
                if ((pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    if (STR_CMP(baseName, T(".")) == 0 || STR_CMP(baseName, T("..")) == 0
                        || (pFindData->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))           // 隠しフォルダは対象外.
                    {
                        ;
                    } else {
                        int d;
                        str_l_cat(baseName, DIRSEP_STR, baseNameSz);
                        str_l_cat(baseName, srch      , baseNameSz);
                        d = ExArgv_Vector_findFname(pVec, pathBuf, 1);
                        if (d < 0) {
                            pVec = NULL;
                            num  = -1;
                            break;
                        }
                        num += d;
                    }
                }
            } while (FindNextFile(hdl, pFindData) != 0);
            FindClose(hdl);
        }
    }
   #elif defined _MSC_VER || defined __WATCOMC__ || defined __BORLANDC__
    recFlag;
   #endif

    ExArgv_free(pathBuf);
    ExArgv_free(pFindData);
    return num;

  #else // linux/unix系...
    struct dirent** namelist = 0;
    unsigned        num      = 0;
    char_t*         pathBuf  = EXARGV_ALLOC(char_t, FILEPATH_SZ);
    int             dirNum;
    char_t*         srchBase = ExArgv_fname_baseName(srchName);
    char_t*         baseName;
    size_t          baseNameSz;
    int             flag = 0;

    if (pathBuf == NULL)
        return -1;

    str_l_cpy(pathBuf, srchName, FILEPATH_SZ);

    baseName    = ExArgv_fname_baseName(pathBuf);

    if (baseName == pathBuf) {          // ディレクトリ部が無い場合.
        str_l_cpy(pathBuf, T("./"), 3); // カレント指定を一時的に設定.
        baseName = pathBuf+2;
        flag     = 1;
    }
    *baseName   = 0;
    baseNameSz  = FILEPATH_SZ - STR_LEN(pathBuf);
    assert(baseNameSz >= MAX_PATH);

    // ディレクトリエントリの取得.
    baseName[-1] = 0;
    dirNum = scandir(pathBuf, &namelist, 0, alphasort);
    baseName[-1] = T('/');

    if (flag) { // 一時的なカレント指定だったならば、捨てる.
        baseName  = pathBuf;
        *baseName = T('\0');
    }

    if (namelist) {
        struct stat statBuf;
        int         i;

        // ファイル名を取得.
        for (i = 0; i < dirNum; ++i) {
            struct dirent* d = namelist[i];
            if (fnmatch(srchBase, d->d_name, FNM_NOESCAPE) == 0) {
                str_l_cpy(baseName, d->d_name, baseNameSz);
                if (stat(pathBuf, &statBuf) >= 0) {
                    if ((statBuf.st_mode & S_IFMT) != S_IFDIR) {
                        if (ExArgv_Vector_push( pVec, pathBuf ) == 0) {
                            return -1;
                        }
                        ++num;
                    }
                }
            }
        }

       #if EXARGV_USE_WC_REC
        // ディレクトリがあれば再帰.
        if (recFlag && baseNameSz >= 16) {
            char_t const* srch = ExArgv_fname_baseName(srchName);
            for (i = 0; i < dirNum; ++i) {
                struct dirent* d = namelist[i];
                str_l_cpy(baseName, d->d_name, baseNameSz);
                if (stat(pathBuf, &statBuf) >= 0 && STR_CMP(baseName,T(".")) != 0 && STR_CMP(baseName,T("..")) !=0 ) {
                    if ((statBuf.st_mode & S_IFMT) == S_IFDIR) {
                        int d;
                        str_l_cat(baseName, T("/"), baseNameSz);
                        str_l_cat(baseName, srch  , baseNameSz);
                        d = ExArgv_Vector_findFname(pVec, pathBuf, 1);
                        if (d < 0) {
                            num = -1;
                            break;
                        }
                        num += d;
                    }
                }
            }
        }
      #endif

        // 使ったメモリを開放.
        for (i = 0; i < dirNum; ++i) {
            free( namelist[i] );
        }
        free( namelist );
    }
    ExArgv_free( pathBuf );
    return num;
  #endif
}

#endif


// ===========================================================================

/** 引数文字列リストを管理する根元を作成.
 */
static ExArgv_Vector* ExArgv_Vector_create(unsigned size)
{
    ExArgv_Vector* pVec = EXARGV_ALLOC( ExArgv_Vector, 1  );
    if (pVec) {
        size            = ((size + EXARGV_VECTOR_CAPA_BASE) / EXARGV_VECTOR_CAPA_BASE) * EXARGV_VECTOR_CAPA_BASE;
        pVec->capa      = size;
        pVec->size      = 0;
        pVec->buf       = EXARGV_ALLOC(char_t*, size);
        if (!pVec->buf) {
            ExArgv_free(pVec);
            pVec = NULL;
        }
    }
    return pVec;
}


/** 引数文字列リストに、文字列を追加.
 */
static BOOL ExArgv_Vector_push(ExArgv_Vector* pVec, char_t const* pStr)
{
    assert(pVec != 0);
    assert(pStr != 0);
    if (pStr && pVec) {
        char_t*     p;
        unsigned    capa = pVec->capa;
        if (pVec->buf == NULL)
            return 0; //assert(pVec->buf != 0);
        if (pVec->size >= capa) {   // キャパを超えていたら、メモリを確保しなおす.
            char_t**        buf;
            unsigned        newCapa = capa + EXARGV_VECTOR_CAPA_BASE;
          #if 1 // 普通はコレ以前にメモリ不足になるだろうが一応.
            if (newCapa < capa) {   // 溢れたらエラー...
                if (capa < 0xFFFFFFFF) {
                    newCapa = 0xFFFFFFFF;
                } else {
                    ERR_PUTS("Too many arguments.\n");
                    ExArgv_Vector_release(pVec);
                    return 0; //exit(1);
                }
            }
          #endif
            //x printf("!  %p: %p %d %d ::%s\n", pVec, pVec->buf, pVec->capa, pVec->size, pStr);
            assert(pVec->size == capa);
            pVec->capa  = newCapa;
            buf         = EXARGV_ALLOC(char_t*, pVec->capa);
            if (buf == NULL) {
                ExArgv_Vector_release(pVec);
                return 0;
            }
            memcpy(buf, pVec->buf, capa*sizeof(void*));
            memset(buf+capa, 0, EXARGV_VECTOR_CAPA_BASE*sizeof(void*));
            ExArgv_free(pVec->buf);
            pVec->buf   = buf;
        }
        assert(pVec->size < pVec->capa);
        pVec->buf[ pVec->size ] = p = ExArgv_strdup(pStr);
        if (!p) {
            ExArgv_Vector_release(pVec);
            return 0;
        }
        ++ pVec->size;
        //x printf("!!  %p: %p %d %d ::%s\n", pVec, pVec->buf, pVec->capa, pVec->size, pStr);
    }
    return 1;
}


// ===========================================================================

/** malloc
 */
static void* ExArgv_alloc(unsigned size)
{
    void* p = malloc(size);
    if (p == NULL) {
        ERR_PUTS("Not enough memory.\n");
        return NULL; //exit(1);
    }
    memset(p, 0, size);
    return p;
}


/** strdup
 */
static char_t* ExArgv_strdup(char_t const* s)
{
    size_t   sz = STR_LEN(s) + 1;
    char_t*  p  = (char_t*)malloc(sz * sizeof(char_t));
    if (p == NULL) {
        ERR_PUTS("Not enough memory.\n");
        return NULL; //exit(1);
    }
    return (char_t*) memcpy(p, s, sz*sizeof(char_t));
}


/** free
 */
static void ExArgv_free(void* s)
{
    if (s)
        free(s);
}
