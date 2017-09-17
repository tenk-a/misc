/**
 *  @file   filn.cpp
 *  @brief  ���������A�}�N���@�\���������e�L�X�g�t�@�C�����̓��[�`��.
 *  @author Masashi KITAMURA (tenka@6809.net)
 *  @date   1996-2017
 *  @note
 *  	Boost Software License Version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>
#include "Filn.h"
#include "mbc.h"



/*--------------------------------------------------------------------------*/
/* ���W���[�����ϐ� 	    	    	    	    	    	    	    */

typedef unsigned char	    UCHAR;

#define STDERR	    	    stderr

#define FILN_FNAME_SIZE     2100
#define FILN_NAME_SIZE	    256     /* ���x�����̕�����       */
#define FILN_INCL_NUM	    512     /* �ő� include�t�@�C���� */
#define FILN_ARG_NUM	    256     /* �}�N�������̍ő��   */
#define FILN_MAC_NEST	    256     /* �}�N���W�J�̍ő�l�X�g�� */
#define FILN_REPT_NEST	    256     /* rept�̃l�X�g��	    	*/
#define FILN_IF_NEST	    128     /* if �̃l�X�g��	    	*/

#define LINBUF0_SIZE	    0x20000U
#define LINBUF_SIZE 	    0x30000U
#define M_MBUF_SIZE 	    0x40000U

#if defined(_MSC_VER) && _MSC_VER < 1600
#define S_FMT_LL    	    "I64"
#else
#define S_FMT_LL    	    "ll"
#endif
#define S_FMT_X     	    "%" S_FMT_LL "x"
#define S_FMT_D     	    "%" S_FMT_LL "d"
#define S_FMT_U     	    "%" S_FMT_LL "u"

typedef int64_t     	    filn_val_t;
typedef filn_val_t  	    val_t;


typedef struct SRCLIST {
    struct SRCLIST* link;
    char*   	    s;
} SRCLIST;

typedef struct FILN_INC_T {
    FILE*   	fp;
    char*   	name;
    uint32_t   line;
    uint32_t   linCnt;
/*  int     lcflg;*/
} FILN_INC_T;


typedef struct filn_local_t {
/*private:*/

    /* �G���[�֌W */
    int     	errNum;
    int     	warNum;
    char const* errMacFnm;
    long    	errMacLin;

    /* �s���́A�}�N���W�J�̃o�b�t�@ */
    char*   	linbuf0;
    char*   	linbuf;

    /* �}�N���W�J�֌W */
    void*   	mtree;	    	   /* �}�N���E���x����o�^�����       */
    void*   	macBgnStr;  	   /* "begin"���ߗp 	    	       */

    char*   	M_str;
    char*   	M_mbuf;
    char*   	M_mbuf_end;
    int     	M_mbuf_size;
    char*   	M_mptr;     	   /* ������̓W�J(�o��)��o�b�t�@ */

    char    	M_name[FILN_NAME_SIZE+2];
    val_t   	M_val;
    int     	M_sym;

    char*   	M_argv[FILN_ARG_NUM];
    int     	M_argc;

    char const* M_ep;
    char    	M_expr_str[FILN_NAME_SIZE+2];
    char    	M_expr_str2[FILN_NAME_SIZE+2];

    char    	Expr0_nam[FILN_NAME_SIZE+2];
    char*   	Expr0_fnm;
    uint32_t	Expr0_line;
    int     	errLblSkip; /* && �̍��ӂ��U�̏ꍇ�̉E�ӂ̃X�L�b�v�p */

    char*   	chkEol_name;
    uint32_t	chkEol_line;

    char*   	MM_name[FILN_MAC_NEST];
    int     	MM_cnt;
    char    	MM_wk[30];
    char*   	MM_ptr;
    long    	MM_localNo;
    int     	MM_nest;    	    	    /* �}�N���W�J���̃l�X�g�� */

    char    	mac_chrs[2];
    char    	mac_chrs2[2];

    /* rept�Ǘ� */
    int     	rept_argc;
    char const* rept_name[FILN_REPT_NEST+2];
    char const* rept_argv[FILN_REPT_NEST+2];

    /* if�Ǘ� */
    int     	mifCur;     	    	    /* #if�̃l�X�g�Ǘ�	  */
    int     	mifChkSnc;  	    	    /* #if�̋U�͈̔͂̃l�X�g�Ǘ� */
    UCHAR   	mifStk[FILN_IF_NEST+2];     /* #if �̃l�X�g�Ǘ� */
    int     	mifMacCur;  	    	    /* �}�N���̃l�X�g�Ǘ� */
    UCHAR   	mifMacStk[FILN_IF_NEST+2];  /* exitm �p �}�N���J�n����mifCur�̒l��ێ� */
    int     	MM_mifChk;

    // ���\�[�X���R�����g�����ē��͂��邽�߂̏��� (V.opt_orgSrc �w�莞) �p
    //char* 	sl_buf;
    SRCLIST*	sl_lst;

    /* include�ǂ݂��݂̊Ǘ��֌W */
    int     	inclNo;
    FILN_INC_T* inclp;
    FILN_INC_T	inclStk[FILN_INCL_NUM];

    /*char  	Z.pc3name[1026];*/
    /*long  	Z.pc3line = -1;*/
} filn_local_t;


filn_t*     	    	Filn	   = NULL;
static filn_local_t*	filn_local = NULL;

#define V   	(*Filn)
#define Z   	(*filn_local)


#define SAFE_FREE(p)	    do { freeE(p); (p) = NULL; } while (0)



/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
/* �G���[�֌W	*/



static int Filn_ErrVPrintf(char const* fmt, void *app)
{
    int n;

    if (Z.inclp && Z.inclp->name && Z.inclp->name[0]) {
    	fprintf(V.errFp, "%-13s %5u : ", Z.inclp->name, Z.inclp->line);
    }
    n = vfprintf(V.errFp, fmt, app);
    if (Z.errMacFnm && V.macErrFlg) {
    	fprintf(V.errFp, "(%-12s %5lu :\t%cmacro,%crept���̒�`�̒�)\n", Z.errMacFnm, Z.errMacLin, V.mac_chr, V.mac_chr);
    }
    fflush(V.errFp);
    return n;
}


int Filn_Error(char const* fmt, ...)
{
    va_list app;

    va_start(app, fmt);
    Filn_ErrVPrintf(fmt, app);
    va_end(app);
    Z.errNum++;
    return 0;
}


int Filn_Warnning(char const* fmt, ...)
{
    va_list app;

    va_start(app, fmt);
    Filn_ErrVPrintf(fmt, app);
    va_end(app);
    Z.warNum++;
    return 0;
}


volatile int Filn_Exit(char const* fmt, ...)
{
    va_list app;

    va_start(app, fmt);
    Filn_ErrVPrintf(fmt, app);
    va_end(app);
  #if 0
    fprintf(V.errFp, "  �G���[�� %d. �x���� %d.\n", Z.errNum, Z.warNum);
  #endif
    exit(1);
    return 0;
}


/** �G���[�o�͐���Đݒ肷��Bname���Ȃ���΁A���� fp ���A���O�������
 * freopn �����t�@�C���ɂ��܂��Bfp == NULL �̏ꍇ�� STDERR ���̗p���܂�
 */
FILE *Filn_ErrReOpen(char const* name, FILE* fp)
{
    if (name == NULL) {
    	if (fp == NULL)
    	    fp = STDERR;
    } else {
    	if (fp == NULL)
    	    fp = fopen(name, "at");
    	else
    	    fp = freopen(name, "at", STDERR);
    	if (fp == NULL) {
    	    V.errFp = STDERR;
    	    return NULL;
    	}
    }
    V.errFp = fp;
    return fp;
}


void Filn_ErrClose(void)
{
    if (V.errFp && V.errFp != STDERR && V.errFp != stdout && V.errFp != stderr) {
    	fclose(V.errFp);
    	V.errFp = STDERR;
    }
}



/** �G���[�ƌx���̐���Ԃ�
 */
void Filn_GetErrNum(int* errNum, int* warnNum)
{
    if (errNum)
    	*errNum = Z.errNum;
    if (warnNum)
    	*warnNum = Z.warNum;
}



/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
/* �����ɂƂ��Ă̔ėp�T�u���[�`������ */

#define MEMBER_OFFSET(t,m)  ((long)&(((t*)0)->m))   /* �\���̃����o���́A�I�t�Z�b�g�l�����߂� */

//#define ISKANJI(c)	  ((UCHAR)(c) >= 0x81 && ((UCHAR)(c) <= 0x9F || ((UCHAR)(c) >= 0xE0 && (UCHAR)(c) <= 0xFC)))
//#define ISKANJI2(c)	  ((UCHAR)(c) >= 0x40 && (UCHAR)(c) <= 0xfc && (c) != 0x7f)
#define STREND(p)   	((p)+strlen(p))
#define STPCPY(d,s) 	(strcpy(d,s) + strlen(s))

#define FILN_MBC_IS_LEAD(c) (V.opt_kanji && mbs_islead(c))

static char* strncpyZ(char* dst, char const* src, size_t size)
{
    strncpy(dst, src, size);
    dst[size-1] = 0;
    return dst;
}


/** �G���[������Α�exit�� malloc()
 */
static void* mallocE(size_t a)
{
    void* p;

    if (a == 0)
    	a = 1;
    p = malloc(a);
    if (p == NULL) {
    	Filn_Exit("������������Ȃ�(%d byte(s))\n",a);
    }
    return p;
}


/** �G���[������Α�exit�� calloc()
 */
static void* callocE(size_t a, size_t b)
{
    void* p;

    if (a == 0)
    	a = 1;
    if (b == 0)
    	b = 1;
    p = calloc(a,b);
    if (p == NULL) {
    	Filn_Exit("������������Ȃ�(%d*%d byte(s))\n",a,b);
    }
    return p;
}


/** �G���[������Α�exit�� calloc()
 */
static void* reallocE(void* a, size_t b)
{
    void* p;

    if (a == 0) {
    	return malloc(b);
    }
    if (b == 0)
    	b = 1;
    p = realloc(a,b);
    if (p == NULL) {
    	Filn_Exit("������������Ȃ�(%d byte(s))\n",b);
    }
    return p;
}


/** �G���[������Α�exit�� strdup()
 */
static char* strdupE(char const* s)
{
    char* p;
    if (s == NULL)
    	return callocE(1,1);
    p = strdup(s);
    if (p == NULL) {
    	Filn_Exit("������������Ȃ�(����%d+1)\n",strlen(s));
    }
    return p;
}

/** �G���[������Α�exit�� strdup()
 */
static char *strndupE(char const* s, size_t n)
{
    char* p;
    if (s == NULL)
    	return callocE(1,1);
    p = callocE(1, n);
    strncpyZ(p, s, n);
    return p;
}

static int freeE(void* p)
{
    if (p)
    	free(p);
    return 0;
}


static SRCLIST* SRCLIST_Add(SRCLIST** p0, char const* s)
{
    SRCLIST* p;
    p = *p0;
    if (p == NULL) {
    	p = callocE(1, sizeof(SRCLIST));
    	if (s != NULL)
    	    p->s = strdupE(s);
    	*p0 = p;
    } else {
    	while (p->link != NULL) {
    	    p = p->link;
    	}
    	p->link = callocE(1, sizeof(SRCLIST));
    	p = p->link;
    	if (s != NULL)
    	    p->s = strdupE(s);
    }
    return p;
}



/*------------------------------------------------------------------------*/
/* AVL�� */

typedef struct TREE_NODE {
    struct TREE_NODE *link[2];
    void    	     *element;
    short   	     avltFlg;
} TREE_NODE;

typedef void* (*TREE_NEW)(void const*);
typedef void  (*TREE_DEL)(void*);
typedef int   (*TREE_CMP)(void const*, void const*);
typedef void* (*TREE_MALLOC)(unsigned);
typedef void  (*TREE_FREE)(void*);

typedef struct TREE {
    TREE_NODE*	root;
    TREE_NODE*	node;
    int     	flag;
    TREE_NEW	newElement;
    TREE_DEL	delElement;
    TREE_CMP	cmpElement;
    TREE_MALLOC malloc;
    TREE_FREE	free;
} TREE;

static TREE* TREE_Make(TREE_NEW newElement,TREE_DEL delElement,TREE_CMP cmpElement, TREE_MALLOC funcMalloc, TREE_FREE funcFree);
static void* TREE_Insert(TREE* tree, void* e);
static void* TREE_Search(TREE* tree, void* p);
static int   TREE_Delete(TREE* tree, void* e);	/* �v�f��؂���폜 */
static void  TREE_Clear(TREE* tree);
static void  TREE_DoAll(TREE* tree, void (*func)(void*));


//#define MSGF(x)   (printf x)
#define MSGF(x)

static TREE*	    curTree;
static void*	    elem;

static TREE_CMP     cmpElement;
static TREE_DEL     delElement;
static TREE_NEW     newElement;
static TREE_MALLOC  funcMalloc;
static TREE_FREE    funcFree;


/** �񕪖؂��쐬���܂��B�����́A�v�f�̍쐬,�폜,��r�̂��߂̊֐��ւ��߲��
 */
static TREE*	TREE_Make(TREE_NEW newElement,TREE_DEL delElement,TREE_CMP cmpElement, TREE_MALLOC funcMalloc, TREE_FREE funcFree)
{
    TREE*   p;

    if (funcMalloc == NULL)
    	return NULL;
    p = funcMalloc(sizeof(TREE));
    if (p) {
    	p->root = NULL;
    	p->node = NULL;
    	p->flag = 0;
    	p->newElement  = newElement;
    	p->delElement  = delElement;
    	p->cmpElement  = cmpElement;
    	p->malloc      = funcMalloc;
    	p->free        = funcFree;
    }
    return p;
}


static int  insRebalance(TREE_NODE** pp, int dir)
{
    int     	ndr, pt_dir, pt_ndr,grown;
    TREE_NODE	*ap,*bp,*cp;

    ndr = (dir == 0) ? 1 : 0;
    pt_dir = (1<<dir);
    pt_ndr = (1<<ndr);

    grown = 0;
    ap = *pp;
    if (ap->avltFlg == pt_ndr) {    	/* ���X�΂��Ă���΁A�΂��������P����̂Ńo�����X�ɂȂ�*/
    	ap->avltFlg =  0;
    } else if (ap->avltFlg == 0) {  	/* ���X�o�����X��ԂȂ�� */
    	ap->avltFlg |= pt_dir;	    	/* �폜�̔��Α��ɕ΂� */
    	grown = 1;
    } else {	    	    	    	/* �؂̍č\�� */
    	bp = ap->link[dir];
    	if (bp->avltFlg == pt_dir) {	/* ���] */
    	    ap->link[dir] = bp->link[ndr];
    	    bp->link[ndr] = ap;
    	    ap->avltFlg = 0;
    	    bp->avltFlg = 0;
    	    *pp = bp;
    	} else if (bp->avltFlg == pt_ndr) { 	    /* ���] */
    	    cp = bp->link[ndr];
    	    ap->link[dir] = cp->link[ndr];
    	    bp->link[ndr] = cp->link[dir];
    	    cp->link[ndr] = ap;
    	    cp->link[dir] = bp;
    	    if (cp->avltFlg != pt_ndr) {
    	    	bp->avltFlg =  0;
    	    } else {
    	    	bp->avltFlg =  pt_dir;
    	    }
    	    if (cp->avltFlg != pt_dir) {
    	    	ap->avltFlg =  0;
    	    } else {
    	    	ap->avltFlg =  pt_ndr;
    	    }
    	    cp->avltFlg = 0;
    	    *pp = cp;
    	} else {
    	    ;	/* ����͂��肦�Ȃ� */
    	}
    }
    return grown;
}

static int  insNode(TREE_NODE** pp)
{
    int grown,b,g;

    grown = 0;
    if (pp == NULL)
    	return 0;
    if (*pp == NULL) {
    	curTree->node = *pp = funcMalloc(sizeof(TREE_NODE));
    	if (*pp == NULL)
    	    return 0;
    	memset(*pp, 0x00, sizeof(TREE_NODE));
    	curTree->flag = 1;  /* �V���ɍ쐬���ꂽ */
    	(*pp)->element = newElement(elem);
    	/* MSGF(("elem=%d\n",(*pp)->element));*/
    	grown = 1;
    	return grown;
    }
    b = cmpElement(elem, (*pp)->element);
    /* MSGF(("b=%d\n",b));*/
    if (b == 0) {
    	curTree->node = (*pp);
    	return 0;
    }
  #if 1
    b = (b > 0) ? 1 : 0;
    g = insNode( & ((*pp)->link[b]) );
    if (g)
    	grown = insRebalance(pp, b);
  #else
    if (b < 0) {
    	g = insNode( & ((*pp)->link[0/*left*/]) );
    	if (g)
    	    grown = insRebalance(pp, 0);
    } else if (b > 0) {
    	g = insNode( & ((*pp)->link[1/*right*/]) );
    	if (g)
    	    grown = insRebalance(pp, 1/*right*/);
    }
  #endif
    return grown;
}


/** �v�f��؂ɑ}��
 */
static void*	TREE_Insert(TREE* tree, void* e)
{
    curTree 	= tree;
    funcMalloc	= tree->malloc;
    cmpElement	= tree->cmpElement;
    newElement	= tree->newElement;
    curTree->flag = 0;
    curTree->node = NULL;
    elem    	= e;
    insNode(&tree->root);
    if (curTree->node)
    	return curTree->node->element;
    return NULL;
}


/** �؂���v�f��T��
 */
static void* TREE_Search(TREE* tree, void* e)
{
    TREE_NODE*	np;
    int     	n;

    cmpElement	= tree->cmpElement;
    np = tree->root;
    while (np) {
    	n = cmpElement(e, np->element);
    	if (n < 0)
    	    np = np->link[0];
    	else if (n > 0)
    	    np = np->link[1];
    	else
    	    break;
    }
    tree->node = np;
    if (np == NULL)
    	return NULL;
    return np->element;
}


/** �폜�Ŗ؂̃o�����X��ۂ��߂̏���
 */
static int  	delRebalance(TREE_NODE** pp, int dir)
{
    int shrinked, ndr, pt_dir, pt_ndr;
    TREE_NODE *ap,*bp,*cp;

    ndr = (dir == 0) ? 1 : 0;
    pt_dir = (1<<dir);
    pt_ndr = (1<<ndr);

    ap = *pp;
    if (ap->avltFlg == 0) { 	    	/* ���X�o�����X��ԂȂ�� */
    	ap->avltFlg |= pt_ndr;	    	/* �폜�̔��Α��ɕ΂� */
    	shrinked    =  0;
    } else if (ap->avltFlg == pt_dir) { /* ���X�΂��Ă���΁A�΂��������P����̂Ńo�����X�ɂȂ�*/
    	ap->avltFlg =  0;
    	shrinked    =  1;
    } else {	    	    	    	/* �؂̍č\�� */
    	bp = ap->link[ndr];
    	if (bp->avltFlg != pt_dir) {	/* ���] */
    	    ap->link[ndr] = bp->link[dir];
    	    bp->link[dir] = ap;
    	    if (bp->avltFlg == 0) {
    	    	ap->avltFlg = pt_ndr;
    	    	bp->avltFlg = pt_dir;
    	    	shrinked = 0;
    	    } else {
    	    	ap->avltFlg = 0;
    	    	bp->avltFlg = 0;
    	    	shrinked = 1;
    	    }
    	    *pp = bp;
    	} else {    	    	    	/* ���] */
    	    cp = bp->link[dir];
    	    ap->link[ndr] = cp->link[dir];
    	    bp->link[dir] = cp->link[ndr];
    	    cp->link[dir] = ap;
    	    cp->link[ndr] = bp;
    	    if (cp->avltFlg != pt_ndr) {
    	    	ap->avltFlg =  0;
    	    } else {
    	    	ap->avltFlg =  pt_dir;
    	    }
    	    if (cp->avltFlg != pt_dir) {
    	    	bp->avltFlg =  0;
    	    } else {
    	    	bp->avltFlg =  pt_ndr;
    	    }
    	    cp->avltFlg = 0;
    	    *pp = cp;
    	    shrinked = 1;
    	}
    }
    return shrinked;
}


static int  delExtractMax(TREE_NODE** pp, TREE_NODE** qq)
{
    int s, shrinked;
    enum {L=0,R=1};

    if ((*pp)->link[R] == NULL) {
    	*qq = *pp;
    	*pp = (*pp)->link[L];
    	shrinked = 1;
    } else {
    	shrinked = 0;
    	s = delExtractMax(&((*pp)->link[R]), qq);
    	if (s)
    	    shrinked = delRebalance(pp, R);
    }
    return shrinked;
}


static int  DeleteNode(TREE_NODE** pp)
{
    int     	c,s,shrinked;
    TREE_NODE*	p;
    TREE_NODE*	t;
    enum {L=0,R=1};

    shrinked = 0;
    p = *pp;
    if (p == NULL) {
    	return -1;  	/* �폜���ׂ� node ��������Ȃ� */
    	/*printf ("PRGERR: AVL-TREE DELETE\n");*/
    	/*exit(1);*/
    }
    c = cmpElement(elem,p->element);
    if (c < 0) {
    	s = DeleteNode(&p->link[L]);
    	if (s < 0)
    	    return -1;
    	if (s)
    	    shrinked = delRebalance(&p, L);
    } else if (c > 0) {
    	s = DeleteNode(&p->link[R]);
    	if (s < 0)
    	    return -1;
    	if (s)
    	    shrinked = delRebalance(&p, R);
    } else {
    	if (p->link[L] == NULL) {
    	    t = p;
    	    p = p->link[R];
    	    delElement(t->element);
    	    funcFree(t);
    	    shrinked = 1;
    	} else {
    	    s = delExtractMax(&p->link[L], &t);
    	    t->link[L] = p->link[L];
    	    t->link[R] = p->link[R];
    	    t->avltFlg = p->avltFlg;
    	    delElement(p->element);
    	    funcFree(p);
    	    p = t;
    	    if (s)
    	    	shrinked = delRebalance(&p, L);
    	}
    }
    *pp = p;
    return shrinked;
}


/** �v�f��؂���폜
 */
static int TREE_Delete(TREE* tree, void* e)
{
    int c;

    curTree 	= tree;
    funcFree	= tree->free;
    cmpElement	= tree->cmpElement;
    delElement	= tree->delElement;
    elem    	= e;
    c = DeleteNode(&tree->root);
    if (c < 0)
    	return -1;  /* �폜���ׂ����̂��݂�����Ȃ����� */
    return 0;
}


static void DelAllNode(TREE_NODE* np)
{
    if (np == NULL)
    	return;
    if (np->link[0])
    	DelAllNode(np->link[0]);
    if (np->link[1])
    	DelAllNode(np->link[1]);
    if (delElement)
    	delElement(np->element);
    funcFree(np);
    return;
}


/** �؂���������
 */
static void TREE_Clear(TREE* tree)
{
    delElement	= tree->delElement;
    funcFree	= tree->free;
    DelAllNode(tree->root);
    funcFree(tree);
    return;
}


static void DoElement(TREE_NODE* np, void (*DoElem)(void*))
{
    if (np == NULL)
    	return;
    if (np->link[0])
    	DoElement(np->link[0],DoElem);
    DoElem(np->element);
    if (np->link[1])
    	DoElement(np->link[1],DoElem);
    return;
}


/** �؂̂��ׂĂ̗v�f�ɂ��� func(void *) �����s.
 *  func�ɂ͗v�f�ւ̃|�C���^���n�����
 */
static void TREE_DoAll(TREE* tree, void (*func)(void*))
{
    DoElement(tree->root,func);
}




/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/

static void MTREE_Init(void);
static void MM_Init(void);
static void MTREE_Term(void);
static void M_ClearArg(void);
static int  Filn_Open0(char const *name, int md);


/** ������
 */
filn_t	*Filn_Init(void)
{
    if (Filn == NULL)
    	Filn = calloc(1, sizeof(filn_t));
    if (Filn == NULL)
    	return NULL;

    filn_local = calloc(1, sizeof(filn_local_t));
    if (filn_local == NULL)
    	return NULL;

    V.dir   	    = NULL; 	/* include ���Ɍ�������f�B���N�g���ꗗ */
    V.errFp 	    = STDERR;	/* �G���[�̏o�͐�   	    	    	*/
    V.opt_delspc    = 0;    	/* 0�ȊO�Ȃ�΋󔒂̈��k������	    	*/
    V.opt_dellf     = 1;    	/* 0�ȊO�Ȃ�΁����s�ɂ��s�A�����s��	*/
    V.opt_sscomment = 1;    	/* 0�ȊO�Ȃ��//�R�����g���폜����  	*/
    V.opt_blkcomment= 1;    	/* 0�ȊO�Ȃ�΁^���R�����g���^���폜����*/
    V.opt_kanji     = 1;    	/* 0�ȊO�Ȃ��MS�S�p�ɑΉ�  	    	*/
    V.opt_sq_mode   = 1;    	/* ' �� �y�A�ŕ����萔�Ƃ��Ĉ��� */
    V.opt_wq_mode   = 1;    	/* " �� �y�A�ŕ�����萔�Ƃ��Ĉ��� */
    V.opt_mot_doll  = 0;    	/* $ �� ���g���[���� 16�i���萔�J�n�����Ƃ��Ĉ��� */
    V.opt_oct	    = 1;    	/* 1: 0����n�܂鐔���͂W�i��  0:10�i */

    V.opt_orgSrc    	= 0;	    /* 1:���̃\�[�X���R�����g�ɂ��ďo�� 2:TAG JUMP�`�� 3:#line file  0:�o�͂��Ȃ� */

    V.orgSrcPre     = ";";  	/* ���\�[�X�o�͎��̍s���ɂ��镶����	*/
    V.orgSrcPost    = "";   	/* ���\�[�X�o�͎��̍s���ɂ��镶����	*/
    V.immMode	    = 0;    	/* 1:�����t10�i 2:������10�i 3:0xFF 4:$FF X(5:0FFh) */
    V.cmt_chr[0]    = 0;    	/* �R�����g�J�n�����ɂȂ镶�� */
    V.cmt_chr[1]    = 0;    	/* �R�����g�J�n�����ɂȂ镶�� */
    V.cmt_chrTop[0] = 0;    	/* �s���R�����g�J�n�����ɂȂ镶�� */
    V.cmt_chrTop[1] = 0;    	/* �s���R�����g�J�n�����ɂȂ镶�� */
    V.macErrFlg     = 1;    	/* �}�N�����̃G���[�s�ԍ����\�� 1:���� 0:���Ȃ� */
    V.mac_chr	    = '#';  	/* �}�N���s�J�n���� */
    V.mac_chr2	    = '#';  	/* �}�N���̓���W�J�w�蕶��.  */
    V.localPrefix   = "_LCL_";
    V.opt_yen	    = 1;    	/* \\������C�̂悤�Ɉ���Ȃ�. 1:���� 2:'"���̂�  3,4:�ϊ��������Ⴄ(����) */

    V.sym_chr_doll  = '$';
    V.sym_chr_atmk  = '@';
    V.sym_chr_qa    = '?';
    V.sym_chr_shp   = 0/*'#'*/;
    V.sym_chr_prd   = 0/*'.'*/;

    V.getsAddSiz    = 16;

    memset(Z.inclStk, 0, sizeof(Z.inclStk));
    Z.inclNo = -1;
    Z.inclp = &Z.inclStk[0];	/* �Ƃ肠�����_�~�[�ŃZ�b�g */

    /*Z.pc3line = -1; */

    Z.errNum = 0;
    Z.warNum = 0;

    // ��ʂ̃o�b�t�@���m��
    Z.linbuf0	  = callocE(1, LINBUF0_SIZE+2);
    Z.linbuf	  = callocE(1, LINBUF_SIZE+2);
    Z.M_str 	  = Z.linbuf0;
    Z.M_mbuf_size = M_MBUF_SIZE;
    Z.M_mbuf	  = callocE(1, Z.M_mbuf_size);
    Z.M_mbuf_end  = Z.M_mbuf + Z.M_mbuf_size;

    // ���O�Ǘ��̖؂̏�����
    MTREE_Init();
    MM_Init();
    return Filn;
}


/** �I������.
 */
void Filn_Term(void)
{
    int     	i;
    SRCLIST*	sl;
    SRCLIST*	sn;

    if (Filn == NULL)
    	return;

    SAFE_FREE(Z.linbuf0);
    SAFE_FREE(Z.linbuf);
    SAFE_FREE(Z.M_mbuf);
    for (i = 0; i < FILN_INCL_NUM; i++) {
    	if (Z.inclStk[i].fp) {
    	    fclose(Z.inclStk[i].fp);
    	    Z.inclStk[i].fp = NULL;
    	    SAFE_FREE(Z.inclStk[i].name);
    	}
    }
    M_ClearArg();
    for (sl = Z.sl_lst; sl; sl = sn) {
    	sn = sl->link;
    	SAFE_FREE(sl->s);
    	SAFE_FREE(sl);
    }
    Filn_ErrClose();
    MTREE_Term();
    SAFE_FREE(filn_local);
    SAFE_FREE(Filn);
}



static int  Filn_Close(void)
{
    if (Z.inclNo < 0) {
    	Filn_Exit("PRGERR: too many 'Close'\n");
    }
    /*printf("%d %lx %s %d %lx ]\n",Z.inclNo, Z.inclp, Z.inclp->name, Z.inclp->line, Z.inclp->fp);*/
    fclose(Z.inclp->fp);
    SAFE_FREE(Z.inclp->name);
    memset(Z.inclp, 0, sizeof(*Z.inclp));
    --Z.inclNo;
    if (Z.inclNo < 0)
    	return -1;
    Z.inclp = &Z.inclStk[Z.inclNo];
    return 0;
}


#if 1
void Filn_AddIncDir(char const* dirname)
{
    char const* p;
    char const* s = dirname;
    char*   	m;
    FILN_DIRS*	sl;
    FILN_DIRS*	btm = NULL;
    ptrdiff_t	l;

    if (s == NULL)
    	return;
    btm = V.dir;
    if (btm) {
    	while (btm->link != NULL) {
    	    btm     = btm->link;
    	}
    }
    for (;;) {
    	p = strchr(s, ';');
    	if (p == NULL)
    	    p = s + strlen(s);
    	l = p - s;
    	m = (char*)mallocE(l+1);
    	memcpy(m, s, l);
    	m[l] = 0;
    	sl = (FILN_DIRS*)mallocE(sizeof(*sl));
    	sl->s = m;
    	sl->link = NULL;
    	if (V.dir == NULL)
    	    V.dir = sl;
    	if (btm) {
    	    btm->link = sl;
    	}
    	btm = sl;
    	s = p;
    	if (*s == 0 || s[1] == 0)
    	    break;
    	s++;
    }
  #if 0
    for(sl = V.dir; sl; sl = sl->link) {
    	printf("%s\n", sl->s);
    }
  #endif
}

#else

void Filn_AddIncDir(char* dirname)
{
    FILN_DIRS* p;

    p = V.dir;
    if (p == NULL) {
    	p   	= callocE(1, sizeof(FILN_DIRS));
    	p->s	= strdupE(dirname);
    	V.dir	= p;
    } else {
    	while (p->link != NULL) {
    	    p	= p->link;
    	}
    	p->link = callocE(1, sizeof(FILN_DIRS));
    	p   	= p->link;
    	p->s	= strdupE(dirname);
    }
}

#endif


int  Filn_Open(char const* name)
{
    return Filn_Open0(name, 0);
}


/** md = 0 �Ȃ�A�J�����g���猟���Bmd!=0�Ȃ�A�J�����g�ȊO�������B
 */
static int  Filn_Open0(char const* name, int md)
{
    FILE*   	fp = NULL;
    char    	fnam[FILN_FNAME_SIZE];
    char*   	p;
    char const* d;
    FILN_DIRS const*	dl;

    Z.mac_chrs[0]  = V.mac_chr;
    Z.mac_chrs[1]  = 0;
    Z.mac_chrs2[0] = V.mac_chr2;
    Z.mac_chrs2[1] = 0;

    if (Z.inclNo >= FILN_INCL_NUM)
    	Filn_Exit("include�̃l�X�g���[������\n");

    if (name == NULL) {
    	strcpy(fnam, "");
    	fp = stdin;
    } else {
    	strcpy(fnam, name);
    	if (md == 0)
    	    fp = fopen(fnam, "rt");
    	if (fp == NULL) {
    	    for (dl = V.dir; dl; dl = dl->link) {
    	    	d = dl->s;
    	    	if (d && *d) {
    	    	    strcpy(fnam, d);
    	    	    p = STREND(fnam);
    	    	    if (p[-1] != '\\' && p[-1] != '/')
    	    	    	*p++ = '\\';
    	    	    strcpy(p, name);
    	    	    fp = fopen(fnam, "rt");
    	    	    if (fp != NULL)
    	    	    	break;
    	    	}
    	    }
    	  #if 0
    	    if (md && fp == NULL) {
    	    	strcpy(fnam, name);
    	    	fp = fopen(fnam, "rt");
    	    }
    	  #endif
    	    if (fp == NULL) {
    	    	Filn_Error("%s ���I�[�v���ł��Ȃ�����.\n", name);
    	    	return -1;
    	    }
    	}
    }
    ++Z.inclNo;
    Z.inclp = &Z.inclStk[Z.inclNo];
    Z.inclp->name = strdupE(fnam);
    Z.inclp->line = 0;
    /*Z.inclp->lcflg = 0;*/
    Z.inclp->fp = fp;
    return 0;
}


#if 0
void	Filn_PutsSrcLine(void)
{
    if (V.opt_orgSrc && V.opt_orgSrc == 3) {
      #if 1
    	char *p = callocE(1, 8 + 12 + strlen(Z.inclp->name));
    	sprintf(p, "# %u \"%s\"\n", Z.inclp->line, Z.inclp->name);
    	SRCLIST_Add(&Z.sl_lst, p);
      #else
    	if (Z.errMacFnm && Z.errMacLin > 0) {
    	    if (Z.errMacLin+1 == Z.pc3line)
    	    	Z.pc3line--;
    	    if (Z.errMacLin != Z.pc3line || strcmp(Z.errMacFnm, Z.pc3name))
    	    	fprintf(V.wrtFp, "# %u \"%s\"\n", Z.errMacLin, Z.errMacFnm);
    	    Z.pc3line = Z.errMacLin;
    	    strncpyZ(Z.pc3name, Z.errMacFnm, sizeof Z.pc3name);
    	} else {
    	    if (Z.inclp->line != Z.pc3line || strcmp(Z.inclp->name, Z.pc3name))
    	    	fprintf(V.wrtFp, "# %u \"%s\"\n", Z.inclp->line, Z.inclp->name);
    	    Z.pc3line = Z.inclp->line;
    	    strncpyZ(Z.pc3name, Z.inclp->name, sizeof Z.pc3name);
    	}
    	Z.pc3line++;
      #endif
    }
}
#endif


/** ��s����͂���B�s���̉��s�R�[�h�͎�菜��
 */
char *Filn_GetStr(char* buf, size_t len)
{
    char*   p;
    size_t  i;
    int     c;

    Z.inclp->line++;
    --len;
    p = buf;
    buf[0] = 0;
    i = 0;
    for (;;) {
     /* J1: */
    	if (i == len) {
    	    /*Z.inclp->lcflg = 1;*/
    	    Filn_Error("1�s����������\n");
    	    break;
    	}
    	c = fgetc(Z.inclp->fp);
    	if (feof(Z.inclp->fp)) {
    	  #if 0
    	 // if (Filn_Close()) {
    	 // 	*p = 0;
    	 // 	return NULL;
    	 // }
    	 // goto J1;
    	  #else
    	    if (i > 0) {
    	    	break;
    	    }
    	    return NULL;
    	  #endif
    	}
    	if (ferror(Z.inclp->fp)) {
    	    Filn_Exit("���[�h�G���[���N���܂���.\n");
    	}
    	if (c == '\0') {
    	    Filn_Error("�s���� '\\0' ���������Ă���\n");
    	    c = ' ';
    	}
    	if (c == '\n')
    	    break;
    	*p++ = c;
    	++i;
    }
    *p = 0;
    if (V.opt_orgSrc) {
    	char *s;
    	if (V.opt_orgSrc == 4) {
    	    s = callocE(1, 8 + strlen(V.orgSrcPre) + strlen(Z.inclp->name) + 12 + strlen(V.orgSrcPost) + 4 + V.getsAddSiz);
    	    sprintf(s, "%s %s %u %s\n", V.orgSrcPre, Z.inclp->name, Z.inclp->line, V.orgSrcPost);
    	} else if (V.opt_orgSrc == 3) {
    	    s = callocE(1, 8 + 12 + strlen(Z.inclp->name) + 4 + V.getsAddSiz);
    	    sprintf(s, "# %u \"%s\"\n", Z.inclp->line, Z.inclp->name);
    	} else if (V.opt_orgSrc == 2) {
    	    s = callocE(1, 8 + strlen(V.orgSrcPre) + strlen(Z.inclp->name) + 12 + strlen(buf) + strlen(V.orgSrcPost) + 4 + V.getsAddSiz);
    	    sprintf(s, "%s %-13s %5u : %s %s\n",   V.orgSrcPre, Z.inclp->name, Z.inclp->line, buf, V.orgSrcPost);
    	} else {
    	    s = callocE(1, 4 + strlen(V.orgSrcPre) + strlen(Z.inclp->name) + strlen(buf) + strlen(V.orgSrcPost) + 4 + V.getsAddSiz);
    	    sprintf(s, "%s %s %s\n", V.orgSrcPre, buf, V.orgSrcPost);
    	}
    	SRCLIST_Add(&Z.sl_lst, s);
    }
    return buf;
}



/*-----------------------------------------------------------*/


static void InitStMbuf(void)
{
    Z.M_mptr	= Z.M_mbuf;
    Z.M_mptr[0] = 0;
}

static int StMbuf(char const* s)
{
    size_t l;

    l = strlen(s);
    if (Z.M_mptr + l >= Z.M_mbuf_end) {
    	char*	p;
    	// �Ƃ肠�����A�T�C�Y�g���������݂�.
    	Z.M_mbuf_size += 0x1000;    	    	    // �T�C�Y���g��
    	p = reallocE(Z.M_mbuf, Z.M_mbuf_size);	    // �������Ċm��
    	if (p == Z.M_mbuf) {	    	    	    // �A�h���X�������Ȃ�A�떂������
    	    Z.M_mptr = p + (Z.M_mptr - Z.M_mbuf);   // �|�C���^�Đ���
    	    Z.M_mbuf = p;
    	    Z.M_mbuf_end = Z.M_mbuf + Z.M_mbuf_size;
    	} else {    	    	    	    	    // �A�h���X���ς��Ƃ�΂��̂ŁA������߂�
    	    Filn_Exit("���͂Łi�s�A�����}�N���W�J���Łj�s�o�b�t�@�����ӂ�܂���\n");
    	}
    	// �� ��ŁAMM_Macc() �� Z.M_mptr ���ꎞ�ޔ�����Ƃ��A�|�C���^�łȂ��I�t�Z�b�g�l�ōs���悤�ɕύX���邱�Ɓ�����
    }
    if (l)
    	memcpy(Z.M_mptr, s, l);
    Z.M_mptr += l;
    *Z.M_mptr = 0;
    return 0;
}


static ptrdiff_t GetStMbufOfs(void)
{
    return Z.M_mptr - Z.M_mbuf;
}


#if 0
typedef struct StD_stk_t {
    char *buf;
    char *p;
    char *bufend;
} StD_stk_t;

StD_stk_t StD_stk[STD_STK_MAX];

static void StD_stat_mark(void)
{
}
#endif




/*--------------------------------------------------------------------------*/
#define ISSPACE(c)  	( ((UCHAR)(c) <= 0x20 && (c) != '\n' && (c) != 0) || (c == 0x7f) /*|| ((UCHAR)(c) >= 0xfd)*/)
#define TOEOS(s)    	do{ do {c = *s++;} while (c != '\n' && c != 0);--s;}while(0)
#define ER()	    	if (d >= endadr) goto ERR;
#define StC(d,c)    	(((d) > Z.linbuf+LINBUF_SIZE) ? Filn_Exit("�������ꂽ�P�s�����߂���\n") : 0, *d++ = (c) )
#define GetC(d)     	(*(unsigned char*)((d)++))


static int IsSymKigo(int c)
{
    if (c == 0)
    	return 0;
    if (    c == '_'
    	||  c == V.sym_chr_doll
    	||  c == V.sym_chr_atmk
    	||  c == V.sym_chr_qa
    	||  c == V.sym_chr_shp
    	||  c == V.sym_chr_prd
    ){
    	return 1;
    }
    return 0;
}


static char *SkipSpc(char const* s)
{
    int c;

    do {
    	c = *(const UCHAR*)s;
    	s++;
    } while (ISSPACE(c));
    --s;
    return (char*)s;
}


static unsigned M_GetEscChr(char const** s0)
{
    char const* s;
    unsigned	c,k,l,i;

    s = *s0;
    c = *s++;
    l = 8;
    switch (c) {
    case 'a':	c = '\a';   break;
    case 'b':	c = '\b';   break;
    case 'e':	c = 0x1b;   break;
    case 'f':	c = '\f';   break;
    case 'n':	c = '\n';   break;
    case 'N':	c = 0x0d0a; break;
    case 'r':	c = '\r';   break;
    case 't':	c = '\t';   break;
    case 'v':	c = '\v';   break;
    case '\\':	c = '\\';   break;
    case '\'':	c = '\'';   break;
    case '\"':	c = '\"';   break;
    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7':
    	l = 3;
    	c -= '0';
    	for (i = 0; i < l; i++) {
    	    k = *s;
    	    if (k >= '0' && k <= '7')
    	    	c = c * 8 + k-'0';
    	    else
    	    	break;
    	    s++;
    	}
    	break;
    case 'x':
    	l = 2;
    case 'X':
    	c = 0;
    	for (i = 0; i < l; i++) {
    	    k = *s;
    	    if (isdigit(k))
    	    	c = c * 16 + k-'0';
    	    else if (k >= 'A' && k <= 'F')
    	    	c = c * 16 + 10+k-'A';
    	    else if (k >= 'a' && k <= 'f')
    	    	c = c * 16 + 10+k-'a';
    	    /*else if (k == '_')
    	    	;*/
    	    else
    	    	break;
    	    s++;
    	}
    	break;
    case 0:
    	--s;
    	break;
    default:
    	if ((/*c != '\t' &&*/ c < 0x20) || c == 0x7f || c >= 0xFD) {
    	    Filn_Error("\\�̒���ɃR���g���[���R�[�h������\n");
    	    /*s[-1] = ' ';*/
    	}
    }
    *s0 = s;
    return c;
}


/** 1�s����. �R�����g�폜�A�󔒈��k�A�s�A�������s��
 */
static char *Filn_GetLine(void)
{
    int     	macFlg = 0;
    int     	c;
    unsigned	l;
    unsigned	j;
    char const* s;
    char*   	d;

    Z.inclp->line = Z.inclp->linCnt;
    Z.linbuf[0] = 0;
    d = Z.linbuf;

  LOOP:
    /*Z.inclp->lcflg = 0;*/
    Z.inclp->linCnt++;
    if (Filn_GetStr(Z.linbuf0, LINBUF0_SIZE) == NULL) {
    	Z.linbuf[0] = 0;
    	Z.linbuf0[0] = 0;
    	return NULL;
    }
    /*if (Z.inclp->lcflg) Z.inclp->linCnt--;*/

    /* �s���R�����g�̃`�F�b�N */
    if (V.cmt_chrTop[0]) {
    	s = SkipSpc(Z.linbuf0);
    	if (*s == V.cmt_chrTop[0]) {
    	    s++;
    	    goto EOS;
    	}
    }
    if (V.cmt_chrTop[1]) {
    	s = SkipSpc(Z.linbuf0);
    	if (*s == V.cmt_chrTop[1]) {
    	    s++;
    	    goto EOS;
    	}
    }

    if (V.mac_chr && V.mac_chr == *SkipSpc(Z.linbuf0))
    	macFlg = 1;

    s = Z.linbuf0;
    for (; ;) {
    	c = GetC(s);	//c = *s++;
    	if (c == 0) {	    	    	    	    	    /* ���s */
      EOS:
    	    if (V.opt_dellf) {
    	    	if (V.opt_delspc) { /* 0�ȊO�Ȃ�΋󔒂̈��k������  	    */
    	    	    while (d > Z.linbuf) {
    	    	    	c = d[-1];
    	    	    	if (c == 0 || c > 0x20)
    	    	    	    break;
    	    	    	--d;
    	    	    }
    	    	}
    	    	if (d > Z.linbuf && d[-1] == '\\' && V.opt_yen) {
    	    	    --d;
    	    	    StC(d,' ');
    	    	    goto LOOP;
    	    	}
    	    }
    	    break;

    	} else if (FILN_MBC_IS_LEAD(c)) {   	    	    	    /* �S�p */
    	    --s;
    	    l = mbs_len1(s);
    	    for (j = 0; j < l; ++j) {
    	    	c = GetC(s);
    	    	StC(d, c);
    	    }

    	} else if (c == '\'') {     	    /* '������'�̏��� */
    	    if (V.opt_sq_mode == 1) /* �y�A�ň����Ƃ� */
    	    	goto A1;
    	    /* �����萔������, 1�̂� (6809��) �̂Ƃ� */
    	    StC(d,c);

    	} else if (c == '"' && V.opt_wq_mode) {     	    /* "������"�̏��� */
    	    int k;
    	  A1:
    	    k = c;
    	    StC(d,c);
    	    for (; ;) {
    	    	c = GetC(s);	//c = *s++;
    	    	if (c == 0) {
    	    	    if (macFlg == 0)
    	    	    	Filn_Error("%s�̏I����%c���Ȃ�\n", (k=='"')?"\"������\"" : "'����'",k);
    	    	    /*StC(d,k);*/
    	    	    --s;
    	    	    break;

    	    	} else if (c == k) {
    	    	    StC(d, c);
    	    	    break;

    	    	} else if (c == '\\' && V.opt_yen) {
    	    	    StC(d,c);
    	    	    c = GetC(s);    //c = *s++;
    	    	    if (c == '\0') {
    	    	    	Filn_Error("%s���ł�\\\\n�ɂ��s�A���͂ł��Ȃ�\n",(k=='"')?"\"������\"" : "'����'");
    	    	    	/*--d;*/
    	    	    	/*StC(d,'"');*/
    	    	    	--s;
    	    	    	break;
    	    	    }
    	    	    if (FILN_MBC_IS_LEAD(c)) {
    	    	    	Filn_Warnning("\\�̒���ɑS�p����������\n");
    	    	    	goto J1;
    	    	    }
    	    	    if (V.opt_yen >= 3) {
    	    	    	d--;
    	    	    	s--;
    	    	    	c = M_GetEscChr((char const**)&s);
    	    	    	if (c >= 0x1000000)
    	    	    	    StC(d, (UCHAR)(c>>24));
    	    	    	if (c >= 0x10000)
    	    	    	    StC(d, (UCHAR)(c>>16));
    	    	    	if (c >= 0x100)
    	    	    	    StC(d, (UCHAR)(c>>8));
    	    	    	c = (UCHAR)c; /*StC(d, (UCHAR)c);*/
    	    	    }
    	    	    StC(d,c);

    	    	} else if (FILN_MBC_IS_LEAD(c)) {
    	      J1:
    	    	    --s;
    	    	    l = mbs_len1(s);
    	    	    for (j = 0; j < l; ++j) {
    	    	    	c = GetC(s);
    	    	    	StC(d, c);
    	    	    }
    	    	} else {
    	    	    StC(d,c);
    	    	}
    	    }

    	} else if (ISSPACE(c)||c == 0xFF) { 	    	    /* �� */
    	    if (V.opt_delspc) {
    	    	s = SkipSpc(s);
    	    	StC(d, ' ');
    	    } else {
    	    	StC(d, c);
    	    }

    	} else if (c == '/') {
    	    c = GetC(s);    //c = *s++;
    	    if (c == '/' && V.opt_sscomment) {	    	/* // �R�����g */
    	    	goto EOS;

    	    } else if (c == '*' && V.opt_blkcomment) {	/* �^�� �R�����g ���^ */
    	    	StC(d, ' ');
    	    	for (; ;) {
    	    	    c = GetC(s);    //c = *s++;
    	    	    if (c == '*' && *s == '/') {
    	    	    	s = SkipSpc(s+1);
    	    	    	break;
    	    	    } else if (c == '\0') {
    	    	    	/*Z.inclp->lcflg = 0;*/
    	    	    	Z.inclp->linCnt++;
    	    	    	if (Filn_GetStr(Z.linbuf0, LINBUF0_SIZE) == NULL)
    	    	    	    Filn_Exit("/*�R�����g*/�̓r����EOF�����ꂽ\n");
    	    	    	/*if (Z.inclp->lcflg) Z.inclp->linCnt--;*/
    	    	    	s = Z.linbuf0;
    	    	    }
    	    	}
    	    } else {
    	    	--s;
    	    	StC(d,'/');
    	    }
    	} else if (V.cmt_chr[0] && c == V.cmt_chr[0]) {
    	    goto EOS;
    	} else if (V.cmt_chr[1] && c == V.cmt_chr[1]) {
    	    goto EOS;
    	} else {
    	    StC(d,c);
    	}
    }
    StC(d,'\0');
    return Z.linbuf;
}



/*--------------------------------------------------------------------------*/
/* #�v���v���Z�X����	    	    	    	    	    	    	    */
/*--------------------------------------------------------------------------*/

typedef struct MTREE_T {
    char const* name;	    	/* ��`��   	    	    	    */
    int     	atr;	    	/* ���O�̈����̑��� 0:�폜 1:set 2:define 3:macro */
    val_t   	argb;	    	/* �����̐� or #set�萔     	    */
    int     	argc;	    	/* �����̐�+���[�J�����̐�  	    */
    char**  	argv;	    	/* �����ꗗ(+#local)	    	    */
    char*   	buf;	    	/* #define,#macro�o�b�t�@   	    */
    char*   	fname;	    	/* ��`�̂������t�@�C����   	    */
    int     	line;	    	/* �s�ԍ�   	    	    	    */
} MTREE_T;


enum {M_ATR_0=0, M_ATR_RSV, M_ATR_SET, M_ATR_DEF, M_ATR_MAC};
enum {M_RSV_FILE=1, M_RSV_LINE, M_RSV_DATE, M_RSV_TIME};

/** TREE ���[�`���ŁA�V�����v�f�𑢂�Ƃ��ɌĂ΂��
 */
static void*	MTREE_New(MTREE_T const* t)
{
    MTREE_T*	p;
    p = callocE(1,sizeof(MTREE_T));
    memcpy(p, t, sizeof(MTREE_T));
    p->name  = strdupE(t->name);
    if (t->buf == 0 || t->buf == '\0')
    	p->buf = calloc(1,1);
    else
    	p->buf = strdupE(t->buf);
    return p;
}

static void M_FreeArg(int* pArgc, char*** pArgv);

/** TREE ���[�`���ŁA�������J���̂Ƃ��ɌĂ΂��
 */
static void MTREE_Del(void* ff)
{
    MTREE_T*	p = (MTREE_T*)ff;
    SAFE_FREE(*(char**)&p->name);
    SAFE_FREE(p->buf);
    M_FreeArg(&p->argc, &p->argv);
    SAFE_FREE(ff);
}


/** TREE ���[�`���ŁA�p�������r����
 */
static int  MTREE_Cmp(MTREE_T const* f1, MTREE_T const* f2)
{
    return strcmp(f1->name, f2->name);
}


/** TREE ��������
 */
static void 	MTREE_Init(void)
{
    Z.mtree = TREE_Make((TREE_NEW)MTREE_New, (TREE_DEL)MTREE_Del, (TREE_CMP)MTREE_Cmp, (TREE_MALLOC)mallocE, (TREE_FREE)freeE);
}

/** TREE ���J��
 */
static void MTREE_Term(void)
{
    TREE_Clear(Z.mtree);
}


/** ���݂̖��O���؂ɓo�^���ꂽ���x�����ǂ����T��
 */
static MTREE_T* MTREE_Search(char const* lbl_name)
{
    MTREE_T 	t;
    MTREE_T*	p;

    memset (&t, 0, sizeof(MTREE_T));
    t.name = lbl_name;
    p = TREE_Search(Z.mtree, &t);
    if (p && p->atr == M_ATR_0)
    	p = NULL;
    return p;
}


/** ���x��(���O)��؂ɓo�^����
 */
static MTREE_T	*MTREE_Add(char const* name, int atr, val_t argb, int argc, char** argv, char* buf, int md)
{
    MTREE_T 	t;
    MTREE_T*	p;

  #if 0
    DB {
    	int i;
    	printf("[MTREE_Add] %s, %d, %d, %d\n", name, atr, argb, argc);
    	for (i = 0; i < argc; i++) {
    	    printf("\t[%d]%s\n", i,argv[i]);
    	}
    	if (buf)
    	    printf("\t%s\n",buf);
    	else
    	    printf("\t(null)\n");
    }
  #endif
    memset (&t, 0, sizeof(MTREE_T));
    t.name  = name;
    t.atr   = atr;
    t.argb  = argb;
    t.argc  = argc;
    t.argv  = argv;
    t.buf   = buf;
    t.fname = Z.inclp->name;
    t.line  = Z.inclp->line;
    p = TREE_Insert(Z.mtree, &t);
    if (p && ((TREE*)Z.mtree)->flag == 0) {
    	if (p->atr == M_ATR_0 || (p->atr == M_ATR_SET && atr == M_ATR_SET)) {
    	  J1:
    	    p->atr   = atr;
    	    p->argb  = argb;
    	    p->argc  = argc;
    	    p->argv  = argv;
    	    p->fname = Z.inclp->name;
    	    p->line  = Z.inclp->line;
    	} else {
    	    /*if (p != ((MTREE_T*)Z.macBgnStr))*/
    	    if (md == 0)
    	    	Filn_Error("��`�ς݂�#�}�N��(%s)���Ē�`���悤�Ƃ���\n", p->name);
    	    if ((p->atr == M_ATR_DEF || p->atr == M_ATR_MAC) && (atr == M_ATR_DEF || atr == M_ATR_MAC)) {
    	    	SAFE_FREE(p->buf);
    	     #if 1
    	    	M_FreeArg(&p->argc, &p->argv);
    	     #else
    	    	if (p->argv) {
    	    	    int i;
    	    	    for (i = 0; i < p->argc; i++)
    	    	    	SAFE_FREE(p->argv[i]);
    	    	    SAFE_FREE(p->argv);
    	    	}
    	     #endif
    	    	p->atr = M_ATR_0;
    	    	goto J1;
    	    }
    	}
    }

  #if 0
    {
    	int i;
    	printf("%-12s %5d : [%s] (%d)", p->fname, p->line, p->name, p->atr);
    	if (p->atr == M_ATR_SET) {
    	    printf("\t%ld\n",p->argb);
    	} else if (p->atr) {
    	    printf("\t%d,%d\n",p->argb,p->argc);
    	    for (i = 0; i < p->argc; i++)
    	    	printf("\tA\t%s\n",p->argv[i]);
    	    printf("{\n");
    	    printf("%s\n",p->buf);
    	    printf("}\n");
    	} else {
    	    printf("\n");
    	}
    }
  #endif
    return p;
}



/*--------------------------------------------------------------------------*/
/* #����    	    	    	    	    	    	    	    	    */

#define M_ODRNUM    (sizeof(M_odrs)/sizeof(M_odrs[0]))

enum {
    M_MAC_E,	M_MAC_S,    M_ARRAY,	M_BEGIN,    M_DEFINE,
    M_DEFINED,	M_ELIF,     M_ELSE, 	M_END,	    M_ENDFOR,
    M_ENDIF,	M_ENDIPR,   M_ENDMACRO, M_ENDREPT,  M_ENUM,
    M_ERROR,	M_EXITM,    M_FILE, 	M_FOR,	    M_IF,
    M_IFDEF,	M_IFEQ,     M_IFGE, 	M_IFGT,     M_IFLE,
    M_IFLT, 	M_IFNDEF,   /*M_IFNE,*/ M_INCLUDE,  M_IPR,
    M_LINE, 	M_LOCAL,    M_MACRO,	M_PRAGMA,   M_PRINT,
    M_REPT, 	M_SELF,     M_SET,  	M_UNDEF,
};

static int const M_odrsNo[] = {
    -1,
    M_MAC_E,	M_MAC_S,    M_ARRAY,	M_BEGIN,    M_DEFINE,
    M_DEFINED,	M_ELIF,     M_ELSE, 	M_END,	    M_ENDFOR,
    M_ENDIF,	M_ENDIPR,   M_ENDMACRO, M_ENDMACRO, M_ENDREPT,
    M_ENDREPT,	M_ENUM,     M_ERROR,	M_EXITM,    M_EXITM,
    M_FILE, 	M_FOR,	    M_IF,   	M_IFDEF,    M_IFEQ,
    M_IFGE, 	M_IFGT,     M_IFLE, 	M_IFLT,     M_IFNDEF,
    M_IF/*NE*/, M_INCLUDE,  M_IPR,  	M_LINE,     M_LOCAL,
    M_MACRO,	M_PRAGMA,   M_PRINT,	M_REPT,     /*M_SELF,*/
    M_SET,  	M_UNDEF,
};

static char const* const M_odrs[] = {
    "_E\xFF_",	"_S\xFF_",  "array",	"begin",    "define",
    "defined",	"elif",     "else", 	"end",	    "endfor",
    "endif",	"endipr",   "endm", 	"endmacro", "endr",
    "endrept",	"enum",     "error",	"exitm",    "exitmacro",
    "file", 	"for",	    "if",   	"ifdef",    "ifeq",
    "ifge", 	"ifgt",     "ifle", 	"iflt",     "ifndef",
    "ifne", 	"include",  "ipr",  	"line",     "local",
    "macro",	"pragma",   "print",	"rept",     /*"self",*/
    "set",  	"undef",
};


/**
 *  key:������������ւ̃|�C���^
 *  tbl:������ւ̃|�C���^�������߂��z��
 *  nn:�z��̃T�C�Y
 *  ���A�l:��������������̔ԍ�(0���)  �݂���Ȃ������Ƃ�-1
 */
 static int stblSearch(char const* const tbl[], int nn, char const* key)
{
    int     low, mid, f;

    low = 0;
    while (low < nn) {
    	mid = (low + nn - 1) / 2;
    	f = strcmp(key, tbl[mid]);
    	if (f < 0)
    	    nn = mid;
    	else if (f > 0)
    	    low = mid + 1;
    	else
    	    return mid;
    }
    return -1;
}


static int M_OdrSearch(char const* name)
{
    int o;

    o = stblSearch(M_odrs, M_ODRNUM, name);
    o = M_odrsNo[o+1];
    return o;
}



/*--------------------------------------------------------------------------*/

static char *M_ImmStr(char* str, val_t l, int typ)
{
    switch(V.immMode) {
    default:
    	Filn_Error("PrgErr:M_ImmStr\n");
    case 0:
    case 1:
    	if (typ == 1) {
    	    if (l < -128 || l > 255)
    	    	l &= 0xff;
    	} else if (typ == 2) {
    	    if (l < -0x8000L || l > 0xFFFFL)
    	    	l &= 0xffff;
    	}
    	sprintf(str,(l>=0)?S_FMT_D:"(" S_FMT_D ")",l);
    	break;
    case 2:
    	l = (typ == 1) ? (l & 0xff) : (typ == 2) ? (l & 0xFFFF) : l;
    	sprintf(str, S_FMT_U, l);
    	break;
    case 3:
    	l = (typ == 1) ? (l & 0xff) : (typ == 2) ? (l & 0xFFFF) : l;
    	sprintf(str, "0x" S_FMT_X, l);
    	break;
    case 4:
    	l = (typ == 1) ? (l & 0xff) : (typ == 2) ? (l & 0xFFFF) : l;
    	sprintf(str, "$" S_FMT_X, l);
    	break;
  #if 0
    case 5: sprintf(str, "0%lXh", l);	break;
    case 6: sprintf(str, "%s%lx", Filn_mac_immPrefix, l);   break;
  #endif
    }
    return str;
}

static char const* M_GetSymLabel(int c, char const* s, char const* p)
{
    unsigned	l;
    unsigned	k;
    char*   	t;

    t  = Z.M_name;
    *t = 0;
    l  = sizeof(Z.M_name) - 1;
    for (; ;) {
    	if (isalpha(c) || isdigit(c) || IsSymKigo(c)||c == 0xff) {  /* 0xff�͓����ł̓��ʏ����p */
    	    if (l) {
    	    	*t++ = c;
    	    	l--;
    	    }
    	} else if (FILN_MBC_IS_LEAD(c)) {
    	    --s;
    	    k = mbs_len1(s);
    	    if (k < 2) {
    	    	Filn_Error("�S�p�����Q�o�C�g��(�ȍ~)����������(%02x:%02x)\n",c,*s);
    	    } else if (l >= k) {
    	    	unsigned j;
    	    	l -= k;
    	    	for (j = 0; j < k; ++j)
    	    	    *t++ = *s++;
    	    } else {
    	    	l = 0;
    	    }
    	} else {
    	    break;
    	}
    	c = *(UCHAR*)s++;
    }
    *t = 0;
    --s;
    return s;
}


static char const* M_GetSymSqt(unsigned c, char const* s)
{
    while ((c = *s++) != '\'') {
    	if (FILN_MBC_IS_LEAD(c)) {
    	    unsigned l;
    	    --s;
    	    l = mbs_len1(s);
    	    if (l < 2) {
    	    	Filn_Warnning("'�ň͂܂ꂽ���ɕs���ȑS�p������\n");
    	    	goto E1;
    	    }
    	    c = mbs_getc(&s);
    	    if (c <= 0xffff && Z.M_val <= 0xFFFF)
    	    	Z.M_val = (Z.M_val << 16) | c;
    	    else
    	    	Z.M_val = c;
    	} else if (c == '\\' && (V.opt_yen == 1||V.opt_yen == 2)) {
    	    c = M_GetEscChr(&s);
    	    if (c > 0xFFFFFF)	Z.M_val = c;
    	    else if (c >0xFFFF) Z.M_val = (Z.M_val << 24) | c;
    	    else if (c > 255)	Z.M_val = (Z.M_val << 16) | c;
    	    else    	    	Z.M_val = (Z.M_val <<  8) | c;
    	} else if (c == 0) {
    	    /* Filn_Error("'�����Ă��Ȃ�\n"); */
    	    --s;
    	    break;
    	} else {
    	  #if 0
    	    if (/*(c != '\t' &&*/ c < 0x20) || c == 0x7f || c >= 0xFD) {
    	    	Filn_Error("'�ň͂܂ꂽ�͈͂ɃR���g���[���R�[�h�����ړ����Ă���\n");
    	    	/*s[-1] = ' ';*/
    	    }
    	  #endif
      E1:
    	    Z.M_val = Z.M_val * 256 + c;
    	}
    }
    return s;
}


static char const* M_GetSymWqt(unsigned c, char const* s)
{
    char*   p;
    *Z.M_str = 0;
    p = Z.M_str;
    *p++ = '"';
    for (;;) {
    	c = *s++;
    	if (c == 0) {
    	    /*Filn_Error("\"�����Ă��Ȃ�\n");*/
    	    /* *p++ = '"'; */
    	    --s;
    	    break;
    	}
    	if (c == '"') {
    	    *p++ = '"';
    	    break;
    	}
    	if (FILN_MBC_IS_LEAD(c)) {
    	    unsigned l;
    	    --s;
    	    l = mbs_len1(s);
    	    if (l < 2) {
    	    	Filn_Warnning("'�ň͂܂ꂽ���ɕs���ȑS�p������\n");
    	    	c = ' ';
    	    	*p++ = c;
    	    } else {
    	    	memcpy(p, s, l);
    	    	p += l;
    	    	s += l;
    	    }
    	} else if (c == '\\' && (V.opt_yen == 1||V.opt_yen == 2)) {
    	    *p++ = c;
    	    c = *s++;
    	    *p++ = c;
    	} else {
    	    *p++ = c;
    	}
      #if 0
    	if (/*(c != '\t' &&*/ c < 0x20) || c == 0x7f || c >= 0xFD) {
    	    Filn_Error("\"������\"���ɃR���g���[���R�[�h�����ړ����Ă���\n");
    	    c = ' ';
    	    *p++ = c;
    	}
      #endif
    }
    *p = 0;
    return s;
}


static char const* M_GetSymDigit(int c, char const* s)
{
    int     bf=0/*,of=0*//*,df=0*/;
    val_t   bv=0,ov=0,dv=0,xv=0;

    Z.M_val = 0;
    if (c == '0' && *s == 'x') {
    	for (; ;) {
    	    c = *++s;
    	    if (isdigit(c))
    	    	Z.M_val = Z.M_val * 16 + c-'0';
    	    else if (c >= 'A' && c <= 'F')
    	    	Z.M_val = Z.M_val * 16 + 10+c-'A';
    	    else if (c >= 'a' && c <= 'f')
    	    	Z.M_val = Z.M_val * 16 + 10+c-'a';
    	    else if (c == '_')
    	    	;
    	    else
    	    	break;
    	}
    } else if (c == '0' && *s == 'b') {
    	for (; ;) {
    	    c = *++s;
    	    if (c == '0' || c == '1')
    	    	Z.M_val = Z.M_val * 2 + c-'0';
    	    else if (c == '_')
    	    	;
    	    else
    	    	break;
    	}
    } else if (V.opt_oct && c == '0' && isdigit(*s)) {
    	/* �Ƃ肠�����A2,8,16�i�`�F�b�N */
    	for (; ;) {
    	    c = *s;
    	    if (c == '0' || c == '1') {
    	    	bv = bv*2 + (c-'0');
    	    	ov = ov*8 + (c-'0');
    	    	xv = xv*16+ (c-'0');
    	    } else if (c >= '0' && c <= '7') {
    	    	bf = 1;
    	    	ov = ov*8 + (c-'0');
    	    	xv = xv*16+ (c-'0');
    	    } else if (c == '8' || c == '9') {
    	    	bf = 1;
    	    	/*of = 1;*/
    	    	xv = xv*16+ (c-'0');
    	    } else if (c >= 'A' && c <= 'F') {
    	    	if (bf == 0 && c == 'B')
    	    	    bf = -1;
    	    	/*of = 1;*/
    	    	xv = xv*16+ (c-'A'+10);
    	    } else if (c >= 'a' && c <= 'f') {
    	    	if (bf == 0 && c == 'b')
    	    	    bf = -1;
    	    	/*of = 1;*/
    	    	xv = xv*16+ (c-'a'+10);
    	    } else if (c == '_') {
    	    	;
    	    } else {
    	    	break;
    	    }
    	    s++;
    	}
    	if (bf == 0 && (*s == 'B' || *s == 'b')) {
    	    bf = -1;
    	    s++;
    	}
    	if (bf < 0) {	/* 2�i�������� */
    	    Z.M_val = bv;
    	} else if (*s == 'H' || *s == 'h') {	/* 16�i�������� */
    	    Z.M_val = xv;
    	    s++;
    	} else /*if (of == 0)*/ {   /* 8�i�������� */
    	    Z.M_val = ov;
    	}
    } else {
    	/* �Ƃ肠�����A2,10,16�i�`�F�b�N */
    	--s;
    	for (; ;) {
    	    c = *s;
    	    if (c == '0' || c == '1') {
    	    	bv = bv*2 + (c-'0');
    	    	dv = dv*10 + (c-'0');
    	    	xv = xv*16+ (c-'0');
    	    } else if (c >= '0' && c <= '9') {
    	    	bf = 1;
    	    	dv = dv*10+ (c-'0');
    	    	xv = xv*16+ (c-'0');
    	    } else if (c >= 'A' && c <= 'F') {
    	    	if (bf == 0 && c == 'B')
    	    	    bf = -1;
    	    	/*df = 1;*/
    	    	xv = xv*16+ (c-'A'+10);
    	    } else if (c >= 'a' && c <= 'f') {
    	    	if (bf == 0 && c == 'b')
    	    	    bf = -1;
    	    	/*df = 1;*/
    	    	xv = xv*16+ (c-'a'+10);
    	    } else if (c == '_') {
    	    	;
    	    } else {
    	    	break;
    	    }
    	    s++;
    	}
    	if (bf == 0 && (*s == 'B' || *s == 'b')) {
    	    bf = -1;
    	    s++;
    	}
    	if (bf < 0) {	/* 2�i�������� */
    	    Z.M_val = bv;
    	} else if (*s == 'H' || *s == 'h') {	/* 16�i�������� */
    	    s++;
    	    Z.M_val = xv;
    	} else /*if (df == 0)*/ {   /* 10�i�������� */
    	    Z.M_val = dv;
    	}
    }
  #if 0
    while (*s == 'U' || *s == 'u' || *s == 'L' || *s == 'l')
    	s++;
  #else
    while (*s && (isalnum(*s) || *s == '_'))
    	s++;
  #endif
    return s;
}


static char const* M_GetSym0(char const* s)
{
    unsigned	c;
    char const* p;

    *Z.M_name = 0;
    *Z.M_str  = 0;
    Z.M_sym   = 0;

    p = s;
    c = *(UCHAR const*)s++;
    if (ISSPACE(c)) {
    	s = SkipSpc(s);
    	strncpyZ(Z.M_str, p, s-p+1);
    	Z.M_sym = ' ';
    	goto RET;
    }
    if (isalpha(c) || IsSymKigo(c) || FILN_MBC_IS_LEAD(c)) {
    	s = M_GetSymLabel(c, s, p);
    	strcpy(Z.M_str,Z.M_name);
    	Z.M_sym = 'A';
    	goto RET;
    }
    if (isdigit(c)) {
    	s = M_GetSymDigit(c,s);
    	Z.M_sym = '0';
    	strncpyZ(Z.M_str, p, s-p+1);
    	goto RET;
    }

    switch (c) {
    case '\'':
    	Z.M_val = 0;
    	if (V.opt_sq_mode == 0) {
    	    Z.M_sym = '\'';
    	} else {
    	    if (V.opt_sq_mode == 1) {
    	    	s = M_GetSymSqt(c, s);
    	    } else {
    	    	c = *s++;
    	    	Z.M_val = c;
    	    	if (c == 0)
    	    	    --s;
    	    }
    	    Z.M_sym = '0';
    	}
    	strncpyZ(Z.M_str, p, s-p+1);
    	/*memcpy(Z.M_str, p, s-p); Z.M_str[s-p] = 0;*/
    	break;

    case '"':
    	if (V.opt_wq_mode) {
    	    s = M_GetSymWqt(c, s);
    	} else {
    	    strncpyZ(Z.M_str, p, s-p+1);
    	}
    	Z.M_sym = '"';
    	break;

    case '\n':
    	Z.M_sym = '\n';
    	memcpy(Z.M_str, p, s-p); Z.M_str[s-p] = 0;
    	break;

    case '\0':
    	--s;
    	Z.M_sym = 0;
    	Z.M_str[0] = 0;
    	break;

    case '&':
    	if (*s == '&')	c += *s++ * 256;
    	goto SS;
    case '|':
    	if (*s == '|')	c += *s++ * 256;
    	goto SS;
    case '<':
    	if (*s == '<')	c += *s++ * 256;
    	else if (*s == '=') c += *s++ * 256;
    	goto SS;
    case '>':
    	if (*s == '>')	c += *s++ * 256;
    	/*else if (*s == '<')	c += *s++ * 256;*/  /*���ꂷ����̂Ŕp�~^^; */
    	else if (*s == '=') c += *s++ * 256;
    	goto SS;
    case '/':
    	if (*s == '/')	c += *s++ * 256;
    	else if (*s == '=') c += *s++ * 256;
    	goto SS;
    case '!':
    case '=':
    	if (*s == '=')	c += *s++ * 256;
    	goto SS;
    case '$':
    	if (V.opt_mot_doll) {
    	    Z.M_val = 0;
    	    for (; ;) {
    	    	c = *s++;
    	    	if (isdigit(c))
    	    	    Z.M_val = Z.M_val * 16 + c-'0';
    	    	else if (c >= 'A' && c <= 'F')
    	    	    Z.M_val = Z.M_val * 16 + 10+c-'A';
    	    	else if (c >= 'a' && c <= 'f')
    	    	    Z.M_val = Z.M_val * 16 + 10+c-'a';
    	    	else if (c == '_')
    	    	    ;
    	    	else
    	    	    break;
    	    }
    	    --s;
    	    c = '0';
    	    goto SS;
    	}
    case '+':
    case '-':
    case '*':
    case '%':
    case '^':
    case '~':
    case '(':
    case ')':
    case '[':
    case ']':
    case '{':
    case '}':
    case ',':
    case ';':
    case ':':

    case '#':
    case '?':
    case '@':
    case '.':

    default:
    	if (c == V.mac_chr) {
    	    if (*s == V.mac_chr)
    	    	c += *s++ * 256;
    	} else if (c == V.mac_chr2) {
    	    if (*s == V.mac_chr2)
    	    	c += *s++ * 256;
    	}
  SS:
    	Z.M_sym = c;
    	strncpyZ(Z.M_str, p, s-p+1);
    	/*memcpy(Z.M_str, p, s-p); Z.M_str[s-p] = 0;*/
    	break;
    }
  RET:
    return s;
}


static char const* M_GetSym(char const* s)
{
    if (ISSPACE(*s)) {
    	s = SkipSpc(s);
    }
    return M_GetSym0(s);
}



/*-------------------------------------------*/

static val_t M_ExprA(void);

static int M_ChkDefined(char const** s0)
{
    int     	    k= 0;
    int     	    c = 0;
    char    const*  s = *s0;

    s = M_GetSym(s);
    if (Z.M_sym == '(') {
    	k = 1;
    	s = M_GetSym(s);
    }
    if (Z.M_sym == 'A') {
    	c = (MTREE_Search(Z.M_name) != NULL) ? 1 : 0;
    }
    if (k) {
    	s = M_GetSym(s);
    	if (Z.M_sym != ')') {
    	    /*MM_MaccErr(fname, line);*/
    	    Filn_Error("defined()�̎w�肪��������!\n");
    	}
    }
    *s0 = s;
    return (c != 0);
}



static val_t M_Expr0(void)
{
    val_t n;

    Z.M_expr_str[0] = 0;
    Z.M_ep = M_GetSym(Z.M_ep);
    n = 0;

    if (Z.M_sym == 'A') {
    	MTREE_T *t;
    	if (strcmp(Z.M_name, "defined") == 0) { // #elif�̃o�O���}���u�B�{�������ł��ׂ��łȂ��c�c
    	    n = M_ChkDefined(&Z.M_ep);
    	} else if ((t = MTREE_Search(Z.M_name)) == NULL) {
    	    if (strcmp(Z.Expr0_nam, Z.M_name) == 0 && Z.Expr0_fnm == Z.inclp->name && Z.Expr0_line == Z.inclp->line)
    	    	;
    	    else if (Z.errLblSkip)
    	    	;
    	    else
    	    	Filn_Error("%c�s�Ŗ���`�̖��O %s ���Q�Ƃ��ꂽ\n", V.mac_chr, Z.M_name);
    	    Z.Expr0_fnm  = Z.inclp->name;
    	    Z.Expr0_line = Z.inclp->line;
    	    strcpy(Z.Expr0_nam, Z.M_name);
    	    n = 0;
    	} else if (t->atr == M_ATR_SET) {
    	    n = t->argb;
    	} else {
    	    Filn_Error("PrgErr:#set���x��\n");
    	}
    	Z.M_ep = M_GetSym(Z.M_ep);

    } else if (Z.M_sym == '0') {
    	n = Z.M_val;
    	Z.M_ep = M_GetSym(Z.M_ep);

    } else if (Z.M_sym == '(') {
    	n = M_ExprA();
    	if (Z.M_sym != ')')
    	    Filn_Error("')'������Ȃ�\n");
    	Z.M_ep = M_GetSym(Z.M_ep);
    } else if (Z.M_sym == '-') {
    	n = -M_Expr0();

    } else if (Z.M_sym == '+') {
    	n = +M_Expr0();

    } else if (Z.M_sym == '~') {
    	n = ~M_Expr0();

    } else if (Z.M_sym == '!') {
    	n = !M_Expr0();

    } else if (Z.M_sym == '"' && V.opt_wq_mode) {
    	n = 0;
    	strncpyZ(Z.M_expr_str, Z.M_str, sizeof(Z.M_expr_str));
    	Z.M_ep = M_GetSym(Z.M_ep);

    } else if (Z.M_sym == 0) {
    	n = 0;
    	Z.M_ep = M_GetSym(Z.M_ep);

    } else if (Z.M_sym == '$' && V.immMode == 4/*���g���[��16�i */) {
    	Z.M_ep = M_GetSym(Z.M_ep);
    	if (Z.M_sym != '0')
    	    goto J1;
    	n = Z.M_val;
    	Z.M_ep = M_GetSym(Z.M_ep);

    } else {
  J1:
    	Filn_Error("�z�肵�Ă��Ȃ���/�w�肾\n");
    	return n;
    }
    //Z.M_ep = M_GetSym(Z.M_ep);
    return n;
}


static val_t M_Expr1(void)
{
    val_t m,n;
    m = M_Expr0();
    for (; ;) {
    	if (Z.M_sym == '*') {
    	    n = M_Expr0();
    	    m = m * n;
    	} else if (Z.M_sym == '/') {
    	    n = M_Expr0();
    	    if (n == 0) {
    	    	Filn_Error("0�Ŋ��낤�Ƃ���\n");
    	    	return 0;
    	    }
    	    m = m / n;
    	} else if (Z.M_sym == '%') {
    	    n = M_Expr0();
    	    if (n == 0) {
    	    	Filn_Error("0�Ŋ��낤�Ƃ���.\n");
    	    	return 0;
    	    }
    	    m = m % n;
    	} else {
    	    break;
    	}
    }
    return m;
}


static val_t M_Expr2(void)
{
    val_t m;
    m = M_Expr1();
    for (; ;) {
    	switch(Z.M_sym) {
    	case '+':   m = m + M_Expr1();	break;
    	case '-':   m = m - M_Expr1();	break;
    	default:    return m;
    	}
    }
}



#define M_C(a,b)    ((a)|((b)*256))

static val_t M_Expr3(void)
{
    val_t m;
    m = M_Expr2();
    for (; ;) {
    	switch(Z.M_sym) {
    	case M_C('<','<'):  m = (m << M_Expr2());   break;
    	case M_C('>','>'):  m = (m >> M_Expr2());   break;
    	default:    return m;
    	}
    }
}


static val_t M_Expr4(void)
{
    val_t   m;
    int     n;
    m = M_Expr3();
    for (; ;) {
    	if (Z.M_expr_str[0]) {
    	    switch(Z.M_sym) {
    	    case '>':
    	    case '<':
    	    case M_C('>','='):
    	    case M_C('<','='):
    	    case M_C('>','<'):
    	    	n = Z.M_sym;
    	    	strcpy(Z.M_expr_str2, Z.M_expr_str);
    	    	M_Expr3();
    	    	m = strcmp(Z.M_expr_str2, Z.M_expr_str);
    	    	switch (n) {
    	    	case '>':   	    m = (m > 0);    break;
    	    	case '<':   	    m = (m < 0);    break;
    	    	case M_C('>','='):  m = (m >= 0);   break;
    	    	case M_C('<','='):  m = (m <= 0);   break;
    	    	default:
    	    	    m = (m > 0) ? 1 : (m < 0) ? -1 : 0;
    	    	}
    	    	Z.M_expr_str[0] = 0;
    	    	Z.M_expr_str2[0] = 0;
    	    	break;
    	    default:
    	    	return m;
    	    }
    	} else {
    	    switch(Z.M_sym) {
    	    case '>':	    	m = (m >  M_Expr3());	break;
    	    case '<':	    	m = (m <  M_Expr3());	break;
    	    case M_C('>','='):	m = (m >= M_Expr3());	break;
    	    case M_C('<','='):	m = (m <= M_Expr3());	break;
    	    case M_C('>','<'):	m = (m > 0) ? 1 : (m < 0) ? -1 : 0; break;
    	    default:	    	return m;
    	    }
    	}
    }
}


static val_t M_Expr5(void)
{
    val_t   m,n;
    long    c;
    m = M_Expr4();
    for (; ;) {
    	switch(Z.M_sym) {
    	case M_C('=','='):
    	case M_C('!','='):
    	    c = Z.M_sym;
    	    if (Z.M_expr_str[0])
    	    	strcpy(Z.M_expr_str2, Z.M_expr_str);
    	    n = M_Expr4();
    	    if (Z.M_expr_str[0]) {
    	    	m = (strcmp(Z.M_expr_str2, Z.M_expr_str) == 0);
    	    } else {
    	    	m = (m == n);
    	    }
    	    if (c == M_C('!','='))
    	    	m = !m;
    	    Z.M_expr_str[0] = 0;
    	    Z.M_expr_str2[0] = 0;
    	    break;
    	default:
    	    return m;
    	}
    }
}


static val_t M_Expr6(void)
{
    val_t m;
    m = M_Expr5();
    for (; ;) {
    	switch(Z.M_sym) {
    	case '&':   m = (m & M_Expr5());    break;
    	default:    return m;
    	}
    }
}


static val_t M_Expr7(void)
{
    val_t m;
    m = M_Expr6();
    for (; ;) {
    	switch(Z.M_sym) {
    	case '^':   m = (m ^ M_Expr6());    break;
    	default:    return m;
    	}
    }
}


static val_t M_Expr8(void)
{
    val_t m;
    m = M_Expr7();
    for (; ;) {
    	switch(Z.M_sym) {
    	case '|':   m = (m | M_Expr7());    break;
    	default:    return m;
    	}
    }
}


static val_t M_Expr9(void)
{
    val_t m,n;
    int  sv;
    m = M_Expr8();
    for (; ;) {
    	switch(Z.M_sym) {
    	case M_C('&','&'):
    	    sv = Z.errLblSkip;
    	    Z.errLblSkip = 1;
    	    n = M_Expr8();
    	    Z.errLblSkip = sv;
    	    m = (m && n);
    	    break;
    	default:
    	    return m;
    	}
    }
}


static val_t M_ExprA(void)
{
    val_t m,n;
    int sv;

    m = M_Expr9();
    for (; ;) {
    	switch(Z.M_sym) {
    	case M_C('|','|'):
    	    sv = Z.errLblSkip;
    	    Z.errLblSkip = 1;
    	    n = M_Expr9();
    	    Z.errLblSkip = sv;
    	    m = (m || n);
    	    break;
    	default:
    	    return m;
    	}
    }
}


static val_t M_Expr(char const* s)
{
    val_t n;
    Z.M_ep = s;
    //fprintf(STDERR, ":%d>%s\n", n=0, Z.M_ep);
    n = M_ExprA();
    //fprintf(STDERR, ":%d>%s\n", n, Z.M_ep);
    return n;
}


#define M_ChkEol(sym,s)     M_ChkEol_nm_l(sym, s, __FILE__, __LINE__)

static char const* M_ChkEol_nm_l(int sym, char const* s, char const* nm, int l)
{
    int c;

    if (sym != 0 && sym != '\n' && sym != M_C('/','/'))
    	goto J1;
    c = *(s = SkipSpc(s));
    if (c == '/' && s[1] == '/') {
    	while (*s != '\n' && *s != '\0')
    	    s++;
    } else {
    	if (c != 0 && c != '\n') {
  J1:
    	    if (Z.chkEol_name != Z.inclp->name && Z.chkEol_line != Z.inclp->line) {
    	    	//fprintf(V.errFp, "%s %d :\n", nm, l);
    	    	Filn_Error("�s���܂łɕs�v�ȕ���������... %s\n", s);
    	    }
    	    Z.chkEol_name = Z.inclp->name;
    	    Z.chkEol_line = Z.inclp->line;
    	    TOEOS(s);
    	}
    }
    return s;
}



/*---------------------------------------------------------*/


static void M_MoveArg(int *pArgc, char*** pArgv)
{
    if (Z.M_argc > 0) {
    	char** argv = callocE(1,Z.M_argc*sizeof(char*));
    	memcpy(argv, Z.M_argv, Z.M_argc*sizeof(char*));
    	*pArgc = Z.M_argc;
    	*pArgv = argv;
    } else {
    	*pArgc = 0;
    	*pArgv = NULL;
    }
    memset(Z.M_argv, 0, sizeof(Z.M_argv));
    Z.M_argc = 0;
}


static void M_FreeArg(int* pArgc, char*** pArgv) {
    int     i;
    char**  argv = *pArgv;
    if (argv) {
    	for (i = 0; i < *pArgc; i++) {
    	    SAFE_FREE(argv[i]);
    	}
    }
    *pArgc = 0;
}


static void M_ClearArg(void)
{
    int i;
    for (i = 0; i < Z.M_argc; i++) {
    	SAFE_FREE(Z.M_argv[i]);
    }
    memset(Z.M_argv, 0, sizeof(Z.M_argv));
    Z.M_argc = 0;
}


static char const* M_GetArg(char const* s, int cont, char const* tit)
{
    int  k,i,c;

    if (cont == 0) {
    	M_ClearArg();
    }
    k = 0;
    if (*s == '(') {
    	k = 1;
    	s++;
    }
    c = *(s = SkipSpc(s));
    if (c == ')') {
    	s++;
    	if (k == 0)
    	    Filn_Error("%c%s��`����������\n", V.mac_chr, tit);
    	return s;
    }
    if (c == 0) {
    	if (k)
    	    Filn_Error("%c%s��`����������\n", V.mac_chr, tit);
    	return s;
    }
    /* �������̎擾 */
    for (; ;) {
    	s = M_GetSym(s);
    	if (Z.M_argc >= FILN_ARG_NUM) {
    	    Filn_Error("%c%s��`�ň������������܂�\n", V.mac_chr, tit);
    	    break;
    	}
    	if (Z.M_sym != 'A') {
    	    Filn_Error("%c%s��`�ł̈�������������\n", V.mac_chr, tit);
    	    break;
    	}
    	for (i = 0; i < Z.M_argc; i++) {
    	    if (strcmp(Z.M_argv[i], Z.M_name) == 0) {
    	    	Filn_Error("%c%s�w��̈����œ������O������\n", V.mac_chr, tit);
    	    }
    	}
    	Z.M_argv[Z.M_argc++] = strdupE(Z.M_name);

    	s = M_GetSym(s);
    	if (Z.M_sym == ')') {
    	    if (k == 0)
    	    	Filn_Error("%c%s��`����������\n",V.mac_chr,tit);
    	    break;
    	}
    	if (Z.M_sym == 0) {
    	    if (k)
    	    	Filn_Error("%c%s��`����������\n",V.mac_chr,tit);
    	    break;
    	}
    	if (Z.M_sym != ',') {
    	    Filn_Error("%c%s��`�̈����w�肪��������\n",V.mac_chr,tit);
    	    break;
    	}
    }
    return s;
}


/** #define ��`�s�̏���
 */
static int M_Define(char const* s)
{
    char    	name[FILN_NAME_SIZE+2];
    char**  	argv;
    char const* p;
    int     	argc,atr;
    int     	c;

    /* ���O�𓾂� */
    s = M_GetSym(s);
    if (Z.M_sym != 'A') {
    	Filn_Error("%cdefine��`����������\n",V.mac_chr);
    	return 0;
    }
    strcpy(name, Z.M_name);
    argc = 0;
    argv = NULL;
    if (*s == '(') {	/* �֐��^�̃}�N�����H */
    	s = M_GetArg(s,0,"define");
    	//argc = Z.M_argc;
     #if 1
    	M_MoveArg(&argc, &argv);
     #else
    	if (Z.M_argc) {
    	    argv = callocE(1,Z.M_argc*sizeof(char*));
    	    memcpy(argv, Z.M_argv, Z.M_argc*sizeof(char*));
    	}
     #endif
    	s = SkipSpc(s);
    	atr = M_ATR_MAC;
    } else {
    	s = SkipSpc(s);
    	atr = M_ATR_DEF;
    }
    p = s; TOEOS(p);
 #if 1	// char const* �� �� allocate���@�ύX.
    MTREE_Add(name, atr, argc, argc, argv, strndupE(s, p - s + 1), 0);
 #else
    c = *p; *(char*)p = 0;
    MTREE_Add(name, atr, argc, argc, argv, strdupE(s), 0);
    *(char*)p = c;
 #endif
    M_ClearArg();
    return 0;
}


#if 0
static int M_Array(char *s)
{
    /* #array ��`�s�̏��� */
  #if 0
    char name[FILN_NAME_SIZE+2];
    char **argv, *p;
    int  argc,atr;
    int  c;

    /* ���O�𓾂� */
    s = M_GetSym(s);
    if (Z.M_sym != 'A') {
    	Filn_Error("%carray��`����������\n",V.mac_chr);
    	return 0;
    }
    strcpy(name, Z.M_name);
    argc = 0;
    argv = NULL;
    s = SkipSpc(s);

    atr = M_ATR_DEF;

    p = s; TOEOS(p); c = *p; *p = 0;
    MTREE_Add(name, atr, argc, argc, argv, strdupE(s), 0);
    *p = c;
  #endif
    return 0;
}

#endif



static int M_ChkMchr(char const* s)
{
    if (*s == V.mac_chr && ISSPACE(s[1]) == 0)
    	return 1;
    else
    	return 0;
}


static int M_Macro(char const* s)
{
    char    	name[FILN_NAME_SIZE+2];
    char    	wk[8];
    char ** 	argv;
    int     	argb;
    int     	argc;
    int     	macflg;
    MTREE_T*	p;
    char*   	fname;
    long    	line;

    /* */
    fname = Z.inclp->name;
    line  = Z.inclp->line-1;	    	    	    /* -1 ��\n_S\xFF_\n �̍ŏ���\n��ǉ��������߂̒��� */

    argb   = 0;
    argc   = 0;
    argv   = NULL;
    macflg = 0;
    /* ���O�𓾂� */
    s = M_GetSym(s);
    if (Z.M_sym == 'A') {
    	strcpy(name, Z.M_name);
    	s = SkipSpc(s);
    	s = M_GetArg(s,0, "macro");
    	argb = Z.M_argc;
    	M_ChkEol(0,s);
    } else if (Z.M_sym == 0 || Z.M_sym == '\n' || Z.M_sym == M_C('/','/')) {	/* ���O�����}�N�� */
    	macflg = -1;
    	strcpy(name, "_macro\xff_");
     #if 1
    	M_ClearArg();
     #else
    	Z.M_argc = 0;
    	memset(Z.M_argv, 0, sizeof(Z.M_argv));
     #endif
    	/*s = SkipSpc(s);*/
    	M_ChkEol(Z.M_sym,s);
    } else {
    	Filn_Error("%c%s ��`����������\n",V.mac_chr, "macro");
    	return 0;
    }

    InitStMbuf();
    sprintf(wk,"\n%c_S\xFF_\n",V.mac_chr);  	/* exitm �����p�Ƀ}�N���͈͂̊J�n�ʒu�𖄂߂��� */
    StMbuf(wk);

    for (; ;) {
    	s = Filn_GetLine();
    	if (s == NULL) {
    	    Filn_Exit("%c%s��`�r���Ńt�@�C�����I�����Ă���\n",V.mac_chr,"macro");
    	}
    	s = SkipSpc(s);
    	if (M_ChkMchr(s)) {
    	    int o;
    	    s = M_GetSym(s+1);
    	    o = M_OdrSearch(Z.M_name);
    	    if (Z.M_sym == 'A') {
    	    	if (o == M_LOCAL) {/*#local��`*/
    	    	    s = M_GetArg(s,1,"local");
    	    	    M_ChkEol(0,s);
    	    	    continue;
    	    	} else if (o == M_ENDMACRO) {
    	    	    break;
    	    	} else if (o == M_UNDEF || o == M_MACRO) {
    	    	    Filn_Error("%c%s��`���ł� %c%s �ł��܂���\n",V.mac_chr,"macro",V.mac_chr,Z.M_name);
    	    	    continue;
    	    	}
    	    }
    	}
    	StMbuf(Z.linbuf);
    	StMbuf("\n");
    }
    sprintf(wk,"\n%c_E\xFF_\n",V.mac_chr);  /* exitm �����p�Ƀ}�N���͈͂̏I���ʒu�𖄂߂��� */
    StMbuf(wk);

 #if 1
    M_MoveArg(&argc, &argv);
 #else
    if (Z.M_argc) {
    	argv = callocE(1,Z.M_argc*sizeof(char*));
    	memcpy(argv, Z.M_argv, Z.M_argc*sizeof(char*));
    	memset(Z.M_argv, 0, sizeof(Z.M_argv));
    }
 #endif
    p = MTREE_Add(name, M_ATR_MAC, argb, argc, argv, strdupE(Z.M_mbuf), 0);
    M_ClearArg();
    p->fname = fname;
    p->line  = line;
    if (macflg) {
    	Z.macBgnStr = p;
    }
    return macflg;
}


static char const* M_Undef(char const* name)
{
    MTREE_T*	p;
    char const* s;

    //DB printf("[undef]%s\n", name);
    s = M_GetSym(name);
    if (Z.M_sym != 'A')
    	return s;
    p = MTREE_Search(Z.M_name);
    if (p) {
    	SAFE_FREE(p->buf);
      #if 1
    	M_FreeArg(&p->argc, &p->argv);
      #else
    	if (p->argv) {
    	    int     	i;
    	    for (i = 0; i < p->argc; i++)
    	    	SAFE_FREE(p->argv[i]);
    	    SAFE_FREE(p->argv);
    	}
      #endif
    	p->atr = M_ATR_0;
    }
    return s;
}


/** #rept �` #endrept ���o�b�t�@�֒�����
 * ���ۂ̓W�J�� M_Macc ���ōs��
 */
static int M_Rept(char *s)
{
    int endflg, r,o;
    char *b;

    /*printf("<rept>\n");*/
    /*printf("%s\n",s);*/
    StMbuf(s);	    	/* #rept�s���̂��o�b�t�@�� */
    StMbuf("\n");
    endflg = r = 0;
    while (endflg == 0) {
    	b = s = Filn_GetLine();
    	if (s == NULL)
    	    Filn_Exit("%crept ��`�r���Ńt�@�C�����I�����Ă���\n",V.mac_chr);
    	s = SkipSpc(s);
    	if (M_ChkMchr(s)) {
    	    /*s =*/ M_GetSym(s+1);
    	    if (Z.M_sym == 'A') {
    	    	o = M_OdrSearch(Z.M_name);
    	    	if (o == M_MACRO || o == M_LOCAL || o == M_ENDMACRO) {
    	    	    Filn_Error("%crept ���ł� %c%s �ł��܂���\n",V.mac_chr,V.mac_chr,Z.M_name);
    	    	    continue;
    	    	} else if (o == M_REPT) {
    	    	    r++;
    	    	} else if (o == M_ENDREPT) {
    	    	    if (r == 0)
    	    	    	endflg = 1;
    	    	    else
    	    	    	r--;
    	    	}
    	    }
    	}
    	StMbuf(b);
    	StMbuf("\n");
    	/*printf("%s\n",b);*/
    }
    /*printf("<endrept>\n");*/
    return 0;
}


/** #ipr �` #endipr ���o�b�t�@�֒�����
 * ���ۂ̓W�J�� M_Macc ���ōs��
 */
static int M_Ipr(char const* s)
{
    int     	endflg;
    int     	r;
    char const* b;

    StMbuf(s);	    	/* #ipr�s���̂��o�b�t�@�� */
    StMbuf("\n");
    endflg = r = 0;
    while (endflg == 0) {
    	if ((b = s = Filn_GetLine()) == NULL)
    	    Filn_Exit("%cipr ��`�r���Ńt�@�C�����I�����Ă���\n",V.mac_chr);
    	s = SkipSpc(s);
    	if (M_ChkMchr(s)) {
    	    /*s =*/M_GetSym(s+1);
    	    if (Z.M_sym == 'A') {
    	    	int o;
    	    	o = M_OdrSearch(Z.M_name);
    	    	if (o == M_MACRO || o == M_LOCAL || o == M_ENDMACRO) {
    	    	    Filn_Error("%cipr�ł� %c%s �ł��܂���\n",V.mac_chr,V.mac_chr,Z.M_name);
    	    	    continue;
    	    	} else if (o == M_IPR) {
    	    	    r++;
    	    	} else if (o == M_ENDIPR) {
    	    	    if (r == 0)
    	    	    	endflg = 1;
    	    	    else
    	    	    	r--;
    	    	}
    	    }
    	}
    	StMbuf(b);
    	StMbuf("\n");
    }
    return 0;
}



/** #include
 */
static int M_Include(char const* s)
{
    int     i,k;
    char    name[FILN_FNAME_SIZE+2];

    if (*s == '"')
    	s++, k = '"';
    else if (*s == '<')
    	s++, k = '>';
    else
    	k = 0;
    for (i = 0; i < (sizeof name)-1; i++) {
    	if (ISSPACE(*s) || *s == k || *s == 0 || *s == '\n')
    	    break;
    	name[i] = *s++;
    }
    name[i] = 0;
    if (*s == '"' || *s == '>')
    	s++;
    M_ChkEol(0,s);
    Filn_Open0(name, k == '>');
    return 0;
}


#if 0
static int M_Set(char const* s)
{
    MTREE_T* t;
    val_t    n;
    char     name[FILN_NAME_SIZE+2];

    s = M_GetSym(s);
    if (Z.M_sym != 'A') {
    	Filn_Error("%cset��`����������\n",V.mac_chr);
    	return 0;
    }
    strcpy(name, Z.M_name);
    s = M_GetSym(s);
    if (Z.M_sym != '=') {
    	Filn_Error("%cset��`����������\n",V.mac_chr);
    	return 0;
    }
    n = M_Expr(s);
    s = Z.M_ep;
    M_ChkEol(Z.M_sym, s);
    t = MTREE_Add(name, M_ATR_SET, n, 0, NULL, NULL, 0);
    return 0;
}
#endif


/** #print �s�\��
 */
static int M_Print(char const* s)
{
    val_t   	n;
    unsigned	c;
    char const* p;

    for (; ;) {
    	p = s;
    	s = M_GetSym(s);
      J1:
    	if (Z.M_sym == 0 || Z.M_sym == '\n') {
    	    fputc('\n', stdout);
    	    break;
    	} else if (Z.M_sym == '"') {
    	    *(STREND(Z.M_str)-1) = '\0';
    	    p = Z.M_str+1;
    	    while (*p) {
    	    	c = *p;
    	    	if (FILN_MBC_IS_LEAD(c)) {
    	    	    unsigned l = mbs_len1(p);
    	    	    unsigned j;
    	    	    for (j = 0; j < l; ++j) {
    	    	    	c = *p++;
    	    	    	fputc(c, stdout);
    	    	    }
    	    	} else if (c == '\\' && (V.opt_yen <= 2)) {
    	    	    ++p;
    	    	    c = M_GetEscChr(&p);
    	    	    fputc(c, stdout);
    	    	} else {
    	    	    ++p;
    	    	    fputc(c, stdout);
    	    	}
    	    }
    	} else if (Z.M_sym == 'A') {
    	    printf("%s",Z.M_name);

    	} else if (Z.M_sym == '0' || Z.M_sym == '(' || Z.M_sym == '-'
    	    	|| Z.M_sym == '+' || Z.M_sym == '~' || Z.M_sym == '!'
    	    	|| (Z.M_sym == '\'' && V.opt_sq_mode))
    	{
    	    n = M_Expr(p);
    	    p = s = Z.M_ep;
    	    printf(S_FMT_D, n);
    	    goto J1;

    	} else {
    	    printf("%s", Z.M_str);
    	}
    }
    return 0;
}



/*-----------------------------------------------------------*/
static void MM_MaccMacroAtrRsv(int argb, char const* name);
static char const* MM_MaccMacro(char const* s, int sh, MTREE_T* t);
static void MM_Macc_MRept(char const* name, char* d, char const** s0 /*, char *fname, uint32_t line*/);
static void MM_Macc_MIpr(char const* name, char const** s0, int argc, char const* const* argv/*, char *fname, uint32_t line*/);
static void MM_Macc_MSet(char const* name, char const* s);


static void MM_Init(void)
{
    Z.MM_ptr = NULL;
    Z.MM_localNo = 0;
    MTREE_Add("__FILE__", M_ATR_RSV, M_RSV_FILE, 0, NULL, NULL, 0);
    MTREE_Add("__LINE__", M_ATR_RSV, M_RSV_LINE, 0, NULL, NULL, 0);
    MTREE_Add("__DATE__", M_ATR_RSV, M_RSV_DATE, 0, NULL, NULL, 0);
    MTREE_Add("__TIME__", M_ATR_RSV, M_RSV_TIME, 0, NULL, NULL, 0);
}

#if 0
static void MM_MaccErr(char *fname, uint32_t line)
{
    if (fname)
    	fprintf(V.errFp, "%-13s %5lu : %cmacro,%crept���̒�`�̒���\n", V.mac_chr, fname, V.mac_chr, line);
}
#endif


/**
 */
static void MM_Macc(char const* s, int exmacF, MTREE_T* m, char** v /*, char *fname, uint32_t line*/, char const* dbms)
{
    char*   	msetp  = NULL;
    char*   	mreptp = NULL;
    char*   	exprp  = NULL;
    int     	exprFlg= exmacF;
    int     	expr_r = 0;
    int     	gyoto  = 0;
    char    	name[FILN_NAME_SIZE+2];
    int     	sh; /* #�}�N�����ɂ��"�}�N����"�W�J�̃t���O */
    char const* p;
    char*   	p2;
    char const* q;
    char**  	a;
    char    	tmp[0x40];
    int     	i,c,k;
    val_t   	l;

    /*StMbuf("");*/
    if (s == NULL)
    	return;
    Z.MM_nest++;

  #if 0
    DB {
    	int z;
    	printf("�}�N��%s�W�J(nest %d)  [%s]\n", m?m->name:"", Z.MM_nest, dbms);
    	if (m && v && v[0]) {
    	    for (z = 0; z < m->argc; z++) {
    	    	printf("  ����[%d:%s] = %s\n", z, m->argv[z], v[z]);
    	    }
    	}
    	printf("%s\n", s);
    }
  #endif
  #if 0
    //if (Z.MM_nest > 1)
    //	Filn_PutsSrcLine();
  #endif
    for (;;) {
    LOOP:
    	//p = Z.M_mptr;
    	if (ISSPACE(*s)) {  /* �󔒂𔲂������Ȃ��悤�ɂ��邽�߂̏��� */
    	    if (V.opt_delspc) {     /* 0�ȊO�Ȃ�΋󔒂̈��k������ */
    	    	StMbuf(" ");
    	    } else {
    	    	s = M_GetSym0(s);
    	    	StMbuf(Z.M_str);
    	    }
    	}
    	s = M_GetSym(s);

      LOP2:
    	sh = 0;
    	gyoto++;
    	if (Z.M_sym == 0 || Z.M_sym == '\n') {	/* �s���A�܂��́A�}�N���̏I��� */
    	    gyoto = 0;
    	  #if 0
    	    //if (Z.MM_nest > 1)
    	    //	Filn_PutsSrcLine();
    	  #endif
    	    k = Z.M_sym;
    	    if (msetp) {    /* ���ۂ�#set�s�̏��� */
    	    	MM_Macc_MSet(name, msetp);
    	    	Z.M_mptr = msetp;
    	    	*Z.M_mptr = 0;
    	    }
    	    if (mreptp) {   	/* ���ۂ�#rept�`#endrept�̏��� */
    	    	if (k == 0) {	/* �Ή�����#endrept�������̂����炩 */
    	    	    /*MM_MaccErr(fname, line);*/
    	    	    Filn_Error("�Ή����� %cendrept ���Ȃ�\n",V.mac_chr);
    	    	    *mreptp = 0;
    	    	} else {
    	    	    // --s;
    	    	    MM_Macc_MRept(name, mreptp, &s/*, fname, line*/);
    	    	}
    	    }
    	    if (expr_r) {
    	    	/*MM_MaccErr(fname, line);*/
    	    	Filn_Error("(��)�̌��� ) ���Ȃ�\n");
    	    }
    	    exprFlg = exmacF;
    	    exprp = mreptp = msetp = NULL;
    	    ++Z.errMacLin;
    	    if (k == '\0') {
    	    	break;
    	    }
    	    StMbuf("\n");

    	} else if (Z.M_sym == '0') {	    	/* �萔 */
    	    if (V.immMode)
    	    	StMbuf(M_ImmStr(tmp, Z.M_val, 0));
    	    else
    	    	StMbuf(Z.M_str);
    	} else if (Z.M_sym == '"' && V.opt_wq_mode) {	    	/* "������" */
    	    StMbuf(Z.M_str);
    	} else if (Z.M_sym == M_C('/','/')) {
    	    if (msetp || mreptp)
    	    	TOEOS(s);
    	    else
    	    	StMbuf("//");
    	    goto LOOP;

    	} else if (exprFlg && Z.M_sym == M_C(V.mac_chr2, V.mac_chr2)) { /* ## �ɂ�镶����A�� */
    	    ;
    	} else if (Z.M_sym == V.mac_chr && !ISSPACE(*s)) {  	    /*====== #���� ======*/
    	    s = M_GetSym(s);
    	    if (exprFlg && V.mac_chr == V.mac_chr2 && Z.M_sym == '(') { /*-- #(��) --*/
    	    	goto JJJ_EXPR;
    	    }
    	    if (exprFlg && V.mac_chr == V.mac_chr2 && Z.M_sym == '&') {
    	    	goto JJJ_STR_REF;
    	    }
    	    if (Z.M_sym != 'A') {
    	    	StMbuf(Z.mac_chrs/*"#"*/);
//printf("!A0 %d %s\n",sh, Z.M_name);
    	    	goto LOP2;
    	    }
    	    k = M_OdrSearch(Z.M_name);
    	    if (k < 0) {    	    	    	    	    	    /* #���߂łȂ����� */
    	    	if (V.mac_chr == V.mac_chr2)
    	    	    goto JJJJ;
    	    	StMbuf(Z.mac_chrs/*"#"*/);
    	    	goto JJJ_LBL;
    	    }
    	    if (gyoto == 1) {	/* �s���̎��̂ݗL���Ȗ��� */
    	/* GYOTO_MAC: */
    	    	if (k == M_DEFINE) {	    	    	    	    /*-- #define --*/
    	    	    p = s;
    	    	    TOEOS(s);
    	    	  #if 0
    	    	    if (*s == '\n') /* �}�N�����ł̉��s�΍� */
    	    	    	*s++ = 0;
    	    	  #endif
    	    	    M_Define(p);
    	    	    goto LOOP;

    	    	} else if (k == M_UNDEF) {  	    	    	/*-- #undef --*/
    	    	    if (Z.MM_cnt) { /* ���ݓW�J���̃}�N�����Ɠ����Ȃ�΁A#undef�ł��Ȃ� */
    	    	    	for (i = 0; i < Z.MM_cnt; i++) {
    	    	    	    if (strcmp(Z.M_name, Z.MM_name[i]) == 0) {
    	    	    	    	/*MM_MaccErr(fname, line);*/
    	    	    	    	Filn_Error("�}�N�� %s �͓W�J���Ȃ̂� #undef �ł��܂���\n", Z.M_name);
    	    	    	    	TOEOS(s);
    	    	    	    	goto LOOP;
    	    	    	    }
    	    	    	}
    	    	    }
    	    	    s = M_Undef(s);
    	    	    M_ChkEol(0, s);
    	    	    goto LOOP;

    	    	} else if (k == M_SET) {    	    	    	/*-- #set --*/
    	    	    /* #set ���x��=�� �̎��̂��߂ɍs���܂Ń}�N���W�J���A�s���Ŏ��ۂ̏��������� */
    	    	    if (msetp || mreptp || exprp) {
    	    	    	/*MM_MaccErr(fname, line);*/
    	    	    	Filn_Error("%cset ��`����������\n",V.mac_chr);
    	    	    	TOEOS(s);
    	    	    	goto LOOP;
    	    	    }
    	    	    exprFlg = 1;    /* defined()�L�� */
    	    	    msetp = NULL;
    	    	    s = M_GetSym(s);
    	    	    if (Z.M_sym != 'A') {
    	    	    	/*MM_MaccErr(fname, line);*/
    	    	    	Filn_Error("%cset��`����������.\n",V.mac_chr);
    	    	    	TOEOS(s);
    	    	    	goto LOOP;
    	    	    }
    	    	    strcpy(name, Z.M_name);
    	    	    s = M_GetSym(s);
    	    	    if (Z.M_sym != '=') {
    	    	    	/*MM_MaccErr(fname, line);*/
    	    	    	Filn_Error("%cset=��`����������\n",V.mac_chr);
    	    	    	TOEOS(s);
    	    	    	goto LOOP;
    	    	    }
    	    	    msetp = Z.M_mptr;
    	    	    goto LOOP;

    	    	} else if (k == M_IFDEF || k == M_IFNDEF) { 	/*-- #ifdef --*/
    	    	    /* ��芸�����A��`����Ă��邩�ǂ�����������, #if �ɕϊ� */
    	    	    StMbuf("#if ");
    	    	    s = M_GetSym(s);
    	    	    c = 0;
    	    	    if (Z.M_sym == 'A')
    	    	    	c = (MTREE_Search(Z.M_name) != NULL) ? 1 : 0;
    	    	    if (k == M_IFNDEF)
    	    	    	c = !c;
    	    	    StMbuf(c?"1":"0");
    	    	    goto LOOP;

    	    	} else if (k == M_REPT) {   	    	    	/*-- #rept --*/
    	    	    /* #rept ���x��=�� �̎��̂��߂ɍs���܂Ń}�N���W�J���A�s���Ŏ��ۂ̏��������� */
    	    	    if (mreptp || msetp || exprp) {
    	    	    	/*MM_MaccErr(fname, line);*/
    	    	    	Filn_Error("%crept ��`����������\n",V.mac_chr);
    	    	    	TOEOS(s);
    	    	    	goto LOOP;
    	    	    }
    	    	    exprFlg = 1;    /* defined()�L�� */
    	    	    name[0] = 0;
    	    	    mreptp = Z.M_mptr;
    	    	    p = s;
    	    	    s = M_GetSym(s);
    	    	    if (Z.M_sym == 'A') {
    	    	    	strcpy(name, Z.M_name);
    	    	    	s = M_GetSym(s);
    	    	    	if (Z.M_sym == '=') {
    	    	    	    /*StMbuf(name);StMbuf("=");*/
    	    	    	    mreptp = Z.M_mptr;
    	    	    	} else {
    	    	    	    name[0] = 0;
    	    	    	    s = p;
    	    	    	}
    	    	    } else {
    	    	    	s = p;
    	    	    }
    	    	    goto LOOP;

    	    	} else if (k == M_IPR) {    	    	    /*-- #ipr --*/
    	    	    /* #ipr ���x��=����1,����2, ... */
    	    	    if (msetp || mreptp) {
    	    	    	/*MM_MaccErr(fname, line);*/
    	    	    	Filn_Error("%cipr ��`����������\n",V.mac_chr);
    	    	    	TOEOS(s);
    	    	    	goto LOOP;
    	    	    }
    	    	    exprFlg = 1;    /* defined()�L�� */
    	    	    s = M_GetSym(s);
    	    	    if (Z.M_sym != 'A') {
    	    	    	/*MM_MaccErr(fname, line);*/
    	    	    	Filn_Error("%cipr ��`����������.\n",V.mac_chr);
    	    	    	TOEOS(s);
    	    	    	goto LOOP;
    	    	    }
    	    	    strcpy(name, Z.M_name);
    	    	    s = M_GetSym(s);
    	    	    if (Z.M_sym != '=') {
    	    	    	/*MM_MaccErr(fname, line);*/
    	    	    	Filn_Error("%cipr��`����������.\n",V.mac_chr);
    	    	    	TOEOS(s);
    	    	    	goto LOOP;
    	    	    }

    	    	    /* ipr �̈������擾 */
    	    	    a = callocE(sizeof(char*), FILN_ARG_NUM);
    	    	    c = 0;
    	    	    do {
    	    	    	int r, k1, k2;
    	    	    	/*r = k1 = k2 = 0;*/
    	    	    	s = SkipSpc(s);
    	    	    	p = s;
    	    	    	do {
    	    	    	    q = s;
    	    	    	    s = M_GetSym(s);
    	    	    	    if (Z.M_sym == '(' || Z.M_sym == '[' || Z.M_sym == '{') {
    	    	    	    	k1 = Z.M_sym;
    	    	    	    	k2 = (k1 == '(') ? ')' : (k1 == '[') ? ']' : '}';
    	    	    	    	r = 1;
    	    	    	    	do {
    	    	    	    	    q = s;
    	    	    	    	    s = M_GetSym(s);
    	    	    	    	    if (Z.M_sym == k1)
    	    	    	    	    	++r;
    	    	    	    	    else if (Z.M_sym == k2)
    	    	    	    	    	--r;
    	    	    	    	} while(Z.M_sym != 0 && Z.M_sym != '\n' && (r || Z.M_sym != ','));
    	    	    	    	/*k2 = k1 = 0;*/
    	    	    	    }
    	    	    	} while (Z.M_sym != 0 && Z.M_sym != '\n' && Z.M_sym != ',');
    	    	    	if (c == FILN_ARG_NUM) {
    	    	    	    /*MM_MaccErr(fname, line);*/
    	    	    	    Filn_Error("%cipr �̈�������������(%d�܂�)\n",V.mac_chr,FILN_ARG_NUM);
    	    	    	}
    	    	    	if (c < FILN_ARG_NUM) {
    	    	    	    a[c] = mallocE(q-p+1);
    	    	    	    strncpyZ(a[c], p, q-p+1);
    	    	    	    c++;
    	    	    	}
    	    	    } while(Z.M_sym == ',');
    	    	    /* ���[�v�W�J */
    	    	    MM_Macc_MIpr(name, &s, c, a /*, fname, line*/);
    	    	 #if 1
    	    	    M_FreeArg(&c, &a);
    	    	 #else
    	    	    for (i = 0; i < c; i++)
    	    	    	SAFE_FREE(a[i]);
    	    	    SAFE_FREE(a);
    	    	 #endif
    	    	    goto LOOP;
    	    	} else if (k == M_ARRAY) {
    	    	    //p = s;
    	    	    TOEOS(s);
    	    	    //M_Array(p);
    	    	    goto LOOP;
    	    	} else if (k == M_SELF) {
    	    	    StMbuf(Z.mac_chrs/*"#"*/);
    	    	    if (v)
    	    	    	StMbuf(Z.M_name);
    	    	    goto LOOP;
    	    	} else {
    	    	  #if 0
    	    	    if (k == M_IF    || k == M_ELIF
    	    	    	|| k == M_IFEQ /*|| k == M_IFNE*/
    	    	    	|| k == M_IFGT	|| k == M_IFGE
    	    	    	|| k == M_IFLT	|| k == M_IFLE
    	    	    	|| k == M_ENDIF || k == M_ELSE
    	    	    	|| k == M_ENDIPR|| k == M_ENDREPT
    	    	    	|| k == M_ENDMACRO|| k == M_EXITM
    	    	    	|| k == M_LINE	|| k == M_FILE
    	    	    	|| k == M_MAC_S || k == M_MAC_E
    	    	    	|| k == M_PRINT
    	    	    )
    	    	  #endif
    	    	    exprFlg = 1;    /* defined()�L�� */
    	    	    StMbuf(Z.mac_chrs/*"#"*/);
    	    	    StMbuf(Z.M_name);
    	    	    goto LOOP;
    	    	}
    	    }
    	    if (V.mac_chr == V.mac_chr2) {
    	    	goto JJJJ;
    	    } else {
    	    	StMbuf(Z.mac_chrs/*"#"*/);
    	    	goto JJJ_LBL;
    	    }

    	} else if (Z.M_sym == V.mac_chr2 && !ISSPACE(*s)) { /*====== #���� ======*/
    	    s = M_GetSym(s);

    	    if (exprFlg && Z.M_sym == '(') {	    	    	    /*-- #(��) --*/
      JJJ_EXPR:
    	    	if (exprp) {
    	    	    /*Filn_Error("%cex()����%cex�����邱�Ƃ͏o���Ȃ�\n",V.mac_chr2,V.mac_chr2);*/
    	    	    goto LOOP;
    	    	}
    	    	exprFlg = 1;	/* defined()�L�� */
    	    	exprp = Z.M_mptr;
    	    	expr_r = 1;
    	    	/*expr_ty = k;*/
    	    	goto LOOP;
    	    }

    	  JJJ_STR_REF:
    	    if (Z.M_sym == '&' && V.opt_wq_mode) {
    	    	s = M_GetSym(s);
    	    	if (Z.M_sym == '"' && Z.M_str[0] == '"') {
    	    	    p2 = STREND(Z.M_str);
    	    	    *--p2 = 0;
    	    	    p = Z.M_str+1;
    	    	} else {
    	    	    p = Z.M_str;
    	    	}
    	    	StMbuf(p);
    	    	goto LOOP;
    	    } else if (Z.M_sym != 'A') {
//printf("!A2 %d %s\n",sh, Z.M_name);
    	    	StMbuf(Z.mac_chrs2/*"#"*/);
    	    	goto LOP2;
    	    }
    	    k = M_OdrSearch(Z.M_name);
    	    if (k < 0) {
    	  JJJJ:
//printf("JJJ %d %s\n",sh, Z.M_name);
    	    	if (/*v == 0 &&*/ exprFlg) {	/* #���߂łȂ����� */
    	    	    sh = 1/*V.mac_chr2*/;
    	    	    /*StMbuf(Z.mac_chrs2);*/
    	    	    goto JJJ_LBL;
    	    	} else {
    	    	    StMbuf(Z.mac_chrs2);
    	    	    goto LOP2;
    	    	    /*goto JJJ_LBL;*/
    	    	}
    	    }
    	    if (k == M_SELF) {
    	    	StMbuf(Z.mac_chrs2/*"#"*/);
    	    	if (v)
    	    	    StMbuf(Z.M_name);
    	    	goto LOOP;

    	  #if 0
    	    } else if (k == M_DEFINED) {
    	    	goto JJJ_DEFINED;

    	  #endif
    	    } else {
    	    	StMbuf(Z.mac_chrs2/*"#"*/);
    	      #if 0
    	    	goto JJJ_LBL;
    	      #else
    	    	StMbuf(Z.M_name);
    	    	goto LOOP;
    	      #endif
    	    }

    	} else if (exprp && Z.M_sym == '(') {
    	    expr_r++;

    	} else if (exprp && Z.M_sym == ')') {
    	    if (expr_r == 0) {
    	    	/*MM_MaccErr(fname, line);*/
    	    	Filn_Error("PrgErr:#ex ) \n");

    	    } else if (--expr_r == 0) {
    	    	l = M_Expr(exprp);
    	    	Z.M_mptr = exprp;
    	    	StMbuf(M_ImmStr(tmp, l, 0/*(expr_ty == M_WORD)?2:(expr_ty == M_BYTE)?1:0*/));
    	    	name[0] = 0;
    	    	exprp = NULL;
    	    }

    	} else if (Z.M_sym == 'A') {	    	    	    /*====== ���O�� ======*/
    	  #if 0
    	    if (V.mac_chr == -1 && gyoto == 1) {    /* �}�N���J�n���������ōs���̂Ƃ� */
    	    	switch (M_OdrSearch(Z.M_name)) {    	/* �}�N�����߂Ȃ�΂��̏�����*/
    	    	case M_DEFINE:	case M_UNDEF:	case M_IPR: 	case M_REPT:
    	    	case M_IF:  	case M_IFEQ:	case M_ELIF:	case M_ELSE:
    	    	case M_IFGE:	case M_IFGT:	case M_IFLE:	case M_IFLT:
    	    	case M_IFDEF:	case M_IFNDEF:	case M_SET:
    	    	    goto GYOTO_MAC;
    	    	}
    	    }
    	  #endif
    	    if (exprFlg && strcmp(Z.M_name, "defined") == 0) {	    /* defined(���x��)�� 0 �� 1 �ɕϊ� */
      /*JJJ_DEFINED:*/
    	    	/*printf("!%d\n",exprFlg);*/
    	      #if 1
    	    	c = M_ChkDefined(&s);
    	    	StMbuf(c?"1":"0");
    	      #else
    	    	s = M_GetSym(s);
    	    	k = 0;
    	    	c = 0;
    	    	if (Z.M_sym == '(') {
    	    	    k = 1;
    	    	    s = M_GetSym(s);
    	    	}
    	    	if (Z.M_sym == 'A') {
    	    	    c = (MTREE_Search(Z.M_name) != NULL) ? 1 : 0;
    	    	}
    	    	if (k) {
    	    	    s = M_GetSym(s);
    	    	    if (Z.M_sym != ')') {
    	    	    	/*MM_MaccErr(fname, line);*/
    	    	    	Filn_Error("defined()�̎w�肪��������!\n");
    	    	    }
    	    	}
    	    	StMbuf(c?"1":"0");
    	      #endif
    	    	goto LOOP;
    	    }
      JJJ_LBL:
    	    if (Z.MM_cnt) { /* ���ݓW�J���̃}�N�����Ɠ����Ȃ�΁A���̖��O�̂܂܂ɂ��� */
    	    	for (i = 0; i < Z.MM_cnt; i++) {
    	    	    if (strcmp(Z.M_name, Z.MM_name[i]) == 0) {
    	    	    	if (sh) StMbuf("\"");
    	    	    	StMbuf(Z.M_name);
    	    	    	if (sh) {StMbuf("\"");/*sh = 0;*/}
    	    	    	goto LOOP;
    	    	    }
    	    	}
    	    }
    	    if (m && v) {   /* �}�N���̈������i���[�J�����j�̂Ƃ� */
    	    	for (i = 0; i < m->argc; i++) {
    	    	    if (strcmp(Z.M_name, m->argv[i]) == 0) {
    	    	    	//printf("%s %s\n", Z.M_name, v[i]);
    	    	    	if (sh) StMbuf("\"");
    	    	    	MM_Macc(v[i], 1, NULL, NULL/*, fname, line*/, "<marg>");
    	    	    	if (sh) {StMbuf("\"");/*sh = 0;*/}
    	    	    	goto LOOP;
    	    	    }
    	    	}
    	    }
    	    if (m && v == NULL) {
    	    	/*MM_MaccErr(fname, line);*/
    	    	Filn_Error("PrgErr:macro v\n");
    	    }

    	    /* #rept, #ipr �Ŏw�肳�ꂽ���O�̂Ƃ� */
    	    for (i = 0; i < Z.rept_argc; i++) {
    	    	if (Z.rept_name[i] && Z.rept_name[i][0] && strcmp(Z.M_name, Z.rept_name[i]) == 0) {
    	    	    if (sh) StMbuf("\"");
    	    	    MM_Macc(Z.rept_argv[i], 1, NULL, NULL/*, fname, line*/, "<ipr arg>");
    	    	    if (sh) {StMbuf("\"");/*sh = 0;*/}
    	    	    goto LOOP;
    	    	}
    	    }
    	    if (v) {	/* ����������Ƃ��́A�P�Ɉ�����W�J���邾���ŁA���̒��g�̃}�N���ϊ��͖߂��Ă��炷�� */
    	    	if (sh) {StMbuf(Z.mac_chrs2/*"#"*/);/*sh = 0;*/}
    	    	StMbuf(Z.M_name);
    	    } else {
    	    	MTREE_T *t;

    	    	t = MTREE_Search(Z.M_name); /* �}�N������T�� */
    	    	//printf("MacroName %s 0x%x\n", Z.M_name, t);
    	    	if (t == NULL) {
    	    	    //if (sh)	{StMbuf(Z.mac_chrs2/*"#"*/); sh = 0;}
    	    	    if (sh) StMbuf("\"");
    	    	    StMbuf(Z.M_name);
    	    	    if (sh) {StMbuf("\"");/*sh = 0;*/}
    	    	} else {
    	    	    char *mmp,*tm;
    	    	    if (sh) StMbuf("\"");
    	    	    mmp = Z.M_mptr;
    	    	    s = MM_MaccMacro(s, 0/*sh*/, t);
#if 1
    	    	    //printf(">M\n");fflush(stdout);
    	    	    if (mmp && *mmp) {	/* �}�N���W�J�̈����u���̂��� */
    	    	    	tm = strdupE(mmp);
    	    	    	Z.M_mptr  = mmp;
    	    	    	/*mmp  = Z.M_mptr;*/
    	    	    	*Z.M_mptr = 0;
    	    	    	MM_Macc(tm, 1, m, v, "<macmac>");
    	    	    	SAFE_FREE(tm);
    	    	    }
    	    	    //printf("<M\n");fflush(stdout);
#endif
    	    	    if (sh) {StMbuf("\"");/*sh = 0;*/}
    	    	}
    	    }
    	} else {
    	    StMbuf(Z.M_str);
    	}
    }
//printf("@ Z.MM_nest %d\n", Z.MM_nest);fflush(stdout);
    Z.MM_nest--;
    return;
}


/** #macro
 */
static char const* MM_MaccMacro(char const* s, int sh, MTREE_T* t)
{
    char const*     p;
    char const*     q;
    char**  	    a;
    int     	    c,k,i;
    long    	    l;

  #if 0
//  MTREE_T *t;
//  /* �}�N������T�� */
//  t = MTREE_Search(Z.M_name);
//  if (t == NULL) {	    	    	/* �}�N�����łȂ������炻�̂܂܏o�� */
//  	if (sh) StMbuf(Z.mac_chrs2/*"#"*/);
//  	StMbuf(Z.M_name);
//  } else
  #endif

//printf("MaccMacro start\n");

    if (t->atr == M_ATR_SET) {	/* #set�Œ�`���ꂽ���O�̂Ƃ� */
    	sprintf(Z.MM_wk, S_FMT_D, t->argb);
    	if (sh) StMbuf("\"");
    	StMbuf(Z.MM_wk);
    	if (sh) StMbuf("\"");

    } else if (t->atr == M_ATR_DEF) {	/* �����Ȃ��}�N�����̂Ƃ� */
  L1:
    	Z.MM_name[Z.MM_cnt] = strdupE(Z.M_name);
    	if (++Z.MM_cnt > FILN_MAC_NEST)
    	    Filn_Exit("�}�N���W�J�ł̃l�X�g���[������(%s �W�J��)\n", Z.MM_name[0]);
    	if (sh) StMbuf("\"");
    	q = Z.errMacFnm, l = Z.errMacLin;
    	Z.errMacFnm = t->fname, Z.errMacLin = t->line;
    	MM_Macc(t->buf, 1, NULL, NULL /*, t->fname, t->line*/, Z.MM_name[Z.MM_cnt-1]);
    	Z.errMacFnm = q, Z.errMacLin = l;
    	if (sh) StMbuf("\"");
    	SAFE_FREE(Z.MM_name[--Z.MM_cnt]);

    } else if (t->atr == M_ATR_MAC) {	/* �֐��^�}�N�����̂Ƃ� */
    	/* �����������̂Ƃ��A����()������ΊO��, M_ATR_DEF�Ɠ������� */
    	if (t->argc == 0) {
    	    p = s;
    	    c = *(p = SkipSpc(p)); p++;
    	    if (c == '(') {
    	    	c = *(p = SkipSpc(p)); p++;
    	    	if (c == ')')
    	    	    s = p;
    	    }
    	    goto L1;
    	}

    	/* ��������}�N���̂Ƃ� */
    	Z.MM_name[Z.MM_cnt] = strdupE(Z.M_name);
    	if (++Z.MM_cnt > FILN_MAC_NEST)
    	    Filn_Exit("�}�N���W�J�ł̃l�X�g���[������(%s �W�J��)\n", Z.MM_name[0]);
    	a = callocE(sizeof(char*), t->argc+1);

    	/* ()�t���ǂ����̃`�F�b�N */
    	k = 0;
    	p = M_GetSym(s);
    	if (Z.M_sym == '(') {
    	    k = ')';
    	    s = p;
    	    if (t->argb == 0) { /* ���[�J�����x�������邯�ǁA�������Ȃ��Ƃ� */
    	    	s = M_GetSym(s);
    	    	goto KKK;
    	    }
    	}
    	/* ��`���̌����A�������擾 */
    	for (i = 0; i < t->argb; i++) {
    	    int r, k1, k2;
    	    /*r = k1 = k2 = 0;*/
    	    s = SkipSpc(s);
    	    p = s;
    	    do {
    	      J11:
    	    	q = s;
    	    	s = M_GetSym(s);
    	    	if (Z.M_sym == '(' || Z.M_sym == '[' || Z.M_sym == '{') {
    	    	    k1 = Z.M_sym;
    	    	    k2 = (k1 == '(') ? ')' : (k1 == '[') ? ']' : '}';
    	    	    r = 1;
    	    	    do {
    	    	    	q = s;
    	    	    	s = M_GetSym(s);
    	    	    	if (Z.M_sym == k1) {
    	    	    	    ++r;
    	    	    	} else if (Z.M_sym == k2) {
    	    	    	    --r;
    	    	    	    if (r == 0) {
    	    	    	    	/*k1 = k2 = 0;*/
    	    	    	    	goto J11;
    	    	    	    }
    	    	    	}
    	    	    	if (Z.M_sym == 0 || Z.M_sym == '\n')
    	    	    	    break;
    	    	    } while(r || Z.M_sym != ',');
    	    	    /*k2 = k1 = 0;*/
    	    	}
    	    } while (Z.M_sym != 0 && Z.M_sym != '\n' && Z.M_sym != ',' && Z.M_sym != k);
    	    a[i] = mallocE(q-p+1);
    	    strncpyZ(a[i], p, q-p+1);
    	    if (Z.M_sym != ',') {
    	    	i++;
    	    	break;
    	    }
    	}
    	if (i != t->argb || Z.M_sym == ',') {
    	    /*MM_MaccErr(fname, line);*/
    	    Filn_Error("�}�N�� %s �̈����̐�����v���Ȃ�\n", t->name);
    	}
    	if (k) {
    KKK:
    	    if (Z.M_sym != k) {
    	    	/*MM_MaccErr(fname, line);*/
    	    	Filn_Error("�}�N�� %s �̊���()�����Ă��Ȃ�\n", t->name);
    	    }
    	}
    	for (i = (int)t->argb; i < t->argc; i++) {   /* ���[�J�����̏��� */
    	    if (strlen(V.localPrefix) == 0)
    	    	Filn_Exit("%clocal ���������郉�x���̐擪�����񂪎w�肳��Ă��Ȃ�\n",V.mac_chr);
    	    sprintf(Z.MM_wk, "%s%u", V.localPrefix, Z.MM_localNo++);
    	    a[i] = strdupE(Z.MM_wk);
    	}
    	if (sh) StMbuf("\"");
    	q = Z.errMacFnm, l = Z.errMacLin;
    	Z.errMacFnm = t->fname, Z.errMacLin = t->line;
    	MM_Macc(t->buf, 1, t, a /*, t->fname, t->line*/, "<macdef>"/*Z.MM_name[Z.MM_cnt-1]*/);
    	Z.errMacFnm = q, Z.errMacLin = l;
    	if (sh) StMbuf("\"");
    	for (i = 0; i < t->argc; i++)
    	    SAFE_FREE(a[i]);
    	SAFE_FREE(a);
    	SAFE_FREE(Z.MM_name[--Z.MM_cnt]);

    } else if (t->atr == M_ATR_RSV) {	/* �\��ꃉ�x���������Ƃ� */
    	MM_MaccMacroAtrRsv((int)t->argb, t->name);

    } else {	/* �L���������� */
    	if (sh) StMbuf(Z.mac_chrs2/*"#"*/);
    	StMbuf(Z.M_name);
    }
    //printf("MaccMacro end\n");
    return s;
}


static void MM_MaccMacroAtrRsv(int argb, char const* t_name)
{
    char    name[FILN_NAME_SIZE+2];

    switch(argb) {
    case M_RSV_FILE:
    	StMbuf("\"");
    	StMbuf(Z.inclp->name);
    	StMbuf("\"");
    	break;
    case M_RSV_LINE:
    	sprintf(name,"%u",Z.inclp->line);
    	StMbuf(name);
    	name[0] = 0;
    	break;
    case M_RSV_DATE:
    	{time_t tp; struct tm *tim;
    	    tp = time(NULL);
    	    if (tp == (time_t)-1) tp = 0;
    	    tim = localtime(&tp);
    	    strftime(name,(sizeof name)-1, "\"%b %d %Y\"", tim);
    	}
    	StMbuf(name);
    	name[0] = 0;
    	break;
    case M_RSV_TIME:
    	{time_t tp; struct tm *tim;
    	    tp = time(NULL);
    	    if (tp == (time_t)-1) tp = 0;
    	    tim = localtime(&tp);
    	    strftime(name,(sizeof name)-1, "\"%H:%M:%S\"", tim);
    	}
    	StMbuf(name);
    	name[0] = 0;
    	break;
    default:
    	/*MM_MaccErr(fname, line);*/
    	Filn_Error("PrgErr:�\��#define��(%s)\n",t_name);
    }
}


/** #set
 */
static void MM_Macc_MSet(char const* name, char const* s)
{
    val_t   n;
    char    buf[1024];

//printf("set<%s>\n",s);
    n = M_Expr(s);
    s = Z.M_ep;
    /*if (Z.M_sym != '\n' && Z.M_sym != 0)*/
    M_ChkEol(Z.M_sym, s);
    sprintf(buf, S_FMT_D, n);
    MTREE_Add(name, M_ATR_SET, n, 0, NULL, strdupE(buf), 0);
//printf("set>> %s = %ld\n", name, n);
}


/** #rept
 */
static void MM_Macc_MRept(char const* name, char* d, char const** s0 /*, char *fname, uint32_t line*/)
{
    int     	r,c,i,num, o;
    char const* s;
    char const* p;
    char*   	q;
    char    	wk[18];
    char const* fname;
    uint32_t	line;
    uint32_t	n;

    /* �G���[�pmacro,rept,ipr�J�n�ʒu��ۑ� */
    fname = Z.errMacFnm, line = Z.errMacLin+1, n = 0;

    /* ���s�[�g�񐔂����߂� */
    num = (int)M_Expr(d);
//printf("{rept}%s(%ld)= <<<%s>>>\n",name,num,d);
    Z.M_mptr = d;
    *Z.M_mptr = 0;
    M_ChkEol(Z.M_sym, Z.M_ep);

    /* �Ή����� #endrept ��T�� */
    s = *s0;
    r = 0;
    goto J1;
    for (; ;) {
    	/* �܂����s��T�� */
    	p = strchr(s, '\n');
    	if (p == NULL) {
    	    s = STREND(s);
    	    Filn_Error("�Ή����� %cendrept ���Ȃ�\n",V.mac_chr);
    	    *s0 = s;
    	    return;
    	}
    	s = p+1;
      J1:
    	s = SkipSpc(s);
    	if (M_ChkMchr(s)) {
    	    p = s;
    	    s = M_GetSym(s+1);
    	    TOEOS(s);
    	    o = M_OdrSearch(Z.M_name);
    	    if (o == M_REPT) {
    	    	r++;
    	    	//DB printf("<rept ++r=%d>\n",r);
    	    } else if (o == M_ENDREPT) {
    	    	if (r == 0)
    	    	    break;
    	    	--r;
    	    	//DB printf("<rept --r=%d>\n",r);
    	    }
    	}
    }
 #if 1
    q = strndupE(*s0, p - *s0 + 1);
 #else
    c = *p;
    *p = 0;
    q = strdupE(*s0);
    *p = c;
 #endif
    *s0 = s;
    if (Z.rept_argc < 0)
    	Filn_Exit("PrgErr:rept_argc\n");
    if (Z.rept_argc >= FILN_REPT_NEST) {
    	Filn_Exit("%crept �̃l�X�g���[������\n",V.mac_chr);
    }
    Z.rept_name[Z.rept_argc] = NULL;
    Z.rept_argv[Z.rept_argc] = NULL;
    if (name[0]) {
    	Z.rept_name[Z.rept_argc] = name;
    	Z.rept_argv[Z.rept_argc] = wk;
    }
    Z.rept_argc++;

    for (i = 0; i < num; i++) {
    	sprintf(wk, "%d", i);
    	Z.errMacFnm = fname, Z.errMacLin = line;
    	MM_Macc(q, 1, NULL, NULL/*, fname, line*/, "<rept arg>");
    }
    --Z.rept_argc;
    Z.rept_name[Z.rept_argc] = NULL;
    Z.rept_argv[Z.rept_argc] = NULL;
    SAFE_FREE(q);

    /* �G���[�pmacro,rept,ipr�J�n�ʒu���X�V */
    Z.errMacFnm = fname, Z.errMacLin = line + n;
}



/** #ipr
 */
static void MM_Macc_MIpr(char const* name, char const** s0, int argc, char const* const* argv/*, char *fname, uint32_t line*/)
{
    int     	r,c,i,o;
    char const* s;
    char const* p;
    char*   	q;
    char const* fname;
    uint32_t	line,n;

    /* �G���[�pmacro,rept,ipr�J�n�ʒu��ۑ� */
    fname = Z.errMacFnm, line = Z.errMacLin+1, n = 0;

    /* �Ή����� #endipr ��T�� */
    s = *s0;
    r = 0;
    goto J1;
    for (; ;) {
    	/* �܂����s��T�� */
    	p = strchr(s, '\n');
    	if (p == NULL) {
    	    s = STREND(s);
    	    Filn_Error("�Ή����� %cendipr ���Ȃ�\n",V.mac_chr);
    	    *s0 = s;
    	    return;
    	}
    	s = p+1;
    	n += 1;
      J1:
    	s = SkipSpc(s);
    	if (M_ChkMchr(s)) {
    	    p = s;
    	    s = M_GetSym(s+1);
    	    TOEOS(s);
    	    o = M_OdrSearch(Z.M_name);
    	    if (o == M_IPR) {
    	    	r++;
    	    } else if (o == M_ENDIPR) {
    	    	if (r == 0)
    	    	    break;
    	    	--r;
    	    }
    	}
    }
 #if 1
    q = strndupE(*s0, p - *s0 + 1);
 #else
    c = *p;
    *p = 0;
    q = strdupE(*s0);
    *p = c;
 #endif
    *s0 = s;
    if (argc < 0)
    	Filn_Exit("PrgErr:Z.rept_argc. %cipr\n",V.mac_chr);
    if (Z.rept_argc >= FILN_REPT_NEST) {
    	Filn_Exit("%cipr, %crept �̃l�X�g���[������\n",V.mac_chr,V.mac_chr);
    }
    Z.rept_name[Z.rept_argc] = name;
    Z.rept_argc++;
    for (i = 0; i < argc; i++) {
    	Z.rept_argv[Z.rept_argc-1] = argv[i];
    	Z.errMacFnm = fname, Z.errMacLin = line;
    	MM_Macc(q, 1, NULL, NULL/*, fname, line*/, "<ipr>");
    }
    --Z.rept_argc;
    Z.rept_name[Z.rept_argc] = NULL;
    Z.rept_argv[Z.rept_argc] = NULL;
    SAFE_FREE(q);

    /* �G���[�pmacro,rept,ipr�J�n�ʒu���X�V */
    Z.errMacFnm = fname, Z.errMacLin = line + n;
}



/** �}�N���W�J���[�h�t��1�s����
 */
static char *Filn_GetLineMac(void)
{
    char const* s;
    char*   	p;
    long    	l;
    int     	c,o;

  RETRY:
    /* �}�N���W�J���ꂽ������o�b�t�@���P�s���o�� */
    Z.errMacFnm = NULL, Z.errMacLin = 0;
    if (Z.MM_ptr) {
    	p = Z.linbuf;
    	while ((c = *Z.MM_ptr++) != '\n') {
    	    if (c == '\0') {
    	    	Z.MM_ptr = NULL;
    	    	break;
    	    }
    	    *p++ = c;
    	}
    	//*p++ = '\n';
    	*p = 0;
    	return Z.linbuf;
    }

  GETL: /* �t�@�C������s����(�s�A���ς�) */
    s = Filn_GetLine();
    if (s == NULL) {
    	Z.linbuf[0] = 0;
    	return NULL;
    }

    /* #if�ŋU�̏��������Ă���Ƃ� */
    if (Z.MM_mifChk) {
    	s = SkipSpc(s);
    	strcpy(Z.M_mbuf, s);
    	Z.MM_ptr = Z.M_mbuf;
    	goto RETRY;
    }

    /* #macro #rept �̏��� */
    s = SkipSpc(s);
    c = *s;
    if (M_ChkMchr(s)) {
    	s = M_GetSym(s+1);
    	s = SkipSpc(s);
    	/*c = *s;*/
    	if (Z.M_sym != 'A') {
    	    Filn_Error("%c�s�̖��߂���������\n",V.mac_chr);
    	    goto GETL;
    	}
    	o = M_OdrSearch(Z.M_name);
    	if (o == M_MACRO) {
    	    if (M_Macro(s)) {
    	    	char**	a = NULL;
    	    	int 	i;
    	    	int 	ac = ((MTREE_T*)Z.macBgnStr)->argc;
    	    	/* ���O�Ȃ��܂���̂Ƃ��̏��� */
    	    	if (ac) {
    	    	    a = callocE(sizeof(char*), ac);
    	    	    for (i = 0; i < ac; i++) {	 /* ���[�J�����̏��� */
    	    	    	if (strlen(V.localPrefix) == 0)
    	    	    	    Filn_Exit("%clocal ���������郉�x���̐擪�����񂪎w�肳��Ă��Ȃ�\n",V.mac_chr);
    	    	    	sprintf(Z.MM_wk, "%s%u", V.localPrefix, Z.MM_localNo++);
    	    	    	a[i] = strdupE(Z.MM_wk);
    	    	    }
    	    	}
    	    	InitStMbuf();
    	    	Z.errMacFnm = Z.inclp->name, Z.errMacLin = Z.inclp->line;
    	    	MM_Macc(((MTREE_T*)Z.macBgnStr)->buf, 1, (a ? ((MTREE_T*)Z.macBgnStr) : NULL), a/*, s, l*/, "<macbgn>");
    	     #if 1
    	    	M_FreeArg(&ac, &a);
    	     #else
    	    	if (((MTREE_T*)Z.macBgnStr)->argc) {
    	    	    for (i = 0; i < ((MTREE_T*)Z.macBgnStr)->argc; i++) {
    	    	    	SAFE_FREE(a[i]);
    	    	    }
    	    	    SAFE_FREE(a);
    	    	}
    	     #endif
    	    	Z.errMacFnm = NULL, Z.errMacLin = 0;
    	    	Z.MM_ptr = Z.M_mbuf;
    	    	M_Undef("_macro\xff_");
    	    	goto RETRY;
    	    }
    	    goto GETL;
    	} else if (o == M_IPR) {
    	    InitStMbuf();
    	    s = Z.inclp->name;
    	    l = Z.inclp->line;
    	    M_Ipr(Z.linbuf);	    	    /* ��U#endipr �܂œǂݍ��� */
    	    goto RPT;
    	} else if (o == M_REPT) {
    	    InitStMbuf();
    	    s = Z.inclp->name;
    	    l = Z.inclp->line;
    	    M_Rept(Z.linbuf);	    	    /* ��U#endrept�܂œǂݍ��� */
    	  RPT:
    	    p = strdupE(Z.M_mbuf);  	/* �o�b�t�@�ɒ��� */
    	    InitStMbuf();   	    	    /* �}�N������#rept�Ɠ��l�ɏ������� */
    	    Z.errMacFnm = s, Z.errMacLin = l;
    	    MM_Macc(p, 1, NULL, NULL/*, s, l*/, "<iprpt>");
    	    Z.errMacFnm = NULL, Z.errMacLin = 0;
    	    SAFE_FREE(p);
    	    Z.MM_ptr = Z.M_mbuf;
    	    goto RETRY;
    	}
    } else if (c == 0) {    /* ��s�̂Ƃ� */
    	if (V.opt_delspc) { /* �󔒂����k����Ȃ���s���X�L�b�v */
    	    goto GETL;
    	} else if (Z.sl_lst) {	/* �\�[�X�}������Ȃ���s�X�L�b�v */
    	    goto GETL;
    	} else {    	    /* ��s�̂܂� */
    	    //strcpy(s, "\n");
    	}
    }

    /* �P�s����U�V���{���ɕ������ēǍ��ݍēx�������P�s�ɂ���B */
    /* �r���}�N��������ΓW�J���� */
    InitStMbuf();
    Z.errMacFnm = NULL, Z.errMacLin = 0;
    MM_Macc(Z.linbuf, 0, NULL, NULL/*, NULL, 0*/,"<linbuf>");
    Z.errMacFnm = NULL, Z.errMacLin = 0;
//printf("T>%s\n",Z.M_mbuf);
    Z.MM_ptr = Z.M_mbuf;
    goto RETRY;
}



/** ��s����. #�Ŏn�܂�s�̏������s��
 */
static char *Filn_GetsMif(void)
{
    char const* s;
    int     	n;
    val_t   	val;

  RETRY:
    /* ��s���� */
    Z.MM_mifChk = (Z.mifChkSnc || Z.mifStk[Z.mifCur] >= 2);
    s = Filn_GetLineMac();
    if (s == NULL) {
    	Z.linbuf[0] = 0;
    	if (Filn_Close())
    	    return NULL;
    	goto RETRY;
    }
//printf(">%s\n",s);

    s = SkipSpc(s); 	    	    	/* �s���󔒃X�L�b�v */
    if (M_ChkMchr(s) == 0) {	    	/* �v���v���Z�X�s�łȂ���Α����A�߂� */
    	if (Z.MM_mifChk)    	    	    /* #if�Ŗ����������Ȃ�Ύ��̍s��ǂݍ��� */
    	    goto RETRY;
    	return Z.linbuf;
    }

//printf("mifCur:%d Z.mifStk:%d %s\n", Z.mifCur, Z.mifStk[Z.mifCur], s);
    s = M_GetSym(s+1);
    if (Z.M_sym != 'A')
    	goto E1;
    n = M_OdrSearch(Z.M_name);
    if (n < 0) {
  E1:
    	Filn_Error("%c�s�̖��߂���������\n",V.mac_chr);
    	goto RETRY;
    }
    s = SkipSpc(s);

    /* #�����R���p�C���ł̓ǂݔ�΂����� */
    if (Z.mifStk[Z.mifCur] >= 2) {
    	switch(n) {
    	case M_IF:  case M_IFEQ:/* case M_IFNE:*/
    	case M_IFGT:case M_IFGE: case M_IFLT:case M_IFLE:
    	case M_IFDEF:case M_IFNDEF:
    	    ++Z.mifChkSnc;
    	    goto RETRY;
    	case M_ENDIF:
    	    if (Z.mifChkSnc) {
    	    	--Z.mifChkSnc;
    	    	goto RETRY;
    	    }
    	    break;
    	case M_INCLUDE:
    	case M_MAC_S:
    	case M_MAC_E:
    	case M_EXITM:
    	case M_PRINT:
    	case M_ERROR:
    	case M_PRAGMA:
    	case M_FILE:
    	case M_LINE:
    	case M_SET:
    	case M_REPT:
    	case M_IPR:
    	case M_DEFINE:
    	case M_UNDEF:
    	case M_MACRO:
    	case M_BEGIN:
    	case M_FOR:
    	case M_LOCAL:
    	case M_END:
    	case M_ENDMACRO:
    	case M_ENDFOR:
    	case M_ENDREPT:
    	case M_ENDIPR:
    	case M_SELF:
    	    goto RETRY;
    	default:    // M_ELSE, M_ELIF
    	    if (Z.mifChkSnc)
    	    	goto RETRY;
    	}
    }
    Z.mifChkSnc = 0;

    switch (n) {
    case M_INCLUDE:
    	M_Include(s);
    	break;

    case M_IF:	case M_IFEQ:/* case M_IFNE:*/
    case M_IFGT:case M_IFGE: case M_IFLT:case M_IFLE:
    	if (Z.mifStk[Z.mifCur] < 2) {
    	    val = M_Expr(s);
    	    s = Z.M_ep;
    	    M_ChkEol(Z.M_sym, s);
    	    if (Z.mifCur >= FILN_IF_NEST)
    	    	Filn_Exit("%cif �̃l�X�g���[������",V.mac_chr);
    	    switch (n) {
    	    /*case M_IFNE:*/
    	    case M_IF:	val = (val != 0); break;
    	    case M_IFEQ:val = (val == 0); break;
    	    case M_IFGT:val = (val >  0); break;
    	    case M_IFGE:val = (val >= 0); break;
    	    case M_IFLT:val = (val <  0); break;
    	    case M_IFLE:val = (val <= 0); break;
    	    }
    	    Z.mifStk[++Z.mifCur] = (val) ? 1 : 2;
    	}
//  	/*printf("mifCur:%d mifStk:%d %d\n", Z.mifCur, Z.mifStk[Z.mifCur], val);*/
    	break;

    case M_ELIF:
    	if (Z.mifStk[Z.mifCur] <= 2) {
    	    val = M_Expr(s);
    	    s = Z.M_ep;
    	    M_ChkEol(Z.M_sym, s);
    	    if (Z.mifStk[Z.mifCur] == 2)
    	    	Z.mifStk[Z.mifCur] = (val) ? 1 : 2;
    	    else if (Z.mifStk[Z.mifCur] == 0)
    	    	Filn_Error("%celif �����܂��Ă���",V.mac_chr);
    	    else
    	    	Z.mifStk[Z.mifCur] = 3;
    	}
    	break;

    case M_ELSE:
    	M_ChkEol(0, s);
    	if (Z.mifStk[Z.mifCur] == 2)
    	    Z.mifStk[Z.mifCur] = 1;
    	else if (Z.mifStk[Z.mifCur] == 0/* || Z.mifStk[Z.mifCur] == 3*/)
    	    Filn_Error("%celse �����܂��Ă���",V.mac_chr);
    	else
    	    Z.mifStk[Z.mifCur] = 3;
//printf("@>else %d:%d\n", Z.mifCur, Z.mifStk[Z.mifCur]);
    	break;

    case M_ENDIF:
    	M_ChkEol(0, s);
    	Z.mifStk[Z.mifCur] = 0;
    	if (Z.mifCur == 0)
    	    Filn_Error("%cendif �����܂��Ă�",V.mac_chr);
    	else
    	    --Z.mifCur;
//printf("@>endif %d\n", Z.mifCur);
    	break;

    case M_MAC_S:
    	M_ChkEol(0, s);
    	if (Z.mifMacCur == FILN_IF_NEST)
    	    Filn_Exit("%c�}�N���̃l�X�g���[�����܂�\n",V.mac_chr);
    	Z.mifMacStk[++Z.mifMacCur] = Z.mifCur;
    	break;

    case M_MAC_E:
    	M_ChkEol(0, s);
    	if (Z.mifMacCur == 0)
    	    Filn_Exit("PrgErr:#<E>\n");
    	val = Z.mifMacStk[Z.mifMacCur--];
    	if ((int)val != Z.mifCur) {
    	    Filn_Error("�}�N������%cif�̑Ή������Ă��Ȃ�\n",V.mac_chr);
    	}
    	Z.mifCur = (int)val;
    	break;

    case M_EXITM:
    	M_ChkEol(0, s);
    	if (Z.mifStk[Z.mifCur] >= 2)
    	    break;
    	if (Z.mifMacCur == 0) {
    	    Filn_Error("%c�}�N���O�� %cexitmacro �����ꂽ\n",V.mac_chr,V.mac_chr);
    	    break;
    	}
    	/* �}�N���̏I���܂ŁA�ǂݔ�΂� */
    	val = 0;
    	for (; ;) {
    	    s = Filn_GetLineMac();
    	    if (s == NULL) {
    	    	Filn_Error("�}�N���W�J�r��(%cexitmacro�E�o��)�� EOF �ƂȂ���!?\n",V.mac_chr);
    	    	return NULL;
    	    }
    	    if (*s == V.mac_chr) {
    	    	if (strcmp(s+1,"_S\xFF_") == 0) {   	    /* �}�N���̎n�܂肪�����ꂽ */
    	    	    val++;
    	    	} else if (strcmp(s+1,"_E\xFF_") == 0) {    /* �}�N���̏I��肪������ */
    	    	    if (val == 0)   	    	    	    /* �Ή�����}�N���̎n�܂肪�Ȃ���ΒT�����߂��}�N���̏I��肾 */
    	    	    	break;
    	    	    --val;
    	    	}
    	    }
    	}
    	if (Z.mifMacCur == 0)
    	    Filn_Exit("PrgErr:#<E>.\n");
    	Z.mifCur = Z.mifMacStk[Z.mifMacCur--];	    	    /* �����I�� #if �̃l�X�g�𒲐����� */
    	break;

    case M_PRINT:
    	if (Z.mifStk[Z.mifCur] < 2)
    	    M_Print(s);
    	break;

    case M_ERROR:
    	if (Z.mifStk[Z.mifCur] < 2)
    	    Filn_Error("[ERROR] %s\n",s);
    	break;

    case M_PRAGMA:
    case M_FILE:
    case M_LINE:
    	break;

    case M_SET:
    case M_IFDEF:
    case M_IFNDEF:
    case M_REPT:
    case M_IPR:
    case M_DEFINE:
    case M_UNDEF:
    case M_MACRO:
    	Filn_Error("PrgErr:%c�Ŏn�܂�s\n",V.mac_chr);
    	break;

    case M_BEGIN:
    case M_FOR:

    case M_LOCAL:
    case M_END:
    case M_ENDMACRO:
    case M_ENDFOR:
    case M_ENDREPT:
    case M_ENDIPR:
    	Filn_Error("%c%s �̑΂ƂȂ�ׂ����̂��Ȃ�\n", V.mac_chr, Z.M_name);
    	break;

    default:
    	Filn_Error("�\�����Ȃ� %c �Ŏn�܂�s������\n", V.mac_chr);
    }

    goto RETRY;
}




/*--------------------------------------------------------------------------*/


/** �}�N���W�J�t�P�s����. malloc ������������Ԃ��̂ŁA�Ăяo�����ŊJ�����邱�ƁB
 * �Ō�ɕK�� '\n' ����
 */
char *Filn_Gets(void)
{
    char*   	s;
    SRCLIST*	sl;

  RETRY:
    /* �}�N���W�J�O�̃\�[�X���R�����g��s�ԍ�/�t�@�C�����Ƃ��ē��͂��� */
    sl = Z.sl_lst;
  #if 0
    if (Z.sl_buf) {
    	free(Z.sl_buf);
    	Z.sl_buf = NULL;
    }
  #endif
    if (sl) {
    	s   	    = sl->s;
    	//Z.sl_buf  = s;
    	Z.sl_lst    = sl->link;
    	sl->s	    = NULL;
    	SAFE_FREE(sl);
    	return s;
    } else {
    	s = Filn_GetsMif();
    	if (s) {
    	    char *m,*p;
    	    m = mallocE(strlen(s) + 4 + V.getsAddSiz);
    	    p = STPCPY(m, s);
    	    *p++ = '\n';
    	    *p = 0;
    	    s = m;
    	} else {
    	    /* s == NULL �� EOF */
    	}
    	if (Z.sl_lst == NULL) {
    	    //Z.sl_buf	= s;
    	    return s;
    	}
    	SRCLIST_Add(&Z.sl_lst, s);
    	goto RETRY;
    }
}



/** ���ݓ��͒��̃t�@�C�����ƍs�ԍ��𓾂�
 */
void Filn_GetFnameLine(char const** s, int* line)
{
    *s	  = Z.inclp->name;
    *line = Z.inclp->line;
}



/** name ��define ����Ă��邩���ׁA����Ă����0�ȊO��Ԃ��B
 * ���̂Ƃ� *strp�ɒ�`������ւ̃|�C���^�����ĕԂ�
 */
int Filn_GetLabel(char const* name, char const** strp)
{
    MTREE_T* t;

    t = MTREE_Search(name);
    if (t) {
    	if (strp)
    	    *strp = t->buf;
    	return 1;
    }
    return 0;
}


/** �}�N���� name �� ���e st �œo�^����.
 * st��NULL�Ȃ�� C�R���p�C���� -D�d�l��name����=������΂���ȍ~�̕�������A�łȂ����"1"��ݒ肷��
 */
int Filn_SetLabel(char const* name, char const* st)
{
    char const* p;

    p = M_GetSym((char*)name);
    if (Z.M_sym == 'A') {
    	if (st == NULL) {
    	    if (*p == '=') {
    	    	p++;
    	    } else {
    	    	p = "1";
    	    }
    	} else {
    	    p = st;
    	}
    	MTREE_Add(strdupE(Z.M_name), M_ATR_DEF, 0, 0, NULL, strdupE(p), 1);
    	return 1;
    } else {
    	return 0;
    }
}


/** ���O s �� undef ����
 */
int Filn_UndefLabel(char const* s)
{
    M_Undef(s);
    return 1;
}




/*--------------------------------------------------------------------------*/
/*�՗�

1996 �t
    	����A�Z���u���v���v���Z�b�T�p�̃}�N���������[�`���̃f�o�b�O��
    	���˂� cpp ���ǂ��Ƃ��č쐬.
    	watcom-c386 �ŃR���p�C��

1996 ��
    	�n���68k�A�Z���u���̂��߂̉��������X.
    	�}�N���J�n������ # �ȊO�ɂł���悤��.

1997-09-27
    	bcc32 �ŃR���p�C�������痎���܂���̂�, �C��.
    	���̑��ׁX�����o�O�Ƃ聕�d�l�ύX�ǉ�.
    	��`�t�@�C��(.cfg)�ǂݍ��݂��\�ɁB
    	defined() ��#�s�ł̂ݗL���Ƃ�(ansi-cpp�ɍ��킹��)�A#defined() ��
    	�ǂ��ł��g���邱�Ƃɂ���.
    	�R�����g�J�n�����w�� -pe -pet ���w��ł���悤�ɂ���.
    	68�n�A�Z���u���p�� -prq,prd ���w��ł���悤�ɂ���.
    	-p�֌W�̃I�v�V�����E�w���v�𕪗������݂̐ݒ���\������悤�ɕύX.
    	�h�L�������g����.

1998-12-09
    	#if 0 �` #endif ���ł� #���߂���������Ă��܂��A#define�Ȃǂ�
    	�G���[�ɂȂ����̂��C���B
    	�}�N���W�J�����������̂��ꕔ�C��.
    	���A�}�N�������̏��������������A�}�N�����ɑ��̃}�N����p���A
    	���̈����̒�`���������Ƃ��A�����̃}�N���̓W�J�ŊO���̃}�N����
    	�������g���Ă��܂��Ƃ����o�O���c���Ă���.

1998-12-19
    	�}�N���ƈ����̓W�J�̏��Ԃ����������̂𒼂��B�Ƃ肠�����A�����
    	�������i�Ǝv��...)
    	#defined(),><���Z�q�̔p�~.
    	'������', "������" �̈����� ����������ϐ���t��

    	�O���[�o���ϐ������ׂč\���̂ɂ܂Ƃ߂Ȃ����B�܂��A�v���t�B�b�N�X
    	��FILN �łȂ� Filn �ɂ���B
    	FILN_GetsMac �� Filn_Gets�ɕύX.
    	Filn_ErrOpen/ErrClose, Filn_WrtOpen/WrtClose, Filn_printf��t��
    	���ė��p���₷������.

2000-09-19
    	#endif �̃l�X�g�`�F�b�N���~�X���Ă���A#if�֌W�̋�����������������
    	�̂��C���B
    	���ŗ��p���₷���悤�ɁA�W�����C�u�����w�b�_�ƁAfiln.h����
    	�ǂ܂Ȃ��ł��ނ悤�ɕύX�B

2000-09-20
    	���\�[�X�̃R�����g�o�́i�܂��̓t�@�C�����s�ԍ�)���A��߁A���͕�����
    	�ɂȂ�悤�ɕύX�B���̓s���AFiln_Gets() ��malloc������������Ԃ��A
    	�J���͌Ăяo�����ɔC�����Ƃɂ��A�܂��A���s'\n'�͊O�����t���ĕԂ��悤�ɂ����B
    	Filn_WrtOpen/WrtClose, Filn_printf��p�~�B
    	�G���[�o�͂�W���o�͂łȂ��A�W���G���[�o�͂ɁB
    	#include <file> �� c �Ɠ��l�̌������ɕύX�B
    	include �p�X��;�ŋ�؂��ĕ�������x�Ɏw��\�ɁB
    	M_Expr0 �̃g�[�N���̎��ԈႢ���������̂��C���i���R�A#if�̌��ʂ����������Ȃ�j

2017-08-12
    	���̒l�� long long �ɕύX. �|�C���^���\�Ȃ�const�ɕύX. utf-8�Ή�
    	����������R��̏C��
*/
