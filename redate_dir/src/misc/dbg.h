/**
 *  @file   dbg.h
 *  @brief  c/c++ �G���[���f�o�b�O����
 *  @author �k��
 *  @note
 *  - �ɗ́AC/C++�����Ŏg����悤�ɐݒ�.
 *  - �^�[�Q�b�g����\�� TARGET �}�N�����ݒ肳��Ă��Ȃ��ꍇ�A
 *    �܂��� TARGET=0 �Ȃ�΁A�R�}���h���C���p��printf�f�o�b�O�Ɖ��߁B
 *  - dbg_,DBG_ �n�͐��i�Ɏc���Ȃ��f�o�b�O�E���[�`�������B
 *  - err_,ERR_ �n�͐��i�Ɏc��f�o�b�O�E���[�`�������B
 *  - DBG_ �� ERR_ �啶���݂̂̓}�N���őg�܂�Ă��āARELEASE�R���p�C���ł��Y��
 *    �ɏ�����
 *    �t�ɏ������̊֐��́A�@�\�͂Ȃ��Ȃ邪�֐��Ăяo�����͎̂c��̂Œ��ӁB
 *    ��{�I�ɁA�}�N���̂ق����g�����ƁI
 *  - �J�����̉�ʏo�͊֌W�� Deve �N���X��p����Bdbg/err�ł͍s��Ȃ��I
 *  - �����}�N���������΃e�N�j�b�N�ł� do {�`; }while(0) ���g���ׂ������A
 *    while(0)���R���p�C���x�������̂ŁA�܂��f�o�b�O���݂̂̃��[�`����
 *    for(;;){�`;break;}��p���Ă���B(���[�v�W�����v�ƒE�o�W�����v���œK�����Ă������Ȃ��\��������)
 */


#ifndef DBG_H
#define DBG_H

// ===========================================================================

// TARGET ���ݒ肳��Ă��Ȃ�������Ƃ肠�����A�����t���ŃR�}���h���C�������Ƃ��Ĉ���

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

// CodeWarriar�ŃR���p�C������Ƃ��̂��܉�킹
#if defined(__cdecl) == 0
#define __cdecl
#endif



// ===========================================================================
// �f�o�b�O�����}�N��

// 2�̈�����A�����ĂP�̃��x���𐶐�
#define DBG_STR_CAT(a,b)    DBG_STR_CAT_2(a,b)
#define DBG_STR_CAT_2(a,b)  a##b

/// �R���p�C����assert.
#if defined _MSC_VER
#define DBG_STATIC_ASSERT(cc)	struct DBG_STR_CAT(STATIC_ASSERT_CHECK_ST,__LINE__) { char chk[(cc)?1:-1];}; enum { DBG_STR_CAT(STATIC_ASSERT_CHECK,__LINE__) = sizeof( DBG_STR_CAT(STATIC_ASSERT_CHECK_ST,__LINE__) ) }
#else
#define DBG_STATIC_ASSERT(cc)	typedef char DBG_STR_CAT(STATIC_ASSERT_CHECK_ST,__LINE__)[(cc)? 1/*OK*/ : -1/*ERROR*/]
#endif


// ���l��"���l"������
#define DBG_I2STR(a)	    DBG_I2STR_2(a)
#define DBG_I2STR_2(a)	    #a
#define DBG_I2I(a)  	    a

// __FUNCTION__������΂�����g��(����C99�ɂȂ��ĂȂ������̂˂�...)
#if defined(__FUNCTION__) == 0
    #if (defined __STDC_VERSION__ == 0 || __STDC_VERSION__ < 199901L)
    	#define __FUNCTION__	""  	    	// __FILE__ "(" DBG_I2STR(__LINE__) ")"
    #else
    	#define __FUNCTION__	__func__
    #endif
#endif




// ===========================================================================
// �f�o�b�O�E���O�o�͊֌W
// ===========================================================================

#if defined TARGET
// -------
// ������,�I��

/// �f�o�b�O�������s�����߂̏�����
int  dbg_init(void);

/// �f�o�b�O�����̏I��
void dbg_term(void);


// -------
// �G���[�o��

/// printf�`���Ń��b�Z�[�W���o���āA�I������֐��B���g�p�ł�ERR_EXIT()��p���邱�ƁB
int __cdecl err_exit(const char *fmt, ...);

/// printf�`���ŃG���[�o�́B
int __cdecl err_printf(const char *fmt, ...);


// -------
// �G���[�o��

/// err_exit�ŕ\������t�@�C�����ƍs��ݒ�. s=NULL�Ȃ�\���Ȃ�
#define err_setFnameLine(s, l)	(g_err_fname__ = (s), g_err_line__ = (l))

/// �֐�/���W���[�����̐ݒ�
//int err_setFuncName(const char *name);
#define err_setFuncName(f)  (g_err_funcName__=(f))  ///< �G���[�o�͂ŕ\������֐�����ݒ�
#define err_fname() 	    g_err_fname__   	    ///< ���݂̃G���[�t�@�C�������擾
#define err_line()  	    g_err_line__    	    ///< ���݂̃G���[�s�ԍ����擾
#define err_funcName()	    g_err_funcName__	    ///< ���݂̃G���[�֐������擾

extern const char   	    *g_err_fname__; 	    ///< err/dbg���W���[�����ϐ�. ���ڎg������ʖڂ�B
extern int  	    	    g_err_line__;   	    ///< werr/dbg���W���[�����ϐ�. ���ڎg������ʖڂ�B
extern const char   	    *g_err_funcName__;	    ///< err/dbg���W���[�����ϐ�. ���ڎg������ʖڂ�B


// -------
// �f�o�b�O

extern int  g_dbg_log_sw__; 	///< [���ڎg�p������_��] dbg_printf���ł̃��O�o�̗͂L����ێ�. 0:off 1:on
#define dbg_setLogSw(sw)    (g_dbg_log_sw__ = (sw)) ///< ���O�o�͂��s�����ۂ���ݒ�
#define dbg_getLogSw()	    (g_dbg_log_sw__)	    ///< ���݂̃��O�o�̗͂L�����擾
#define dbg_revLogSw()	    (g_dbg_log_sw__ ^= 1)   ///< ���O�o�̗͂L�����A���]����




/// printf�`���� �f�o�b�O���O�o��. �����������1024�o�C�g�܂ł̂���
int __cdecl dbg_printf(const char *fmt, ...);

/// 1�s�f�o�b�O���O�o��
void dbg_puts(const char *s);

/// �P�s�f�o�b�O���O�o�͂̏�����. dbg���W���[������p
void dbg_puts_init(void);

/// �A�h���X addr ���� sz �o�C�g���_���v���ă��O�o��
void dbg_dump(const void *addr, int sz);

/// 1�s�f�o�b�O���O�o��(�t�@�C����,�s�t��)
void dbg_flMsg(const char *srcname, int line, const char *msg);


#endif



// ===========================================================================
// �f�o�b�O�E���O�o�̓}�N��
// ===========================================================================
//-----------------------------------------------------
// �}�N��

/** @def DBG_ASSERT(x)
 *  	�� x �̒l���U�Ȃ�΁A�G���[�I��. �W�����C�u������assert()�̑���
 */
/** @def DBG_M()
 *  	c�\�[�X��+�s�ԍ��݂̂̃��O�o��. �ʉ߃`�F�b�N��z��.
 */
/** @def DBG_S(s)
 *  	1�s���O�o��.
 */
/** @def DBG_F(s)
 *  	printf�`���Ń��O�o�́B������ DBG_F(("%s���Ȃ�\n",name)); �̂悤��(())�ŗp����B
 */
/** @def DBG_PRINTF(s)
 *  	printf�`���Ń��O�o�́B������ DBG_PRINTF(("%s���Ȃ�\n",name)); �̂悤��(())�ŗp����B
 */
/** @def DBG_DUMP(a,n)
 *  	�A�h���X a���� n�o�C�g���_���v�o��
 */
/** @def ERR_F(s)
 *  	�G���[���b�Z�[�W�o�́B������ DBG_F(s)�ɓ���
 */
/** @def ERR_PRINTF(s)
 *  	ERR_F(s) �ɓ���
 */
/** @def ERR_EXIT(s)
 *  	�G���[���b�Z�[�W (s)���o�͂��ďI��. ������ DBG_F(s)�ɓ���
 */
/** @def ERR_ROUTE()
 *  	�ʉ߂����Ⴂ���Ȃ����[�g�ɒu���}�N��. assert(0)�̓�����
 */


#if defined(_RELEASE)	/*NDEBUG*/  // �����[�X��. �f�o�b�O���O�o�͂Ȃ��ɂ���B
 #define DBG_M()
 #define DBG_PRINTF(s)
 #define DBG_DUMP(a,n)
 #define DBG_ABORTMSG(s)

 #if defined(TARGET) && TARGET > 0
   // �^�[�Q�b�g�@�Ȃ�A�������Ȃ�. ���܂�G���[�ł��p��������...�^�C�������A�n���O��������́A�ƁB
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
  // �R�}���h���C���̂Ƃ��́A�G���[�I�����Ƃ�
  #define ERR_PRINTF(s)     (printf s)
  #define ERR_ABORTMSG(s)   ((printf s), ((int(*)(int))exit)(1))
  #define ERR_EXIT(s)	    ERR_ABORTMSG(s)
  #define ERR_ROUTE()	    ERR_ABORTMSG(("%s(%d): bad route! in %s\n", __FILE__, __LINE__, __FUNCTION__))
  #define ERR_ABORT()	    (exit(1))
 #endif

#else	// �f�o�b�O���̓��O�o��
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
// invariant�`�F�b�N����
// ===========================================================================

/** @def DBG_LIM_I(a, mi, ma)
 *  ���� a �� [mi, ma] �͈͓̔��Ȃ�ok
 */
/** @def DBG_LIM_U(a, ma)
 *  ���� a �� [0, ma] �͈͓̔��Ȃ�ok.
 *  (gcc ����unsigned�n�ϐ��ɑ΂���>=0���s���ƌx���ɂȂ邽�߁A�p��)
 */
/** @def DBG_LIM_BOOL(a)
 *  ���� a �� [0, 1] �Ȃ�ok
 */
/** @def DBG_LIM_F(a, mi, ma)
 *  float32�^�ϐ� a �� [mi, ma] �͈͓̔��Ȃ�ok
 */
/** @def DBG_LIM_D(a, mi, ma)
 *  float64�^�ϐ� a �� [mi, ma] �͈͓̔��Ȃ�ok
 */
/** @def DBG_LIM_CSTR(a, len)
 *  c������ a �� len�o�C�g�ȉ��Ȃ�ok.
 */
/** @def DBG_LIM_CSTR0(a, len)
 *  c������ a �� NULL �� len�o�C�g�ȉ��Ȃ�ok.
 */
/** @def DBG_LIM_VEC4(v, vmi, vma)
 * float[4] �̊e�v�f�� [vmi,vma] �͈̔͂ɂ���� ok
 */
/** @def DBG_CHK_PTR(p, asz)
 *  p �������pointer�͈̔͂ŁA�A���C�����g�� asz�o�C�g�ł���� ok
 */
/** @def DBG_CHK_PTR0(p, asz)
 *  p ��NULL���A�����pointer�͈̔͂ŁA�A���C�����g�� asz�o�C�g�ł���� ok
 */
/** @def DBG_ASSERT_MESSAGE(msg, cond)
 *   cond �̒l���^�Ȃ�ok. �U�̂Ƃ����b�Z�[�W��\�����Ď��s
 */
/** @def DBG_FAIL( msg )
 *   �K�����s���ă��b�Z�[�W���o��
 */
/** @def DBG_ASSERT_EQUAL( expected, actual )
 *  ���Ғlexpected �� ����actual �����������ok.
 */
/** @def DBG_ASSERT_EQUAL_MESSAGE( message, expected, actual )
 *  ���Ғlexpected �� ����actual �����������ok. �������Ȃ��Ƃ����b�Z�[�W��\�����Ď��s
 */
/** @def DBG_ASSERT_INTS_EQUAL( expected, actual )
 *  ���Ғlexpected �� ����actual �����������ok
 */
/** @def DBG_ASSERT_INT64S_EQUAL( expected, actual )
 *  ���Ғlexpected �� ����actual �̍��� delta �ȓ��Ȃ�ok. ���傫���Ƃ����s
 */
/** @def DBG_ASSERT_FLOATS_EQUAL( expected, actual, delta )
 *  ���Ғlexpected �� ����actual �̍��� delta �ȓ��Ȃ�ok. ���傫���Ƃ����s
 */
/** @def DBG_ASSERT_DOUBLES_EQUAL( expected, actual, delta )
 *  ���Ғlexpected �� ����actual �̍��� delta �ȓ��Ȃ�ok. ���傫���Ƃ����s
 */
/** @def DBG_ASSERT_MEMS_EQUAL( s1, s2, sz )
 *  ������ s1 �� s2 �� sz �o�C�g����������� ok.
 */
/** @def DBG_ASSERT_CSTRS_EQUAL(s1, s2 )
 *  C������ s1 �� s1 ����v����ΐ����A�łȂ���Ύ��s
 */
/** @def DBG_ASSERT_THROW(x)
 *  x ����O�𓊂���ΐ^.
 */

#if defined( NDEBUG )	// defined(NDEBUG) // �����[�X�� - - - - - -

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

// CppUnit �ɂ���`�F�b�N�̗ގ��i
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



#else	// �f�o�b�O�� - - - - - - - - - - - - - - -

// UnitTest�ł̃��b�Z�[�W�\��
#define DBG_ASSERTMSG(x, s) 	do { if (!(x)) ERR_ABORTMSG(("%s(%d):%s> assert(%s) is false.\n\t%s\n", __FILE__, __LINE__, __FUNCTION__, #x, s));} while(0)
#define DBG_ASSERT(x)	    	do { if (!(x)) ERR_ABORTMSG(("%s(%d):%s> assert(%s) is false.\n", __FILE__, __LINE__, __FUNCTION__, #x));} while(0)
//#define DBG_ASSERT(x)     	((x) || ERR_ABORTMSG(("%s(%d):%s> assert(%s) is false.\n", __FILE__, __LINE__, __FUNCTION__, #x)) )
#define DBG_TSTMSG(s)	    	ERR_ABORTMSG(s)

// assert.h �̒�`�𖳌���. ���O�̂ɒu������.
#undef	assert
#define assert(x)   	    	DBG_ASSERT(x)

#define DBG_NOCHK(a)
#define DBG_LIM_I(a, mi, ma)	do {if ((a) < (mi) || (ma) < (a)) DBG_TSTMSG(("%s (%d): %s: %d <= %s <= %d �𖞂����Ȃ�(%d)\n", __FILE__, __LINE__, __FUNCTION__, (mi), #a, (ma),(a) ));} while(0)
#define DBG_LIM_U(a, ma)    	do { if (! (unsigned(a) <= (ma))    	) DBG_TSTMSG(("%s (%d): %s: %d <= %s <= %d �𖞂����Ȃ�(%d)\n", __FILE__, __LINE__, __FUNCTION__, 0, #a, (ma),(a) ));} while(0)
#define DBG_LIM_BOOL(a)     	do {if (int(a)!= 0 && int(a)!=1) DBG_TSTMSG(("%s (%d): %s: 0 <= %s <= 1 �𖞂����Ȃ�(%d)\n", __FILE__, __LINE__, __FUNCTION__, #a, (a) ));} while(0)
#define DBG_LIM_F(a, mi, ma)	do {const float *aa=&(a); if ((a) < (mi) || (ma) < (a)) DBG_TSTMSG(("%s (%d): %s: %f <= %s <= %f �𖞂����Ȃ�(%f)\n", __FILE__, __LINE__, __FUNCTION__, (mi), #a, (ma),(a) ));} while(0)
//#define DBG_LIM_F(a, mi, ma)	do {const float *aa=&(a); if ((a) < (mi) || (ma) < (a)) DBG_TSTMSG(("%s (%d): %s: %f <= %s <= %f �𖞂����Ȃ�(%f)\n", __FILE__, __LINE__, __FUNCTION__, (double)(mi), #a, (double)(ma),(double)(a) ));} while(0)
//#define DBG_LIM_D(a, mi, ma)	do {const double *aa=&(a); if ((a) < (mi) || (ma) < (a)) DBG_TSTMSG(("%s (%d): %s: %f <= %s <= %f �𖞂����Ȃ�(%f)\n", __FILE__, __LINE__, __FUNCTION__, (double)(mi), #a, (double)(ma),(a) ));} while(0)
#define DBG_LIM_CSTR(a, sz) 	do {int l__ = strlen(a); if (l__ <= 0 || (sz) <= l__) DBG_TSTMSG(("%s (%d): %s: %s{`%s'}�̒���%d��1�`%d�͈̔͊O\n", __FILE__, __LINE__, __FUNCTION__, #a, (a), l__, (sz)-1 ));} while(0)
#define DBG_LIM_CSTR0(a, sz)	do {int l__ = strlen(a); if (l__ < 0 || (sz) <= l__) DBG_TSTMSG(("%s (%d): %s: %s{`%s'}�̒���%d��0�`%d�͈̔͊O\n", __FILE__, __LINE__, __FUNCTION__, #a, (a), l__, (sz)-1 ));} while(0)


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

// �|�C���^�Ƃ��Ă������Ȓl�Ȃ�^( ���ˑ��Ȃ̂ŁA�����ł͑�G�c�ɒ�` )
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


/// p������Ȕ͈͂̃A�h���X�Ȃ�ok(NULL�s��)
#define DBG_PTR_ASSERT(p)   	DBG_ASSERT(!DBG_IS_BADPTR(p))	    	// DBG_CHK_PTR(p, 1)
/// p������Ȕ͈͂̃A�h���X��NULL�Ȃ�ok.
#define DBG_PTR0_ASSERT(p)  	DBG_ASSERT(p == 0 || !DBG_IS_BADPTR(p)) // DBG_CHK_PTR0(p, 1)

/// num�̔z��ary �̊e invariant() �����s.
#define DBG_ARY_INVARIANT( ary, num )	for(;;) {   	    	    	    \
    unsigned int num__ = (unsigned int)(num);	    	    	    	    \
    for (unsigned int i__ = 0; i__ < num__; ++i__) {	    	    	    \
    	(ary)[i__].invariant();     	    	    	    	    	    \
    }	    	    	    	    	    	    	    	    	    \
    break;  	    	    	    	    	    	    	    	    \
}

/// x ����O�𓊂���ΐ^.
#define DBG_ASSERT_THROW(x) 	for(;;) { bool f = 0; try { x; } catch (...) { f = 1; } DBG_ASSERT_MESSAGE( #x " ����O�𓊂��Ȃ�\n", f); break;}



// -----------------------------------------
// CppUnit �ɂ���`�F�b�N�̗ގ��i

/// cond �̒l���U�̂Ƃ����b�Z�[�W��\�����Ď��s
#define DBG_ASSERT_MESSAGE(msg, cond)	    	((cond) || (DBG_ABORTMSG(("%s(%d): %s\n", __FILE__, __LINE__, msg)), 0))

/// �K�����s
#define DBG_FAIL( msg )     	    	    	(DBG_ABORTMSG(("%s(%d): %s\n", __FILE__, __LINE__, msg)), 0)

/// ���Ғlexpected �� ����actual ���������Ȃ��Ƃ����b�Z�[�W��\�����Ď��s
#define DBG_ASSERT_EQUAL_MESSAGE( message, expected, actual )  for(;;) {    \
    	if ((expected) != (actual)) 	    	    	    	    	    \
    	    DBG_ABORTMSG(("%s(%d): %s\n", __FILE__, __LINE__, msg));	    \
    	break;	    	    	    	    	    	    	    	    \
    }


/// ���Ғlexpected �� ����actual ���������Ȃ��Ƃ����s
#define DBG_ASSERT_EQUAL( expected, actual )  for(;;) {     	    	    \
    	if ((expected) != (actual)) 	    	    	    	    	    \
    	    DBG_ABORTMSG(("%s(%d): equality assertion failed. %s != %s\n"   \
    	    	, __FILE__, __LINE__, #expected, #actual)); 	    	    \
    	break;	    	    	    	    	    	    	    	    \
    }


/// ���Ғlexpected �� ����actual ���������Ȃ��Ƃ����s
#define DBG_ASSERT_INTS_EQUAL( expected, actual )  for(;;) {	    	    \
    	if ((expected) != (actual)) {	    	    	    	    	    \
    	    DBG_ABORTMSG(("%s(%d): equality assertion failed. %s{%d} != %s{%d}\n" \
    	    	, __FILE__, __LINE__, #expected,expected, #actual,actual)); \
    	}   	    	    	    	    	    	    	    	    \
    	break;	    	    	    	    	    	    	    	    \
    }


/// ���Ғlexpected �� ����actual ���������Ȃ��Ƃ����s
#define DBG_ASSERT_INT64S_EQUAL( expected, actual )  for(;;) {	    	    \
    	if ((expected) != (actual)) {	    	    	    	    	    \
    	    DBG_ABORTMSG(("%s(%d): equality assertion failed. %s{%#x%08x} != %s{%#x%08x}\n" 	    \
    	    	, __FILE__, __LINE__	    	    	    	    	    	    	    	    \
    	    	, #expected, (unsigned int)((uint64_t)(expected)>>32), (unsigned int)(expected)     \
    	    	, #actual,   (unsigned int)((uint64_t)(actual)	>>32), (unsigned int)(actual)	)); \
    	}   	    	    	    	    	    	    	    	    \
    	break;	    	    	    	    	    	    	    	    \
    }

/// ���Ғlexpected �� ����actual �̍��� delta ���傫���Ƃ����s
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

/// ���Ғlexpected �� ����actual �̍��� delta ���傫���Ƃ����s
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



/// ������ s1 �� s1 �� sz �o�C�g����v����ΐ����A�łȂ���Ύ��s
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


/// C������ s1 �� s1 ����v����ΐ����A�łȂ���Ύ��s
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
// c++ ��p�̃f�o�b�O
// ===========================================================================

#ifdef __cplusplus

#ifndef NDEBUG
template<typename T, typename S> T* err_chk_cast(S* p) { T* t = dynamic_cast<T*>(p); assert(t != NULL); return t; }
#else
template<typename T, typename S> T* err_chk_cast(S* p) { return static_cast<T*>(p); }
#endif

// ------------------------------------------------
/** �N���X�����o�ɋL�q���邱�ƂŁA�N���X�̐����񐔂�NMAX��܂łɌ��肷��
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
    	    	DBG_ABORTMSG(("%s �� %d ���(�ő� %d)��\n"  	    	    \
    	    	    , __FUNCTION__, n, NMAX));	    	    	    	    \
    	}   	    	    	    	    	    	    	    	    \
    	~Dbg_Count_Class() {	    	    	    	    	    	    \
    	    int n = dbg_count_class_add(-1);	    	    	    	    \
    	    if (n < 0)	    	    	    	    	    	    	    \
    	    	DBG_ABORTMSG(("%s �Ő������ɂȂ���\n"	    	    	    \
    	    	    , __FUNCTION__));	    	    	    	    	    \
    	}   	    	    	    	    	    	    	    	    \
    };	    	    	    	    	    	    	    	    	    \
    Dbg_Count_Class DBG_STR_CAT(dbg_count_class_,DBG_I2I(NMAX))


// ------------------------------------------------
/// ����(this)�̃N���X�T�C�Y���f�o�b�O�o��
#define DBG_PUT_CLASS_SIZE(c)	    DBG_PRINTF(("%s %#x(%d)bytes\n", #c, sizeof(c), sizeof(c)))



// ------------------------------------------------
// �ċN�Ăяo������Ă͂����Ȃ������̃`�F�b�N�p
// �ԍ�n �́A���ꎋ���鏈���𓯈�ԍ��ɂ��邱��.
// �܂�A���ʂ������O���[�v�ʂɔԍ���ς��Đݒ肷�邱��.

#define DBG_CHK_NEST_CALL(n)	    dbg_chk_nest_call<n>    dbg_chk_nest_call___(__FILE__, __LINE__)
#define DBG_IS_NEST_CALL(n) 	    dbg_chk_nest_call<n>::isNested()

template<unsigned n=0>
class dbg_chk_nest_call {
public:
    dbg_chk_nest_call(const char* name, unsigned line) {
    	if (s_cnt_) {
    	    if (n == 1)
    	    	ERR_PRINTF(("%s (%d) : �G���[ ( mallc�n�̌ďo���ʃX���b�h����s��ꂢ��\����! )\n", name, line));
    	    else
    	    	ERR_PRINTF(("%s (%d) : �G���[ : �l�X�g���Ă̓_���ȌĂяo�����l�X�g���Ă���\n", name, line));
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
