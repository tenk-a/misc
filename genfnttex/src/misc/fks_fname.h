/**
 *  @file   fks_fname.h
 *  @brief  ファイル名の処理.
 *  @author Masashi Kitamura
 *  @note
 *		fks.lib 版作成前の単ファイル版(をfks.lib版と同じ名前に合わせ直した)
 */
#ifndef FKS_FNAME_H_INCLUDED
#define FKS_FNAME_H_INCLUDED

#include <string.h>


// ============================================================================
// マクロ関係.

// Win以外(unix系)で ファイル名中の全角の \ 対策をしたい場合は定義.
// これを定義すると環境変数 LANG をみて SJIS,gbk,gb2312,big5ならMBC処理をする.
//#define FKS_USE_FNAME_MBC

#if defined _WIN32 || defined _DOS || defined _MSDOS
#define FKS_FNAME_WINDOS
#endif

// ファイル名のサイズ. 利用側のバッファ準備用.
// 元々はパス全体の制限だったが現状 1ファイル名の長さになっている感じ.
// ※ win-api 自体は基本的にパス全体でこの制限を受ける.
// fname.cpp では、fks_fnameRelativePath?? のみがこの値を使う. (他は参照しない)
#ifndef FKS_FNAME_MAX_PATH
 #ifdef _WIN32
  #define FKS_FNAME_MAX_PATH    260/*_MAX_PATH*/        ///< 通常のパス名の長さ.※winnt系の場合1ファイル名の長さ.
 #else
  #define FKS_FNAME_MAX_PATH    1024                    ///< 通常のパス名の長さ.
 #endif
#endif

// 一応url向けだが場合によってはこれがパスの最大長かも. winの場合 最長約0x8000 . それ以外は適当.
// (※ winでは特殊な指定をしないと_PAX_PATHを超えては使えないけれど)
#ifndef FKS_FNAME_MAX_URL
 #ifdef _WIN32
  #define FKS_FNAME_MAX_URL     (0x8000)                    ///< urlとして扱う場合のパス名サイズ.
 #else  // 適当に計算.
  #define FKS_FNAME_MAX_URL     (6U*4*FKS_FNAME_MAX_PATH)   ///< urlとして扱う場合のパス名サイズ.
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
#define FKS_FNAME_const             // c++の場合は 基本は非constで、const,非const２種類作る.
#else   // c のとき.
#define FKS_FNAME_const     const   // Cの場合で 引数はconst, 返り値は非const にする場合に使う.
#endif



// ============================================================================
// char版

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

int     fks_fnameIsAbs(const char* path);										///< 絶対パスか否か(ドライブ名の有無は関係なし)
int     fks_fnameHasDrive(const char* path);									///< ドライブ名がついているか. (file: や http:もドライブ扱い)

unsigned fks_fnameAdjustSize(const char* str, unsigned size);					///< (なるべく文字を壊さないで)size文字以内の文字数を返す.
char*   fks_fnameCpy(char dst[], unsigned sz, const char* src);					///< ファイル名のコピー.
char*   fks_fnameCat(char dst[], unsigned sz, const char* src);					///< ファイル名文字列の連結.

char*   fks_fnameBaseName(FKS_FNAME_const char *adr);							///< ファイルパス名中のディレクトリを除いたファイル名の位置を返す.
char*   fks_fnameExt(FKS_FNAME_const char *name);								///< 拡張子の位置を返す.
char*   fks_fnameSkipDrive(FKS_FNAME_const char *name);							///< ドライブ名をスキップした位置を返す.
char*   fks_fnameSkipDriveRoot(FKS_FNAME_const char* name);						///< ドライブ名とルート指定部分をスキップした位置を返す.

char*   fks_fnameDelExt(char name[]);											///< 拡張子を削除する.
char*   fks_fnameGetNoExt(char dst[], unsigned sz, const char *src);			///< 拡張子を外した名前を取得.
char*   fks_fnameGetBaseNameNoExt(char d[],unsigned sz,const char *s);			///< ディレクトリと拡張子を外した名前を取得.

char*   fks_fnameSetExt(char dst[], unsigned sz, const char* src, const char *ext);        ///< 拡張子を、ext に変更する.
char*   fks_fnameSetDefaultExt(char dst[], unsigned sz, const char* src, const char *ext); ///< 拡張子がなければ、ext を追加する.
char*   fks_fnameJoin(char dst[],unsigned sz,const char *dir,const char *nm);              ///< ディレクトリ名とファイル名の連結.

char*   fks_fnameGetDir(char dir[], unsigned sz, const char *nm);				///< ディレクトリ名の取得.
char*   fks_fnameGetDrive(char drv[], unsigned sz, const char *nm);				///< ドライブ名を取得.
char*   fks_fnameGetDriveRoot(char dr[],unsigned sz,const char *nm);			///< ドライブ名を取得.

char*   fks_fnameCheckPosSep(FKS_FNAME_const char* dir, int pos);				///< posの位置に\か/があればそのアドレスをなければNULLを返す.
char*   fks_fnameCheckLastSep(FKS_FNAME_const char* dir);						///< 最後に\か/があればそのアドレスをなければNULLを返す.
char*   fks_fnameDelLastSep(char dir[]);										///< 文字列の最後に \ か / があれば削除.
char*   fks_fnameAddSep(char dst[], unsigned sz);								///< 文字列の最後に \ / がなければ追加.

char*   fks_fnameToUpper(char filename[]);										///< 全角２バイト目を考慮した strupr.
char*   fks_fnameToLower(char filename[]);										///< 全角２バイト目を考慮した strlwr.
char*   fks_fnameBackslashToSlash(char filePath[]);								///< filePath中の \ を / に置換.
char*   fks_fnameSlashToBackslash(char filePath[]);								///< filePath中の / を \ に置換.

char*   fks_fnameFullpath  (char fullpath[], unsigned sz, const char* path, const char* curDir);       ///< フルパス生成. os依存.
char*   fks_fnameFullpathSL(char fullpath[], unsigned sz, const char* path, const char* curDir);       ///< フルパス生成. / 区切.
char*   fks_fnameFullpathBS(char fullpath[], unsigned sz, const char* path, const char* curDir);       ///< フルパス生成. \ 区切.
char*   fks_fnameRelativePath  (char relPath[], unsigned sz, const char* path, const char* curDir);    ///< 相対パス生成. os依存.
char*   fks_fnameRelativePathSL(char relPath[], unsigned sz, const char* path, const char* curDir);    ///< 相対パス生成. / 区切.
char*   fks_fnameRelativePathBS(char relPath[], unsigned sz, const char* path, const char* curDir);    ///< 相対パス生成. \ 区切.

int     fks_fnameCmp(const char* l, const char* r);								///< ファイル名の大小比較.
int     fks_fnameNCmp(const char* l, const char* r, unsigned len);				///< ファイル名のn文字大小比較.
int     fks_fnameDigitCmp(const char* l, const char* r);						///< 桁違いの数字を数値としてファイル名比較.
char*   fks_fnameEquLong(FKS_FNAME_const char* fname, const char* baseName);	///< fnameがbaseNameで始まっているか否か.
int     fks_fnameMatchWildCard(const char* pattern, const char* str);			///< ワイルドカード文字(*?)列比較. マッチしたら真.

/// コマンドライン引数や、;区切りの複数のパス指定から、１要素取得.
char*   fks_fnameScanArgStr(char arg[],unsigned sz,const char *str, unsigned sepChr);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus  // 引数対策.
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
// wchar_t* 版

#if defined _WIN32 && defined __cplusplus

#include <wchar.h>

inline size_t fks_fnameLen(const wchar_t* path) { return /*FKS_STD*/ wcslen(path); }

int         fks_fnameIsAbs(const wchar_t* path);                                            ///< 絶対パスか否か(ドライブ名の有無は関係なし)
int         fks_fnameHasDrive(const wchar_t* path);                                         ///< ドライブ名がついているか. (file: や http:もドライブ扱い)

unsigned    fks_fnameAdjustSize(const wchar_t* str, unsigned size);                         ///< (なるべく文字を壊さないで)size文字以内の文字数を返す.
wchar_t*    fks_fnameCpy(wchar_t dst[], unsigned sz, const wchar_t* src);                   ///< ファイル名のコピー.
wchar_t*    fks_fnameCat(wchar_t dst[], unsigned sz, const wchar_t* src);                   ///< ファイル名文字列の連結.

wchar_t*    fks_fnameBaseName(FKS_FNAME_const wchar_t *adr);                                ///< ファイルパス名中のディレクトリを除いたファイル名の位置を返す.
wchar_t*    fks_fnameExt(FKS_FNAME_const wchar_t *name);                                    ///< 拡張子の位置を返す.
wchar_t*    fks_fnameSkipDrive(FKS_FNAME_const wchar_t *name);                              ///< ドライブ名をスキップした位置を返す.
wchar_t*    fks_fnameSkipDriveRoot(FKS_FNAME_const wchar_t* name);                          ///< ドライブ名とルート指定部分をスキップした位置を返す.

wchar_t*    fks_fnameDelExt(wchar_t name[]);                                                ///< 拡張子を削除する.
wchar_t*    fks_fnameGetNoExt(wchar_t dst[], unsigned sz, const wchar_t *src);              ///< 拡張子を外した名前を取得.
wchar_t*    fks_fnameGetBaseNameNoExt(wchar_t d[],unsigned sz,const wchar_t *s);            ///< ディレクトリと拡張子を外した名前を取得.

wchar_t*    fks_fnameSetExt(wchar_t dst[], unsigned sz, const wchar_t* src, const wchar_t *ext);       ///< 拡張子を、ext に変更する.
wchar_t*    fks_fnameSetDefaultExt(wchar_t dst[],unsigned sz,const wchar_t* src,const wchar_t *ext);   ///< 拡張子がなければ、ext を追加する.
wchar_t*    fks_fnameJoin(wchar_t dst[],unsigned sz,const wchar_t *dir,const wchar_t *nm);             ///< ディレクトリ名とファイル名の連結.

wchar_t*    fks_fnameGetDir(wchar_t dir[], unsigned sz, const wchar_t *nm);                 ///< ディレクトリ名の取得.
wchar_t*    fks_fnameGetDrive(wchar_t drv[], unsigned sz, const wchar_t *nm);               ///< ドライブ名を取得.
wchar_t*    fks_fnameGetDriveRoot(wchar_t dr[],unsigned sz,const wchar_t *nm);              ///< ドライブ名を取得.

wchar_t*    fks_fnameCheckLastSep(FKS_FNAME_const wchar_t* dir);                            ///< 最後に\か/があればそのアドレスをなければNULLを返す.
wchar_t*    fks_fnameDelLastSep(wchar_t dir[]);                                             ///< 文字列の最後に \ か / があれば削除.
wchar_t*    fks_fnameAddSep(wchar_t dst[], unsigned sz);                                    ///< 文字列の最後に \ / がなければ追加.

wchar_t*    fks_fnameToUpper(wchar_t filename[]);                                           ///< 全角２バイト目を考慮した strupr.
wchar_t*    fks_fnameToLower(wchar_t filename[]);                                           ///< 全角２バイト目を考慮した strlwr.
wchar_t*    fks_fnameBackslashToSlash(wchar_t filePath[]);                                  ///< filePath中の \ を / に置換.
wchar_t*    fks_fnameSlashToBackslash(wchar_t filePath[]);                                  ///< filePath中の / を \ に置換.

wchar_t*    fks_fnameFullpath  (wchar_t fullpath[], unsigned sz, const wchar_t* path, const wchar_t* curDir);      ///< フルパス生成. os依存.
wchar_t*    fks_fnameFullpathSL(wchar_t fullpath[], unsigned sz, const wchar_t* path, const wchar_t* curDir);      ///< フルパス生成. / 区切.
wchar_t*    fks_fnameFullpathBS(wchar_t fullpath[], unsigned sz, const wchar_t* path, const wchar_t* curDir);      ///< フルパス生成. \ 区切.
wchar_t*    fks_fnameRelativePath  (wchar_t relPath[], unsigned sz, const wchar_t* path, const wchar_t* curDir);   ///< 相対パス生成. os依存.
wchar_t*    fks_fnameRelativePathSL(wchar_t relPath[], unsigned sz, const wchar_t* path, const wchar_t* curDir);   ///< 相対パス生成. / 区切.
wchar_t*    fks_fnameRelativePathBS(wchar_t relPath[], unsigned sz, const wchar_t* path, const wchar_t* curDir);   ///< 相対パス生成. \ 区切.

int         fks_fnameCmp(const wchar_t* l, const wchar_t* r);								///< ファイル名の大小比較.
int         fks_fnameNCmp(const wchar_t* l, const wchar_t* r, unsigned len);				///< ファイル名のn文字大小比較.
int         fks_fnameDigitCmp(const wchar_t* l, const wchar_t* r);							///< 桁違いの数字を数値としてファイル名比較.
wchar_t*    fks_fnameEquLong(FKS_FNAME_const wchar_t* fname, const wchar_t* baseName);		///< fnameがbaseNameで始まっているか否か.
int         fks_fnameMatchWildCard(const wchar_t* pattern, const wchar_t* str);				///< ワイルドカード文字(*?)列比較. マッチしたら真.

/// コマンドライン引数や、;区切りの複数のパス指定から、１要素取得.
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
