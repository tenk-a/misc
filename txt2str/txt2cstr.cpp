/**
 *	@file	txt2cstr.cpp
 *	@brief	�e�L�X�g�t�@�C����c/c++�̕�����ɕϊ�����R�}���h���C���c�[��
 *
 *	@author	�k����j<NBB00541@nifty.com>
 *	@date	2003-07-26
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <set>

#include "cmisc.h"

using namespace std;
using namespace CMISC;

//#define CERR	cerr
#define CERR	cout


/// �����\�����I��
void usage(void)
{
	CERR<< "txt2cstr [-opts] file(s)\n"
		   " SJIS�e�L�X�g�t�@�C����c/c++�\�[�X�p��\"������\"�ɕϊ�����\n"
		   "  -o[FILE]  �o�̓t�@�C����\n"
		   "  -s        ���̓t�@�C�����Ȃ���ΕW�����͂ɁA�o�͎w�肪������ΕW���o�͂ɂ���\n"
		   "  -c        �s���̍Ō�� , ��u��\n"
		   "  -fSTR     �o�͕������printf�`���Ŏw��B%s�����͍s�ɒu�������B\n"
		   "            �Ⴆ�� -fputs(%s); �̂悤�Ɏw��\n"
	;
	exit(1);
}


// ---------------------------------------------------------------------------

/// �I�v�V�����v�f�̊Ǘ�
class Opts {
  public:
	string	outName_;		///< �o�͖�(1�񂫂�)
	string	appName_;		///< �A�v���P�[�V������
	string	fmt_;			///< �o�̓t�H�[�}�b�g�̎w��
	int		stdio_;			///< 1:"-s"�ŕW�����o�͉\�ɂ���. 0:�W�����o�͂��Ȃ�

	Opts() : outName_(""), appName_(""), fmt_("%s"), stdio_(0) {}
	~Opts() {}
	/// ���̃v���O��������
	void setAppName(const char *appNm) {appName_ = string(appNm);}
    void get(const char *p);
};


/// -�Ŏn�܂�R�}���h���C���I�v�V�����̉��
void Opts::get(const char *arg)
{
	const char *p = arg + 1;
	int c = *p++;
	c = toupper(c);
	switch (c) {
	case 'O':
		outName_ = string(p);
		break;
	case 'S':
		stdio_   = (*p != '-');
		break;
	case 'C':
		fmt_ = "%s,";
		break;
	case 'F':
		fmt_ = p;
		break;
	case '?':
		::usage();
		break;
	default:
		CERR << arg << " �͒m��Ȃ��I�v�V�����ł�\n";
		exit(1);
	}
}



// ---------------------------------------------------------------------------

/// �e�L�X�g -> c�\�[�X �ϊ�
class Conv {
	string fmt_;
	static char *chTbl[256];
	int  convLine(vector<char> &st, const char *src);
  public:
	Conv() {
		fmt_ = "%s";
	}
	~Conv() {}
	/// �o�̓t�H�[�}�b�g��ݒ肷��
	bool setFmt(string &fmt);
	/// �ϊ������s����
	int  run(const char *name, const char *outName);
};


/// �ϊ��{��
///
int Conv::run(const char *name, const char *outName)
{
	FILE *ifp;
	if (name && name[0]) {
		ifp = fopen(name, "rt");
		if (ifp == NULL) {
			CERR << name << " ��open�ł��Ȃ�\n";
			return false;
		}
	} else {
		ifp = stdin;
	}

	FILE *ofp;
	if (outName && outName[0]) {
		ofp = fopen(outName, "wt");
		if (ofp == NULL) {
			CERR << outName << " ��open�ł��Ȃ�\n";
			return false;
		}
	} else {
		ofp = stdout;
	}

	vector<char> st(0x10000);	// �Ȃ�ƂȂ��Astring�łȂ�vector<char>�ł�����
	//int lineNum = 0;
	while (feof(ifp) == 0) {
		//lineNum++;
		char buf[0x10000];
		if (fgets(buf, sizeof buf, ifp) == NULL)
			break;

		st.clear();
		st.push_back('"');
		convLine(st, buf);
		st.push_back('"');
		st.push_back('\0');
		fprintf(ofp, "\t");
		fprintf(ofp, fmt_.c_str(), &st[0]);
		fprintf(ofp, "\n");
	}

	if (ofp != stdout)
		fclose(ofp);
	if (ifp != stdin)
		fclose(ifp);

	return true;
}


/// ����ȕ����R�[�h��ϊ����邽�߂̃e�[�u��
char *Conv::chTbl[256] = {				// a:0x07,b:0x08,t:0x09,n:0x0a,v:0x0b,f:0x0c,r:0x0d,
	"\\x00", "\\x01", "\\x02", "\\x03", "\\x04", "\\x05", "\\x06", "\\a"  ,
	"\\b"  , "\\t"  , "\\n"  , "\\v"  , "\\f"  , "\\r"  , "\\x0e", "\\x0f",
	"\\x10", "\\x11", "\\x12", "\\x13", "\\x14", "\\x15", "\\x16", "\\x17",
	"\\x18", "\\x19", "\\x1a", "\\x1b", "\\x1c", "\\x1d", "\\x1e", "\\x1f",
	0/* */ , 0/*!*/ , "\\\"" , 0/*#*/ , 0/*$*/ , 0/*%*/ , 0/*&*/ , 0/*'*/ ,
	0/*(*/ , 0/*)*/ , 0/***/ , 0/*+*/ , 0/*,*/ , 0/*-*/ , 0/*.*/ , 0/* / */,
	0/*0*/ , 0/*1*/ , 0/*2*/ , 0/*3*/ , 0/*4*/ , 0/*5*/ , 0/*6*/ , 0/*7*/ ,
	0/*8*/ , 0/*9*/ , 0/*:*/ , 0/*;*/ , 0/*<*/ , 0/*=*/ , 0/*>*/ , 0/*?*/ ,
	0/*@*/ , 0/*A*/ , 0/*B*/ , 0/*C*/ , 0/*D*/ , 0/*E*/ , 0/*F*/ , 0/*G*/ ,
	0/*H*/ , 0/*I*/ , 0/*J*/ , 0/*K*/ , 0/*L*/ , 0/*M*/ , 0/*N*/ , 0/*O*/ ,
	0/*P*/ , 0/*Q*/ , 0/*R*/ , 0/*S*/ , 0/*T*/ , 0/*U*/ , 0/*V*/ , 0/*W*/ ,
	0/*X*/ , 0/*Y*/ , 0/*Z*/ , 0/*[*/ , "\\\\" , 0/*]*/ , 0/*^*/ , 0/*_*/ ,
	0/*`*/ , 0/*a*/ , 0/*b*/ , 0/*c*/ , 0/*d*/ , 0/*e*/ , 0/*f*/ , 0/*g*/ ,
	0/*h*/ , 0/*i*/ , 0/*j*/ , 0/*k*/ , 0/*l*/ , 0/*m*/ , 0/*n*/ , 0/*o*/ ,
	0/*p*/ , 0/*q*/ , 0/*r*/ , 0/*s*/ , 0/*t*/ , 0/*u*/ , 0/*v*/ , 0/*w*/ ,
	0/*x*/ , 0/*y*/ , 0/*z*/ , 0/*{*/ , 0/*|*/ , 0/*}*/ , 0/*~*/ , "\\x7f",
	// 0x80�`0xFF �͂Ƃ肠�����A�S��0
};


/// �P�s�ϊ�
int Conv::convLine(vector<char> &st, const char *src)
{
	const unsigned char *s = (const unsigned char *)src;
	while (*s) {
		int c = *s++;
		if (ISKANJI(c) && *s) {
			st.push_back(c);
			st.push_back(*s);
			s++;
		} else {
			const char *p = chTbl[c];
			if (p) {
				while (*p)
					st.push_back(*p++);
			} else {
				st.push_back(c);
			}
		}
	}
	return 1;
}


/// �o�̓t�H�[�}�b�g��ݒ肷��
bool Conv::setFmt(string &fmt)
{
	int n  = 0;
	int sf = 0;

	// �`�F�b�N
	for (;;) {
		n = fmt.find_first_of('%', n);
		if (size_t(n) == string::npos)
			break;
		++n;
		if (fmt[n] == '%') {
			++n;
		} else if (fmt[n] == 's') {
			++n;
			++sf;
			if (sf > 1) {
				CERR << "-f�w�蒆 %s ����������\n";
				return false;
			}
		} else {
			CERR << "%% %s �ȊO��%�w�肪����悤��\n";
			return false;
		}
	}
	fmt_ = fmt;
	return true;
}


// ---------------------------------------------------------------------------


class App {
	Opts	opts_;
	Conv	conv_;
  public:
	App() {}
	~App() {}
	int main(int argc, char *argv[]);
	int oneFile(const char *name);
};


int App::main(int argc, char *argv[])
{
	if (argc < 2)
		::usage();

	opts_.setAppName(argv[0]);
	int rc = 0, n = 0;
	for (int i = 1; i < argc; i++) {
		char *p = argv[i];
		if (*p == '-') {
			opts_.get(p);
		} else {
			rc = oneFile(p);
			n++;
		}
	}
	if (n == 0 && opts_.stdio_) {
		rc = oneFile("");
	}
	rc = !rc;		// main()�̕��A�l�� 0:���� 0�ȊO:�G���[ �Ȃ�ŁA���̂悤�ɕϊ�
	return rc;
}


int App::oneFile(const char *name)
{
	string inm(name);
	string onm(name);

	if (opts_.outName_.empty()) {	// �I�v�V����-o ���Ȃ��ꍇ
		if (opts_.stdio_)			// �W���o�͎w�肪����΁A�t�@�C�����𖳂���
			onm = "";
		else						// �ʏ�� .c �ɂ����t�@�C���ɏo��
			onm += ".c";
	} else {						// �I�v�V����-o���������ꍇ
		onm = opts_.outName_;
		opts_.outName_ = "";		// -o�͈�񂱂�����Ȃ�ŁA��������ɏ�����
	}
	if (conv_.setFmt(opts_.fmt_) == false)
		return 0;
	return conv_.run(inm.c_str(), onm.c_str());
}



// ---------------------------------------------------------------------------

/// �������n�܂�
int main(int argc, char *argv[])
{
	int rc;
	App app;
	try {
	  #ifdef _MSC_VER
		// VC(7)�ŃR���p�C��������Aargv[0]�Ƀt���p�X�����Ă��Ȃ��A�R�}���h���C��
		// �Ń^�C�v�����v���O�������������Ă��肷��̂ŁA�����Ώ�
		argv[0] = _pgmptr;
	  #endif
		rc = app.main(argc, argv);
	}
	catch (const exception &ex) {
		CERR << ex.what() << endl;
		rc = 1;
	}
	return rc;
}



