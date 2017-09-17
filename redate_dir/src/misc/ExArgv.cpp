/**
 *  @file   ExArgv.c
 *  @brief  main(int argc,char* argv[]) のargc,argvを,
 *  	    レスポンスファイル、ワイルドカード展開したものに拡張するための関数.
 *  @author Masashi KITAMURA
 *  @date   2006,2007
 *  @note
 *  -	dos/win系のコマンドラインプログラム用を想定.<br>
 *  	それらを、比較的楽に、ワイルドカード対応＆ディレクトリ再帰可能に
 *  	するためのルーチン.
 *  -	だけどオプション文字は '-' 専用. ('/'対応予定無)
 *  	'-'で始まる文字列は、ワイルドカードや再帰検索の対象にならない.
 *  -	unix系コマンドでの -x YYYY のような空白の入るオプション指定には不向き.<br>
 *  	その手のことをするなら getopt とかで.
 *  -	一応、狭い範囲だけどlinux gccでのコンパイル対応. (cygwin)<br>
 *  	ただlinux(unix)だとシェルがワイルドカード展開できるし
 *  	作法も違うので、出番はなさそう.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "ExArgv.h"


//#define EXARGV_NO_WC_REC  	// 定義すれば、ワイルドカード文字機能やディレクトリ再帰をしない.
//#define EXARGV_TOSLASH    	// 定義すれば、filePath中の \ を / に置換.
//#define EXARGV_TOBACKSLASH	// 定義すれば、filePath中の / を \ に置換.

#ifdef EXARGV_TINY  	    	// タイニー版は, "@c"および環境変数のみ有効. ワイルドカードやdir再帰不可.
#define EXARGV_NO_WC_REC
#endif




// ===========================================================================
// 辻褄あわせ.

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

/// flags に設定する値.
enum EXARGV_EFlags {
    EXARGV_RESFILE    = 0x01,	    	///< @レスポンスファイルを有効にする.
    EXARGV_WILDCARD   = 0x02,	    	///< ワイルドカード指定があればファイル名に展開する.
    EXARGV_RECURSIVE  = 0x04,	    	///< サブディレクトリも再帰的に検索.
    EXARGV_IGNORECASE = 0x08,	    	///< 再帰オプションの文字列比較で、大文字小文字を無視.
    EXARGV_OPTEND     = 0x10,	    	///< -- によるオプションの終了指定あり.
    EXARGV_REC_WC     = 0x20,	    	///< 再帰検索を、ワイルドカード指定時のみにする.
    EXARGV_CONFIG     = 0x40,	    	///< .exeを.cfgに置換したパス名から読込.
};


static unsigned     s_flags;	    	///< フラグ
static const char*  s_pRecOpt1;     	///< 再帰検索指定のオプション文字列1.
static const char*  s_pRecOpt2;     	///< 再帰検索指定のオプション文字列2.
static BOOL 	    s_recFlg;	    	///< 再帰検索オプションが指定されていたらon.
static BOOL 	    s_wildCFlg;     	///< ワイルドカード文字列が設定されていたらon.
static BOOL 	    s_optEndFlg;    	///< オプション終了の -- の指定があればon.



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
#define     	ExArgv_convBackSlash(pVec)  	// 指定がない場合は、何もしない.
#endif

static BOOL 	fname_ismbblead(unsigned char c);
static char *	fname_baseName(const char *adr);
static char *	fname_scanArgStr(const char *str, char arg[], int argSz);

static void*	ExArgv_malloc(unsigned size);
static char*	ExArgv_strdup(const char* s);
static void 	ExArgv_free(void* s);



// ===========================================================================

/** argc,argv をレスポンスファイルやワイルドカード展開して、argc, argvを更新して返す.
 * recOpt1,2はWC時に再帰検索する場合のオプション文字列.
 *  @param  pArgc   	argcのアドレス.(argvの数)
 *  @param  pppArgv 	argvのアドレス.
 *  @param  flags   	フラグ文字列.
 *  	    	    	@   	@response_file 有り.
 *  	    	    	*   	wildcard 文字有り.
 *  	    	    	--  	"--"によるオプション終了指定有り.
 *  	    	    	i   	再帰オプション文字列の大文字小文字を判別しない.
 *  	    	    	r   	ワイルドカード指定のみ、ディレクトリ再帰検索する.
 *  	    	    	R   	ファイル名は全てディレクトリ再帰検索する.
 *  	    	    	C   	.exeを.cfgに変えたファイルパス名のコンフィグを読み込む.
 *  @param  recOpt1 	ディレクトリ再帰指定. その１.
 *  @param  recOpt2 	ディレクトリ再帰指定. その２(ロングオプション等で複数表現する場合用)
 */
void ExArgv_get(int* pArgc, char*** pppArgv, const char* flags, const char* envName, const char* recOpt1, const char* recOpt2)
{
    int     	    f;
    int     	    argc;
    char**  	    ppArgv;
    ExArgv_Vector*  pVec;

    assert( pArgc != 0 && pppArgv != 0 );

    ExArgv_initVar(flags, recOpt1, recOpt2);	    // 作業変数初期化
    ppArgv = *pppArgv;
    argc   = *pArgc;
    assert(argc > 0 && ppArgv != 0);
  #ifdef _MSC_VER   	    	    	    	    // 古いソース用に、exeのフルパスを設定.
    ppArgv[0] = _pgmptr;
  #endif
    if (argc < 2 || ppArgv == 0)
    	return;

    f = ExArgv_checkWcResfile(argc, ppArgv);	    // 現状のargc,argvを弄る必要があるか?
    if (f == 0 && (envName==NULL||envName[0] == 0))
    	return;     	    	    	    	    // なかったらかえる.

    pVec = ExArgv_Vector_create(argc+1);    	    // argvが増減するので、作業用のリストを用意.

    //x printf("@4 %d %p(%p)\n", argc, ppArgv, *ppArgv);
    //x printf("   %p: %p %d %d\n", pVec, pVec->buf, pVec->capa, pVec->size);

    ExArgv_argvToVector(argc, ppArgv, envName,pVec);// .cfgや@レスポンスを処理しながら、 argvから作業用リストを作成.
    ExArgv_wildCard(pVec);  	    	    	    // ワイルドカードやディレクトリ再帰してパスを取得.
    ExArgv_convBackSlash(pVec);     	    	    // define設定に従って、場合によっては \ と / についての変換をかます. (基本的には何もしない).

    ExArgv_VectorToArgv( &pVec, pArgc, pppArgv );   // 作業リストを argc,argv に変換し、作業リスト自体は開放.
}




#ifdef _WINDOWS
/** winアプリで、WinMain初っ端で、argc,argvを作る場合用.
 */
void ExArgv_forWinMain(const char* pCmdLine, int* pArgc, char*** pppArgv, const char* flags, const char* envName, const char* recOpt1, const char* recOpt2)
{
    ExArgv_Vector*  pVec;
    char    	    arg[ FILEPATH_SZ + 4 ];
    const char*     s;
    int     	    n;

    assert(pArgc != 0 && pppArgv != 0);

    ExArgv_initVar(flags, recOpt1, recOpt2);	    // 作業変数初期化

    pVec = ExArgv_Vector_create(1); 	    	    // 作業用のリストを用意.

    // 実行ファイル名を得て、それを初っ端に登録.
    n = GetModuleFileName(NULL, arg, FILEPATH_SZ);
    if (n > 0) {
    	ExArgv_Vector_push(pVec, arg);
    } else {
      #ifdef _MSC_VER	    // 古いソース用に、exeのフルパスを設定.
    	ExArgv_Vector_push(pVec, _pgmptr);
      #endif
    }

    // 1行で渡されるコマンドラインを分割.
    s = pCmdLine;
    while ( (s = fname_scanArgStr(s, arg, FILEPATH_SZ)) != NULL ) {
    	const char* p = arg;
    	if (*p == '@' && (s_flags & EXARGV_RESFILE)) {
    	    ExArgv_getResFile(p+1, pVec);
    	} else if (ExArgv_checkArgOpt(p)) { 	    // 再帰検索指定,オプション終わり"--",ワイルドカードの有無をチェック.
    	    ExArgv_Vector_push( pVec, p );
    	}
    }

    ExArgv_wildCard(pVec);  	    	    	    // ワイルドカードやディレクトリ再帰してパスを取得.
    ExArgv_convBackSlash(pVec);     	    	    // define設定に従って、場合によっては \ と / についての変換をかます. (基本的には何もしない).

    ExArgv_VectorToArgv( &pVec, pArgc, pppArgv );   // 作業リストを argc,argv に変換し、作業リスト自体は開放.
}
#endif




// ===========================================================================

/// ファイルstatic変数の初期化.
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



/// オプション終了を表す "--" かどうか.
static BOOL ExArgv_isOptEnd(const char* s)
{
    return (s_flags & EXARGV_OPTEND) && (memcmp(s, "--", 3) == 0);
}



#if defined(EXARGV_NO_WC_REC) == 0

/// ワイルドカード文字が混じっているか?
static inline BOOL  fname_isWildCard(const char* s) {
  #if defined _WIN32 || defined _WIN64
    return strpbrk(s, "*?") != 0;
  #else // linux(fnmatch)
    return strpbrk(s, "*?[]\\") != 0;
  #endif
}



/// オプション文字列 a と b は等しい?
static inline BOOL ExArgv_optEqu(const char* a, const char* b) {
    return (s_flags & EXARGV_IGNORECASE) ?  (stricmp(a,b) == 0) : (strcmp(a,b) == 0);
}



/// オプション文字列 s は、再起検索指定か?
static BOOL ExArgv_strEquRecOpt(const char* s)
{
    return  ( (s_flags & EXARGV_RECURSIVE)
    	    	&& (   (s_pRecOpt1 && ExArgv_optEqu(s, s_pRecOpt1))
    	    	    || (s_pRecOpt2 && ExArgv_optEqu(s, s_pRecOpt2)) )
    	    );
}

#endif



/// 引数文字列配列に、レスポンスファイル指定、ワイルドカード指定、リカーシブ指定があるかチェック.
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
    	    rc |= 1;	// レスポンスファイル指定.
      #if defined(EXARGV_NO_WC_REC) == 0
    	} else if (fname_isWildCard(p) && (flags & EXARGV_WILDCARD)) {
    	    rc |= 2;	// ワイルドカード指定.
    	} else if (ExArgv_strEquRecOpt(p)) {
    	    rc |= 4;	// 再帰指定.
      #endif
    	} else if (ExArgv_isOptEnd(p)) {
    	    rc |= 8;	// オプションの終わり指定(-で始まるファイル名を許す指定).
    	}
    }
    return rc;
}



/// 引数文字をチェック.
static int ExArgv_checkArgOpt(const char* p)
{
  #if defined(EXARGV_NO_WC_REC) == 0
    if (ExArgv_strEquRecOpt(p)) {
    	s_recFlg = 1;	    	// 再帰検索指定があった.
    	return 0;   	    	// Vecに登録しない文字列.

    } else {
    	if (ExArgv_isOptEnd(p))
    	    s_optEndFlg = 1;	// オプションの終了があった.
      #if defined(EXARGV_NO_WC_REC) == 0
    	else if (fname_isWildCard(p) && (s_flags & EXARGV_WILDCARD))
    	    s_wildCFlg	= 1;	// ワイルドカード指定があった.
      #endif
    	return 1;   	    	// リストに登録する文字列.
    }
  #else
    if (ExArgv_isOptEnd(p))
    	s_optEndFlg = 1;    	// オプションの終了があった.
    return 1;	    	    	// リストに登録する文字列.
  #endif
}




/// (argc,argv)を pVec に変換. レスポンスファイルやワイルドカード、リカーシブ等の処理も行う.
static void ExArgv_argvToVector(int argc, char** ppArgv, const char* envName, ExArgv_Vector* pVec)
{
    int i;

    // 実行ファイル名の取得
    if (argc > 0)
    	ExArgv_Vector_push( pVec, ppArgv[0] );	// Vecに登録.

    // 環境変数の取得
    if (envName && envName[0])
    	ExArgv_getEnv(envName, pVec);

    // コンフィグファイルの読込.
    if (s_flags & EXARGV_CONFIG)
    	ExArgv_getCfgFile( ppArgv[0], pVec );

    //x printf("%p %x %#x %p\n",pVec, pVec->capa, pVec->size, pVec->buf);

    // 引数の処理.
    for (i = 1; i < argc; ++i) {
    	const char* p = ppArgv[i];
    	if (i > 0 && *p == '@' && (s_flags & EXARGV_RESFILE)) {
    	    ExArgv_getResFile(p+1, pVec);   // レスポンスファイル読込.
    	} else if (ExArgv_checkArgOpt(p)) { // 再帰検索指定,オプション終わり"--",ワイルドカードの有無をチェック.
    	    ExArgv_Vector_push( pVec, p );  // Vecに登録.
    	}
    }
}



/// 環境変数があれば、登録.
static void ExArgv_getEnv(const char* envName, ExArgv_Vector* pVec)
{
    const char* env = getenv(envName);
    if (env && env[0]) {
    	char	    	arg[ FILEPATH_SZ + 4 ];
    	while ( (env = fname_scanArgStr(env, arg, FILEPATH_SZ)) != NULL ) {
    	    const char* p = arg;
    	    if (ExArgv_checkArgOpt(p))	    	// 再帰検索指定,オプション終わり"--",ワイルドカードの有無をチェック.
    	    	ExArgv_Vector_push( pVec, p );
    	}
    }
}



/// コンフィグファイルの読み込み
static void ExArgv_getCfgFile(const char* exeName, ExArgv_Vector* pVec)
{
    FILE*   fp;
    char    name[ FILEPATH_SZ+4 ];
    char*   p;

    // 実行ファイル名からコンフィグ名を生成.
    strncpy(name, exeName, FILEPATH_SZ-4);
    name[FILEPATH_SZ] = '\0';
    p = strrchr(name, '.');
    if (p)
    	strcpy(p, ".cfg");
    else
    	strcat(name, ".cfg");

    // ファイルの存在チェック.
    fp = fopen(name, "rb");
    if (fp) {	// ファイルがあったら読み込む.
    	fclose(fp);
    	ExArgv_getResFile(name, pVec);
    }
}



/// レスポンスファイルを(argc,argv)を pVec に変換.
/// レスポンスファイルやワイルドカード、リカーシブ等の処理も行う.
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
    	    if (c == ';' || c == '#' || c == '\0') {	// 空行やコメントの時.
    	    	break;
    	    } else if (ExArgv_checkArgOpt(arg)) {   // 再帰検索指定,オプション終わり"--",ワイルドカードの有無をチェック.
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



/// ワイルドカード、再帰処理.
///
static void ExArgv_wildCard(ExArgv_Vector* pVec)
{
  #if defined(EXARGV_NO_WC_REC) == 0
    BOOL    	    fnameOnly = 0;
  #endif
    char**  	    pp;
    char**  	    ee;
    ExArgv_Vector*  wk;

    // 再帰指定もワイルドカードもオプション終了も指定されていないなら、
    // わざわざリスト再構築する必要なしなので、かえる.
    if (s_recFlg == 0 && s_wildCFlg == 0 && s_optEndFlg == 0)
    	return;

    // 再構築.
    wk = ExArgv_Vector_create( pVec->size+1 );
    ee = pVec->buf+pVec->size;
    for (pp = pVec->buf; pp != ee; ++pp) {
    	const char* s = *pp;
      #if defined(EXARGV_NO_WC_REC) == 0
    	if (   (*s != '-' || fnameOnly)     	    	    // オプション以外の文字列で、
    	    && ( ((s_flags & EXARGV_REC_WC)==0 && s_recFlg) // 全ファイル再帰チェックする場合か
    	    	 || fname_isWildCard( s )    )	    	    // 　ワイルドカード指定のあり、
    	    && (pp != pVec->buf)    	    	    	    // 初っ端以外([0]は実行ファイル名なので検索させない)のとき
    	 ){
    	    ExArgv_Vector_findFname(wk, s, s_recFlg);

    	} else	{
    	    if (ExArgv_isOptEnd(s)) 	    	    	    // -- 指定以降はオプション無く、ファイル名だけとして扱う.
    	    	fnameOnly = 1;
    	    ExArgv_Vector_push( wk, s );
    	}
      #else
    	ExArgv_Vector_push( wk, s );
      #endif
    }

    // 元のリストを開放.
    for (pp = pVec->buf; pp != ee; ++pp) {
    	char* p = *pp;
    	if (p)
    	    ExArgv_free(p);
    }
    ExArgv_free(pVec->buf);

    // 今回生成したものを、pVecに設定.
    pVec->buf  = wk->buf;
    pVec->size = wk->size;
    pVec->capa = wk->capa;

    // 作業に使ったメモリを開放
    ExArgv_free(wk);
}



#if (defined EXARGV_TOSLASH) || (defined EXARGV_TOBACKSLASH)
/// ファイル(パス)名中の \ / の変換. -で始まるオプション文字列は対象外.
/// 最近のwin環境ではどちらの指定でもokなので、無理に変換する必要なし.
/// (オプション中にファイル名があると結局自前で変換せざるえないので、ここでやらないほうが無難かも)
///
static void ExArgv_convBackSlash(ExArgv_Vector* pVec)
{
    BOOL    	fnameOnly = 0;
    char**  	pp;
    char**  	ee = pVec->buf + pVec->size;

    for (pp = pVec->buf; pp != ee; ++pp) {
    	char* s = *pp;
    	if (*s != '-' || fnameOnly) {	    // オプション以外の文字列で、
    	  #if (defined EXARGV_TOSLASH)
    	    fname_backslashToSlash(s);	    // \ を / に置換.
    	  #else
    	    fname_slashToBackslash(s);	    // / を \ に置換.
    	  #endif
    	} else if (ExArgv_isOptEnd(s)) {    // -- 指定以降はオプション無く、ファイル名だけとして扱う.
    	    fnameOnly = 1;
    	} else {    	    	    	    // オプションなら、下手に変換しないでおく.
    	    ;
    	}
    }
}
#endif



/// pVecから、(argc,argv)を生成. ppVecは開放する.
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

    // char*配列のためのメモリを取得.
    *pArgc   = ac;
    av	     = (char**) ExArgv_malloc(sizeof(char*) * (ac + 2));
    *pppArgv = av;

    memcpy(av, pVec->buf, sizeof(char*) * ac);
    av[ac]   = NULL;
    av[ac+1] = NULL;

    // 作業に使ったメモリを開放.
    ExArgv_free(pVec->buf);
    ExArgv_free(pVec);
    *ppVec   = NULL;
}




// ===========================================================================

//x #define DIRSEP_STR	    "\\"
#define DIRSEP_STR  	    "/"


/// 文字 C が MS全角の１バイト目か否か. (utf8やeucは \ 問題は無いので 0が帰ればok)
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
/// filePath中の \ を / に置換.
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
/// filePath中の / を \ に置換.
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



/// コマンドラインで指定されたファイル名として、""を考慮して,
/// 空白で区切られた文字列(ファイル名)を取得.
/// @return スキャン更新後のアドレスを返す。strがEOSだったらNULLを返す.
static char *fname_scanArgStr(const char *str, char arg[], int argSz)
{
    const unsigned char* s = (const unsigned char*)str;
    char*   	    	 d = arg;
    char*   	    	 e = d + argSz;
    unsigned	    	 f = 0;
    int     	    	 c;

    assert( str != 0 && arg != 0 && argSz > 1 );

    // 空白をスキップ.
    while ( isspace(*s) )
    	++s;

    if (*s == '\0') // EOSだったら、見つからなかったとしてNULLを返す.
    	return NULL;

    do {
    	c = *s++;
    	if (c == '"') {
    	    f ^= 1; 	    	    	// "の対の間は空白をファイル名に許す.ためのフラグ.

    	    // ちょっと気持ち悪いが、Win(XP)のcmd.exeの挙動に合わせてみる.
    	    // (ほんとにあってるか、十分には調べてない)
    	    if (*s == '"' && f == 0)	// 閉じ"の直後にさらに"があれば、それはそのまま表示する.
    	    	++s;
    	    else
    	    	continue;   	    	// 通常は " は省いてしまう.
    	}
    	if (d < e) {
    	    *d++ = (char)c;
    	}
    } while (c >= 0x20 && (c != ' ' || f != 0));
    *--d  = '\0';
    --s;
    return (char *)s;
}



/// ファイルパス名中のディレクトリを除いたファイル名の位置を返す.
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

/** srchNameで指定されたパス名(ワイルドカード文字対応) にマッチするパス名を全て pVec に入れて返す.
 *  recFlag が真なら再帰検索を行う.
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
    	// ファイル名を取得.
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

    // ファイル名を取得.
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

    if (baseName == pathBuf) {	// ディレクトリ部が無い場合.
    	strcpy(pathBuf, "./");	// カレント指定を一時的に設定.
    	baseName = pathBuf+2;
    	flag	 = 1;
    }
    *baseName	= 0;
    baseNameSz	= FILEPATH_SZ - strlen(pathBuf);
    assert(baseNameSz >= MAX_PATH);

    // ディレクトリエントリの取得.
    baseName[-1] = 0;
    dirNum = scandir(pathBuf, &namelist, 0, alphasort);
    baseName[-1] = '/';

    if (flag) { // 一時的なカレント指定だったならば、捨てる.
    	baseName  = pathBuf;
    	*baseName = '\0';
    }

    if (namelist) {
    	struct stat statBuf;
    	int 	    i;

    	// ファイル名を取得.
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

    	// ディレクトリがあれば再帰.
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

    	// 使ったメモリを開放.
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


/// 引数文字列リストを管理する根元を作成.
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


/// 引数文字列リストに、文字列を追加.
///
static void ExArgv_Vector_push(ExArgv_Vector* pVec, const char* pStr)
{
    assert(pVec != 0);
    assert(pStr  != 0);
    if (pStr && pVec) {
    	unsigned    capa = pVec->capa;
    	assert(pVec->buf != 0);
    	if (pVec->size >= capa) {   // キャパを超えていたら、メモリを確保しなおす.
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


