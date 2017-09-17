/*
  unsigned char *jstruplow(unsigned char *jstr,unsigned short opts);
    unsigned char *jstr; 変換をする文字列. 直接書き換えます
    unsigned short opts; 変換方法

  指定された変換を文字列jstr に対し行います. 復帰値はjstrのｱﾄﾞﾚｽです。
  jstrを直接書き換えますので気をつけてください。
  変換方法は J2KATA|J2UPPER|J2LOWER のように'|'を使って同時に指定できます。

  opts の値
    A2UPPER  (0x01)   半角ｱﾙﾌｧﾍﾞｯﾄ小文字を大文字に変換します
    A2LOWER  (0x02)   半角ｱﾙﾌｧﾍﾞｯﾄ大文字を小文字に変換します
    J2UPPER  (0x04)   全角アルファベット小文字を大文字に変換します
    J2LOWER  (0x08)   全角アルファベット大文字を小文字に変換します
    J2KATA   (0x10)   全角ひらがなをカタカナに変換します
    J2HIRA   (0x20)   全角カタカナをひらがなに変換します
    JSPC2SPC (0x40)   全角空白を半角空白２ﾊﾞｲﾄに変換します

    * 1991/06/20 Writen by M.Kitamura
    * 1995/12/30 ANSI-Cスタイルに修正
 */

#include "jstr.h"

typedef unsigned char UCHAR;

char *jstruplow(char *jstr, unsigned mode)
{
    UCHAR up_f;
    UCHAR low_f;
    UCHAR jup_f;
    UCHAR jlow_f;
    UCHAR kata_f;
    UCHAR hira_f;
    UCHAR jspc_f;
    UCHAR d;
    UCHAR c;
    UCHAR *js;

    js = (UCHAR*)jstr;
    up_f = mode & A2UPPER;
    low_f = mode & A2LOWER;
    jup_f = mode & J2UPPER;
    jlow_f = mode & J2LOWER;
    kata_f = mode & J2KATA;
    hira_f = mode & J2HIRA;
    jspc_f = mode & JSPC2SPC;

    while ((c = *js++) != '\0') {
    	if (c <= 0x80) {
    	    if (c < 'A') {
    	    	;
    	    } else if (c <= 'Z') {
    	    	if (low_f)
    	    	    *(js - 1) = c + 0x20;
    	    } else if (c < 'a') {
    	    	;
    	    } else if (c <= 'z') {
    	    	if (up_f)
    	    	    *(js - 1) = c - 0x20;
    	    }
    	} else if (c <= 0x9f || (c >= 0xe0 && c <= 0xfc)) { /* ｼﾌﾄJIS */
    	    d = *(js);
    	    if (d == 0)
    	    	*(--js) = '\0';
    	    if (c == 0x81) {
    	    	if (d == 0x40 && jspc_f) {
    	    	    *(js - 1) = 0x20;
    	    	    *(js) = 0x20;
    	    	}
    	    } else if (c == 0x82) {
    	    	if (d >= 0x60 && d <= 0x79) {
    	    	    if (jlow_f) {
    	    	    	d += 0x81 - 0x60;
    	    	    	*(js - 1) = 0x82;
    	    	    	*(js) = d;
    	    	    }
    	    	} else if (d >= 0x81 && d <= 0x9a) {
    	    	    if (jup_f) {
    	    	    	d -= 0x81 - 0x60;
    	    	    	*(js - 1) = 0x82;
    	    	    	*(js) = d;
    	    	    }
    	    	} else if (d >= 0x9f && d <= 0xf1) {
    	    	    if (kata_f) {
    	    	    	d -= 0x9f - 0x40;
    	    	    	if (d > 0x7e)
    	    	    	    ++d;
    	    	    	*(js - 1) = 0x83;
    	    	    	*(js) = d;
    	    	    }
    	    	}
    	    } else if (c == 0x83 && d >= 0x40 && d <= 0x93) {
    	    	if (hira_f) {
    	    	    if (d > 0x7e)
    	    	    	--d;
    	    	    d += 0x9f - 0x40;
    	    	    *(js - 1) = 0x82;
    	    	    *(js) = d;
    	    	}
    	    }
    	    js++;
    	}
    }/* end of while */

    return jstr;
}
