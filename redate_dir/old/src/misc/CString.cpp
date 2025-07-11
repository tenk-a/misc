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
 *  	ライセンスは CPL または MPL.
 */

#include "stdafx.h"

#include <string.h>
#include <stdio.h>
using namespace std;
#include "CString.h"


#ifndef min
#define min(a,b)    	(((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)    	(((b) < (a)) ? (a) : (b))
#endif


#ifndef _T
#define _T(x)	    x
#endif

#ifndef _WIN32
#ifndef _WIN64
typedef unsigned long	DWORD_PTR;
#else
typedef unsigned __int64 DWORD_PTR;
#endif
typedef const char* 	LPCSTR;
#endif



#ifndef _MSC_VER
CStringData::S_INT  CString::rgInitData[]   = { -1, 0, 0, 0 };
CStringData*	    CString::_atltmpDataNil = (CStringData*) &rgInitData;
LPCSTR	    	    CString::_atltmpPchNil  = (LPCSTR) ( ( (unsigned char*) &rgInitData ) + sizeof (CStringData) );
#else	// vs上でのメンバー表示で、表示させないようにするため、あえてメンバーをやめ.
typedef __int64     int64_t;
static CStringData::S_INT   rgInitData[]    = { -1, 0, 0, 0 };
static CStringData* 	    _atltmpDataNil  = (CStringData*) &rgInitData;
static LPCSTR	    	    _atltmpPchNil   = (LPCSTR) ( ( (unsigned char*) &rgInitData ) + sizeof (CStringData) );
#endif




// =============================================================
// アロケータ

#if defined TARGET && TARGET > 0    //適当に自前バッファを使う... heap_dualとかのほうがよかったかも...
#include "MemBlkAlc.h"
class CStringInnrBuf {
public:
    enum { UNIT_SIZE = 32   	    	    };	    // 管理ヘッダ6バイトなので、9(+1)文字が最小単位.
    enum { UNIT_NUM  = 2*1024	    	    };
    enum { SIZE      = UNIT_NUM * UNIT_SIZE };
    CStringInnrBuf() {	s_blcMem.init(&s_cstring_buf[0], SIZE, UNIT_SIZE); }

    static bool   isStringInnrBufMem(const void* p)  { return size_t((char*)p - (char*)s_cstring_buf) < SIZE; }

    static unsigned char    	    	    	    	s_cstring_buf[ SIZE ];
    static MemBlkAlc<unsigned char*, UNIT_NUM, short>	s_blcMem;
};

unsigned char	    	    	    	    	    	    CStringInnrBuf::s_cstring_buf[ CStringInnrBuf::SIZE ];
MemBlkAlc<unsigned char*, CStringInnrBuf::UNIT_NUM, short>  CStringInnrBuf::s_blcMem;
static	CStringInnrBuf	s_cstringInnrBuf;

void*	CString::Alloc(unsigned size)
{
    if (size <= 1024-sizeof(CStringData)) {
    	void *p = CStringInnrBuf::s_blcMem.alloc(size);
    	if (p)
    	    return p;
    }
    return operator new(size);
}


void	    CString::Free(void* ptr)
{
    if (CStringInnrBuf::isStringInnrBufMem(ptr)) {
    	CStringInnrBuf::s_blcMem.dealloc((unsigned char*)ptr);
    } else {
    	operator delete(ptr);
    }
}

#else
inline void*	CString::Alloc(unsigned size) { return operator new(size); }
inline void 	CString::Free(void* ptr) { operator delete(ptr); }
#endif



// =============================================================================


static inline void memcpy_x(void* pDest, size_t cbDest, const void* pSrc, size_t cbSrc)
{
    if(cbDest >= cbSrc)
    	memcpy(pDest, pSrc, cbSrc);
    else
    	assert(0);
}


static inline void memmove_x(void* pDest, size_t cbDest, const void* pSrc, size_t cbSrc)
{
    if(cbDest >= cbSrc)
    	memmove(pDest, pSrc, cbSrc);
    else
    	assert(0);
}




inline int CString::InterlockedIncrement(CStringData::S_INT* pCnt)
{
    assert(pCnt != NULL);
    assert(0 <= *pCnt && *pCnt < 0x7fff);
    ++*pCnt;
    return *pCnt;
}


inline int CString::InterlockedDecrement(CStringData::S_INT* pCnt)
{
    assert(pCnt != NULL);
    assert(0 < *pCnt && *pCnt <= 0x7fff);
    --*pCnt;
    return *pCnt;
}



CString::CString(const CString &stringSrc) {
    assert(stringSrc.GetData()->nRefs != 0);

    if (stringSrc.GetData()->nRefs >= 0) {
    	assert(stringSrc.GetData() != _atltmpDataNil);
    	m_pchData = stringSrc.m_pchData;
    	InterlockedIncrement(&GetData()->nRefs);
    } else {
    	Init();
    	*this = stringSrc.m_pchData;
    }
}



CString::CString(char ch, int nRepeat) {
    assert( !_ismbblead(ch) );	// can't create a lead byte string
    Init();
    if (nRepeat >= 1) {
    	if ( AllocBuffer(nRepeat) ) {
    	    memset(m_pchData, ch, nRepeat);
    	}
    }
}



CString::CString(LPCSTR lpsz) {
    Init();
    int nLen = SafeStrlen(lpsz);
    if (nLen != 0) {
    	if ( AllocBuffer(nLen) )
    	    memcpy_x( m_pchData, (nLen + 1) * sizeof (char), lpsz, nLen * sizeof (char) );
    }
}



CString::CString(LPCSTR lpch, int nLength) {
    Init();
    if (nLength != 0) {
    	if ( AllocBuffer(nLength) )
    	    memcpy_x( m_pchData, (nLength + 1) * sizeof (char), lpch, nLength * sizeof (char) );
    }
}



#if 0 //北村.
CString::CString(const CStringData& stringData)
{
    assert(stringData.nRefs == -1);
    m_pchData = const_cast<CStringData*>(&stringData)->data();
    ((CStringData*)&stringData)->nDataLength = strlen(m_pchData);   //北村  定数領域だけど、無理やり長さを設定... できれば...
}
#endif



// overloaded assignment
CString&    CString::operator =(const CString &stringSrc) {
    if (m_pchData != stringSrc.m_pchData) {
    	if ( (GetData()->nRefs < 0 && GetData() != _atltmpDataNil) || stringSrc.GetData()->nRefs < 0 ) {
    	    // actual copy necessary since one of the strings is locked
    	    AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
    	} else	 {
    	    // can just copy references around
    	    Release();
    	    assert(stringSrc.GetData() != _atltmpDataNil);
    	    m_pchData = stringSrc.m_pchData;
    	    InterlockedIncrement(&GetData()->nRefs);
    	}
    }
    return *this;
}



// 注意! std::stringのempty()と違い、これはclearする!
void CString::Empty() {  // free up the data
    if (GetData()->nDataLength == 0)
    	return;

    if (GetData()->nRefs >= 0)
    	Release();
    else
    	*this = _T("");

    assert(GetData()->nDataLength == 0);
    assert(GetData()->nRefs < 0 || GetData()->nAllocLength == 0);
}



CString     CString::Mid(int nFirst, int nCount) const {
    // out-of-bounds requests return sensible things
    if (nFirst < 0)
    	nFirst = 0;

    if (nCount < 0)
    	nCount = 0;

    if (nFirst + nCount > GetData()->nDataLength)
    	nCount = GetData()->nDataLength - nFirst;

    if (nFirst > GetData()->nDataLength)
    	nCount = 0;

    CString dest;
    AllocCopy(dest, nCount, nFirst, 0);
    return dest;
}



CString     CString::Left(int nCount) const {
    if (nCount < 0)
    	nCount = 0;
    else if (nCount > GetData()->nDataLength)
    	nCount = GetData()->nDataLength;

    CString 	dest;
    AllocCopy(dest, nCount, 0, 0);
    return dest;
}



CString     CString::Right(int nCount) const {
    if (nCount < 0)
    	nCount = 0;
    else if (nCount > GetData()->nDataLength)
    	nCount = GetData()->nDataLength;

    CString dest;
    AllocCopy(dest, nCount, GetData()->nDataLength - nCount, 0);
    return dest;
}



// trimming whitespace (either side)
void CString::TrimRight() {
    CopyBeforeWrite();

    // find beginning of trailing spaces by starting at beginning (DBCS aware)
    char*   lpsz     = m_pchData;
    char*   lpszLast = NULL;

    while ( *lpsz != _T('\0') ) {
    	if ( isspace(*lpsz) ) {
    	    if (lpszLast == NULL)
    	    	lpszLast = lpsz;
    	} else	 {
    	    lpszLast = NULL;
    	}

    	lpsz = CharNext(lpsz);
    }

    if (lpszLast != NULL) {
    	// truncate at trailing space start
    	*lpszLast   	       = _T('\0');
    	GetData()->nDataLength = (int) (DWORD_PTR) (lpszLast - m_pchData);
    }
}


void CString::TrimLeft() {
    CopyBeforeWrite();

    // find first non-space character
    LPCSTR lpsz     = m_pchData;

    while ( isspace(*lpsz) )
    	lpsz = CharNext(lpsz);

    // fix up data and length
    int     nDataLength = GetData()->nDataLength - (int) (DWORD_PTR) (lpsz - m_pchData);
    memmove_x( m_pchData, (GetData()->nAllocLength + 1) * sizeof (char), lpsz, (nDataLength + 1) * sizeof (char) );
    GetData()->nDataLength = nDataLength;
}



// remove continuous occurrences of chTarget starting from right
void CString::TrimRight(char chTarget) {
    // find beginning of trailing matches
    // by starting at beginning (DBCS aware)

    CopyBeforeWrite();
    char* lpsz	= m_pchData;
    char* lpszLast = NULL;

    while ( *lpsz != _T('\0') ) {
    	if (*lpsz == chTarget) {
    	    if (lpszLast == NULL)
    	    	lpszLast = lpsz;
    	} else
    	    lpszLast = NULL;

    	lpsz = CharNext(lpsz);
    }

    if (lpszLast != NULL) {
    	// truncate at left-most matching character
    	*lpszLast   	       = _T('\0');
    	GetData()->nDataLength = (int) (DWORD_PTR) (lpszLast - m_pchData);
    }
}



// remove continuous occcurrences of characters in passed string, starting from right
void CString::TrimRight(LPCSTR lpszTargetList) {
    // find beginning of trailing matches by starting at beginning (DBCS aware)

    CopyBeforeWrite();
    char* lpsz	= m_pchData;
    char* lpszLast = NULL;

    while ( *lpsz != _T('\0') ) {
    	char* pNext = CharNext(lpsz);
      #if 1
    	int   c     = *(unsigned char*)lpsz;
    	if (pNext > lpsz + 1)
    	    c = (c << 8) | *((unsigned char*)lpsz + 1);
    	if (_mbschr((unsigned char*)lpszTargetList, c) != NULL) {
    	    if (lpszLast == NULL)
    	    	lpszLast = lpsz;
    	} else	 {
    	    lpszLast = NULL;
    	}
      #else
    	if (pNext > lpsz + 1) {

    	    if (_cstrchr_db( lpszTargetList, *lpsz, *(lpsz + 1) ) != NULL) {
    	    	if (lpszLast == NULL)
    	    	    lpszLast = lpsz;
    	    } else   {
    	    	lpszLast = NULL;
    	    }
    	} else	 {
    	    if (_mbschr(lpszTargetList, *lpsz) != NULL) {
    	    	if (lpszLast == NULL)
    	    	    lpszLast = lpsz;
    	    } else   {
    	    	lpszLast = NULL;
    	    }
    	}
      #endif

    	lpsz = pNext;
    }

    if (lpszLast != NULL) {
    	// truncate at left-most matching character
    	*lpszLast   	       = _T('\0');
    	GetData()->nDataLength = (int) (DWORD_PTR) (lpszLast - m_pchData);
    }
}




// remove continuous occurrences of chTarget starting from left
void CString::TrimLeft(char chTarget) {
    // find first non-matching character

    CopyBeforeWrite();
    LPCSTR lpsz = m_pchData;

    while (chTarget == *lpsz)
    	lpsz = CharNext(lpsz);

    if (lpsz != m_pchData) {
    	// fix up data and length
    	int nDataLength = GetData()->nDataLength - (int) (DWORD_PTR) (lpsz - m_pchData);
    	memmove_x( m_pchData, (GetData()->nAllocLength + 1) * sizeof (char), lpsz, (nDataLength + 1) * sizeof (char) );
    	GetData()->nDataLength = nDataLength;
    }
}



// remove continuous occcurrences of characters in passed string, starting from left
void CString::TrimLeft(LPCSTR lpszTargets) {
    // if we're not trimming anything, we're not doing any work
    if (SafeStrlen(lpszTargets) == 0)
    	return;

    CopyBeforeWrite();
    LPCSTR lpsz = m_pchData;

    while ( *lpsz != _T('\0') ) {
    	const char* pNext = CharNext(lpsz);
      #if 1
    	int   c     = *(unsigned char*)lpsz;
    	if (pNext > lpsz + 1)
    	    c = (c << 8) | *((unsigned char*)lpsz + 1);
    	if (_mbschr((unsigned char*)lpszTargets, c) == NULL) {
    	    break;
    	}
      #else
    	if (pNext > lpsz + 1) {
    	    if (_cstrchr_db( lpszTargets, *lpsz, *(lpsz + 1) ) == NULL)
    	    	break;
    	} else	 {
    	    if (_mbschr(lpszTargets, *lpsz) == NULL)
    	    	break;
    	}
      #endif
    	lpsz = pNext;
    }

    if (lpsz != m_pchData) {
    	// fix up data and length
    	int nDataLength = GetData()->nDataLength - (int) (DWORD_PTR) (lpsz - m_pchData);
    	memmove_x( m_pchData, (GetData()->nAllocLength + 1) * sizeof (char), lpsz, (nDataLength + 1) * sizeof (char) );
    	GetData()->nDataLength = nDataLength;
    }
}




// advanced manipulation
// replace occurrences of chOld with chNew
int CString::Replace(char chOld, char chNew) {
    int nCount = 0;

    // short-circuit the nop case
    if (chOld != chNew) {
    	// otherwise modify each character that matches in the string
    	CopyBeforeWrite();
    	char* psz   	= m_pchData;
    	char* pszEnd	= psz + GetData()->nDataLength;

    	while (psz < pszEnd) {
    	    // replace instances of the specified character only
    	    if (*psz == chOld) {
    	    	*psz = chNew;
    	    	nCount++;
    	    }

    	    psz = CharNext(psz);
    	}
    }

    return nCount;
}



// replace occurrences of substring lpszOld with lpszNew;
// empty lpszNew removes instances of lpszOld
int CString::Replace(LPCSTR lpszOld, LPCSTR lpszNew) {
    // can't have empty or NULL lpszOld

    int    nSourceLen	   = SafeStrlen(lpszOld);

    if (nSourceLen == 0)
    	return 0;

    int    nReplacementLen = SafeStrlen(lpszNew);

    // loop once to figure out the size of the result string
    int    nCount   	   = 0;
    char* lpszStart    = m_pchData;
    char* lpszEnd   	   = m_pchData + GetData()->nDataLength;
    char* lpszTarget	   = NULL;

    while (lpszStart < lpszEnd) {
    	while ( ( lpszTarget = (char*) _mbsstr((unsigned char*)lpszStart, (unsigned char*)lpszOld) ) != NULL ) {
    	    nCount++;
    	    lpszStart = lpszTarget + nSourceLen;
    	}

    	lpszStart += _tcslen((LPCTSTR)lpszStart) + 1;
    }

    // if any changes were made, make them
    if (nCount > 0) {
    	CopyBeforeWrite();

    	// if the buffer is too small, just allocate a new buffer (slow but sure)
    	int nOldLength = GetData()->nDataLength;
    	int nNewLength =  nOldLength + (nReplacementLen - nSourceLen) * nCount;

    	if (GetData()->nAllocLength < nNewLength || GetData()->nRefs > 1) {
    	    CStringData* pOldData = GetData();
    	    char*   	 pstr	  = m_pchData;

    	    if ( !AllocBuffer(nNewLength) )
    	    	return -1;

    	    memcpy_x( m_pchData, (nNewLength + 1) * sizeof (char), pstr, pOldData->nDataLength * sizeof (char) );
    	    CString::Release(pOldData);
    	}

    	// else, we just do it in-place
    	lpszStart   	       = m_pchData;
    	lpszEnd     	       = m_pchData + GetData()->nDataLength;

    	// loop again to actually do the work
    	while (lpszStart < lpszEnd) {
    	    while ( ( lpszTarget = (char*) _mbsstr((unsigned char*)lpszStart, (unsigned char*)lpszOld) ) != NULL ) {
    	    	int nBalance   = nOldLength - ( (int) (DWORD_PTR) (lpszTarget - m_pchData) + nSourceLen );
    	    	int cchBuffLen = GetData()->nAllocLength - (int) (DWORD_PTR) (lpszTarget - m_pchData);
    	    	memmove_x( lpszTarget + nReplacementLen, (cchBuffLen - nReplacementLen + 1) * sizeof (char),
    	    	    	    	    	lpszTarget + nSourceLen,
    	    	    	    	    	nBalance * sizeof (char) );
    	    	memcpy_x( lpszTarget, (cchBuffLen + 1) * sizeof (char), lpszNew, nReplacementLen * sizeof (char) );
    	    	lpszStart   	    = lpszTarget + nReplacementLen;
    	    	lpszStart[nBalance] = _T('\0');
    	    	nOldLength  	   += (nReplacementLen - nSourceLen);
    	    }

    	    lpszStart += _tcslen((LPCTSTR)lpszStart) + 1;
    	}

    	assert( m_pchData[nNewLength] == _T('\0') );
    	GetData()->nDataLength = nNewLength;
    }

    return nCount;
}



// remove occurrences of chRemove
int CString::Remove(char chRemove) {
    CopyBeforeWrite();

    char* pstrSource = m_pchData;
    char* pstrDest   = m_pchData;
    char* pstrEnd     = m_pchData + GetData()->nDataLength;

    while (pstrSource < pstrEnd) {
    	if (*pstrSource != chRemove) {
    	    *pstrDest = *pstrSource;
    	    pstrDest  = CharNext(pstrDest);
    	}

    	pstrSource = CharNext(pstrSource);
    }

    *pstrDest	    	    = _T('\0');
    int    nCount     = (int) (DWORD_PTR) (pstrSource - pstrDest);
    GetData()->nDataLength -= nCount;

    return nCount;
}



// insert character at zero-based index; concatenates if index is past end of string
int CString::Insert(int nIndex, char ch) {
    CopyBeforeWrite();

    if (nIndex < 0)
    	nIndex = 0;

    int nNewLength = GetData()->nDataLength;

    if (nIndex > nNewLength)
    	nIndex = nNewLength;

    nNewLength++;

    if (GetData()->nAllocLength < nNewLength) {
    	CStringData* pOldData = GetData();
    	char*	     pstr     = m_pchData;

    	if ( !AllocBuffer(nNewLength) )
    	    return -1;

    	memcpy_x( m_pchData, (nNewLength + 1) * sizeof (char), pstr, (pOldData->nDataLength + 1) * sizeof (char) );
    	CString::Release(pOldData);
    }

    // move existing bytes down
    memmove_x( m_pchData + nIndex + 1,
    	    	    	    (GetData()->nAllocLength - nIndex) * sizeof (char), m_pchData + nIndex,
    	    	    	    (nNewLength - nIndex) * sizeof (char) );
    m_pchData[nIndex]	   = ch;
    GetData()->nDataLength = nNewLength;

    return nNewLength;
}



// insert substring at zero-based index; concatenates if index is past end of string
int CString::Insert(int nIndex, LPCSTR pstr) {
    if (nIndex < 0)
    	nIndex = 0;

    int nInsertLength = SafeStrlen(pstr);
    int nNewLength    = GetData()->nDataLength;

    if (nInsertLength > 0) {
    	CopyBeforeWrite();

    	if (nIndex > nNewLength)
    	    nIndex = nNewLength;

    	nNewLength  	      += nInsertLength;

    	if (GetData()->nAllocLength < nNewLength) {
    	    CStringData* pOldData = GetData();
    	    char*   	 pstr	  = m_pchData;

    	    if ( !AllocBuffer(nNewLength) )
    	    	return -1;

    	    memcpy_x( m_pchData, (nNewLength + 1) * sizeof (char), pstr, (pOldData->nDataLength + 1) * sizeof (char) );
    	    CString::Release(pOldData);
    	}

    	// move existing bytes down
    	memmove_x( m_pchData + nIndex + nInsertLength,
    	    	    	    	(GetData()->nAllocLength + 1 - nIndex - nInsertLength) * sizeof (char), m_pchData + nIndex,
    	    	    	    	(nNewLength - nIndex - nInsertLength + 1) * sizeof (char) );
    	memcpy_x( m_pchData + nIndex, (GetData()->nAllocLength + 1 - nIndex) * sizeof (char), pstr, nInsertLength * sizeof (char) );
    	GetData()->nDataLength = nNewLength;
    }

    return nNewLength;
}



// delete nCount characters starting at zero-based index
int CString::Delete(int nIndex, int nCount) {
    if (nIndex < 0)
    	nIndex = 0;

    int nLength = GetData()->nDataLength;

    if (nCount > 0 && nIndex < nLength) {
    	if ( (nIndex + nCount) > nLength )
    	    nCount = nLength - nIndex;

    	CopyBeforeWrite();
    	int nBytesToCopy = nLength - (nIndex + nCount) + 1;

    	memmove_x( m_pchData + nIndex,
    	    	    	    	(GetData()->nAllocLength + 1 - nIndex) * sizeof (char), m_pchData + nIndex + nCount,
    	    	    	    	nBytesToCopy * sizeof (char) );
    	nLength     	      -= nCount;
    	GetData()->nDataLength = nLength;
    }

    return nLength;
}



int CString::ReverseFind(int ch) const {
    // find last single character
    LPCSTR lpsz = (LPCSTR)_mbsrchr((unsigned char*)m_pchData, ch);
    // return -1 if not found, distance from beginning otherwise
    return (lpsz == NULL) ? -1 : (int) (lpsz - m_pchData);
}


// look for a specific sub-string
// find a sub-string (like strstr)
int CString::Find(int ch, int nStart) const 	    // starting at index
{
    int     nLength = GetData()->nDataLength;

    if (nStart < 0 || nStart >= nLength)
    	return -1;

    // find first single character
    LPCSTR lpsz = (LPCSTR)_mbschr((unsigned char*)m_pchData + nStart, ch);

    // return -1 if not found and index otherwise
    return (lpsz == NULL) ? -1 : (int) (lpsz - m_pchData);
}



int CString::Find(LPCTSTR lpszSub, int nStart) const   // starting at index
{
    assert(_IsValidString(lpszSub));

    int nLength = GetData()->nDataLength;
    if (nStart < 0 || nStart > nLength)
    	return -1;

    // find first matching substring
    LPCTSTR lpsz = (LPCTSTR)_mbsstr((unsigned char*)m_pchData + nStart, (unsigned char*)lpszSub);

    // return -1 for not found, distance from beginning otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}




int CString::FindOneOf(LPCSTR lpszCharSet) const {
    assert( _IsValidString(lpszCharSet) );
    LPCSTR lpsz = (LPCSTR)_mbspbrk((unsigned char*)m_pchData, (unsigned char*)lpszCharSet);
    return (lpsz == NULL) ? -1 : (int) (lpsz - m_pchData);
}



// Concatentation for non strings
CString&    CString::Append(int n) {
    const int cchBuff	    	= 12+4;
    char      szBuffer[cchBuff] = { 0 };
    sprintf(szBuffer, /*cchBuff,*/ _T("%d"), n);
    ConcatInPlace(SafeStrlen(szBuffer), szBuffer);
    return *this;
}



// simple formatting
// formatting (using wsprintf style formatting)
BOOL CString::Format(LPCSTR lpszFormat, ...) {
    assert( _IsValidString(lpszFormat) );

    va_list argList;
    va_start(argList, lpszFormat);
    BOOL    bRet = FormatV(lpszFormat, argList);
    va_end(argList);
    return bRet;
}



BOOL CString::FormatV(LPCSTR lpszFormat, va_list argList) {
    assert( _IsValidString(lpszFormat) );

    int nMaxLen = CountFormatV(lpszFormat, argList);

    if (GetBuffer(nMaxLen) == NULL)
    	return 0;

    int size = GetAllocLength()+1;
    int rc   = 0;
    if (size > 0)
    	rc   = vsprintf(m_pchData, /*size,*/ lpszFormat, argList);
    assert( rc < size );
    ReleaseBuffer();
    va_end(argList);
    return 1;
}



int CString::CountFormatV(LPCSTR lpszFormat, va_list argList)
{
    enum _FormatModifiers {
    	FORCE_ANSI    = 0x10000,
    	FORCE_UNICODE = 0x20000,
    	FORCE_INT64   = 0x40000
    };

    // make a guess at the maximum length of the resulting string
    int     nMaxLen 	= 0;
    for ( LPCSTR lpsz = lpszFormat; *lpsz != _T('\0'); lpsz = CharNext(lpsz) ) {
    	// handle '%' character, but watch out for '%%'
    	if ( *lpsz != _T('%') || *( lpsz = CharNext(lpsz) ) == _T('%') ) {
    	    nMaxLen += (int) (CharNext(lpsz) - lpsz);
    	    continue;
    	}

    	int nItemLen   = 0;

    	// handle '%' character with format
    	int nWidth     = 0;

    	for ( ; *lpsz != _T('\0'); lpsz = CharNext(lpsz) ) {
    	    // check for valid flags
    	    if ( *lpsz == _T('#') )
    	    	nMaxLen += 2;	// for '0x'
    	    else if ( *lpsz == _T('*') )
    	    	nWidth = va_arg(argList, int);
    	    else if ( *lpsz == _T('-') || *lpsz == _T('+') || *lpsz == _T('0') || *lpsz == _T(' ') )
    	    	;
    	    else    	    	// hit non-flag character
    	    	break;
    	}

    	// get width and skip it
    	if (nWidth == 0) {
    	    // width indicated by
    	    nWidth = _cstrtoi(lpsz);

    	    for ( ; *lpsz != _T('\0') && isdigit(*lpsz); lpsz = CharNext(lpsz) )
    	    	;
    	}

    	assert(nWidth >= 0);

    	int nPrecision = 0;

    	if ( *lpsz == _T('.') ) {
    	    // skip past '.' separator (width.precision)
    	    lpsz = CharNext(lpsz);

    	    // get precision and skip it
    	    if ( *lpsz == _T('*') ) {
    	    	nPrecision = va_arg(argList, int);
    	    	lpsz	   = CharNext(lpsz);
    	    } else   {
    	    	nPrecision = _cstrtoi(lpsz);

    	    	for ( ; *lpsz != _T('\0') && isdigit(*lpsz); lpsz = CharNext(lpsz) )
    	    	    ;
    	    }

    	    assert(nPrecision >= 0);
    	}

    	// should be on type modifier or specifier
    	int nModifier  = 0;

    	if ( lpsz[0] == _T('I') && lpsz[1] == _T('6') && lpsz[2] == _T('4') ) {
    	    lpsz     += 3;
    	    nModifier = FORCE_INT64;
    	} else	 {
    	    switch (*lpsz) {
    	    	// modifiers that affect size
    	    case _T('h'):
    	    	nModifier = FORCE_ANSI;
    	    	lpsz	  = CharNext(lpsz);
    	    	break;

    	    case _T('l'):
    	    	nModifier = FORCE_UNICODE;
    	    	lpsz	  = CharNext(lpsz);
    	    	break;

    	    	// modifiers that do not affect size
    	    case _T('F'):
    	    case _T('N'):
    	    case _T('L'):
    	    	lpsz	  = CharNext(lpsz);
    	    	break;
    	    }
    	}

    	// now should be on specifier
    	switch (*lpsz | nModifier) {
    	    // single characters
    	case _T('c'):
    	case _T('C'):
    	    nItemLen = 2;
    	    va_arg(argList, char);
    	    break;

    	case _T('c') | FORCE_ANSI:
    	case _T('C') | FORCE_ANSI:
    	    nItemLen = 2;
    	    va_arg(argList, char);
    	    break;

    	case _T('c') | FORCE_UNICODE:
    	case _T('C') | FORCE_UNICODE:
    	    nItemLen = 2;
    	    va_arg(argList, wchar_t);
    	    break;

    	    // strings
    	case _T('s'):
    	    {
    	    	LPCSTR pstrNextArg = va_arg(argList, LPCSTR);

    	    	if (pstrNextArg == NULL) {
    	    	    nItemLen = 6;   // "(null)"
    	    	} else	 {
    	    	    nItemLen = (int)_tcslen((LPCTSTR)pstrNextArg);
    	    	    nItemLen = max(1, nItemLen);
    	    	}

    	    	break;
    	    }

    	case _T('s') | FORCE_ANSI:
    	case _T('S') | FORCE_ANSI:
    	    {
    	    	LPCSTR pstrNextArg = va_arg(argList, LPCSTR);

    	    	if (pstrNextArg == NULL) {
    	    	    nItemLen = 6;   // "(null)"
    	    	} else	 {
    	    	    nItemLen = (int)strlen(pstrNextArg);
    	    	    nItemLen = max(1, nItemLen);
    	    	}

    	    	break;
    	    }
    	}

    	// adjust nItemLen for strings
    	if (nItemLen != 0) {
    	    nItemLen = max(nItemLen, nWidth);

    	    if (nPrecision != 0)
    	    	nItemLen = min(nItemLen, nPrecision);
    	} else {
    	    switch (*lpsz) {
    	    	// integers
    	    case _T('d'):
    	    case _T('i'):
    	    case _T('u'):
    	    case _T('x'):
    	    case _T('X'):
    	    case _T('o'):

    	    	if (nModifier & FORCE_INT64)
    	    	    va_arg(argList, int64_t);
    	    	else
    	    	    va_arg(argList, int);

    	    	nItemLen = 32;
    	    	nItemLen = max(nItemLen, nWidth + nPrecision);
    	    	break;

    	  #ifndef USE_CSTRING_FLOAT
    	    case _T('e'):
    	    case _T('E'):
    	    case _T('f'):
    	    case _T('g'):
    	    case _T('G'):
    	    	assert(!"floating point (%%e, %%E, %%f, %%g, and %%G) is not supported by the WTL::CString class.");
    	    	break;

    	  #else // USE_CSTRING_FLOAT
    	    case _T('e'):
    	    case _T('E'):
    	    case _T('g'):
    	    case _T('G'):
    	    	va_arg(argList, double);
    	    	nItemLen = 128;
    	    	nItemLen = max(nItemLen, nWidth + nPrecision);
    	    	break;

    	    case _T('f'):
    	    	{
    	    	    va_arg(argList, double);
    	    	    // 312 == strlen("-1+(309 zeroes).")
    	    	    // 309 zeroes == max precision of a double
    	    	    // 6 == adjustment in case precision is not specified,
    	    	    //	 which means that the precision defaults to 6
    	    	    nItemLen	    = max(nWidth, 312 + nPrecision + 6);
    	    	}
    	    	break;
    	  #endif    // USE_CSTRING_FLOAT

    	    case _T('p'):
    	    	va_arg(argList, void*);
    	    	nItemLen = 32;
    	    	nItemLen = max(nItemLen, nWidth + nPrecision);
    	    	break;

    	    	// no output
    	    case _T('n'):
    	    	va_arg(argList, int*);
    	    	break;

    	    default:
    	    	assert(0);  // unknown formatting option
    	    }
    	}

    	// adjust nMaxLen for output nItemLen
    	nMaxLen += nItemLen;
    }
    return nMaxLen;
}



// Access to string implementation buffer as "C" character array
char* CString::GetBuffer(int nMinBufLength) {
    assert(nMinBufLength >= 0);

    if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength) {
    	// we have to grow the buffer
    	CStringData* pOldData = GetData();
    	int 	     nOldLen  = GetData()->nDataLength; // AllocBuffer will tromp it

    	if (nMinBufLength < nOldLen)
    	    nMinBufLength = nOldLen;

    	if ( !AllocBuffer(nMinBufLength) )
    	    return NULL;

    	memcpy_x( m_pchData, (nMinBufLength + 1) * sizeof (char), pOldData->data(), (nOldLen + 1) * sizeof (char) );
    	GetData()->nDataLength = nOldLen;
    	CString::Release(pOldData);
    }

    assert(GetData()->nRefs <= 1);

    // return a pointer to the character storage for this string
    assert(m_pchData != NULL);
    return m_pchData;
}



void CString::ReleaseBuffer(int nNewLength) {
    CopyBeforeWrite();	 // just in case GetBuffer was not called

    if (nNewLength == -1)
    	nNewLength = (int)_tcslen((LPCTSTR)m_pchData);	 // zero terminated

    assert(nNewLength <= GetData()->nAllocLength);
    GetData()->nDataLength = nNewLength;
    m_pchData[nNewLength]  = _T('\0');
}



char* CString::GetBufferSetLength(int nNewLength) {
    assert(nNewLength >= 0);

    if (GetBuffer(nNewLength) == NULL)
    	return NULL;

    GetData()->nDataLength = nNewLength;
    m_pchData[nNewLength]  = _T('\0');
    return m_pchData;
}



void CString::FreeExtra() {
    assert(GetData()->nDataLength <= GetData()->nAllocLength);

    if (GetData()->nDataLength != GetData()->nAllocLength) {
    	CStringData* pOldData = GetData();

    	if ( AllocBuffer(GetData()->nDataLength) ) {
    	    memcpy_x( m_pchData, (GetData()->nAllocLength + 1) * sizeof (char), pOldData->data(), pOldData->nDataLength * sizeof (char) );
    	    assert( m_pchData[GetData()->nDataLength] == _T('\0') );
    	    CString::Release(pOldData);
    	}
    }

    assert(GetData() != NULL);
}



char* CString::LockBuffer() {
    char*   lpsz = GetBuffer(0);

    if (lpsz != NULL)
    	GetData()->nRefs = -1;

    return lpsz;
}


void CString::UnlockBuffer() {
    assert(GetData()->nRefs == -1);

    if (GetData() != _atltmpDataNil)
    	GetData()->nRefs = 1;
}



BOOL CString::AllocCopy(CString &dest, int nCopyLen, int nCopyIndex, int nExtraLen) const {
    // will clone the data attached to this string
    // allocating 'nExtraLen' characters
    // Places results in uninitialized string 'dest'
    // Will copy the part or all of original data to start of new string

    BOOL bRet	 = 0;
    int  nNewLen = nCopyLen + nExtraLen;

    if (nNewLen == 0) {
    	dest.Init();
    	bRet = 1;
    } else if (nNewLen >= nCopyLen)    {
    	if ( dest.AllocBuffer(nNewLen) ) {
    	    memcpy_x( dest.m_pchData, (nNewLen + 1) * sizeof (char), m_pchData + nCopyIndex, nCopyLen * sizeof (char) );
    	    bRet = 1;
    	}
    }

    return bRet;
}



// always allocate one extra character for '\0' termination
// assumes [optimistically] that data length will equal allocation length
BOOL CString::AllocBuffer(int nLen) {
    assert(nLen >= 0);
    assert(nLen <= INT_MAX - 1);    // max size (enough room for 1 extra)

    if (nLen == 0) {
    	Init();
    } else {
    	CStringData*	pData = NULL;
    	size_t	    	len   = sizeof(CStringData) + (nLen + 1) * sizeof (char);
    	assert(len <= CStringData::MAX_LENGTH);
    	if (len <= CStringData::MAX_LENGTH)
    	    pData = (CStringData*) Alloc(len);
    	if (pData == NULL)
    	    return 0;

    	pData->nRefs	    = 1;
    	pData->data()[nLen] = _T('\0');
    	pData->nDataLength  = nLen;
    	pData->nAllocLength = nLen;
    	m_pchData   	    = pData->data();
    }

    return 1;
}



// Assignment operators
//  All assign a new value to the string
//  	(a) first see if the buffer is big enough
//  	(b) if enough room, copy on top of old buffer, set size and type
//  	(c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CString&' so that
//  	assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//
void CString::AssignCopy(int nSrcLen, LPCSTR lpszSrcData) {
    if ( AllocBeforeWrite(nSrcLen) ) {
    	memcpy_x( m_pchData, (nSrcLen + 1) * sizeof (char), lpszSrcData, nSrcLen * sizeof (char) );
    	GetData()->nDataLength = nSrcLen;
    	m_pchData[nSrcLen]     = _T('\0');
    }
}



// Concatenation
// NOTE: "operator +" is done as friend functions for simplicity
//  	There are three variants:
//  	    CString + CString
// and for ? = char, LPCSTR
//  	    CString + ?
//  	    ? + CString
BOOL CString::ConcatCopy(int nSrc1Len, LPCSTR lpszSrc1Data, int nSrc2Len, LPCSTR lpszSrc2Data) {
    // -- master concatenation routine
    // Concatenate two sources
    // -- assume that 'this' is a new CString object

    BOOL bRet	 = 1;
    int  nNewLen = nSrc1Len + nSrc2Len;

    if (nNewLen < nSrc1Len || nNewLen < nSrc2Len) {
    	bRet = 0;
    } else if (nNewLen != 0)	{
    	bRet = AllocBuffer(nNewLen);

    	if (bRet) {
    	    memcpy_x( m_pchData, (nNewLen + 1) * sizeof (char), lpszSrc1Data, nSrc1Len * sizeof (char) );
    	    memcpy_x( m_pchData + nSrc1Len, (nNewLen + 1 - nSrc1Len) * sizeof (char), lpszSrc2Data, nSrc2Len * sizeof (char) );
    	}
    }

    return bRet;
}



void CString::ConcatInPlace(int nSrcLen, LPCSTR lpszSrcData) {
    //	-- the main routine for += operators

    // concatenating an empty string is a no-op!
    if (nSrcLen == 0)
    	return;

    // if the buffer is too small, or we have a width mis-match, just
    //	 allocate a new buffer (slow but sure)
    if (GetData()->nRefs > 1 || GetData()->nDataLength + nSrcLen > GetData()->nAllocLength) {
    	// we have to grow the buffer, use the ConcatCopy routine
    	CStringData* pOldData = GetData();

    	if ( ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData) ) {
    	    assert(pOldData != NULL);
    	    CString::Release(pOldData);
    	}
    } else   {
    	// fast concatenation when buffer big enough
    	memcpy_x( m_pchData + GetData()->nDataLength, (GetData()->nAllocLength + 1) * sizeof (char), lpszSrcData, nSrcLen * sizeof (char) );
    	GetData()->nDataLength	    	 += nSrcLen;
    	assert(GetData()->nDataLength <= GetData()->nAllocLength);
    	m_pchData[GetData()->nDataLength] = _T('\0');
    }
}



void CString::CopyBeforeWrite() {
    if (GetData()->nRefs > 1) {
    	CStringData* pData = GetData();
    	Release();

    	if ( AllocBuffer(pData->nDataLength) )
    	    memcpy_x( m_pchData, (GetData()->nAllocLength + 1) * sizeof (char), pData->data(), (pData->nDataLength + 1) * sizeof (char) );
    }

    assert(GetData()->nRefs <= 1);
}



BOOL CString::AllocBeforeWrite(int nLen) {
    BOOL bRet = 1;

    if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength) {
    	Release();
    	bRet = AllocBuffer(nLen);
    }

    assert(GetData()->nRefs <= 1);
    return bRet;
}



CString::~CString() {	    	    	    //	free any attached data
  #if 1
    CStringData*    pData = GetData();
    if (pData != _atltmpDataNil && pData) {
    	if (InterlockedDecrement(&pData->nRefs) <= 0)
    	    Free( (unsigned char*)pData );
    }
  #else
    if (GetData() != _atltmpDataNil) {
    	assert(GetData()->nRefs != 0);
    	if (InterlockedDecrement(&GetData()->nRefs) <= 0)
    	    Free( (unsigned char*)GetData() );
    }
  #endif
}



void CString::Release() {
  #if 1
    CStringData*    pData = GetData();
    if (pData != _atltmpDataNil && pData) {
    	if (InterlockedDecrement(&pData->nRefs) <= 0)
    	    Free( (unsigned char*)pData );

    	Init();
    }
  #else
    if (GetData() != _atltmpDataNil) {
    	assert(GetData()->nRefs != 0);
    	if (InterlockedDecrement(&GetData()->nRefs) <= 0)
    	    Free( (unsigned char*)GetData() );

    	Init();
    }
  #endif
}



void CString::Release(CStringData* pData) {
  #if 1
    if (pData != _atltmpDataNil && pData)
  #else
    if (pData != _atltmpDataNil)
  #endif
    {
    	assert(pData->nRefs != 0);
    	if (InterlockedDecrement(&pData->nRefs) <= 0)
    	    Free( (unsigned char*) pData );
    }
}



const CString&	CString::_GetEmptyString()
{
    return *(CString*) &_atltmpPchNil;
}



// CString "operator +" functions
CString operator +(const CString &string1, const CString &string2) {
    CString s;
    s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData, string2.GetData()->nDataLength, string2.m_pchData);
    return s;
}


CString operator +(const CString &string, char ch) {
    CString s;
    s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData, 1, &ch);
    return s;
}


CString operator +(char ch, const CString &string) {
    CString s;
    s.ConcatCopy(1, &ch, string.GetData()->nDataLength, string.m_pchData);
    return s;
}


CString operator +(const CString &string, LPCSTR lpsz) {
    assert( lpsz == NULL || CString::_IsValidString(lpsz) );
    CString s;
    s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData, CString::SafeStrlen(lpsz), lpsz);
    return s;
}


CString operator +(LPCSTR lpsz, const CString &string) {
    assert( lpsz == NULL || CString::_IsValidString(lpsz) );
    CString s;
    s.ConcatCopy(CString::SafeStrlen(lpsz), lpsz, string.GetData()->nDataLength, string.m_pchData);
    return s;
}



#if 0
void CString_test()
{
    //CSTRING_DEF(test1, "test1");
    CString fxstr = CSTRINGDATA("fixed string");
    CString str0;
    CString str1 = "1";
    CString str2("2a2");
    CString str3('3');
    CString str4(10, '4');

    str0 += str1;
    str0  = str0 + str2;
    str1  = '1' + str3;
    str2  = "2" + str2;

    DBG_PRINTF(("%s %s %s %s %s\n", LPCSTR(str0), LPCSTR(str1), str2.GetBuffer(0), LPCSTR(str3), LPCSTR(str4)));
}
#endif







// ===========================================================================
// ===========================================================================

// CString で使われる mbstring.h 関係をフリーソフトよりコピペ.

#ifndef _WIN32
//extern "C" {

/** sを１文字分アドレスを進めたアドレスを返す.
 */
unsigned char*	_mbsinc   (const unsigned char* s)
{
    size_t num = s != NULL;
    num += num && (_ismbblead(*s) != 0) && (s[1] != 0);
    return (unsigned char*)s + num;
}



/** 本来のmbstring.hに含まれない関数.
 *  文字列strの最後'\0'のアドレスを返す.
 */
static unsigned char *_mbsend(const unsigned char* str)
{
    const unsigned char *s;

    s = (const unsigned char *)str;
    while (*s != '\0') {
    	if (_ismbblead(*s)) {
    	    s++;
    	    if (*s == '\0') {
    	    	--s;
    	    	break;
    	    }
    	}
    	s++;
    }
    return (unsigned char*)s;
}




/** 文字列のバイト数を返す.
 */
size_t	    	_mbslen(const unsigned char* str)
{
  #if 0
    return _mbsend(str) - str;
  #else
    const unsigned char  *s;

    s = (const unsigned char *)str;
    while (*s != '\0') {
    	if (_ismbblead(*s)) {
    	    s++;
    	    if (*s == '\0')
    	    	break;
    	}
    	s++;
    }
    return s - str;
  #endif
}



/** 文字列コピー.
 */
unsigned char*	_mbscpy   (unsigned char* dst, const unsigned char* s)
{
    unsigned char   *d;
    int     	    c;

    assert(dst != NULL);
    assert(s   != NULL);

    d = dst;
    while ((c = *d++ = *s++) != '\0') {
    	if (_ismbblead(c) != 0) {
    	    if (*s == '\0') {
    	    	*--d = '\0';
    	    	break;
    	    }
    	    *d++ = *s++;
    	}
    	;
    }
    return dst;
}



/** 文字列dstをcで埋める.
 */
unsigned char*	_mbsset   (unsigned char* dst, unsigned c)
{
    unsigned char  *d, *e;
    char  a,b;

    d = dst;
    e = d;
    while (*e != '\0')
    	e++;
    if (c < 0x100) {
    	while (d != e)
    	    *d++ = c;
    } else {
    	a = c >> 8;
    	b = c;
    	c = (e - d) & 1;
    	e -= c;     	    	/* 偶数バイトにする */
    	while (d != e) {
    	    *d++ = a;
    	    *d++ = b;
    	}
    	if (c)	    	    	/* 端数には 空白を詰める */
    	    *d++ = ' ';
    	*d = 0;
    }
    return dst;
}




/** 文字列反転.
 */
unsigned char*	_mbsrev   (unsigned char* str)
{
    unsigned char *s;
    unsigned char *e;
    int     	  c;

    s = (unsigned char *)str;
    e = s;
    while ((c = *e) != '\0') {
    	if (_ismbblead(c)) {
    	    if (e[1] == '\0')
    	    	break;
    	    e[0] = e[1];
    	    e[1] = c;
    	    e++;
    	}
    	e++;
    }
    *e = '\0';
    while (s < --e) {
    	c   = *e;
    	*e  = *s;
    	*s++ = c;
    }
    return str;
}



/** 文字列dstにaddStrを連結.
 */
unsigned char*	_mbscat   (unsigned char* dst, const unsigned char* addStr)
{
    if (dst) {
    	unsigned char *p = _mbsend(dst);
    	_mbscpy(p, addStr);
    }
    return dst;
}



/** 文字列srcの先頭から文字keyを探して最初見つかったアドレスを返す.無ければNULL.
 */
unsigned char*	_mbschr(const unsigned char* src, unsigned key)
{
    const unsigned char *s;
    int   c, k;

    k = key;
    s = (const unsigned char *)src;
    if (key < 0x100) {
    	while ((c = *s) != '\0') {
    	    if (_ismbblead(c)) {
    	    	s++;
    	    	if (*s == '\0')
    	    	    break;
    	    } else if (c == k) {
    	    	return (unsigned char*)s;
    	    }
    	    s++;
    	}
    } else {
    	while ((c = *s) != '\0') {
    	    if (_ismbblead(c)) {
    	    	if (s[1] == '\0')
    	    	    break;
    	    	c = (c << 8) | s[1];
    	    	if (c == k)
    	    	    return (unsigned char*)s;
    	    	s++;
    	    }
    	    s++;
    	}

    }
    return NULL;
}



/** 文字列srcの末から文字keyを探して最初見つかったアドレスを返す.無ければNULL.
 */
unsigned char*	_mbsrchr  (const unsigned char* src, unsigned key)
{
    const unsigned char *s;
    const unsigned char *t;
    int   c, k;

    k = key;
    t = NULL;
    s = (const unsigned char *)src;
    if (key < 0x100) {
    	while ((c = *s) != '\0') {
    	    if (_ismbblead(c)) {
    	    	if (s[1] == '\0')
    	    	    break;
    	    	s++;
    	    } else if (c == k) {
    	    	t = s;
    	    }
    	    s++;
    	}
    } else {
    	while ((c = *s) != '\0') {
    	    if (_ismbblead(c)) {
    	    	if (s[1] == '\0')
    	    	    break;
    	    	c = (c << 8) | s[1];
    	    	if (c == k)
    	    	    t = s;
    	    	s++;
    	    }
    	    s++;
    	}

    }
    return (unsigned char*)t;
}



/** srcの先頭から文字列ptnがあるか探し見つかればそのアドレスを返す. なければNULL.
 *  @param src	検索される文字列
 *  @param ptn	探す文字列
 */
unsigned char *_mbsstr(const unsigned char *src, const unsigned char *ptn)
{
    const unsigned char *s, *p, *e;
    unsigned k,c;

    s = (const unsigned char *)src;
    p = (const unsigned char *)ptn;
    k = *p++;
    if (k == 0)
    	return (unsigned char *)s;
    e = p;
    while (*e)
    	e++;
    if (_ismbblead(k)) {
    	if (*p == 0)
    	    return (unsigned char *)s;
    	k = (k << 8) | *p++;
    	while ((c = *s) != 0) {
    	    if (_ismbblead(c) && s[1]) {
    	    	c = (c << 8) | s[1];
    	    	s += 2;
    	    	if (c == k) {
    	    	    const unsigned char *l, *r;
    	    	    l = (const unsigned char *)s;
    	    	    r = (const unsigned char *)p;
    	    	    c = 0;
    	    	    while (r != e) {
    	    	    	c = *l++;
    	    	    	if (c == 0)
    	    	    	    return NULL;
    	    	    	c = c - *r++;
    	    	    	if (c != 0)
    	    	    	    break;
    	    	    }
    	    	    if (c == 0)
    	    	    	return (unsigned char *)s-2;
    	    	}
    	    } else {
    	    	s++;
    	    }
    	}
    } else {
    	while ((c = *s) != 0) {
    	    if (_ismbblead(c) && s[1]) {
    	    	s += 2;
    	    } else {
    	    	++s;
    	    	if (k == c) {
    	    	    const unsigned char *l, *r;
    	    	    l = (const unsigned char *)s;
    	    	    r = (const unsigned char *)p;
    	    	    c = 0;
    	    	    while (r != e) {
    	    	    	c = *l++;
    	    	    	if (c == 0)
    	    	    	    return NULL;
    	    	    	c = c - *r++;
    	    	    	if (c != 0)
    	    	    	    break;
    	    	    }
    	    	    if (c == 0)
    	    	    	return (unsigned char *)(s-1);
    	    	}
    	    }
    	}
    }
    return NULL;
}



/** srcの末から文字列ptnがあるか探し見つかればそのアドレスを返す. なければNULL.
 *  @param src	検索される文字列
 *  @param ptn	探す文字列
 */
unsigned char *_mbsrstr(const unsigned char *src, const unsigned char *ptn)
{
    const unsigned char *s, *p, *e;
    unsigned k,c;

    s = (const unsigned char*)src;
    p = (const unsigned char*)ptn;
    k = *p++;
    if (k == 0)
    	return (unsigned char *)s;
    e = p;
    while (*e)
    	e++;
    if (_ismbblead(k)) {
    	if (*p == 0)
    	    return (unsigned char *)s;
    	k = (k << 8) | *p++;
    	while ((c = *s) != 0) {
    	    if (_ismbblead(c) && s[1]) {
    	    	c = (c << 8) | s[1];
    	    	s += 2;
    	    	if (c == k) {
    	    	    const unsigned char *l, *r;
    	    	    l = (const unsigned char *)s;
    	    	    r = (const unsigned char *)p;
    	    	    c = 0;
    	    	    while (r != e) {
    	    	    	c = *l++;
    	    	    	if (c == 0)
    	    	    	    return NULL;
    	    	    	c = c - *r++;
    	    	    	if (c != 0)
    	    	    	    break;
    	    	    }
    	    	    if (c == 0)
    	    	    	return (unsigned char *)s-2;
    	    	}
    	    } else {
    	    	s++;
    	    }
    	}
    } else {
    	while ((c = *s) != 0) {
    	    if (_ismbblead(c) && s[1]) {
    	    	s += 2;
    	    } else {
    	    	++s;
    	    	if (k == c) {
    	    	    const unsigned char *l, *r;
    	    	    l = (const unsigned char *)s;
    	    	    r = (const unsigned char *)p;
    	    	    c = 0;
    	    	    while (r != e) {
    	    	    	c = *l++;
    	    	    	if (c == 0)
    	    	    	    return NULL;
    	    	    	c = c - *r++;
    	    	    	if (c != 0)
    	    	    	    break;
    	    	    }
    	    	    if (c == 0)
    	    	    	return (unsigned char *)(s-1);
    	    	}
    	    }
    	}
    }
    return NULL;
}



/** tbl中にない文字が、文字列srcで最初に現れた位置のバイトオフセットを返す.
 */
size_t	_mbsspn(const unsigned char*src, const unsigned char* tbl)
{
    const unsigned char *s, *t;
    int c,k;
    size_t n;

    s = (const unsigned char*)src;
    n = 0;
    while ((c = *s) != '\0') {
    	if (_ismbblead(c)) {
    	    if (s[1] == 0)
    	    	break;
    	    c = (c << 8) | s[1];
    	    t = (const unsigned char *)tbl;
    	    for (;;) {
    	    	k = *t++;
    	    	if (k == 0)
    	    	    goto RET;
    	    	if (_ismbblead(k)) {
    	    	    if (*t == 0)
    	    	    	goto RET;
    	    	    k = (k<<8) | *t++;
    	    	    if (c == k)
    	    	    	break;
    	    	}
    	    }
    	    s+=2;
    	    n++;
    	} else {
    	    t = (const unsigned char *)tbl;
    	    while (c != *t) {
    	    	if (_ismbblead(*t))
    	    	    t++;
    	    	if (*t == 0)
    	    	    goto RET;
    	    	t++;
    	    }
    	    s++;
    	    n++;
    	}
    }
  RET:
   #if 0 /* 文字数 なら */
    return n;
   #else    /* mbstring系ならバイト数 */
    return (size_t)((char*)s - (char*)src);
   #endif
}



/** tblのいづれかの文字が、文字列srcで最初に現れた位置のバイトオフセットを返す.
 */
size_t _mbscspn  (const unsigned char* src, const unsigned char* tbl)
{
    const unsigned char *s, *t;
    int  c,k;
    size_t n;

    s = (const unsigned char*)src;
    n = 0;
    while ((c = *s) != '\0') {
    	if (_ismbblead(c)) {
    	    if (s[1] == 0)
    	    	break;
    	    c = (c << 8) | s[1];
    	    t = (const unsigned char *)tbl;
    	    for (;;) {
    	    	k = *t++;
    	    	if (k == 0)
    	    	    break;
    	    	if (_ismbblead(k)) {
    	    	    if (*t == 0)
    	    	    	break;
    	    	    k = (k<<8) | *t++;
    	    	    if (c == k) {
    	    	    	goto RET;
    	    	    }
    	    	}
    	    }
    	    s += 2;
    	    n++;
    	} else {
    	    t = (const unsigned char *)tbl;
    	    while (*t != '\0') {
    	    	if ((_ismbblead(*t) != 0) && (t[1] != '\0')) {
    	    	    t+=2;
    	    	    continue;
    	    	}
    	    	if (c == *t) {
    	    	    goto RET;
    	    	}
    	    	t++;
    	    }
    	    s++;
    	    n++;
    	}
    }
  RET:
   #if 0 /* 文字数の場合 */
    return n;
   #else    /* mbstring系ならバイト数 */
    return (size_t)((char*)s - (char*)src);
   #endif
}



/** tblのいづれかの文字が、文字列srcで最初に現れた位置のアドレスを返す. なければNULL.
 */
unsigned char*	_mbspbrk  (const unsigned char*src, const unsigned char *tbl)
{
    const unsigned char *s, *t;
    int c,k;
    //size_t n;

    //n = 0;
    s = (const unsigned char*)src;
    while ((c = *s) != '\0') {
    	if (_ismbblead(c)) {
    	    if (s[1] == 0)
    	    	break;
    	    c = (c << 8) | s[1];
    	    t = (const unsigned char *)tbl;
    	    for (;;) {
    	    	k = *t++;
    	    	if (k == 0)
    	    	    break;
    	    	if (_ismbblead(k)) {
    	    	    if (*t == 0)
    	    	    	break;
    	    	    k = (k<<8) | *t++;
    	    	    if (c == k) {
    	    	    	return (unsigned char*)s;
    	    	    }
    	    	}
    	    }
    	    s += 2;
    	    //n++;
    	} else {
    	    t = (const unsigned char *)tbl;
    	    while (*t != '\0') {
    	    	if ((_ismbblead(*t) != 0) & (t[1] != '\0')) {
    	    	    t+=2;
    	    	    continue;
    	    	}
    	    	if (c == *t) {
    	    	    return (unsigned char*)s;
    	    	}
    	    	t++;
    	    }
    	    s++;
    	    //n++;
    	}
    }
    return NULL;
}




/** 文字列比較.
 *  @return leftが小さければ 負, 同じなら 0, 大きければ 正, の値を返す.
 */
int 	_mbscmp   (const unsigned char* left, const unsigned char* right)
{
    const unsigned char *lp = left;
    const unsigned char *rp = right;
    unsigned l,r;

    do{
    	l = *lp++;
    	if ((_ismbblead(l)!=0) && (*lp != 0))
    	    l = (l << 8) | *lp++;
    	r = *rp++;
    	if ((_ismbblead(r)!=0) && (*rp != 0))
    	    r = (r << 8) | *rp++;
    	l -= r;
    } while ((l == 0) & (r != 0));
    return l;
}



/** 半角アルファベットの大小は同一視して文字列比較.
 *  @return leftが小さければ 負, 同じなら 0, 大きければ 正, の値を返す.
 */
int 	    	_mbsicmp  (const unsigned char* left, const unsigned char* right)
{
    const unsigned char *lp = left;
    const unsigned char *rp = right;
    unsigned l,r;

    do{
    	l = *lp++;
    	if (_ismbblead(l) && *lp) {
    	    l = (l << 8) | *lp++;
    	    //l = jtoupper(l);	// たぶん、いらね
    	} else {
    	    l = toupper(l);
    	}
    	r = *rp++;
    	if (_ismbblead(r) && *rp) {
    	    r = (r << 8) | *rp++;
    	    //r = jtoupper(r);	// たぶんいらね
    	} else {
    	    r = toupper(r);
    	}
    	l -= r;
    } while ((l == 0) & (r != 0));
    return l;
}




/** 文字列中の半角英字を小文字化する.
 */
unsigned char*	_mbslwr   (unsigned char* src)
{
    unsigned char *s;
    int c;

    s = (unsigned char *)src;
    while ((c = *s) != 0) {
    	if (_ismbblead(c)) {
    	    if (s[1] == 0) {
    	    	*s = 0;
    	    	break;
    	    }
    	    s += 2;
    	} else {
    	    if (('Z' >= c) & (c >= 'A')) {  	    /* (isupper(c)) */
    	    	s[0] = (unsigned char)(c + 0x20);   /* tolower(c)   */
    	    }
    	    s++;
    	}
    }
    return src;
}



/** 文字列中の半角英字を大文字化する.
 */
unsigned char*	_mbsupr   (unsigned char* src)
{
    unsigned char *s;
    int c;

    s = (unsigned char *)src;
    while ((c = *s) != 0) {
    	if (_ismbblead(c)) {
    	    if (s[1] == 0) {
    	    	*s = 0;
    	    	break;
    	    }
    	    s+=2;
    	} else {
    	    if (('a' <= c) & (c <= 'z')) {  	/* (islower(c)) */
    	    	s[0] = c - 0x20;    	    	/* toupper(c)	*/
    	    }
    	    s++;
    	}
    }
    return src;
}

//}
#endif
