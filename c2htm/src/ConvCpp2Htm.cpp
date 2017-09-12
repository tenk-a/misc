/**
 *	@file	ConvCpp2Htm.cpp
 *	@brief	c/c++�\�[�X��html��
 *
 *	@author	�k����j<NBB00541@nifty.com>
 *	@date	2003-07-21 �`
 */

#include <iostream>
#include <fstream>
#include "ConvCpp2htm.h"
#include "cmisc.h"

using namespace std;


/// c/c++ �\�[�X���P�s�Ahtml �ŐF���ɕϊ��B
/// (�u���b�N�R�����g�╶�����Ԃ͓����ɕێ����Čp��)
bool ConvCpp2Htm::convLine(std::string &dst, const char *src)
{
	// ����̂P�s����͗p�ɃZ�b�g
	setSrcPtr(src);

	// ���[�h���獡��̃g�[�N��(�Ƃ������F�t���̎��)��ݒ�
	chkSetMode2CurTok();

	escEolFlg_ = false;			// �Ōオ \ ���������̃t���O���N���A

	string spc("");

	// �s���݂̂̏���
	if (mode_ == MODE_NORM && tokSw_[T_SHARP]) {
		int c;
		tokStr_.clear();
		for (;;) {
			c = getC();
			if (c == 0 || c > ' ')
				break;
			spc += char(c);
		}
		if (c == '#' && tokSw_[T_SHARP]) {	// #�s��������
			cur_tok_ = chkGetSharpName(tokStr_);
			spc += tokStr_;
		} else {
			ungetC(c);	// �������߂��Ƃ�
		}
	}

	dst += mrkBgn_[cur_tok_];	// �Ƃ肠�����A�o�͂̕���(�F)�ݒ���Z�b�g
	dst += spc;					// �s����(���邢��#�P��)������Ώo�͂ɒǉ�

	// ������(�s)���I���܂ŌJ��Ԃ�
	for (;;) {
		tokStr_.clear();
		tok_t tok = getTok(tokStr_);
		if (tok == T_ZERO)		// ������̏I���(�s��)������
			break;
		bak_tok_ = tok;
		if (tok == T_MEMB)
			tok = T_SYM;
		// ���݂̃^�C�v�ƈႤ�^�C�v�������H
		if (tok != cur_tok_) {
			// ����̂�NONE �łȂ����ANONE�ł��O��̂�NONE���p���Ώۂɂ��Ȃ���ނ̏ꍇ
			// �o�͐ݒ��ύX
			if (tok != T_NONE || nextTokNoneOkTbl[cur_tok_] == false) {
				if (cur_tok_ != T_NONE) {
					dst += mrkEnd_[cur_tok_];
				}
				cur_tok_ = tok;
				if (cur_tok_ != T_NONE) {
					dst += mrkBgn_[cur_tok_];
				}
			}
		}
		getSpc(tokStr_);
		dst += tokStr_;
	}

	dst += mrkEnd_[cur_tok_];	// �o�͂̕���(�F)�ݒ�̏I�����Z�b�g
	return true;
}


/// ���[�h���獡��̃g�[�N��(�Ƃ������F�t���̎��)��ݒ�
///
void ConvCpp2Htm::chkSetMode2CurTok()
{
	// ���݂̃��[�h�ɂ��킹�āA���s�����[�̐F�̂��߂�
	// cur_tok_ �ɑΉ�����g�[�N����ݒ�B
	switch (mode_) {
	case MODE_STR_R:
	case MODE_STR:		// " ������
		cur_tok_ = T_WQUOT;
		break;

	case MODE_CVAL:		// ' �����萔
		cur_tok_ = T_SQUOT;
		break;

	case MODE_RSTR:		// ` ������
		cur_tok_ = T_BSQUOT;
		break;

	case MODE_BLKCMT:	// /*  �R�����g
		cur_tok_ = (cmt_typ_ == CMT_DOXY) ? T_CMTJ
				 : (cmt_typ_ == CMT_NOSPC)  ? T_CMTK
				 : T_CMT;
		break;

	case MODE_BLKCMTD:	// /+  �R�����g
		cur_tok_ = (cmt_typ_ == CMT_DOXY) ? T_CMTJ
				 : T_CMTD;
		break;

	case MODE_CMT1:		// //  �R�����g
		if (escEolFlg_) {
			// //�R�����g�̍s���� \�������ꍇ... �����1�s�R�����g:-<
			cur_tok_ = (cmt_typ_ == CMT_DOXY) ? T_CMTJ
					 : (cmt_typ_ == CMT_NOSPC)  ? T_CMTK
					 : T_CMT;
		} else {
			// �ʏ�́A�R�����g�I��
			mode_    = MODE_NORM;
			cur_tok_ = T_NUM;
		}
		break;

	//case MODE_NORM:
	default:
		// ��Ζ{�҂Ɍ���Ȃ��^�C�v�ɂ���
		cur_tok_ = T_NUM;
		break;
	}
}


/// �L��,�P��,�R�����g���o�͌����ɕϊ����Ď擾
/// @param st	�擾��������������ĕԂ�
/// @return		�g�[�N��
ConvCpp2Htm::tok_t ConvCpp2Htm::getTok(string &st)
{
	int k;
	int c = getC();
	tok_t tok = chTypTbl_[c];

	switch (tok) {
	case T_LF:		// '\n' �����A���Ԃ�o�����Ȃ�����...������
		//st += char(c);			// ����ɏo�͂���Ɩʓ|���낤�Ŕj��
		// �p��

	case T_ZERO:	// '\0'
		//if (mode_ == MODE_CMT1)	// 1�s�R�����g�̏������͎��s�擪�̔��f
			//mode_ = MODE_NORM;	// �ł���̂ŁA�����͔j��
		break;

	case T_NONE:	// ���̑�
		st += char(c);
		break;

	case T_SYM:		// �L��
		st += char(c);
		tok   = chkSymTok(c, T_SYM);
		break;

	case T_AMP:		// &
		st += "&amp;";
		tok   = chkSymTok(c, T_SYM);
		break;

	case T_LT:		// <
		st += "&lt;";
		tok   = chkSymTok(c, T_SYM);
		break;

	case T_GT:		// >
		st += "&gt;";
		tok   = chkSymTok(c, T_SYM);
		break;

	case T_SHARP:	// # �s���`�F�b�N�͕ʂɂ���Ă���̂ł����ł́A�P�Ȃ�L�� #,## �̂���
		st += char(c);
		tok   = chkSymTok(c, ((tokSw_[T_SHARP]) ? T_SHARP : T_SYM));
		break;

	case T_WQUOT:	// " ������
		st += "&quot;";
		if (isSym(c)) {
			if (tokSw_[tok]) {
				if (mode_ == MODE_NORM) {
					if (str_r_mode_ == 0)
						mode_ = MODE_STR;
					else
						mode_ = MODE_STR_R;
				} else if (mode_ == MODE_STR) {
					mode_ = MODE_NORM;
				} else if (mode_ == MODE_STR_R) {
					mode_ = MODE_NORM;
				} else {
					tok = T_NONE;
				}
			} else {
				tok   = chkSymTok(c, T_SYM);
			}
		} else {
			tok = T_NONE;
		}
		break;

	case T_SQUOT:	// ' �����萔
		st += char(c);
		if (isSym(c)) {
			if (tokSw_[tok]) {
				if (mode_ == MODE_NORM) {
					mode_ = MODE_CVAL;
				} else if (mode_ == MODE_CVAL) {
					mode_ = MODE_NORM;
				} else {
					tok = T_NONE;
				}
			} else {
				tok   = chkSymTok(c, T_SYM);
			}
		} else {
			tok = T_NONE;
		}
		break;

	case T_BSQUOT:	// ` ������
		st += char(c);
		if (isSym(c)) {
			if (tokSw_[tok]) {
				if (mode_ == MODE_NORM) {
					mode_ = MODE_RSTR;
				} else if (mode_ == MODE_RSTR) {
					mode_ = MODE_NORM;
				} else {
					tok = T_NONE;
				}
			} else {
				tok   = chkSymTok(c, T_SYM);
			}
		} else {
			tok = T_NONE;
		}
		break;

	case T_ESC:	//	\ �G�X�P�[�v
		if (tokSw_[T_ESC] && isSym(c)) {			// �ݒ肪����ꍇ
			chkGetYenEsc(st);
			// �����ŏ�������������̂ŁA���ɉe���łȂ��悤��NONE
			tok = T_NONE;
		} else {	// �F�ݒ肪�Ȃ����A�ʏ�̋L������
			tok   = chkSymTok(c, T_SYM);
		}
		break;

	case T_OVAL:		// 0�Ŏn�܂�
		k = peekC();				// ���Ɏ��̈ꕶ��������Ă݂�
		if (k < '0' || '9' < k)		// 0�ɑ�����'0'�`'9'�ȊO�Ȃ�΁A���ʂ̐��l����
			tok = T_IVAL;			//   0�݂̂� 0x 0. �Ŏn�܂鐔�l��z��
		// �P�C�]�N

	case	T_IVAL:		// ���l
		ungetC(c);
		tok = getVal(st, tok);
		tok   = (mode_ == MODE_NORM) ? tok : T_NONE;
		break;

	case	T_PERIOD:	// '.'
		k = peekC();			// ���Ɏ��̈ꕶ��������Ă݂�
		if (isSym(c)) {
			if (isdigit(k) && tokSw_[T_IVAL]) {
				ungetC(c);
				//st += char(c);
				tok = getVal(st, T_IVAL);
				tok = (mode_ == MODE_NORM) ? tok : T_NONE;
			} else {
				st += char(c);
				if (k == '*') {		// .*
					c = getC();
					st += char(c);
				}
				tok   = (mode_ == MODE_NORM) ? T_MEMB : T_NONE;
			}
		} else {
			st += char(c);
			tok = T_NONE;
		}
		break;

	case	T_MINUS:	// '-'
		k = peekC();			// ���Ɏ��̈ꕶ��������Ă݂�
		st += char(c);
		if (isSym(c)) {
			if (k == '>' && isSym(k)) {			// ->
				st += getC();
				k = peekC();					// ���Ɏ��̈ꕶ��������Ă݂�
				if (k == '*' && isSym(k)) {		// ->*
					c = getC();
					st += char(c);
				}
				tok   = (mode_ == MODE_NORM) ? T_MEMB : T_NONE;
			} else {
				tok   = chkSymTok(c, T_SYM);
			}
		} else {
			tok = T_NONE;
		}
		break;

	case	T_NAME:		// ���̂��A�g�[�N����
		k = peekC();			// ���Ɏ��̈ꕶ��������Ă݂�
		if (c == 'L'
			 && ((k == '\'' && tokSw_[T_SQUOT])
			   || (k == '"'  && tokSw_[T_WQUOT]))
			 && mode_ == MODE_NORM)
		{
			// L�̒����'��"�������wchar_t�p������/�萔�Ƃ��ď����B
			// ��L�Ƃ̊Ԃɋ󔒂͒u���Ȃ����@�Ȃ�ŋC�ɂ��Ȃ�^^;
			st += char(c);
			return getTok(st);
		}
		if (c == 'r'
			 && (/*(k == '\'' && tokSw_[T_BSQUOT])||*/
			     (k == '"' && tokSw_[T_WQUOT] && tokSw_[T_BSQUOT]))
			 && mode_ == MODE_NORM)
		{
			// r�̒����'��"������� D�����r"" r''�Ƃ��ď����B
			// ��r�Ƃ̊Ԃɋ󔒂͒u���Ȃ����@�Ȃ�ŋC�ɂ��Ȃ�^^;
			st += char(c);
			str_r_mode_ = 1;
			tok_t tt = getTok(st);
			str_r_mode_ = 0;
			return tt;
		}
		ungetC(c);
		getName(st);				// ���O���擾
		//cout << "[" << st << "]" << endl;
		tok = chkName2Tok(st, tok);	// ���O�����ʂ��擾
		break;

	case	T_SLA:		// / �R�����g��
		if (isSym(c) && lineCmtChr_ == 0) {
			tok = chkGetCmt(st);
		} else {
			st += char(c);
			tok   = chkSymTok(c, T_SYM);
		}
		break;

	case T_ASTA:		// *
		st += char(c);
		if (mode_ == MODE_BLKCMT && isSym(c)) {
			tok = T_NONE;
			c = getC();
			if (c == '/') {
				// */ �Ńu���b�N�R�����g�̏I��肾����
				st += char(c);
				mode_ = MODE_NORM;
			} else {
				ungetC(c);
			}
		} else {
			tok   = chkSymTok(c, T_SYM);
		}
		break;

	case T_PLUS:		// +
		st += char(c);
		if (mode_ == MODE_BLKCMTD && isSym(c)) {
			tok = T_NONE;
			c = getC();
			if (c == '/') {
				// +/ �Ńu���b�N�R�����g�̏I��肾����
				st += char(c);
				mode_ = MODE_NORM;
			} else {
				ungetC(c);
			}
		} else {
			tok   = chkSymTok(c, T_SYM);
		}
		break;

	default:
		//CERR << "[PRGERR]getTok\n";
		;
	}
	// �L���́A�F�ݒ肪�Ȃ�������A�ʏ핶������
	if (tok == T_SYM && tokSw_[tok] == false)
		tok = T_NONE;
	if (tok == T_MEMB && tokSw_[tok] == false)
		tok = T_NONE;
	// �W�i���́A�F�ݒ肪�Ȃ�������A�ʏ�̐��l����
	if (tok == T_OVAL && tokSw_[tok] == false)
		tok = T_IVAL;
	// ���l�́A�F�ݒ肪�Ȃ�������A�ʏ핶������
	if (tok == T_IVAL && tokSw_[tok] == false)
		tok = T_NONE;
	return tok;
}


/// \ �G�X�P�[�v�V�[�P���X�̏���
///
void ConvCpp2Htm::chkGetYenEsc(string &st)
{
	// ������Ɠ��ʂȂ�ŁA�\�߁A�F�Â������Ⴄ���j
	int c = getC();
	ungetC(c);
	tok_t k = T_ESCX;
	if (tokSw_[k] == false)	// �ݒ肳��ĂȂ�������ʏ�Ɠ����悤�ɓ��ꎋ
		k = T_ESC;
	if (c == '\0' || c == '\n') {
		// �s����\�͂��Ɠ���Ȃ�ŐF�t��
		st += mrkBgn_[k];
		st += char('\\');
		st += mrkEnd_[k];
		escEolFlg_ = true;		// �s���� \ ���������Ƃ����s�ɋ�����
	} else if (mode_ == MODE_NORM) {
		// �\�[�X���Ɍ��ꂽ \ �������ꍇ
		st += mrkBgn_[k];
		st += char('\\');
		getEsc(st);
		st += mrkEnd_[k];
	} else if (mode_ == MODE_STR || mode_ == MODE_CVAL /*|| mode_ == MODE_RSTR*/) {
		// ������, �����萔���������ꍇ
		st += mrkBgn_[T_ESC];
		st += char('\\');
		getEsc(st);
		st += mrkEnd_[T_ESC];
	} else {
		// �R�����g���������ꍇ... �F����ł������A��
		// D����ł�`������`�������ĐF����K�v�Ȃ��A�Ƃ��Ƃ���
		st += char('\\');
	}
}


/// '#' �P��̃`�F�b�N.
/// @param name	�擾��������������ĕԂ�
/// @return		�g�[�N��
ConvCpp2Htm::tok_t ConvCpp2Htm::chkGetSharpName(string &st)
{
	string	spc("");

	// #�ƒP��̊Ԃɂ͋󔒂������Ă����̂ŁA������擾
	for (;;) {
		int c = getC();
		if (c == 0 || c > ' ') {
			ungetC(c);
			break;
		}
		spc += char(c);
	}
	getName(st);

	string nm('#'+st);
	st  = '#' + spc + st;
	return chkName2Tok(nm, T_NAME);
}


/// ���[�h�Ɩ��O����A����̃g�[�N��������
/// @param name	�`�F�b�N���閼�O
/// @return		�g�[�N��
ConvCpp2Htm::tok_t ConvCpp2Htm::chkName2Tok(const string &name, ConvCpp2Htm::tok_t tok)
{
	if (mode_ == MODE_NORM) {
	  #if 0	// . -> .* ->* �̒���̖��O�͐F�Â��Ώۂɂ��Ȃ��ꍇ�́A���̔���𕜊�������
	  		// �c�c���A�F�������ق������ǖ��c���̂��߂ɂ͂悢��ł͂Ȃ����ƁB
		if (bak_tok_ != T_MEMB)
	  #endif
		{
			WordTbl::iterator wd = wordTbl_.find(name);
			if (wd != wordTbl_.end())
				tok = wd->second;
		}
	} else {
		tok = T_NONE;
	}
	tok   = (mode_ == MODE_NORM) ? tok : T_NONE;
	return tok;
}


/// '/'�Ŏn�܂�R�����g�̏���
/// @param st	�擾��������������ĕԂ�
/// @return		�g�[�N��
ConvCpp2Htm::tok_t ConvCpp2Htm::chkGetCmt(string &st)
{
	tok_t	tok;	//=T_SLA;
	int		c;

	st += char('/');
	if (mode_ == MODE_NORM) {
		c = getC();
		if (c == '/') {
			// //�R�����g������
			st      += char(c);
			tok      = T_CMT;
			mode_    = MODE_CMT1;
			cmt_typ_ = CMT_NORM;
			c = getC();
			if (c == '/' && tokSw_[T_CMTJ]) {
				st += char(c);
				c = getC();
				if (c != '/') {
					// /// �� doxygen(jdoc�`��) �Ƃ��Ƃ�
					tok      = T_CMTJ;
					cmt_typ_ = CMT_DOXY;
				}
				ungetC(c);
			} else if (c == '!' && tokSw_[T_CMTJ]) {
				// //! �� doxygen(qt�`��) �Ƃ��Ƃ�
				st 	    += char(c);
				tok      = T_CMTJ;
				cmt_typ_ = CMT_DOXY;
			} else if (tokSw_[T_CMTEX1] && c == cmtExChr_[0] && c) {
				if (c)
					st  += char(c);
				tok      = T_CMTEX1;
				cmt_typ_ = CMT_EX1;
			} else if (tokSw_[T_CMTEX2] && c == cmtExChr_[1] && c) {
				if (c)
					st  += char(c);
				tok      = T_CMTEX2;
				cmt_typ_ = CMT_EX2;
			} else if ((c != ' ' && c != '\t' && c != '\0' && c != '\n') && tokSw_[T_CMTK]) {
				// // �̒���ɋ󔒂��Ȃ��ꍇ
				if (c)
					st += char(c);
				tok      = T_CMTK;
				cmt_typ_ = CMT_NOSPC;
			} else {
				ungetC(c);
			}
		} else if (c == '*') {
			// /* �R�����g������
			st += char(c);
			tok      = T_CMT;
			mode_    = MODE_BLKCMT;
			cmt_typ_ = CMT_NORM;
			c = getC();
			if (c == '*' && tokSw_[T_CMTJ]) {
				st += char(c);
				c = getC();
				if (c != '*') {
					// /** �� doxygen(jdoc�`��) �Ƃ��Ƃ�
					tok      = T_CMTJ;
					cmt_typ_ = CMT_DOXY;
				}
				ungetC(c);
			} else if (c == '!' && tokSw_[T_CMTJ]) {
				// /*! �� doxygen(qt�`��) �Ƃ��Ƃ�
				st += char(c);
				tok      = T_CMTJ;
				cmt_typ_ = CMT_DOXY;
			} else if ((c != ' ' && c != '\t' && c != '\0' && c != '\n') && tokSw_[T_CMTK]) {
				// /* �̒���ɋ󔒂��Ȃ��ꍇ
				if (c)
					st += char(c);
				tok      = T_CMTK;
				cmt_typ_ = CMT_NOSPC;
			} else {
				ungetC(c);
			}
		} else if (c == '+' && tokSw_[T_CMTD]) {
			// /+ �R�����g������
			st += char(c);
			tok      = T_CMTD;
			mode_    = MODE_BLKCMTD;
			cmt_typ_ = CMT_D;
			c = getC();
			if (c == '+' && tokSw_[T_CMTJ]) {
				st += char(c);
				c = getC();
				if (c != '+') {
					// /++ �ŉR������ doxygen�����ɂ���
					tok      = T_CMTJ;
					cmt_typ_ = CMT_DOXY;
				}
				ungetC(c);
			} else if (c == '!' && tokSw_[T_CMTJ]) {
				// /+! �ŉR������ doxygen�����ɂ���
				st += char(c);
				tok      = T_CMTJ;
				cmt_typ_ = CMT_DOXY;
			} else {
				ungetC(c);
			}
		} else {
			ungetC(c);
			tok = T_SYM;
		}
	} else {
		tok = T_NONE;
	}
	return tok;
}


/// �P��̎擾(�S�p������Ƃ����܂Ƃ߂�)
/// @param st	�擾��������������ĕԂ�
void ConvCpp2Htm::getName(std::string &st)
{
	for (;;) {
		int c = getC();
		tok_t tok = chTypTbl_[c];
		if (tok != T_NAME && tok != T_IVAL && tok != T_OVAL /*&& tok != T_SHARP*/) {
			ungetC(c);
			break;
		}
	  #if 1
		if (useUtf8_) {
			if (c < 0x80) {
				st += char(c);
			} else {
				int n = (c < 0xC0) ? 1-1
					  : (c < 0xE0) ? 2-1
					  : (c < 0xF0) ? 3-1
					  : (c < 0xF8) ? 4-1
					  : (c < 0xFC) ? 5-1
					  :              6-1;
				st += char(c);
				for (int i = 0; i < n; i++) {
					c = getC();
					if (c == 0) {
						//cout << "bad utf8!\n";
						break;
					}
					st += char(c);
				}
			}
		} else {
			if (ISKANJI(c)) {
				int c2 = getC();
				if (c2 == 0) {
					break;
				}
				st += char(c);
				c   = c2;
			}
			st += char(c);
		}
	  #endif
	}
}


/// ���l������.
/// ����͂킴�ƊÂ����Ă���B�����Ŏn�܂薼�O�����������Ԃ͂���ƌ��􂷁B
/// @param st	�擾��������������ĕԂ�
/// @param tok	�����tok
/// @return		��͌��ʁA�ύX�ƂȂ���tok
ConvCpp2Htm::tok_t ConvCpp2Htm::getVal(std::string &st, ConvCpp2Htm::tok_t tok)
{
	int b = 0;

	for (;;) {
		int c = getC();
		tok_t t = chTypTbl_[c];
		if (c == '.') {
			t = T_IVAL;
			tok = t;
		}
		if (c == 'x' || c == 'X')
			tok = T_IVAL;
		if ((b == 'e' || b == 'E' || b == 'p' || b == 'P') && (c == '+' || c == '-')) {
			t = T_IVAL;
			tok = t;
		}
		if (c >= 0x7F || (t != T_NAME && t != T_IVAL && t != T_OVAL)) {
			ungetC(c);
			break;
		}
		st += char(c);
		b = c;
	}
	return tok;
}


/// \�G�X�P�[�v�����̏���
/// @param st	�擾��������������ĕԂ�
void ConvCpp2Htm::getEsc(std::string &st)
{
	int l, i;
	int c = getC();
	switch (c) {
	case 'a':		// abvntfr\'"
	case 'b':
	case 'v':
	case 'n':
	case 't':
	case 'f':
	case 'r':
	case '\\':
	case '\'':
	case '"':
	case '?':
		st += char(c);
		break;

	case 'x':
	case 'u':
	case 'U':
		l = (c == 'x') ? 2 : (c == 'u') ? 4 : 8;
		for (i = 0; i < l; i++) {
			c = getC();
			if (isxdigit(c)) {
				st += char(c);
			} else {
				ungetC(c);
				break;
			}
		}
		break;

  #if 0	// perl�֌W
	case 'e':
	case 'l':
	case 'u':
	case 'L':
	case 'U':
	case 'E':
	//case 'c':
		st += char(c);
		break;
  #endif

	default:
		if (isdigit(c)) {
			st += char(c);
			for (i = 0; i < 2; i++) {
				c = getC();
				if (isdigit(c)) {
					st += char(c);
				} else {
					ungetC(c);
					break;
				}
			}
		} else {
			// �A���m�E���́A�F���Ȃ��A���A�e�����󂯂邱�Ƃ�
			// �������߂ɁA�F�������ق����悳��
			//ungetC(c);
			st += char(c);
		}
		break;
	}
}



/// �󔒂��擾
///
void ConvCpp2Htm::getSpc(string &st)
{
	int c;
	for (;;) {
		c = getC();
		if (c == 0 || c > ' ')
			break;
		st += char(c);
	}
	ungetC(c);
}


// ------------------------------------------------------------
// �������֌W

/// �R���X�g���N�^
///
ConvCpp2Htm::ConvCpp2Htm()
{
	srcPtr_     = 0;
	mode_		= MODE_NORM;
	cur_tok_	= T_NONE;
	cmt_typ_	= CMT_NORM;
	useUtf8_	= false;
	escEolFlg_	= false;
	str_r_mode_ = false;
	lineCmtChr_ = 0;
	tokStr_.clear();
	tokStr_.resize(0x1000);
	initChTop(std::string(""), std::string(""));
	//initTbl();
	initMrkBgnEnd();
	for (int i = 0; i < T_NUM;i++)
		tokSw_[i] = false;
}


/// �������ʗp�e�[�u���̏�����
/// @param	symChrs		�L�������̕���
/// @pram 	nameSymChrs	���O��������L������.
void ConvCpp2Htm::initChTop(const std::string &symChrs, const std::string &nameSyms)
{
	int i;

	symChrs_  = symChrs;
	nameSyms_ = nameSyms;

	chTypTbl_[0] = T_ZERO;
	for (i = 1; i < 0x100; i++)
		chTypTbl_[i] = T_NONE;

	for (i = '0'; i <= '9'; i++)
		chTypTbl_[i] = T_IVAL;
	chTypTbl_['0']   = T_OVAL;
	//
	for (i = 'A'; i <= 'Z'; i++)
		chTypTbl_[i] = T_NAME;

	for (i = 'a'; i <= 'z'; i++)
		chTypTbl_[i] = T_NAME;

	//if (useUtf8_) {
		for (i = 0x80; i <= 0xFF; i++)	// utf8�̏ꍇ...
			chTypTbl_[i] = T_NAME;
	//} else {
		//for (i = 0x81; i <= 0xFC; i++)	// sjis�̏ꍇ...
			//chTypTbl_[i] = T_NAME;
	//}
	//chTypTbl_['_']  = T_NAME;
	//chTypTbl_['$']  = T_NAME;
	//chTypTbl_['@']  = T_NAME;
	for (i = 0; i < int(nameSyms.size()); i++) {
		chTypTbl_[nameSyms[i]] = T_NAME;
	}
	// �w�肳�ꂽ�L��������o�^
	for (i = 0; i < int(symChrs.size()); i++) {
		chTypTbl_[symChrs[i]] = T_SYM;
	}
	//
	chTypTbl_['&']  = T_AMP;
	chTypTbl_['<']  = T_LT;
	chTypTbl_['>']  = T_GT;
	chTypTbl_['"']  = T_WQUOT;
	chTypTbl_['\''] = T_SQUOT;
	chTypTbl_['`']  = T_BSQUOT;
	chTypTbl_['\\'] = T_ESC;
	chTypTbl_['/']  = T_SLA;
	chTypTbl_['*']  = T_ASTA;
	chTypTbl_['#']  = T_SHARP;
	chTypTbl_['.']  = T_PERIOD;
	chTypTbl_['-']  = T_MINUS;
	chTypTbl_['+']  = T_PLUS;
}



/// HTML �ł̐F�t���̂��߂̃e�[�u��������
///
void ConvCpp2Htm::initMrkBgnEnd()
{
	mrkBgn_.clear();
	mrkEnd_.clear();
	for (int i = T_NONE; i <= T_NUM; i++) {
		mrkBgn_[tok_t(i)] = string("");
		mrkEnd_[tok_t(i)] = string("");
	}
	for (mrkTbl_t *m = mrkTbl_; m->bgn; m++) {
		mrkBgn_[m->tok] = m->bgn;
		mrkEnd_[m->tok] = m->end;
	}
}


/// �F/�t�H���g��ݒ肷��HTML������
ConvCpp2Htm::mrkTbl_t	ConvCpp2Htm::mrkTbl_[] = {
	{T_SYM,		"<span class=C_SY>",		"</span>"},
	{T_WQUOT,	"<span class=C_STR>",		"</span>"},
	{T_SQUOT,	"<span class=C_CH>",		"</span>"},
	{T_BSQUOT,	"<span class=C_RSTR>",		"</span>"},
	{T_ESC,		"<span class=C_ESC>",		"</span>"},
	{T_ESCX,	"<span class=C_ESCX>",		"</span>"},
	{T_IVAL,	"<span class=C_VAL>",		"</span>"},
	{T_OVAL,	"<span class=C_OCT>",		"</span>"},
	{T_CMT,		"<span class=C_CMT>",		"</span>"},
	{T_CMTJ,	"<span class=C_CMTJ>",		"</span>"},
	{T_CMTK,	"<span class=C_CMTK>",		"</span>"},
	{T_CMTD,	"<span class=C_CMTD>",		"</span>"},
	{T_CMTEX1,	"<span class=C_CMTEX1>",	"</span>"},
	{T_CMTEX2,	"<span class=C_CMTEX2>",	"</span>"},
	{T_SHARP,	"<strong class=C_SH>",		"</strong>"},
	{T_WORD1,	"<strong class=C_W1>",		"</strong>"},
	{T_WORD2,	"<strong class=C_W2>",		"</strong>"},
	{T_WORD3,	"<strong class=C_W3>",		"</strong>"},
	{T_WORD4,	"<strong class=C_W4>",		"</strong>"},
	{T_WORD5,	"<span class=C_W5>",		"</span>"},
	{T_WORD6,	"<span class=C_W6>",		"</span>"},
	{T_WORD7,	"<span class=C_W7>",		"</span>"},
	{T_WORD8,	"<span class=C_W8>",		"</span>"},
	{T_WORD9,	"<span class=C_W9>",		"</span>"},
	{T_NONE,	0,	0},
};



/// �����tok���ANONE�̂Ƃ��A���ꎋ���邩�ۂ��̃`�F�b�N�e�[�u��
bool ConvCpp2Htm::nextTokNoneOkTbl[T_NUM+1] = {
	true,		// T_NONE
	false,		// T_ZERO
	false,		// T_SYM
	true,		// T_WQUOT
	true,		// T_SQUOT
	true,		// T_BSQUOT
	false,		// T_ESC
	false,		// T_ESCX
	false,		// T_IVAL
	false,		// T_OVAL
	true,		// T_CMT
	true,		// T_CMTJ
	true,		// T_CMTK
	true,		// T_CMTD
	true,		// T_CMTEX1
	true,		// T_CMTEX2
	false,		// T_SHARP
	false,		// T_WORD1
	false,		// T_WORD2
	false,		// T_WORD3
	false,		// T_WORD4
	false,		// T_WORD5
	false,		// T_WORD6
	false,		// T_WORD7
	false,		// T_WORD8
	false,		// T_WORD9
	false,		// T_NAME
	false,		// T_MEMB
	false,		// T_LF
	false,		// T_SLA
	false,		// T_ASTA
	false,		// T_AMP
	false,		// T_LT
	false,		// T_GT
	false,		// T_PERIOD
	false,		// T_MINUS
	false,		// T_PLUS
	false,		// T_NUM
};


/* ---------------------------------------------------------------------------
 *  �����F
 *  	2003-07-23 �ꉞ�A����Ȃ�Ȃ��̂������B���\�C���悵�B
 *		�Ń\�[�X�������ĂāA���R���p�C�������炱�̃t�@�C�����Ȃ��ƃG���[.
 *		.bak ���e�X�g�Ŏg�������̃t�@�C���̃R�s�[�Ȃǂ��ꗥ�폜��������ŁA
 *		�t�@�C��������htm�̕������g���q�ƌ��ԈႦ�č폜�����͗l(T T)
 *		�S���З؂��c���ĂȂ�����...�ŁA���ǁA�v���o���č�蒼���B
 *		�o�В��O�ꎞ�Ԃő�؏����o����(���\�o���Ă����)�A���f�o�b�O�ŕ���.
 *		2003-08-08 \0���܂� \+8�i���ɂ����āA�ŏ��̐������A�o�͂��Y��Ă��̂��C��
 *		2003-08-09 �L�������̐ݒ���O������̐ݒ�ɕύX.
 *		�g�[�N����ʂ̔����on/off ����ʂ�\�ɁB
 *		c�`���ȊO�̈�s�R�����g�̐ݒ���\��(�蔲����)
 *		2003-08-18 '.'���L�������ɂ��Ă��Ȃ��Ƃ��A�o�͂��猇���Ă��܂����̂��C��
 */
