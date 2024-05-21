/**
 *  @file   ExArgv.h
 *  @brief  Extended processing for argc, argv (wildcards, response files).
 *  @author Masashi KITAMURA
 *  @date   2006-2024
 *  @license Boost Software License Version 1.0
 *  @note
 *  - Primarily intended for use with Win/Dos (command line tools).
 *    Usable with mac, linux gcc/clang as well.
 *
 *  - Converts argc, argv in main(int argc, char* argv[]) to expand
 *    wildcards and response file specifications.
 *    Call as early as possible in main():
 *        ExArgv_conv(&argc, &argv);
 *    If you want to perform WC conversion later:
 *        ExArgv_convEx(&argc, &argv, 0); // First, perform non-WC expansion like ResFile.
 *        (perform some operations)
 *        ExArgv_convEx(&argc, &argv, 2|1); // Perform WC expansion and free the old argv.
 *    Each string in the resulting argv will be malloc'ed.
 *    Free argv explicitly with ExArgv_release(&argv).
 *
 *  - When using with WinMain(), define EXARGV_FOR_WINMAIN and call:
 *        ExArgv_cmdLineToArgv(cmdl, &argc, &argv);
 *        ExArgv_conv(&argc, &argv);
 *    Alternatively, you can use:
 *        ExArgv_forWinMain(cmdl, &argc, &argv);
 *
 *  - ExArgv.h is a header but also serves as a configuration file for ExArgv.c.
 *    Intended for copying ExArgv.h and ExArgv.c for each application and
 *    customizing ExArgv.h.
 *    Alternatively, prepare ExArgv_conf.h and define EXARGV_UE_CONF_H to set
 *    configurations there.
 *
 *  - Configurable elements include:
 *    - Wildcards (on/off)
 *    - Recursive wildcard specification (**) (on/off)
 *    - @response files (on/off)
 *    - Reading .cfg files associated with .exe (on/off)
 *    - Use of optional environment variable names.
 *    etc.
 *
 *  - If the argument string starts with '-', it's likely an option,
 *    so wildcard characters within it are not expanded.
 *  - Assumes win for defined _WIN32, otherwise unix-like.
 *  - Define the UNICODE or EXARGV_ENCODE_WCHAR macro for wchar_t,
 *    otherwise use char.
 *  - With the prevalence of UTF8, handle the second byte '\' of MBC only
 *    if defined(EXARGV_ENCODE_MBC).
 *  - Define EXARGV_ENCODE_UTF8 to prepare conversion functions
 *    for handling UTF8 names in environments prior to Win10 1903.
 *    e.g.) int wmain(int argc, wchar_t* wargv[]) {
 *              char** argv = ExArgv_convExToUtf8(&argc, wargv, 1);
 */

#ifndef EXARGV_H_INCLUDED__
#define EXARGV_H_INCLUDED__

// ---------------------------------------------------------------------------
// Configuration settings.

// [] Define to generate ExArgv_forWinMain for use with WinMain.
//#define EXARGV_FOR_WINMAIN

// [] Define one of the following for path string character encoding.
//    If omitted, WCHAR is used if UNICODE is defined, otherwise UTF8.
//    Assumes UTF8 for non-Windows. MBC is for considering SJIS, etc.
//#define EXARGV_ENCODE_ANSI    // Use A-type API on Windows. Assumes UTF8 for non-Windows.
//#define EXARGV_ENCODE_MBC     // Use A-type API on Windows. Considers 2nd byte of DBC.
//#define EXARGV_ENCODE_UTF8    // Use W-type API on Windows for UTF8 encoding. Same as ANSI for non-Windows.
//#define EXARGV_ENCODE_WCHAR   // Use W-type API on Windows. Not for non-Windows.

// [] Enable wildcard specification: 1=enabled, 0=disabled, undefined=1
//#define EXARGV_USE_WC             1

// [] For wildcard enabled, recursive search if wildcard ** is present:
//      1=enabled, 0=disabled, undefined=1
//#define EXARGV_USE_WC_REC         1

// [] Enable @response files:
//      1=enabled, 0=disabled, undefined=0
//#define EXARGV_USE_RESFILE        0

// [] Enable simple config (response) file input:
//      1=enabled, 0=disabled, undefined=0
//    When enabled, replaces .exe with .ini for win/dos.
//    For non-Windows, assumes unix-style ~/.executable_name.ini
//#define EXARGV_USE_CONFIG         0

// [] When config file input is enabled, define this to set the config file extension.
//#define EXARGV_CONFIG_EXT         ".ini"

// [] Define to use this environment variable as the command line string.
//#define EXARGV_ENVNAME            "your_app_env_name"

// [] Windows only. Full path for argv[0] executable name:
//      1=enabled, 0=disabled, undefined=0
//    Note: bcc, dmc, watcom already use full path, so this is for vc, gcc.
//#define EXARGV_USE_FULLPATH_ARGV0  0

// [] Define to replace \ with / in filepath.
//#define EXARGV_TOSLASH

// [] Define to replace / with \ in filepath.
//#define EXARGV_TOBACKSLASH

// [] Treat / as option start character (1) or not (0)
//#define EXARGV_USE_SLASH_OPT      0


// ---------------------------------------------------------------------------

#if defined(__has_include) && !defined(EXARGV_USE_CONF_H)
 #if __has_include("ExArgv_conf.h")
  #define EXARGV_USE_CONF_H
 #endif
#endif
#if defined(EXARGV_USE_CONF_H)
 #include "ExArgv_conf.h"
#endif

#if (1 <  defined(EXARGV_ENCODE_UTF8)+defined(EXARGV_ENCODE_ANSI)+defined(EXARGV_ENCODE_MBC)+defined(EXARGV_ENCODE_WCHAR))
#error Define one of EXARGV_ENCODE_UTF8, EXARGV_ENCODE_ANSI, EXARGV_ENCODE_MBC, or EXARGV_ENCODE_WCHAR
#endif

#if (0 == defined(EXARGV_ENCODE_UTF8)+defined(EXARGV_ENCODE_ANSI)+defined(EXARGV_ENCODE_MBC)+defined(EXARGV_ENCODE_WCHAR))
 #if defined(UNICODE) || defined(_UNICODE)
  #define EXARGV_ENCODE_WCHAR
 #elif 1 //defined(_WIN32)
  #define EXARGV_ENCODE_UTF8
 #else
  #define EXARGV_ENCODE_ANSI
 #endif
#endif

#include <stddef.h>

#if defined(EXARGV_ENCODE_WCHAR) && !defined(__cplusplus)
#include <wchar.h>  // wchar_t
#endif


// ---------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

#if defined(EXARGV_ENCODE_WCHAR)
 #define EXARGV_CHAR_T  wchar_t
#else
 #define EXARGV_CHAR_T  char
#endif

#define ExArgv_RC       int

ExArgv_RC ExArgv_conv(int* pArgc, EXARGV_CHAR_T*** pppArgv);
ExArgv_RC ExArgv_convEx(int* pArgc, EXARGV_CHAR_T*** pppArgv, unsigned wcFlags);
void      ExArgv_release(EXARGV_CHAR_T*** pppArgv);

#if defined(EXARGV_FOR_WINMAIN) // for win-gui.
 #if defined(EXARGV_ENCODE_UTF8)
  ExArgv_RC ExArgv_cmdLineToArgv(wchar_t const* pCmdLine, int* pArgc, char*** pppArgv);
  ExArgv_RC ExArgv_forWinMain(wchar_t const* pCmdLine, int* pArgc, char*** pppArgv);
 #else
  ExArgv_RC ExArgv_cmdLineToArgv(EXARGV_CHAR_T const* pCmdLine, int* pArgc, EXARGV_CHAR_T*** pppArgv);
  ExArgv_RC ExArgv_forWinMain(EXARGV_CHAR_T const* pCmdLine, int* pArgc, EXARGV_CHAR_T*** pppArgv);
 #endif
#endif

#if defined(EXARGV_ENCODE_UTF8) && defined(_WIN32)
 char**    ExArgv_convExToUtf8(int* pArgc, wchar_t** pppArgv, unsigned wcFlags);
 char**    ExArgv_wargvToUtf8(int argc, wchar_t* ppWargv[]);
 wchar_t** ExArgv_u8argvToWcs(int argc, char* ppArgv[]);
#endif


// ---------------------------------------------------------------------------

EXARGV_CHAR_T*  ExArgv_fnameBase(EXARGV_CHAR_T const* adr);
EXARGV_CHAR_T*  ExArgv_strdupAddCapa(EXARGV_CHAR_T const* s, size_t add_capa);

#if defined(_WIN32) && defined(EXARGV_ENCODE_UTF8)
  char*     ExArgv_u8strdupFromWcs(wchar_t const* wcs);
  wchar_t*  ExArgv_wcsdupFromUtf8(char const* u8s);
#endif

#if EXARG_USE_UTIL || EXARGV_USE_RESFILE || EXARGV_USE_CONFIG
 void*      ExArgv_fileLoadMalloc(EXARGV_CHAR_T const* fpath, size_t* pSize);
 size_t     ExArgv_fileSize(EXARGV_CHAR_T const* fpath);
#endif
#if EXARG_USE_UTIL || defined(EXARGV_ENVNAME) || defined(EXARGV_FOR_WINMAIN)
 EXARGV_CHAR_T* ExArgv_scanArgStr(EXARGV_CHAR_T const* str, EXARGV_CHAR_T arg[], size_t argSz);
#endif
#if EXARG_USE_UTIL || defined(EXARGV_ENVNAME)
 EXARGV_CHAR_T* ExArgv_getenvDup(EXARGV_CHAR_T const* envName);
#endif

#ifdef __cplusplus
}
#endif


// ---------------------------------------------------------------------------
#if EXARG_USE_UTIL
 int        ExArgv_fileExist(EXARGV_CHAR_T const* fpath);   // 0:none 1:file 2:dir
#endif

// ---------------------------------------------------------------------------
#endif  // EXARGV_H_INCLUDED__
