/**
 *	@file	disk.h
 *	@brief	ディスクのファイル関係
 *	@note
 *		フリーソフトよりパク. os-apiを使うので、いまのところwin専用.
 */


#include "stdafx.h"			// こちらで、TCHAR,CString関係は設定済みであること!
#include "disk.h"
#include "CString_Misc.h"
#include <io.h>


bool	disk_fileExist(const TCHAR* fname)
{
	return ( 0xFFFFFFFF != GetFileAttributes( fname ) );
}



/// 指定したファイル名のファイルサイズを返す.
size_t	disk_getFileSize(const TCHAR* fname)
{
	if (fname == NULL || fname[0] == 0)
		return 0;
	size_t	filesize  = 0;
	FILE *	fp = _tfopen(fname, _T("rb"));
	if (fp) {
		filesize = _filelength(_fileno(fp));
		fclose(fp);
	}
	return filesize;
}



/// ファイルをロード.
void*	disk_fileLoad(const TCHAR* fname, void* mem, size_t size)
{
	assert(mem != NULL && size > 0);
	FILE *	fp = _tfopen(fname, _T("rb"));
	if (fp) {
		fread(mem, 1, size, fp);
		if (ferror(fp))
			mem = NULL;
		fclose(fp);
	} else {
		mem = NULL;
	}
	return mem;
}



/// ファイルをロード.
bool disk_fileSave(const TCHAR* fname, const void* mem, size_t size)
{
	assert(mem != NULL);
	assert(size > 0);
	if (mem == 0 || size == 0)
		return false;

	FILE *	fp = _tfopen(fname, _T("wb"));
	if (fp == NULL)
		return false;
	size_t	n = fwrite(mem, 1, size, fp);
	if (ferror(fp))
		n = size - 1;
	fclose(fp);

	return n == size;
}




/// モジュールのパス取得用の初期化. 起動時に hInstanceを渡して初期化すること.
const TCHAR* disk_initExePath( HINSTANCE hInstance )
{
	static TCHAR 		tchPath[ _MAX_PATH + 2 ];
	if (hInstance) {
		//static HINSTANCE	s_hInstance = 0;
		//s_hInstance = hInstance;
		tchPath[0]  = 0;
		::GetModuleFileName( hInstance, tchPath, _MAX_PATH );
	}
	return tchPath;
}



/// モジュールのパス取得. 起動時に disk_initExePathにhInstanceを渡して初期化すること.
const TCHAR* disk_getExePath()
{
	return disk_initExePath( 0 );
}



//   exeのあるディレクトリ. 末の'\\'は無し.
const CString disk_getExeDirectory()
{
	return fname_getDirName( disk_getExePath() );
}



//   カレントディレクトリの取得.
const CString disk_getCurDirectory()
{
	TCHAR dir[MAX_PATH+2];
	dir[0] = 0;
	::GetCurrentDirectory(MAX_PATH, dir);
	return CString(dir);
}


/// カレントディレクトリを設定.
int disk_setCurDirectory(LPCTSTR pDirName)
{
	return ::SetCurrentDirectory(pDirName);
}


/// フルパスを返す.
const CString disk_fullPath(const CString& strFname)
{
	return fname_fullPath( strFname, disk_getCurDirectory() );
}



/// テキストファイルを文字列として取得.
const CString disk_loadTextString(LPCTSTR fname)
{
  #if 1	//
	CString 	buf;
	size_t		size = disk_getFileSize(fname);
	if (size > 0) {
		if (disk_fileLoad(fname, buf.GetBuffer(size), size) == 0)
			buf.Empty();
	}
	return buf;
  #else
	FILE *	fp = _tfopen(fname, _T("rb"));
	if (fp == NULL)
		return CString(_T(""));
	size_t 	filesize = _filelength(_fileno(fp));
	if (filesize == 0)
		return CString(_T(""));
	if (filesize > 16 * 1024 ) {		//+++ とりあえず、適当に 16Kバイトで打ち止めにしとく
		assert( filesize > 16*1024 );
		return CString(_T(""));
	}
	//手抜きで TCHAR でなく charのまま処理
	char*	buf = new char[ filesize + 4 ];
	memset(buf, 0, filesize+4);
	size_t r = fread(buf, 1, filesize, fp);
	fclose(fp);
	ATLASSERT(r == filesize);
	if (r > filesize)
		r = filesize;
	buf[r] = '\0';
	CString str( buf );
	delete[] buf;
	return str;
  #endif
}




void 	disk_fileTimeToSystemTime(__int64 ftim, SYSTEMTIME& st)
{
	FILETIME		localTim;
	::FileTimeToLocalFileTime((const FILETIME*)&ftim, (FILETIME*)&localTim);
	::FileTimeToSystemTime(&localTim, &st);
}


// ファイルの時間を、ローカル時間に変換して設定.
__int64 disk_systemTimeToFileTime(SYSTEMTIME& st)
{
	__int64  localTim 	= 0;
	__int64	 tim 		= 0;
	return	(    SystemTimeToFileTime((const SYSTEMTIME*)&st, (FILETIME*)&localTim)
		      && LocalFileTimeToFileTime((const FILETIME*)&localTim, (FILETIME*)&tim) )
		    ? tim
		    :   0 ;
}



__int64 disk_getFileWriteTime(const TCHAR* fname)
{
	WIN32_FIND_DATA		findData;
	HANDLE handle = ::FindFirstFile(fname, &findData);
	if (handle != INVALID_HANDLE_VALUE) {
		::FindClose(handle);
		return *(__int64*)&findData.ftLastWriteTime;
	}
	return 0;
}



bool disk_setFileWriteTime(const TCHAR* fname, __int64 ftim)
{
	if (fname == 0)
		fname = 0;		// NULLだとハングする環境もあるがopenをしないとerrnoが設定されないので代用.

	HANDLE handle = ::CreateFile((PCTSTR)fname, GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	if (handle == INVALID_HANDLE_VALUE)
		return false;
	bool rc = ::SetFileTime(handle, (FILETIME*)0, (FILETIME*)0, (FILETIME*)&ftim) != 0;
	::CloseHandle(handle);
	return rc;
}
