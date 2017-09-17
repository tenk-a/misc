/**
 *  @file   ConvCpp2Htm.h
 *  @brief  c/c++�\�[�X��html��
 *
 *  @author Masahi KITAMURA (tenka@6809.net)
 *  @date   2003-07-21 �`
 */

#ifndef CONVCPP2HTM_H
#define CONVCPP2HTM_H
#include <string>
#include <map>
#include <assert.h>


/// c/c++ �\�[�X�������html�ɕϊ�����
class ConvCpp2Htm {
public:
    enum tok_t {
    	T_NONE,     ///< ��ڂȂ�...�����łȂ��A�O�̃g�[�N���ɓ����A�̖��Ƃ��Ďg���Ă��܂��Ă�B
    	T_ZERO,     ///< '\0' �����������s�R�[�h����
    	T_SYM,	    ///< ���̑��̋L��
    	T_WQUOT,    ///< " ������
    	T_SQUOT,    ///< ' �����萔
    	T_BSQUOT,   ///< ` �����萔(for D����)
    	T_ESC,	    ///< �����񒆂� \�G�X�P�[�v
    	T_ESCX,     ///< �\�[�X���� \�G�X�P�[�v
    	T_IVAL,     ///< ���l
    	T_OVAL,     ///< 8�i��
    	T_CMT,	    ///< //, /* �R�����g
    	T_CMTJ,     ///< ///, /**, //!, /*! doxygen�R�����g
    	T_CMTK,     ///< // �R�����g ��//�̒���ɋ�(���s�܂�)���Ȃ��ꍇ
    	T_CMTD,     ///< D����� /+ +/ (�\�[�X�̃R�����g�A�E�g����)
    	T_CMTEX1,   ///< //x ���̃��[�U�[��`�ŐF�Ⴂ�ɂ���//�R�����g ����1
    	T_CMTEX2,   ///< //* ���̃��[�U�[��`�ŐF�Ⴂ�ɂ���//�R�����g ����2
    	T_SHARP,    ///< #.. �����A�Ƃ肠�����ʏ�̒P�ꈵ���ŏ���
    	T_WORD1,    ///< �o�^�P��1
    	T_WORD2,    ///< �o�^�P��2
    	T_WORD3,    ///< �o�^�P��3
    	T_WORD4,    ///< �o�^�P��4
    	T_WORD5,    ///< �o�^�P��5
    	T_WORD6,    ///< �o�^�P��6
    	T_WORD7,    ///< �o�^�P��7
    	T_WORD8,    ///< �o�^�P��8
    	T_WORD9,    ///< �o�^�P��9
    	//T_HT,     ///< �n�[�t�g�[������

    	T_NAME,     ///< ���O(�ɂȂ肻���ȕ���)
    	T_MEMB,     ///< �����o���̑O�ɗ���L�� . -> .* ->*
    	T_LF,	    ///< '\n' �����o�����Ȃ�����

    	// ��͎��̔��f�p
    	T_SLA,	    ///< '/'
    	T_ASTA,     ///< '*'
    	T_AMP,	    ///< '&'
    	T_LT,	    ///< '<'
    	T_GT,	    ///< '>'
    	T_PERIOD,   ///< '.'
    	T_MINUS,    ///< '-'
    	T_PLUS,     ///< '+'
    	//T_L,
    	T_NUM,	    ///< �g�[�N����ʂ̐�( �͈͊O�������Ƃ��Ă����p)
    };

    ConvCpp2Htm();
    ~ConvCpp2Htm() {};

    /// �������ʗp�e�[�u���̏�����. symChrs:�L�������̕���. nameSymChrs:���O�����̋L������.
    void initChTop(const std::string &symChrs, const std::string &nameSymChrs);

    /// ������Ԃ��N���A
    //void clear() {mode_ = MODE_NORM;}

    /// src ��c/c++�e�L�X�g��html �ɕϊ�����dst�ɓ���ĕԂ�
    bool convLine(std::string &dst, const char *src);

    /// SH,W1�`W9 �̒P��o�^
    void defWord(const std::string &name, tok_t tok) {wordTbl_[name] = tok;}

    /// �g�[�N����ޕʂ̐F�Â���on/off �X�C�b�`�ݒ�
    void useTokGroupSw(tok_t tok, bool sw) {tokSw_[tok] = sw;}

    /// // /* �̃R�����g��p�~���A��s�R�����g�� c �Ŏn�܂郂�m�ɕύX
    void setLineCmtChr(int c) { lineCmtChr_ = c;}

    /// ���͕�����Ƃ��� utf8 ���g��
    void setUtf8(bool sw) {useUtf8_ = sw;}

    void setExCmtChr(int n, int c) {
    	assert(n >= 0 && n <= 1);
    	cmtExChr_[n] = c;
    }

    void setExCmtChr(int n, const char *s) {
    	assert(s && n >= 0 && n <= 1);
    	cmtExChr_[n] = *s;
    	//x printf("cmtEx%d:%c\n", n+1, *s);
    }

private:
    /// html�o�͗p�̃e�[�u���̏�����
    void initMrkBgnEnd();

    /// �\��P��̓o�^(������)
    void initTbl();

    /// ����̃\�[�X�s������̐ݒ�
    void setSrcPtr(const char *p)   {srcPtr_ = (const unsigned char *)p;}

    /// 1�����擾
    int  getC()     	    	    {int c = *srcPtr_; if (c) srcPtr_++; return c;}

    /// ���Ŏ��̈ꕶ��������Ă݂�B����Ɣ����Z�����B
    int  peekC()    	    	    {int c = *srcPtr_; return c;}
    int  peekNextC()	    	    {int c = srcPtr_[0] ? srcPtr_[1] : 0; return c;}

    /// 1�����ԋp
    void ungetC(int c)	    	    {if (c) --srcPtr_;}     // �蔲���Ȃ�ŁA�����͖���

    /// �󔒂��擾
    void getSpc(std::string &st);

    /// ���O��������擾
    void getName(std::string &st);

    /// ���l��������擾
    tok_t getVal(std::string &st, tok_t tok);

    /// \�G�X�P�[�v��������擾
    void getEsc(std::string &st);

    /// \ �G�X�P�[�v�V�[�P���X�̏���
    void chkGetYenEsc(std::string &st);

    /// ���[�h�Ɩ��O����A����̃g�[�N��������
    tok_t chkName2Tok(const std::string &name, tok_t tok);

    /// '#' �P��̃`�F�b�N.
    tok_t chkGetSharpName(std::string &st);

    /// �g�[�N����������擾
    tok_t getTok(std::string &st);

    /// '/'�Ŏn�܂�R�����g�̏���
    tok_t chkGetCmt(std::string &st);

    /// �s���ł�tok�̒����킹
    void chkSetMode2CurTok();

    /// �������L�����ǂ���
    bool isSym(char c) {
    	size_t n = symChrs_.find_first_of(c);
    	return n != std::string::npos;
    }

    /// �L�������p�Ƀ��[�h�����l�����āAtok�𒲐�
    tok_t chkSymTok(int c, tok_t tok) {
    	tok  = (isSym(c) && mode_ == MODE_NORM) ? tok : T_NONE;
    	if (c == lineCmtChr_ && tok != T_NONE) {
    	    // 1�s�R�����g�����������ꍇ
    	    tok      = T_CMT;
    	    mode_    = MODE_CMT1;
    	    cmt_typ_ = CMT_NORM;
    	}
    	return tok;
    }


private:
   #if 0    // �R���t�B�O���Ή����̏������p
    struct  ini_cword_t {
    	const char *name;
    	tok_t	    tok;
    };
    static ini_cword_t	ini_cwords_[];
   #endif
    struct  mrkTbl_t {
    	tok_t	    tok;
    	const char *bgn;
    	const char *end;
    };
    /// html�o�͂ŃL�[���[�h�̑O��ɒ�����<strong class=...></strong>�Ȃǂ̕�����ݒ�
    static mrkTbl_t 	mrkTbl_[];
    static bool     	nextTokNoneOkTbl[T_NUM+1];

    /// ���ݏ������̃\�[�X�̏��.
    enum    mode_t {
    	MODE_NORM,  	///< �ʏ��c�v���O�����e�L�X�g
    	MODE_STR,   	///< "������"��
    	MODE_CVAL,  	///< '����'�萔��
    	MODE_RSTR,  	///< `������`��
    	MODE_STR_R, 	///< r"������"��
    	MODE_CMT1,  	///< // �P�s�R�����g
    	MODE_BLKCMT,	///< /* �u���b�N�R�����g
    	MODE_BLKCMTD,	///< /+ �u���b�N�R�����g
    };
    mode_t  	    	mode_;	    	    ///< ���

    /// �R�����g�E�^�C�v
    enum cmt_typ_t {
    	CMT_NORM,   	///< �ʏ�̃R�����g.
    	CMT_DOXY,   	///< doxygen�`���̃R�����g
    	CMT_NOSPC,  	///< "// "��"/* "�̂悤�ɃR�����g����ɋ󔒂��Ȃ��ꍇ
    	CMT_D,	    	///< D����� /+ +/ �R�����g
    	CMT_EX1,    	///< ���[�U�[�g���R�����g
    	CMT_EX2,    	///< ���[�U�[�g���R�����g
    };
    cmt_typ_t	    	cmt_typ_;   	    ///< �R�����g�E�^�C�v

    const unsigned char *srcPtr_;   	    ///< 1�����擾�ł̃|�C���^
    std::string     	tokStr_;    	    ///< �g�[�N���擾�p�̃o�b�t�@
    bool    	    	escEolFlg_; 	    ///< �Ōオ \ ���������̃t���O
    bool    	    	tokSw_[T_NUM];	    ///< ����̃g�[�N���O���[�v�̎g�p��on/off
    tok_t   	    	cur_tok_;   	    ///< ���݂̃g�[�N��
    tok_t   	    	bak_tok_;
    int     	    	lineCmtChr_;	    ///< 1�s�R�����g�ɂ��镶��
    tok_t   	    	chTypTbl_[256];     ///< �擪�P�����̔���p�e�[�u��
    std::string     	symChrs_;   	    ///< �L�������ɂ���L�������̏W�܂�
    std::string     	nameSyms_;  	    ///< ���O�Ɋ܂߂�L������
    bool    	    	useUtf8_;   	    ///< ���͂� UTF8 ���ۂ�
    bool    	    	str_r_mode_;	    ///< r"..." �`�F�b�N�p
    char    	    	cmtExChr_[2];	    ///< ���[�U�[�g���ȃR�����g�̕���

    typedef std::map<std::string, tok_t >   WordTbl;
    WordTbl 	    	    	    wordTbl_;	///< �P�꒠
    std::map<tok_t, std::string>    mrkBgn_;	///< html�o�͂ł̊J�n������
    std::map<tok_t, std::string>    mrkEnd_;	///< html�o�͂ł̏I��������
};



#endif	// CONVCPP2HTM_H
