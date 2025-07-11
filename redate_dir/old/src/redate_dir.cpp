/**
 *  @file   redate_dir.cpp
 *  @brief  指定したフォルダの日付を、フォルダ内の最新ファイルの日付にする.
 *  @author tenk*
 */

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "App.h"
#include "misc/ExArgv.h"
#include <direct.h>


int g_db_mode;	    ///< デバッグ出力モード.



/// コマンドライン・ヘルプ
static bool usage()
{
    printf("Usage>redate_dir [opts] dir\n");
    printf(" 指定したフォルダの日付を、フォルダ内の最新ファイルの日付にする.\n"
    	  "  -m[NAME]  日付取得の対象となるファイル名を指定.(ワイルドカード指定有)\n"
    );
    return 1;
}


/// オプション管理.
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
    	    printf("しらないオプション%s\n", argv_[rNo]);
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



// 起動メイン.
int main(int argc, char* argv[])
{
    ExArgv_get(&argc, &argv);
    if (argc < 2) {
    	return usage();
    }

    COpts   	    	opts(argc, argv);
    vector<CString> 	fnames;

    // オプション.
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
    	printf("ファイルが指定されていない\n");
    	return 1;
    }

    std::auto_ptr<CApp>     app(new CApp());
    if (app->run(fnames, opts.appOpts_) == 0) {
    	return 1;   // error
    }

    return 0;	// ok
}
