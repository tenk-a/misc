/**
 *	@file	App.h
 *	@brief	アプリケーション
 */
#ifndef APP_H
#define APP_H


#pragma once

#include "stdafx.h"



/// コマンドライン・オプションの情報.
class CAppOpts {
public:
	CAppOpts() : debugMode_(0) {}

public:
	int 		debugMode_;
	CString 	matchName_;
};



/// スクリプトコンバータのメイン処理.
class CApp {
public:
	CApp();
	~CApp();

	bool run(vector<CString> const& dirNames, CAppOpts const& opts);	///< スクリプト・コンパイル実行.

private:
	bool	 one(CString const& rDir, CAppOpts const& rOpts);
	uint64_t getset_file_date(CString const& dir, CString const& matchName, uint64_t dirdate);

private:
	vector<CString>		dirs_;
	CAppOpts			opts_;				///< コマンドラインオプション.
};


#endif
