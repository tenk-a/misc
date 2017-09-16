/**
 *	@file	disk.h
 *	@brief	�f�B�X�N�̃t�@�C���֌W
 *	@note
 *		�t���[�\�t�g���p�N. os-api���g���̂ŁA���܂̂Ƃ���win��p.
 */

#ifndef DISK_H
#define DISK_H

#pragma once

#include "stdafx.h"			// ������ŁATCHAR,CString�֌W�͐ݒ�ς݂ł��邱��!

#ifdef _WIN32
#include <windows.h>
#endif


// �t�@�C���֌W. os-api���g������. �c�[���p.
#ifdef _WIN32
const TCHAR* 	disk_initExePath( HINSTANCE hInstance );
#endif
const TCHAR* 	disk_getExePath();
const CString 	disk_getExeDirectory();

const CString 	disk_getCurDirectory();
int 			disk_setCurDirectory(LPCTSTR pDirName);		// ���ꎩ�̂�CString�֌W�Ȃ���..�֘A���Ƃ���.
const CString 	disk_fullPath(const CString& strFname);
const CString 	disk_loadTextString(LPCTSTR fname);
size_t			disk_getFileSize(const TCHAR* fname);
bool			disk_fileExist(const TCHAR* fname);
void*			disk_fileLoad(const TCHAR* fname, void* mem, size_t size);
bool			disk_fileSave(const TCHAR* fname, const void* mem, size_t size);


bool 			disk_setFileWriteTime(const TCHAR* fname, __int64 ftim);
__int64 		disk_getFileWriteTime(const TCHAR* fname);
__int64 		disk_systemTimeToFileTime(SYSTEMTIME& st);
void 			disk_fileTimeToSystemTime(__int64 ftim, SYSTEMTIME& st);



/// std::vector<T>�݊��̃R���e�i�Ƀt�@�C�����e�����[�h.
template<class VEC>
bool	disk_fileLoad(const TCHAR* fname, VEC& rVec)
{
	rVec.clear();
	size_t file_bytes = disk_getFileSize(fname);
	if (file_bytes > 0) {
		size_t	n     = file_bytes / sizeof(rVec[0]);
		rVec.resize( n );
		size_t	bytes = rVec.size() * sizeof(rVec[0]);
		if (bytes > 0)
			return disk_fileLoad( fname, &rVec[0], bytes ) != 0;
	}
	return disk_fileExist(fname);
}


/// std::vector<T>�݊��̃R���e�i�Ƀt�@�C�����e�����[�h.
inline vector<unsigned char>	disk_fileLoad(const TCHAR* fname)
{
	vector<unsigned char>	buf;
	disk_fileLoad(fname, buf);
	return buf;
}


/// std::vector<T>�݊��̃R���e�i�Ƀt�@�C�����e�����[�h.
template<class VEC>
bool	disk_fileSave(const TCHAR* fname, const VEC& rVec)
{
	const void* adr   = &rVec[0];
	unsigned	bytes = rVec.size() * sizeof(rVec[0]);
	return disk_fileSave(fname, adr, bytes) != 0;
}


#endif
