/**
 *	@file 	ExArgv.h
 *	@brief	argc,argv�̊g������(���C���h�J�[�h,���X�|���X�t�@�C��).
 *	@author	Masashi KITAMURA
 *	@date	2006-2010
 *	@note
 *	-	main(int argc,char* argv[]) ��argc,argv�ɑ΂��A
 *		���C���h�J�[�h�w��⃌�X�|���X�t�@�C���w�蓙��W�J����argc,argv�ɕϊ�.
 *		main()�̏����[���炢�� ExArgv_conv(&argc, &argv); �̂悤�ɌĂяo��.
 *		���邢�� WinMain() �ł�, ExArgv_forWinMain(cmdl, &argc, &argv);
 *
 *	-	���C����dos/win�n(�̃R�}���h���C���c�[��)��z��.
 *		�ꉞ linux gcc�ł̃R���p�C����.
 *		(unix�n���ƃ��C���h�J�[�h�̓V�F���C�����낤�ŁA�����b�g���Ȃ�)
 *
 *	-	ExArgv.h�́A�ꉞ�w�b�_�����AExArgv.c �̐ݒ�t�@�C���ł�����.
 *		�A�v�����Ƃ� ExArgv.h ExArgv.c ���R�s�[���āAExArgv.h��
 *		�J�X�^�����Ďg���̂�z��.
 *	-	�ݒ�ł���v�f�́A
 *			- ���C���h�J�[�h (on/off)
 *			- ���C���h�J�[�h���̍ċA�w��(**)�̗L�� (on/off)
 *			- @���X�|���X�t�@�C�� (on/off)
 *			- .exe�A�� .cfg �t�@�C�� �Ǎ� (on/off)
 *			- �I�v�V�������ϐ����̗��p
 *			��
 *
 *	-	����������̐擪��'-'�Ȃ�΃I�v�V�������낤�ŁA���̕����񒆂�
 *		���C���h�J�[�h�����������Ă��W�J���Ȃ�.
 *	-	�}�N�� UNICODE ����`����Ă���΁Awchar_t�p�A�łȂ����char�p.
 *	-	_WIN32 ����`����Ă���� win�p�A�łȂ���� unix�n��z��.
 *
 *	- Public Domain Software
 */

#ifndef EXARGV_INCLUDED
#define EXARGV_INCLUDED

// ---------------------------------------------------------------------------
// �ݒ�.

//[] ���C���h�J�[�h�w��� 1=�L��  0=����  ����`=�f�t�H���g�ݒ�(1)
//#define EXARGV_USE_WC			1


//[] ���C���h�J�[�hon���ɁA���C���h�J�[�h���� ** ������΍ċA������
//	 	1=���� 0=���Ȃ� ����`=�f�t�H���g�ݒ�(1)
//#define EXARGV_USE_WC_REC		1


//[] @���X�|���X�t�@�C����
//		1=�L��	 0=����  ����`=�f�t�H���g�ݒ�(1)
//#define EXARGV_USE_RESFILE	1


//[] �ȈՃR���t�B�O(���X�|���X)�t�@�C�����͂�
//		1=�L��	0=����	����`=�f�t�H���g(0)
//	 �L�����́Awin/dos�Ȃ� .exe �� .cfg �ɒu�������p�X��.
//			   �ȊO�Ȃ� unix �n���낤�� ~/.���s�t�@�C����.cfg
//#define EXARGV_USE_CONFIG		0


//[] �R���t�B�O�t�@�C�����͗L���̂Ƃ��A������`����΁A
//		�R���t�B�O�t�@�C���̊g���q������ɂ���.
//#define EXARGV_CONFIG_EXT		".cfg"	// .conf


//[] ��`����ƁA���̖��O�̊��ϐ����R�}���h���C��������Ƃ��ė��p.
//#define EXARGV_ENVNAME	"your_app_env_name"


//[] win���̂�. argv[0] �̎��s�t�@�C�������t���p�X��
//		1=�L��	0=����		����`=�f�t�H���g(0)
//	 ��bcc,dmc,watcom�͌�����t���p�X�Ȃ̂ŉ������܂���. �̂�vc,gcc��.
//#define EXARGV_USE_FULLPATH_ARGV0


//[] ��`����΁AfilePath���� \ �� / �ɒu��.
//#define EXARGV_TOSLASH


//[] ��`����΁AfilePath���� / �� \ �ɒu��.
//#define EXARGV_TOBACKSLASH


//[] ��`����΁A/ ���I�v�V�����J�n�����Ƃ݂Ȃ�.
//#define EXARGV_USE_SLASH_OPT


//[] ����. VC�̂�. ��`����ƁAsetargv�̑�p�i�Ƃ��ăR���p�C��(ExArgv_get�͖�)
//	 ���� setargv.obj �̃����N���K�v.
// #define EXARGV_USE_SETARGV



// ---------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif


#if defined EXARGV_USE_SETARGV	// VC�̈Öُ����̒u�������p.

#elif defined _WINDOWS		// win-gui ���p.
 #if defined UNICODE
  void ExArgv_forWinMain(const wchar_t* pCmdLine, int* pArgc, wchar_t*** pppArgv);
 #else
  void ExArgv_forWinMain(const char* 	pCmdLine, int* pArgc, char***	 pppArgv);
 #endif
#else						// �R�}���h���C���c�[���p. main�̏����[���炢�ɌĂԂ̂�z��.
 #if defined UNICODE
  void ExArgv_conv(int* pArgc, wchar_t*** pppArgv);
 #else
  void ExArgv_conv(int* pArgc, char*** pppArgv);
 #endif
#endif


#ifdef __cplusplus
}
#endif
// ---------------------------------------------------------------------------
#endif	// EXARGV_INCLUDED
