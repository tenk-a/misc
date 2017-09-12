/**
 *  @file   os9fdimg.c
 *  @brief  OS9 FDD Image -> Win32 ファイルコンバータ
 *  @note
 *      getos9
 *        OS9 -> MSDOS ファイルコンバータ Copyright (c) 1989,1990,1992 by keiji murakami
 *      を改造して fdd読み取り機能を外し、ファイル名のdos8.3形式変換を無しにしたもの。
 *      元ソースは pc98版でなく、それを改造されたpc-at(by Gigo)版がベース。
 *
 *      2006 modified by tenk* (Masashi KITAMURA)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#ifdef _MSC_VER
# include <sys/utime.h>
#else
# include <utime.h>
#endif

#ifdef _WIN32       // for win32
# include <io.h>
# include <direct.h>
# define  DIR_SEP_STR       "\\"
#else               // gcc for linux
# include <unistd.h>
# define  DIR_SEP_STR       "/"
#endif



// ---------------------------------------------------------------------------

#define inline          __inline
typedef signed   char   Sint8;
typedef unsigned char   Uint8;
typedef int             Sint32;
typedef unsigned int    Uint32;
typedef unsigned short  Uint16;

#define ISKANJI(c)      ((Uint8)(c) >= 0x81 && ((Uint8)(c) <= 0x9F || ((Uint8)(c) >= 0xE0 && (Uint8)(c) <= 0xFC)))


/// 更新後のアドレスを返す文字列コピー.
inline char *stpcpy(char *d, const char *s)
{
    while ((*d++ = *s++) != 0)
        ;
    return (--d);
}



/// ファイルの日付時間を設定. yy:西暦(1900以降) mm:1-12 dd:1-31 hh:0-23 min:0-59 sec:0-61
void setFileDate(const char *fname, int yy, int mm, int dd, int hh, int min, int sec)
{
    struct utimbuf  utb;
    struct tm       tms;
    time_t          t;

  #ifdef MY_DISK    // 時間設定を失敗してたディスクを適当にごまかす.
    if (yy <= 1900 && mm >= 80) {
        yy = 1900+mm;
        mm = dd;
        if (mm < 1 || mm > 12) {
            mm = 1;
            dd = 1;
        } else {
            dd = hh;
        }
        if (dd < 0 || dd > 31)
            dd = 1;
        hh  = 0;
        min = 0;
        sec = 0;
    }
  #endif

    assert( fname != NULL && fname[0] );
    assert(yy >= 1900);

    if (mm < 1 || mm > 12)  mm  = 1;
    if (dd < 1 || dd > 31)  dd  = 1;
    if ((Uint32)hh  >= 24)  hh  = 0;
    if ((Uint32)min >= 60)  min = 0;
    if ((Uint32)sec >= 62)  sec = 0;

    tms.tm_sec   = sec;
    tms.tm_min   = min;
    tms.tm_hour  = hh;
    tms.tm_mday  = dd;
    tms.tm_mon   = mm - 1;          // なんで 0オリジンやねん(T T)
    tms.tm_year  = yy - 1900;
    tms.tm_wday  = -1;
    tms.tm_yday  = -1;
    tms.tm_isdst = -1;

    t = mktime(&tms);
    utb.actime  = t;
    utb.modtime = t;
    utime( fname, &utb );
}



/// ファイルサイズを取得.
Uint32  getFileSize(const char *fname)
{
    Uint32  l;
    FILE    *fp;
    assert( fname != NULL && fname[0] );
    fp = fopen(fname, "rb");
    if (fp == NULL)
        return 0;
    //x l = filelength( fileno(fp) );
    fseek(fp, 0, SEEK_END);
    l = ftell(fp);
    fclose(fp);
    return l;
}



// ---------------------------------------------------------------------------

/* ディスクＩ／Ｏ */

int         disk_offset;
int         disk_size     = 0;
static FILE *disk_fp      = 0;
static char *disk_errSect = 0;


/// fddディスクイメージを設定する.
void v_disk_open(const char *dname, int offset)
{
    disk_size = getFileSize( dname );
    disk_fp   = fopen(dname, "rb");
    if (disk_fp == NULL) {
        fprintf(stderr, "can't open : %s\n", dname);
        exit(1);
    }
    if (offset < 0) {
        offset  = 16*2;
        if (disk_size >= 70*2*26*256)   // 適当に 1M以上あれば 2hd とみなす.
            offset = 0x27;              // 26*2;
    }
    disk_offset = offset*256;
    //x printf("%s : size=%x, spt=%d -> ofs=%x\n", dname, size, spt, disk_offset);
    disk_errSect = calloc(1, (disk_size+255) >> 8 );
}



/// セクターエラー情報をファイルから読み込む.
void v_disk_readErrSectInfo(const char *fname)
{
    int  c;
    int  n = 0;
    FILE *fp = fopen(fname, "rb");
    if (fp == NULL)
        return;
    do {
        c = fgetc(fp);
        if (c == '.')
            disk_errSect[n++] = 0;
        else if (c == 'x')
            disk_errSect[n++] = 1;
    } while (c >= 0 && n < (disk_size+255)/256 );
    fclose(fp);
}



/// ディスク内の絶対位置へシーク.
int v_disk_seek(Sint32 pos)
{
    Sint32 ofs = pos + disk_offset;

    fseek(disk_fp, ofs, SEEK_SET);
    if (ofs > disk_size) {
        fprintf(stderr, "\tbad seek (%d)\n", ofs);
        return 0;
    }
    return 1;
}



/// 現在位置から読み込む.
void v_disk_read(void *buf, int sz)
{
    // エラーセクタ情報があるなら、セクタエラーをチェック.
    if (disk_errSect) {
        Sint32  pos    = ftell( disk_fp );
        Sint32  posEnd = pos + sz - 1;
        posEnd >>= 8;
        for (pos >>= 8; pos <= posEnd; ++pos) {
            if (disk_errSect[pos])
                fprintf(stderr, "\tsector %d read error\n", pos);
        }
    }

    fread(buf, 1, sz, disk_fp);
}



/// ディスクアクセスの終了.
void v_disk_close(void)
{
    if (disk_fp)
        fclose( disk_fp );
    disk_fp = NULL;

    if (disk_errSect)
        free( disk_errSect );
    disk_errSect = NULL;
}




// ---------------------------------------------------------------------------

/* ＯＳ９ファイルシステム */

/// os9/09でのファイル名の文字数.
#define OS9MAXNAME      29


typedef Uint8           BigEnd24[3];
#define b3toUint32(b)   (((b)[0] << 16) | ((b)[1] << 8) | (b)[2])



/// ディレクトリ・エントリ. 32バイト.
struct DirSlot {
    char        fname[OS9MAXNAME];
    BigEnd24    lsn;
};
typedef struct DirSlot      DirSlot;


/// ディレクトリ・セグメント管理.
struct FileSeg {
    BigEnd24    lsn;
    Uint8       ss[2];
};
typedef struct FileSeg      FileSeg;



#define B_DIR       0x80

/// os9 ファイル・ディスクリプタ.
struct OS9_FILE_D {
    Uint8       at;                     ///< 属性.
    Uint8       posH[2];                ///< 位置. 上位2バイト.
    Uint8       yy, mm, dd, hh, min;    ///< 日付情報.
    Uint8       sn, cpos;
    BigEnd24    fsize;                  ///< ファイルサイズ.
    char        spos;
    Uint8       posL[2];                ///< 位置. 下位2バイト.
    FileSeg     seglst[48];             ///< ファイルセグメント.
};
typedef struct OS9_FILE_D   OS9_FILE_D;



/// ルートセクタの位置を強制的に変更する場合(壊れディスクのサルベージでサブディレクトリを指定したい場合向け).
Uint32      os9rootSct     = 0;
OS9_FILE_D *os9root;                    ///< ルートディレクトリ情報.


int         os9KanjiRBF_sw = 0;         ///< 漢字RBF 用FDD の時onにすること.


/// os9ファイルディスクリプタから、位置情報を取得.
static inline int   OF9_FILE_D_getPos( OS9_FILE_D *fp )
{
    return (fp->posH[0] << 24) | (fp->posH[1] << 16) | (fp->posL[0] << 8) | fp->posL[1];
}



/// os9ファイルディスクリプタに、位置情報を設定.
static inline void  OF9_FILE_D_setPos( OS9_FILE_D *fp, int pos )
{
    fp->posH[0] = (Uint8)(pos >> 24);
    fp->posH[1] = (Uint8)(pos >> 16);
    fp->posL[0] = (Uint8)(pos >>  8);
    fp->posL[1] = (Uint8)pos;
}



/// ファイルセグメントのサイズ情報を取得.
static inline int   FileSeg_setSS( const FileSeg *seg )
{
    return (seg->ss[0] << 8) | seg->ss[1];
}



/// ファイルセグメントの位置情報を取得.
static inline int   FileSeg_getLSN( const FileSeg *seg )
{
    return b3toUint32( seg->lsn );
}



/// os9ファイル名データをc文字列にして返す. buf[]は OS9MAXNAME+1以上の領域があること.
char   *os9_toStr(char buf[], const char *s)
{
    int         c;
    int         i = 0;

    if (os9KanjiRBF_sw) {
        while (*s && i < OS9MAXNAME) {
            c = *(unsigned *)s++;
            if (*s == 0 || i == OS9MAXNAME-1) { // 次の文字が'\0'なら終わりなので、終端ビットを消す.
                c &= 0x7f;
            } else if (ISKANJI(c)) {
                buf[i++] = (char)c;
                c        = *s++;
            }
            buf[i++] = (char)c;
        }
    } else {
        // 最上位ビットが立っていたら(負数になったら)終わり.
        while (*(Sint8 *)s > 0 && i < OS9MAXNAME) {
            c        = *s++;
            buf[i++] = (char)c;
        }
        c = *s;
        buf[i++] = (char)(c & 0x7f);
    }
    buf[i] = '\0';
    return buf;
}



int os9_seek(OS9_FILE_D * fp, Sint32 pos)
{
    if ((Sint32)b3toUint32(fp->fsize) < pos) {
        return -1;
    }
    OF9_FILE_D_setPos(fp, pos);
    return  0;
}



OS9_FILE_D *os9_open_s(Sint32 sct)
{
    OS9_FILE_D *p = calloc(1, sizeof(OS9_FILE_D));

    if (p != NULL) {
        v_disk_seek(sct << 8);
        v_disk_read(p, 256);
        os9_seek(p, 0);
    }
    return p;
}



#define os9_close(fp)       free(fp)



void os9_init(const char *dname, int offset)
{
    BigEnd24    sctbuf;
    char        ddbuf[32];
    static char fname[OS9MAXNAME + 3];
    Uint32      sct;

    v_disk_open(dname, offset);
    v_disk_seek(8);
    v_disk_read(&sctbuf, 3);
    v_disk_seek(31);
    v_disk_read(ddbuf, 32);
    sct = b3toUint32(sctbuf);
    if ( os9rootSct > (Uint32)(disk_offset>>8) )
        sct = os9rootSct - (disk_offset>>8);
    printf("Disk name: %s \t(image size=%#x, os9root-sect=%d)\n", os9_toStr(fname, ddbuf), disk_size, sct);
    os9root = os9_open_s(sct);
}



unsigned os9_read(OS9_FILE_D * fp, Uint8 *buf, unsigned len)
{
    //x Uint16  r, l;
    Uint32  r, l;
    Sint32  fsize = b3toUint32(fp->fsize);
    Sint32  pos   = OF9_FILE_D_getPos( fp );
    Uint32  i     = (Uint32)-1;
    Sint32  sl    = 0;

    for (r = len; r > 0; r -= l) {
        Sint32  ofs, sct;
        Sint32  s = fsize - pos;
        if (s <= 0)
            return (len - r);
        //x l = ((Uint32)s < r) ? (Uint16) s : r;
        l = ((Uint32)s < r) ? (Uint32) s : r;
        while ((s = sl - pos) <= 0) {
            sct = FileSeg_setSS( &fp->seglst[++i] );
            sl += (Sint32)sct << 8;
        }
        //x l   = ((Uint32)s < l) ? (Uint16) s : l;
        l   = ((Uint32)s < l) ? (Uint32) s : l;
        sct =  FileSeg_getLSN( &fp->seglst[i] );
        sct += FileSeg_setSS( &fp->seglst[i] );
        ofs = sct << 8;
        ofs -= s;
        v_disk_seek( ofs );
        if (l > 0)
            v_disk_read(buf, l);
        buf += l;
        pos += l;
        os9_seek(fp, pos);
    }
    return len;
}


#define os9_term()      v_disk_close()





// ---------------------------------------------------------------------------
/* tree コマンド */

// tree_sub,get_file中のローカル変数 dirSlot, dirSlotNum を暗黙に参照してるので注意.
/// 次のDirSlotへdを進める.
#define NEXT_DIR(fp, d)     ( ++d < dirSlot + dirSlotNum || ( (d = dirSlot), (dirSlotNum = os9_read( fp, (Uint8 *)dirSlot, 256) / 32) > 0 ))


#define DBUFSIZE    (8*1024)


/// ファイルを一括生成.
void get_file(OS9_FILE_D * fp, int nest, const char *path)
{
    static char newNm[OS9MAXNAME+3];
    OS9_FILE_D *nfp;
    int         dirSlotNum;
    DirSlot     dirSlot[8];
    DirSlot     *d;
    char        newpath[1024];
    unsigned    n;
    FILE        *ofp = NULL;
    int         sz;
    int         rc;

    // ディレクトリスロットの最初の1セクタを読み込む.
    sz         = os9_read(fp, (Uint8 *) dirSlot, 256);
    dirSlotNum = sz / 32;
    d   = dirSlot + 1;
    for (;;) {
        if (NEXT_DIR(fp, d) == 0)
            break;

        // 削除ずみファイルはスキップ.
        if ((d->fname[0] & 0x7f) <= ' ')
            continue;
        if ((d->fname[0] & 0x7f) == '?')
            continue;

        // ファイル名を取得.
        os9_toStr(newNm, d->fname);

        // os9ファイルをオープン.
        nfp = os9_open_s(b3toUint32(d->lsn));

        strcpy( stpcpy(newpath, path), newNm);
        if (nfp->at & B_DIR) {
            // ディレクトリの場合.
          #ifdef _WIN32     // _MSC_VER
            rc = mkdir(newpath);
          #else
            rc = mkdir(newpath, 0777);
          #endif
            if (rc) {
                fputs("\n Can't make a directory:", stderr);
                fputs(newpath, stderr);
                exit(2);
                return;
            }
        } else {
            // 通常のファイルの場合.
            if (access(newpath, 0) == 0) {
                printf("File already exist. Skip : %s\n", newpath);
                continue;
            }

            ofp = fopen(newpath, "wb");
            if (ofp == NULL) {
                fputs("\n Can't open: ", stderr);
                fputs(newpath, stderr);
                exit(2);
                return;
            }
        }

        printf( "%s\n", newpath );

        if (nfp->at & B_DIR) {
            // ディレクトリの場合は、再帰で処理.
            get_file(nfp, nest + 1, strcat(newpath, DIR_SEP_STR));
        } else {
            // 1ファイルの抜き出し.
            static Uint8    dbuf[DBUFSIZE];
            while ( (n = os9_read(nfp, dbuf, DBUFSIZE)) != 0 ) {
                if (fwrite(dbuf, 1, n, ofp) == 0) {
                    fprintf(stderr, "\n %s : Write error\n", newpath);
                    exit(2);
                }
            }
            fclose(ofp);

            // 時間を設定.
            setFileDate( newpath, 1900+nfp->yy, nfp->mm, nfp->dd, nfp->hh, nfp->min, 0 );
        }
        os9_close(nfp);
    }
}




/// ツリー表示の本体.
void tree_sub(OS9_FILE_D * fp, int nest)
{
    OS9_FILE_D *nfp;
    static char tabs[] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
    int         sct;
    char        nm[OS9MAXNAME+3];
    int         dirSlotNum;
    DirSlot     dirSlot[8];
    DirSlot     *d;
    int         sz;

    sz = os9_read(fp, (Uint8 *) dirSlot, 256);
    dirSlotNum = sz / 32;
    d = dirSlot + 1;
    for (;;) {
        if (NEXT_DIR(fp, d) == 0)
            break;
        os9_toStr(nm, d->fname);
        if (nm[0] == '\0')
            continue;
        sct = b3toUint32(d->lsn);
        nfp = os9_open_s(sct);
        printf("%04d-%02d-%02d %02d:%02d%8d %2d ", 1900+nfp->yy, nfp->mm, nfp->dd, nfp->hh, nfp->min, b3toUint32(nfp->fsize), nfp->sn);
        printf("%s%s\n", &tabs[sizeof(tabs) - nest], nm);
        if (nfp->at & B_DIR)
            tree_sub(nfp, nest + 1);
        os9_close(nfp);
    }
}


/// ツリー表示.
void tree(void)
{
    tree_sub(os9root, 1);
}





// ---------------------------------------------------------------------------

void help(void)
{
  #ifdef JAPANESE
    fputs("os9img2file v0.50  os9 FDDベタイメージからファイルを取得.  " __DATE__ "\n"
          "                                                     modified by tenk*\n"
          "     元プログラム   getos9 v0.3  OS9->MS-DOS file converter for PC9801\n"
          "                            Copyright (c) 1989,1990,1992 by k.murakami\n"
          "                                          (modified for PC-AT by Gigo)\n"
          "\n"
          "Usage> os9img2file command [option] diskimage\n"
          "  command: tree | get\n"
          "  option:  -o#     論理セクターのオフセット\n"
          "           -k[-]   漢字RBF ディスク対策 -k- off\n"
          "           -r#     強制的にrootセクタをNに設定\n"
          "           -dDIR   出力ディレクトリ\n"
          "           -eIMH   エラーセクタ情報ファイル(.imh)の読込\n"
          , stderr);
  #else
    fputs("os9img2file v0.50  os9 FDD Image -> file converter.  " __DATE__ "\n"
          "                                               modified by tenk*\n"
          "   [original] getos9 v0.3  OS9->MS-DOS file converter for PC9801\n"
          "                      Copyright (c) 1989,1990,1992 by k.murakami\n"
          "                                    (modified for PC-AT by Gigo)\n"
          "\n"
          "Usage> os9img2file command [option] diskimage\n"
          "  command: tree | get\n"
          "  option:  -o#     logical sector offset\n"
          "           -k[-]   for Kanji RBF   -k- off\n"
          "           -r#     root sector offset\n"
          "           -dDIR   output directory\n"
          "           -eIMH   use .imh-file (sector-error-info.)\n"
          , stderr);
  #endif
    exit(1);
}



#define DefaultOffset       (-1)


int main(int argc, char *argv[])
{
    int         i;
    int         offset        = DefaultOffset;
    const char  *dstDir       = "";
    const char  *cmd          = NULL;
    const char  *img          = NULL;
    const char  *errSectFName = NULL;

    for (i = 1; i < argc; i++) {
        const char  *p = argv[i];
        if (p[0] == '-') {
            if (p[1] == 'o')        // -o
                offset = strtol(p+2,0,0);
            else if (p[1] == 'k')   // -k
                os9KanjiRBF_sw = (p[2] != '-');
            else if (p[1] == 'r')   // -r
                os9rootSct = strtol(p+2,0,0);
            else if (p[1] == 'd')   // -d
                dstDir = p+2;
            else if (p[1] == 'e')   // -d
                errSectFName = p+2;
            else
                help();
        } else {
            if (cmd == NULL)
                cmd = p;
            else if (img == NULL)
                img = p;
            else
                help();
        }
    }

    if (cmd == NULL || img == NULL)
        help();

    os9_init(img, offset);
    if (errSectFName)
        v_disk_readErrSectInfo(errSectFName);

    if (strcmp(cmd, "tree") == 0) {
        // ファイルをツリー表示.
        tree();

    } else if (strcmp(cmd, "get") == 0) {
        // 生成先ディレクトリの準備.
        char newpath[ 1024 ];
        char *d = stpcpy( newpath, dstDir );
        if (d > newpath && d[-1] != DIR_SEP_STR[0])
            strcpy((char*)d, DIR_SEP_STR);
        if (newpath[0]) {
          #ifdef _WIN32 // _MSC_VER
            mkdir(newpath);
          #else
            mkdir(newpath, 0777);
          #endif
        }

        // ファイルを一括生成.
        get_file(os9root, 1, newpath);

    } else {
        help();
    }

    os9_term();
    return 0;
}
