/**
 *  @file   ExArgv.c
 *  @brief  main(int argc,char* argv[]) ��argc,argv��,
 *  	    ���X�|���X�t�@�C���A���C���h�J�[�h�W�J�������̂Ɋg�����邽�߂̊֐�.
 *  @author Masashi KITAMURA
 *  @date   2006,2007
 *  @note
 *  -	dos/win�n�̃R�}���h���C���v���O�����p��z��.<br>
 *  	�������A��r�I�y�ɁA���C���h�J�[�h�Ή����f�B���N�g���ċA�\��
 *  	���邽�߂̃��[�`��.
 *  -	�����ǃI�v�V���������� '-' ��p. ('/'�Ή��\�薳)
 *  	'-'�Ŏn�܂镶����́A���C���h�J�[�h��ċA�����̑ΏۂɂȂ�Ȃ�.
 *  -	unix�n�R�}���h�ł� -x YYYY �̂悤�ȋ󔒂̓���I�v�V�����w��ɂ͕s����.<br>
 *  	���̎�̂��Ƃ�����Ȃ� getopt �Ƃ���.
 *  -	�ꉞ�A�����͈͂�����linux gcc�ł̃R���p�C���Ή�. (cygwin)<br>
 *  	����linux(unix)���ƃV�F�������C���h�J�[�h�W�J�ł��邵
 *  	��@���Ⴄ�̂ŁA�o�Ԃ͂Ȃ�����.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "ExArgv.h"


//#define EXARGV_NO_WC_REC  	// ��`����΁A���C���h�J�[�h�����@�\��f�B���N�g���ċA�����Ȃ�.
//#define EXARGV_TOSLASH    	// ��`����΁AfilePath���� \ �� / �ɒu��.
//#define EXARGV_TOBACKSLASH	// ��`����΁AfilePath���� / �� \ �ɒu��.

#ifdef EXARGV_TINY  	    	// �^�C�j�[�ł�, "@c"����ъ��ϐ��̂ݗL��. ���C���h�J�[�h��dir�ċA�s��.
#define EXARGV_NO_WC_REC
#endif




// ===========================================================================
// ���날�킹.

#if defined _WIN32 || defined _WIN64
#include <windows.h>
//x #include <mbctype.h>
#else	// linux
#include <dirent.h>
#include <sys/stat.h>
#include <fnmatch.h>
#define stricmp     strcasecmp
#endif


#undef BOOL
#if defined __cplusplus
#define BOOL	bool
#else
#define BOOL	int
#define inline __inline
#endif


#define STDERR	    stderr

#ifndef MAX_PATH
#define MAX_PATH    260
#endif




// ===========================================================================

enum {FILEPATH_SZ = (MAX_PATH*2 > 4096) ? MAX_PATH*2 : 4096 };

/// flags �ɐݒ肷��l.
enum EXARGV_EFlags {
    EXARGV_RESFILE    = 0x01,	    	///< @���X�|���X�t�@�C����L���ɂ���.
    EXARGV_WILDCARD   = 0x02,	    	///< ���C���h�J�[�h�w�肪����΃t�@�C�����ɓW�J����.
    EXARGV_RECURSIVE  = 0x04,	    	///< �T�u�f�B���N�g�����ċA�I�Ɍ���.
    EXARGV_IGNORECASE = 0x08,	    	///< �ċA�I�v�V�����̕������r�ŁA�啶���������𖳎�.
    EXARGV_OPTEND     = 0x10,	    	///< -- �ɂ��I�v�V�����̏I���w�肠��.
    EXARGV_REC_WC     = 0x20,	    	///< �ċA�������A���C���h�J�[�h�w�莞�݂̂ɂ���.
    EXARGV_CONFIG     = 0x40,	    	///< .exe��.cfg�ɒu�������p�X������Ǎ�.
};


static unsigned     s_flags;	    	///< �t���O
static const char*  s_pRecOpt1;     	///< �ċA�����w��̃I�v�V����������1.
static const char*  s_pRecOpt2;     	///< �ċA�����w��̃I�v�V����������2.
static BOOL 	    s_recFlg;	    	///< �ċA�����I�v�V�������w�肳��Ă�����on.
static BOOL 	    s_wildCFlg;     	///< ���C���h�J�[�h�����񂪐ݒ肳��Ă�����on.
static BOOL 	    s_optEndFlg;    	///< �I�v�V�����I���� -- �̎w�肪�����on.



// ===========================================================================

typedef struct ExArgv_Vector {
    char**  	    buf;
    unsigned	    size;
    unsigned	    capa;
} ExArgv_Vector;

static ExArgv_Vector* ExArgv_Vector_create(unsigned size);
static void 	      ExArgv_Vector_push(ExArgv_Vector* pVec, const char* pStr);
static int  	      ExArgv_Vector_findFname(ExArgv_Vector* pVec, const char* pPathName, int recFlag);

static void 	ExArgv_initVar(const char* flags, const char* recOpt1, const char* recOpt2);
static unsigned ExArgv_checkWcResfile(int argc, char** argv);
static BOOL 	ExArgv_optEqu(const char* a, const char* b);
static void 	ExArgv_argvToVector(int argc, char** ppArgv, const char* envName, ExArgv_Vector* pVec);
static void 	ExArgv_getEnv(const char* envName, ExArgv_Vector* pVec);
static void 	ExArgv_getCfgFile(const char* exeName, ExArgv_Vector* pVec);
static void 	ExArgv_getResFile(const char* fname, ExArgv_Vector* pVec);
static void 	ExArgv_wildCard(ExArgv_Vector* pVec);
static int  	ExArgv_checkArgOpt(const char* p);
static unsigned ExArgv_flagsStr2u(const char* mode);
static void 	ExArgv_VectorToArgv(ExArgv_Vector** pVec, int* pArgc, char*** pppArgv);

#if (defined EXARGV_TOSLASH) || (defined EXARGV_TOBACKSLASH)
static void 	ExArgv_convBackSlash(ExArgv_Vector* pVec);
static char 	*fname_slashToBackslash(char filePath[]);
static char 	*fname_backslashToSlash(char filePath[]);
#else
#define     	ExArgv_convBackSlash(pVec)  	// �w�肪�Ȃ��ꍇ�́A�������Ȃ�.
#endif

static BOOL 	fname_ismbblead(unsigned char c);
static char *	fname_baseName(const char *adr);
static char *	fname_scanArgStr(const char *str, char arg[], int argSz);

static void*	ExArgv_malloc(unsigned size);
static char*	ExArgv_strdup(const char* s);
static void 	ExArgv_free(void* s);



// ===========================================================================

/** argc,argv �����X�|���X�t�@�C���⃏�C���h�J�[�h�W�J���āAargc, argv���X�V���ĕԂ�.
 * recOpt1,2��WC���ɍċA��������ꍇ�̃I�v�V����������.
 *  @param  pArgc   	argc�̃A�h���X.(argv�̐�)
 *  @param  pppArgv 	argv�̃A�h���X.
 *  @param  flags   	�t���O������.
 *  	    	    	@   	@response_file �L��.
 *  	    	    	*   	wildcard �����L��.
 *  	    	    	--  	"--"�ɂ��I�v�V�����I���w��L��.
 *  	    	    	i   	�ċA�I�v�V����������̑啶���������𔻕ʂ��Ȃ�.
 *  	    	    	r   	���C���h�J�[�h�w��̂݁A�f�B���N�g���ċA��������.
 *  	    	    	R   	�t�@�C�����͑S�ăf�B���N�g���ċA��������.
 *  	    	    	C   	.exe��.cfg�ɕς����t�@�C���p�X���̃R���t�B�O��ǂݍ���.
 *  @param  recOpt1 	�f�B���N�g���ċA�w��. ���̂P.
 *  @param  recOpt2 	�f�B���N�g���ċA�w��. ���̂Q(�����O�I�v�V�������ŕ����\������ꍇ�p)
 */
void ExArgv_get(int* pArgc, char*** pppArgv, const char* flags, const char* envName, const char* recOpt1, const char* recOpt2)
{
    int     	    f;
    int     	    argc;
    char**  	    ppArgv;
    ExArgv_Vector*  pVec;

    assert( pArgc != 0 && pppArgv != 0 );

    ExArgv_initVar(flags, recOpt1, recOpt2);	    // ��ƕϐ�������
    ppArgv = *pppArgv;
    argc   = *pArgc;
    assert(argc > 0 && ppArgv != 0);
  #ifdef _MSC_VER   	    	    	    	    // �Â��\�[�X�p�ɁAexe�̃t���p�X��ݒ�.
    ppArgv[0] = _pgmptr;
  #endif
    if (argc < 2 || ppArgv == 0)
    	return;

    f = ExArgv_checkWcResfile(argc, ppArgv);	    // �����argc,argv��M��K�v�����邩?
    if (f == 0 && (envName==NULL||envName[0] == 0))
    	return;     	    	    	    	    // �Ȃ������炩����.

    pVec = ExArgv_Vector_create(argc+1);    	    // argv����������̂ŁA��Ɨp�̃��X�g��p��.

    //x printf("@4 %d %p(%p)\n", argc, ppArgv, *ppArgv);
    //x printf("   %p: %p %d %d\n", pVec, pVec->buf, pVec->capa, pVec->size);

    ExArgv_argvToVector(argc, ppArgv, envName,pVec);// .cfg��@���X�|���X���������Ȃ���A argv�����Ɨp���X�g���쐬.
    ExArgv_wildCard(pVec);  	    	    	    // ���C���h�J�[�h��f�B���N�g���ċA���ăp�X���擾.
    ExArgv_convBackSlash(pVec);     	    	    // define�ݒ�ɏ]���āA�ꍇ�ɂ���Ă� \ �� / �ɂ��Ă̕ϊ������܂�. (��{�I�ɂ͉������Ȃ�).

    ExArgv_VectorToArgv( &pVec, pArgc, pppArgv );   // ��ƃ��X�g�� argc,argv �ɕϊ����A��ƃ��X�g���̂͊J��.
}




#ifdef _WINDOWS
/** win�A�v���ŁAWinMain�����[�ŁAargc,argv�����ꍇ�p.
 */
void ExArgv_forWinMain(const char* pCmdLine, int* pArgc, char*** pppArgv, const char* flags, const char* envName, const char* recOpt1, const char* recOpt2)
{
    ExArgv_Vector*  pVec;
    char    	    arg[ FILEPATH_SZ + 4 ];
    const char*     s;
    int     	    n;

    assert(pArgc != 0 && pppArgv != 0);

    ExArgv_initVar(flags, recOpt1, recOpt2);	    // ��ƕϐ�������

    pVec = ExArgv_Vector_create(1); 	    	    // ��Ɨp�̃��X�g��p��.

    // ���s�t�@�C�����𓾂āA����������[�ɓo�^.
    n = GetModuleFileName(NULL, arg, FILEPATH_SZ);
    if (n > 0) {
    	ExArgv_Vector_push(pVec, arg);
    } else {
      #ifdef _MSC_VER	    // �Â��\�[�X�p�ɁAexe�̃t���p�X��ݒ�.
    	ExArgv_Vector_push(pVec, _pgmptr);
      #endif
    }

    // 1�s�œn�����R�}���h���C���𕪊�.
    s = pCmdLine;
    while ( (s = fname_scanArgStr(s, arg, FILEPATH_SZ)) != NULL ) {
    	const char* p = arg;
    	if (*p == '@' && (s_flags & EXARGV_RESFILE)) {
    	    ExArgv_getResFile(p+1, pVec);
    	} else if (ExArgv_checkArgOpt(p)) { 	    // �ċA�����w��,�I�v�V�����I���"--",���C���h�J�[�h�̗L�����`�F�b�N.
    	    ExArgv_Vector_push( pVec, p );
    	}
    }

    ExArgv_wildCard(pVec);  	    	    	    // ���C���h�J�[�h��f�B���N�g���ċA���ăp�X���擾.
    ExArgv_convBackSlash(pVec);     	    	    // define�ݒ�ɏ]���āA�ꍇ�ɂ���Ă� \ �� / �ɂ��Ă̕ϊ������܂�. (��{�I�ɂ͉������Ȃ�).

    ExArgv_VectorToArgv( &pVec, pArgc, pppArgv );   // ��ƃ��X�g�� argc,argv �ɕϊ����A��ƃ��X�g���̂͊J��.
}
#endif




// ===========================================================================

/// �t�@�C��static�ϐ��̏�����.
static void ExArgv_initVar(const char* flags, const char* recOpt1, const char* recOpt2)
{
    s_flags 	= ExArgv_flagsStr2u(flags);
    s_optEndFlg = 0;
  #if defined(EXARGV_NO_WC_REC) == 0
    s_pRecOpt1	= recOpt1;
    s_pRecOpt2	= recOpt2;
    s_recFlg	= 0;
    s_wildCFlg	= 0;
    assert( (s_pRecOpt1 == 0 && s_pRecOpt2 == 0) || ((s_flags & EXARGV_RECURSIVE) != 0) );
  #else
    recOpt1;
    recOpt2;
  #endif
}



static unsigned ExArgv_flagsStr2u(const char* mode)
{
    unsigned flags = 0;
    const char *s = mode;
    if (s && *s) {
    	do {
    	    switch (*s) {
    	    case '@':	flags |= EXARGV_RESFILE;    	    	 break;
    	  #if defined(EXARGV_NO_WC_REC) == 0
    	    case '*':	flags |= EXARGV_WILDCARD;   	    	 break;
    	    case 'i':	flags |= EXARGV_IGNORECASE; 	    	 break;
    	    case 'r':	flags |= EXARGV_RECURSIVE|EXARGV_REC_WC; break;
    	    case 'R':	flags |= EXARGV_RECURSIVE;  	    	 break;
    	  #endif
    	    case 'C':	flags |= EXARGV_CONFIG;     	    	 break;
    	    case '-':
    	    	if (s[1] == '-') {
    	    	    flags |= EXARGV_OPTEND;
    	    	    ++s;
    	    	}
    	    	break;
    	    case ' ':
    	    case '\t':
    	    	break;
    	    default:
    	    	assert(0);
    	    }
    	} while (*++s);
    }

    return flags;
}



/// �I�v�V�����I����\�� "--" ���ǂ���.
static BOOL ExArgv_isOptEnd(const char* s)
{
    return (s_flags & EXARGV_OPTEND) && (memcmp(s, "--", 3) == 0);
}



#if defined(EXARGV_NO_WC_REC) == 0

/// ���C���h�J�[�h�������������Ă��邩?
static inline BOOL  fname_isWildCard(const char* s) {
  #if defined _WIN32 || defined _WIN64
    return strpbrk(s, "*?") != 0;
  #else // linux(fnmatch)
    return strpbrk(s, "*?[]\\") != 0;
  #endif
}



/// �I�v�V���������� a �� b �͓�����?
static inline BOOL ExArgv_optEqu(const char* a, const char* b) {
    return (s_flags & EXARGV_IGNORECASE) ?  (stricmp(a,b) == 0) : (strcmp(a,b) == 0);
}



/// �I�v�V���������� s �́A�ċN�����w�肩?
static BOOL ExArgv_strEquRecOpt(const char* s)
{
    return  ( (s_flags & EXARGV_RECURSIVE)
    	    	&& (   (s_pRecOpt1 && ExArgv_optEqu(s, s_pRecOpt1))
    	    	    || (s_pRecOpt2 && ExArgv_optEqu(s, s_pRecOpt2)) )
    	    );
}

#endif



/// ����������z��ɁA���X�|���X�t�@�C���w��A���C���h�J�[�h�w��A���J�[�V�u�w�肪���邩�`�F�b�N.
static unsigned ExArgv_checkWcResfile(int argc, char** argv)
{
    int     	i;
    unsigned	flags = s_flags;
    unsigned	rc    = 0;

    if (flags & EXARGV_CONFIG)
    	return 0x10;

    for (i = 1; i < argc; ++i) {
    	const char* p = argv[i];
    	if (*p == '@' && (flags & EXARGV_RESFILE)) {
    	    rc |= 1;	// ���X�|���X�t�@�C���w��.
      #if defined(EXARGV_NO_WC_REC) == 0
    	} else if (fname_isWildCard(p) && (flags & EXARGV_WILDCARD)) {
    	    rc |= 2;	// ���C���h�J�[�h�w��.
    	} else if (ExArgv_strEquRecOpt(p)) {
    	    rc |= 4;	// �ċA�w��.
      #endif
    	} else if (ExArgv_isOptEnd(p)) {
    	    rc |= 8;	// �I�v�V�����̏I���w��(-�Ŏn�܂�t�@�C�����������w��).
    	}
    }
    return rc;
}



/// �����������`�F�b�N.
static int ExArgv_checkArgOpt(const char* p)
{
  #if defined(EXARGV_NO_WC_REC) == 0
    if (ExArgv_strEquRecOpt(p)) {
    	s_recFlg = 1;	    	// �ċA�����w�肪������.
    	return 0;   	    	// Vec�ɓo�^���Ȃ�������.

    } else {
    	if (ExArgv_isOptEnd(p))
    	    s_optEndFlg = 1;	// �I�v�V�����̏I����������.
      #if defined(EXARGV_NO_WC_REC) == 0
    	else if (fname_isWildCard(p) && (s_flags & EXARGV_WILDCARD))
    	    s_wildCFlg	= 1;	// ���C���h�J�[�h�w�肪������.
      #endif
    	return 1;   	    	// ���X�g�ɓo�^���镶����.
    }
  #else
    if (ExArgv_isOptEnd(p))
    	s_optEndFlg = 1;    	// �I�v�V�����̏I����������.
    return 1;	    	    	// ���X�g�ɓo�^���镶����.
  #endif
}




/// (argc,argv)�� pVec �ɕϊ�. ���X�|���X�t�@�C���⃏�C���h�J�[�h�A���J�[�V�u���̏������s��.
static void ExArgv_argvToVector(int argc, char** ppArgv, const char* envName, ExArgv_Vector* pVec)
{
    int i;

    // ���s�t�@�C�����̎擾
    if (argc > 0)
    	ExArgv_Vector_push( pVec, ppArgv[0] );	// Vec�ɓo�^.

    // ���ϐ��̎擾
    if (envName && envName[0])
    	ExArgv_getEnv(envName, pVec);

    // �R���t�B�O�t�@�C���̓Ǎ�.
    if (s_flags & EXARGV_CONFIG)
    	ExArgv_getCfgFile( ppArgv[0], pVec );

    //x printf("%p %x %#x %p\n",pVec, pVec->capa, pVec->size, pVec->buf);

    // �����̏���.
    for (i = 1; i < argc; ++i) {
    	const char* p = ppArgv[i];
    	if (i > 0 && *p == '@' && (s_flags & EXARGV_RESFILE)) {
    	    ExArgv_getResFile(p+1, pVec);   // ���X�|���X�t�@�C���Ǎ�.
    	} else if (ExArgv_checkArgOpt(p)) { // �ċA�����w��,�I�v�V�����I���"--",���C���h�J�[�h�̗L�����`�F�b�N.
    	    ExArgv_Vector_push( pVec, p );  // Vec�ɓo�^.
    	}
    }
}



/// ���ϐ�������΁A�o�^.
static void ExArgv_getEnv(const char* envName, ExArgv_Vector* pVec)
{
    const char* env = getenv(envName);
    if (env && env[0]) {
    	char	    	arg[ FILEPATH_SZ + 4 ];
    	while ( (env = fname_scanArgStr(env, arg, FILEPATH_SZ)) != NULL ) {
    	    const char* p = arg;
    	    if (ExArgv_checkArgOpt(p))	    	// �ċA�����w��,�I�v�V�����I���"--",���C���h�J�[�h�̗L�����`�F�b�N.
    	    	ExArgv_Vector_push( pVec, p );
    	}
    }
}



/// �R���t�B�O�t�@�C���̓ǂݍ���
static void ExArgv_getCfgFile(const char* exeName, ExArgv_Vector* pVec)
{
    FILE*   fp;
    char    name[ FILEPATH_SZ+4 ];
    char*   p;

    // ���s�t�@�C��������R���t�B�O���𐶐�.
    strncpy(name, exeName, FILEPATH_SZ-4);
    name[FILEPATH_SZ] = '\0';
    p = strrchr(name, '.');
    if (p)
    	strcpy(p, ".cfg");
    else
    	strcat(name, ".cfg");

    // �t�@�C���̑��݃`�F�b�N.
    fp = fopen(name, "rb");
    if (fp) {	// �t�@�C������������ǂݍ���.
    	fclose(fp);
    	ExArgv_getResFile(name, pVec);
    }
}



/// ���X�|���X�t�@�C����(argc,argv)�� pVec �ɕϊ�.
/// ���X�|���X�t�@�C���⃏�C���h�J�[�h�A���J�[�V�u���̏������s��.
static void ExArgv_getResFile(const char* fname, ExArgv_Vector* pVec)
{
    unsigned	n = 0;
    char    	buf[32*1024+512];
    FILE*   	fp;
    if ((s_flags & EXARGV_RESFILE) == 0)
    	return;
    fp = fopen(fname, "rt");
    if (fp == NULL) {
    	fprintf(STDERR, "Response-file '%s' is not opened.\n", fname);
    	exit(1);    // return;
    }
    while (fgets(buf, sizeof buf, fp)) {
    	char	arg[FILEPATH_SZ + 4];
    	char*	s = buf;
    	while ( (s = fname_scanArgStr(s, arg, FILEPATH_SZ)) != NULL ) {
    	    int c = ((unsigned char*)arg)[0];
    	    if (c == ';' || c == '#' || c == '\0') {	// ��s��R�����g�̎�.
    	    	break;
    	    } else if (ExArgv_checkArgOpt(arg)) {   // �ċA�����w��,�I�v�V�����I���"--",���C���h�J�[�h�̗L�����`�F�b�N.
    	    	ExArgv_Vector_push(pVec, arg );
    	    }
    	}
    }
    if (ferror(fp)) {
    	fprintf(STDERR, "%s (%d) : file read error.\n", fname, n);
    	exit(1);
    }
    fclose(fp);
}



/// ���C���h�J�[�h�A�ċA����.
///
static void ExArgv_wildCard(ExArgv_Vector* pVec)
{
  #if defined(EXARGV_NO_WC_REC) == 0
    BOOL    	    fnameOnly = 0;
  #endif
    char**  	    pp;
    char**  	    ee;
    ExArgv_Vector*  wk;

    // �ċA�w������C���h�J�[�h���I�v�V�����I�����w�肳��Ă��Ȃ��Ȃ�A
    // �킴�킴���X�g�č\�z����K�v�Ȃ��Ȃ̂ŁA������.
    if (s_recFlg == 0 && s_wildCFlg == 0 && s_optEndFlg == 0)
    	return;

    // �č\�z.
    wk = ExArgv_Vector_create( pVec->size+1 );
    ee = pVec->buf+pVec->size;
    for (pp = pVec->buf; pp != ee; ++pp) {
    	const char* s = *pp;
      #if defined(EXARGV_NO_WC_REC) == 0
    	if (   (*s != '-' || fnameOnly)     	    	    // �I�v�V�����ȊO�̕�����ŁA
    	    && ( ((s_flags & EXARGV_REC_WC)==0 && s_recFlg) // �S�t�@�C���ċA�`�F�b�N����ꍇ��
    	    	 || fname_isWildCard( s )    )	    	    // �@���C���h�J�[�h�w��̂���A
    	    && (pp != pVec->buf)    	    	    	    // �����[�ȊO([0]�͎��s�t�@�C�����Ȃ̂Ō��������Ȃ�)�̂Ƃ�
    	 ){
    	    ExArgv_Vector_findFname(wk, s, s_recFlg);

    	} else	{
    	    if (ExArgv_isOptEnd(s)) 	    	    	    // -- �w��ȍ~�̓I�v�V���������A�t�@�C���������Ƃ��Ĉ���.
    	    	fnameOnly = 1;
    	    ExArgv_Vector_push( wk, s );
    	}
      #else
    	ExArgv_Vector_push( wk, s );
      #endif
    }

    // ���̃��X�g���J��.
    for (pp = pVec->buf; pp != ee; ++pp) {
    	char* p = *pp;
    	if (p)
    	    ExArgv_free(p);
    }
    ExArgv_free(pVec->buf);

    // ���񐶐��������̂��ApVec�ɐݒ�.
    pVec->buf  = wk->buf;
    pVec->size = wk->size;
    pVec->capa = wk->capa;

    // ��ƂɎg�������������J��
    ExArgv_free(wk);
}



#if (defined EXARGV_TOSLASH) || (defined EXARGV_TOBACKSLASH)
/// �t�@�C��(�p�X)������ \ / �̕ϊ�. -�Ŏn�܂�I�v�V����������͑ΏۊO.
/// �ŋ߂�win���ł͂ǂ���̎w��ł�ok�Ȃ̂ŁA�����ɕϊ�����K�v�Ȃ�.
/// (�I�v�V�������Ƀt�@�C����������ƌ��ǎ��O�ŕϊ������邦�Ȃ��̂ŁA�����ł��Ȃ��ق��������)
///
static void ExArgv_convBackSlash(ExArgv_Vector* pVec)
{
    BOOL    	fnameOnly = 0;
    char**  	pp;
    char**  	ee = pVec->buf + pVec->size;

    for (pp = pVec->buf; pp != ee; ++pp) {
    	char* s = *pp;
    	if (*s != '-' || fnameOnly) {	    // �I�v�V�����ȊO�̕�����ŁA
    	  #if (defined EXARGV_TOSLASH)
    	    fname_backslashToSlash(s);	    // \ �� / �ɒu��.
    	  #else
    	    fname_slashToBackslash(s);	    // / �� \ �ɒu��.
    	  #endif
    	} else if (ExArgv_isOptEnd(s)) {    // -- �w��ȍ~�̓I�v�V���������A�t�@�C���������Ƃ��Ĉ���.
    	    fnameOnly = 1;
    	} else {    	    	    	    // �I�v�V�����Ȃ�A����ɕϊ����Ȃ��ł���.
    	    ;
    	}
    }
}
#endif



/// pVec����A(argc,argv)�𐶐�. ppVec�͊J������.
static void ExArgv_VectorToArgv(ExArgv_Vector** ppVec, int* pArgc, char*** pppArgv)
{
    ExArgv_Vector*  pVec;
    char**  	    av;
    int     	    ac;

    assert( pppArgv != 0 && pArgc != 0 && ppVec != 0 );

    *pppArgv = NULL;
    *pArgc   = 0;

    pVec     = *ppVec;
    if (pVec == NULL)
    	return;

    ac	     = (int)pVec->size;
    if (ac == 0)
    	return;

    // char*�z��̂��߂̃��������擾.
    *pArgc   = ac;
    av	     = (char**) ExArgv_malloc(sizeof(char*) * (ac + 2));
    *pppArgv = av;

    memcpy(av, pVec->buf, sizeof(char*) * ac);
    av[ac]   = NULL;
    av[ac+1] = NULL;

    // ��ƂɎg�������������J��.
    ExArgv_free(pVec->buf);
    ExArgv_free(pVec);
    *ppVec   = NULL;
}




// ===========================================================================

//x #define DIRSEP_STR	    "\\"
#define DIRSEP_STR  	    "/"


/// ���� C �� MS�S�p�̂P�o�C�g�ڂ��ۂ�. (utf8��euc�� \ ���͖����̂� 0���A���ok)
static inline BOOL fname_ismbblead(unsigned char c)
{
  #if defined _WIN32 || defined _WIN64
    return IsDBCSLeadByte(c) != 0;
  #elif defined HAVE_MBCTYPE_H
    return _ismbblead(c) != 0;
  #elif defined USE_SJIS
    return (c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFE);
  #else
    return 0;
  #endif
}



#if defined EXARGV_TOSLASH
/// filePath���� \ �� / �ɒu��.
static char 	*fname_backslashToSlash(char filePath[])
{
    char *p;
    for (p = filePath; *p != '\0'; ++p) {
    	if (*p == '\\') {
    	    *p = '/';
    	} else if (fname_ismbblead((*(unsigned char *)p)) && *(p+1) ) {
    	    ++p;
    	}
    }
    return filePath;
}
#endif



#if defined EXARGV_TOBACKSLASH
/// filePath���� / �� \ �ɒu��.
static char 	*fname_slashToBackslash(char filePath[])
{
    char *p;
    for (p = filePath; *p != '\0'; ++p) {
    	if (*p == '/') {
    	    *p = '\\';
    	}
    }
    return filePath;
}
#endif



/// �R�}���h���C���Ŏw�肳�ꂽ�t�@�C�����Ƃ��āA""���l������,
/// �󔒂ŋ�؂�ꂽ������(�t�@�C����)���擾.
/// @return �X�L�����X�V��̃A�h���X��Ԃ��Bstr��EOS��������NULL��Ԃ�.
static char *fname_scanArgStr(const char *str, char arg[], int argSz)
{
    const unsigned char* s = (const unsigned char*)str;
    char*   	    	 d = arg;
    char*   	    	 e = d + argSz;
    unsigned	    	 f = 0;
    int     	    	 c;

    assert( str != 0 && arg != 0 && argSz > 1 );

    // �󔒂��X�L�b�v.
    while ( isspace(*s) )
    	++s;

    if (*s == '\0') // EOS��������A������Ȃ������Ƃ���NULL��Ԃ�.
    	return NULL;

    do {
    	c = *s++;
    	if (c == '"') {
    	    f ^= 1; 	    	    	// "�̑΂̊Ԃ͋󔒂��t�@�C�����ɋ���.���߂̃t���O.

    	    // ������ƋC�����������AWin(XP)��cmd.exe�̋����ɍ��킹�Ă݂�.
    	    // (�ق�Ƃɂ����Ă邩�A�\���ɂ͒��ׂĂȂ�)
    	    if (*s == '"' && f == 0)	// ��"�̒���ɂ����"������΁A����͂��̂܂ܕ\������.
    	    	++s;
    	    else
    	    	continue;   	    	// �ʏ�� " �͏Ȃ��Ă��܂�.
    	}
    	if (d < e) {
    	    *d++ = (char)c;
    	}
    } while (c >= 0x20 && (c != ' ' || f != 0));
    *--d  = '\0';
    --s;
    return (char *)s;
}



/// �t�@�C���p�X�����̃f�B���N�g�����������t�@�C�����̈ʒu��Ԃ�.
static char *fname_baseName(const char *adr)
{
    const char *p;
    for (p = adr; *p != '\0'; ++p) {
    	if (*p == ':' || *p == '/'
    	  #if defined _WIN32 || defined _WIN64
    	    || *p == '\\'
    	  #endif
    	) {
    	    adr = p + 1;
    	}
    	if (fname_ismbblead((*(unsigned char *)p)) && *(p+1) )
    	    ++p;
    }
    return (char*)adr;
}



#if 0
char** fname_matchs(const char* srchName, int recFlag, int* pArgc)
{
    ExArgv_Vector* pVec = ExArgv_Vector_create(1);
    char**   ppArgv;
    ExArgv_Vector_findFname(pVec, srchName, recFlag);
    assert(pArgc != 0);
    *pArgc   = pVec->size;
    ppArgv   = pVec->buf;
    ExArgv_free(pVec);
    return ppArgv;
}
#endif


#if defined(EXARGV_NO_WC_REC) == 0

/** srchName�Ŏw�肳�ꂽ�p�X��(���C���h�J�[�h�����Ή�) �Ƀ}�b�`����p�X����S�� pVec �ɓ���ĕԂ�.
 *  recFlag ���^�Ȃ�ċA�������s��.
 */
static int  ExArgv_Vector_findFname(ExArgv_Vector* pVec, const char* srchName, int recFlag)
{
  #if defined _WIN32 || defined _WIN64
    unsigned	    	num 	    = 0;
    WIN32_FIND_DATA*	pFindData   = (WIN32_FIND_DATA*)ExArgv_malloc(sizeof(WIN32_FIND_DATA));
    HANDLE  	    	hdl 	    = FindFirstFile(srchName, pFindData);
    char*   	    	pathBuf;
    char*   	    	baseName;
    size_t  	    	baseNameSz;

    pathBuf  = (char*)ExArgv_malloc(FILEPATH_SZ);
    strncpy(pathBuf, srchName, FILEPATH_SZ);
    pathBuf[ FILEPATH_SZ-1 ] = '\0';

    baseName	= fname_baseName(pathBuf);
    *baseName	= 0;
    baseNameSz	= FILEPATH_SZ - strlen(pathBuf);
    assert(baseNameSz >= MAX_PATH);

    if (hdl != INVALID_HANDLE_VALUE) {
    	// �t�@�C�������擾.
    	do {
    	    strncpy(baseName, pFindData->cFileName, baseNameSz);
    	    pathBuf[ FILEPATH_SZ-1 ] = '\0';	//x baseName[baseNameSz-1] = '\0';
    	    if ((pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
    	    	ExArgv_Vector_push( pVec, pathBuf );
    	    	++num;
    	    }
    	} while (FindNextFile(hdl, pFindData) != 0);
    	FindClose(hdl);
    }

    // �t�@�C�������擾.
    if (recFlag && baseNameSz >= 16) {
    	const char* srch = fname_baseName(srchName);
    	strcpy(baseName, "*.*");
    	hdl = FindFirstFile(pathBuf, pFindData);
    	if (hdl != INVALID_HANDLE_VALUE) {
    	    do {
    	    	strncpy(baseName, pFindData->cFileName, baseNameSz);
    	    	pathBuf[ FILEPATH_SZ-1 ] = '\0';    //x baseName[baseNameSz-1] = '\0';
    	    	if ((pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
    	    	    if (strcmp(baseName, ".") == 0 || strcmp(baseName, "..") == 0) {
    	    	    	;
    	    	    } else {
    	    	    	strncat(baseName, DIRSEP_STR, baseNameSz);
    	    	    	strncat(baseName, srch, baseNameSz);
    	    	    	num += ExArgv_Vector_findFname(pVec, pathBuf, 1);
    	    	    }
    	    	}
    	    } while (FindNextFile(hdl, pFindData) != 0);
    	    FindClose(hdl);
    	}
    }

    ExArgv_free(pathBuf);
    ExArgv_free(pFindData);
    return num;
  #else
    struct dirent** namelist = 0;
    unsigned	    num      = 0;
    char*   	    pathBuf  = (char*)ExArgv_malloc(FILEPATH_SZ);
    int     	    dirNum;
    char*   	    srchBase = fname_baseName(srchName);
    char*   	    baseName;
    size_t  	    baseNameSz;
    int     	    flag = 0;

    strncpy(pathBuf, srchName, FILEPATH_SZ);
    pathBuf[ FILEPATH_SZ-1 ] = '\0';

    baseName	= fname_baseName(pathBuf);

    if (baseName == pathBuf) {	// �f�B���N�g�����������ꍇ.
    	strcpy(pathBuf, "./");	// �J�����g�w����ꎞ�I�ɐݒ�.
    	baseName = pathBuf+2;
    	flag	 = 1;
    }
    *baseName	= 0;
    baseNameSz	= FILEPATH_SZ - strlen(pathBuf);
    assert(baseNameSz >= MAX_PATH);

    // �f�B���N�g���G���g���̎擾.
    baseName[-1] = 0;
    dirNum = scandir(pathBuf, &namelist, 0, alphasort);
    baseName[-1] = '/';

    if (flag) { // �ꎞ�I�ȃJ�����g�w�肾�����Ȃ�΁A�̂Ă�.
    	baseName  = pathBuf;
    	*baseName = '\0';
    }

    if (namelist) {
    	struct stat statBuf;
    	int 	    i;

    	// �t�@�C�������擾.
    	for (i = 0; i < dirNum; ++i) {
    	    struct dirent* d = namelist[i];
    	    if (fnmatch(srchBase, d->d_name, 0) == 0) {
    	    	strncpy(baseName, d->d_name, baseNameSz);
    	    	pathBuf[ FILEPATH_SZ-1 ] = '\0';    //x baseName[baseNameSz-1] = '\0';
    	    	if (stat(pathBuf, &statBuf) >= 0) {
    	    	    if ((statBuf.st_mode & S_IFMT) != S_IFDIR) {
    	    	    	ExArgv_Vector_push( pVec, pathBuf );
    	    	    	++num;
    	    	    }
    	    	}
    	    }
    	}

    	// �f�B���N�g��������΍ċA.
    	if (recFlag && baseNameSz >= 16) {
    	    const char* srch = fname_baseName(srchName);
    	    for (i = 0; i < dirNum; ++i) {
    	    	struct dirent* d = namelist[i];
    	    	strncpy(baseName, d->d_name, baseNameSz);
    	    	pathBuf[ FILEPATH_SZ-1 ] = '\0';    //x baseName[baseNameSz-1] = '\0';
    	    	if (stat(pathBuf, &statBuf) >= 0 && strcmp(baseName,".") != 0 && strcmp(baseName,"..") !=0 ) {
    	    	    if ((statBuf.st_mode & S_IFMT) == S_IFDIR) {
    	    	    	strncat(baseName, "/", baseNameSz);
    	    	    	strncat(baseName, srch, baseNameSz);
    	    	    	num += ExArgv_Vector_findFname(pVec, pathBuf, 1);
    	    	    }
    	    	}
    	    }
    	}

    	// �g�������������J��.
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

#define EXARGV_VECTOR_CAPA0 	2048


/// ���������񃊃X�g���Ǘ����鍪�����쐬.
///
static ExArgv_Vector* ExArgv_Vector_create(unsigned size)
{
    ExArgv_Vector* pVec = (ExArgv_Vector*)ExArgv_malloc( sizeof(ExArgv_Vector) );
    size    	    	= ((size + EXARGV_VECTOR_CAPA0) / EXARGV_VECTOR_CAPA0) * EXARGV_VECTOR_CAPA0;
    pVec->capa	    	= size;
    pVec->size	    	= 0;
    pVec->buf	    	= (char**)ExArgv_malloc(sizeof(void*) * size);
    return pVec;
}


/// ���������񃊃X�g�ɁA�������ǉ�.
///
static void ExArgv_Vector_push(ExArgv_Vector* pVec, const char* pStr)
{
    assert(pVec != 0);
    assert(pStr  != 0);
    if (pStr && pVec) {
    	unsigned    capa = pVec->capa;
    	assert(pVec->buf != 0);
    	if (pVec->size >= capa) {   // �L���p�𒴂��Ă�����A���������m�ۂ��Ȃ���.
    	    char**  	buf;
    	    //x printf("!  %p: %p %d %d ::%s\n", pVec, pVec->buf, pVec->capa, pVec->size, pStr);
    	    assert(pVec->size == capa);
    	    pVec->capa	= capa + EXARGV_VECTOR_CAPA0;
    	    buf     	= (char**)ExArgv_malloc(sizeof(void*) * pVec->capa);
    	    if (pVec->buf)
    	    	memcpy(buf, pVec->buf, capa*sizeof(void*));
    	    memset(buf+capa, 0, EXARGV_VECTOR_CAPA0*sizeof(void*));
    	    ExArgv_free(pVec->buf);
    	    pVec->buf	= buf;
    	}
    	assert(pVec->size < pVec->capa);
    	pVec->buf[ pVec->size ] = ExArgv_strdup(pStr);
    	++ pVec->size;
    	//x printf("!!	%p: %p %d %d ::%s\n", pVec, pVec->buf, pVec->capa, pVec->size, pStr);
    }
}



// ===========================================================================

/// malloc
static void* ExArgv_malloc(unsigned size)
{
    void* p = malloc(size);
    if (p == NULL) {
    	fprintf(STDERR, "not enough memory.\n");
    	exit(1);
    }
    memset(p, 0, size);
    return p;
}



/// strdup
static char* ExArgv_strdup(const char* s)
{
    size_t   sz = strlen(s) + 1;
    char*    p	= (char*)malloc(sz);
    if (p == NULL) {
    	fprintf(STDERR, "not enough memory.\n");
    	exit(1);
    }
    return (char*)memcpy(p, s, sz);
}



/// free
static void ExArgv_free(void* s)
{
    if (s)
    	free(s);
}


