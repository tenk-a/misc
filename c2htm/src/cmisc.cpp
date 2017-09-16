/**
 *  @file   cmisc.cpp
 *  @brief  基本的にC言語で書かれた雑多なルーチン郡の寄せ集め
 *
 *  @author Masahi KITAMURA (tenka@6809.net)
 *  @date   2003-07-22 他のcソースから抜き出してまとめ直し
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifdef __cplusplus
#include <string>
#endif

#include "cmisc.h"

using namespace std;

//----------------------------------------------------------------------------
#ifdef __cplusplus
namespace CMISC {
#endif

typedef unsigned char   Uint8;
typedef unsigned int    Uint32;


/** 簡易に内部バッファでsprintfをしてそのアドレスを返す */
char *strTmpF(const char *fmt, ...)
{
    static char buf[1024];
    va_list     args;

    if (fmt) {
        va_start(args, fmt);
        vsprintf(buf, fmt, args);
        va_end(args);
    } else {
        memset(buf, 0, sizeof(buf));
    }
    return buf;
}


/** 最後に必ず￥０を置く strncpy */
char *strNCpyZ(char *dst, const char *src, size_t size)
{
    strncpy(dst, src, size);
    dst[size - 1] = 0;
    return dst;
}

/** 余分に addSz バイトメモリを確保する strdup() */
char *strDup(const char *s, int addSz)
{
 #ifdef __cplusplus
    char *d = (char*)calloc(1, strlen(s) + 1 + addSz);
  #ifdef strcpy // bccのオプティマイズ対策
    return strcpy(d,s);
  #else
    return strcpy(d, s);
  #endif
 #else
    char *d = (char*)calloc(1, strlen(s) + 1 + addSz);
    return strcpy(d,s);
 #endif
}


/** 最後に\nがあればそれを削除 */
char *strDelLf(char s[])
{
    char *p;

    //assert(s);
    p = s + strlen(s);
    if (p != s && p[-1] == '\n') {
        p[-1] = 0;
    }
    return s;
}


#if 0
/** K,M,Gがついていたらそれぞれ 1024,1024*1024,1024*1024*1024 倍する strtol */
long strtolKM(char *s, char **d, int r)
{
    long l;

    l = strtol(s, &s, r);
    if (*s == 'k' || *s == 'K') {
        s++;
        l *= 1024;
    } else if (*s == 'm' || *s == 'M') {
        s++;
        l *= 1024*1024;
    } else if (*s == 'g' || *s == 'G') {
        s++;
        l *= 1024*1024*1024;
    }
    *d = s;
    return l;
}

/** 指定したsz でmalloc して失敗したら、minSz ～sz の範囲で適当に試してメモリを確保する
 *  aln が 0以上であれば 2のaln乗でサイズをアライメントします
 */
void *mallocMa(int sz, int minSz, int aln)
{
    void *p;
    int  a;

    /* アライメント用のマスクを生成 */
    if (aln <= 0)
        a = 1;
    else
        a = (1<<aln);
    a = a - 1;

    /* サイズを調整 */
    sz = (sz + a) & ~a;
    minSz = (minSz + a) & ~a;

    p = malloc(sz);
    if (p)
        return p;

    /* 最小サイズが確保できるかチェック */
    if (minSz <= 0)
        return NULL;
    p = malloc(minSz);
    if (p == NULL)
        return NULL;
    free(p);

    /* 確保できるサイズを探す*/
    do {
        sz = (sz/2 + a) & ~a;
        if (sz < minSz)
            sz = minSz;
        p = malloc(sz);
    } while (p == NULL && sz > minSz);

    return p;
}


#endif




// ----------------------------------------------------------------------------
// ファイル名処理関係
// ----------------------------------------------------------------------------

/** パス名中のファイル名位置を探す(MS-DOS依存) */
char *fname_getBase(const char *adr)
{
    const char *p;

    p = adr;
    while (*p != '\0') {
        if (*p == ':' || *p == '/' || *p == '\\')
            adr = p + 1;
      #ifdef USE_FNAME_SJIS
        if (ISKANJI((*(unsigned char *) p)) && *(p + 1))
            p++;
      #endif
        p++;
    }
    return (char*)adr;
}


/** 拡張子の位置を返す。なければ名前の最後を返す. */
char *fname_getExt(const char *name)
{
    char *p;

    name = fname_getBase(name);
    p = strrchr((char*)name, '.');
    if (p) {
        return p+1;
    }
    return (char *)(name+strlen(name));
}


#ifdef __cplusplus
/** 拡張子を付け替える */
std::string& fname_chgExt(string &fname, const char *ext)
{
    char       *p;

    p = fname_getBase(fname.c_str());
    p = strrchr(p, '.');
    if (p == NULL) {
        if (ext) {
            fname += ".";
            fname += ext;
        }
    } else {
        int n = p - fname.c_str();
        if (ext == NULL)
            fname.replace(n, string::npos, "");
        else
            fname.replace(n+1, string::npos, ext);
    }
    return fname;
}
#endif


/** filenameの拡張子をextに付け替える.
 *  ext=""だと'.'が残るが、ext=NULLなら'.'ごと拡張子を外す.
 */
char *fname_chgExt(char filename[], const char *ext)
{
    char       *p;

    p = (char *) fname_getBase(filename);
    p = strrchr(p, '.');
    if (p == NULL) {
        if (ext) {
            strcat(filename, ".");
            strcat(filename, ext);
        }
    } else {
        if (ext == NULL)
            *p = 0;
        else
            strcpy(p + 1, ext);
    }
    return filename;
}


/** filenameに拡張子が無かったらextを付け足す。
 */
char *fname_addExt(char filename[], const char *ext)
{
    if (strrchr(fname_getBase(filename), '.') == NULL) {
        strcat(filename, ".");
        strcat(filename, ext);
    }
    return filename;
}



/** path から ./ と ../ のサブディレクトリをルールに従って削除 (MS-DOS依存)
 */
char *fname_delDotDotDir(char *path)
{
    Uint8 *p = (Uint8*)path;
    Uint8 *d;
    Uint8 *dir = NULL;
    int c;

    if (*p && p[1] == ':') {        // とりあえず、手抜きで１文字だけ。ネットワークとかは考えない
        p += 2;
    }
    if (memcmp(p, "//", 2) == 0 || memcmp(p, "\\\\", 2) == 0) {     // ネットワークコンピュータ？
        p += 2;
        dir = p;
    }
    d = p;
    while (*p) {
        c = *p++;
        if (c == '/' || c == '\\') {
            c = '\\';
            *d++ = c;
            if (p[0] == '.') {
                if (p[1] == '/' || p[1] == '\\') {
                    p += 2;
                } else if (dir && p[1] == '.' && (p[2] == '/' || p[2] == '\\')) {
                    p += 3;
                    d = dir;
                }
            }
            dir = d;
      #ifdef USE_FNAME_SJIS
        } else if (ISKANJI(c) && *p) {
            *d++ = c;
            *d++ = *p++;
      #endif
        } else {
            *d++ = c;
        }
    }
    *d = 0;
    return path;
}



/** 文字列の最後に \ か / があれば削除
 */
char *fname_delLastDirSep(char *dir)
{
    char *p, *s;

    if (dir) {
        s = fname_getBase(dir);
        if (strlen(s) > 1) {
            p = s + strlen(s);
            if (p[-1] == '/') {
                p[-1] = 0;
            } else if (p[-1] == '\\') {
                //if (FIL_sjisFlag == 0) {
                    //p[-1] = 0;
                //} else
                {
                    int f = 0;
                    while (*s) {
                        f = 0;
                      #ifdef USE_FNAME_SJIS
                        if (ISKANJI(*s) && s[1]) {
                            s++;
                            f = 1;
                        }
                      #endif
                        s++;
                    }
                    if (f == 0)
                        p[-1] = 0;
                }
            }
        }
    }
    return dir;
}



//----------------------------------------------------------
// CRC 計算ルーチン
//----------------------------------------------------------

/// CRC テーブル
Uint32      memCrc32table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
};


/** アドレスdatからsizバイト数分をうまく計算して、メモリ内容が
 *  少しでも違えば、(なるべく)異なる(CRC)値を返す。
 */
int memCrc32(void *dat, int siz)
{
    Uint8   *s = (Uint8 *) dat;
    Uint32  r;

    r = 0xFFFFFFFFUL;
    while (--siz >= 0)
        r = (r >> 8) ^ memCrc32table[((Uint8) r) ^ *s++];
    return (int) (r ^ 0xFFFFFFFFUL);
}



#if __cplusplus
};      // CMISC
#endif
//----------------------------------------------------------------------------

