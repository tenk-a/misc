/**
 *  @file   ExArgv.cpp
 *  @brief  Extended processing for argc, argv (wildcards, response files).
 *  @author Masashi KITAMURA
 *  @date   2006-2024
 *  @license Boost Software License Version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(assert)
#include <assert.h>
#endif

#include "ExArgv.h"

#if !defined(_WIN32) && defined(EXARGV_ENCODE_WCHAR)
#error EXARGV_ENCODE_WCHAR is only available when _WIN32 is defined.
#endif

#if defined(EXARGV_ENCODE_WCHAR)
#if !defined(UNICODE)
#define UNICODE
#endif
#include <wchar.h>
#else
#undef UNICODE
#endif

#if defined(_WIN32) == 0 && defined(EXARGV_ENCODE_UTF8)
  #undef  EXARGV_ENCODE_UTF8
  #define EXARGV_ENCODE_ANSI
#endif


#if defined(_MSC_VER)
//#pragma warning(disable:4267)
#pragma warning(disable:4996)
#endif

// ===========================================================================
// Default configuration

#if !defined(EXARGV_USE_UTIL)
#define EXARGV_USE_UTIL     0
#endif

#if !defined(EXARGV_USE_WC)
#define EXARGV_USE_WC       1
#endif

#if !defined(EXARGV_USE_WC_REC)
#define EXARGV_USE_WC_REC   EXARGV_USE_WC
#endif

#if !defined(EXARGV_USE_RESFILE)
#define EXARGV_USE_RESFILE  0
#endif

#if !defined(EXARGV_USE_CONFIG)
#define EXARGV_USE_CONFIG   0
#endif

#if EXARGV_USE_CONFIG
#if !defined(EXARGV_CONFIG_EXT)
#define EXARGV_CONFIG_EXT ".ini"
#endif
#endif

#if !defined(EXARGV_USE_FULLPATH_ARGV0)
#define EXARGV_USE_FULLPATH_ARGV0   0
#endif

#if !defined(EXARGV_USE_SLASH_OPT)
#define EXARGV_USE_SLASH_OPT        0
#endif

//#define EXARGV_TOSLASH
//#define EXARGV_TOBACKSLASH
//#define EXARGV_ENVNAME    "your_app_env_name"


// ===========================================================================

#if !defined(DOSWIN32) && (defined(_WIN32) /*|| defined(_DOS)*/)
 #define DOSWIN32   1
#endif


#if defined(_WIN32)
 #include <windows.h>
 #if defined(_MSC_VER) && defined(EXARGV_ENCODE_MBC) // for CharNext()
  #pragma comment(lib, "User32.lib")
 #endif
//#elif defined(_DOS)
// #include <mbctype.h>
// #include <dirent.h>
// #include <sys/stat.h>
#else   // linux
 #include <dirent.h>
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <fnmatch.h>
#endif

#if !defined(BOOL)
#define BOOL            int //bool
#endif

#if defined(__cplusplus)
 //#define BOOL         bool
 #define EXTERN_C       extern "C"
#else
 //#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
 //#define BOOL          _Bool
 //#else
 //#define BOOL          char
 //#endif
 #define EXTERN_C       extern
 #if defined(DOSWIN32)
  #define inline        __inline
 #endif
#endif


#if !defined(MAX_PATH)
 #if defined(_MAX_PATH)
  #define MAX_PATH      _MAX_PATH
 #else
  #if defined(_DOS)
   #define MAX_PATH     260
  #else
   #define MAX_PATH     1024
  #endif
 #endif
#endif

#if defined(_WIN32)
 #if defined(EXARGV_ENCODE_UTF8)
  #define FILENAME_LEN   (255*3)
 #else
  #define FILENAME_LEN   255
 #endif
#else
 #define FILENAME_LEN   1024
#endif

#if defined(EXARGV_ENCODE_WCHAR)
#define MAC_TO_STR(x)   MAC_TO_STR_2(x)
#define MAC_TO_STR_2(x) L##x
#else
#define MAC_TO_STR(x)   x
#endif


// ===========================================================================

#if defined(EXARGV_ENCODE_WCHAR)
#undef  _pgmptr
#define _pgmptr         _wpgmptr
#define T(x)            L##x
typedef wchar_t         char_t;
typedef wchar_t         uchar_t;
#define STR_LEN(a)      wcslen(a)
#define STR_CMP(a,b)    wcscmp(a,b)
#define STR_R_CHR(a,b)  wcsrchr(a,b)
#else
#define T(x)            x
typedef char            char_t;
typedef unsigned char   uchar_t;
#define STR_LEN(a)      strlen(a)
#define STR_CMP(a,b)    strcmp(a,b)
#define STR_R_CHR(a,b)  strrchr(a,b)
#endif

#if defined(_WIN32)
static void ExArgv_err_puts(char const* s);
#define ERR_PUTS(s)     ExArgv_err_puts(s)
#else
#define ERR_PUTS(s)     fprintf(stderr, "%s", s)
#endif


// ===========================================================================

#if defined(EXARGV_ENCODE_UTF8)
 enum { FILEPATH_ENC_RATIO  = 3 };
#elif defined(EXARGV_ENCODE_MBC)
 enum { FILEPATH_ENC_RATIO  = 2 };
#else
 enum { FILEPATH_ENC_RATIO  = 1 };
#endif
//enum { FILEPATH_SZ            = (MAX_PATH*2 > 0x8010) ? MAX_PATH*2 : 0x8010 };
enum { FILEPATH_SZ              = 0x8010 * FILEPATH_ENC_RATIO };
enum { EXARGV_VECTOR_CAPA_BASE  = 4096 };


#if defined(EXARGV_TOBACKSLASH)
#define DIRSEP_STR          T("\\")
#else
#define DIRSEP_STR          T("/")
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
static void*        ExArgv_allocE(size_t size);
static char_t*      ExArgv_strdupE(char_t const* s);
static void         ExArgv_free(void* s);

#define EXARGV_ALLOC(T,size)    ((T*)ExArgv_allocE((size) * sizeof(T)))

#if EXARGV_USE_WC
static int          ExArgv_Vector_findFname(ExArgv_Vector* pVec, char_t const* pPathName, int recFlag);
static BOOL         ExArgv_wildCard(ExArgv_Vector* pVec);
#endif
#if defined(EXARGV_FOR_WINMAIN)
static int          ExArgv_forCmdLine1(char_t const* pCmdLine, int* pArgc, char_t*** pppArgv);
#endif
#if defined(EXARGV_ENVNAME)
static BOOL         ExArgv_getAppEnv(char_t const* envName, ExArgv_Vector* pVec);
#endif
#if EXARGV_USE_CONFIG
static BOOL         ExArgv_loadConfigFile(char_t const* exeName, ExArgv_Vector* pVec);
#endif
#if EXARGV_USE_RESFILE || EXARGV_USE_CONFIG
static BOOL         ExArgv_loadResponseFile(char_t const* fname, ExArgv_Vector* pVec, BOOL notFoundOk);
#endif

#if defined(EXARGV_TOSLASH) || defined(EXARGV_TOBACKSLASH)
static void         ExArgv_convBackSlash(ExArgv_Vector* pVec);
#endif
#if defined(EXARGV_TOBACKSLASH)
static char_t*      ExArgv_fname_slashToBackslash(char_t filePath[]);
#endif
#if defined(EXARGV_TOSLASH)
static char_t*      ExArgv_fname_backslashToSlash(char_t filePath[]);
#endif

#if EXARGV_USE_WC
static unsigned     ExArgv_hasWildCard(char_t const* s);
#endif

/**
 */
static inline int ExArgv_isOpt(int c)
{
  #if EXARGV_USE_SLASH_OPT
    return c == T('-') || c == T('/');
  #else
    return c == T('-');
  #endif
}

//  -   -   -   -   -

#if defined(_WIN32)

#if defined(EXARGV_FOR_WINMAIN) || defined(EXARGV_USE_FULLPATH_ARGV0)
static size_t ExArgv_getModuleFileName(char_t fpath[], size_t capa);
#endif

#if /*defined(EXARGV_ENCODE_UTF8) ||*/ defined(EXARGV_ENCODE_WCHAR) && (EXARGV_USE_RESFILE || EXARGV_USE_CONFIG)
char*     ExArgv_u8strdupFromWcs(wchar_t const* wcs);
wchar_t*  ExArgv_wcsdupFromUtf8(char const* u8s);
#endif

#if EXARGV_USE_UTIL || EXARGV_USE_RESFILE || EXARGV_USE_CONFIG || EXARGV_USE_WC
#if defined(EXARGV_ENCODE_UTF8) == 0

typedef WIN32_FIND_DATA             ExArgv_finddata_t;
#define ExArgv_FindFirstFile(fn,dt) FindFirstFileEx((fn),FindExInfoStandard,(dt),FindExSearchNameMatch,NULL,2/*FIND_FIRST_EX_LARGE_FETCH*/)
#define ExArgv_FindNextFile(h,dt)   FindNextFile(h, (dt))
#define ExArgv_FindClose(h)         FindClose(h)

#else   // defined(defined(EXARGV_ENCODE_UTF8)
// A UTF-16 character can be 1-3 bytes in UTF-8, and 260 characters can require up to 260*3 bytes.
// However, the ANSI API for UTF-8 still uses only 260 bytes, leading to buffer shortage, so use an alternative.

typedef struct ExArgv_finddata_t {
    DWORD       dwFileAttributes;
    FILETIME    ftCreationTime;
    FILETIME    ftLastAccessTime;
    FILETIME    ftLastWriteTime;
    DWORD       nFileSizeHigh;
    DWORD       nFileSizeLow;
    DWORD       __dw0;
    DWORD       __dw1;
    WCHAR       cFileNameW[ MAX_PATH ];
    WCHAR       cAlternateFileNameW[ 14 ];

    CHAR        cFileName[MAX_PATH*3+4];
} ExArgv_finddata_t;

static HANDLE ExArgv_FindFirstFile(char_t const* fpath, ExArgv_finddata_t* data);
static BOOL   ExArgv_FindNextFile(HANDLE hdl, ExArgv_finddata_t* data);
#define ExArgv_FindClose(h)     FindClose(h)

#endif  // defined(defined(EXARGV_ENCODE_UTF8)
#endif

#endif  // _WIN32

// ===========================================================================

/** Expands the response file and wildcards, updates argc and argv, and returns them.
 *  argv and each string become malloced memory.
 *  @param  pArgc       argc address.(Number of argv)
 *  @param  pppArgv     argv address.
 *  @param  wcFlags     bit0: Perform wildcard expansion.
 *                      bit1: Memory is released as argv of ExArgv_conv is input.
 */
int ExArgv_convEx(int* pArgc, char_t*** pppArgv, unsigned wcFlags)
{
    int             argc;
    char_t**        ppArgv;
    ExArgv_Vector*  pVec;
    int             i;

    assert( pArgc != NULL && pppArgv != NULL );
    if (pArgc == NULL || pppArgv == NULL)
        return 0;

    ppArgv = *pppArgv;
    argc   = *pArgc;
    assert(argc > 0 && ppArgv != NULL);
    if (argc == 0 || ppArgv == NULL)
        return 0;

    // When setting the full path of the exe in argv[0] according to the old specifications.
  #if EXARGV_USE_FULLPATH_ARGV0 && defined(_WIN32)
   #if 0 //defined(_MSC_VER)
    ppArgv[0] = _pgmptr;
   #else
    {
        static char_t nm[MAX_PATH];
        if (ExArgv_getModuleFileName(nm, MAX_PATH) > 0)
            ppArgv[0] = nm;
    }
   #endif
  #endif

    if (argc < 1)
        return 0;

    pVec = ExArgv_Vector_create(argc+1);

    if (argc > 0) {
        if (ExArgv_Vector_push( pVec, ppArgv[0] ) == 0)
            return 0;
    }

  #if defined(EXARGV_ENVNAME)
    //assert(STR_LEN(EXARGV_ENVNAME) > 0);
    if (ExArgv_getAppEnv(MAC_TO_STR(EXARGV_ENVNAME), pVec) == 0)
        return 0;
  #endif

  #if EXARGV_USE_CONFIG
    if (ExArgv_loadConfigFile( ppArgv[0], pVec ) == 0)
        return 0;
  #endif

    for (i = 1; i < argc; ++i) {
        char_t const* p = ppArgv[i];
      #if EXARGV_USE_RESFILE
        if (i > 0 && *p == T('@')) {
            if (ExArgv_loadResponseFile(p+1, pVec, 0) == 0)
                return 0;
        } else
      #endif
        {
            if (ExArgv_Vector_push( pVec, p ) == 0)
                return 0;
        }
    }

  #if EXARGV_USE_WC
    if (wcFlags & 1) {
        if (ExArgv_wildCard(pVec) == 0)
            return 0;
    }
  #endif

  #if defined(EXARGV_TOSLASH) || defined(EXARGV_TOBACKSLASH)
    ExArgv_convBackSlash(pVec);
  #endif

    // Convert the work list to argc, argv and open the work list itself.
    {
        int rc = ExArgv_VectorToArgv( &pVec, pArgc, pppArgv ) != 0;
        if (wcFlags & 2)
            ExArgv_release(&ppArgv);
        return rc;
    }
}

#if defined(EXARGV_ENCODE_UTF8)
char** ExArgv_convExToUtf8(int* pArgc, wchar_t** ppWargv, unsigned wcFlags)
{
    char** ppArgv = ExArgv_wargvToUtf8(*pArgc, ppWargv);
    int    rc     = ExArgv_convEx(pArgc, &ppArgv, wcFlags);
    if (!rc)
        ExArgv_release(&ppArgv);
    return ppArgv;
}
#endif


int ExArgv_conv(int* pArgc, char_t*** pppArgv)
{
    return ExArgv_convEx(pArgc, pppArgv, 1);
}


// ===========================================================================

#if defined(EXARGV_FOR_WINMAIN)

#if defined(EXARGV_ENCODE_UTF8)
static int ExArgv_cmdLineToArgvSub(char const* pCmdLine, int* pArgc, char*** pppArgv)
#else
int ExArgv_cmdLineToArgv( char_t const* pCmdLine, int* pArgc, char_t*** pppArgv)
#endif
{
    ExArgv_Vector*  pVec;
    char_t*         arg;
    char_t const*   s;

    assert(pArgc != NULL && pppArgv != NULL);
    if (pArgc == NULL || pppArgv == NULL)
        return 0;

    arg = EXARGV_ALLOC(char_t, FILEPATH_SZ + 4);
    if (arg == NULL)
        return 0;

    if (ExArgv_getModuleFileName(arg, FILEPATH_SZ) == 0)
        return 0;

    pVec = ExArgv_Vector_create(1);
    if (pVec == NULL) {
        ExArgv_free(arg);
        return 0;
    }

    if (ExArgv_Vector_push(pVec, arg) == 0) {
        ExArgv_free(arg);
        return 0;
    }

    // Splits a command line passed in one line.
    s = pCmdLine;
    while ( (s = ExArgv_scanArgStr(s, arg, FILEPATH_SZ)) != NULL ) {
        if (ExArgv_Vector_push( pVec, arg ) == 0) {
            ExArgv_free(arg);
            return 0;
        }
    }
    ExArgv_free(arg);

    // Convert the work list to argc, argv and open the work list itself.
    return ExArgv_VectorToArgv( &pVec, pArgc, pppArgv ) != 0;
}

#if defined(EXARGV_ENCODE_UTF8)
int ExArgv_cmdLineToArgv( wchar_t const* pCmdLineW, int* pArgc, char*** pppArgv)
{
    char* pCmdLine = ExArgv_u8strdupFromWcs(pCmdLineW);
    int   rc       = ExArgv_cmdLineToArgvSub(pCmdLine, pArgc, pppArgv);
    ExArgv_free(pCmdLine);
    return rc;
}
#endif

/** From WinMain.
 */
#if defined(EXARGV_ENCODE_UTF8)
int ExArgv_forWinMain(wchar_t const* pCmdLine, int* pArgc, char*** pppArgv)
{
    if (ExArgv_cmdLineToArgv( pCmdLine, pArgc, pppArgv ))
        return ExArgv_conv(pArgc, pppArgv);
    return 0;
}
#else
int ExArgv_forWinMain(char_t const* pCmdLine, int* pArgc, char_t*** pppArgv)
{
    if (ExArgv_cmdLineToArgv( pCmdLine, pArgc, pppArgv ))
        return ExArgv_conv(pArgc, pppArgv);
    return 0;
}
#endif

#endif  // EXARGV_FOR_WINMAIN


// ===========================================================================

#if defined(EXARGV_ENCODE_UTF8) //|| defined(EXARGV_ENCODE_WCHAR) && (EXARGV_USE_RESFILE || EXARGV_USE_CONFIG)

/** Convert argv of whar_t string to utf8 string.
 *  If argc is 0, it is assumed that there is a terminating NULL pointer.
 */
char** ExArgv_wargvToUtf8(int argc, wchar_t* ppWargv[])
{
    char**  av;
    int     i;
    assert( ppWargv != NULL );

    if (argc == 0) {    // If argc is 0, count argv until NULL appears.
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

/** Convert utf8 string argv to wchar_t string.
 *  If argc is 0, it is assumed that there is a terminating NULL pointer.
 */
wchar_t** ExArgv_u8argvToWcs(int argc, char* ppArgv[])
{
    wchar_t** av;
    int       i;
    assert( ppArgv != NULL );

    if (argc == 0) {    // If argc is 0, count argv until NULL appears.
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

#endif  // defined(EXARGV_ENCODE_UTF8)



// ===========================================================================

/**
 */
static inline int ExArgv_isDirSep(int c)
{
  #if defined(DOSWIN32)
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

/** Is there a wildcard character?
 */
static unsigned  ExArgv_hasWildCard(char_t const* s)
{
  #if defined(DOSWIN32)
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


/** If there is a recursive **, make it one *.
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


// -    -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

#if defined(EXARGV_ENVNAME)
/** Get arguments from environment variables.
 */
static BOOL ExArgv_getAppEnv(char_t const* envName, ExArgv_Vector* pVec)
{
    BOOL    rc = 1;
    char_t* env;
    if (envName == NULL || envName[0] == 0)
        return 1;
    env = ExArgv_getenvDup(envName);
    if (env && env[0]) {
        char_t* arg = EXARGV_ALLOC(char_t, FILEPATH_SZ + 4);
        if (arg == NULL)
            return 0;
        while ( (env = ExArgv_scanArgStr(env, arg, FILEPATH_SZ)) != NULL ) {
            char_t const* p = arg;
            if (ExArgv_Vector_push( pVec, p ) == 0) {
                rc = 0;
                break;
            }
        }
        ExArgv_free(arg);
    }
    ExArgv_free(env);
    return rc;
}
#endif


#if EXARGV_USE_CONFIG
/** Load config file.
 */
static BOOL ExArgv_loadConfigFile(char_t const* exeName, ExArgv_Vector* pVec)
{
    char_t*       p;
    BOOL          rc     = 0;
    char_t const* ext    = MAC_TO_STR(EXARGV_CONFIG_EXT);
    size_t        extlen = STR_LEN(ext);
 #if defined(DOSWIN32)
    size_t        exelen = STR_LEN(exeName);
    char_t*       name   = EXARGV_ALLOC(char_t, exelen + extlen + 4);
    if (name == NULL)
        return 0;
    memcpy(name, exeName, (exelen+1) * sizeof(char_t));
    p = STR_R_CHR(name, T('.'));
    if (p) {
        memcpy(p, ext, (extlen+1)*sizeof(char_t));
    } else {
        memcpy(name+STR_LEN(name), ext, (extlen+1)*sizeof(char_t));
    }
 #else
    char_t*       base   = ExArgv_fnameBase(exeName);
    size_t        baselen= STR_LEN(base);
    size_t        nameCap= baselen + extlen + 8;
    char_t*       name   = EXARGV_ALLOC(char_t, nameCap);
    if (name == NULL)
        return 0;
    snprintf(name, nameCap, "~/.%s%s", base, ext);
 #endif
    rc = ExArgv_loadResponseFile(name, pVec, 1);
    ExArgv_free(name);
    return rc;
}
#endif


#if EXARGV_USE_RESFILE || EXARGV_USE_CONFIG

static BOOL ExArgv_getResponseArgs(ExArgv_Vector* pVec, unsigned char* src, size_t bytes, char_t const* fpath);

/** Load response file.
 */
static BOOL ExArgv_loadResponseFile(char_t const* fpath, ExArgv_Vector* pVec, BOOL notFoundOk)
{
    size_t          bytes = 0;
    unsigned char*  src   = (unsigned char*)ExArgv_fileLoadMalloc(fpath, &bytes);
    if (src == NULL) {
        if (notFoundOk) {
            return 1;
        } else {
         #if defined(EXARGV_ENCODE_WCHAR)
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
    if (bytes == 0)
    	return 1;
	return ExArgv_getResponseArgs(pVec, src, bytes, fpath);
}


static BOOL ExArgv_getResponseArgs(ExArgv_Vector* pVec, unsigned char* src, size_t bytes, char_t const* fpath)
{
    enum { BUF_SZ = 0x8010 * 3 };
    unsigned char*  s     = src;
    unsigned char*  src_e = src + bytes;
    unsigned        lno   = 1;
    BOOL            rc    = 1;
    unsigned char*  dst   = EXARGV_ALLOC(unsigned char, BUF_SZ+4);
    unsigned char*  dst_e;

    if (dst == NULL)
        return 0;
    dst_e = dst + BUF_SZ;

    src_e = src + bytes;
    s     = src;

    while (s < src_e) {
        unsigned char*  d;
        BOOL            dst_ovr = 0;
        unsigned        f = 0;
        unsigned        c;

        while (s < src_e && *s != '\n' && (*s <= 0x20 || *s == 0x7f))
            ++s;
        if (*s == '\n') {
            ++s;
            ++lno;
            continue;
        }
        if (*s == '#' /*|| *s == ';'*/) {
            while (s < src_e && *s != '\n')
                ++s;
            ++s;
            ++lno;
            continue;
        }
        d = dst;
        do {
            c = *s++;
            if (c == '\0' || c == '\n')
                break;
            // """ should be changed to ", but only " should be deleted.
            if (c == '"') {
                f ^= 1;
                if (*s == '"' && f == 0)
                    ++s;
                else
                    continue;
            }
            if (d < dst_e)
                *d++ = c;
            else
                dst_ovr = 1;
        } while (c > 0x20 || (f && c == ' '));
        --s;
        if (d > dst)
            *--d = 0;

        if (dst_ovr && fpath) {
         #if defined(EXARGV_ENCODE_WCHAR)
            char* fnm = ExArgv_u8strdupFromWcs(fpath);
         #else
            char const* fnm = fpath;
         #endif
            ERR_PUTS(fnm);
         #if defined(EXARGV_ENCODE_WCHAR)
            ExArgv_free(fnm);
         #endif
            ERR_PUTS(" : Argument too long.\n");
        }

        {
          #if defined(EXARGV_ENCODE_WCHAR)
            char_t* arg = ExArgv_wcsdupFromUtf8((char*)dst);
          #else
            char*   arg = (char*)dst;
          #endif
            rc = ExArgv_Vector_push(pVec, arg);
          #if defined(EXARGV_ENCODE_WCHAR)
            ExArgv_free(arg);
          #endif
            if (rc == 0)
                break;
        }
    }
    ExArgv_free(dst);
    return rc;
}

#endif


// ===========================================================================

#if EXARGV_USE_WC
/** Perform wildcard expansion and recursive operations.
 */
static BOOL ExArgv_wildCard(ExArgv_Vector* pVec)
{
    char_t**        pp;
    char_t**        ee;
    char_t*         name;
    ExArgv_Vector*  wk;
    int             mode;
    BOOL            optCk = 1;
    BOOL            hasWildCard = 0;

    ee = pVec->buf + pVec->size;
    for (pp = pVec->buf; pp != ee; ++pp)
        hasWildCard |= ExArgv_hasWildCard(*pp);
    if (hasWildCard == 0)
        return 1;

    wk = ExArgv_Vector_create( pVec->size+1 );
    if (wk == NULL)
        return 0;
    name = EXARGV_ALLOC(char_t, FILEPATH_SZ+4);
    if (name == NULL)
        return 0;
    for (pp = pVec->buf; pp != ee; ++pp) {
        char_t const* s = *pp;
      #if EXARGV_USE_WC
        BOOL   opt = 0;
        if (optCk && ExArgv_isOpt(*s)) {
            if (*s == '-' && s[1] == '-' && s[2] == 0)
                optCk = 0;
            opt = 1;
        }
        // When specifying a wildcard other than [0]
        if ( !opt && (pp != pVec->buf)
            && ((mode = ExArgv_hasWildCard( s )) != 0)
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

    // Set what was generated this time to pVec.
    if (wk) {
        // Release the original list.
        for (pp = pVec->buf; pp != ee; ++pp) {
            char_t* p = *pp;
            if (p)
                ExArgv_free(p);
        }
        ExArgv_free(pVec->buf);

        pVec->buf  = wk->buf;
        pVec->size = wk->size;
        pVec->capa = wk->capa;
        ExArgv_free(wk);
    }
    return 1;
}
#endif


#if defined(EXARGV_TOSLASH) || defined(EXARGV_TOBACKSLASH)
/** Converting \ and / in file path names.
 *  Option strings starting with - are not applicable.
 */
static void ExArgv_convBackSlash(ExArgv_Vector* pVec)
{
    char_t**    pp;
    char_t**    ee = pVec->buf + pVec->size;
    BOOL        optCk = 1;

    for (pp = pVec->buf; pp != ee; ++pp) {
        char_t* s = *pp;
        if (optCk && ExArgv_isOpt(*s)) {
            if (*s == '-' && s[1] == '-' && s[2] == 0)
                optCk = 0;
        } else {
          #if defined(EXARGV_TOSLASH)
            ExArgv_fname_backslashToSlash(s);       // \ -> / .
          #else
            ExArgv_fname_slashToBackslash(s);       // / -> \ .
          #endif
        }
    }
}
#endif


/** Generate (argc, argv) from pVec. Release ppVec.
 */
static char_t** ExArgv_VectorToArgv(ExArgv_Vector** ppVec, int* pArgc, char_t*** pppArgv)
{
    ExArgv_Vector*  pVec;
    char_t**        av;
    int             ac;

    assert( pppArgv != NULL && pArgc != NULL && ppVec != NULL );

    *pppArgv = NULL;
    *pArgc   = 0;

    pVec     = *ppVec;
    if (pVec == NULL)
        return NULL;

    ac       = (int)pVec->size;
    if (ac == 0)
        return NULL;

    *pArgc   = ac;
    av       = EXARGV_ALLOC(char_t*, ac + 2);
    if (av == NULL)
        return NULL;
    *pppArgv = av;

    memcpy(av, pVec->buf, sizeof(char_t*) * ac);

    av[ac]   = NULL;
    av[ac+1] = NULL;

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

/**
 */
void ExArgv_release(EXARGV_CHAR_T*** pppArgv)
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

#if defined(DOSWIN32)== 0 && defined(EXARGV_ENCODE_MBC)
// Assuming the environment variable LANG=ja_JP.SJIS.

static unsigned char    s_shift_char_type = 0;

/**
 */
static void  ExArgv_fname_check_locale()
{
    const char*         lang = getenv("LANG");
    s_shift_char_type  = 1;
    if (lang) {
        const char*     p    = strrchr(lang, '.');
        if (p) {
            ++p;
            // Check if the encoding requires measures against 0x5c. (Unconfirmed except for sjis)
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

/** Is it the 1st byte of DBC?
 */
static int ExArgv_fname_is_mbblead(unsigned c) {
  RETRY:
    switch (s_shift_char_type) {
    case 0 /* INIT */: ExArgv_fname_check_locale(); goto RETRY;
    case 2 /* SJIS */: return ((c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC));
    case 3 /* BIG5 */: return ((c >= 0xA1 && c <= 0xC6) || (c >= 0xC9 && c <= 0xF9));
    case 4 /* GBK  */: return  (c >= 0x81 && c <= 0xFE);
    default:           return  0;
    }
}
#endif  // defined(DOSWIN32)== 0 && defined(EXARGV_ENCODE_MBC)


/// Is it the 1st byte of DBC? (UTF8 and EUC have no problem with 0x5c(\\), so 0 is fine)
#if defined(EXARGV_ENCODE_WCHAR) || defined(EXARGV_ENCODE_MBC) == 0
//#define EXARGV_FNAME_ISMBBLEAD(c)     (0)
#elif defined(_WIN32)
 #define EXARGV_FNAME_ISMBBLEAD(c)      IsDBCSLeadByte((unsigned)c)
#elif defined(EXARGV_HAVE_MBCTYPE_H)
 #define EXARGV_FNAME_ISMBBLEAD(c)      _ismbblead(c)
#else
 #define EXARGV_FNAME_ISMBBLEAD(c)      ((c) >= 0x80 && ExArgv_fname_is_mbblead(c))
#endif

/// Advances the pointer to the next character.
#if defined(EXARGV_ENCODE_WCHAR) || defined(EXARGV_ENCODE_MBC) == 0
#define EXARGV_FNAME_CHARNEXT(p)        ((p) + 1)
#elif  defined(_WIN32)
#define EXARGV_FNAME_CHARNEXT(p)        (TCHAR*)CharNext((TCHAR*)(p))
#else
#define EXARGV_FNAME_CHARNEXT(p)        ((p) + 1 + (EXARGV_FNAME_ISMBBLEAD(*(unsigned char*)(p)) && (p)[1]))
#endif


#if defined(EXARGV_TOSLASH)
/** Replace \ with / in filePath.
 */
static char_t   *ExArgv_fname_backslashToSlash(char_t filePath[])
{
    char_t *p = filePath;
    while (*p != T('\0')) {
        if (*p == T('\\')) {
            *p = T('/');
        }
        p = EXARGV_FNAME_CHARNEXT(p);
    }
    return filePath;
}
#endif


#if defined(EXARGV_TOBACKSLASH)
/** Replace / with \ in filePath.
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


#if EXARG_USE_UTIL || defined(EXARGV_ENVNAME) || defined(EXARGV_FOR_WINMAIN)
/** Converts a string to a space-separated argument list.
 *  Spaces enclosed in '"' are not separated.
 *  " is removed, but '"""' is replaced with '"'.
 *  @return Returns the address after scan update. Returns NULL if str is EOS.
 */
char_t *ExArgv_scanArgStr(char_t const *str, char_t arg[], size_t argSz)
{
    const uchar_t*  s = (const uchar_t *)str;
    char_t*         d = arg;
    char_t*         e = d + argSz;
    unsigned        f = 0;
    int             c;

    if (s == NULL)
        return NULL;

    assert( str != NULL && arg != NULL && argSz > 1 );

    // Skip space.
    // while ( *s < 0x7f && isspace(*s) )
    while ((0 < *s && *s <= 0x20) || *s == 0x7f)
        ++s;

    if (*s == T('\0'))
        return NULL;

    do {
        c = *s++;
        if (c == T('"')) {
            f ^= 1;
            if (*s == T('"') && f == 0) // '"""' -> '"'
                ++s;
            else
                continue;               // Remove '"'
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


#if 1 //EXARG_USE_UTIL || EXARGV_USE_WC || EXARGV_USE_CONFIG
/** The location of the file name, excluding the directory from the path name.
 */
char_t*  ExArgv_fnameBase(char_t const* adr)
{
    char_t const *p = adr;
    while (*p != T('\0')) {
        if (
            ExArgv_isDirSep(*p)
         #if defined(_WIN32)
            || *p == T(':')
         #endif
        ) {
            adr = p + 1;
        }
        p = EXARGV_FNAME_CHARNEXT(p);
    }
    return (char_t*)adr;
}
#endif


#if EXARGV_USE_WC

/** File search with wildcard specification.
 *  @param pVec     Search results.
 *  @param srchName Search file name. Wildcard specified.
 *  @param recFlag  Presence of recursive search.
 */
static int  ExArgv_Vector_findFname(ExArgv_Vector* pVec, char_t const* srchName, int recFlag)
{
 #if defined(_WIN32)
    char_t*             pathBuf;
    char_t*             baseName;
    size_t              baseNameCapa;
    size_t              pathBufCapa;
    unsigned            num         = 0;
    ExArgv_finddata_t*  pFindData   = EXARGV_ALLOC(ExArgv_finddata_t, 1);
    HANDLE              hdl         = ExArgv_FindFirstFile(srchName, pFindData);

    if (hdl == INVALID_HANDLE_VALUE)
        return 0;

    pathBufCapa = STR_LEN(srchName) + 1 + FILENAME_LEN + 4;
    pathBuf  = EXARGV_ALLOC(char_t, pathBufCapa);
    if (pathBuf == NULL)
        return -1;

    str_l_cpy(pathBuf, srchName, pathBufCapa);

    baseName    = ExArgv_fnameBase(pathBuf);
    *baseName   = T('\0');
    baseNameCapa  = pathBufCapa - STR_LEN(pathBuf);
    assert(baseNameCapa > FILENAME_LEN);

    // Get file name. * Hidden files are not included.
    do {
        str_l_cpy(baseName, pFindData->cFileName, baseNameCapa);
        if ((pFindData->dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_HIDDEN)) == 0) {
            if (ExArgv_Vector_push( pVec, pathBuf ) == 0) {
                pVec = NULL;
                num  = -1;
                break;
            }
            ++num;
        }
    } while (ExArgv_FindNextFile(hdl, pFindData) != 0);
    ExArgv_FindClose(hdl);

   #if EXARGV_USE_WC_REC
    // Get file name using directory recursion.
    if (num >= 0 && recFlag && baseNameCapa >= 16) {
        char_t const* srch = ExArgv_fnameBase(srchName);
        str_l_cpy(baseName, T("*"), 4);
        hdl = ExArgv_FindFirstFile(pathBuf, pFindData);
        if (hdl != INVALID_HANDLE_VALUE) {
            do {
                str_l_cpy(baseName, pFindData->cFileName, baseNameCapa);
                if ((pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    if (STR_CMP(baseName, T(".")) == 0 || STR_CMP(baseName, T("..")) == 0
                        || (pFindData->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))   // Hidden folders are not included.
                    {
                        ;
                    } else {
                        int d;
                        str_l_cat(baseName, DIRSEP_STR, baseNameCapa);
                        str_l_cat(baseName, srch      , baseNameCapa);
                        d = ExArgv_Vector_findFname(pVec, pathBuf, 1);
                        if (d < 0) {
                            pVec = NULL;
                            num  = -1;
                            break;
                        }
                        num += d;
                    }
                }
            } while (ExArgv_FindNextFile(hdl, pFindData) != 0);
            ExArgv_FindClose(hdl);
        }
    }
   #elif defined(_MSC_VER) || defined(__WATCOMC__) || defined(__BORLANDC__)
    (void)recFlag;
   #endif

    ExArgv_free(pathBuf);
    ExArgv_free(pFindData);
    return num;

 #else // linux/unix...
    struct dirent** namelist = 0;
    unsigned        num      = 0;
    char_t*         pathBuf  = EXARGV_ALLOC(char_t, FILEPATH_SZ);
    int             dirNum;
    char_t*         srchBase = ExArgv_fnameBase(srchName);
    char_t*         baseName;
    size_t          baseNameCapa;
    int             noDirFlag = 0;

    if (pathBuf == NULL)
        return -1;

    str_l_cpy(pathBuf, srchName, FILEPATH_SZ);

    baseName    = ExArgv_fnameBase(pathBuf);

    // If there is no directory name, add the current directory(./)
    if (baseName == pathBuf) {
        str_l_cpy(pathBuf, T("./"), 3);
        baseName = pathBuf+2;
        noDirFlag     = 1;
    }
    *baseName   = 0;
    baseNameCapa  = FILEPATH_SZ - STR_LEN(pathBuf);
    assert(baseNameCapa >= MAX_PATH);

    // Get directory entries.
    baseName[-1] = 0;
    dirNum = scandir(pathBuf, &namelist, 0, alphasort);
    baseName[-1] = T('/');

    if (noDirFlag) {    // Remove temporary (./)
        baseName  = pathBuf;
        *baseName = T('\0');
    }

    if (namelist) {
        struct stat statBuf;
        int         i;

        // Get file names.
        for (i = 0; i < dirNum; ++i) {
            struct dirent* d = namelist[i];
            if (fnmatch(srchBase, d->d_name, FNM_NOESCAPE) == 0) {
                str_l_cpy(baseName, d->d_name, baseNameCapa);
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
        // Recurse if a directory exists.
        if (recFlag && baseNameCapa >= 16) {
            char_t const* srch = ExArgv_fnameBase(srchName);
            for (i = 0; i < dirNum; ++i) {
                struct dirent* d = namelist[i];
                str_l_cpy(baseName, d->d_name, baseNameCapa);
                if (stat(pathBuf, &statBuf) >= 0 && STR_CMP(baseName,T(".")) != 0 && STR_CMP(baseName,T("..")) !=0 ) {
                    if ((statBuf.st_mode & S_IFMT) == S_IFDIR) {
                        int d;
                        str_l_cat(baseName, T("/"), baseNameCapa);
                        str_l_cat(baseName, srch  , baseNameCapa);
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

        for (i = 0; i < dirNum; ++i)
            free( namelist[i] );
        free( namelist );
    }
    ExArgv_free( pathBuf );
    return num;
 #endif
}

#endif


// ===========================================================================

/** Create argument string list.
 */
static ExArgv_Vector* ExArgv_Vector_create(unsigned size)
{
    ExArgv_Vector* pVec = EXARGV_ALLOC( ExArgv_Vector, 1  );
    if (pVec) {
        size            = ((size + EXARGV_VECTOR_CAPA_BASE) / EXARGV_VECTOR_CAPA_BASE) * EXARGV_VECTOR_CAPA_BASE;
        pVec->capa      = size;
        pVec->size      = 0;
        pVec->buf       = EXARGV_ALLOC(char_t*, size);
        if (pVec->buf == NULL) {
            ExArgv_free(pVec);
            pVec = NULL;
        }
    }
    return pVec;
}


/** Add a string to the argument string list.
 */
static BOOL ExArgv_Vector_push(ExArgv_Vector* pVec, char_t const* pStr)
{
    if (pStr && pVec) {
        char_t*     p;
        unsigned    capa = pVec->capa;
        if (pVec->buf == NULL)
            return 0; //assert(pVec->buf != 0);
        if (pVec->size >= capa) {
            char_t**        buf;
            unsigned        newCapa = capa + EXARGV_VECTOR_CAPA_BASE;
            if (newCapa < capa) {   // Overflow?
                if (capa < 0xFFFFFFFF) {
                    newCapa = 0xFFFFFFFF;
                } else {
                    ERR_PUTS("Too many arguments.\n");
                    ExArgv_Vector_release(pVec);
                    return 0; //exit(1);
                }
            }
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
        pVec->buf[ pVec->size ] = p = ExArgv_strdupE(pStr);
        if (p == NULL) {
            ExArgv_Vector_release(pVec);
            return 0;
        }
        ++ pVec->size;
    } else {
        assert(pVec != NULL && pStr != NULL);
	}
    return 1;
}


// ===========================================================================

/** malloc
 */
static void* ExArgv_allocE(size_t size)
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
static char_t* ExArgv_strdupE(char_t const* s)
{
    char_t* p = ExArgv_strdupAddCapa(s, 0);
    if (p == NULL)
        ERR_PUTS("Not enough memory.\n");
    return p;
}

/** strdup to allocate extra memory.
 */
char_t* ExArgv_strdupAddCapa(char_t const* s, size_t add_capa)
{
    size_t   sl = STR_LEN(s) + 1;
    size_t   sz = sl + add_capa;
    char_t*  p  = (char_t*)malloc(sz * sizeof(char_t));
    if (p)
        memcpy(p, s, sl * sizeof(char_t));
    return p;

}

/** free
 */
void ExArgv_free(void* s)
{
    if (s)
        free(s);
}



// ===========================================================================

#if defined(_WIN32)

#if defined(EXARGV_ENCODE_UTF8) || defined(EXARGV_ENCODE_WCHAR) && (EXARGV_USE_RESFILE || EXARGV_USE_CONFIG)

#define EXARGV_WCS_FROM_MBS(d,dl,s,sl) MultiByteToWideChar(65001,0,(s),(int)(sl),(d),(int)(dl))
#define EXARGV_MBS_FROM_WCS(d,dl,s,sl) WideCharToMultiByte(65001,0,(s),(int)(sl),(d),(int)(dl),0,0)

/**
 */
char*  ExArgv_u8strdupFromWcs(wchar_t const* wcs) {
    size_t  len, wlen;
    char* u8s;
    if (wcs == NULL)
        wcs = L"";
    wlen = wcslen(wcs);
    len  = EXARGV_MBS_FROM_WCS(NULL,0, wcs, wlen);
    u8s  = EXARGV_ALLOC(char, len + 2);
    if (u8s)
        EXARGV_MBS_FROM_WCS(u8s, len + 1, wcs, wlen + 1);
    return u8s;
}

/**
 */
wchar_t*  ExArgv_wcsdupFromUtf8(char const* u8s) {
    size_t  len, wlen;
    wchar_t* wcs;
    if (u8s == NULL)
        u8s = "";
    len  = strlen(u8s);
    wlen = EXARGV_WCS_FROM_MBS(NULL,0, u8s, len);
    wcs  = EXARGV_ALLOC(wchar_t, wlen + 1);
    if (wcs)
        EXARGV_WCS_FROM_MBS(wcs, wlen + 1, u8s, len + 1);
    return wcs;
}
#endif


#if defined(EXARGV_ENCODE_UTF8) && EXARGV_USE_WC

enum { EXARGV_INNR_FPATH_MAX = 260 };

#define EXARGV_WPATH_FROM_CS_INI(n)    wchar_t __fSy_wpath_buf[n][EXARGV_INNR_FPATH_MAX+6], *__fSy_wpath_alc[n] = {NULL}
#define EXARGV_WPATH_FROM_CS_TERM(n)   do { int __i; for (__i = 0; __i < (n); ++__i) free(__fSy_wpath_alc[__i]);} while (0)

// * Note on optimization because it depends on the memory of the local scope array variable remaining.
#define EXARGV_WPATH_FROM_CS(__fSy_n, d, s) do {                                        \
        const char* __fSy_s  = (s);                                                     \
        wchar_t*    __fSy_d  = __fSy_wpath_buf[__fSy_n];                                \
        size_t      __fSy_sl, __fSy_dl;                                                 \
        __fSy_dl = ExArgv_priv_wpath_from_cs_subr1(__fSy_s, &__fSy_sl);                 \
        if (__fSy_dl >= EXARGV_INNR_FPATH_MAX)                                          \
            __fSy_wpath_alc[__fSy_n] = __fSy_d = (wchar_t*)malloc((__fSy_dl+6)*sizeof(wchar_t)); \
        (d) = ExArgv_priv_wpath_from_cs_subr2(__fSy_d, __fSy_dl, __fSy_s, __fSy_sl);    \
    } while (0)

/**
 */
size_t ExArgv_priv_wpath_from_cs_subr1(char const* s, size_t* pLen)
{
    size_t  len = strlen(s);
    *pLen = len;
    return (size_t)EXARGV_WCS_FROM_MBS(0,0,s,len);
}

/**
 */
wchar_t* ExArgv_priv_wpath_from_cs_subr2(wchar_t* d, size_t dl, char const* s, size_t sl)
{
    if (d) {
        wchar_t* d2 = d;
        if (dl >= EXARGV_INNR_FPATH_MAX
                && ((*s <= 'z') && (*s >= 'A') && (*s >= 'a' || *s <= 'Z'))
                && (s[1] == ':') && (s[2] == '\\' || s[2] == '/')
        ) {
            d[0] = d[1] = d[3] = (wchar_t)'\\';
            d[2] = (wchar_t)'?';
            d2   += 4;
        }
        EXARGV_WCS_FROM_MBS(d2, dl, s, sl);
        d2[dl] = 0;
    } else {
        static wchar_t dmy[1] = {0};
        assert(0 && "Path name is too long.");
        d = dmy;
    }
    return d;
}

#endif  // defined(EXARGV_ENCODE_UTF8)


/**
 */
void ExArgv_err_puts(char const* s)
{
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), (s), (DWORD)strlen(s), NULL, NULL);
}


#if defined(EXARGV_FOR_WINMAIN) || defined(EXARGV_USE_FULLPATH_ARGV0)
/**
 */
size_t ExArgv_getModuleFileName(char_t fpath[], size_t capa)
{
 #if defined(EXARGV_ENCODE_UTF8) == 0
    return GetModuleFileName(NULL, fpath, (DWORD)capa);
 #else  // defined(EXARGV_ENCODE_UTF8)
    DWORD    len   = 0;
    wchar_t* wpath = EXARGV_ALLOC(wchar_t, capa);
    if (wpath) {
        len  = GetModuleFileNameW(NULL, wpath, (DWORD)capa);
        if (len) {
            EXARGV_MBS_FROM_WCS(fpath, capa, wpath, wcslen(wpath)+1);
        }
    } else {
        len  = GetModuleFileNameA(NULL, fpath, (DWORD)capa);
    }
    return len;
 #endif
}
#endif


#if defined(EXARGV_ENCODE_UTF8) && (EXARGV_USE_UTIL || EXARGV_USE_RESFILE || EXARGV_USE_CONFIG || EXARGV_USE_WC)

/**
 */
static HANDLE ExArgv_FindFirstFile(char_t const* fpath, ExArgv_finddata_t* data)
{
    HANDLE           h     = NULL;
    wchar_t*         wpath = NULL;
    EXARGV_WPATH_FROM_CS_INI(1);
    if (fpath && data) {
        EXARGV_WPATH_FROM_CS(0, wpath, fpath);
        //h = FindFirstFileW(wpath, (WIN32_FIND_DATAW*)data);
        h = FindFirstFileExW(wpath,0/*FindExInfoStandard*/,(WIN32_FIND_DATAW*)data
                            ,0/*FindExSearchNameMatch*/,NULL,2/*FIND_FIRST_EX_LARGE_FETCH*/);
        if (h != INVALID_HANDLE_VALUE) {
            EXARGV_MBS_FROM_WCS(data->cFileName, sizeof(data->cFileName), data->cFileNameW, wcslen(data->cFileNameW)+1);
        }
    } else {
        h = INVALID_HANDLE_VALUE;
    }
    EXARGV_WPATH_FROM_CS_TERM(1);
    return h;
}

/**
 */
static BOOL   ExArgv_FindNextFile(HANDLE hdl, ExArgv_finddata_t* data)
{
    BOOL rc = FindNextFileW(hdl, (WIN32_FIND_DATAW*)data);
    if (rc)
        EXARGV_MBS_FROM_WCS(data->cFileName, sizeof(data->cFileName), data->cFileNameW, wcslen(data->cFileNameW)+1);
    return rc;
}

#endif


#endif  // _WIN32


// ===========================================================================


#if EXARGV_USE_UTIL || EXARGV_USE_RESFILE || EXARGV_USE_CONFIG
/** File size (bytes)
 */
size_t ExArgv_fileSize(char_t const* fpath)
{
 #if defined(_WIN32) == 0
    struct stat st;
    int   rc = stat(fpath, &st);
    return (rc == 0) ? (size_t)st.st_size : (size_t)-1;
 #else  // defined(_WIN32)
    if (fpath) {
        ExArgv_finddata_t d = {0};
        HANDLE          h = ExArgv_FindFirstFile(fpath, &d);
        if (h != INVALID_HANDLE_VALUE) {
            ExArgv_FindClose(h);
            #if defined(_WIN64)
                return (((size_t)d.nFileSizeHigh<<32) | (size_t)d.nFileSizeLow);
            #else
                return (d.nFileSizeHigh) ? (size_t)-1 : d.nFileSizeLow;
            #endif
        }
    }
    return (size_t)(-1);
 #endif
}

/** Load the file, add '\0'(*4) to the end and return malloced memory.
 */
void* ExArgv_fileLoadMalloc(char_t const* fpath, size_t* pSize)
{
    char*  buf;
    size_t rbytes = (size_t)-1;
    size_t bytes  = ExArgv_fileSize(fpath);
    if (bytes == (size_t)(-1))
        return NULL;

    buf    = EXARGV_ALLOC(char, bytes + 4);
    if (buf == NULL)
        return NULL;

 #if defined(_WIN32) == 0
  #if 1
    {
        FILE* fp = fopen(fpath, "rb");
        if (fp) {
            rbytes = fread(buf, 1, bytes, fp);
            fclose(fp);
        }
    }
  #else
    {
        int fd = open(fpath, O_RDONLY);
        if (fd != -1) {
            rbytes = read(fd, buf, bytes);
            close(fd);
        }
    }
  #endif
 #else  // defined(_WIN32)
  #if defined(EXARGV_ENCODE_UTF8) == 0
    {
        HANDLE hdl = CreateFile(fpath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        DWORD  r   = 0;
        if (hdl != INVALID_HANDLE_VALUE) {
            if (ReadFile(hdl, buf, (DWORD)bytes, &r, 0) == 0)
                r = 0;
            rbytes = r;
            CloseHandle(hdl);
        }
    }
  #else
    {
        wchar_t* wpath = NULL;
        HANDLE   hdl;
        EXARGV_WPATH_FROM_CS_INI(1);
        EXARGV_WPATH_FROM_CS(0, wpath, fpath);
        hdl = CreateFileW(wpath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (hdl != INVALID_HANDLE_VALUE) {
            DWORD  r   = 0;
            if (ReadFile(hdl, buf, (DWORD)bytes, &r, 0) == 0)
                r = 0;
            rbytes = r;
            CloseHandle(hdl);
        }
        EXARGV_WPATH_FROM_CS_TERM(1);
    }
  #endif
 #endif
    if (rbytes == bytes) {  // * bytes != (size_t)-1
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

#endif


#if EXARGV_USE_UTIL || defined(EXARGV_ENVNAME)
/** Malloc and return the contents of the environment variable.
 */
char_t* ExArgv_getenvDup(char_t const* envName)
{
 #if defined(_WIN32) == 0
    char* env = getenv(envName);
    return (env) ? ExArgv_strdupE(env) : NULL;
 #else // defined(_WIN32)
  #if defined(EXARGV_ENCODE_UTF8) == 0
    char_t* dst = NULL;
    char_t  buf[ 1024 + 1 ] = {0};
    DWORD   len = GetEnvironmentVariable(envName, buf, 1024);
    if (len == 0)
        return NULL;
    if (len < 1024) {
        dst = ExArgv_strdupE(buf);
    } else {
        dst = EXARGV_ALLOC(char_t, len+1);
        if (dst)
            GetEnvironmentVariable(envName, dst, len+1);
    }
    return dst;
  #else // defined(EXARGV_ENCODE_UTF8)
    DWORD   len;
    char*   dst = NULL;
    wchar_t wbuf[ 1024 + 1 ];
    wchar_t* wenvName = ExArgv_wcsdupFromUtf8(envName);
    if (wenvName == NULL)
        return NULL;
    len = GetEnvironmentVariableW(wenvName, wbuf, 1024);
    if (len < 1024) {
        dst = ExArgv_u8strdupFromWcs(wbuf);
    } else {
        wchar_t* wbuf2 = EXARGV_ALLOC(wchar_t, len+1);
        if (wbuf2) {
            GetEnvironmentVariableW(wenvName, wbuf2, len+1);
            dst = ExArgv_u8strdupFromWcs(wbuf2);
            ExArgv_free(wbuf2);
        }
    }
    ExArgv_free(wenvName);
    return dst;
  #endif
 #endif
}
#endif


// ===========================================================================

#if EXARGV_USE_UTIL
/** File existence check.
 *  @return 0:none  1:file  2:directory
 */
int ExArgv_fileExist(char_t const* fpath)
{
 #if defined(_WIN32) == 0
    struct stat st;
    int    rc = stat(fpath, &st);
    return (rc == 0) ? (S_ISDIR(statbuf.st_mode) ? 2 : 1) : 0;
 #else
    ExArgv_finddata_t   data = {0};
    HANDLE h = ExArgv_FindFirstFile(fpath, &data);
    if (h != INVALID_HANDLE_VALUE) {
        ExArgv_FindClose(h);
        return (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 2 : 1;
    }
    return 0;
 #endif
}
#endif


// ===========================================================================
