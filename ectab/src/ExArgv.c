/**
 *  @file   ExArgv.cpp
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
 // 2009 再帰指定を**にすることで、仕様を単純化.
 // 2023 UTF-8 対処. vcpkgがWin用に必ず_WINDOWSを定義するためWinMain指定変更.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef assert
#include <assert.h>
#endif

#if defined UNICODE && defined EXARGV_USE_WCHAR == 0
#define EXARGV_USE_WCHAR
#endif

#ifdef EXARGV_USE_WCHAR
#include <wchar.h>
#endif

// ヘッダ(というかユーザ設定)の読込.
#ifndef EXARGV_INCLUDED
 #include "ExArgv.h"
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
#define EXARGV_CONFIG_EXT   ".cfg"              ///< コンフィグファイル入力有の時の拡張子. 拡張子は4文字以内のこと.
#endif

#if 0 //ndef EXARGV_USE_FULLPATH_ARGV0
#define EXARGV_USE_FULLPATH_ARGV0   1           ///< argv[0] の実行ファイル名をフルパスにする/しない. win環境のみ.
#endif

//#define EXARGV_TOSLASH                        ///< 定義すれば、filePath中の \ を / に置換.
//#define EXARGV_TOBACKSLASH                    ///< 定義すれば、filePath中の / を \ に置換.
//#define EXARGV_USE_SLASH_OPT                  ///< 定義すれば、/ もオプション開始文字とみなす.
//#define EXARGV_USE_SETARGV                    ///< 実験. 定義すると、setargvの代用品としてコンパイル(ExArgv_get等無し)



// ===========================================================================
// 辻褄あわせ.

/*
#if defined _MSDOS || defined __DOS__
 #ifndef MSODS
  #define MSDOS     1                           // DOS系定義. ※といっても16ビットDOSには未対応.
 #endif
#endif
*/

#if defined _WIN32 || defined MSODS
 #define DOSWIN32   1                           // DOS/WIN系なら定義.
#endif


#if defined _WIN32
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
 #include <fnmatch.h>
#endif


#undef BOOL
#if defined __cplusplus
 #define BOOL           bool
 #define EXTERN_C       extern "C"
#else
 #define BOOL           int
 #define EXTERN_C       extern
 #if defined DOSWIN32
  #define inline        __inline
 #endif
#endif


#ifndef MAX_PATH
 #ifdef _MAX_PATH
  #define MAX_PATH      _MAX_PATH
 #else
  #if defined MSDOS	//DOSWIN32
   #define MAX_PATH     260
  #else
   #define MAX_PATH     1024
  #endif
 #endif
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
typedef FILE*           FILEPTR;
#define FILEPTR_IS_OK(a) (a)
#define FOPEN_RB(fnm)   _wfopen(fnm, T("rb"))
#define FOPEN_RT(fnm)   _wfopen(fnm, T("rt"))
#define FCLOSE(fp)      fclose(fp)
#define STDERR          stderr
#define FPRINTF         fwprintf
#define FERROR(fp)      ferror(fp)
#define FGETS(b,l,h)    fgetws(b,l,h)
#else
#define T(x)            x
typedef char            char_t;
typedef unsigned char   uchar_t;
#define STR_LEN(a)      strlen(a)
#define STR_CMP(a,b)    strcmp(a,b)
#define STR_R_CHR(a,b)  strrchr(a,b)
#define GET_ENV(s)      getenv(s)
typedef FILE*           FILEPTR;
#define FILEPTR_IS_OK(a) (a)
#define FOPEN_RB(fnm)   fopen(fnm, T("rb"))
#define FOPEN_RT(fnm)   fopen(fnm, T("rt"))
#define FCLOSE(fp)      fclose(fp)
#define STDERR          stderr
#define FPRINTF         fprintf
#define FERROR(fp)      ferror(fp)
#define FGETS(b,l,h)    fgets(b,l,h)
#endif



// ===========================================================================

enum { FILEPATH_SZ              = (MAX_PATH*2 > 8192) ? MAX_PATH*2 : 8192 };
enum { EXARGV_VECTOR_CAPA_BASE  = 4096 };


#ifdef EXARGV_TOBACKSLASH
#define DIRSEP_STR          T("\\")
#else
#define DIRSEP_STR          T("/")
#endif


#if EXARGV_USE_WC
static unsigned char        s_wildMode;         ///< ワイルドカード文字列が設定されていたらon.
#endif

#if (EXARGV_USE_WC || EXARGV_USE_RESFILE) && !EXARGV_USE_CONFIG && !defined(EXARGV_ENVNAME) \
        && !defined(EXARGV_FOR_WINMAIN) && !defined(EXARGV_USE_SETARGV) \
        && !defined EXARGV_TOSLASH && !defined EXARGV_TOBACKSLASH
    #define EXARGV_USE_CHK_CHR
#endif


// ===========================================================================

typedef struct ExArgv_Vector {
    char_t**        buf;
    unsigned        size;
    unsigned        capa;
} ExArgv_Vector;

static ExArgv_Vector *ExArgv_Vector_create(unsigned size);
static void         ExArgv_Vector_push(ExArgv_Vector* pVec, const char_t* pStr);
static void**       ExArgv_VectorToArgv(ExArgv_Vector** pVec, int* pArgc, char_t*** pppArgv);
static void*        ExArgv_alloc(unsigned size);
static char_t*      ExArgv_strdup(const char_t* s);
static void         ExArgv_free(void* s);

#if EXARGV_USE_WC
static int          ExArgv_Vector_findFname(ExArgv_Vector* pVec, const char_t* pPathName, int recFlag);
static void         ExArgv_wildCard(ExArgv_Vector* pVec);
#endif
#if defined EXARGV_FOR_WINMAIN || defined EXARGV_USE_SETARGV
static int          ExArgv_forCmdLine1(const char_t* pCmdLine, int* pArgc, char_t*** pppArgv);
#endif
#if defined EXARGV_USE_CHK_CHR
static unsigned     ExArgv_checkWcResfile(int argc, char_t** argv);
#endif
#ifdef EXARGV_ENVNAME
static void         ExArgv_getEnv(const char_t* envName, ExArgv_Vector* pVec);
#endif
#if EXARGV_USE_CONFIG
static void         ExArgv_getCfgFile(const char_t* exeName, ExArgv_Vector* pVec);
#endif
#if EXARGV_USE_RESFILE || EXARGV_USE_CONFIG
static void         ExArgv_getResFile(const char_t* fname, ExArgv_Vector* pVec, BOOL notFoundOk);
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

#if EXARGV_USE_WC || (EXARGV_USE_CONFIG && defined DOSWIN32 == 0)
static char_t*      ExArgv_fname_baseName(const char_t* adr);
#endif
#if EXARGV_USE_RESFILE || EXARGV_USE_CONFIG || defined EXARGV_ENVNAME \
    || defined EXARGV_FOR_WINMAIN || defined EXARGV_USE_SETARGV
static char_t*      ExArgv_fname_scanArgStr(const char_t* str, char_t arg[], int argSz);
#endif
#if EXARGV_USE_WC
static int          ExArgv_fname_isWildCard(const char_t* s);
#endif



// ===========================================================================

#ifdef EXARGV_USE_SETARGV
 #ifndef _MSC_VER
  #error No _MSC_VER, though EXARGV_USE_SETARGV was defined.
 #endif

#if defined EXARGV_USE_WCHAR
_CRTIMP EXTERN_C wchar_t *_wcmdln;
/** vc++ で、main()に渡される argc,argv を生成する処理(をこれに置き換える)
 */
EXTERN_C int __cdecl __wsetargv (void)
{
    return ExArgv_forCmdLine1( _wcmdln, &__argc, &__wargv);
}
#else
_CRTIMP EXTERN_C char *_acmdln;
/** vc++ で、main()に渡される argc,argv を生成する処理(をこれに置き換える)
 */
EXTERN_C int __cdecl __setargv (void)
{
    return ExArgv_forCmdLine1( _acmdln, &__argc, &__argv);
}
#endif



#elif defined EXARGV_FOR_WINMAIN

/** winアプリで、WinMain初っ端で、argc,argvを作りたいときに使うのを想定.
 */
void ExArgv_forWinMain(const char_t* pCmdLine, int* pArgc, char_t*** pppArgv)
{
    ExArgv_forCmdLine1(pCmdLine, pArgc, pppArgv);
}

#endif



#if defined EXARGV_FOR_WINMAIN || defined EXARGV_USE_SETARGV
/** 1行の文字列pCmdLine からargc,argvを生成. (先に環境変数や.cfgファイルを処理)
 */
#if defined(EXARGV_USE_WCHAR_TO_UTF8)
static int ExArgv_to_utf8_forCmdLine1(const char_t* pCmdLine, int* pArgc, char_t*** pppArgv, char*** pppUtf8s)
#else
static int ExArgv_forCmdLine1(const char_t* pCmdLine, int* pArgc, char_t*** pppArgv)
#endif
{
    ExArgv_Vector*  pVec;
    char_t          arg[ FILEPATH_SZ + 4 ];
    const char_t*   s;
    int             n;

    assert(pArgc != 0 && pppArgv != 0);
    if (pArgc == 0 || pppArgv == 0)
        return -1;

    pVec = ExArgv_Vector_create(1);                 // 作業用のリストを用意.
    if (pVec == 0)
        return -1;

    // 実行ファイル名を得て、それを初っ端に登録.
    n = GetModuleFileName(NULL, arg, FILEPATH_SZ);
    if (n > 0) {
        ExArgv_Vector_push(pVec, arg);
    } else {
        // error.
      #if defined _MSC_VER
        ExArgv_Vector_push(pVec, _pgmptr);
      #endif
    }
    if (pVec->size == 0)
        return -1;

    // 環境変数の取得.
  #ifdef EXARGV_ENVNAME
    assert(STR_LEN(EXARGV_ENVNAME) > 0);
    ExArgv_getEnv(EXARGV_ENVNAME, pVec);
  #endif

    // コンフィグファイルの読込.
  #if EXARGV_USE_CONFIG
    ExArgv_getCfgFile(pVec->buf[0], pVec );
  #endif

  #if EXARGV_USE_WC
    s_wildMode  = 0;
  #endif

    // 1行で渡されるコマンドラインを分割.
    s = pCmdLine;
    while ( (s = ExArgv_fname_scanArgStr(s, arg, FILEPATH_SZ)) != NULL ) {
        const char_t* p = arg;
      #if EXARGV_USE_RESFILE
        if (*p == T('@')) {
            ExArgv_getResFile(p+1, pVec, 0);
        } else
      #endif
        {
          #if EXARGV_USE_WC
            s_wildMode |= ExArgv_fname_isWildCard(p);
          #endif
            ExArgv_Vector_push( pVec, p );
        }
    }

  #if EXARGV_USE_WC
    if (s_wildMode)
        ExArgv_wildCard(pVec);                      // ワイルドカードやディレクトリ再帰してパスを取得.
  #endif
  #if (defined EXARGV_TOSLASH) || (defined EXARGV_TOBACKSLASH)
    ExArgv_convBackSlash(pVec);                     // define設定に従って、\ と / の変換. (基本的には何もしない)
  #endif

 #if defined(EXARGV_USE_WCHAR_TO_UTF8)
	*pppUtf8s = (char**)ExArgv_VectorToArgv( &pVec, pArgc, pppArgv );  // 作業リストを argc,argv に変換. 使用済み開放.
 #else
    ExArgv_VectorToArgv( &pVec, pArgc, pppArgv );   // 作業リストを argc,argv に変換し、作業リスト自体は開放.
 #endif

    return 0;
}
#endif



// ===========================================================================

#if (defined EXARGV_FOR_WINMAIN) == 0 || (defined EXARGV_USE_SETARGV) == 0

/** argc,argv をレスポンスファイルやワイルドカード展開して、argc, argvを更新して返す.
 *  @param  pArgc       argcのアドレス.(argvの数)
 *  @param  pppArgv     argvのアドレス.
 */
#if defined(EXARGV_USE_WCHAR_TO_UTF8)
char** ExArgv_conv_to_utf8(int* pArgc, char_t*** pppArgv)
#else
void** ExArgv_conv(int* pArgc, char_t*** pppArgv)
#endif
{
    int             argc;
    char_t**        ppArgv;
    ExArgv_Vector*  pVec;
    int             i;

    assert( pArgc != 0 && pppArgv != 0 );
    if (pArgc == 0 || pppArgv == 0)
        return NULL;

    ppArgv = *pppArgv;
    argc   = *pArgc;
    assert(argc > 0 && ppArgv != 0);
    if (argc == 0 || ppArgv == 0)
        return NULL;

  #if defined EXARGV_USE_FULLPATH_ARGV0 && defined _WIN32       // 古いソース用に、exeのフルパスを設定.
   #if defined _MSC_VER     // vcならすでにあるのでそれを流用.
    ppArgv[0] = _pgmptr;
   #elif defined __GNUC__   // わからないのでモジュール名取得.
    {
        static char_t nm[MAX_PATH];
        if (GetModuleFileName(NULL, nm, MAX_PATH) > 0)
            ppArgv[0] = nm;
    }
   #endif
  #endif

  #if !defined(EXARGV_USE_WCHAR_TO_UTF8)
    if (argc < 2)
        return NULL;
  #endif

  #if !EXARGV_USE_CONFIG && !defined(EXARGV_ENVNAME) && !defined(EXARGV_TOSLASH) && !defined(EXARGV_TOBACKSLASH) && !defined(EXARGV_USE_WCHAR_TO_UTF8)
   #if !EXARGV_USE_WC && !EXARGV_USE_RESFILE
    return NULL; //(void**)*ppArgv;     // ほぼ変換無し...
   #elif defined EXARGV_USE_CHK_CHR
    if (ExArgv_checkWcResfile(argc, ppArgv) == 0)   // 現状のargc,argvを弄る必要があるか?
        return NULL; //(void**)ppArgv;
   #endif
  #endif

    pVec = ExArgv_Vector_create(argc+1);            // argvが増減するので、作業用のリストを用意.

    //x printf("@4 %d %p(%p)\n", argc, ppArgv, *ppArgv);
    //x printf("   %p: %p %d %d\n", pVec, pVec->buf, pVec->capa, pVec->size);

    // 実行ファイル名の取得.
    if (argc > 0)
        ExArgv_Vector_push( pVec, ppArgv[0] );      // Vecに登録.

    // 環境変数の取得.
  #ifdef EXARGV_ENVNAME
    assert(STR_LEN(EXARGV_ENVNAME) > 0);
    ExArgv_getEnv(EXARGV_ENVNAME, pVec);
  #endif

    // コンフィグファイルの読込.
  #if EXARGV_USE_CONFIG
    ExArgv_getCfgFile( ppArgv[0], pVec );
  #endif

    //x printf("%p %x %#x %p\n",pVec, pVec->capa, pVec->size, pVec->buf);

  #if EXARGV_USE_WC
    s_wildMode  = 0;
  #endif

    // 引数の処理.
    for (i = 1; i < argc; ++i) {
        const char_t* p = ppArgv[i];
      #if EXARGV_USE_RESFILE
        if (i > 0 && *p == T('@')) {
            ExArgv_getResFile(p+1, pVec, 0);        // レスポンスファイル読込.
        } else
      #endif
        {
          #if EXARGV_USE_WC
            s_wildMode |= ExArgv_fname_isWildCard(p);
          #endif
            ExArgv_Vector_push( pVec, p );          // Vecに登録.
        }
    }

  #if EXARGV_USE_WC
    if (s_wildMode)
        ExArgv_wildCard(pVec);                      // ワイルドカードやディレクトリ再帰してパスを取得.
  #endif
  #if (defined EXARGV_TOSLASH) || (defined EXARGV_TOBACKSLASH)
    ExArgv_convBackSlash(pVec);                     // define設定に従って、\ と / の変換. (基本的には何もしない)
  #endif

 #if defined(EXARGV_USE_WCHAR_TO_UTF8)
    return (char**)ExArgv_VectorToArgv( &pVec, pArgc, pppArgv ); // 作業リストを argc,argv に変換.
 #else
    return ExArgv_VectorToArgv( &pVec, pArgc, pppArgv );   // 作業リストを argc,argv に変換し、作業リスト自体は開放.
 #endif
}

#endif



// ===========================================================================


static inline int ExArgv_isOpt(int c)
{
  #ifdef EXARGV_USE_SLASH_OPT
    return c == T('-') || c == T('/');
  #else
    return c == T('-');
  #endif
}


static inline int ExArgv_isDirSep(int c)
{
  #if defined DOSWIN32
    return c == T('/') || c == T('\\');
  #else
    return c == T('/');
  #endif
}



static inline void str_l_cpy(char_t* d, const char_t* s, size_t l)
{
    const char_t*   e = d + l - 1;
    while (d < e && *s) {
        *d++ = *s++;
    }
    *d = T('\0');
}



static inline void str_l_cat(char_t* d, const char_t* s, size_t l)
{
    const char_t*   e = d + l - 1;
    while (d < e && *d) {
        ++d;
    }
    while (d < e && *s) {
        *d++ = *s++;
    }
    *d = T('\0');
}



#if EXARGV_USE_WC

/// ワイルドカード文字が混じっているか?
static int  ExArgv_fname_isWildCard(const char_t* s) {
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



/// リカーシブ指定の**があれば、*一つにする.
static inline void  ExArgv_fname_removeRecChr(char_t* d, const char_t* s)
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



#if defined EXARGV_USE_CHK_CHR
/// 引数文字列配列に、レスポンスファイル指定、ワイルドカード指定、リカーシブ指定があるかチェック.
static unsigned ExArgv_checkWcResfile(int argc, char_t** argv)
{
    int         i;
    unsigned    rc    = 0;

    for (i = 1; i < argc; ++i) {
        const char_t* p = argv[i];
      #if EXARGV_USE_RESFILE
        if (*p == T('@')) {
            rc |= 4;    // レスポンスファイル指定.
        } else
      #endif
        {
          #if EXARGV_USE_WC
            if (ExArgv_isOpt(*p) == 0) {
                int mode = ExArgv_fname_isWildCard(p);
                s_wildMode |= mode;
                if (mode > 0) {
                    rc |= 1;    // ワイルドカード指定.
                    if (mode == 2)
                        rc |= 2;
                }
            }
          #endif
        }
    }
    return rc;
}
#endif



#ifdef EXARGV_ENVNAME
/// 環境変数があれば、登録.
static void ExArgv_getEnv(const char_t* envName, ExArgv_Vector* pVec)
{
    const char_t* env;
    if (envName == 0 || envName[0] == 0)
        return;
    env = GET_ENV(envName);
    if (env && env[0]) {
        char_t          arg[ FILEPATH_SZ + 4 ];
        while ( (env = ExArgv_fname_scanArgStr(env, arg, FILEPATH_SZ)) != NULL ) {
            const char_t* p = arg;
          #if EXARGV_USE_WC
            s_wildMode |= ExArgv_fname_isWildCard(p);
          #endif
            ExArgv_Vector_push( pVec, p );
        }
    }
}
#endif



#if EXARGV_USE_CONFIG
/// コンフィグファイルの読み込み.
static void ExArgv_getCfgFile(const char_t* exeName, ExArgv_Vector* pVec)
{
    char_t  name[ FILEPATH_SZ+4 ];
    char_t* p = name;

    // 実行ファイル名からコンフィグパス名を生成.
  #ifdef DOSWIN32
    str_l_cpy(p, exeName, FILEPATH_SZ-10);
  #else
    *p++ = T('~'), *p++ = T('/'), *p++ = T('.');
    str_l_cpy(p, ExArgv_fname_baseName(exeName), FILEPATH_SZ - 3 - 10);
  #endif

    p = STR_R_CHR(p, T('.'));
    if (p)
        str_l_cpy(p, T(EXARGV_CONFIG_EXT), 10);
    else
        str_l_cpy(name+STR_LEN(name), T(EXARGV_CONFIG_EXT), 10);
    ExArgv_getResFile(name, pVec, 1);
}
#endif



#if EXARGV_USE_RESFILE || EXARGV_USE_CONFIG
/** レスポンスファイルを(argc,argv)を pVec に変換.
 * レスポンスファイルやワイルドカード、リカーシブ等の処理も行う.
 */
static void ExArgv_getResFile(const char_t* fname, ExArgv_Vector* pVec, BOOL notFoundOk)
{
    unsigned    n = 0;
    enum {      BUF_SZ = 16 * 1024 };
    char_t      buf[BUF_SZ];
    FILEPTR     fp;
    fp = FOPEN_RT(fname);
    if (FILEPTR_IS_OK(fp) == 0) {
        if (notFoundOk)
            return;
        FPRINTF(STDERR, T("Response-file '%s' is not opened.\n"), fname);
        exit(1);    // return;
    }
    while (FGETS(buf, BUF_SZ, fp)) {
        char_t  arg[FILEPATH_SZ + 4];
        char_t* s = buf;
        while ( (s = ExArgv_fname_scanArgStr(s, arg, FILEPATH_SZ)) != NULL ) {
            int c = ((uchar_t*)arg)[0];
            if (c == T(';') || c == T('#') || c == T('\0')) {   // 空行やコメントの時.
                break;
            }
            // 再帰検索指定,ワイルドカードの有無をチェック.
          #if EXARGV_USE_WC
            s_wildMode |= ExArgv_fname_isWildCard(arg);
          #endif
            ExArgv_Vector_push(pVec, arg );
        }
    }
    if (FERROR(fp)) {
        FPRINTF(STDERR, T("%s (%d) : file read error.\n"), fname, n);
        exit(1);
    }
    FCLOSE(fp);
}
#endif



#if EXARGV_USE_WC
/** ワイルドカード、再帰処理.
 */
static void ExArgv_wildCard(ExArgv_Vector* pVec)
{
    char_t**        pp;
    char_t**        ee;
    ExArgv_Vector*  wk;
    int             mode;

    // 再構築.
    wk = ExArgv_Vector_create( pVec->size+1 );
    ee = pVec->buf + pVec->size;
    for (pp = pVec->buf; pp != ee; ++pp) {
        const char_t* s = *pp;
      #if EXARGV_USE_WC
        if (   ExArgv_isOpt(*s) == 0                    // オプション以外の文字列で,
            && (pp != pVec->buf)                        // 初っ端以外([0]は実行ファイル名なので検索させない)のときで,
            && ((mode = ExArgv_fname_isWildCard( s )) != 0) // ワイルドカード指定のありのとき.
         ){
            char_t  name[FILEPATH_SZ+4];
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
            ExArgv_Vector_findFname(wk, s, recFlag);

        } else  {
            ExArgv_Vector_push( wk, s );
        }
      #else
        ExArgv_Vector_push( wk, s );
      #endif
    }

    // 元のリストを開放.
    for (pp = pVec->buf; pp != ee; ++pp) {
        char_t* p = *pp;
        if (p)
            ExArgv_free(p);
    }
    ExArgv_free(pVec->buf);

    // 今回生成したものを、pVecに設定.
    pVec->buf  = wk->buf;
    pVec->size = wk->size;
    pVec->capa = wk->capa;

    // 作業に使ったメモリを開放.
    ExArgv_free(wk);
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

    for (pp = pVec->buf; pp != ee; ++pp) {
        char_t* s = *pp;
        if (ExArgv_isOpt(*s) == 0) {        // オプション以外の文字列で、
          #if (defined EXARGV_TOSLASH)
            ExArgv_fname_backslashToSlash(s);       // \ を / に置換.
          #else
            ExArgv_fname_slashToBackslash(s);       // / を \ に置換.
          #endif
        } else {                            // オプションなら、下手に変換しないでおく.
            ;
        }
    }
}
#endif



#if defined(EXARGV_USE_WCHAR_TO_UTF8)
//#define U8F_WCS_FROM_MBS(d,dl,s,sl) MultiByteToWideChar(65001,0,(s),(int)(sl),(d),(int)(dl))
#define U8F_MBS_FROM_WCS(d,dl,s,sl) WideCharToMultiByte(65001,0,(s),(int)(sl),(d),(int)(dl),0,0)

static char*  ExArgv_strdupFromWcs(wchar_t const* wcs) {
	size_t  len, wlen;
	char* u8s;
	if (!wcs)
		wcs = L"";
	wlen = wcslen(wcs);
	len  = U8F_MBS_FROM_WCS(NULL,0, wcs, wlen);
	u8s  = (char*)ExArgv_alloc(len + 1);
	if (u8s)
		U8F_MBS_FROM_WCS(u8s, len + 1, wcs, wlen + 1);
	return u8s;
}

/** pVecから、(argc,argv)を生成. ppVecは開放する.
 */
static void** ExArgv_VectorToArgv(ExArgv_Vector** ppVec, int* pArgc, char_t*** pppArgv)
{
    ExArgv_Vector*  pVec;
    char**          av;
    int             ac;
	int				i;

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
    av       = (char**) ExArgv_alloc(sizeof(char*) * (ac + 2));
    // *pppArgv = av;

	for (i = 0; i < ac; ++i) {
		char_t* s  = pVec->buf[i];
		av[i] = ExArgv_strdupFromWcs(s);
		ExArgv_free(s);
		pVec->buf[i] = NULL;
	}

    av[ac]   = NULL;
    av[ac+1] = NULL;

    // 作業に使ったメモリを開放.
    ExArgv_free(pVec->buf);
    ExArgv_free(pVec);
    *ppVec   = NULL;

    return (void**)av;
}
#else
/** pVecから、(argc,argv)を生成. ppVecは開放する.
 */
static void** ExArgv_VectorToArgv(ExArgv_Vector** ppVec, int* pArgc, char_t*** pppArgv)
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
    av       = (char_t**) ExArgv_alloc(sizeof(char_t*) * (ac + 2));
    *pppArgv = av;

    memcpy(av, pVec->buf, sizeof(char_t*) * ac);

    av[ac]   = NULL;
    av[ac+1] = NULL;

    // 作業に使ったメモリを開放.
    ExArgv_free(pVec->buf);
    ExArgv_free(pVec);
    *ppVec   = NULL;

    return (void**)av;
}
#endif

void ExArgv_Free(void*** pppArgv)
{
	void** pp = *pppArgv;
	while (*pp) {
		void* p = *pp;
		if (p)
			free(p);
		++pp;
	}
	free(pp);
	*pppArgv = 0;
}

void ExArgv_FreeA(char*** pppArgv)
{
	ExArgv_free((void***)pppArgv);
}

void ExArgv_FreeW(wchar_t*** pppArgv)
{
	ExArgv_free((void***)pppArgv);
}

// ===========================================================================

#if defined DOSWIN32 == 0 && defined EXARGV_USE_WCHAR == 0   // 環境変数 LANG=ja_JP.SJIS のような状態を前提.

static unsigned char        s_shift_char_type = 0;

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
 #define ExArgv_FNAME_ISMBBLEAD(c)      IsDBCSLeadByte(c)
#elif defined HAVE_MBCTYPE_H
 #define ExArgv_FNAME_ISMBBLEAD(c)      _ismbblead(c)
#else
 #define ExArgv_FNAME_ISMBBLEAD(c)      ((c) >= 0x80 && ExArgv_fname_is_mbblead(c))
#endif


/// 次の文字へポインタを進める. ※CharNext()がサロゲートペアやutf8対応してくれてたらいいなと期待(駄目かもだけど)
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



#if EXARGV_USE_RESFILE || EXARGV_USE_CONFIG || defined EXARGV_ENVNAME \
    || defined EXARGV_FOR_WINMAIN || defined EXARGV_USE_SETARGV
/** コマンドラインで指定されたファイル名として、""を考慮して,
 *  空白で区切られた文字列(ファイル名)を取得.
 *  @return スキャン更新後のアドレスを返す。strがEOSだったらNULLを返す.
 */
static char_t *ExArgv_fname_scanArgStr(const char_t *str, char_t arg[], int argSz)
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



#if EXARGV_USE_WC || (EXARGV_USE_CONFIG && defined DOSWIN32 == 0)
/** ファイルパス名中のディレクトリを除いたファイル名の位置を返す.
 */
static char_t*  ExArgv_fname_baseName(const char_t* adr)
{
    const char_t *p = adr;
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
static int  ExArgv_Vector_findFname(ExArgv_Vector* pVec, const char_t* srchName, int recFlag)
{
  #if defined _WIN32        // ※dos未対応(同様に作成可能だけど)
    unsigned            num         = 0;
    WIN32_FIND_DATA*    pFindData   = (WIN32_FIND_DATA*)ExArgv_alloc(sizeof(WIN32_FIND_DATA));
    HANDLE              hdl         = FindFirstFile(srchName, pFindData);
    char_t*             pathBuf;
    char_t*             baseName;
    size_t              baseNameSz;

    pathBuf  = (char_t*)ExArgv_alloc(FILEPATH_SZ);
    str_l_cpy(pathBuf, srchName, FILEPATH_SZ);

    baseName    = ExArgv_fname_baseName(pathBuf);
    *baseName   = T('\0');
    baseNameSz  = FILEPATH_SZ - STR_LEN(pathBuf);
    assert(baseNameSz >= MAX_PATH);

    if (hdl != INVALID_HANDLE_VALUE) {
        // ファイル名を取得. ※ 隠しファイルは対象外にしておく.
        do {
            str_l_cpy(baseName, pFindData->cFileName, baseNameSz);
            if ((pFindData->dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_HIDDEN)) == 0) {
                ExArgv_Vector_push( pVec, pathBuf );
                ++num;
            }
        } while (FindNextFile(hdl, pFindData) != 0);
        FindClose(hdl);
    }

   #if EXARGV_USE_WC_REC
    // ディレクトリ再帰でファイル名を取得.
    if (recFlag && baseNameSz >= 16) {
        const char_t* srch = ExArgv_fname_baseName(srchName);
        str_l_cpy(baseName, T("*.*"), 4);
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
                        str_l_cat(baseName, DIRSEP_STR, baseNameSz);
                        str_l_cat(baseName, srch      , baseNameSz);
                        num += ExArgv_Vector_findFname(pVec, pathBuf, 1);
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
    char_t*         pathBuf  = (char_t*)ExArgv_alloc(FILEPATH_SZ);
    int             dirNum;
    char_t*         srchBase = ExArgv_fname_baseName(srchName);
    char_t*         baseName;
    size_t          baseNameSz;
    int             flag = 0;

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
                        ExArgv_Vector_push( pVec, pathBuf );
                        ++num;
                    }
                }
            }
        }

       #if EXARGV_USE_WC_REC
        // ディレクトリがあれば再帰.
        if (recFlag && baseNameSz >= 16) {
            const char_t* srch = ExArgv_fname_baseName(srchName);
            for (i = 0; i < dirNum; ++i) {
                struct dirent* d = namelist[i];
                str_l_cpy(baseName, d->d_name, baseNameSz);
                if (stat(pathBuf, &statBuf) >= 0 && STR_CMP(baseName,T(".")) != 0 && STR_CMP(baseName,T("..")) !=0 ) {
                    if ((statBuf.st_mode & S_IFMT) == S_IFDIR) {
                        str_l_cat(baseName, T("/"), baseNameSz);
                        str_l_cat(baseName, srch  , baseNameSz);
                        num += ExArgv_Vector_findFname(pVec, pathBuf, 1);
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
    ExArgv_Vector* pVec = (ExArgv_Vector*)ExArgv_alloc( sizeof(ExArgv_Vector) );
    size                = ((size + EXARGV_VECTOR_CAPA_BASE) / EXARGV_VECTOR_CAPA_BASE) * EXARGV_VECTOR_CAPA_BASE;
    pVec->capa          = size;
    pVec->size          = 0;
    pVec->buf           = (char_t**)ExArgv_alloc(sizeof(void*) * size);
    return pVec;
}



/** 引数文字列リストに、文字列を追加.
 */
static void ExArgv_Vector_push(ExArgv_Vector* pVec, const char_t* pStr)
{
    assert(pVec != 0);
    assert(pStr != 0);
    if (pStr && pVec) {
        unsigned    capa = pVec->capa;
        assert(pVec->buf != 0);
        if (pVec->size >= capa) {   // キャパを超えていたら、メモリを確保しなおす.
            char_t**        buf;
            unsigned        newCapa = capa + EXARGV_VECTOR_CAPA_BASE;
          #if 1 // 普通はコレ以前にメモリ不足になるだろうが一応.
            if (newCapa < capa) {   // 溢れたらエラー...
                if (capa < 0xFFFFFFFF) {
                    newCapa = 0xFFFFFFFF;
                } else {
                    FPRINTF(STDERR, T("too many arguments.\n"));
                    exit(1);
                }
            }
          #endif
            //x printf("!  %p: %p %d %d ::%s\n", pVec, pVec->buf, pVec->capa, pVec->size, pStr);
            assert(pVec->size == capa);
            pVec->capa  = newCapa;
            buf         = (char_t**)ExArgv_alloc(sizeof(void*) * pVec->capa);
            if (pVec->buf)
                memcpy(buf, pVec->buf, capa*sizeof(void*));
            memset(buf+capa, 0, EXARGV_VECTOR_CAPA_BASE*sizeof(void*));
            ExArgv_free(pVec->buf);
            pVec->buf   = buf;
        }
        assert(pVec->size < pVec->capa);
        pVec->buf[ pVec->size ] = ExArgv_strdup(pStr);
        ++ pVec->size;
        //x printf("!!  %p: %p %d %d ::%s\n", pVec, pVec->buf, pVec->capa, pVec->size, pStr);
    }
}



// ===========================================================================

/** malloc
 */
static void* ExArgv_alloc(unsigned size)
{
    void* p = malloc(size);
    if (p == NULL) {
        FPRINTF(STDERR, T("not enough memory.\n"));
        exit(1);
    }
    memset(p, 0, size);
    return p;
}



/** strdup
 */
static char_t* ExArgv_strdup(const char_t* s)
{
    size_t   sz = STR_LEN(s) + 1;
    char_t*  p  = (char_t*)malloc(sz * sizeof(char_t));
    if (p == NULL) {
        FPRINTF(STDERR, T("not enough memory.\n"));
        exit(1);
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
