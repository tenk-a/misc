/**
 *  @file   App.cpp
 *  @brief  メイン処理
 */

#include "stdafx.h"
#include "App.h"
#include <iostream>
#include <fstream>
#include <direct.h>

#include "misc/disk.h"



CApp::CApp()
{
}



CApp::~CApp()
{
}



bool CApp::run(const vector<CString>& rDirs, const CAppOpts& opts)
{
    opts_   	= opts;
    dirs_   	= rDirs;

    for (unsigned i = 0; i < dirs_.size(); ++i) {
    	one(dirs_[i], opts);
    }
    return 1;
}



bool CApp::one(const CString& rDir, const CAppOpts& rOpts)
{
    CString dir = rDir;
    fname_delLastDirSep(dir);

    unsigned atr = ::GetFileAttributes( dir );
    if (atr & FILE_ATTRIBUTE_DIRECTORY) {
    	uint64_t dd = disk_getFileWriteTime(dir);
    	getset_file_date(dir, rOpts.matchName_, dd);
    	return true;
    } else {
    	if (atr == 0xFFFFFFFF) {
    	    std::cerr << dir << " をオープンできない\n";
    	} else {
    	    std::cerr << dir << " がディレクトリでない\n";
    	}
    	return false;
    }
}


uint64_t CApp::getset_file_date(const CString& dir, const CString& matchName, uint64_t dirdate)
{
    CString mname = matchName;
    if (mname.IsEmpty()) {
    	mname = _T("*.*");
    }

    CString name = dir + "\\" + mname;

    WIN32_FIND_DATA 	findData    = { 0 };
    HANDLE  	    	hdl 	    = ::FindFirstFile(name, &findData);
    uint64_t	    	da  	    = 0;
    if (hdl != INVALID_HANDLE_VALUE) {
    	do {
    	    CString 	fnd = findData.cFileName;
    	    uint64_t	d   = *(__int64*)&findData.ftLastWriteTime;
    	    if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
    	    	if (fnd == "." || fnd == "..") {
    	    	    ;
    	    	} else {
    	    	    CString subdir = dir + "\\" + fnd;
    	    	    d	    	   = getset_file_date(subdir, matchName, d);
    	    	    if (da < d)
    	    	    	da = d;
    	    	}
    	    } else {
    	    	if (da < d)
    	    	    da = d;
    	    }
    	} while (FindNextFile(hdl, &findData) != 0);
    	::FindClose(hdl);
    }
    if (da > 0 && da != dirdate) {
    	disk_setFileWriteTime( dir, da );
    }
    return da;
}
