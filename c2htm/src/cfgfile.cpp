/**
 *  @file   cfg.cpp
 *  @brief  コンフィグファイルを扱う関数
 *
 *  @author Masahi KITAMURA (tenka@6809.net)
 *  @note
 *      別ツールからぬきだしてでっちあげ
 */

#include "cfgfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>


// 寄せ集めソースをネームスペースで隠す

namespace CfgFile_Tmp {

typedef unsigned char Uint8;

/** 文字列 s の先頭空白文字をスキップしたアドレスを返す */
static inline const char *strSkipSpc(const char *s)
{
    while ((*s && *(const unsigned char *)s <= ' ') || *s == 0x7f) {
        s++;
    }
    return s;
}

/** ファイル一括読み込み(ロード)
 *  @param  name    読みこむファイル
 *  @param  buf     読みこむメモリ。NULLが指定されれば mallocし、16バイト余分に確保する
 *  @param  bufsz   bufのサイズ。0が指定されれば ファイルサイズとなる
 *  @param  rdszp   NULLでなければ、読みこんだファイルサイズを入れて返す
 *  @return         bufのアドレスかmallocされたアドレス. エラー時はNULLを返す
 */
static void *file_loadAbs(const char *name, void *buf, int bufsz, int *rdszp)
{
    FILE *fp;
    int  l;

    fp = fopen(name, "rb");
    if (fp == NULL)
        return NULL;
    l = filelength(fileno(fp));
    if (rdszp)
        *rdszp = l;
    if (bufsz == 0)
        bufsz = l;
    if (l > bufsz)
        l = bufsz;
    if (buf == NULL) {
        bufsz = (bufsz + 15 + 16) & ~15;
        buf = calloc(1, bufsz);
        if (buf == NULL)
            return NULL;
    }
    fread(buf, 1, l, fp);
    if (ferror(fp)) {
        fclose(fp);
        buf = NULL;
    }
    fclose(fp);
    return buf;
}


/* ------------------------------------------------------------------------ */
/* テキストファイル行入力                                                   */
/* ------------------------------------------------------------------------ */
#define FILE_NMSZ   1024

/// テキスト行入力ルーチン用
typedef struct txtf_t {
    char        txtName[FILE_NMSZ];
    const char  *txtTop;
    const char  *txtEnd;
    const char  *txtPtr;
    int         txtLine;
    int         txtMallocFlg;
} txtf_t;

static txtf_t txtf;

/** テキストファイルオープン(行入力用). 一括でファイルをメモリーにロード */
void txtf_open(const char *fname)
{
    int         sz = 0;

    strncpy(txtf.txtName, fname, FILE_NMSZ);
    txtf.txtName[FILE_NMSZ-1] = '\0';
    txtf.txtTop         = (const char *)file_loadAbs(fname, NULL, 0, &sz);  // ファイルサイズ分Mallocしてロード
    txtf.txtMallocFlg   = 1;                                    // Mallocしたメモリを使う
    txtf.txtPtr         = txtf.txtTop;
    txtf.txtEnd         = txtf.txtTop + sz;
    txtf.txtLine        = 0;
}


/** メモリのテキストデータを対象にする。Mallocしない！ */
void txtf_openMem(const void *mem, int sz)
{
    txtf.txtTop         = (const char *)mem;
    txtf.txtMallocFlg   = 0;                        // Mallocしてないぞ
    txtf.txtPtr         = txtf.txtTop;
    txtf.txtEnd         = txtf.txtTop + sz;
    txtf.txtLine        = 0;
}


/** テキスト行入力の終了 */
void txtf_close(void)
{
    if (txtf.txtTop && txtf.txtMallocFlg) {     // Mallocしたメモリなら解放
        free((void*)txtf.txtTop);
    }
    txtf.txtMallocFlg   = 0;
    txtf.txtTop         = (const char *)NULL;
    txtf.txtPtr         = (const char *)NULL;
    txtf.txtLine        = 0;
}



/** テキスト行入力 */
char *txtf_gets(char *buf, int sz)
{
    const char *s;
    char       *d;

    if (sz <= 0)
        return buf;
    --sz;
    d = buf;
    s = txtf.txtPtr;
    if (s == 0 || *s == 0 || s >= txtf.txtEnd)
        return (char*)NULL;
    while (*s && s < txtf.txtEnd) {
        if (*s == '\r' && s[1] == '\n') {
            s++;
            continue;
        }
        if (sz == 0)
            break;
        *d++ = *s++;
        --sz;
        if (s[-1] == '\n')
            break;
    }
    txtf.txtPtr = s;
    *d = 0;
    txtf.txtLine++;
    return buf;
}



/* ------------------------------------------------------------------------ */
/* コンフィグ・ファイル読み込み                                             */
/* ------------------------------------------------------------------------ */

//#define CMT_CHR           ';'
//#define CMT_CHR           '%'
#define CMT_CHR             '/'
// 2個並べるときは定義
#define CMT_2

static char *cfg_buf;       // 読み込んだcfgファイルの内容
static int  cfg_size;       // そのバイト数

/** コンフィグ情報(ファイル)の準備 */
int cfg_init(const char *fn)
{
    cfg_buf = (char *)file_loadAbs(fn, NULL, 0, &cfg_size);
    return cfg_buf != NULL;
}


/** コンフィグ情報の準備。オンメモリ版 */
int cfg_init4mem(const char *mem, int memSz)
{
    //if (mem == NULL || memSz < 0) {ERR_ROUTE(); return 0;}
    cfg_buf = (char *)calloc(1, memSz);
    if (cfg_buf)
        memcpy(cfg_buf, mem, memSz);
    cfg_size = memSz;
    return cfg_buf != NULL;
}


/** コンフィグ情報の使用を終了 */
void cfg_term(void)
{
    if (cfg_buf)
        free(cfg_buf);
    cfg_buf = (char *)NULL;
}


/** tnに対応する文字列を取得 */
char *cfg_getStr(char *dst, int len, const char *tn)
{
    static char buf[0x1000];
    char *s;
    char *p;
    int  tnlen,rc=0;

    if (cfg_buf == NULL || tn == NULL)
        return NULL;

    //DBG_ASSERT(tn != NULL);
    tnlen = strlen(tn);

    if (dst == NULL) {  // 指定がなければ、内部バッファに取得してアドレスを返す
        dst = buf;
        len = sizeof(buf);
    }
    txtf_openMem(cfg_buf, cfg_size);

    for (;;) {
        // 一行取得
        dst[0] = 0;
        s = txtf_gets(dst, len);
        if (s == NULL) {    // ファイルの終わりなら終了
            dst[0] = 0;
            break;
        }
        // 行頭空白をスキップ
        s = (char*)strSkipSpc(s);
        if (memcmp(s, tn, tnlen) == 0) {    // 指定したTAGNAMEと同じか？
            rc = 1;
            // = があれば飛ばす
            s = (char*)strSkipSpc(s+tnlen);
            // { があれば、複数行定義、なければ一行定義
            if (*s == '=') {    // 一行定義
                s = (char*)strSkipSpc(s+1);
                // バッファガード
                if (strlen(s) >= (size_t)len) {
                    s[len-1] = 0;
                }
                int md = 0;
                for (p = s; *(Uint8*)p >= 0x20 && *p != 0x7f; p++) {
                    if (*p == '"')
                        md ^= 1;
                  #ifdef CMT_2
                    if (md == 0 && p > s && p[-1] <= ' ' && *p == CMT_CHR && p[1] == CMT_CHR)       // 直前が空白の ; はコメント扱い
                        break;
                  #else
                    if (md == 0 && p > s && p[-1] <= ' ' && *p == CMT_CHR)      // 直前が空白の ; はコメント扱い
                        break;
                  #endif
                }
                while (p > s && p[-1] == ' ') --p;      // ケツにある空白は削除
                *p = 0;
                if (s < p-1 && *s == '"' && p[-1] == '"') {
                    *s++ = 0;
                    *--p = 0;
                }
                if (s <= p)
                    memmove(dst, s, strlen(s)+1);
                break;
            } else if (*s == '{') {     // {}による複数行定義
                char buf2[0x1000];
                int l, n = 0;
                dst[0] = 0;
                for (;;) {
                    s = txtf_gets(buf2, sizeof buf2);
                    if (s == NULL)
                        break;
                    s = (char*)strSkipSpc(s);
                    if (*s == '}')
                        break;
                    if (*s == '\0')
                        continue;
                  #ifdef CMT_2
                    if (*s == CMT_CHR && s[1] == CMT_CHR)
                        continue;
                    for (p = s; *(Uint8*)p >= 0x20 && *p != 0x7f; p++) {
                        // 直前が空白の // はコメント扱い
                        if (p > s && p[-1] <= ' ' && *p == CMT_CHR && p[1] == CMT_CHR)
                            break;
                    }
                  #else
                    if (*s == CMT_CHR)
                        continue;
                    for (p = s; *(Uint8*)p >= 0x20 && *p != 0x7f; p++) {
                        if (p > s && p[-1] <= ' ' && *p == CMT_CHR)     // 直前が空白の ; はコメント扱い
                            break;
                    }
                  #endif
                    while (p > s && p[-1] == ' ') --p;      // ケツにある空白は削除
                    *p = 0;
                    l = strlen(s);
                    if (n + l + 2 > len) {
                        l = len - n - 2;
                        if (l <= 0)
                            break;
                    }
                    if (l <= 0)
                        continue;
                    memcpy(dst+n, s, l);
                    n += l;
                    dst[n] = '\n';
                    n++;
                    dst[n] = '\0';
                }
                break;
            }
        }
    }
    return rc ? dst : NULL;
}


/** tn の値を取得 */
int     cfg_getVal(const char *tn)
{
    char buf[256], *rp, *s;
    int  rc = 0;

    txtf_openMem(cfg_buf, cfg_size);

    for (;;) {
        rp = txtf_gets(buf, 256);
        if (rp == NULL)
            break;
        rp = (char*)strSkipSpc(rp);
        if (memcmp(rp, tn, strlen(tn)) == 0) {  // 指定したTAGNAMEと同じか？
            s = (char*)strSkipSpc(rp+strlen(tn));
            if (*s == '=')
                s = (char*)strSkipSpc(s+1);
            rc = strtol(s, (char**)NULL, 0);        //rc = cfg_getval0(rp + strlen(tn));
            //DBG_F(("cfg[%d=%s]>%s",rc,s,rp));
            break;
        }
    }
    txtf_close();
    return rc;
}

};      // CfgFile_Tmp


// 寄せ集めを使うのはここだけー
using namespace CfgFile_Tmp;

using namespace std;


/** 初期化 */
bool CfgFile::init(
    const char  *name,      ///< ファイル名
    int         memSw)      ///< 0:ファイル読み込み !0:メモリ
{
    int rc;
    if (memSw == 0) // ファイルを読み込む
        rc = cfg_init(name);
    else            // メモリにおかれた文字列をファイル扱いにする
        rc = cfg_init4mem(name, strlen(name));
    return rc != 0;
}

void CfgFile::term()
{
    cfg_term();
}


/** tagName の持つ値を取得 */
int  CfgFile::getVal(const char *tagName)
{
    int val = cfg_getVal(tagName);
    return val;
}

/** tagName の保持する文字列を取得 */
bool CfgFile::getStr(string &dst, const char *tagName)
{
    char    buf[0x10000];
    buf[0] = 0;
    char *p = cfg_getStr(buf, sizeof buf, tagName);
    if (p)
        dst = p;
    else
        dst = "";
    return p != NULL && buf[0] != 0;
}


/** ファイル名pathリストをvector lstに分解する */
bool CfgFile::getStrVec(vector<string> &lst, const char *tagName)
{
    string wk("");
    int rc = getStr(wk, tagName);
    if (rc) {
        rc = str2vec(lst, wk.c_str());
    }
    return rc != 0;
}


/** 文字列を'￥ｎ'か'￥ｔ'で区切って、文字列のリストを作る
 *  行頭に%があるか、空白の直後に%がある場合、改行までをコメントとして読み飛ばす
 */
int CfgFile::str2vec(vector<string> &lst, const char *src)
{
    const char  *s = src;

    lst.clear();
    while (*s) {
        const char *nm = s = strSkipSpc(s);
      #ifdef CMT_2
        if (*s == CMT_CHR && s[1] == CMT_CHR)
      #else
        if (*s == CMT_CHR)
      #endif
        {       // コメントがあったら、スキップ
            s = strchr(s, '\n');
            if (s == NULL) {
                break;
            }
            s++;
            continue;
        }
        if (*s == '\0')
            break;
        for (s = nm; *(Uint8*)s >= 0x20 && *s != 0x7f; s++) {
          #ifdef CMT_2
            // 直前が空白の // はコメント扱い
            if (s > nm && s[-1] <= ' ' && *s == CMT_CHR && s[1] == CMT_CHR)
                break;
          #else
            if (s > nm && s[-1] <= ' ' && *s == CMT_CHR)        // 直前が空白の ; はコメント扱い
                break;
          #endif
        }
        while (s > nm && s[-1] == ' ') --s;     // ケツにある空白は削除
        int    len = s - nm;
        string tmp("");
        tmp.assign(nm, len);
        lst.push_back(tmp);
    }
    return lst.size();
}


#if 0
#include <iostream>

int main()
{
    CfgFile cfg;

    cfg.init("c2htm.cfg");
    //cout << cfg_buf << endl;
    string st("");
    cfg.getStr(st, "C_SY");
    cout << "C_SY =" <<  st << endl;

    st.clear();
    cfg.getStr(st, "HEADER");
    cout << "HEADER =" <<  st << endl;

    st.clear();
    cfg.getStr(st, "WORD1");
    cout << "WORD1 =" <<  st << endl;

    st.clear();
    cfg.getStr(st, "SHARP");
    cout << "SHARP =" <<  st << endl;

    st.clear();
    cfg.getStr(st, "WORD4");
    cout << "WORD4 =" <<  st << endl;

    return 0;
}

#endif

