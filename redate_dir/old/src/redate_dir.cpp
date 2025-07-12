/**
 *  @file   redate_dir.cpp
 *  @brief  �w�肵���t�H���_�̓��t���A�t�H���_���̍ŐV�t�@�C���̓��t�ɂ���.
 *  @author tenk*
 */

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "App.h"
#include "misc/ExArgv.h"
#include <direct.h>


int g_db_mode;	    ///< �f�o�b�O�o�̓��[�h.



/// �R�}���h���C���E�w���v
static bool usage()
{
    printf("Usage>redate_dir [opts] dir\n");
    printf(" �w�肵���t�H���_�̓��t���A�t�H���_���̍ŐV�t�@�C���̓��t�ɂ���.\n"
    	  "  -m[NAME]  ���t�擾�̑ΏۂƂȂ�t�@�C�������w��.(���C���h�J�[�h�w��L)\n"
    );
    return 1;
}


/// �I�v�V�����Ǘ�.
class COpts {
public:
    COpts(int argc, char** argv) : argc_(argc), argv_(argv) {}

    bool scanOpt(int& rNo) {
    	assert(rNo < argc_);
    	const char* p = argv_[rNo];
    	assert(*p == '-');
    	if (str_equLong(p, "-?" , &p) || str_equLong(p, "--help",&p)) {
    	    return usage();
    	} else if (str_equLong(p, "-debug" , &p) || str_equLong(p, "--debug" , &p)) {
    	    if (*p)
    	    	appOpts_.debugMode_ = strtoul(p,0,0);
    	    else
    	    	appOpts_.debugMode_ = 1;
    	} else if (str_equLong(p, "-m" , &p)) {
    	    appOpts_.matchName_ = getArgStr(p, rNo);
    	} else {
    	    printf("����Ȃ��I�v�V����%s\n", argv_[rNo]);
    	    return 1;
    	}
    	return 0;
    }

private:
    const char* getArgStr(const char* p, int& rNo) {
    	if (*p)
    	    return p;
    	if (rNo < argc_) {
    	    ++rNo;
    	    return argv_[rNo];
    	}
    	return "";
    }

public:
    CAppOpts	appOpts_;

private:
    int     	argc_;
    char**  	argv_;
};



// �N�����C��.
int main(int argc, char* argv[])
{
    ExArgv_get(&argc, &argv);
    if (argc < 2) {
    	return usage();
    }

    COpts   	    	opts(argc, argv);
    vector<CString> 	fnames;

    // �I�v�V����.
    for (int i = 1; i < argc; ++i) {
    	const char* p = argv[i];
    	if (*p == '-') {
    	    if (opts.scanOpt(i))
    	    	return 1;
    	} else {
    	    CString nm = p;
    	    fnames.push_back(nm);
    	}
    }

    if (fnames.empty()) {
    	printf("�t�@�C�����w�肳��Ă��Ȃ�\n");
    	return 1;
    }

    std::auto_ptr<CApp>     app(new CApp());
    if (app->run(fnames, opts.appOpts_) == 0) {
    	return 1;   // error
    }

    return 0;	// ok
}
