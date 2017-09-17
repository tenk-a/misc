/*
  unsigned char *jstrcnv(js1,js2,opts)
     char      * js1; 変換された文字列を格納するバッファへのポインタ
     char const* js2; 元の文字列
     short  	 opts; 変換方法

    文字列js2をoptsで指定された変換方法で変換し、結果をjs1に格納します。
  値としてjs1を返します。ただし、エラーがあったときはNULLを返します。
    js1は結果を格納するのに十分なサイズが必要です。
    記号は半角、全角で一対一に対応した変換を行います。

    全角を半角にするとき、濁音・半濁音文字は分解して変換します。
  たとえば 'ダ' なら' ﾀﾞ' というふうにします。
    また半角ｶﾅ を全角に変換するとき、濁音・半濁音付きの文字で全角に対応する
  文字があれば、その全角文字に変換します。'ﾀﾞ'なら'ダ'というふうになります。

    濁音(゛)・半濁音(゜)・長音符号(ー)・なか点（・）は、全角のばあい、カタ
  カナの後ろにあればカタカナとして、ひらがなの後ろにあればひらがなとして、
  そうでなければカナ記号として扱います。また、半角ならば、ｶﾅの後ろにあれば
  ｶﾅ として、でなければ ｶﾅ記号として扱います。
  たとえば全角カタカナの半角化のとき、"すたー・ウォーズ"なら"すたー・ｳｫｰｽﾞ"
　に、"スター・うぉーず" なら "ｽﾀｰ･うぉーず" になります。

    変換できない全角文字（ようするに2ﾊﾞｲﾄ目が不正なもの）があったばあい、
  文字によって次のような処理をします。
  2ﾊﾞｲﾄ目が、
    '\0'の場合:
       1byte目も'\0'にする。
    表示不可能な文字（0x01～0x08,0x0a～0x1f,0x7f,0xfd～0xff）の場合:
       その不正な文字(2ﾊﾞｲﾄ)をとばして処理を続けます。
       復帰値としてNULLを返します。
    表示可能な文字(0x09,0x20～0x3f)の場合:
       その1ﾊﾞｲﾄ目のみとばして処理を続けます。
　　　　復帰値としてNULLを返します。

    変換方法は JTOPUNS|JTODGT のように'|'でつなげて指定してください。

  opts の値
    JTOPUNS  (0x0001)  全角記号を半角に変換します（空白も変換します）
    JTODGT   (0x0002)  全角数字を半角に変換します
    JTOALPH  (0x0004)  全角アルファベットを半角にします
    JTOKPUN  (0x0008)  全角カナ記号を半角に変換します
    JTOKATA  (0x0010)  全角カタカナを半角ｶﾅに変換します
    JTOHIRA  (0x0020)  全角ひらがなを半角ｶﾅに変換します
    JSPC2SPC (0x0040)  全角記号を半角に変換するとき、全角空白を半角空白２
    	    	   　　文字に変換します
    YENOFF   (0x0080)  \ に対応する全角が ￥ でなく ＼ にする

    TOJPUNS  (0x4100)  半角記号を全角に変換します(空白も変換します)
    TOJPUN   (0x0100)  半角記号を全角に変換します(空白は変換しません！)
    TOJDGT   (0x0200)  半角数字を全角に変換します
    TOJALPH  (0x0400)  半角ｱﾙﾌｧﾍﾞｯﾄを全角に変換します
    TOJKPUN  (0x0800)  半角ｶﾅ記号を全角に変換します
    TOJKATA  (0x1000)  半角ｶﾅを全角カタカナに変換します
    TOJHIRA  (0x2000)  半角ｶﾅを全角ひらがなに変換します
    TOJSPC   (0x4000)  半角空白を全角に変換します
    DAKUOFF  (0x8000)  半角ｶﾅの全角化で濁音・半濁音文字への変換を抑制し、
    	    	   濁音・半濁音をたえず１文字として扱います

   * TOJKATA と TOJHIRA が同時に指定されたばあいは TOJKATA が優先されます。

   * 1991/06/20 Writen by M.Kitamura
   * 1991/08/23 Debug:全角数字の半角化の判定ﾐｽ。('&&'と'&'の打ち間違いが2つ...)
   * 1992/10/25 Debug:’(8166h)と‘(8165h)を半角にするとき逆に変換していた
    	    	Debug:｜(8162h)が半角にできなかった.
    	    	\ に対応する全角が ￥ か ＼ かを指定できるようにした
   * 1995/12/30 os9/09(K&R)対応をやめ、ANSI-Cｽﾀｲﾙにする
   * memo: テーブルよりプログラムの方が小さくなる8bitCPU 向けに作ったもので
    	   今ならもっとテーブルで済ませてる...
 */

#include <stddef.h>
#include "jstr.h"

typedef unsigned char UCHAR;
#define iskanji2(c) ((UCHAR)(c) >= 0x40 && (UCHAR)(c) <= 0xfc && (UCHAR)(c) != 0x7f)

/***/
char *jstrcnv(char *jstr1, char const* jstr2, unsigned opts)
{
    static UCHAR const tojkigo[] = {
    	0x49,0x68,0x94,0x90,0x93, /* ！”＃＄％ */
    	0x95,0x66,0x69,0x6a,0x96, /* ＆’（）＊ */
    	0x7b,0x43,0x7c,0x44,0x5e, /* ＋，―．／ */
    	0x46,0x47,0x83,0x81,0x84, /* ：；＜＝＞ */
    	0x48,0x97,  	    	  /* ？＠   	*/
    	    	  0x6d,0x8f,0x6e, /*	 ［￥］ */
    	0x4f,0x51,0x65,     	  /* ＾＿‘ 	*/
    	    	       0x6f,0x62, /*	   ｛｜ */
    	0x70,0x50,  	    	  /* ｝￣   	*/
    	    	  0x42,0x75,0x76, /*	 。「」 */
    	0x41,	    	    	  /* 、     	*/
    	     0x45,  	    	  /*  ・    	*/
    	    	  0x4A,0x4B,	  /*	゛゜	*/
    	    	    	    0x5f  /*	     ＼ */
    };
    static UCHAR const jtokigo[] = {
    	      ' ', 0xA4/*'､'*/, 0xA1/*'｡'*/, ',', '.', 0xA5/*'･'*/, ':',
    	 ';', '?', '!', 0xDE/*'ﾞ'*/, 0xDF/*'ﾟ'*/,0x00,0x00,0x00,
    	 '^', '~', '_',0x00,0x00,0x00,0x00,0x00,
    	0x00,0x00,0x00,0x00, 0xB0/*'ｰ'*/,0x00,0x00, '/',
    	0x00,0x00,0x00, '|',0x00,0x00,0x60,'\'',
    	0x00,'\"', '(', ')',0x00,0x00, '[', ']',
    	'{' ,'}' ,0x00,0x00,0x00,0x00, 0xA2/*'｢'*/, 0xA3/*'｣'*/,
    	0x00,0x00,0x00,0x00, '+', '-',0x00,0x00,0x00,
    	0x00, '=',0x00, '<', '>',0x00,0x00,0x00,
    	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0/*'\\'*/,
    	 '$',0x00,0x00, '%', '#', '&', '*', '@'
    };
    static UCHAR const tojkana[] = {
    	0x51,0x00,0x02,0x04,0x06,0x08,	/* ｦ */
    	0x42,0x44,0x46,0x22,0x00,   	/* ｬ */
    	0x01,0x03,0x85,0x07,0x09,   	/* ｱ */
    	0x8a,0x8c,0x8e,0x90,0x92,   	/* ｶ */
    	0x94,0x96,0x98,0x9a,0x9c,   	/* ｻ */
    	0x9e,0xA0,0xA3,0xA5,0xA7,   	/* ﾀ */
    	0x29,0x2a,0x2b,0x2c,0x2d,   	/* ﾅ */
    	0xAe,0xB1,0xB4,0xB7,0xBa,   	/* ﾊ */
    	0x3d,0x3e,0x3f,0x40,0x41,   	/* ﾏ */
    	0x43,0x45,0x47,     	    	/* ﾔ */
    	0x48,0x49,0x4a,0x4b,0x4c,   	/* ﾗ */
    	0x4e,0x52,0x00	    	    	/* ﾜ */
    };
    static UCHAR const jtokana[] = {
    	0x01,0x0b,0x02,0x0c,0x03,0x0d,0x04,0x0e,0x05,0x0f,  	/* あ */
    	0x10,0x90,0x11,0x91,0x12,0x92,0x13,0x93,0x14,0x94,  	/* か */
    	0x15,0x95,0x16,0x96,0x17,0x97,0x18,0x98,0x19,0x99,  	/* さ */
    	0x1a,0x9a,0x1b,0x9b,0x09,0x1c,0x9c,0x1d,0x9d,0x1e,0x9e, /* た */
    	0x1f,0x20,0x21,0x22,0x23,   	    	    	    	/* な */
    	0x24,0xA4,0x64,0x25,0xA5,0x65,0x26,0xA6,0x66,	    	/* は */
    	0x27,0xA7,0x67,0x28,0xA8,0x68,
    	0x29,0x2a,0x2b,0x2c,0x2d,   	    	    	    	/* ま */
    	0x06,0x2e,0x07,0x2f,0x08,0x30,	    	    	    	/* や */
    	0x31,0x32,0x33,0x34,0x35,   	    	    	    	/* ら */
    	0xff,0x36,0xff,0xff,0x00,0x37,0x8d
    };
    UCHAR   	 f;
    UCHAR   	 err_f;
    UCHAR * 	 js1;
    UCHAR const* js2;
    UCHAR   	 d;
    UCHAR   	 c;
    unsigned	 jc;

    err_f = f = 0;
    js1 = (UCHAR*)jstr1;
    js2 = (UCHAR const*)jstr2;
    while ( (jc = *(js2++)) != '\0') {
    	if (jc <= 0x80) {   	    	/* 半角文字のとき */
    	    f = 0;
    	    if (jc <= 0x1f) {	    	/* ｺﾝﾄﾛｰﾙ･ｺｰﾄﾞ */
    	    	;
    	    } else if (jc == 0x20) {	/* ｽﾍﾟｰｽ */
    	    	if (opts & TOJSPC)
    	    	    jc = 0x8140;
    	    } else if (jc <= 0x2f) {	/* 記号 */
    	    	c = '!';
    	    	goto KIGO;
    	    } else if (jc <= '9') { 	/* 数字 */
    	    	if (opts & TOJDGT)
    	    	    jc += 0x824f - '0';
    	    } else if (jc <= '@') { 	/* 記号 */
    	    	c  = ':' - 15;
    	    	goto KIGO;
    	    } else if (jc <= 'Z') { 	/* ｱﾙﾌｧﾍﾞｯﾄの大文字 */
    	    	if (opts & TOJALPH)
    	    	    jc += 0x825f - '@';
    	    } else if (jc <= 0x60) {	/* 記号 */
    	    	c = '[' - 22;
    	    	if (jc == '\\' && (opts & YENOFF))
    	    	    c = '\\' - 39;
    	    	goto KIGO;
    	    } else if (jc <= 'z') { 	/* ｱﾙﾌｧﾍﾞｯﾄの小文字 */
    	    	if (opts & TOJALPH)
    	    	    jc += 0x8280 - 0x60;
    	    } else if (jc <= 0x7e) {	/* 記号 */
    	    	c = '{' - 28;
KIGO:
    	    	if (opts & TOJPUN)
    	    	    jc = 0x8100 + tojkigo[jc - c];
    	    }
    	} else {
    	    if (jc <= 0x9f) {	    	/* ｼﾌﾄJIS1ﾊﾞｲﾄ目 */
    	    	goto SJIS;
    	    } else if (jc == 0xa0) {
    	    	;
    	    } else if (jc <= 0xa4) {	/* 半角仮名記号 */
    	    	if (opts & TOJKPUN)
    	    	    jc = 0x8100 + tojkigo[jc - 0xA1/*'｡'*/ + 32];
    	    } else if (jc == 0xa5 || jc == 0xb0) {/* 半角なかてん||(ｰ) */
    	    	if (	(f == 1 && (opts & (TOJKATA|TOJHIRA)))
    	    	    || (f != 1 && (opts & TOJKPUN)) 	   )
    	    	{
    	    	    jc = (jc == 0xa5) ? 0x8145 : 0x815b;
    	    	}
    	    } else if (jc <= 0xdd) {	/* 半角ｶﾅ */
    	    	f = 1;
    	    	if (opts & (TOJKATA|TOJHIRA)) {
    	    	    c = tojkana[jc - 0xA6/*'ｦ'*/];
    	    	    d = c & 0x7f;
    	    	    if (*js2 == 0xDE/*'ﾞ'*/ && (c & 0x80)) {
    	    	    	++js2;
    	    	    	if (jc == 0xB3/*'ｳ'*/ && (opts & TOJKATA)
    	    	    	    && ((opts & DAKUOFF) == 0)	 ) {
    	    	    	    jc = 0x8394;
    	    	    	    goto KJ1;
    	    	    	} else
    	    	    	    ++d;
    	    	    } else if (*js2 == 0xDF/*'ﾟ'*/
    	    	    	&& (jc >= 0xCA/*'ﾊ'*/ && jc <= 0xCE/*'ﾎ'*/)
    	    	    	&& ((opts & DAKUOFF) == 0)   ) {
    	    	    	++js2;
    	    	    	d += 2;
    	    	    }
    	    	    if (opts & TOJKATA) {
    	    	    	if (d > 0x3e)
    	    	    	    ++d;
    	    	    	jc = d + 0x8340;
    	    	    } else {
    	    	    	jc = d + 0x829f;
    	    	    }
    	    	}
    	    } else if (jc <= 0xdf) {	/* 半角濁音・半濁音 */
    	    	if ((f==1 && (opts&(TOJKATA|TOJHIRA))) || (opts&TOJKPUN) )
    	    	    jc = 0x8100 + tojkigo[jc - 0xDE/*'ﾞ'*/ + 37];
    	    } else if (jc <= 0xfc) {	/* ｼﾌﾄJIS */
    	 SJIS:
    	    	if ((c = *(js2++)) == '\0') 	    /* 2ﾊﾞｲﾄ目が'\0' */
    	    	    break;/* exit while */
    	    	else if (iskanji2(c) == 0) {	    /* 2ﾊﾞｲﾄ目が不正のとき*/
    	    	    f = 2;
    	    	    err_f = 1;
    	    	    if (c < 0x08)
    	    	    	continue;

    	    	    else if (c == 0x09)
    	    	    	goto ERR1;
    	    	    else if (c <= 0x1f)
    	    	    	continue;
    	    	    else if (c <= 0x3f) {
    	    	 ERR1:
    	    	    	--js2;
    	    	    	continue;
    	    	    } else if (c == 0x7f || c >= 0xfd)
    	    	    	continue;
    	    	}
    	    	if (jc == 0x81) {   	/* 全角記号 */
    	    	    if (c == 0x40 && (opts & JSPC2SPC)) {
    	    	    	f = 2;
    	    	    	*(js1++) = ' ';
    	    	    	jc = ' ';
    	    	    } else if(c==0x5b||c==0x4a||c==0x4b||c==0x45) {
    	    	    	/*  	ー  	゛  	    ゜	    ・	 */
    	    	    	if ( (f == 3 && (opts & JTOHIRA))
    	    	    	    ||(f == 4 && (opts & JTOKATA))
    	    	    	    ||(f != 3 && f != 4 && (opts & JTOKPUN)) )
    	    	    	{
    	    	    	    goto JKIGO;
    	    	    	} else {
    	    	    	    goto JCC;
    	    	    	}
    	    	    } else if(c==0x41||c==0x42||c==0x75||c==0x76) {
    	    	    	/*  	、  	。  	    「	    」	*/
    	    	    	if (opts & JTOKPUN)
    	    	    	    goto JKIGO;
    	    	    	else
    	    	    	    goto JCC;
    	    	    } else if ( (c <= 0x97 && (opts & JTOPUNS)) ) {
    	    	    	f = 2;
    	    	    	if (opts & YENOFF) {
    	    	    	    if (c == 0x5f) { /*＼*/
    	    	    	    	jc = '\\';
    	    	    	    	goto KJ1;
    	    	    	    }
    	    	    	} else {
    	    	    	    if (c == 0x8f) { /*￥*/
    	    	    	    	jc = '\\';
    	    	    	    	goto KJ1;
    	    	    	    }
    	    	    	}
    	    	 JKIGO:
    	    	    	if ((d = jtokigo[c - 0x40]) == 0x00)
    	    	    	    goto JCC;
    	    	    	jc = d;
    	    	    } else
    	    	    	goto JCC;
    	    	} else if (jc == 0x82) {
    	    	    if (c >= 0x4f && c <= 0x58) {    /* 全角数字 */
    	    	    	f = 2;
    	    	    	if (opts & JTODGT)
    	    	    	    jc = c - 0x4f + '0';
    	    	    	else
    	    	    	    goto JCC;
    	    	    } else if (c >= 0x60 && c <= 0x79) {/* 全角ｱﾙﾌｧﾍﾞｯﾄ */
    	    	    	f = 2;
    	    	    	if (opts & JTOALPH)
    	    	    	    jc = c - 0x60 + 'A';
    	    	    	else
    	    	    	    goto JCC;
    	    	    } else if (c >= 0x81 && c <= 0x9a) {/* 全角ｱﾙﾌｧﾍﾞｯﾄ */
    	    	    	f = 2;
    	    	    	if (opts & JTOALPH)
    	    	    	    jc = c - 0x81 + 'a';
    	    	    	else
    	    	    	    goto JCC;
    	    	    } else {
    	    	    	f = 3;
    	    	    	if (c >= 0x9f && c <= 0xf1) { /* ひらがな */
    	    	    	    if (opts & JTOHIRA) {
    	    	    	    	d = c - 0x9f;
    	    	    	    	goto JKANA;
    	    	    	    } else
    	    	    	    	goto JCC;
    	    	    	} else
    	    	    	    goto JCC;
    	    	    }
    	    	} else if (jc == 0x83) {
    	    	    f = 4;
    	    	    if (c >= 0x40 && c <= 0x94) {/*ｶﾀｶﾅ*/
    	    	    	if (opts & JTOKATA) {
    	    	    	    d = c - 0x40;
    	    	    	    if (d > 0x3e)
    	    	    	    	--d;
JKANA:
    	    	    	    d = jtokana[d];
    	    	    	    if (d == 0xFF)
    	    	    	    	goto JCC;
    	    	    	    jc = (d & 0x3f) + 0xA6/*'ｦ'*/;
    	    	    	    if (d & 0x80) {
    	    	    	    	*(js1++) = (UCHAR)jc;
    	    	    	    	jc = 0xDE/*'ﾞ'*/;
    	    	    	    } else if (d & 0x40) {
    	    	    	    	*(js1++) = (UCHAR)jc;
    	    	    	    	jc = 0xDF/*'ﾟ'*/;
    	    	    	    }
    	    	    	} else
    	    	    	    goto JCC;
    	    	    } else
    	    	    	goto JCC;
    	    	} else {
    	    	    f = 2;
JCC:	    	    /* 変換しないとき */
    	    	    *js1++ = (UCHAR)jc;
    	    	    jc = c;
    	    	}
    	    } /* else {
    	    	;
    	    } */
    	}
KJ1:
    	if (jc > 0xff) {
    	    *(js1++) = (UCHAR)(jc / 0x100);
    	    jc &= 0xff;
    	}
    	*(js1++) = (UCHAR)jc;
    } /* end of while */

    *js1 = '\0';
    if (err_f)
    	return NULL;
    else
    	return jstr1;
}
