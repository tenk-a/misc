/**
    @file   filn.h
    @brief  ���������A�}�N���@�\���������e�L�X�g�t�@�C�����̓��[�`��.
    @author Masashi KITAMURA (tenka@6809.net)
    @date   1996-2017
    @note
     - �R�}���h���C���c�[���ł̗��p��O��B
     - �v���I�G���[������� exit(1) �ŏI������B
     - �g����
        - �܂��A�ŏ��� Filn_Init() �ŏ���������B
        - Filn->???? �ŁA�I�v�V�����v�f���D�݂ɐݒ�B
        - include�����p�X�́AFiln_AddIncDir()�ŉ\�B
        - �I�v�V�����ݒ�́A���͂��J�n����O�ɍς܂����ƁB���͒��ɕύX�����ꍇ�̋����͕s���B
        - �G���[�͊�{stderr�o�͂����A�K�v�Ȃ�� Filn_ErrReOpen()�ŃG���[�o�͐��ウ��B
        - Filn_Open(name) �ŁA���̓I�[�v���Bname=NULL ���ƕW�����́B
        - Filn_Gets() �łP�s���́B�}�N���W�J��#if��#include�̏�����̕�����𓾂�B
          ���\�[�X���R�����g�Ŋ܂߂�ݒ�ɂ��Ă���΁A���̂悤�ɓǂ݂��܂��B
          �߂�l�́A�擾��������������߂� malloc()�����������ւ̃|�C���^��Ԃ��B
          ���̃������̊J���͌Ăяo�������s���K�v������B
        - ���ݓ��͒��̍s���A���̂ǂ̃t�@�C���̉��s�ڂ��́AFiln_GetFileLine() �œ�����B
        - Filn���ŃG���[������Ώ���ɃG���[�o�͂���B
          ���͂����s�ɑΉ������G���[���A�A�v�����ŏo�������ꍇ�́A
          Filn_Error() �� Filn_Warning() ���g���B�G���[���A�x������ Filn_GetErrNum() �Ŏ擾�\�B
        - ���͂̏I���� Filn_Gets() ��NULL��Ԃ������ǂ����Ŕ��肷��B
          include�̓s���A�����I�Ƀt�@�C���E�N���[�Y����̂ŁA�Ăяo�����ŃN���[�Y�̕K�v�͂Ȃ��B
        - �Ō�� Filn_Term() ���ĂׂΏI���B

     - ����
        - ����̔łł́A�擾�����������̊J�����s�[���Ń��[�N����d�l�Ȃ̂ŁA
          Filn_Init/Filn_Term�̃y�A�͋N�����Ĉ�񂵂��g���Ȃ��B
        - ���܂�g������ł��Ȃ�/�g�����ɕ΂肪����A�̂ŁA�o�O�͂�������c���Ă���Ǝv����B
        - &&��||�ŁA���ӂŌ��ʂ��m�肵�Ă��Ă��E�ӂ�]�����邵���݂ɂȂ��Ă���B
            #if defined(M1) && M1(2,3) == 0
          �̂悤�ȋL�q�� M1 ����`�̂Ƃ��A�Ӑ}�ʂ�ɂ͂Ȃ炸�G���[�ɂȂ��Ă��܂��B
            #if defined(M1)
              #if M1(2,3) == 0
          �̂悤�ɍs�𕪂��đΏ��̂���.
        - #(��) ���ł� defined���g���Ȃ��ł��B

     - license
        Boost Software License Version 1.0
 */
#ifndef FILN_H
#define FILN_H

#include <stdio.h>

typedef struct FILN_DIRS {
    struct FILN_DIRS    *link;
    char                *s;
} FILN_DIRS;


typedef struct filn_t {
/* public:*/ /* ���[�U���ݒ��ύX���邱�Ƃ̂���ϐ� */
    int         opt_kanji;      /* 0�ȊO�Ȃ��MS�S�p�ɑΉ� */
    int         opt_sscomment;  /* 0�ȊO�Ȃ��//�R�����g���폜���� */
    int         opt_blkcomment; /* 0�ȊO�Ȃ�΁^���R�����g���^���폜���� */
    int         opt_dellf;      /* 0�ȊO�Ȃ�΁����s�ɂ��s�A�����s�� */
    int         opt_delspc;     /* 0�ȊO�Ȃ�΋󔒂̈��k������ */
    int         opt_oct;        /* 1: 0����n�܂鐔���͂W�i��  0:10�i */
    int         opt_yen;        /* \ ������C�̂悤�� 0:����Ȃ�. 1:���� 2:'"���̂�  3,4:�ϊ��������Ⴄ(����) */
    int         opt_sq_mode;    /* ' �𕶎��萔���ā@0:����Ȃ� 1:���� 2:�������y�A�ɂ��Ȃ�(���g���[����) */
    int         opt_wq_mode;    /* " �𕶎���萔����0:����Ȃ� 1:���� */
    int         opt_mot_doll;   /* $ �� ���g���[���� 16�i���萔�J�n�����Ƃ��Ĉ��� */

    int         opt_orgSrc;     /* 1:���̃\�[�X���R�����g�ɂ��ďo�� 2:TAG JUMP�`��  0:�o�͂��Ȃ� */
    char        *orgSrcPre;     /* ���\�[�X�o�͎��̍s���ɂ��镶���� */
    char        *orgSrcPost;    /* ���\�[�X�o�͎��̍s���ɂ��镶���� */

    int         sym_chr_doll;   /* ���O�̈ꕔ�Ƃ��� $���g���Ȃ�'$'��, �����łȂ����0��ݒ肷�� */
    int         sym_chr_atmk;   /* ���O�̈ꕔ�Ƃ��� @���g���Ȃ�'@'��, �����łȂ����0��ݒ肷�� */
    int         sym_chr_qa;     /* ���O�̈ꕔ�Ƃ��� ?���g���Ȃ�'?'��, �����łȂ����0��ݒ肷�� */
    int         sym_chr_shp;    /* ���O�̈ꕔ�Ƃ��� #���g���Ȃ�'#'��, �����łȂ����0��ݒ肷�� */
    int         sym_chr_prd;    /* ���O�̈ꕔ�Ƃ��� .���g���Ȃ�'.'��, �����łȂ����0��ݒ肷�� */

    int         macErrFlg;      /* �}�N�����̃G���[�s�ԍ����\�� 1:���� 0:���Ȃ� */
    int         mac_chr;        /* �}�N���s�J�n���� */
    int         mac_chr2;       /* �}�N���̓���W�J�w�蕶��.  */
    int         immMode;        /* ���l�̏o�� 0:�܂�  1:10�i 2:������ 3:0xFF 4:0FFh 5:$FF */
    char        cmt_chr[2];     /* �R�����g�J�n�����ɂȂ镶�� */
    char        cmt_chrTop[2];  /* �s���R�����g�J�n�����ɂȂ镶�� */
    char        *localPrefix;   /* #local�Ő������郉�x���̃v���t�B�b�N�X*/

    int         getsAddSiz;     /* Filn_Gets()�ŕԂ� malloc�T�C�Y�ŁA���Ȃ��Ƃ����̃o�C�g�����]���Ɋm�ۂ��� */
/*private:*/ /* �ȉ��͎Q�Ƃ��Ă������A���������Ă͂��� */
    FILN_DIRS   *dir;           /* include ���Ɍ�������f�B���N�g���ꗗ */
    FILE        *errFp;         /* �G���[�o�͐� */
} filn_t;


extern  filn_t *Filn;

filn_t *Filn_Init(void);                                /* Filn�̏��������[�`���B�^����ɌĂяo������*/
void Filn_Term(void);                                   /* Filn�̏I��. �������J���Ȃ�. Filn_ErrClose�͍s��Ȃ� */

int  Filn_Open(char const* name);                       /* �\�[�X�t�@�C�����I�[�v������ */
char *Filn_Gets(void);                                  /* �}�N���W�J�t�P�s����. malloc ������������Ԃ��̂ŁA�ďo���ŊJ���̂���*/

void Filn_AddIncDir(char const* dir);                   /* (������)���̓f�B���N�g����ݒ肷�� */
void Filn_GetFnameLine(char const** s, int* line);      /* ���ݓ��͒��̃t�@�C�����ƍs�ԍ��𓾂� */
int  Filn_UndefLabel(char const* p);                    /* �}�N�����̍폜. �o�^����ĂȂ���΂Ȃɂ����Ȃ� */
int  Filn_SetLabel(char const* name, char const* st);   /* �}�N�����̓o�^. st=NULL�Ȃ��name�� C�R���p�C����-Dname �Ɠ��l�̏��� */
int  Filn_GetLabel(char const* name,char const** strp); /* name ��define ����Ă��邩���ׁA����Ă����0�ȊO��Ԃ��B*/
                                                        /* ���̂Ƃ� *strp�ɒ�`������ւ̃|�C���^�����ĕԂ�. *strp�̔j��͕s�B*/

FILE *Filn_ErrReOpen(char const* name, FILE *fp);       /* �G���[�o�͐�fp��name�ōăI�[�v��. */
                                                        /* name=NULL���ƒP��fp�ɂ��Afp=NULL���ƐV�K�Ƀt�@�C���֏o�� */
void Filn_ErrClose(void);                               /* �G���[�o�͐��close */
int  Filn_Error(char const* fmt, ...);                  /* �t�@�C�����s�ԍ��t�̃G���[�o��printf */
volatile int Filn_Exit(char const* fmt, ...);           /* Filn_Error �Ɠ������Ƃ������̂�exit(1)���� */
void Filn_GetErrNum(int *errNum, int *warnNum);         /* �G���[�ƌx���̐���Ԃ� */

#endif /* FILN_H */
