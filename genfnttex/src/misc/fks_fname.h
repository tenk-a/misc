/**
 *  @file   fks_fname.h
 *  @brief  �t�@�C�����̏���.
 *  @author Masashi Kitamura
 *  @note
 *		fks.lib �ō쐬�O�̒P�t�@�C����(��fks.lib�łƓ������O�ɍ��킹������)
 */
#ifndef FKS_FNAME_H_INCLUDED
#define FKS_FNAME_H_INCLUDED

#include <string.h>


// ============================================================================
// �}�N���֌W.

// Win�ȊO(unix�n)�� �t�@�C�������̑S�p�� \ �΍���������ꍇ�͒�`.
// ������`����Ɗ��ϐ� LANG ���݂� SJIS,gbk,gb2312,big5�Ȃ�MBC����������.
//#define FKS_USE_FNAME_MBC

#if defined _WIN32 || defined _DOS || defined _MSDOS
#define FKS_FNAME_WINDOS
#endif

// �t�@�C�����̃T�C�Y. ���p���̃o�b�t�@�����p.
// ���X�̓p�X�S�̂̐��������������� 1�t�@�C�����̒����ɂȂ��Ă��銴��.
// �� win-api ���̂͊�{�I�Ƀp�X�S�̂ł��̐������󂯂�.
// fname.cpp �ł́Afks_fnameRelativePath?? �݂̂����̒l���g��. (���͎Q�Ƃ��Ȃ�)
#ifndef FKS_FNAME_MAX_PATH
 #ifdef _WIN32
  #define FKS_FNAME_MAX_PATH    260/*_MAX_PATH*/        ///< �ʏ�̃p�X���̒���.��winnt�n�̏ꍇ1�t�@�C�����̒���.
 #else
  #define FKS_FNAME_MAX_PATH    1024                    ///< �ʏ�̃p�X���̒���.
 #endif
#endif

// �ꉞurl���������ꍇ�ɂ���Ă͂��ꂪ�p�X�̍ő咷����. win�̏ꍇ �Œ���0x8000 . ����ȊO�͓K��.
// (�� win�ł͓���Ȏw������Ȃ���_PAX_PATH�𒴂��Ă͎g���Ȃ������)
#ifndef FKS_FNAME_MAX_URL
 #ifdef _WIN32
  #define FKS_FNAME_MAX_URL     (0x8000)                    ///< url�Ƃ��Ĉ����ꍇ�̃p�X���T�C�Y.
 #else  // �K���Ɍv�Z.
  #define FKS_FNAME_MAX_URL     (6U*4*FKS_FNAME_MAX_PATH)   ///< url�Ƃ��Ĉ����ꍇ�̃p�X���T�C�Y.
 #endif
#endif

#if defined FKS_FNAME_WINDOS
#define FKS_FNAME_SEP_CHR       '\\'
#define FKS_FNAME_SEP_STR       "\\"
#define FKS_FNAME_SEP_WCS       L"\\"
#define FKS_FNAME_SEP_TCS       _T("\\")
#else
#define FKS_FNAME_SEP_CHR       '/'
#define FKS_FNAME_SEP_STR       "/"
#define FKS_FNAME_SEP_WCS       L"/"
#define FKS_FNAME_SEP_TCS       "/"
#endif

#ifdef __cplusplus
#define FKS_FNAME_const             // c++�̏ꍇ�� ��{�͔�const�ŁAconst,��const�Q��ލ��.
#else   // c �̂Ƃ�.
#define FKS_FNAME_const     const   // C�̏ꍇ�� ������const, �Ԃ�l�͔�const �ɂ���ꍇ�Ɏg��.
#endif



// ============================================================================
// char��

enum {
    FNAME_MAX_PATH  = FKS_FNAME_MAX_PATH,
    FNAME_MAX_URL   = FKS_FNAME_MAX_URL,
};

#ifdef __cplusplus
extern "C" {
#endif

#if defined FKS_FNAME_WINDOS
inline int      fks_fnameIsSep(unsigned c) { return c == '\\' || c == '/'; }
#else
inline int      fks_fnameIsSep(unsigned c) { return c == '/'; }
#endif

inline size_t   fks_fnameLen(const char* path) { return /*FKS_STD*/ strlen(path); }

int     fks_fnameIsAbs(const char* path);										///< ��΃p�X���ۂ�(�h���C�u���̗L���͊֌W�Ȃ�)
int     fks_fnameHasDrive(const char* path);									///< �h���C�u�������Ă��邩. (file: �� http:���h���C�u����)

unsigned fks_fnameAdjustSize(const char* str, unsigned size);					///< (�Ȃ�ׂ��������󂳂Ȃ���)size�����ȓ��̕�������Ԃ�.
char*   fks_fnameCpy(char dst[], unsigned sz, const char* src);					///< �t�@�C�����̃R�s�[.
char*   fks_fnameCat(char dst[], unsigned sz, const char* src);					///< �t�@�C����������̘A��.

char*   fks_fnameBaseName(FKS_FNAME_const char *adr);							///< �t�@�C���p�X�����̃f�B���N�g�����������t�@�C�����̈ʒu��Ԃ�.
char*   fks_fnameExt(FKS_FNAME_const char *name);								///< �g���q�̈ʒu��Ԃ�.
char*   fks_fnameSkipDrive(FKS_FNAME_const char *name);							///< �h���C�u�����X�L�b�v�����ʒu��Ԃ�.
char*   fks_fnameSkipDriveRoot(FKS_FNAME_const char* name);						///< �h���C�u���ƃ��[�g�w�蕔�����X�L�b�v�����ʒu��Ԃ�.

char*   fks_fnameDelExt(char name[]);											///< �g���q���폜����.
char*   fks_fnameGetNoExt(char dst[], unsigned sz, const char *src);			///< �g���q���O�������O���擾.
char*   fks_fnameGetBaseNameNoExt(char d[],unsigned sz,const char *s);			///< �f�B���N�g���Ɗg���q���O�������O���擾.

char*   fks_fnameSetExt(char dst[], unsigned sz, const char* src, const char *ext);        ///< �g���q���Aext �ɕύX����.
char*   fks_fnameSetDefaultExt(char dst[], unsigned sz, const char* src, const char *ext); ///< �g���q���Ȃ���΁Aext ��ǉ�����.
char*   fks_fnameJoin(char dst[],unsigned sz,const char *dir,const char *nm);              ///< �f�B���N�g�����ƃt�@�C�����̘A��.

char*   fks_fnameGetDir(char dir[], unsigned sz, const char *nm);				///< �f�B���N�g�����̎擾.
char*   fks_fnameGetDrive(char drv[], unsigned sz, const char *nm);				///< �h���C�u�����擾.
char*   fks_fnameGetDriveRoot(char dr[],unsigned sz,const char *nm);			///< �h���C�u�����擾.

char*   fks_fnameCheckPosSep(FKS_FNAME_const char* dir, int pos);				///< pos�̈ʒu��\��/������΂��̃A�h���X���Ȃ����NULL��Ԃ�.
char*   fks_fnameCheckLastSep(FKS_FNAME_const char* dir);						///< �Ō��\��/������΂��̃A�h���X���Ȃ����NULL��Ԃ�.
char*   fks_fnameDelLastSep(char dir[]);										///< ������̍Ō�� \ �� / ������΍폜.
char*   fks_fnameAddSep(char dst[], unsigned sz);								///< ������̍Ō�� \ / ���Ȃ���Βǉ�.

char*   fks_fnameToUpper(char filename[]);										///< �S�p�Q�o�C�g�ڂ��l������ strupr.
char*   fks_fnameToLower(char filename[]);										///< �S�p�Q�o�C�g�ڂ��l������ strlwr.
char*   fks_fnameBackslashToSlash(char filePath[]);								///< filePath���� \ �� / �ɒu��.
char*   fks_fnameSlashToBackslash(char filePath[]);								///< filePath���� / �� \ �ɒu��.

char*   fks_fnameFullpath  (char fullpath[], unsigned sz, const char* path, const char* curDir);       ///< �t���p�X����. os�ˑ�.
char*   fks_fnameFullpathSL(char fullpath[], unsigned sz, const char* path, const char* curDir);       ///< �t���p�X����. / ���.
char*   fks_fnameFullpathBS(char fullpath[], unsigned sz, const char* path, const char* curDir);       ///< �t���p�X����. \ ���.
char*   fks_fnameRelativePath  (char relPath[], unsigned sz, const char* path, const char* curDir);    ///< ���΃p�X����. os�ˑ�.
char*   fks_fnameRelativePathSL(char relPath[], unsigned sz, const char* path, const char* curDir);    ///< ���΃p�X����. / ���.
char*   fks_fnameRelativePathBS(char relPath[], unsigned sz, const char* path, const char* curDir);    ///< ���΃p�X����. \ ���.

int     fks_fnameCmp(const char* l, const char* r);								///< �t�@�C�����̑召��r.
int     fks_fnameNCmp(const char* l, const char* r, unsigned len);				///< �t�@�C������n�����召��r.
int     fks_fnameDigitCmp(const char* l, const char* r);						///< ���Ⴂ�̐����𐔒l�Ƃ��ăt�@�C������r.
char*   fks_fnameEquLong(FKS_FNAME_const char* fname, const char* baseName);	///< fname��baseName�Ŏn�܂��Ă��邩�ۂ�.
int     fks_fnameMatchWildCard(const char* pattern, const char* str);			///< ���C���h�J�[�h����(*?)���r. �}�b�`������^.

/// �R�}���h���C��������A;��؂�̕����̃p�X�w�肩��A�P�v�f�擾.
char*   fks_fnameScanArgStr(char arg[],unsigned sz,const char *str, unsigned sepChr);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus  // �����΍�.
static inline const char*   fks_fnameBaseName(const char *p)                               { return fks_fnameBaseName((char*)p); }
static inline const char*   fks_fnameExt(const char *name)                                 { return fks_fnameExt((char*)name); }
static inline const char*   fks_fnameSkipDrive(const char *name)                           { return fks_fnameSkipDrive((char*)name); }
static inline const char*   fks_fnameSkipDriveRoot(const char *name)                       { return fks_fnameSkipDriveRoot((char*)name); }
static inline const char*   fks_fnameCheckPosSep(const char* dir, int pos)                 { return fks_fnameCheckPosSep((char*)dir,pos); }
static inline const char*   fks_fnameCheckLastSep(const char* dir)                         { return fks_fnameCheckLastSep((char*)dir); }
static inline const char*   fks_fnameEquLong(const char* fname, const char* baseName)      { return fks_fnameEquLong((char*)fname, baseName); }

static inline char*         fks_fnameScanArgStr(char arg[],unsigned sz,const char *str)    { return fks_fnameScanArgStr(arg,sz,str, ' '); }
//static inline char*       fks_fnameSetExt(char name[], unsigned sz, const char *ext)     { return fks_fnameSetExt(name, sz, name, ext); }
//static inline char*       fks_fnameSetDefaultExt(char name[],unsigned sz,const char *ext){ return fks_fnameSetDefaultExt(name, sz, name, ext); }
#endif



// ============================================================================
// wchar_t* ��

#if defined _WIN32 && defined __cplusplus

#include <wchar.h>

inline size_t fks_fnameLen(const wchar_t* path) { return /*FKS_STD*/ wcslen(path); }

int         fks_fnameIsAbs(const wchar_t* path);                                            ///< ��΃p�X���ۂ�(�h���C�u���̗L���͊֌W�Ȃ�)
int         fks_fnameHasDrive(const wchar_t* path);                                         ///< �h���C�u�������Ă��邩. (file: �� http:���h���C�u����)

unsigned    fks_fnameAdjustSize(const wchar_t* str, unsigned size);                         ///< (�Ȃ�ׂ��������󂳂Ȃ���)size�����ȓ��̕�������Ԃ�.
wchar_t*    fks_fnameCpy(wchar_t dst[], unsigned sz, const wchar_t* src);                   ///< �t�@�C�����̃R�s�[.
wchar_t*    fks_fnameCat(wchar_t dst[], unsigned sz, const wchar_t* src);                   ///< �t�@�C����������̘A��.

wchar_t*    fks_fnameBaseName(FKS_FNAME_const wchar_t *adr);                                ///< �t�@�C���p�X�����̃f�B���N�g�����������t�@�C�����̈ʒu��Ԃ�.
wchar_t*    fks_fnameExt(FKS_FNAME_const wchar_t *name);                                    ///< �g���q�̈ʒu��Ԃ�.
wchar_t*    fks_fnameSkipDrive(FKS_FNAME_const wchar_t *name);                              ///< �h���C�u�����X�L�b�v�����ʒu��Ԃ�.
wchar_t*    fks_fnameSkipDriveRoot(FKS_FNAME_const wchar_t* name);                          ///< �h���C�u���ƃ��[�g�w�蕔�����X�L�b�v�����ʒu��Ԃ�.

wchar_t*    fks_fnameDelExt(wchar_t name[]);                                                ///< �g���q���폜����.
wchar_t*    fks_fnameGetNoExt(wchar_t dst[], unsigned sz, const wchar_t *src);              ///< �g���q���O�������O���擾.
wchar_t*    fks_fnameGetBaseNameNoExt(wchar_t d[],unsigned sz,const wchar_t *s);            ///< �f�B���N�g���Ɗg���q���O�������O���擾.

wchar_t*    fks_fnameSetExt(wchar_t dst[], unsigned sz, const wchar_t* src, const wchar_t *ext);       ///< �g���q���Aext �ɕύX����.
wchar_t*    fks_fnameSetDefaultExt(wchar_t dst[],unsigned sz,const wchar_t* src,const wchar_t *ext);   ///< �g���q���Ȃ���΁Aext ��ǉ�����.
wchar_t*    fks_fnameJoin(wchar_t dst[],unsigned sz,const wchar_t *dir,const wchar_t *nm);             ///< �f�B���N�g�����ƃt�@�C�����̘A��.

wchar_t*    fks_fnameGetDir(wchar_t dir[], unsigned sz, const wchar_t *nm);                 ///< �f�B���N�g�����̎擾.
wchar_t*    fks_fnameGetDrive(wchar_t drv[], unsigned sz, const wchar_t *nm);               ///< �h���C�u�����擾.
wchar_t*    fks_fnameGetDriveRoot(wchar_t dr[],unsigned sz,const wchar_t *nm);              ///< �h���C�u�����擾.

wchar_t*    fks_fnameCheckLastSep(FKS_FNAME_const wchar_t* dir);                            ///< �Ō��\��/������΂��̃A�h���X���Ȃ����NULL��Ԃ�.
wchar_t*    fks_fnameDelLastSep(wchar_t dir[]);                                             ///< ������̍Ō�� \ �� / ������΍폜.
wchar_t*    fks_fnameAddSep(wchar_t dst[], unsigned sz);                                    ///< ������̍Ō�� \ / ���Ȃ���Βǉ�.

wchar_t*    fks_fnameToUpper(wchar_t filename[]);                                           ///< �S�p�Q�o�C�g�ڂ��l������ strupr.
wchar_t*    fks_fnameToLower(wchar_t filename[]);                                           ///< �S�p�Q�o�C�g�ڂ��l������ strlwr.
wchar_t*    fks_fnameBackslashToSlash(wchar_t filePath[]);                                  ///< filePath���� \ �� / �ɒu��.
wchar_t*    fks_fnameSlashToBackslash(wchar_t filePath[]);                                  ///< filePath���� / �� \ �ɒu��.

wchar_t*    fks_fnameFullpath  (wchar_t fullpath[], unsigned sz, const wchar_t* path, const wchar_t* curDir);      ///< �t���p�X����. os�ˑ�.
wchar_t*    fks_fnameFullpathSL(wchar_t fullpath[], unsigned sz, const wchar_t* path, const wchar_t* curDir);      ///< �t���p�X����. / ���.
wchar_t*    fks_fnameFullpathBS(wchar_t fullpath[], unsigned sz, const wchar_t* path, const wchar_t* curDir);      ///< �t���p�X����. \ ���.
wchar_t*    fks_fnameRelativePath  (wchar_t relPath[], unsigned sz, const wchar_t* path, const wchar_t* curDir);   ///< ���΃p�X����. os�ˑ�.
wchar_t*    fks_fnameRelativePathSL(wchar_t relPath[], unsigned sz, const wchar_t* path, const wchar_t* curDir);   ///< ���΃p�X����. / ���.
wchar_t*    fks_fnameRelativePathBS(wchar_t relPath[], unsigned sz, const wchar_t* path, const wchar_t* curDir);   ///< ���΃p�X����. \ ���.

int         fks_fnameCmp(const wchar_t* l, const wchar_t* r);								///< �t�@�C�����̑召��r.
int         fks_fnameNCmp(const wchar_t* l, const wchar_t* r, unsigned len);				///< �t�@�C������n�����召��r.
int         fks_fnameDigitCmp(const wchar_t* l, const wchar_t* r);							///< ���Ⴂ�̐����𐔒l�Ƃ��ăt�@�C������r.
wchar_t*    fks_fnameEquLong(FKS_FNAME_const wchar_t* fname, const wchar_t* baseName);		///< fname��baseName�Ŏn�܂��Ă��邩�ۂ�.
int         fks_fnameMatchWildCard(const wchar_t* pattern, const wchar_t* str);				///< ���C���h�J�[�h����(*?)���r. �}�b�`������^.

/// �R�}���h���C��������A;��؂�̕����̃p�X�w�肩��A�P�v�f�擾.
wchar_t*    fks_fnameScanArgStr(wchar_t arg[],unsigned sz,const wchar_t *str, unsigned sepChr);

static inline const wchar_t* fks_fnameBaseName(const wchar_t *p)                                   { return fks_fnameBaseName((wchar_t*)p); }
static inline const wchar_t* fks_fnameExt(const wchar_t *name)                                     { return fks_fnameExt((wchar_t*)name); }
static inline const wchar_t* fks_fnameSkipDrive(const wchar_t *name)                               { return fks_fnameSkipDrive((wchar_t*)name); }
static inline const wchar_t* fks_fnameSkipDriveRoot(const wchar_t *name)                           { return fks_fnameSkipDriveRoot((wchar_t*)name); }
static inline const wchar_t* fks_fnameCheckLastSep(const wchar_t* dir)                             { return fks_fnameCheckLastSep((wchar_t*)dir); }
static inline const wchar_t* fks_fnameEquLong(const wchar_t* fname, const wchar_t* baseName)       { return fks_fnameEquLong((wchar_t*)fname, baseName); }

static inline wchar_t*       fks_fnameScanArgStr(wchar_t arg[],unsigned sz,const wchar_t *str)     { return fks_fnameScanArgStr(arg,sz,str,L' '); }
//static inline wchar_t*     fks_fnameSetExt(wchar_t name[], unsigned sz, const wchar_t *ext)      { return fks_fnameSetExt(name, sz, name, ext); }
//static inline wchar_t*     fks_fnameSetDefaultExt(wchar_t name[],unsigned sz,const wchar_t *ext) { return fks_fnameSetDefaultExt(name, sz, name, ext); }

#endif  //  defined _WIN32 && defined __cplusplus

#undef FKS_FNAME_const

#endif      // FKS_FNAME_H_INCLUDED.
