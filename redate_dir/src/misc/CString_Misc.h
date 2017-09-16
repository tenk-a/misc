/**
 *	@file	CString_Misc.h
 *	@brief	CString関係の雑多なルーチン群.
 */

#ifndef CSTRING_MISC_H
#define CSTRING_MISC_H

#pragma once


#include "CString.h"


CString			str_fmt( const char* fmt, ... );		///< sprintf書式のCString文字列生成.
CString 		str_trim(const CString& src, const TCHAR* separaters = _T(" \t\n\r\f"));
//CString&		str_replace(CString& str, const char* src, const char* dst);

#ifdef _WIN32
inline bool		str_equLong(const CString& s, const CString& key) {
	return _tcsncmp(LPCTSTR(s), LPCTSTR(key), key.GetLength()) == 0;
}

inline bool		str_equLong(const TCHAR* s, const TCHAR* key, const TCHAR** pNextPtr=0) {
	size_t	l = _tcslen(key);
	bool	rc = _tcsncmp(s, key, l) == 0;
	if (rc && pNextPtr)
		*pNextPtr = s + l;
	return rc;
}
#endif


// ==========================================================================
// ファイル名 関係.
const CString	fname_getBaseName(const CString& strFileName);
const CString	fname_getDirName(const CString& strFileName);
const CString	fname_getFileExt(const CString& strFileName);
const CString	fname_getBaseNameNoExt(const CString& strFileName);
const CString	fname_getFileNameNoExt(const CString& strFileName);
void 			fname_bkSl_to_sl(CString& str);
void 			fname_sl_to_bkSl(CString& str);
const CString 	fname_fullPath(const CString& strFileName, const CString& strCurrentDir );

void			fname_addDirSep(CString& strFileName, TCHAR sepChr=_T('/'));
void			fname_delLastDirSep(CString& strFileName);


// ==========================================================================
// inline & template

// separatersは半角文字のみ有効.
template <class VEC_CSTRING>
bool			str_split(VEC_CSTRING& vecStr, const TCHAR* src, const TCHAR* separaters=_T(" \t\n\r\f")) {
	assert(src	!= 0);
	assert(separaters != 0);
	if (src && separaters) {
		vecStr.clear();
		const TCHAR*	s = LPCTSTR(src);
		unsigned		n;
		do {
			s = s + _tcsspn(s, separaters);
			if (*s == 0)
				break;
			n = _tcscspn(s, separaters);
			if (n > 0)
				vecStr.push_back(CString(s, n));
			s += n;
		} while (*s && n > 0);
		return true;
	}
	return false;
}



/** テキストファイルを改行で区切られた文字列に変換する.
 *  切り分けでは'\0'は考慮しない.
 * 入力に'\0'が混ざっていると文字列が反するので注意!
 */
template <class VEC_CSTRING>
bool	str_textToLines(VEC_CSTRING& vecStr, const TCHAR* src, unsigned size, TCHAR outEOS=_T('\0')) {
	assert(src	!= 0);
	if (src == 0 || size == 0)
		return 0;
	vecStr.clear();
	const TCHAR*	e = src + size;
	const TCHAR*	b = src;
	const TCHAR*	s = src;
	const TCHAR* 	t = s;
	while (s < e) {
		bool bEol = 0;
		t     	  = s;
		int  c 	  = *s++;
		if (c == '\r') {
			if (s < e && *s == '\n')
				++s;
			bEol = 1;
		} else if (c == '\n') {
			bEol = 1;
		}
		if (bEol) {
			size_t l = t - b;
			if (l > 0) {
				vecStr.push_back(CString(b, l));
			} else {
				vecStr.push_back(CString());
			}
			if (outEOS)
				vecStr.back() += outEOS;
			b = s;
		}
	}
	if (b < t) {
		size_t l = t - b;
		if (l > 0) {
			vecStr.push_back(CString(b, l));
		} else {
			vecStr.push_back(CString());
		}
		if (outEOS)
			vecStr.back() += outEOS;
	}
	return true;
}

#endif
