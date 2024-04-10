/**
 *  @file   ExArgv.h
 *  @brief  argc,argv�̊g������(���C���h�J�[�h,���X�|���X�t�@�C��).
 *  @author Masashi KITAMURA
 *  @date   2006-2010,2023
 *  @note
 *  -   main(int argc,char* argv[]) ��argc,argv�ɑ΂��A
 *      ���C���h�J�[�h�w��⃌�X�|���X�t�@�C���w�蓙��W�J����argc,argv�ɕϊ�.
 *      main()�̏����[���炢��
 *          ExArgv_conv(&argc, &argv);
 *      �̂悤�ɌĂяo��.
 *  -   WinMain() �Ŏg���ꍇ�� EXARGV_FOR_WINMAIN ���`���A
 *          ExArgv_forWinMain(cmdl, &argc, &argv);
 *      �̂悤�ɌĂяo��.
 *
 *  -   ���Win/Dos�n(�̃R�}���h���C���c�[��)�ł̗��p��z��.
 *      �ꉞ mac,linux gcc/clang �ł̃R���p�C����.
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
 *  -   �}�N�� UNICODE �� EXARGV_USE_WCHAR ���`�� wchar_t�p�A�Ȃ����char�p.
 *  -   UTF8 �����y�����̂ŁAEXARGV_USE_MBC ��`���̂�MBC��2�o�C�g��'\'�Ώ�.
 *  -   _WIN32 ����`����Ă���� win�p�A�łȂ���� unix�n��z��.
 *
 *  - Public Domain Software
 */

#ifndef EXARGV_INCLUDED__
#define EXARGV_INCLUDED__

// ---------------------------------------------------------------------------
// �ݒ�.

//[] ��`����ƁAWinMain �p�� ExArgv_forWinMain �𐶐�.(ExArgv_conv �͖�)
//#define EXARGV_FOR_WINMAIN

//[] ��`���� ���� UNICODE ����`�Ȃ� MBCS �Ƃ���2�o�C�g��\�����Ώ����s��
//#define EXARGV_USE_MBC

//[] ��`����ƁAwchar_t �p�Ƃ��Đ���. UNICODE ��`���͎����Œ�`�����.
//#define EXARGV_USE_WCHAR

//[] EXARGV_USE_WCHAR���ɒ�`����ƁAExArgv_conv �łȂ� ExArgv_conv_to_utf8 �𐶐�.
//#define EXARGV_USE_WCHAR_TO_UTF8

//[] ���C���h�J�[�h�w��� 1=�L��  0=����  ����`=�f�t�H���g�ݒ�(1)
//#define EXARGV_USE_WC         1

//[] ���C���h�J�[�hon���ɁA���C���h�J�[�h���� ** ������΍ċA������
//      1=���� 0=���Ȃ� ����`=�f�t�H���g�ݒ�(1)
#define EXARGV_USE_WC_REC     1


//[] @���X�|���X�t�@�C����
//      1=�L��   0=����  ����`=�f�t�H���g�ݒ�(0)
//#define EXARGV_USE_RESFILE    0


//[] �ȈՃR���t�B�O(���X�|���X)�t�@�C�����͂�
//      1=�L��  0=����  ����`=�f�t�H���g(0)
//   �L�����́Awin/dos�Ȃ� .exe �� .cfg �ɒu�������p�X��.
//             �ȊO�Ȃ� unix �n���낤�� ~/.���s�t�@�C����.cfg
//#define EXARGV_USE_CONFIG     0


//[] �R���t�B�O�t�@�C�����͗L���̂Ƃ��A������`����΁A
//      �R���t�B�O�t�@�C���̊g���q������ɂ���.
//#define EXARGV_CONFIG_EXT     ".cfg"  // .conf


//[] ��`����ƁA���̖��O�̊��ϐ����R�}���h���C��������Ƃ��ė��p.
//#define EXARGV_ENVNAME    "your_app_env_name"


//[] win���̂�. argv[0] �̎��s�t�@�C�������t���p�X��
//      1=�L��  0=����      ����`=�f�t�H���g(0)
//   ��bcc,dmc,watcom�͌�����t���p�X�Ȃ̂ŉ������܂���. �̂�vc,gcc��.
//#define EXARGV_USE_FULLPATH_ARGV0

//[] ��`����΁AfilePath���� \ �� / �ɒu��.
//#define EXARGV_TOSLASH


//[] ��`����΁AfilePath���� / �� \ �ɒu��.
//#define EXARGV_TOBACKSLASH


//[] ��`����΁A/ ���I�v�V�����J�n�����Ƃ݂Ȃ�.
//#define EXARGV_USE_SLASH_OPT

//[] ����. VC�̂�. ��`����ƁAsetargv�̑�p�i�Ƃ��ăR���p�C��(ExArgv_get�͖�)
//   ���� setargv.obj �̃����N���K�v.
// #define EXARGV_USE_SETARGV



// ---------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif


#if defined EXARGV_USE_SETARGV  // VC�̈Öُ����̒u�������p.

#elif defined EXARGV_FOR_WINMAIN // win-gui�p. _WINDOWS ��CONSOLE�p�ł���`����邱�Ƃ�����p�~.
 #if defined UNICODE || defined EXARGV_USE_WCHAR
  #if defined(EXARGV_USE_WCHAR_TO_UTF8)
   void ExArgv_to_utf8_forWinMain(const wchar_t* pCmdLine, int* pArgc, wchar_t*** pppArgv, char*** pppUtf8s);
  #else
   void ExArgv_forWinMain(const wchar_t* pCmdLine, int* pArgc, wchar_t*** pppArgv);
  #endif
 #else
   void ExArgv_forWinMain(const char*    pCmdLine, int* pArgc, char***    pppArgv);
 #endif
#else                       // �R�}���h���C���c�[���p. main�̏����[���炢�ɌĂԂ̂�z��.
 #if defined UNICODE || defined EXARGV_USE_WCHAR
  #if defined(EXARGV_USE_WCHAR_TO_UTF8)
   char** ExArgv_conv_to_utf8(int* pArgc, wchar_t*** ppArgv);
  #else
   void** ExArgv_conv(int* pArgc, wchar_t*** pppArgv);
  #endif
 #else
   void** ExArgv_conv(int* pArgc, char*** pppArgv);
 #endif
#endif
void ExArgv_Free(void*** pppArgv);
void ExArgv_FreeA(char*** pppArgv);
void ExArgv_FreeW(wchar_t*** pppArgv);


#ifdef __cplusplus
}
#endif
// ---------------------------------------------------------------------------
#endif  // EXARGV_INCLUDED__
