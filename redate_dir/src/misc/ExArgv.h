/**
 *  @file   ExArgv.h
 *  @brief  main(int argc,char* argv[]) ��argc,argv���A
 *  	    ���X�|���X�t�@�C���A���C���h�J�[�h�W�J�������̂Ɋg������.
 *  @author Masashi KITAMURA
 *  @date   2007
 */

#ifndef EXARGV_H
#define EXARGV_H


#if defined __cplusplus == 0

/// argc,argv �����X�|���X�t�@�C���⃏�C���h�J�[�h�W�J���āAargc, argv���X�V���ĕԂ�.
void ExArgv_get(int* pArgc, char*** pppArgv, const char* flags, const char* envName, const char* recOpt1, const char* recOpt2);

    /*	[flags����]
     *	    @	    @response_file �L��.
     *	    C	    .exe��.cfg�ɕς����t�@�C���p�X���̃R���t�B�O��ǂݍ���.
     *
     *	    *	    wildcard �����L��.
     *	    r	    ���C���h�J�[�h�w��̂݁A�f�B���N�g���ċA��������.
     *	    R	    �t�@�C�����͑S�ăf�B���N�g���ċA��������.
     *	    i	    �ċA�I�v�V����������̑啶���������𔻕ʂ��Ȃ�.
     *	    --	    �ċA�I�v�V����������,--�ȍ~�A-�Ŏn�܂镶����̓I�v�V�����łȂ��t�@�C����.
     *
     *	�� �}�N�� EXARGV_TINY ��`���� @c����ъ��ϐ��̂ݗL���B*rRi--�͕s��.
     */


#ifdef _WINDOWS
/// win�A�v���ŁAWinMain�����[�ŁAargc,argv�����ꍇ�p.
void ExArgv_forWinMain(const char* pCmdLine, int* pArgc, char*** pppArgv, const char* flags, const char* envName, const char* recOpt1, const char* recOpt2 );
#endif	// _WINDOWS



#else	// c++ �̂Ƃ��́A�f�t�H���g������ݒ�.
extern "C" {
    void ExArgv_get(int* pArgc, char*** pppArgv, const char* flags="@*", const char* envName=0, const char* recOpt1=0, const char* recOpt2=0);
   #ifdef _WINDOWS
    void ExArgv_forWinMain(const char* pCmdLine, int* pArgc, char*** pppArgv, const char* flags="@*", const char* envName=0, const char* recOpt1=0, const char* recOpt2=0 );
   #endif
}
#endif


#endif	// EXARGV_H
