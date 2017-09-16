/**
 *  @file   ExArgv.c
 *  @brief  argc,argv�̊g������(���C���h�J�[�h,���X�|���X�t�@�C��).
 *  @author Masashi KITAMURA
 *  @date   2006-2010
 *  @note
 *  -   main(int argc,char* argv[]) ��argc,argv�ɑ΂��A
 *      ���C���h�J�[�h�w��⃌�X�|���X�t�@�C���w�蓙��W�J����argc,argv�ɕϊ�.
 *      main()�̏����[���炢�� ExArgv_conv(&argc, &argv); �̂悤�ɌĂяo��.
 *      ���邢�� WinMain() �ł�, ExArgv_forWinMain(cmdl, &argc, &argv);
 *
 *  -   ���C����dos/win�n(�̃R�}���h���C���c�[��)��z��.
 *      �ꉞ linux gcc�ł̃R���p�C����.
 *      (unix�n���ƃ��C���h�J�[�h�̓V�F���C�����낤�ŁA�����b�g���Ȃ�)
 *
 *  -   ExArgv.h�́A�ꉞ�w�b�_�����AExArgv.c �̐ݒ�t�@�C���ł�����.
 *      �A�v�����Ƃ� ExArgv.h ExArgv.c ���R�s�[���āAExArgv.h��
 *      �J�X�^�����Ďg���̂�z��.
 *  -   �ݒ�ł���v�f�́A
 *          - ���C���h�J�[�h (on/off)
 *          - ���C���h�J�[�h���̍ċA�w��(**)�̗L�� (on/off)
 *          - @���X�|���X�t�@�C�� (on/off)
 *          - .exe�A�� .cfg �t�@�C�� �Ǎ� (on/off)
 *          - �I�v�V�������ϐ����̗��p
 *          ��
 *
 *  -   ����������̐擪��'-'�Ȃ�΃I�v�V�������낤�ŁA���̕����񒆂�
 *      ���C���h�J�[�h�����������Ă��W�J���Ȃ�.
 *  -   �}�N�� UNICODE ����`����Ă���΁Awchar_t�p�A�łȂ����char�p.
 *  -   _WIN32 ����`����Ă���� win�p�A�łȂ���� unix�n��z��.
 */
 // 2009 �ċA�w���**�ɂ��邱�ƂŁA�d�l��P����.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef assert
#include <assert.h>
#endif

#ifdef UNICODE
#include <wchar.h>
#endif

// �w�b�_(�Ƃ��������[�U�ݒ�)�̓Ǎ�.
#ifndef EXARGV_INCLUDED
 #include "ExArgv.h"
#endif

#ifdef _MSC_VER
#pragma warning(disable:4996)                   // MS�̂��n���ȃZ�L�����e�B�֐��g���𖳎�.
#endif

/// ��`����Ƃ��̖��O�̊��ϐ��̒��g���R�}���h���C��������Ƃ��Ďg����悤�ɂ���.
//#define EXARGV_ENVNAME    "your_app_env_name"

#ifndef EXARGV_USE_WC
#define EXARGV_USE_WC       1                   ///< ���C���h�J�[�h�w�肪����΃t�@�C�����ɓW�J����.
#endif

#ifndef EXARGV_USE_WC_REC
#define EXARGV_USE_WC_REC   1                   ///< EXARGV_USE_WC���ɁA**������΍ċA���C���h�J�[�h�ɂ���.
#endif

#ifndef EXARGV_USE_RESFILE
#define EXARGV_USE_RESFILE  1                   ///< @���X�|���X�t�@�C����L���ɂ���.
#endif

#ifndef EXARGV_USE_CONFIG
#define EXARGV_USE_CONFIG   0                   ///< .exe��.cfg�ɒu�������p�X������Ǎ�.
#endif

#ifndef EXARGV_CONFIG_EXT
#define EXARGV_CONFIG_EXT   ".cfg"              ///< �R���t�B�O�t�@�C�����͗L�̎��̊g���q. �g���q��4�����ȓ��̂���.
#endif

#if 0 //ndef EXARGV_USE_FULLPATH_ARGV0
#define EXARGV_USE_FULLPATH_ARGV0   1           ///< argv[0] �̎��s�t�@�C�������t���p�X�ɂ���/���Ȃ�. win���̂�.
#endif


//#define EXARGV_TOSLASH                        ///< ��`����΁AfilePath���� \ �� / �ɒu��.
//#define EXARGV_TOBACKSLASH                    ///< ��`����΁AfilePath���� / �� \ �ɒu��.
//#define EXARGV_USE_SLASH_OPT                  ///< ��`����΁A/ ���I�v�V�����J�n�����Ƃ݂Ȃ�.
//#define EXARGV_USE_SETARGV                    ///< ����. ��`����ƁAsetargv�̑�p�i�Ƃ��ăR���p�C��(ExArgv_get������)



// ===========================================================================
// ���날�킹.

/*
#if defined _MSDOS || defined __DOS__
 #ifndef MSODS
  #define MSDOS     1                           // DOS�n��`. ���Ƃ����Ă�16�r�b�gDOS�ɂ͖��Ή�.
 #endif
#endif
*/

#if defined _WIN32 || defined MSODS
 #define DOSWIN32   1                           // DOS/WIN�n�Ȃ��`.
#endif


#if defined _WIN32
 #include <windows.h>
 #if defined _MSC_VER   // CharNext()�ŕK�v.
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
  #if defined DOSWIN32
   #define MAX_PATH     260
  #else
   #define MAX_PATH     1024
  #endif
 #endif
#endif



// ===========================================================================
// char,wchar_t �؂�ւ��̒��덇�킹�֌W.

#ifdef UNICODE
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
static unsigned char        s_wildMode;         ///< ���C���h�J�[�h�����񂪐ݒ肳��Ă�����on.
#endif

#if (EXARGV_USE_WC || EXARGV_USE_RESFILE) && !EXARGV_USE_CONFIG && !defined(EXARGV_ENVNAME) \
        && !defined(_WINDOWS) && !defined(EXARGV_USE_SETARGV) && !defined EXARGV_TOSLASH && !defined EXARGV_TOBACKSLASH
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
static void         ExArgv_VectorToArgv(ExArgv_Vector** pVec, int* pArgc, char_t*** pppArgv);
static void*        ExArgv_alloc(unsigned size);
static char_t*      ExArgv_strdup(const char_t* s);
static void         ExArgv_free(void* s);

#if EXARGV_USE_WC
static int          ExArgv_Vector_findFname(ExArgv_Vector* pVec, const char_t* pPathName, int recFlag);
static void         ExArgv_wildCard(ExArgv_Vector* pVec);
#endif
#if defined _WINDOWS || defined EXARGV_USE_SETARGV
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
#if EXARGV_USE_RESFILE || EXARGV_USE_CONFIG || defined EXARGV_ENVNAME || defined _WINDOWS || defined EXARGV_USE_SETARGV
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

#if defined UNICODE
_CRTIMP EXTERN_C wchar_t *_wcmdln;
/** vc++ �ŁAmain()�ɓn����� argc,argv �𐶐����鏈��(������ɒu��������)
 */
EXTERN_C int __cdecl __wsetargv (void)
{
    return ExArgv_forCmdLine1( _wcmdln, &__argc, &__wargv);
}
#else
_CRTIMP EXTERN_C char *_acmdln;
/** vc++ �ŁAmain()�ɓn����� argc,argv �𐶐����鏈��(������ɒu��������)
 */
EXTERN_C int __cdecl __setargv (void)
{
    return ExArgv_forCmdLine1( _acmdln, &__argc, &__argv);
}
#endif



#elif defined _WINDOWS

/** win�A�v���ŁAWinMain�����[�ŁAargc,argv����肽���Ƃ��Ɏg���̂�z��.
 */
void ExArgv_forWinMain(const char_t* pCmdLine, int* pArgc, char_t*** pppArgv)
{
    ExArgv_forCmdLine1(pCmdLine, pArgc, pppArgv);
}

#endif



#if defined _WINDOWS || defined EXARGV_USE_SETARGV
/** 1�s�̕�����pCmdLine ����argc,argv�𐶐�. (��Ɋ��ϐ���.cfg�t�@�C��������)
 */
static int ExArgv_forCmdLine1(const char_t* pCmdLine, int* pArgc, char_t*** pppArgv)
{
    ExArgv_Vector*  pVec;
    char_t          arg[ FILEPATH_SZ + 4 ];
    const char_t*   s;
    int             n;

    assert(pArgc != 0 && pppArgv != 0);
    if (pArgc == 0 || pppArgv == 0)
        return -1;

    pVec = ExArgv_Vector_create(1);                 // ��Ɨp�̃��X�g��p��.
    if (pVec == 0)
        return -1;

    // ���s�t�@�C�����𓾂āA����������[�ɓo�^.
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

    // ���ϐ��̎擾.
  #ifdef EXARGV_ENVNAME
    assert(strlen(EXARGV_ENVNAME) > 0);
    ExArgv_getEnv(EXARGV_ENVNAME, pVec);
  #endif

    // �R���t�B�O�t�@�C���̓Ǎ�.
  #if EXARGV_USE_CONFIG
    ExArgv_getCfgFile(pVec->buf[0], pVec );
  #endif

  #if EXARGV_USE_WC
    s_wildMode  = 0;
  #endif

    // 1�s�œn�����R�}���h���C���𕪊�.
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
        ExArgv_wildCard(pVec);                      // ���C���h�J�[�h��f�B���N�g���ċA���ăp�X���擾.
  #endif
  #if (defined EXARGV_TOSLASH) || (defined EXARGV_TOBACKSLASH)
    ExArgv_convBackSlash(pVec);                     // define�ݒ�ɏ]���āA\ �� / �̕ϊ�. (��{�I�ɂ͉������Ȃ�)
  #endif

    ExArgv_VectorToArgv( &pVec, pArgc, pppArgv );   // ��ƃ��X�g�� argc,argv �ɕϊ����A��ƃ��X�g���̂͊J��.

    return 0;
}
#endif



// ===========================================================================

#if (defined _WINDOWS) == 0 && (defined EXARGV_USE_SETARGV) == 0

/** argc,argv �����X�|���X�t�@�C���⃏�C���h�J�[�h�W�J���āAargc, argv���X�V���ĕԂ�.
 *  @param  pArgc       argc�̃A�h���X.(argv�̐�)
 *  @param  pppArgv     argv�̃A�h���X.
 */
void ExArgv_conv(int* pArgc, char_t*** pppArgv)
{
    int             argc;
    char_t**        ppArgv;
    ExArgv_Vector*  pVec;
    int             i;

    assert( pArgc != 0 && pppArgv != 0 );
    if (pArgc == 0 || pppArgv == 0)
        return;

    ppArgv = *pppArgv;
    argc   = *pArgc;
    assert(argc > 0 && ppArgv != 0);
    if (argc == 0 || ppArgv == 0)
        return;

  #if defined EXARGV_USE_FULLPATH_ARGV0 && defined _WIN32       // �Â��\�[�X�p�ɁAexe�̃t���p�X��ݒ�.
   #if defined _MSC_VER     // vc�Ȃ炷�łɂ���̂ł���𗬗p.
    ppArgv[0] = _pgmptr;
   #elif defined __GNUC__   // �킩��Ȃ��̂Ń��W���[�����擾.
    {
        static char_t nm[MAX_PATH];
        if (GetModuleFileName(NULL, nm, MAX_PATH) > 0)
            ppArgv[0] = nm;
    }
   #endif
  #endif

    if (argc < 2)
        return;

  #if !EXARGV_USE_CONFIG && !defined(EXARGV_ENVNAME) && !defined(EXARGV_TOSLASH) && !defined(EXARGV_TOBACKSLASH)
   #if !EXARGV_USE_WC && !EXARGV_USE_RESFILE
    return;     // �قڕϊ�����...
   #else
    if (ExArgv_checkWcResfile(argc, ppArgv) == 0)   // �����argc,argv��M��K�v�����邩?
        return;
   #endif
  #endif

    pVec = ExArgv_Vector_create(argc+1);            // argv����������̂ŁA��Ɨp�̃��X�g��p��.

    //x printf("@4 %d %p(%p)\n", argc, ppArgv, *ppArgv);
    //x printf("   %p: %p %d %d\n", pVec, pVec->buf, pVec->capa, pVec->size);

    // ���s�t�@�C�����̎擾.
    if (argc > 0)
        ExArgv_Vector_push( pVec, ppArgv[0] );      // Vec�ɓo�^.

    // ���ϐ��̎擾.
  #ifdef EXARGV_ENVNAME
    assert(strlen(EXARGV_ENVNAME) > 0);
    ExArgv_getEnv(EXARGV_ENVNAME, pVec);
  #endif

    // �R���t�B�O�t�@�C���̓Ǎ�.
  #if EXARGV_USE_CONFIG
    ExArgv_getCfgFile( ppArgv[0], pVec );
  #endif

    //x printf("%p %x %#x %p\n",pVec, pVec->capa, pVec->size, pVec->buf);

  #if EXARGV_USE_WC
    s_wildMode  = 0;
  #endif

    // �����̏���.
    for (i = 1; i < argc; ++i) {
        const char_t* p = ppArgv[i];
      #if EXARGV_USE_RESFILE
        if (i > 0 && *p == T('@')) {
            ExArgv_getResFile(p+1, pVec, 0);        // ���X�|���X�t�@�C���Ǎ�.
        } else
      #endif
        {
          #if EXARGV_USE_WC
            s_wildMode |= ExArgv_fname_isWildCard(p);
          #endif
            ExArgv_Vector_push( pVec, p );          // Vec�ɓo�^.
        }
    }

  #if EXARGV_USE_WC
    if (s_wildMode)
        ExArgv_wildCard(pVec);                      // ���C���h�J�[�h��f�B���N�g���ċA���ăp�X���擾.
  #endif
  #if (defined EXARGV_TOSLASH) || (defined EXARGV_TOBACKSLASH)
    ExArgv_convBackSlash(pVec);                     // define�ݒ�ɏ]���āA\ �� / �̕ϊ�. (��{�I�ɂ͉������Ȃ�)
  #endif

    ExArgv_VectorToArgv( &pVec, pArgc, pppArgv );   // ��ƃ��X�g�� argc,argv �ɕϊ����A��ƃ��X�g���̂͊J��.
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



static inline void str_l_cpy(char_t* d, const char_t* s, unsigned l)
{
    const char_t*   e = d + l - 1;
    while (d < e && *s) {
        *d++ = *s++;
    }
    *d = T('\0');
}



static inline void str_l_cat(char_t* d, const char_t* s, unsigned l)
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

/// ���C���h�J�[�h�������������Ă��邩?
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



/// ���J�[�V�u�w���**������΁A*��ɂ���.
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
/// ����������z��ɁA���X�|���X�t�@�C���w��A���C���h�J�[�h�w��A���J�[�V�u�w�肪���邩�`�F�b�N.
static unsigned ExArgv_checkWcResfile(int argc, char_t** argv)
{
    int         i;
    unsigned    rc    = 0;

    for (i = 1; i < argc; ++i) {
        const char_t* p = argv[i];
      #if EXARGV_USE_RESFILE
        if (*p == T('@')) {
            rc |= 4;    // ���X�|���X�t�@�C���w��.
        } else
      #endif
        {
          #if EXARGV_USE_WC
            if (ExArgv_isOpt(*p) == 0) {
                int mode = ExArgv_fname_isWildCard(p);
                s_wildMode |= mode;
                if (mode > 0) {
                    rc |= 1;    // ���C���h�J�[�h�w��.
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
/// ���ϐ�������΁A�o�^.
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
/// �R���t�B�O�t�@�C���̓ǂݍ���.
static void ExArgv_getCfgFile(const char_t* exeName, ExArgv_Vector* pVec)
{
    char_t  name[ FILEPATH_SZ+4 ];
    char_t* p = name;

    // ���s�t�@�C��������R���t�B�O�p�X���𐶐�.
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
/** ���X�|���X�t�@�C����(argc,argv)�� pVec �ɕϊ�.
 * ���X�|���X�t�@�C���⃏�C���h�J�[�h�A���J�[�V�u���̏������s��.
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
            if (c == T(';') || c == T('#') || c == T('\0')) {   // ��s��R�����g�̎�.
                break;
            }
            // �ċA�����w��,���C���h�J�[�h�̗L�����`�F�b�N.
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
/** ���C���h�J�[�h�A�ċA����.
 */
static void ExArgv_wildCard(ExArgv_Vector* pVec)
{
    char_t**        pp;
    char_t**        ee;
    ExArgv_Vector*  wk;
    int             mode;

    // �č\�z.
    wk = ExArgv_Vector_create( pVec->size+1 );
    ee = pVec->buf + pVec->size;
    for (pp = pVec->buf; pp != ee; ++pp) {
        const char_t* s = *pp;
      #if EXARGV_USE_WC
        if (   ExArgv_isOpt(*s) == 0                    // �I�v�V�����ȊO�̕������,
            && (pp != pVec->buf)                        // �����[�ȊO([0]�͎��s�t�@�C�����Ȃ̂Ō��������Ȃ�)�̂Ƃ���,
            && ((mode = ExArgv_fname_isWildCard( s )) != 0) // ���C���h�J�[�h�w��̂���̂Ƃ�.
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

    // ���̃��X�g���J��.
    for (pp = pVec->buf; pp != ee; ++pp) {
        char_t* p = *pp;
        if (p)
            ExArgv_free(p);
    }
    ExArgv_free(pVec->buf);

    // ���񐶐��������̂��ApVec�ɐݒ�.
    pVec->buf  = wk->buf;
    pVec->size = wk->size;
    pVec->capa = wk->capa;

    // ��ƂɎg�������������J��.
    ExArgv_free(wk);
}
#endif



#if (defined EXARGV_TOSLASH) || (defined EXARGV_TOBACKSLASH)
/** �t�@�C��(�p�X)������ \ / �̕ϊ�. -�Ŏn�܂�I�v�V����������͑ΏۊO.
 *  �ŋ߂�win���ł͂ǂ���̎w��ł�ok�Ȃ̂ŁA�����ɕϊ�����K�v�Ȃ�.
 *  (�I�v�V�������Ƀt�@�C����������ƌ��ǎ��O�ŕϊ������邦�Ȃ��̂ŁA�����ł��Ȃ��ق��������)
 */
static void ExArgv_convBackSlash(ExArgv_Vector* pVec)
{
    char_t**    pp;
    char_t**    ee = pVec->buf + pVec->size;

    for (pp = pVec->buf; pp != ee; ++pp) {
        char_t* s = *pp;
        if (ExArgv_isOpt(*s) == 0) {        // �I�v�V�����ȊO�̕�����ŁA
          #if (defined EXARGV_TOSLASH)
            ExArgv_fname_backslashToSlash(s);       // \ �� / �ɒu��.
          #else
            ExArgv_fname_slashToBackslash(s);       // / �� \ �ɒu��.
          #endif
        } else {                            // �I�v�V�����Ȃ�A����ɕϊ����Ȃ��ł���.
            ;
        }
    }
}
#endif



/** pVec����A(argc,argv)�𐶐�. ppVec�͊J������.
 */
static void ExArgv_VectorToArgv(ExArgv_Vector** ppVec, int* pArgc, char_t*** pppArgv)
{
    ExArgv_Vector*  pVec;
    char_t**        av;
    int             ac;

    assert( pppArgv != 0 && pArgc != 0 && ppVec != 0 );

    *pppArgv = NULL;
    *pArgc   = 0;

    pVec     = *ppVec;
    if (pVec == NULL)
        return;

    ac       = (int)pVec->size;
    if (ac == 0)
        return;

    // char_t*�z��̂��߂̃��������擾.
    *pArgc   = ac;
    av       = (char_t**) ExArgv_alloc(sizeof(char_t*) * (ac + 2));
    *pppArgv = av;

    memcpy(av, pVec->buf, sizeof(char_t*) * ac);
    av[ac]   = NULL;
    av[ac+1] = NULL;

    // ��ƂɎg�������������J��.
    ExArgv_free(pVec->buf);
    ExArgv_free(pVec);
    *ppVec   = NULL;
}




// ===========================================================================

#if defined DOSWIN32 == 0 && defined UNICODE == 0   // ���ϐ� LANG=ja_JP.SJIS �̂悤�ȏ�Ԃ�O��.

static unsigned char        s_shift_char_type = 0;

static void  ExArgv_fname_check_locale()
{
    const char*         lang = getenv("LANG");
    s_shift_char_type  = 1;
    if (lang) {
        // ja_JP.SJIS �̂悤�Ȍ`���ł��邱�Ƃ�O���SJIS,big5,gbk�����`�F�b�N.
        const char*     p    = strrchr(lang, '.');
        if (p) {
            ++p;
            // 0x5c�΍􂪕K�v��encoding�����`�F�b�N. (sjis�ȊO�͖��`�F�b�N)
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


/// ���� C �� MS�S�p�̂P�o�C�g�ڂ��ۂ�. (utf8��euc�� \ ���͖����̂� 0���A���ok)
#if defined _WIN32
 #define ExArgv_FNAME_ISMBBLEAD(c)      IsDBCSLeadByte(c)
#elif defined UNICODE
 //#define ExArgv_FNAME_ISMBBLEAD(c)    (0)
#elif defined HAVE_MBCTYPE_H
 #define ExArgv_FNAME_ISMBBLEAD(c)      _ismbblead(c)
#else
 #define ExArgv_FNAME_ISMBBLEAD(c)      ((c) >= 0x80 && ExArgv_fname_is_mbblead(c))
#endif


/// ���̕����փ|�C���^��i�߂�. ��CharNext()���T���Q�[�g�y�A��utf8�Ή����Ă���Ă��炢���ȂƊ���(�ʖڂ���������)
#if  defined _WIN32
#define ExArgv_FNAME_CHARNEXT(p)        (TCHAR*)CharNext((TCHAR*)(p))
#elif defined UNICODE
#define ExArgv_FNAME_CHARNEXT(p)        ((p) + 1)
#else
#define ExArgv_FNAME_CHARNEXT(p)        ((p) + 1 + (ExArgv_FNAME_ISMBBLEAD(*(unsigned char*)(p)) && (p)[1]))
#endif



#if defined EXARGV_TOSLASH
/** filePath���� \ �� / �ɒu��.
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
/** filePath���� / �� \ �ɒu��.
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



#if EXARGV_USE_RESFILE || EXARGV_USE_CONFIG || defined EXARGV_ENVNAME || defined _WINDOWS || defined EXARGV_USE_SETARGV
/** �R�}���h���C���Ŏw�肳�ꂽ�t�@�C�����Ƃ��āA""���l������,
 *  �󔒂ŋ�؂�ꂽ������(�t�@�C����)���擾.
 *  @return �X�L�����X�V��̃A�h���X��Ԃ��Bstr��EOS��������NULL��Ԃ�.
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

    // �󔒂��X�L�b�v.
    // while ( *s < 0x7f && isspace(*s) )
    while ((0 < *s && *s <= 0x20) || *s == 0x7f)    // ascii,sjis,utf8,utf16 �Ȃ炱��ł���...
        ++s;

    if (*s == T('\0'))  // EOS��������A������Ȃ������Ƃ���NULL��Ԃ�.
        return NULL;

    do {
        c = *s++;
        if (c == T('"')) {
            f ^= 1;                     // "�̑΂̊Ԃ͋󔒂��t�@�C�����ɋ���.���߂̃t���O.

            // ������ƋC�����������AWin(XP)��cmd.exe�̋����ɍ��킹�Ă݂�.
            // (�ق�Ƃɂ����Ă邩�A�\���ɂ͒��ׂĂȂ�)
            if (*s == T('"') && f == 0) // ��"�̒���ɂ����"������΁A����͂��̂܂ܕ\������.
                ++s;
            else
                continue;               // �ʏ�� " �͏Ȃ��Ă��܂�.
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
/** �t�@�C���p�X�����̃f�B���N�g�����������t�@�C�����̈ʒu��Ԃ�.
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

/** srchName�Ŏw�肳�ꂽ�p�X��(���C���h�J�[�h�����Ή�) �Ƀ}�b�`����p�X����S�� pVec �ɓ���ĕԂ�.
 *  recFlag ���^�Ȃ�ċA�������s��.
 */
static int  ExArgv_Vector_findFname(ExArgv_Vector* pVec, const char_t* srchName, int recFlag)
{
  #if defined _WIN32        // ��dos���Ή�(���l�ɍ쐬�\������)
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
        // �t�@�C�������擾. �� �B���t�@�C���͑ΏۊO�ɂ��Ă���.
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
    // �f�B���N�g���ċA�Ńt�@�C�������擾.
    if (recFlag && baseNameSz >= 16) {
        const char_t* srch = ExArgv_fname_baseName(srchName);
        str_l_cpy(baseName, T("*.*"), 4);
        hdl = FindFirstFile(pathBuf, pFindData);
        if (hdl != INVALID_HANDLE_VALUE) {
            do {
                str_l_cpy(baseName, pFindData->cFileName, baseNameSz);
                if ((pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    if (STR_CMP(baseName, T(".")) == 0 || STR_CMP(baseName, T("..")) == 0
                        || (pFindData->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))           // �B���t�H���_�͑ΏۊO.
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

  #else // linux/unix�n...
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

    if (baseName == pathBuf) {          // �f�B���N�g�����������ꍇ.
        str_l_cpy(pathBuf, T("./"), 3); // �J�����g�w����ꎞ�I�ɐݒ�.
        baseName = pathBuf+2;
        flag     = 1;
    }
    *baseName   = 0;
    baseNameSz  = FILEPATH_SZ - STR_LEN(pathBuf);
    assert(baseNameSz >= MAX_PATH);

    // �f�B���N�g���G���g���̎擾.
    baseName[-1] = 0;
    dirNum = scandir(pathBuf, &namelist, 0, alphasort);
    baseName[-1] = T('/');

    if (flag) { // �ꎞ�I�ȃJ�����g�w�肾�����Ȃ�΁A�̂Ă�.
        baseName  = pathBuf;
        *baseName = T('\0');
    }

    if (namelist) {
        struct stat statBuf;
        int         i;

        // �t�@�C�������擾.
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
        // �f�B���N�g��������΍ċA.
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

        // �g�������������J��.
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


/** ���������񃊃X�g���Ǘ����鍪�����쐬.
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



/** ���������񃊃X�g�ɁA�������ǉ�.
 */
static void ExArgv_Vector_push(ExArgv_Vector* pVec, const char_t* pStr)
{
    assert(pVec != 0);
    assert(pStr  != 0);
    if (pStr && pVec) {
        unsigned    capa = pVec->capa;
        assert(pVec->buf != 0);
        if (pVec->size >= capa) {   // �L���p�𒴂��Ă�����A���������m�ۂ��Ȃ���.
            char_t**        buf;
            unsigned        newCapa = capa + EXARGV_VECTOR_CAPA_BASE;
          #if 1 // ���ʂ̓R���ȑO�Ƀ������s���ɂȂ邾�낤���ꉞ.
            if (newCapa < capa) {   // ��ꂽ��G���[...
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
