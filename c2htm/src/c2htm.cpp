/**
 *  @file   c2htm.cpp
 *  @brief  c/c++�\�[�X��F��html�ɕϊ�����R�}���h���C���c�[��
 *
 *  @author Masahi KITAMURA (tenka@6809.net)
 *  @date   2003-07-21 .. 2017
 */

#include <cstring>

#include <fstream>
#include <iostream>
#include <Iomanip>
#include <string>
#include <vector>
#include <set>
#include <stdlib.h>

#include "cmisc.h"
#include "ConvCpp2Htm.h"
#include "cfgfile.h"

#define CERR    cout
using namespace CMISC;
using namespace std;


/** �����\�����I��
 */
void usage(void)
{
    cout<< "c2htm [-opts] file(s)    // v1.73  " __DATE__  "  writen by M.Kitamura\n"
           "     https://github.com/tenk-a/misc/tree/master/c2htm\n"
           " c/c++ source text ��F��html�ɕϊ�(html4.0)\n"
           " ��`�t�@�C���Ƃ��� ���� .exe �p�X���� .cfg �ɂ������̂�Ǎ���\n"
           " �I�v�V����:\n"
           "  :NAME         exe�Ɠ����t�H���_�ɂ��� c2htm-NAME.cfg ��Ǎ���\n"
           "  -c[CFGFILE]   �f�t�H���g�ȊO�̒�`�t�@�C����Ǎ���\n"
           "  -o[FILE]      �o�̓t�@�C����\n"
           "  -n[N[:S]]     �s�ԍ���t��. N=����(�ȗ�:5).S:�s�ԍ��Ɩ{���̕~��������(" ")\n"
           "  -n-           �s�ԍ����O��\n"
           "  -t[N]         �^�u�T�C�Y�̐ݒ�(�ȗ�:4)\n"
           "  -m[N]         �w�b�_���t�b�_�g�̔ԍ�\n"
           "  -ql<STR>      �s����STR�̂���s�̂ݐF�ϊ��A�ȊO�̓n�[�t�g�[������\n"
           "  -qr<STR>      -ql �Ƃقړ��l�����ASTR���͍̂폜���ďo��\n"
           "  -u[-]         UTF8 �œ��͂��� / -u- ���Ȃ�(Multi Byte Char)\n"
           "  -s            �W�����o�͂��s��\n"
           "  -gencfg       �f�t�H���gcfg��`��W���o��\n" ;
    exit(1);
}


// ---------------------------------------------------------------------------

/** �I�v�V�����v�f�̊Ǘ� */
class Opts {
public:
    Opts(const char *appNm="")
        : outName_(), cfgName_(), tabSz_(-1), lnoClm_(-1), lnoSep_()
        , stdio_(0), cfgOut_(0), hdrFtrMd_(0), markStr_(), markMd_(0), useUtf8_(-1)
    {
        cfgName_ = string(appNm);
        fname_chgExt(cfgName_, "cfg");
    }
    ~Opts() {}
    void get(const char *p);

public:
    string  outName_;       ///< �o�͖�(1�񂫂�)
    string  cfgName_;       ///< �R���t�B�O�t�@�C����.
    int     tabSz_;         ///< �^�u�E�T�C�Y.
    int     lnoClm_;        ///< 0�ȏ�:�s�ԍ�������.�l�͌��� -1:���Ȃ�.
    string  lnoSep_;        ///< �s�ԍ��������́A�ԍ��ƃ\�[�X�s�Ƃ̊Ԃ̕���.����" "
    int     stdio_;         ///< 1:"-s"�ŕW�����o�͉\�ɂ���. 0:�W�����o�͂��Ȃ�.
    int     cfgOut_;        ///< �f�t�H���g��cfg�̓��e���o��.
    int     hdrFtrMd_;      ///< 1:�w�b�_�t�b�_�̏o�͂�}�~.
    string  markStr_;       ///< �s���ɂ��̕����񂪂���ΐF�ϊ��A������΃n�[�t�g�[��.
    int     markMd_;        ///< markStr_ �� 0:���g�p 1:���̂܂܏o�� 2:�폜���ďo��.
    int     useUtf8_;       ///< ���̓\�[�X�� utf8���ۂ�.
};



/** '-'�Ŏn�܂�R�}���h���C���I�v�V�����̉��.
 */
void Opts::get(const char *arg)
{
    const char *p = arg + 1;
    int c = *p++;
    c = toupper(c);
    switch (c) {
    case 'O':
        outName_ = string(p);
        break;
    case 'C':
        cfgName_ = string(p);
        break;
    case 'T':
        tabSz_ = strtol(p,0,0);
        break;
    case 'S':
        stdio_   = (*p != '-');
        break;
    case 'G':
        cfgOut_ = stricmp(p-1, "gencfg") == 0;
        break;
    case 'M':
        hdrFtrMd_ = strtol(p,0,0);
        break;
    case 'Q':
        c = *p++;
        c = toupper(c);
        if (c == 'L') {
            markMd_  = 1;
            markStr_ = string(p);
            if (markStr_.empty())
                goto OPTS_ERR;
        } else if (c == 'R') {
            markMd_  = 2;
            markStr_ = string(p);
            if (markStr_.empty())
                goto OPTS_ERR;
        } else {
            goto OPTS_ERR;
        }
        break;
    case 'N':
        lnoClm_ = 5;
        if (*p == 0)
            break;
        if (*p == '-') {
            lnoClm_ = 0;
            p++;
        } else {
            lnoClm_ = strtol(p,(char **)&p,0);
        }
        if (*p == ':') {
            p++;
            if (strlen(p) > 0)
                lnoSep_ = string(p);
        }
        break;
    case 'U':
        useUtf8_ = (*p != '-');
        break;
    case '?':
        ::usage();
        break;

    default:
  OPTS_ERR:
        CERR << arg << " �͒m��Ȃ��I�v�V�����ł�\n";
        exit(1);
    }
}



// ---------------------------------------------------------------------------

/** c/c++�\�[�X -> html �ϊ��̊Ǘ� */
class Conv {
public:
    Conv()  : hdrFtrMd_(0)
            , tabSz_(0)
            , useUtf8_(false)
            , lnoClm_(0)
            , lnoSep_()
            , css_()
            , cfgName_()
            , dfltCfgName_()
            , symChrs_()
            , nameSyms_()
            , markStr_()
            , markMd_(0)
            , cpp2htm_()
            , cpp2txt_()
    {
        for (int i = 0; i < HF_NUM; i++) {
            header_[i] = "";
            footer_[i] = "";
        }
        for (int j = 0; j < int(ConvCpp2Htm::T_NUM); j++) {
            cssStr_[j] = "";
            cssStrSw_[j] = false;
        }
    }
    ~Conv() {}

    /// �f�t�H���g�̃R���t�B�O�t�@�C������ݒ�.
    void setDfltCfgName(string &cfgname) {dfltCfgName_ = cfgname;}

    /// �R���t�B�O�t�@�C�������[�h�ς��ۂ�.
    bool isCfgLoaded() const {return cfgName_.empty() == false;}

    /// �R���t�B�O�t�@�C����ǂݍ���.
    bool cfgLoad(const string &cfgFname);

    /// �w�b�_�t�b�_���o�͂��邩�ۂ���ݒ�.
    void setHdrFtrMode(int md) {
        if (md < 0 || md >= HF_NUM) {
            CERR << "-m[N]�� N �� 0�`" << (HF_NUM-1) << " �̒l�ɂ��Ă�������\n";
            md = 0;
        }
        hdrFtrMd_ = md;
    }

    /// �^�u�T�C�Y��ݒ肷��.
    void setTabSz(int sz)   {tabSz_  = sz;}

    /// �s�ԍ��̌�����ݒ肷��.
    void setLnoClm(int clm) {lnoClm_ = clm;}

    /// �s�ԍ��ƃe�L�X�g�̋�؂蕶�����ݒ肷��.
    void setLnoSep(const string &sep) {lnoSep_ = sep;}

    /// �}�[�N�s�̈�����ݒ�.
    void setMarkLine(int mode, string &str) {markMd_ = mode; markStr_ = str;}

    /// �\�[�X��utf8���ۂ�.
    void setUtf8(int sw) { useUtf8_ = sw != 0; cpp2htm_.setUtf8(sw != 0);}

    /// �ϊ������s����.
    int  run(const char *name, const char *outName);

    static const char dfltCfgData_[];       ///< �f�t�H���g�̃R���t�B�O�f�[�^.


private:
    typedef ConvCpp2Htm::tok_t  tok_t;

    /// tok�ɑΉ�����css�����񂪐ݒ肳��Ă��邩�ۂ�.
    bool    isCssStr(int tok) { return (cssStr_[tok].empty() == false);}

    /// �R���t�B�O�ŁAtok�O���[�v�ɑ�����P���o�^.
    void    defWord(CfgFile &cf, const char *tag, tok_t tok);

    /// �w�b�_��t�b�^���� "*fn*" ����������ۂ̃t�@�C�����ɒu��������̂Ɏg��.
    void    replaceFn(const char *name, string &hdr);

    /// �}�[�N�s�̃`�F�b�N
    bool    checkMarkLine(char *buf);


private:
    enum {      HF_NUM = 4 };
    static const char *c_tag_[];                ///< �R���t�B�O���̖��O.
    int         hdrFtrMd_;                      ///< �w�b�_�t�b�_�̏o�̗͂L��.
    int         tabSz_;                         ///< �^�u�T�C�Y.
    bool        useUtf8_;                       ///< �\�[�X��UTF8���ۂ�.
    int         lnoClm_;                        ///< 1�ȏ�:�s�ԍ���t��.�l�͌��� 0�ȉ�:���Ȃ�.
    string      lnoSep_;                        ///< �s�ԍ�������ꍇ�́A�ԍ��ƃe�L�X�g�̊Ԃ̕�����.����" "
    string      header_[HF_NUM];                ///< html �Ƃ��ēf���o���擪.
    string      footer_[HF_NUM];                ///< html �Ƃ��ēf���o���Ō�.
    string      css_;                           ///< html �Ƃ��ēf���o���F�t���̐ݒ�.
    string      cfgName_;                       ///< �R���t�B�O�t�@�C���̖��O.
    string      dfltCfgName_;                   ///< �f�t�H���g�̃R���t�B�O�t�@�C���̖��O.
    bool        cssStrSw_[int(ConvCpp2Htm::T_NUM)]; ///< C_��`�����邩�ǂ���.
    string      cssStr_[int(ConvCpp2Htm::T_NUM)];
    string      symChrs_;
    string      nameSyms_;
    string      markStr_;                       ///< �s���ɂ��̕����񂪂���ΐF�ϊ��A������΃n�[�t�g�[��.
    int         markMd_;                        ///< markStr_ �� 0:���g�p 1:���̂܂܏o�� 2:�폜���ďo��.
    ConvCpp2Htm cpp2htm_;                       ///< c/c++ -> html ��1�s�ϊ�.
    ConvCpp2Htm cpp2txt_;                       ///< �}�[�N�s�O���n�[�t�g�[���ɂ���ꍇ�̂P�s�ϊ�.
};



/** �F�t�������ނ̖��O */
const char *Conv::c_tag_[] = {
    "",
    "C_LNO", "C_SY",  "C_STR", "C_CH",   "C_RSTR",
    "C_ESC", "C_ESCX","C_VAL", "C_OCT",
    "C_CMT", "C_CMTJ", "C_CMTK","C_CMTD", "C_CMTEX1", "C_CMTEX2",
    "C_SH",  "C_W1",  "C_W2",  "C_W3",   "C_W4",
    "C_W5",  "C_W6",  "C_W7",  "C_W8",   "C_W9",
    "C_HT",
};


/** �f�t�H���g�̃R���t�B�O�f�[�^ */
const char Conv::dfltCfgData_[] = {
    #include "c2htm.cfg.cc"
};


/** �R���t�B�O�t�@�C����ǂݍ���
 *  �ǂݍ��݂͂P�x�݂̂����A������_��
 */
bool Conv::cfgLoad(const string &cfgFname)
{
    CfgFile cf;

    if (!cfgName_.empty()) {
        CERR << "cfg�������w�肳��Ă��܂�" << endl;
        return false;
    }

    int rc = cf.init(cfgFname);
    if (rc == 0) {
        //
        CERR << "�R���t�B�O�t�@�C�� " << cfgFname << " ��������܂���B\n";
        if (cfgFname == dfltCfgName_) {
            CERR << "�Œ�� exe�Ɠ����t�H���_�� " << fname_getBase(dfltCfgName_.c_str())
                 << " ��u���Ă��������B\n";
        }
        CERR << "����͓����̃f�t�H���g�ݒ�ŕϊ����܂��B\n"
                "(�����f�t�H���g�� c2htm -gencfg �Ƃ���Ίm�F�ł��܂�)\n";
        // �R���t�B�O�t�@�C��������������A�f�t�H���g�̃e�[�u�����g��.
        rc = cf.init(dfltCfgData_, 1);
        if (rc == 0) {
            return false;
        }
    }

    // �I�v�V�����v�f�̎擾.
    tabSz_  = cf.getVal("TAB");
    lnoClm_ = cf.getVal("LNO");
    cf.getStr(lnoSep_, "LNOSEP");

    setUtf8(cf.getVal("UTF8"));

    // �L�������̈����Ɋւ���擾.
    cf.getStr(symChrs_, "SYM");
    cf.getStr(nameSyms_, "NAMESYM");
    cpp2htm_.initChTop(symChrs_, nameSyms_);
    {   // // /* �R�����g����߁A�ʂ�1�s�R�����g���s���ꍇ.
        string tmpStr;
        rc = cf.getStr(tmpStr, "LINECMT");
        if (rc) {
            cpp2htm_.setLineCmtChr(tmpStr[0]);
        }
    }

    // ���[�U�[�g���P�s�R�����g�̕���.
    string cmtExStr;
    cf.getStr(cmtExStr, "CMTEX1");
    cpp2htm_.setExCmtChr(0, cmtExStr.c_str());
    cmtExStr.clear();
    cf.getStr(cmtExStr, "CMTEX2");
    cpp2htm_.setExCmtChr(1, cmtExStr.c_str());

    // ��ނ��Ƃ̐ݒ�.
    css_.clear();
    for (int i = ConvCpp2Htm::T_ZERO; i <= ConvCpp2Htm::T_WORD9; i++) {
        tok_t   tok = tok_t(i);

        // �ݒ�̗L��(on/off)�Ɠ��e���擾.
        cssStr_[tok].clear();
        cssStrSw_[tok] = cf.getStr(cssStr_[tok], c_tag_[i]);
        if (cssStrSw_[tok] && cssStr_[tok].empty())
            cssStrSw_[tok] = false;
        cpp2htm_.useTokGroupSw(tok, cssStrSw_[tok]);

        // html�o�͗p�̒�`���쐬.
        if (isCssStr(tok)) {
            css_ += ".";
            css_ += c_tag_[tok];
            css_ += "{";
            css_ += cssStr_[tok];
            css_ += "}\n";
        }
    }
    {   // �n�[�t�g�[���ݒ�͕ʘg�����A�����ł�����Ⴄ.
        string htStr("");
        if (cf.getStr(htStr, "C_HT") && !htStr.empty())
            css_ += ".C_HT{" + htStr + "}\n";
    }

    // �F������P��̎擾���o�^.
    defWord(cf, "SHARP", ConvCpp2Htm::T_SHARP);
    defWord(cf, "WORD1", ConvCpp2Htm::T_WORD1);
    defWord(cf, "WORD2", ConvCpp2Htm::T_WORD2);
    defWord(cf, "WORD3", ConvCpp2Htm::T_WORD3);
    defWord(cf, "WORD4", ConvCpp2Htm::T_WORD4);
    defWord(cf, "WORD5", ConvCpp2Htm::T_WORD5);
    defWord(cf, "WORD6", ConvCpp2Htm::T_WORD6);
    defWord(cf, "WORD7", ConvCpp2Htm::T_WORD7);
    defWord(cf, "WORD8", ConvCpp2Htm::T_WORD8);
    defWord(cf, "WORD9", ConvCpp2Htm::T_WORD9);

    // �w�b�_�t�b�^�̎擾.
    cf.getStr(header_[0], "HEADER");
    cf.getStr(footer_[0], "FOOTER");
    cf.getStr(header_[1], "HEADER1");
    cf.getStr(footer_[1], "FOOTER1");
    cf.getStr(header_[2], "HEADER2");
    cf.getStr(footer_[2], "FOOTER2");
    cf.getStr(header_[3], "HEADER3");
    cf.getStr(footer_[3], "FOOTER3");
    cf.term();

    // �w�b�_�t�b�_���� *css* �����ۂ̐ݒ�ɒu��.
    for (int n = 0; n < HF_NUM; n++) {
        for (;;) {
            rc = header_[n].find("*css*");
            if (rc == int(string::npos))
                break;
            header_[n].replace(rc, 5, css_);
        }
        for (;;) {
            rc = footer_[n].find("*css*");
            if (rc == int(string::npos))
                break;
            footer_[n].replace(rc, 5, css_);
        }
    }
    return true;
}



/** �R���t�B�O�ŁAtok�O���[�v�ɑ�����P���o�^
 *  @param cf   �������̃R���t�B�O�t�@�C���̓��e
 *  @param tag  �R���t�B�O�t�@�C�����̒�`��
 *  @param tok  �o�^����P��̎��(T_SHARP,T_WORD1�`T_WORD9)
 */
void Conv::defWord(CfgFile &cf, const char *tag, tok_t tok)
{
    vector<string> lst;
    lst.clear();
    if (cssStrSw_[tok]) {
        if (cf.getStrVec(lst, tag)) {
            for (int i=0; i < int(lst.size()); i++) {
                cpp2htm_.defWord(lst[i], tok);
            }
        }
    }
}



/** name��c/c++�t�@�C����outName��html�ɕϊ����ďo��
 */
int Conv::run(const char *name, const char *outName)
{
    // ���̓t�@�C���p��.
    ifstream inFstrm;
    if (name && name[0]) {
        inFstrm.open(name);
        if (!inFstrm) {
            CERR << name << " ��open�ł��Ȃ�\n";
            return false;
        }
    }
    istream &istrm = (name && name[0]) ? static_cast<istream&>(inFstrm) : cin;

    // �o�̓t�@�C���p��.
    ofstream outFstrm;
    if (outName && outName[0]) {
        outFstrm.open(outName);
        if (!outFstrm) {
            CERR << outName << " ��open�ł��Ȃ�\n";
            return false;
        }
    }
    ostream &ostrm = (outName && outName[0]) ? static_cast<ostream&>(outFstrm) : cout;

    // �w�b�_�ƃt�b�_�̏���.
    string hdr(header_[hdrFtrMd_]), ftr(footer_[hdrFtrMd_]);
    replaceFn(name, hdr);
    replaceFn(name, ftr);

    // �w�b�_�o��.
    ostrm << hdr;

    // �\�[�X�ϊ�.
    int lineNum = 0;
    string lnoSp(lnoSep_);
    if (lnoSp.empty())
        lnoSp = " ";
    string st;
    while (!istrm.eof()) {
        lineNum++;
        char buf[0x4000];                               // c/c++�\�[�X�P�s�̃o�b�t�@.
        char buf2[0x10000];                             // tab�W�J��p�̃o�b�t�@.
        st.clear();                                     // �����o�b�t�@��������.
        buf[0] = 0;
        istrm.getline(buf, sizeof buf);                 // 1�s����.
        if (istrm.eof() && buf[0] == 0)
            break;
        strTab(buf2, buf, 0, tabSz_, useUtf8_<<9, sizeof buf2); // tab�ϊ�.
        if (markMd_ == 0 || checkMarkLine(buf2)) {      // �ʏ�̕ϊ��A�܂��́A�}�[�N�s�Ȃ�.
            cpp2htm_.convLine(st, buf2);                // c/c++ �s�� html �ɕϊ�.
            if (lnoClm_ > 0)                            // ����������΍s�ԍ���t����.
                ostrm << "<span class=C_LNO>" << setw(lnoClm_) << lineNum << lnoSp << "</span>";
        } else {                                        // �}�[�N�s���`�F�b�N����ꍇ.
            st += "<span class=C_HT>";
            cpp2txt_.convLine(st, buf2);
            st += "</span>";
            if (lnoClm_ > 0)                            // ����������΍s�ԍ���t����.
                ostrm << "<span class=C_HT>" << setw(lnoClm_) << lineNum << lnoSp << "</span>";
        }
        ostrm << st << endl;
    }

    // �t�b�^�o��
    ostrm << ftr;
    return true;
}



/** �w�b�_��t�b�_������ *fn* ������΁A�t�@�C�����ɒu��������
 */
void Conv::replaceFn(const char *name, string &hdr)
{
    //x hdr = header_;
    //x ftr = footer_;
    int i;
    for (;;) {
        i = hdr.find("*fn*");
        if (i == int(string::npos))
            break;
        hdr.replace(i, 4, string(name));
    }
}



/** �}�[�N�s�H �����[�h�Q�Ȃ�buf2��ҏW
 */
bool Conv::checkMarkLine(char *buf)
{
    size_t  l = strlen(buf);
    if (l < markStr_.size())
        return false;
    char *p = buf + l - markStr_.size();
    if (p != markStr_)
        return false;
    if (markMd_ == 2)
        *p = '\0';
    return true;
}




// ---------------------------------------------------------------------------

/// ���C���̏���
class App {
public:
    App() {}
    ~App() {}
    int main(int argc, char *argv[]);
    int oneFile(const char *name, Conv &conv, Opts &opts);
};



/** ���ۂ̃��C�������B�I�v�V������́��t�@�C���R���o�[�g�Ăяo��
 */
int App::main(int argc, char *argv[])
{
    if (argc < 2)   // �R�}���h���C������������������w���v�\��.
        ::usage();

    Opts    opts(argv[0]);
    Conv    conv;
    conv.setDfltCfgName(opts.cfgName_);
    int rc = 0, n = 0;
    for (int i = 1; i < argc; i++) {
        char *p = argv[i];
        if (*p == '-') {
            opts.get(p);
            // �f�t�H���gcfg�t�@�C���̓f���o��.
            if (opts.cfgOut_) {
                cout << conv.dfltCfgData_;
                opts.cfgOut_ = 0;
            }
        } else if (*p == ':') {
            string nm(argv[0]);
            fname_chgExt(nm, NULL);
            nm += "-";
            nm += p+1;
            nm += ".cfg";
            opts.cfgName_ = nm;
        } else {
            rc = oneFile(p, conv, opts);
            n++;
        }
    }
    if (n == 0 && opts.stdio_) {
        rc = oneFile("", conv, opts);
    }
    rc = !rc;
    return rc;
}


/** 1�t�@�C���ϊ��̏�����go
 */
int App::oneFile(const char *name, Conv &conv, Opts &o)
{
    string inm(name);
    string onm(name);

    if (o.outName_.empty()) {
        if (o.stdio_)
            onm = "";
        else
            onm += ".htm";
    } else {
        onm = string(o.outName_);
        o.outName_ = "";    // -o�͈�񂱂�����Ȃ�ŁA��������ɏ�����.
    }
    if (conv.isCfgLoaded() == false) {
        int rc = conv.cfgLoad(o.cfgName_);
        if (rc == 0) {
            CERR << "�R���t�B�O�t�@�C�� " << o.cfgName_ << " �����[�h�ł��Ȃ�����\n";
            return 0;
        }
    }
    // �I�v�V�����ݒ��ϊ����֒ʒm.
    conv.setHdrFtrMode(o.hdrFtrMd_);
    if (o.tabSz_ > -1)
        conv.setTabSz(o.tabSz_);
    if (o.lnoClm_ >= 0)
        conv.setLnoClm(o.lnoClm_);
    if (!o.lnoSep_.empty())
        conv.setLnoSep(o.lnoSep_);
    if (o.useUtf8_ >= 0)
        conv.setUtf8(o.useUtf8_);
    conv.setMarkLine(o.markMd_, o.markStr_);

    // inm�̃t�@�C����onm�ɕϊ������s.
    return conv.run(inm.c_str(), onm.c_str());
}




// ---------------------------------------------------------------------------

/** �������n�܂� */
int main(int argc, char *argv[])
{
    int rc;
    App app;
    try {
      #ifdef _MSC_VER
        // VC(7)�ŃR���p�C��������Aargv[0]�Ƀt���p�X�łȂ��A�R�}���h���C��.
        // �Ń^�C�v�����v���O���������������Ă��Ȃ������̂ŁA�����Ώ�.
        argv[0] = _pgmptr;
      #endif
        rc = app.main(argc, argv);

    } catch (const exception &ex) {
        CERR << ex.what() << endl;
        rc = 1;
    }
    return rc;
}



// ---------------------------------------------------------------------------
// c2htm ��ƃ���
// 2003-07-21   �쐬�J�n.
// 2003-07-22   �Ƃ肠��������.�R���t�B�O����.
// 2003-07-23   convCpp2htm.cpp�\�[�X����폜���Ă��܂��A�����i�𕜌����Ȃ���....
// 2003-07-24   �R���t�B�O�Ή��B-s�ŕW�����o�͑Ή�(fstream�����FILE��T T)�B
//              �R���t�B�O�ݒ�(�L��)�ŁA���ڂ̈ꕔ�@�\��on/off�\��.
// 2003-07-25   �f�o�b�O�������.
// 2003-07-27   �f�t�H���gcfg�t�@�C�����������悤�ɂ���.
// 2003-07-27   v1.00 �Ƃ��Č��J.
// 2003-07-28   v1.01 �s�ԍ����O���Ȃ��Ȃ��Ă��̂ŏC��. �Ă�Conv�ɏ�����������������.
// 2003-08-08   v1.02 \0 �� \123 ��8�i���ōŏ��̈ꕶ�����o�͂��Y���̂��C��.
//              �s�ԍ�off���A-1���łȂ�0���Ŕ��f����悤�ɏC��.
// 2003-08-09   v1.50
//              �E�z�z�p�b�P�[�W���� cfgfile.cpp,cfgfile.h,c2htm.cfg.c
//              �@�\�[�X�t�@�C���������Ă��܂��� m(_ _)m
//              �E�W�����o�͂�-s��usage�ɏ����Y��Ă��̂�ǉ�.
//              �Efopen/fprintf������߂�ofstream���ɕύX.
//              �Ecfg��̓��[�`���̕s��C��.
//              �Ecfg���ŁA�S�Ă�C_�ݒ���A�폜/�R�����g���A�ŋ@�\off�ł���悤�ɏC��.
//              �Ecfg���ŁASYM=,NAMESYM=�ɂāA�L�������̎�ʂ�ݒ�\��.
//              �Ecfg���ŁALINECMT=�ɂāA// /* ���~�ߋL���ꕶ���ł̂P�s�R�����g���\��.
// 2003-08-10   v1.51  c/c++�\�[�X�Ƃ��ĂP�s�]���ɋ�s���o�͂��Ă����̂��C��.
// 2003-08-11   v1.52 ���� -t �ł̃^�u�w�肪��������Ă��Ȃ�����(T T)
// 2003-08-18   v1.53 '.'���L�������ɂ��Ă��Ȃ��Ƃ��A�o�͂��猇���Ă��܂����̂��C��.
// 2003-08-21   v1.60 �s���Ƀ}�[�N�̂���s�̂ݐF���ϊ��A�ȊO�̓n�[�t�g�[������ �ɂ���I�v�V������ǉ�.
// 2004-01-03   v1.61 strtab.c�̃o�O�C��.
// 2004-01-03   v1.61 strtab.c�̃o�O�C��.
// 2004-01-29   v1.70 D��������̏C���B utf8�̓��́B /+�R�����g+/
//              �E\�G�X�P�[�v���Ȃ�������Ƃ��� r"������" `������` �ɑΉ�.
// 2004-02-07   v1.71 ���l��G�X�P�[�v�����Ή���(D������)�����Ώ�.
// 2005-11-??   v1.72 CMTEX1,CMTEX2�̒ǉ�.
// 2017-09-16   v1.73 c++11�ȍ~�̗\����ǉ�. L""���l�� u"",U"",u8"" �Ή�.
