/**
 *  @file   dbg.h
 *  @brief  c/c++ エラー＆デバッグ処理
 *  @author 北村
 *  @note
 *  - 極力、C/C++両方で使えるように設定.
 *  - ターゲット環境を表す TARGET マクロが設定されていない場合、
 *    または TARGET=0 ならば、コマンドライン用でprintfデバッグと解釈。
 *  - dbg_,DBG_ 系は製品に残さないデバッグ・ルーチン向け。
 *  - err_,ERR_ 系は製品に残るデバッグ・ルーチン向け。
 *  - DBG_ や ERR_ 大文字のみはマクロで組まれていて、RELEASEコンパイルでは綺麗
 *    に消える
 *    逆に小文字の関数は、機能はなくなるが関数呼び出し自体は残るので注意。
 *    基本的に、マクロのほうを使うこと！
 *  - 開発時の画面出力関係は Deve クラスを用いる。dbg/errでは行わない！
 *  - 文をマクロ化する定石テクニックでは do {～; }while(0) を使うべきだが、
 *    while(0)がコンパイル警告されるので、またデバッグ時のみのルーチン故
 *    for(;;){～;break;}を用いている。(ループジャンプと脱出ジャンプが最適化しても消えない可能性がある)
 */


#ifndef DBG_H
#define DBG_H

// ===========================================================================

// TARGET が設定されていなかったらとりあえず、制限付きでコマンドライン扱いとして扱う

//#if defined USE_DEVEMGR
#ifdef USE_DEVEMGR
#define ERR_EXIT_TARGET_PUTS(msg)   	g_deveMgr->urgentErrPuts(msg)
#else
#define ERR_EXIT_TARGET_PUTS(msg)
#endif

//#ifdef __cplusplus
//#include <cassert>
//#else
#include <assert.h>
//#endif

// CodeWarriarでコンパイルするときのつじつま会わせ
#if defined(__cdecl) == 0
#define __cdecl
#endif



// ===========================================================================
// デバッグ向けマクロ

// 2つの引数を連結して１つのラベルを生成
#define DBG_STR_CAT(a,b)    DBG_STR_CAT_2(a,b)
#define DBG_STR_CAT_2(a,b)  a##b

/// コンパイル時assert.
#if defined _MSC_VER
#define DBG_STATIC_ASSERT(cc)	struct DBG_STR_CAT(STATIC_ASSERT_CHECK_ST,__LINE__) { char chk[(cc)?1:-1];}; enum { DBG_STR_CAT(STATIC_ASSERT_CHECK,__LINE__) = sizeof( DBG_STR_CAT(STATIC_ASSERT_CHECK_ST,__LINE__) ) }
#else
#define DBG_STATIC_ASSERT(cc)	typedef char DBG_STR_CAT(STATIC_ASSERT_CHECK_ST,__LINE__)[(cc)? 1/*OK*/ : -1/*ERROR*/]
#endif


// 数値を"数値"文字列化
#define DBG_I2STR(a)	    DBG_I2STR_2(a)
#define DBG_I2STR_2(a)	    #a
#define DBG_I2I(a)  	    a

// __FUNCTION__があればそれを使う(結局C99になってなかったのねん...)
#if defined(__FUNCTION__) == 0
    #if (defined __STDC_VERSION__ == 0 || __STDC_VERSION__ < 199901L)
    	#define __FUNCTION__	""  	    	// __FILE__ "(" DBG_I2STR(__LINE__) ")"
    #else
    	#define __FUNCTION__	__func__
    #endif
#endif




// ===========================================================================
// デバッグ・ログ出力関係
// ===========================================================================

#if defined TARGET
// -------
// 初期化,終了

/// デバッグ処理を行うための初期化
int  dbg_init(void);

/// デバッグ処理の終了
void dbg_term(void);


// -------
// エラー出力

/// printf形式でメッセージを出して、終了する関数。実使用ではERR_EXIT()を用いること。
int __cdecl err_exit(const char *fmt, ...);

/// printf形式でエラー出力。
int __cdecl err_printf(const char *fmt, ...);


// -------
// エラー出力

/// err_exitで表示するファイル名と行を設定. s=NULLなら表示なし
#define err_setFnameLine(s, l)	(g_err_fname__ = (s), g_err_line__ = (l))

/// 関数/モジュール名の設定
//int err_setFuncName(const char *name);
#define err_setFuncName(f)  (g_err_funcName__=(f))  ///< エラー出力で表示する関数名を設定
#define err_fname() 	    g_err_fname__   	    ///< 現在のエラーファイル名を取得
#define err_line()  	    g_err_line__    	    ///< 現在のエラー行番号を取得
#define err_funcName()	    g_err_funcName__	    ///< 現在のエラー関数名を取得

extern const char   	    *g_err_fname__; 	    ///< err/dbgモジュール内変数. 直接使っちゃ駄目よ。
extern int  	    	    g_err_line__;   	    ///< werr/dbgモジュール内変数. 直接使っちゃ駄目よ。
extern const char   	    *g_err_funcName__;	    ///< err/dbgモジュール内変数. 直接使っちゃ駄目よ。


// -------
// デバッグ

extern int  g_dbg_log_sw__; 	///< [直接使用しちゃダメ] dbg_printf等でのログ出力の有無を保持. 0:off 1:on
#define dbg_setLogSw(sw)    (g_dbg_log_sw__ = (sw)) ///< ログ出力を行うか否かを設定
#define dbg_getLogSw()	    (g_dbg_log_sw__)	    ///< 現在のログ出力の有無を取得
#define dbg_revLogSw()	    (g_dbg_log_sw__ ^= 1)   ///< ログ出力の有無を、反転する




/// printf形式で デバッグログ出力. 生成文字列は1024バイトまでのこと
int __cdecl dbg_printf(const char *fmt, ...);

/// 1行デバッグログ出力
void dbg_puts(const char *s);

/// １行デバッグログ出力の初期化. dbgモジュール内専用
void dbg_puts_init(void);

/// アドレス addr から sz バイトをダンプしてログ出力
void dbg_dump(const void *addr, int sz);

/// 1行デバッグログ出力(ファイル名,行付き)
void dbg_flMsg(const char *srcname, int line, const char *msg);


#endif



// ===========================================================================
// デバッグ・ログ出力マクロ
// ===========================================================================
//-----------------------------------------------------
// マクロ

/** @def DBG_ASSERT(x)
 *  	式 x の値が偽ならば、エラー終了. 標準ライブラリのassert()の代わり
 */
/** @def DBG_M()
 *  	cソース名+行番号のみのログ出力. 通過チェックを想定.
 */
/** @def DBG_S(s)
 *  	1行ログ出力.
 */
/** @def DBG_F(s)
 *  	printf形式でログ出力。ただし DBG_F(("%sがない\n",name)); のように(())で用いる。
 */
/** @def DBG_PRINTF(s)
 *  	printf形式でログ出力。ただし DBG_PRINTF(("%sがない\n",name)); のように(())で用いる。
 */
/** @def DBG_DUMP(a,n)
 *  	アドレス aから nバイトをダンプ出力
 */
/** @def ERR_F(s)
 *  	エラーメッセージ出力。引数は DBG_F(s)に同じ
 */
/** @def ERR_PRINTF(s)
 *  	ERR_F(s) に同じ
 */
/** @def ERR_EXIT(s)
 *  	エラーメッセージ (s)を出力して終了. 引数は DBG_F(s)に同じ
 */
/** @def ERR_ROUTE()
 *  	通過しちゃいけないルートに置くマクロ. assert(0)の特化版
 */


#if defined(_RELEASE)	/*NDEBUG*/  // リリース時. デバッグログ出力なしにする。
 #define DBG_M()
 #define DBG_PRINTF(s)
 #define DBG_DUMP(a,n)
 #define DBG_ABORTMSG(s)

 #if defined(TARGET) && TARGET > 0
   // ターゲット機なら、何もしない. ※つまりエラーでも継続させる...運任せだが、ハングさせるよりは、と。
  #define ERR_PRINTF(s)     (0)
  #define ERR_ROUTE()
  #define ERR_ABORT()
  #if defined(USE_DEVEMGR) == 0
   #define ERR_ABORTMSG(s)  (0)
   #define ERR_EXIT(s)	    (0)
  #else
   #define ERR_ABORTMSG(s)  (err_setFnameLine(__FILE__, __LINE__), err_exit s)
   #define ERR_EXIT(s)	    ERR_ABORTMSG(s)
  #endif
 #else
  // コマンドラインのときは、エラー終了しとく
  #define ERR_PRINTF(s)     (printf s)
  #define ERR_ABORTMSG(s)   ((printf s), ((int(*)(int))exit)(1))
  #define ERR_EXIT(s)	    ERR_ABORTMSG(s)
  #define ERR_ROUTE()	    ERR_ABORTMSG(("%s(%d): bad route! in %s\n", __FILE__, __LINE__, __FUNCTION__))
  #define ERR_ABORT()	    (exit(1))
 #endif

#else	// デバッグ時はログ出力
  #define DBG_PRINTF(s)     (dbg_printf s)
  #define DBG_DUMP(a,sz)    dbg_dump(a,sz)
  #define DBG_ABORTMSG(s)   ERR_ABORTMSG(s)

  #define ERR_PRINTF(s)     (err_printf s)
  #define ERR_ABORTMSG(s)   (err_setFnameLine(__FILE__, __LINE__), err_exit s)
  #define ERR_EXIT(s)	    ERR_ABORTMSG(s)
  #define ERR_ROUTE()	    ERR_ABORTMSG(("%s(%d):bad route! in %s\n", __FILE__, __LINE__, __FUNCTION__))

  #ifdef TARGET
   #if TARGET == TT_WIN
    #define ERR_ABORT()     _CrtDbgBreak()
   #elif TARGET == TT_NDS
    //#define ERR_ABORT()   	OS_Panic("%s (%d):abort\n", __FILE__, __LINE__)
    #define ERR_ABORT()     asm __volatile__(" bkpt 0 ")
   #elif TARGET == TT_PS2
    #define ERR_ABORT()     asm __volatile__(" breakc 1\n")
   #elif TARGET == TT_PSP
    #define ERR_ABORT()     asm __volatile__("breakc 0x0")
   #elif TARGET == TT_GC
    #define ERR_ABORT()     OSPanic(__FILE__, __LINE__, "abort\n")
   #else
    #define ERR_ABORT()     ((*(char*)0) = 0)
   #endif
  #else
    #define ERR_ABORT()     exit(1)
  #endif

#endif





// ===========================================================================
// invariantチェック向け
// ===========================================================================

/** @def DBG_LIM_I(a, mi, ma)
 *  整数 a が [mi, ma] の範囲内ならok
 */
/** @def DBG_LIM_U(a, ma)
 *  整数 a が [0, ma] の範囲内ならok.
 *  (gcc だとunsigned系変数に対して>=0を行うと警告になるため、用意)
 */
/** @def DBG_LIM_BOOL(a)
 *  整数 a が [0, 1] ならok
 */
/** @def DBG_LIM_F(a, mi, ma)
 *  float32型変数 a が [mi, ma] の範囲内ならok
 */
/** @def DBG_LIM_D(a, mi, ma)
 *  float64型変数 a が [mi, ma] の範囲内ならok
 */
/** @def DBG_LIM_CSTR(a, len)
 *  c文字列 a が lenバイト以下ならok.
 */
/** @def DBG_LIM_CSTR0(a, len)
 *  c文字列 a が NULL か lenバイト以下ならok.
 */
/** @def DBG_LIM_VEC4(v, vmi, vma)
 * float[4] の各要素が [vmi,vma] の範囲にあれば ok
 */
/** @def DBG_CHK_PTR(p, asz)
 *  p が正常なpointerの範囲で、アライメントが aszバイトであれば ok
 */
/** @def DBG_CHK_PTR0(p, asz)
 *  p がNULLか、正常なpointerの範囲で、アライメントが aszバイトであれば ok
 */
/** @def DBG_ASSERT_MESSAGE(msg, cond)
 *   cond の値が真ならok. 偽のときメッセージを表示して失敗
 */
/** @def DBG_FAIL( msg )
 *   必ず失敗してメッセージを出す
 */
/** @def DBG_ASSERT_EQUAL( expected, actual )
 *  期待値expected と 結果actual が等しければok.
 */
/** @def DBG_ASSERT_EQUAL_MESSAGE( message, expected, actual )
 *  期待値expected と 結果actual が等しければok. 等しくないときメッセージを表示して失敗
 */
/** @def DBG_ASSERT_INTS_EQUAL( expected, actual )
 *  期待値expected と 結果actual が等しければok
 */
/** @def DBG_ASSERT_INT64S_EQUAL( expected, actual )
 *  期待値expected と 結果actual の差が delta 以内ならok. より大きいとき失敗
 */
/** @def DBG_ASSERT_FLOATS_EQUAL( expected, actual, delta )
 *  期待値expected と 結果actual の差が delta 以内ならok. より大きいとき失敗
 */
/** @def DBG_ASSERT_DOUBLES_EQUAL( expected, actual, delta )
 *  期待値expected と 結果actual の差が delta 以内ならok. より大きいとき失敗
 */
/** @def DBG_ASSERT_MEMS_EQUAL( s1, s2, sz )
 *  メモリ s1 と s2 の sz バイトが等しければ ok.
 */
/** @def DBG_ASSERT_CSTRS_EQUAL(s1, s2 )
 *  C文字列 s1 と s1 が一致すれば成功、でなければ失敗
 */
/** @def DBG_ASSERT_THROW(x)
 *  x が例外を投げれば真.
 */

#if defined( NDEBUG )	// defined(NDEBUG) // リリース時 - - - - - -

#define DBG_ASSERT(x)

#define DBG_ASSERTMSG(x, s)
#define DBG_PTR_ASSERT(x)
#define DBG_PTR0_ASSERT(x)

#define DBG_TSTMSG(s)

#define DBG_NOCHK(a)
#define DBG_LIM_I(a, mi, ma)
#define DBG_LIM_U(a, ma)
#define DBG_LIM_BOOL(a)
#define DBG_LIM_F(a, mi, ma)
#define DBG_LIM_D(a, mi, ma)
#define DBG_LIM_CSTR(s, sz)
#define DBG_LIM_CSTR0(s, sz)
#define DBG_LIM_FVEC4(v, vmi, vma)
#define DBG_LIM_FMTX(v, vmi, vma)

#define DBG_CHK_PTR(p, asz)
#define DBG_CHK_PTR0(p, asz)

#define DBG_ASSERT_THROW(x)

// CppUnit にあるチェックの類似品
#define DBG_ASSERT_MESSAGE(msg, cond)
#define DBG_FAIL( msg )
#define DBG_ASSERT_EQUAL( expected, actual )
#define DBG_ASSERT_EQUAL_MESSAGE( message, expected, actual )
#define DBG_ASSERT_INTS_EQUAL( expected, actual )
#define DBG_ASSERT_INT64S_EQUAL( expected, actual )
#define DBG_ASSERT_FLOATS_EQUAL( expected, actual, delta )
#define DBG_ASSERT_DOUBLES_EQUAL( expected, actual, delta )

#define DBG_ASSERT_MEMS_EQUAL( s1, s2, sz )
#define DBG_ASSERT_CSTRS_EQUAL(s1, s2 )



#define DBG_COUNT_CLASS(NMAX)	    typedef int   DBG_STR_CAT(dbg_count_class_,DBG_I2I(NMAX))
#define DBG_PUT_CLASS_SIZE(n)

#define DBG_CHK_NEST_CALL(n)
#define DBG_IS_NEST_CALL(n) 	    (0)



#else	// デバッグ中 - - - - - - - - - - - - - - -

// UnitTestでのメッセージ表示
#define DBG_ASSERTMSG(x, s) 	do { if (!(x)) ERR_ABORTMSG(("%s(%d):%s> assert(%s) is false.\n\t%s\n", __FILE__, __LINE__, __FUNCTION__, #x, s));} while(0)
#define DBG_ASSERT(x)	    	do { if (!(x)) ERR_ABORTMSG(("%s(%d):%s> assert(%s) is false.\n", __FILE__, __LINE__, __FUNCTION__, #x));} while(0)
//#define DBG_ASSERT(x)     	((x) || ERR_ABORTMSG(("%s(%d):%s> assert(%s) is false.\n", __FILE__, __LINE__, __FUNCTION__, #x)) )
#define DBG_TSTMSG(s)	    	ERR_ABORTMSG(s)

// assert.h の定義を無効化. 自前のに置き換え.
#undef	assert
#define assert(x)   	    	DBG_ASSERT(x)

#define DBG_NOCHK(a)
#define DBG_LIM_I(a, mi, ma)	do {if ((a) < (mi) || (ma) < (a)) DBG_TSTMSG(("%s (%d): %s: %d <= %s <= %d を満たさない(%d)\n", __FILE__, __LINE__, __FUNCTION__, (mi), #a, (ma),(a) ));} while(0)
#define DBG_LIM_U(a, ma)    	do { if (! (unsigned(a) <= (ma))    	) DBG_TSTMSG(("%s (%d): %s: %d <= %s <= %d を満たさない(%d)\n", __FILE__, __LINE__, __FUNCTION__, 0, #a, (ma),(a) ));} while(0)
#define DBG_LIM_BOOL(a)     	do {if (int(a)!= 0 && int(a)!=1) DBG_TSTMSG(("%s (%d): %s: 0 <= %s <= 1 を満たさない(%d)\n", __FILE__, __LINE__, __FUNCTION__, #a, (a) ));} while(0)
#define DBG_LIM_F(a, mi, ma)	do {const float *aa=&(a); if ((a) < (mi) || (ma) < (a)) DBG_TSTMSG(("%s (%d): %s: %f <= %s <= %f を満たさない(%f)\n", __FILE__, __LINE__, __FUNCTION__, (mi), #a, (ma),(a) ));} while(0)
//#define DBG_LIM_F(a, mi, ma)	do {const float *aa=&(a); if ((a) < (mi) || (ma) < (a)) DBG_TSTMSG(("%s (%d): %s: %f <= %s <= %f を満たさない(%f)\n", __FILE__, __LINE__, __FUNCTION__, (double)(mi), #a, (double)(ma),(double)(a) ));} while(0)
//#define DBG_LIM_D(a, mi, ma)	do {const double *aa=&(a); if ((a) < (mi) || (ma) < (a)) DBG_TSTMSG(("%s (%d): %s: %f <= %s <= %f を満たさない(%f)\n", __FILE__, __LINE__, __FUNCTION__, (double)(mi), #a, (double)(ma),(a) ));} while(0)
#define DBG_LIM_CSTR(a, sz) 	do {int l__ = strlen(a); if (l__ <= 0 || (sz) <= l__) DBG_TSTMSG(("%s (%d): %s: %s{`%s'}の長さ%dは1～%dの範囲外\n", __FILE__, __LINE__, __FUNCTION__, #a, (a), l__, (sz)-1 ));} while(0)
#define DBG_LIM_CSTR0(a, sz)	do {int l__ = strlen(a); if (l__ < 0 || (sz) <= l__) DBG_TSTMSG(("%s (%d): %s: %s{`%s'}の長さ%dは0～%dの範囲外\n", __FILE__, __LINE__, __FUNCTION__, #a, (a), l__, (sz)-1 ));} while(0)


#define DBG_LIM_FVEC4(v, vmi, vma) for(;;) {	\
    DBG_LIM_F((v)[0], vmi, vma);    	\
    DBG_LIM_F((v)[1], vmi, vma);    	\
    DBG_LIM_F((v)[2], vmi, vma);    	\
    DBG_LIM_F((v)[3], vmi, vma);    	\
    break;  	    	    	    	\
}

#define DBG_LIM_FMTX44(v, vmi, vma) for(;;) {	\
    const float *m__ = (const float*)(v);   \
    unsigned i__;   	    	    	    \
    for (i__ = 0; i__ < 4*4; ++i__) 	    \
    	DBG_LIM_F(m__[i__], vmi, vma);	    \
    break;  	    	    	    	    \
}

// ポインタとしておかしな値なら真( 環境依存なので、ここでは大雑把に定義 )
#ifndef DBG_IS_BADPTR
 #if defined _WIN64
  #define DBG_IS_BADPTR(p)  	(/*(p) != 0 &&*/ (size_t)(ptrdiff_t)(p) < 0x10000)
 #elif defined _WIN32
  //#define DBG_IS_BADPTR(p)	    ((p) != 0 && ((size_t)(ptrdiff_t)(p) < 0x10000)||((size_t)(ptrdiff_t)(p) >= 0x80000000))
  #define DBG_IS_BADPTR(p)  	(/*(p) != 0 &&*/ (size_t)(ptrdiff_t)(p) < 0x10000)
 #elif TARGET == TT_PSP
  #define DBG_IS_BADPTR(p)  	(/*(p) != 0 &&*/ (((size_t)(ptrdiff_t)(p) >= 0x80000000)))
 #elif TARGET == TT_PS2
  #define DBG_IS_BADPTR(p)  	((size_t)(ptrdiff_t)(p) >= 0x80000000)
 #elif TARGET == TT_NDS
  #define DBG_IS_BADPTR(p)  	(/*(p) != 0 &&*/ (((size_t)(ptrdiff_t)(p) < 0x01000000) || ((size_t)(ptrdiff_t)(p) >= 0x0A010000)))
 #else
  #define DBG_IS_BADPTR(p)  	(0)
 #endif
#endif

extern int dbg_chk_ptr(const void* p, int algn, const char* p_str, const char* fname, unsigned line, const char* func);
#define DBG_CHK_PTR(p, asz) 	dbg_chk_ptr((p),(asz),#p, __FILE__, __LINE__, __FUNCTION__)
#define DBG_CHK_PTR0(p, asz)	((p != 0) ? dbg_chk_ptr((p),(asz),#p, __FILE__, __LINE__, __FUNCTION__) : 0)


/// pが正常な範囲のアドレスならok(NULL不可)
#define DBG_PTR_ASSERT(p)   	DBG_ASSERT(!DBG_IS_BADPTR(p))	    	// DBG_CHK_PTR(p, 1)
/// pが正常な範囲のアドレスかNULLならok.
#define DBG_PTR0_ASSERT(p)  	DBG_ASSERT(p == 0 || !DBG_IS_BADPTR(p)) // DBG_CHK_PTR0(p, 1)

/// num個の配列ary の各 invariant() を実行.
#define DBG_ARY_INVARIANT( ary, num )	for(;;) {   	    	    	    \
    unsigned int num__ = (unsigned int)(num);	    	    	    	    \
    for (unsigned int i__ = 0; i__ < num__; ++i__) {	    	    	    \
    	(ary)[i__].invariant();     	    	    	    	    	    \
    }	    	    	    	    	    	    	    	    	    \
    break;  	    	    	    	    	    	    	    	    \
}

/// x が例外を投げれば真.
#define DBG_ASSERT_THROW(x) 	for(;;) { bool f = 0; try { x; } catch (...) { f = 1; } DBG_ASSERT_MESSAGE( #x " が例外を投げない\n", f); break;}



// -----------------------------------------
// CppUnit にあるチェックの類似品

/// cond の値が偽のときメッセージを表示して失敗
#define DBG_ASSERT_MESSAGE(msg, cond)	    	((cond) || (DBG_ABORTMSG(("%s(%d): %s\n", __FILE__, __LINE__, msg)), 0))

/// 必ず失敗
#define DBG_FAIL( msg )     	    	    	(DBG_ABORTMSG(("%s(%d): %s\n", __FILE__, __LINE__, msg)), 0)

/// 期待値expected と 結果actual が等しくないときメッセージを表示して失敗
#define DBG_ASSERT_EQUAL_MESSAGE( message, expected, actual )  for(;;) {    \
    	if ((expected) != (actual)) 	    	    	    	    	    \
    	    DBG_ABORTMSG(("%s(%d): %s\n", __FILE__, __LINE__, msg));	    \
    	break;	    	    	    	    	    	    	    	    \
    }


/// 期待値expected と 結果actual が等しくないとき失敗
#define DBG_ASSERT_EQUAL( expected, actual )  for(;;) {     	    	    \
    	if ((expected) != (actual)) 	    	    	    	    	    \
    	    DBG_ABORTMSG(("%s(%d): equality assertion failed. %s != %s\n"   \
    	    	, __FILE__, __LINE__, #expected, #actual)); 	    	    \
    	break;	    	    	    	    	    	    	    	    \
    }


/// 期待値expected と 結果actual が等しくないとき失敗
#define DBG_ASSERT_INTS_EQUAL( expected, actual )  for(;;) {	    	    \
    	if ((expected) != (actual)) {	    	    	    	    	    \
    	    DBG_ABORTMSG(("%s(%d): equality assertion failed. %s{%d} != %s{%d}\n" \
    	    	, __FILE__, __LINE__, #expected,expected, #actual,actual)); \
    	}   	    	    	    	    	    	    	    	    \
    	break;	    	    	    	    	    	    	    	    \
    }


/// 期待値expected と 結果actual が等しくないとき失敗
#define DBG_ASSERT_INT64S_EQUAL( expected, actual )  for(;;) {	    	    \
    	if ((expected) != (actual)) {	    	    	    	    	    \
    	    DBG_ABORTMSG(("%s(%d): equality assertion failed. %s{%#x%08x} != %s{%#x%08x}\n" 	    \
    	    	, __FILE__, __LINE__	    	    	    	    	    	    	    	    \
    	    	, #expected, (unsigned int)((uint64_t)(expected)>>32), (unsigned int)(expected)     \
    	    	, #actual,   (unsigned int)((uint64_t)(actual)	>>32), (unsigned int)(actual)	)); \
    	}   	    	    	    	    	    	    	    	    \
    	break;	    	    	    	    	    	    	    	    \
    }

/// 期待値expected と 結果actual の差が delta より大きいとき失敗
#define DBG_ASSERT_FLOATS_EQUAL( expected, actual, delta )  for(;;) {	    \
    	double eXPe__ = (double)(float)(expected);  	    	    	    \
    	double aCTu__ = (double)(float)(actual);    	    	    	    \
    	double dElt__ = (double)(delta);    	    	    	    	    \
    	if ((eXPe__ < aCTu__ -dElt__) || (aCTu__+dElt__ < eXPe__)) {	    \
    	    DBG_ABORTMSG(("%s(%d): equality assertion failed. abs(%s{%e} - %s{%e}) > %s(%e)\n"	 \
    	    	, __FILE__, __LINE__	    	    	    	    	    \
    	    	, #expected, eXPe__, #actual, aCTu__, #delta, dElt__));     \
    	}   	    	    	    	    	    	    	    	    \
    	break;	    	    	    	    	    	    	    	    \
    }

/// 期待値expected と 結果actual の差が delta より大きいとき失敗
#define DBG_ASSERT_DOUBLES_EQUAL( expected, actual, delta )  for(;;) {	    \
    	double eXPe__ = (double)(expected); 	    	    	    	    \
    	double aCTu__ = (double)(actual);   	    	    	    	    \
    	double dElt__ = (double)(delta);    	    	    	    	    \
    	double dIFf__ = eXPe__ - aCTu__;    	    	    	    	    \
    	if ((dIFf__ < -dElt__) || (dElt__ < dIFf__)) {	    	    	    \
    	    DBG_ABORTMSG(("%s(%d): equality assertion failed. abs(%s{%e} - %s{%e}) > %s{%e}\n"	 \
    	    	, __FILE__, __LINE__	    	    	    	    	    \
    	    	, #expected, (double)eXPe__, #actual, (double)aCTu__, #delta, (double)dElt__));  \
    	}   	    	    	    	    	    	    	    	    \
    	break;	    	    	    	    	    	    	    	    \
    }



/// メモリ s1 と s1 の sz バイトが一致すれば成功、でなければ失敗
#define DBG_ASSERT_MEMS_EQUAL(s1, s2, sz )  for(;;) {	    	    	    \
    	const unsigned char *cucp1__ = (const unsigned char *)(s1); 	    \
    	const unsigned char *cucp2__ = (const unsigned char *)(s2); 	    \
    	int size__ = (sz);  	    	    	    	    	    	    \
    	DBG_ASSERT( (sz) >= 0 );    	    	    	    	    	    \
    	if (size__ > 0) {   	    	    	    	    	    	    \
    	    int n__;	    	    	    	    	    	    	    \
    	    for (n__ = 0; n__ < size__; ++n__, ++cucp1__, ++cucp2__) {	    \
    	    	if (*cucp1__ != *cucp2__) { 	    	    	    	    \
    	    	    DBG_ABORTMSG(("%s(%d): equality assertion failed. offset %#x> %s{%#x} != %s{%#x}\n" \
    	    	    	, __FILE__, __LINE__, n__, #s1, *cucp1__, #s2, *cucp2__ )); \
    	    	    break;  	    	    	    	    	    	    \
    	    	}   	    	    	    	    	    	    	    \
    	    }	    	    	    	    	    	    	    	    \
    	}   	    	    	    	    	    	    	    	    \
    	break;	    	    	    	    	    	    	    	    \
    }


/// C文字列 s1 と s1 が一致すれば成功、でなければ失敗
#define DBG_ASSERT_CSTRS_EQUAL(s1, s2 ) for(;;) {   	    	    	    \
    	const unsigned char *cucp1__ = (const unsigned char *)(s1); 	    \
    	const unsigned char *cucp2__ = (const unsigned char *)(s2); 	    \
    	int n__;    	    	    	    	    	    	    	    \
    	for (n__ = 0; ; ++n__, ++cucp1__, ++cucp2__) {	    	    	    \
    	    if (*cucp1__ != *cucp2__) {     	    	    	    	    \
    	    	DBG_ABORTMSG(("%s(%d): equality assertion failed. offset %#x> %s{%s} != %s{%s}\n" \
    	    	    , __FILE__, __LINE__, n__, #s1, cucp1__, #s2, cucp2__));\
    	    	break;	    	    	    	    	    	    	    \
    	    }	    	    	    	    	    	    	    	    \
    	    if (*cucp1__ == 0 || *cucp2__ == 0)     	    	    	    \
    	    	break;	    	    	    	    	    	    	    \
    	}   	    	    	    	    	    	    	    	    \
    	break;	    	    	    	    	    	    	    	    \
    }





// ===========================================================================
// c++ 専用のデバッグ
// ===========================================================================

#ifdef __cplusplus

#ifndef NDEBUG
template<typename T, typename S> T* err_chk_cast(S* p) { T* t = dynamic_cast<T*>(p); assert(t != NULL); return t; }
#else
template<typename T, typename S> T* err_chk_cast(S* p) { return static_cast<T*>(p); }
#endif

// ------------------------------------------------
/** クラスメンバに記述することで、クラスの生成回数をNMAX回までに限定する
 */
#define DBG_COUNT_CLASS(NMAX)	    	    	    	    	    	    \
    class Dbg_Count_Class { 	    	    	    	    	    	    \
    	static int dbg_count_class_add(int add) {   	    	    	    \
    	    static int num = 0;     	    	    	    	    	    \
    	    num += add;     	    	    	    	    	    	    \
    	    return num;     	    	    	    	    	    	    \
    	}   	    	    	    	    	    	    	    	    \
    public: 	    	    	    	    	    	    	    	    \
    	Dbg_Count_Class() { 	    	    	    	    	    	    \
    	    int n = dbg_count_class_add(1); 	    	    	    	    \
    	    if (n > NMAX)   	    	    	    	    	    	    \
    	    	DBG_ABORTMSG(("%s が %d 回目(最大 %d)だ\n"  	    	    \
    	    	    , __FUNCTION__, n, NMAX));	    	    	    	    \
    	}   	    	    	    	    	    	    	    	    \
    	~Dbg_Count_Class() {	    	    	    	    	    	    \
    	    int n = dbg_count_class_add(-1);	    	    	    	    \
    	    if (n < 0)	    	    	    	    	    	    	    \
    	    	DBG_ABORTMSG(("%s で数が負になった\n"	    	    	    \
    	    	    , __FUNCTION__));	    	    	    	    	    \
    	}   	    	    	    	    	    	    	    	    \
    };	    	    	    	    	    	    	    	    	    \
    Dbg_Count_Class DBG_STR_CAT(dbg_count_class_,DBG_I2I(NMAX))


// ------------------------------------------------
/// 現在(this)のクラスサイズをデバッグ出力
#define DBG_PUT_CLASS_SIZE(c)	    DBG_PRINTF(("%s %#x(%d)bytes\n", #c, sizeof(c), sizeof(c)))



// ------------------------------------------------
// 再起呼び出しされてはいけない処理のチェック用
// 番号n は、同一視する処理を同一番号にすること.
// つまり、判別したいグループ別に番号を変えて設定すること.

#define DBG_CHK_NEST_CALL(n)	    dbg_chk_nest_call<n>    dbg_chk_nest_call___(__FILE__, __LINE__)
#define DBG_IS_NEST_CALL(n) 	    dbg_chk_nest_call<n>::isNested()

template<unsigned n=0>
class dbg_chk_nest_call {
public:
    dbg_chk_nest_call(const char* name, unsigned line) {
    	if (s_cnt_) {
    	    if (n == 1)
    	    	ERR_PRINTF(("%s (%d) : エラー ( mallc系の呼出が別スレッドから行われいる可能性大! )\n", name, line));
    	    else
    	    	ERR_PRINTF(("%s (%d) : エラー : ネストしてはダメな呼び出しがネストしている\n", name, line));
    	}
    	++s_cnt_;
    }

    ~dbg_chk_nest_call() {
    	--s_cnt_;
    }

    static bool isNested() { return s_cnt_ > 1; }

private:
    static volatile int s_cnt_;
};
template<unsigned n>
volatile int dbg_chk_nest_call<n>::s_cnt_ = 0;



#endif	// __cplusplus

#endif



#endif	// DBG_H
