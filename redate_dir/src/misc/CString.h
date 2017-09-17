/**
 *  @file   CSting.h
 *  @brief  文字列クラス. WTL::CString を char 専用で組み込み用に調整したもの.
 *  @author microsoft
 *  @note
 *  	ファイル名は、std::stringと紛らわしく無いようにするため、あえてC付にしている.
 *  	wtl8.0よりコピペ改造.
 *  	 - char 型のみ.
 *  	 - windows固有の機能は破棄.
 *  	 - ヘッダオンリーだったのを .h と .cpp に分離.
 *  	ライセンスは CPL または MPL.(なので、仕事で使うだけなら問題なし)
 */

#ifndef CSTRING_H
#define CSTRING_H
#pragma once

#include "stdafx.h"


// ============================================================================
// mbstring.h, tchar.h の代わり.

#ifdef _WIN32	    // winならほんもの使う.

#include <windows.h>
#include <tchar.h>
#include <mbstring.h>
#include <assert.h>

#else	    	    // winでないときはSJIS固定で代用品. 必要なもののみフリーソフトよりコピペしたもの.

#define _ismbblead(c)	    (((c) >= 0x81) && (((c) <= 0x9F) | (((c) >= 0xE0) & ((c) <= 0xFC))))
#define _ismbbtrail(c)	    (((c)>=0x40) & ((c)<=0xFC) & ((c)!=0x7F))
unsigned char*	_mbsinc (const unsigned char* s);
size_t	    	_mbslen (const unsigned char* str);
unsigned char*	_mbscpy (unsigned char* dst, const unsigned char* s);
unsigned char*	_mbsset (unsigned char* dst, unsigned c);
unsigned char*	_mbsrev (unsigned char* str);
unsigned char*	_mbscat (unsigned char* dst, const unsigned char* addStr);
unsigned char*	_mbschr (const unsigned char* src, unsigned key);
unsigned char*	_mbsrchr(const unsigned char* src, unsigned key);
unsigned char*	_mbsstr (const unsigned char* src, const unsigned char *ptn);
unsigned char*	_mbsrstr(const unsigned char* src, const unsigned char *ptn);
size_t	    	_mbsspn (const unsigned char* src, const unsigned char* tbl);
size_t	    	_mbscspn(const unsigned char* src, const unsigned char* tbl);
unsigned char*	_mbspbrk(const unsigned char* src, const unsigned char *tbl);
int 	    	_mbscmp (const unsigned char* left, const unsigned char* right);
int 	    	_mbsicmp(const unsigned char* left, const unsigned char* right);
unsigned char*	_mbslwr (unsigned char* src);
unsigned char*	_mbsupr (unsigned char* src);

#ifndef _T  	// これが無い場合は、winでないだろう、で設定.
#define _T(x)	    x

typedef char*	    LPSTR;
typedef const char* LPCSTR;

typedef char	     TCHAR;
typedef TCHAR*	     LPTSTR;
typedef const TCHAR* LPCTSTR;
typedef int 	     BOOL;

inline	size_t	_tcslen (const TCHAR* str)  	    	    	{ return _mbslen ((const unsigned char*) str);	    	     }
inline	TCHAR*	_tcscpy (TCHAR* dst, const TCHAR* s)	    	{ return (TCHAR*)_mbscpy ((unsigned char*)dst, (const unsigned char*) s);   }
inline	TCHAR*	_tcsset (TCHAR* dst, unsigned c)    	    	{ return (TCHAR*)_mbsset ((unsigned char*)dst, (unsigned) c);	     }
inline	TCHAR*	_tcsrev (TCHAR* str)	    	    	    	{ return (TCHAR*)_mbsrev ((unsigned char*)str);     	    	     }
inline	TCHAR*	_tcscat (TCHAR* dst, const TCHAR* addStr)   	{ return (TCHAR*)_mbscat ((unsigned char*)dst, (const unsigned char*) addStr); }
inline	TCHAR*	_tcschr (const TCHAR* src, unsigned key)    	{ return (TCHAR*)_mbschr ((const unsigned char*) src, (unsigned) key); }
inline	TCHAR*	_tcsrchr(const TCHAR* src, unsigned key)    	{ return (TCHAR*)_mbsrchr((const unsigned char*) src, (unsigned) key); }
inline	TCHAR*	_tcsstr (const TCHAR* src, const TCHAR *ptn)	{ return (TCHAR*)_mbsstr ((const unsigned char*) src, (const unsigned char *)ptn); }
inline	TCHAR*	_tcsrstr(const TCHAR* src, const TCHAR *ptn)	{ return (TCHAR*)_mbsrstr((const unsigned char*) src, (const unsigned char *)ptn); }
inline	size_t	_tcsspn (const TCHAR* src, const TCHAR* tbl)	{ return _mbsspn ((const unsigned char*) src, (const unsigned char *)tbl); }
inline	size_t	_tcscspn(const TCHAR* src, const TCHAR* tbl)	{ return _mbscspn((const unsigned char*) src, (const unsigned char *)tbl); }
inline	TCHAR*	_tcspbrk(const TCHAR* src, const TCHAR *tbl)	{ return (TCHAR*)_mbspbrk((const unsigned char*) src, (const unsigned char *)tbl); }
inline	int 	_tcscmp (const TCHAR* left, const TCHAR* right) { return _mbscmp ((const unsigned char*) left, (const unsigned char*) right); }
inline	int 	_tcsicmp(const TCHAR* left, const TCHAR* right) { return _mbsicmp((const unsigned char*) left, (const unsigned char*) right); }
inline	TCHAR*	_tcslwr (TCHAR* src)	    	    	    	{ return (TCHAR*)_mbslwr ((unsigned char*) src); }
inline	TCHAR*	_tcsupr (TCHAR* src)	    	    	    	{ return (TCHAR*)_mbsupr ((unsigned char*) src); }
#endif	// _T

#endif	// _WIN32



// ============================================================================
// CString内部のメモリ管理.

struct CStringData {
  #if defined(USE_CSTRING_SHORT)    	//北村	組み込み向けにサイズを小さくする.
    enum    { MAX_LENGTH = 0x4000 };	//北村	単純な文字列だけに使うので、長さを制限しとく.
    typedef unsigned short  U_INT;
    typedef short   	    S_INT;
  #else
    enum    { MAX_LENGTH = 0xFFFFFFFF };    //北村  適当.
    typedef unsigned	    U_INT;
    typedef int     	    S_INT;
  #endif
    S_INT   nRefs;  	    	    	// 参照数.
    U_INT   nDataLength;    	    	// データの長さ.
    U_INT   nAllocLength;   	    	// アロケートされたサイズ.

    // char data[nAllocLength]
    char*   data() { return (char*)(this + 1); }
};



// ============================================================================
// CString クラス.

// Globals

// For an empty string, m_pchData will point here
// (note: avoids special case of checking for NULL m_pchData)
// empty string data (and locked)

class CString {
protected:
    char*   	    	m_pchData;  	    	    // pointer to ref counted string data

 #ifndef _MSC_VER
  private:
    static CStringData::S_INT	rgInitData[];
    static CStringData*     	_atltmpDataNil;
    static const char*	    	_atltmpPchNil;
 #endif

public:
    CString() { Init(); }
    CString(const CString &stringSrc);
    CString(char ch, int nRepeat = 1);
    CString(const char* lpsz);
    CString(const char* lpch, int nLength);
    CString(const unsigned char* lpsz) { Init(); *this = (const char*) lpsz; }
  #if 0 //北村	追加. CSTRINGDATA(x) で初期化する.
    CString(const CStringData& stringData);
  #endif
    ~CString();     	    	    	//  free any attached data

    int     	GetAllocLength() const { return GetData()->nAllocLength; }
    static BOOL _IsValidString(const char* lpsz, int /*nLength*/ = -1) { return (lpsz != NULL); }


    // Attributes & Operations
    int GetLength() const { 	    // as an array of characters
    	if (m_pchData == 0)
    	    return 0;
    	return GetData()->nDataLength;
    }

    BOOL IsEmpty() const { return (m_pchData == 0) || (GetData()->nDataLength == 0); }

    void Empty();   // free up the data // 注意! std::stringのempty()と違い、これはclearする!

    char GetAt(int nIndex) const {  	    // 0 based
    	assert(nIndex >= 0);
    	assert(nIndex < GetData()->nDataLength);
    	return m_pchData[nIndex];
    }

    char operator [](int nIndex) const {    // same as GetAt
    	// same as GetAt
    	assert(nIndex >= 0);
    	assert(nIndex < GetData()->nDataLength);
    	return m_pchData[nIndex];
    }

    void SetAt(int nIndex, char ch) {
    	assert(nIndex >= 0);
    	assert(nIndex < GetData()->nDataLength);
    	CopyBeforeWrite();
    	m_pchData[nIndex] = ch;
    }

    typedef const char* LPCSTR;
    operator LPCSTR() const { return m_pchData; }   	    // as a C string

    // overloaded assignment
    CString &operator =(const CString &stringSrc);

    CString &operator =(char ch) {
    	AssignCopy(1, &ch);
    	return *this;
    }

    CString &operator =(const char* lpsz) {
    	assert( lpsz == NULL || _IsValidString(lpsz) );
    	AssignCopy(SafeStrlen(lpsz), lpsz);
    	return *this;
    }

    CString &operator =(const unsigned char* lpsz) { return operator=((const char*) lpsz); }

    // string concatenation
    CString &operator +=(const CString &string) { ConcatInPlace(string.GetData()->nDataLength, string.m_pchData); return *this; }
    CString &operator +=(char ch) { ConcatInPlace(1, &ch); return *this; }

    CString &operator +=(const char* lpsz) {
    	assert( lpsz == NULL || _IsValidString(lpsz) );
    	ConcatInPlace(SafeStrlen(lpsz), lpsz);
    	return *this;
    }

    friend CString operator +(const CString& string1, const CString& string2);
    friend CString operator +(const CString& string , char  	     ch     );
    friend CString operator +(char  	     ch     , const CString& string );
    friend CString operator +(const CString& string , const char*    lpsz   );
    friend CString operator +(const char*    lpsz   , const CString& string );


    // string comparison
    int Compare(const char* lpsz) const { return _mbscmp((unsigned char*)m_pchData, (unsigned char*)lpsz); }	// straight character
    int CompareNoCase(const char* lpsz) const { return _mbsicmp((unsigned char*)m_pchData, (unsigned char*)lpsz); } // ignore case

    // simple sub-string extraction
    CString Mid  (int nFirst, int nCount) const;
    CString Mid  (int nFirst) const { return Mid(nFirst, GetData()->nDataLength - nFirst); }
    CString Left (int nCount) const;
    CString Right(int nCount) const;

    CString SpanIncluding(const char* lpszCharSet) const {  // strspn equivalent
    	assert( _IsValidString(lpszCharSet) );
    	return Left( (int)_mbsspn((unsigned char*)m_pchData, (unsigned char*)lpszCharSet) );
    }

    CString SpanExcluding(const char* lpszCharSet) const {  // strcspn equivalent
    	assert( _IsValidString(lpszCharSet) );
    	return Left( (int)_mbscspn((unsigned char*)m_pchData, (unsigned char*)lpszCharSet) );
    }

    // upper/lower/reverse conversion
    void MakeUpper() {
    	CopyBeforeWrite();
    	_mbsupr((unsigned char*)m_pchData);
    }

    void MakeLower() {
    	CopyBeforeWrite();
    	_mbslwr((unsigned char*)m_pchData);
    }

    void MakeReverse() {
    	CopyBeforeWrite();
    	_mbsrev((unsigned char*)m_pchData);
    }

    void TrimRight();
    void TrimRight(char chTarget);
    void TrimRight(const char* lpszTargetList);

    void TrimLeft();
    void TrimLeft(char chTarget);
    void TrimLeft(const char* lpszTargets);

    int Replace(char chOld, char chNew);
    int Replace(const char* lpszOld, const char* lpszNew);
    int Remove(char chRemove);
    int Insert(int nIndex, char ch);
    int Insert(int nIndex, const char* pstr);
    int Delete(int nIndex, int nCount = 1);

    // searching (return starting index, or -1 if not found)
    // look for a single character match
    int Find(int ch) const { return Find(ch, 0); }  	    	    // like "C" strchr
    int Find(int ch, int nStart) const;     	    	    	    // starting at index
    int Find(const char* lpszSub) const { return Find(lpszSub, 0); }	// like "C" strstr
    int Find(const char* lpszSub, int start) const;
    int FindOneOf(const char* lpszCharSet) const;
    int ReverseFind(int ch) const;

    CString &Append(int n);

    // simple formatting
    // formatting (using wsprintf style formatting)
    BOOL    Format(const char* lpszFormat, ...);
    BOOL    FormatV(const char* lpszFormat, va_list argList);

    char*   GetBuffer(int nMinBufLength);

    void    ReleaseBuffer(int nNewLength = -1);
    char*   GetBufferSetLength(int nNewLength);

    void FreeExtra();

    // Use LockBuffer/UnlockBuffer to turn refcounting off
    char* LockBuffer();
    void  UnlockBuffer();

    // Implementation
protected:
    // implementation helpers
    CStringData* GetData() const { assert(m_pchData != NULL); return m_pchData ? (CStringData*)m_pchData - 1 : 0; }

    void    	 Init() { m_pchData = _GetEmptyString().m_pchData; }

    static void Release(CStringData* pData);
    void    	Release();

    BOOL    	AllocBeforeWrite(int nLen);
    void    	CopyBeforeWrite();
    void    	ConcatInPlace(int nSrcLen, const char* lpszSrcData);
    BOOL    	ConcatCopy(int nSrc1Len, const char* lpszSrc1Data, int nSrc2Len, const char* lpszSrc2Data);
    void    	AssignCopy(int nSrcLen, const char* lpszSrcData);
    BOOL    	AllocBuffer(int nLen);
    BOOL    	AllocCopy(CString &dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;

    //static int    SafeStrlen(const char* lpsz) { return (lpsz == NULL) ? 0 : int(_mbslen((unsigned char*)lpsz)); }
    static int	SafeStrlen(const char* lpsz) { return (lpsz == NULL) ? 0 : int(strlen(lpsz)); }
    static int	_cstrtoi(const char* nptr)   { return strtol(nptr, 0, 0); }

    static const CString&   _GetEmptyString();	// { return *(CString*) &_atltmpPchNil; }
    static char*    	    CharNext(char* p)	    { return (char*)_mbsinc((unsigned char*)p); }
    static const char*	    CharNext(const char* p) { return (char*)_mbsinc((unsigned char*)p); }
    static int	    	    InterlockedIncrement(CStringData::S_INT* pCnt);
    static int	    	    InterlockedDecrement(CStringData::S_INT* pCnt);

    static int	    	    CountFormatV(const char* lpszFormat, va_list argList);

    static void*    	    Alloc(unsigned size);
    static void     	    Free(void* ptr);
};


// Compare helpers
inline bool operator ==(const CString &s1, const CString &s2) { return s1.Compare(s2) == 0; }
inline bool operator ==(const CString &s1, const char*	  s2) { return s1.Compare(s2) == 0; }
inline bool operator ==(const char*    s1, const CString &s2) { return s2.Compare(s1) == 0; }
inline bool operator !=(const CString &s1, const CString &s2) { return s1.Compare(s2) != 0; }
inline bool operator !=(const CString &s1, const char*	  s2) { return s1.Compare(s2) != 0; }
inline bool operator !=(const char*    s1, const CString &s2) { return s2.Compare(s1) != 0; }
inline bool operator < (const CString &s1, const CString &s2) { return s1.Compare(s2) <  0; }
inline bool operator < (const CString &s1, const char*	  s2) { return s1.Compare(s2) <  0; }
inline bool operator < (const char*    s1, const CString &s2) { return s2.Compare(s1) >  0; }
inline bool operator > (const CString &s1, const CString &s2) { return s1.Compare(s2) >  0; }
inline bool operator > (const CString &s1, const char*	  s2) { return s1.Compare(s2) >  0; }
inline bool operator > (const char*    s1, const CString &s2) { return s2.Compare(s1) <  0; }
inline bool operator <=(const CString &s1, const CString &s2) { return s1.Compare(s2) <= 0; }
inline bool operator <=(const CString &s1, const char*	  s2) { return s1.Compare(s2) <= 0; }
inline bool operator <=(const char*    s1, const CString &s2) { return s2.Compare(s1) >= 0; }
inline bool operator >=(const CString &s1, const CString &s2) { return s1.Compare(s2) >= 0; }
inline bool operator >=(const CString &s1, const char*	  s2) { return s1.Compare(s2) >= 0; }
inline bool operator >=(const char*    s1, const CString &s2) { return s2.Compare(s1) <= 0; }

#if 1	//北村	追加. 管理方法に依存して、コンパイル時定数になる文字列のアロケートを減らすための定義.
#define CSTRINGDATA(x)	    *(const CStringData*)("\xFF\xFF\x00\x00\x00\x00" ## x)
#define CSTRINGFIXED(x)     CString( CSTRINGDATA(x) )
#endif

#endif
