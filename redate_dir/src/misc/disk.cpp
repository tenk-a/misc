/**
 *	@file	disk.h
 *	@brief	�f�B�X�N�̃t�@�C���֌W
 *	@note
 *		�t���[�\�t�g���p�N. os-api���g���̂ŁA���܂̂Ƃ���win��p.
 */


#include "stdafx.h"			// ������ŁATCHAR,CString�֌W�͐ݒ�ς݂ł��邱��!
#include "disk.h"
#include "CString_Misc.h"
#include <io.h>


bool	disk_fileExist(const TCHAR* fname)
{
	return ( 0xFFFFFFFF != GetFileAttributes( fname ) );
}



/// �w�肵���t�@�C�����̃t�@�C���T�C�Y��Ԃ�.
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



/// �t�@�C�������[�h.
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



/// �t�@�C�������[�h.
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




/// ���W���[���̃p�X�擾�p�̏�����. �N������ hInstance��n���ď��������邱��.
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



/// ���W���[���̃p�X�擾. �N������ disk_initExePath��hInstance��n���ď��������邱��.
const TCHAR* disk_getExePath()
{
	return disk_initExePath( 0 );
}



//   exe�̂���f�B���N�g��. ����'\\'�͖���.
const CString disk_getExeDirectory()
{
	return fname_getDirName( disk_getExePath() );
}



//   �J�����g�f�B���N�g���̎擾.
const CString disk_getCurDirectory()
{
	TCHAR dir[MAX_PATH+2];
	dir[0] = 0;
	::GetCurrentDirectory(MAX_PATH, dir);
	return CString(dir);
}


/// �J�����g�f�B���N�g����ݒ�.
int disk_setCurDirectory(LPCTSTR pDirName)
{
	return ::SetCurrentDirectory(pDirName);
}


/// �t���p�X��Ԃ�.
const CString disk_fullPath(const CString& strFname)
{
	return fname_fullPath( strFname, disk_getCurDirectory() );
}



/// �e�L�X�g�t�@�C���𕶎���Ƃ��Ď擾.
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
	if (filesize > 16 * 1024 ) {		//+++ �Ƃ肠�����A�K���� 16K�o�C�g�őł��~�߂ɂ��Ƃ�
		assert( filesize > 16*1024 );
		return CString(_T(""));
	}
	//�蔲���� TCHAR �łȂ� char�̂܂܏���
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


// �t�@�C���̎��Ԃ��A���[�J�����Ԃɕϊ����Đݒ�.
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
		fname = 0;		// NULL���ƃn���O����������邪open�����Ȃ���errno���ݒ肳��Ȃ��̂ő�p.

	HANDLE handle = ::CreateFile((PCTSTR)fname, GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	if (handle == INVALID_HANDLE_VALUE)
		return false;
	bool rc = ::SetFileTime(handle, (FILETIME*)0, (FILETIME*)0, (FILETIME*)&ftim) != 0;
	::CloseHandle(handle);
	return rc;
}
