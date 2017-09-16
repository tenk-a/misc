/*
    mwfnd
    1995/??/??  v1.00   �쐬
    1996/??/??  v1.01   -m ���[�h��t��
    1997/08/17  v1.02   usage�C��. bcc32�ōăR���p�C�����邽�߂̏C��.
    2007/04/28  v1.03   -l�ǉ�. �\�[�X���`.
    200?/??/??  v1.04   -l0�Ή�. -i�ǉ�. -a�ǉ�.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include "tree.h"


/** �g�����\��.
 */
void usage(void)
{
    puts("[mwfnd] �����̃t�@�C�����畡���̒P����������A�e�X�̌��������ʒu���o��\n"
         "usage mwfnd [-option(s)] <file(s)>\n"
         "  -oNAME    �o�̓t�@�C���w��\n"
         "  --        ��������̖��O�̓t�@�C����\n"
         "  ++        ��������̖��O�͌����P�ꖼ\n"
         "  -q        �Q�Ƃ̖����P��͏o�͂��Ȃ�\n"
         "  -n        �Q�Ƃ͏o�͂������������P��̂ݏo��\n"
         "  -lN       �Q�Ƃ�N�ȉ��̐��̂Ƃ��̂ݏo��\n"
         "  -m[-]     ���������s�̓��e��ێ�(�f�t�H���g)  -m- ���Ȃ�\n"
         "  -i        ���O�̑啶���������𖳎�\n"
         "  -aCHARS   �p�� _ �ȊO�̕����𖼑O�Ɏg����悤�ɂ���\n"
         "  -xN       �o�̓��[�h\n"
         "  -v-       ���b�Z�[�W�o��off\n"
         "  @RESFILE  ���X�|���X�E�t�@�C������\n");
    exit(0);
}



/*------------------------------------------------------------------------*/
typedef unsigned char UCHAR;

#ifdef UNIX                 /** utf8|euc-jp��z��. */
#define FNAME_ISDIRSEP(c)   ((c) == '/' || (c) == ':')
#define ISKANJI(c)          (0)
#define STRCASECMP(a,b)     strcasecmp(a,b)
#else
#define FNAME_ISDIRSEP(c)   ((c) == ':' || (c) == '/' || (c) == '\\')
#define ISKANJI(c)          ((UCHAR)(c) >= 0x81 && ( (UCHAR)(c) <= 0x9F || ((UCHAR)(c) >= 0xE0 && (UCHAR)(c) <= 0xFC)))
/*#define ISKANJI(c)        ((unsigned)((c)^0x20) - 0xA1 < 0x3C) */
/*#define ISKANJI2(c)       ((UCHAR)(c) >= 0x40 && (UCHAR)(c) <= 0xfc && (c) != 0x7f) */
#define STRCASECMP(a,b)     stricmp(a,b)
#endif


#ifdef DOS16    // 16bit MS-DOS �̏ꍇ
#define MAX_PATH        (264)
#define LINE_SIZE       (1024)
#define TOKENNAME_SIZE  (260)
#define MLINE           0
#else           // 32�r�b�g���̏ꍇ.
#define MAX_PATH        (0x2000)
#define LINE_SIZE       (0x8000)
#define TOKENNAME_SIZE  (0x1000)
#define MLINE           1
#endif



int     opt_qk          = 0;    /**< on�Ȃ� �Q�Ƃ̖����P��͕\�����Ȃ�.     */
int     opt_msgFlg      = 1;    /**< �o�߃��b�Z�[�W��\������/���Ȃ�        */
int     opt_nameOnly    = 0;    /**< ���O�����\�����Ȃ�.                    */
int     opt_ex          = 0;    /**< file line : word �ŕ\��                */
int     opt_ltNum       = -1;   /**< �񕉂̎w�萔�ȏ�Q�Ƃ�����Ȃ�\����.  */
int     opt_ignorecase  = 0;    /**< �啶���������̋�ʂ����Ȃ�             */
char    opt_outname[MAX_PATH];  /**< �o�̓t�@�C����.                        */
char*   opt_addnamechr  = "";   /**< �p��_�ȊO��(�L��)�����𖼑O�Ɋ܂߂�    */

#ifdef MLINE
int     opt_linmemFlg   =MLINE; /**< ���������s�̓��e��ێ�.              */
#endif



/** �I�v�V�������
 */
void options(char *s)
{
    char    *p;
    int     c;

    p   = s;
    p++;
    c   = *p++;
    c   = toupper(c);

    switch (c) {
    case 'Q':       /* �Q�Ƃ̖����P��͕\�����Ȃ�.      */
        opt_qk = 1;
        break;

    case 'V':       /* �o�߃��b�Z�[�W��\������/���Ȃ�  */
        opt_msgFlg = 1;
        if (*p == '-')
            opt_msgFlg = 0;
        break;

    case 'X':       /* �\�����[�h. */
        if (*p == 0)
            opt_ex = 1;
        else
            opt_ex = strtoul(p, NULL, 10);
        break;

    case 'N':       /* �Q�Ƃ͕\���������������P��̂ݏo��. */
        opt_nameOnly = 1;
        break;

    case 'L':       /* �Q�Ƃ�N�ȉ��̐��̂Ƃ��̂ݏo��        */
        opt_ltNum    = strtoul(p, NULL, 0);
        break;

    case 'I':
        opt_ignorecase = (*p != '-');
        break;

    case 'A':
        opt_addnamechr = strdup(p);
        break;

  #ifdef MLINE
    case 'M':       /* ���������s�̓��e��ێ� */
        opt_linmemFlg     = 1;
        if (*p == '-')
            opt_linmemFlg = 0;
        break;
  #endif

    case 'O':       /* �o�̓t�@�C���w��         */
        strncpy(opt_outname, p, MAX_PATH-1);
        opt_outname[MAX_PATH-1] = '\0';
        break;

    /* case '\0': */
    case 'H':
    case '?':       /* �w���v                   */
        usage();

    default:
        printf("Incorrect command line option : %s\n", s);
        exit(1);
    }
}



/*---------- �G���[�����t���̕W���֐�-----------------------*/

void       *mallocE(size_t a)
{
    void       *p;

    p = malloc(a);
    if (p == NULL) {
        printf("������������Ȃ�\n");
        exit(1);
    }
    return p;
}



void       *callocE(size_t a, size_t b)
{
    void       *p;

    p = calloc(a, b);
    if (p == NULL) {
        printf("������������Ȃ�\n");
        exit(1);
    }
    return p;
}



char       *strdupE(char *p)
{
    p = strdup(p);
    if (p == NULL) {
        printf("������������Ȃ�\n");
        exit(1);
    }
    return p;
}



FILE       *fopenE(char *name, char *mod)
{
    FILE       *fp;

    fp = fopen(name, mod);
    if (fp == NULL) {
        printf("�t�@�C�� %s ���I�[�v���ł��܂���\n", name);
        exit(1);
    }
    return fp;
}



/*------------------------------------------------------------------------*/
typedef unsigned short LINENUM_T;

typedef struct LBLINK_T {
    struct LBLINK_T *next;
    char            *fname;
    LINENUM_T       line;
  #ifdef MLINE
    char            *mlin;
  #endif
} LBLINK_T;


typedef struct LBL_T {
    LBLINK_T   *next;                           /**< ���x���̎Q�ƃ��X�g */
    char       *name;                           /**< ��`���x����       */
} LBL_T;

TREE        *LBL_tree;                          /**< ���x����o�^����� */

char        *LBL_fname = NULL;                  /**< ���݂̃t�@�C����   */
LINENUM_T   LBL_line;                           /**< ���݂̍s�ԍ�       */



/** TREE ���[�`���ŁA�V�����v�f�𑢂�Ƃ��ɌĂ΂��.
 */
static void *LBL_new(LBL_T * t)
{
    LBL_T      *p;

    p = callocE(1, sizeof(LBL_T));
    memcpy(p, t, sizeof(LBL_T));
    p->name = strdupE(t->name);
    return p;
}



/** TREE ���[�`���ŁA�������J���̂Ƃ��ɌĂ΂��.
 */
static void LBL_del(void *ff)
{
    free(ff);
}



/** TREE ���[�`���ŁA�p�������r����.
 */
static int LBL_cmp(LBL_T * f1, LBL_T * f2)
{
    if (opt_ignorecase)
        return STRCASECMP(f1->name, f2->name);
    else
        return strcmp(f1->name, f2->name);
}



/** TREE ��������.
 */
void LBL_init(void)
{
    LBL_tree = TREE_Make((TREE_NEW) LBL_new, (TREE_DEL) LBL_del, (TREE_CMP) LBL_cmp, (TREE_MALLOC) mallocE, (TREE_FREE) free);
}



/** TREE ���J��.
 */
void LBL_term(void)
{
    TREE_Clear(LBL_tree);
}



/** ���݂̖��O���؂ɓo�^���ꂽ���x�����ǂ����T��.
 */
LBL_T      *LBL_search(char *lbl_name)
{
    LBL_T       t;

    memset(&t, 0, sizeof(LBL_T));
    t.name = lbl_name;
    if (t.name == NULL) {
        printf("������������Ȃ�����\n");
        exit(1);
    }
    t.next = NULL;
    return TREE_Search(LBL_tree, &t);
}



/** ���x��(���O)��؂ɓo�^����.
 */
void LBL_add(char *lbl_name)
{
    LBL_T       t;

    memset(&t, 0, sizeof(LBL_T));
    t.name = lbl_name;
    t.next = NULL;
    TREE_Insert(LBL_tree, &t);
    if (LBL_tree->flag == 0) {                  /* �V�K�o�^�łȂ����� */
        printf("%-12s\t%6ld : %s �����d��`����\n", LBL_fname, (long)LBL_line, lbl_name);
    }
}



/*---------------------------------------------------------------------*/

#define CHK_EOS(c)      ((c) == '\n' || (c) == '\0')
#define CHK_LBLKIGO(c)  ((c) == '_' /*|| (c) == '@' || (c) == '$'*/)



char        tokenName[TOKENNAME_SIZE + 2];      /**< ����擾�������x���� */



/** �X�y�[�X�̃X�L�b�v.
 * @return ���̕����ʒu.
 */
char    *skipSpc(char *s)
{
    while (*(unsigned char *) s <= 0x20 && !CHK_EOS(*s))
        s++;
    return s;
}



/** tokenName�Ƀ��x�����R�s�[����.
 *  @return ���̕����ʒu
 */
char    *getName(char *s)
{
    int     i;

    i = 0;
    s = skipSpc(s);
    while (isalnum(*s) || CHK_LBLKIGO(*s) || strchr(opt_addnamechr, *s)) {
        if (i < TOKENNAME_SIZE - 1) {
            tokenName[i] = *s;
            i++;
        }
        s++;
    }
    tokenName[i] = '\0';
    /* �擪�������̎� */
    if (isdigit(tokenName[0])) {
        tokenName[0] = '\0';
    }
    return s;
}



/** ���x�����\�����镶���ȊO���X�L�b�v����.
 *  @return ���̕����ʒu
 */
char    *skipKigo(char *s)
{
    s = skipSpc(s);
    while (!(isalnum(*s) || CHK_LBLKIGO(*s) || CHK_EOS(*s)))
        s++;
    return s;
}



/** �Q�ƃ��X�g�ɒǉ�����
 */
void    addRefList(LBL_T *p, char *linbuf)
{
    LBLINK_T   *k;
    LBLINK_T   *s;

    /* �o�^ */
    k        = callocE(1, sizeof(LBLINK_T));
    k->next  = NULL;
    k->fname = LBL_fname;
    k->line  = LBL_line;
  #ifdef MLINE
    k->mlin  = strdupE(linbuf);
  #endif
    if (p->next == NULL) {
        p->next = k;
    } else {
        for (s = p->next; s->next != NULL; s = s->next);
        s->next = k;
    }
}



/** ��`���x���e�X�̎g����(�Q��)�ʒu��T�����X�g���쐬
 */
void ref(char *name)
{
    char    buf[LINE_SIZE];
    FILE    *fp;
    char    *p;
    LBL_T   *link;

    LBL_fname = name;
    LBL_line  = 0;
    fp        = fopenE(name, "r");
    while (fgets(buf, sizeof buf, fp) != NULL) {
        LBL_line++;
        p = buf;
        for (;;) {
            p = skipKigo(p);
            if (CHK_EOS(*p))
                break;
            p = getName(p);
            if (tokenName[0] != '\0') {
                link = LBL_search(tokenName);
                if (link != NULL)
                    addRefList(link, buf);
            }
        }
    }

    if (ferror(fp)) {
        printf("%s %ld : ���[�h�G���[���N���܂����B\n", name, (long) LBL_line);
        exit(1);
    }

    fclose(fp);
}



/*-------------------------------------------------------------------------*/
FILE        *outFp;                             /* �o�̓t�@�C�� */
int         fname_len = 4/*12 */;               /* �\���𑵂��邽�߂̌��� */



/** �P�̒�`���x�����ƁA���̎Q�ƃ��X�g��\��
 */
void disp(void *p0)
{
    LBL_T      *p;
    LBLINK_T   *q;

    p = p0;

    if (opt_ltNum >= 0) {           /* �����`�F�b�N����ꍇ */
        int         n = 0;

        for (q = p->next; q != NULL; q = q->next) {
            ++n;
            if (n > opt_ltNum)      /* �w����ȏ�̎Q�Ƃ�����΁A����͖��� */
                return;
        }
    }
    if (opt_nameOnly) {
        if (p->next)
            fprintf(outFp, "%s\n", p->name);
    } else {
        if (opt_qk && p->next == NULL)
            return;
        if (opt_ex) {
            if (opt_ex != 2) {      /* �^�O�W�����v�`�� */
              #ifdef MLINE
                if (opt_linmemFlg) {
                    for (q = p->next; q != NULL; q = q->next)
                        fprintf(outFp, "%-*s\t%6ld [%s] : %s", fname_len, q->fname, (long) q->line, p->name, q->mlin);
                } else
              #endif
                {
                    for (q = p->next; q != NULL; q = q->next)
                        fprintf(outFp, "%-*s\t%6ld : %s\n", fname_len, q->fname, (long) q->line, p->name);
                }
            } else {                /* �t�@�C���� : �P�� */
                for (q = p->next; q != NULL; q = q->next)
                    fprintf(outFp, "%-*s : %s\n", fname_len, q->fname, p->name);
            }
        } else {
            fprintf(outFp, "; %s\n", p->name);
            /* �Q�ƍs��\�� */
          #ifdef MLINE
            if (opt_linmemFlg) {
                for (q = p->next; q != NULL; q = q->next)
                    fprintf(outFp, "\t%-*s\t%6ld : %s", fname_len, q->fname, (long) q->line, q->mlin);
            } else
          #endif
            {
                for (q = p->next; q != NULL; q = q->next)
                    fprintf(outFp, "\t%-*s\t%6ld\n", fname_len, q->fname, (long) q->line);
            }
        }
    }
}



/*------------------------------------------------------------------------*/
char       *dspSameFile_name;

/**
 * �P�̒�`���x�����ƁA���̎Q�ƃ��X�g��\��
 */
void dspSameFile(void *p0)
{
    LBL_T      *p;
    LBLINK_T   *q;

    p = p0;
    for (q = p->next; q != NULL; q = q->next) {
        if (dspSameFile_name == q->fname) {
            fprintf(outFp, "\t%s\n", p->name);
            break;
        }
    }
}



/*------------------------------------------------------------------------*/

char       *Fname_baseName(char *adr)
{
    char       *p;

    p = adr;
    while (*p != '\0') {
        if (FNAME_ISDIRSEP(*p))
            adr = p + 1;
        if (ISKANJI(((unsigned char) *p)) && *(p + 1))
            p++;
        p++;
    }
    return adr;
}



char       *Fname_chgExt(char filename[], char *ext)
{
    char       *p;

    p = Fname_baseName(filename);
    p = strrchr(p, '.');
    if (p == NULL) {
        strcat(filename, ".");
        strcat(filename, ext);
    } else {
        strcpy(p + 1, ext);
    }
    return filename;
}



/*------------------------------------------------------------------------*/

typedef struct FLIST_T {
    struct FLIST_T *next;
    char       *fname;
    short       glb;
} FLIST_T;

FLIST_T    *FLIST_top = NULL;



/** �t�@�C�������X�g�ɒǉ�.
 */
void FLIST_link(char *fname)
{
    FLIST_T    *lk;

    lk      = FLIST_top;
    if (lk == NULL) {
        lk  = FLIST_top = callocE(1, sizeof(FLIST_T));
    } else {
        while (lk->next != NULL) {
            lk   = lk->next;
        }
        lk->next = callocE(1, sizeof(FLIST_T));
        lk       = lk->next;
    }
    lk->fname    = strdupE(fname);
    lk->next     = NULL;
}



/*------------------------------------------------------------------------*/
int         flg_sep = 0;


/** �R�}���h���C�������́A1��̏���.
 */
void getWord(char *p)
{
    if (strcmp(p, "--") == 0) {
        flg_sep = 1;
    } else if (strcmp(p, "++") == 0) {
        flg_sep = 0;
    } else if (flg_sep == 0 && *p == '-') {
        options(p);
    } else {
        if (flg_sep == 0)   /* �P����擾       */
            LBL_add(p);
        else                /* �t�@�C�������擾 */
            FLIST_link(p);
    }
}



/**
 *���X�|���X�E�t�@�C������t�@�C���������o��
 */
void readResFile(char *name)
{
    FILE    *fp;
    char    bf[LINE_SIZE];
    char    fnam[MAX_PATH];
    char    *p;
    int     i;
    int     n;

    fp = fopenE(name, "r");
    n  = 0;
    while (fgets(bf, sizeof bf, fp) != NULL) {
        ++n;
        p = bf;
        for (;;) {
            p = skipSpc(p);
            if (CHK_EOS(*p) || *p == ';')
                break;
            i = 0;
            while ((unsigned char) *p > 0x20)
                fnam[i++] = *p++;
            fnam[i] = '\0';
            getWord(fnam);
        }
    }
    if (ferror(fp)) {
        printf("%s %d : ���[�h�G���[\n", name, n);
        exit(1);
    }
    fclose(fp);
}



int main(int argc, char *argv[])
{
    int         i;
    char        *p;
    FLIST_T     *lk;

    if (argc < 2)
        usage();

    /* ���x���̃c���[������ */
    LBL_init();

    /*-- �I�v�V�����^�t�@�C�����̎擾 --*/
    for (i = 1; i < argc; i++) {
        p = argv[i];
        if (*p == '@')
            readResFile(p + 1);
        else
            getWord(p);
    }

    if (FLIST_top == NULL) {
        printf("�t�@�C�������w�肵�Ă�������\n");
        exit(1);
    }

    /* �t�@�C�����̕����񒷂����߂�...�\���ł̌������̂��� */
    for (lk = FLIST_top; lk != NULL; lk = lk->next) {
        i = strlen(lk->fname);
        if (i > fname_len)
            fname_len = i;
    }

    /*-- �Q�Ə��� --*/
    for (lk = FLIST_top; lk != NULL; lk = lk->next) {
        if (opt_msgFlg)
            printf("[%s]\n", lk->fname);
        ref(lk->fname);
    }

    /*-- ���ʏo�� --*/
    if (opt_ex == 4) {
        for (lk = FLIST_top; lk != NULL; lk = lk->next) {
            dspSameFile_name  = lk->fname;
            strcpy(opt_outname, lk->fname);
            Fname_chgExt(opt_outname, "___");
            outFp = fopenE(opt_outname, "wt");

            /*fprintf(outFp, "%s\n", lk->fname); */
            TREE_DoAll(LBL_tree, dspSameFile);
            fclose(outFp);
        }
    } else {
        if (opt_outname[0] == 0)
            outFp = stdout;
        else
            outFp = fopenE(opt_outname, "wt");
        setvbuf(outFp, NULL, _IOFBF, 0xC000U);

        if (opt_ex != 3) {
            TREE_DoAll(LBL_tree, disp);
        } else {
            for (lk = FLIST_top; lk != NULL; lk = lk->next) {
                dspSameFile_name     = lk->fname;
                fprintf(outFp, "%s\n", lk->fname);
                TREE_DoAll(LBL_tree, dspSameFile);
            }
        }
        if (opt_outname[0])
            fclose(outFp);
    }

    /* ���x���E�c���[�I�� */
    LBL_term();

    return 0;
}

