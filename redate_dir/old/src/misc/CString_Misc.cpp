/**
 *  @file   CString_Misc.cpp
 *  @brief  CString関係の雑多なルーチン群.
 *  @note
 *  	いろいろなフリーソフトからパクってきたものが主.
 */


#include "stdafx.h"
#include <cstdarg>
#include <cstring>
#ifdef _MSC_VER
#include <tchar.h>
#endif
#include "CString.h"



// ===========================================================================
// 文字列一般.

CString     str_fmt(LPCSTR lpszFormat, ...)
{
    assert( lpszFormat != 0 );
    CString str;
    va_list argList;
    va_start(argList, lpszFormat);
    BOOL    bRet = str.FormatV(lpszFormat, argList);
    va_end(argList);
    return str;
}



CString     str_trim(const CString& src, const TCHAR* separaters)
{
    const TCHAR*    s = LPCTSTR(src);
    size_t b	      = _tcsspn(s, separaters);
    size_t e	      = _tcscspn(s+b, separaters);
    if (b == 0 && e == src.GetLength())
    	return src;
    assert( b + e <= src.GetLength() );
    return CString(s + b, e);
}


#if 0
CString&    	str_replace(CString& str, const char* src, const char* dst)
{
    assert(src != 0 && strlen(src) > 0);
    assert(dst != 0);
    int n = str.Find(str, src);
    if (n >= 0) {
    	str.Delete(n, strlen(src));
    	if (strlen(dst))
    	    str.Insert(n, dst);
    }
    return str;
}
#endif



// ===========================================================================
// ファイル名関係.

/// ファイルパス名より、ファイル名を取得
const CString	fname_getBaseName(const CString& strFileName)
{
    int     n	= strFileName.ReverseFind( _T('\\') );
    if (n < 0) {
    	n   = strFileName.ReverseFind( _T('/') );
    	if (n < 0)
    	    return strFileName;
    }
    return  strFileName.Mid(n + 1);
}


/// ファイルパス名より、ディレクトリ名を取得. 最後の'\\'は含まない.
const CString	fname_getDirName(const CString& strFileName)
{
    int     n	= strFileName.ReverseFind( _T('\\') );
    if (n < 0) {
    	n   = strFileName.ReverseFind( _T('/') );
    	if (n < 0)
    	    return _T("."); //strFileName;
    }
    return  strFileName.Left(n);
}


/// ファイル名の拡張子の取得. ※ 結果の文字列には'.'は含まれない.
const CString	fname_getFileExt(const CString& strFileName)
{
    const CString strBaseName = fname_getBaseName(strFileName);
    int     n	= strBaseName.ReverseFind( _T('.') );
    if (n < 0)
    	return CString();
    return strBaseName.Mid(n + 1);
}


/// フォルダ＆拡張子無しのファイル名の取得. ※ 結果の文字列には'.'は含まれない.
const CString	fname_getBaseNameNoExt(const CString& strFileName)
{
    const CString strBaseName = fname_getBaseName(strFileName);
    int     n	= strBaseName.ReverseFind( _T('.') );
    if (n < 0)
    	return strBaseName;
    return strBaseName.Left(n);
}



/// 拡張子無しのファイル名の取得. ※ 結果の文字列には'.'は含まれない.
const CString	fname_getFileNameNoExt(const CString& strFileName)
{
    const CString strBaseName = fname_getBaseName(strFileName);
    int     n	= strBaseName.ReverseFind( _T('.') );
    if (n < 0)
    	return strFileName;

    int     nd	= strFileName.ReverseFind( _T('\\') );
    if (nd < 0)
    	nd  	= strFileName.ReverseFind( _T('/') );
    if (nd >= 0)
    	return strFileName.Left(nd + 1) + strBaseName.Left(n);
    return strBaseName.Left(n);
}



//+++ \ を / に置き換える.
void	fname_bkSl_to_sl(CString& str)
{
  #ifdef UNICODE
    str.Replace(_T('\\'), _T('/'));
  #else
    unsigned l = str.GetLength();
    char*    s = (char*) LPCSTR( str );
    for (unsigned i = 0; i < l; ++i) {
    	int c = (unsigned char)s[i];
    	if (c == 0)
    	    break;
    	if (_ismbblead(c) && i+1 < l)
    	    ++i;
    	else if (c == '\\')
    	    s[i] = _T('/');
    }
  #endif
}


/// '/'を '\\' に置換.
void	fname_sl_to_bkSl(CString& str)
{
  #ifdef UNICODE
    str.Replace(_T('/'), _T('\\'));
  #else
    str.Replace('/', '\\');
  #endif
}



void	fname_addDirSep(CString& strFileName, TCHAR sepChr)
{
  #ifdef UNICODE
    unsigned l = strFileName.GetLength();
    int      c;
    if ( l && ( (c = strFileName.GetAt(l-1)) == _T('/') || (c == _T('\\')) ) ) {
    	;
    } else {
    	strFileName += sepChr;
    }
  #else
    // とりあえず、全角未対応で...
    unsigned l = strFileName.GetLength();
    int      c;
    if ( l && ( (c = strFileName.GetAt(l-1)) == _T('/') || (c == _T('\\')) ) ) {
    	;
    } else {
    	strFileName += sepChr;
    }
  #endif
}


void	fname_delLastDirSep(CString& strFileName)
{
  #ifdef UNICODE
    unsigned l = strFileName.GetLength();
    int      c;
    if ( l && ( (c = strFileName.GetAt(l-1)) == _T('/') || (c == _T('\\')) ) ) {
    	strFileName.Delete(l-1);
    }
  #else
    // とりあえず、全角未対応で...
    unsigned l = strFileName.GetLength();
    int      c;
    if ( l && ( (c = strFileName.GetAt(l-1)) == _T('/') || (c == _T('\\')) ) ) {
    	strFileName.Delete(l-1);
    }
  #endif
}


/// 手抜きなフルパス化. 出力の区切りは'/'になる.
const CString fname_fullPath(const CString& strFileName, const CString& strCurrentDir )
{
    CString 	dir = strCurrentDir;
    fname_bkSl_to_sl(dir);
    //if (dir.Left(7) != _T("file://"))
    //	return strFileName;
    CString 	fnm = dir.Mid(5);
    int     	net = 1;
    if (fnm[2] == _T('/') && fnm[4] == _T(':')) {
    	fnm = fnm.Mid(3);
    	net = 0;
    }
    //DWORD 	atr = ::GetFileAttributes(LPCTSTR(fnm));
    // if (atr == (DWORD)-1) {
    //	if (net == 0)
    //	    return strFileName;
    //	// ネットワークなパスだったらば、ディレクトリ扱いしとく.
    //
    //} else
    // if (atr & FILE_ATTRIBUTE_DIRECTORY) {
    //	    ;
    //
    //} else
    {
    	// ディレクトリでないファイルならば、ファイル名ととってディレクトリを取得
    	int r = dir.ReverseFind(_T('/'));
    	if (r >= 0)
    	    dir = dir.Left(r);
    }

    CString 	src = strFileName;
    fname_bkSl_to_sl(src);
    const TCHAR* p  = LPCTSTR(src);
    if (p == 0 || *p == 0) {
    	return dir;
    }
    unsigned	c  = *p;
    unsigned	c1 = p[1];
    //unsigned	c2 = 0;
    if (c == _T('/') || (c1 == _T(':') && ( p[2] == _T('/'))) ) {
    	return strFileName; // フルパスだろうでそのまま返す.

    } else {
    	int l = dir.GetLength();
    	if (l > 0 && dir[l-1] != _T('/'))
    	    dir += _T('/');

    	l   	    = src.GetLength();
    	int   n     = 0;
    	while (n < l && src[n]) {
    	    if (src[n] == '.' && n+1 < l) {
    	    	if (src[n+1] == _T('/')) {
    	    	    n += 2;
    	    	} else if (n+1 < l && src[n+1] == '.'
    	    	    	&& (n+2 == l || (n+3 < l && src[n+2] == _T('/')))
    	    	) {
    	    	    n += 2 + (n+2 < l);
    	    	    int r = dir.GetLength();
    	    	    if (r > 0 && dir[r-1] == _T('/'))
    	    	    	dir = dir.Left(r-1);
    	    	    r = dir.ReverseFind(_T('/'));
    	    	    if (r >= 0)
    	    	    	dir = dir.Left(r+1);
    	    	} else {
    	    	    goto DIRECT;
    	    	}
    	    } else {
    	  DIRECT:
    	    	while (n < l && src[n] != 0) {
    	    	    TCHAR c = src[n++];
    	    	    dir += c;
    	    	    if (c == _T('/'))
    	    	    	break;
    	    	}
    	    }
    	}
    }
    return dir;
}




