/**
 *  @file   CString_Misc.cpp
 *  @brief  CString�֌W�̎G���ȃ��[�`���Q.
 *  @note
 *  	���낢��ȃt���[�\�t�g����p�N���Ă������̂���.
 */


#include "stdafx.h"
#include <cstdarg>
#include <cstring>
#ifdef _MSC_VER
#include <tchar.h>
#endif
#include "CString.h"



// ===========================================================================
// ��������.

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
// �t�@�C�����֌W.

/// �t�@�C���p�X�����A�t�@�C�������擾
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


/// �t�@�C���p�X�����A�f�B���N�g�������擾. �Ō��'\\'�͊܂܂Ȃ�.
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


/// �t�@�C�����̊g���q�̎擾. �� ���ʂ̕�����ɂ�'.'�͊܂܂�Ȃ�.
const CString	fname_getFileExt(const CString& strFileName)
{
    const CString strBaseName = fname_getBaseName(strFileName);
    int     n	= strBaseName.ReverseFind( _T('.') );
    if (n < 0)
    	return CString();
    return strBaseName.Mid(n + 1);
}


/// �t�H���_���g���q�����̃t�@�C�����̎擾. �� ���ʂ̕�����ɂ�'.'�͊܂܂�Ȃ�.
const CString	fname_getBaseNameNoExt(const CString& strFileName)
{
    const CString strBaseName = fname_getBaseName(strFileName);
    int     n	= strBaseName.ReverseFind( _T('.') );
    if (n < 0)
    	return strBaseName;
    return strBaseName.Left(n);
}



/// �g���q�����̃t�@�C�����̎擾. �� ���ʂ̕�����ɂ�'.'�͊܂܂�Ȃ�.
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



//+++ \ �� / �ɒu��������.
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


/// '/'�� '\\' �ɒu��.
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
    // �Ƃ肠�����A�S�p���Ή���...
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
    // �Ƃ肠�����A�S�p���Ή���...
    unsigned l = strFileName.GetLength();
    int      c;
    if ( l && ( (c = strFileName.GetAt(l-1)) == _T('/') || (c == _T('\\')) ) ) {
    	strFileName.Delete(l-1);
    }
  #endif
}


/// �蔲���ȃt���p�X��. �o�͂̋�؂��'/'�ɂȂ�.
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
    //	// �l�b�g���[�N�ȃp�X��������΁A�f�B���N�g���������Ƃ�.
    //
    //} else
    // if (atr & FILE_ATTRIBUTE_DIRECTORY) {
    //	    ;
    //
    //} else
    {
    	// �f�B���N�g���łȂ��t�@�C���Ȃ�΁A�t�@�C�����ƂƂ��ăf�B���N�g�����擾
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
    	return strFileName; // �t���p�X���낤�ł��̂܂ܕԂ�.

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




