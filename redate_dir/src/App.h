/**
 *	@file	App.h
 *	@brief	�A�v���P�[�V����
 */
#ifndef APP_H
#define APP_H


#pragma once

#include "stdafx.h"



/// �R�}���h���C���E�I�v�V�����̏��.
class CAppOpts {
public:
	CAppOpts() : debugMode_(0) {}

public:
	int 		debugMode_;
	CString 	matchName_;
};



/// �X�N���v�g�R���o�[�^�̃��C������.
class CApp {
public:
	CApp();
	~CApp();

	bool run(vector<CString> const& dirNames, CAppOpts const& opts);	///< �X�N���v�g�E�R���p�C�����s.

private:
	bool	 one(CString const& rDir, CAppOpts const& rOpts);
	uint64_t getset_file_date(CString const& dir, CString const& matchName, uint64_t dirdate);

private:
	vector<CString>		dirs_;
	CAppOpts			opts_;				///< �R�}���h���C���I�v�V����.
};


#endif
