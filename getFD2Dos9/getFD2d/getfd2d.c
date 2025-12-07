/**
 *  @file getFD2D.c
 *  @brief  16bit ms-dos環境で、FDDから image 化.
 *  @author Masashi Kitamura ( https://github.com/tenk-a/ )
 *  @date   2006-01,2025-12
 *  @note
 *          getos9.exe for PC/AT のディスク読込部を参考に作成.
 *          16bit DOS 環境用. (32bitもビルド可能かも?)
 *          open watcom-c/c++ でコンパイル (元々 16bit borland-c だが現状未確認)
 *          64bit windows環境ではコンパイルチェックのみ。(ONLY_COMPILE定義)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "dos_wrap.h"


enum {
    CYL_MAX  =   80+5,
    SECT_MAX =   31,
    SECT_SZ  = 1024,
};


/// usage
///
int usage(void)
{
    fprintf(stderr,
            "usage> getFD2D [-opts] drv: output.img        // v0.60 " __DATE__ " by tenk*\n"
            " (5寸)FDD からベタイメージを作成. デフォルトは -2d b:\n"
            " drv:        a: or b:\n"
            " -2d         80*16*256 bytes\n"
            " -2dd       160*16*256 bytes\n"
            " -2hd       154*26*256 bytes\n"
            " -2hd1024   154* 8*1024 bytes\n"
            " -cN        シリンダー数(1..85)\n"
            " -snN       セクタ数(1..31)\n"
            " -ssN       セクタサイズ(256|512|1024)\n"
            " -rN        リトライ数(1..16)\n"
            " -reset     biosのセクタサイズを512バイトに強制修正\n"
    );
    return 1;
}


// -------------------------------------------------------------------------

#if defined(ONLY_COMPILE)

static int bios_disk(int cmd, int drive, int head, int track, int sect, int nsects, void *buffer)
{
    (void)cmd; (void)drive; (void)head; (void)track; (void)sect; (void)nsects; (void)buffer;
    return 0;
}

#elif (defined(__TURBOC__) || defined(__BORLANDC__)) && !defined(__FLAT__)

#define bios_disk   biosdisk

#elif defined(__WATCOMC__)

///
///
static int bios_disk(int cmd, int drive, int head, int track, int sect, int nsects, void *buffer)
{
    struct diskinfo_t   di;

    di.drive    = drive;
    di.head     = head;
    di.track    = track;
    di.sector   = sect;
    di.nsectors = nsects;
    di.buffer   = (wchar_t*)buffer;

    return _bios_disk(cmd, &di);   /* 0 が成功。エラーコードは上位バイト */
}

#else
 #error "This program requires 16bit DOS BIOS disk interface (Watcom or Borland) or define ONLY_COMPILE."
#endif


// -------------------------------------------------------------------------
// DDPT (Diskette Drive Parameter Table) 関連.

#define DDPT_PARAM_NUM      11

/// セクタサイズ別のパラメータ (256/512/1024)
static const unsigned char ddpt_param[3][DDPT_PARAM_NUM] = {
    {0xdf, 0x02, 0x25, 0x01, 0x1a, 0x1b, 0xff, 0x36, 0xf6, 0x0f, 0x08}, /*256 */
    {0xdf, 0x02, 0x25, 0x02, 0x0F, 0x2a, 0xff, 0x54, 0xf6, 0x0f, 0x08}, /*512 */
    {0xdf, 0x02, 0x25, 0x03, 0x08, 0x35, 0xff, 0x74, 0xf6, 0x0f, 0x08}  /*1024 */
};

static unsigned char FAR*   ddpt_cur;
static int                  ddpt_scnt;
static unsigned char        ddpt_save[DDPT_PARAM_NUM];

#if defined(ONLY_COMPILE)
static unsigned char        ddpt_dmy[DDPT_PARAM_NUM];
#endif

/// DDPT の初期化.
///
int ddpt_init(int sectSz)
{
    int i;

    ddpt_scnt   = (sectSz ==  256) ? 0
                : (sectSz == 1024) ? 2
                : (sectSz ==  512) ? 1
                :                   -1;
    if (ddpt_scnt < 0)
        return -1;

 #if defined(ONLY_COMPILE)
    ddpt_cur   = ddpt_dmy;
 #else
    {
        uint16_t off     = DOS_PEEKW(0x00000078UL);
        uint16_t seg     = DOS_PEEKW(0x0000007AUL);
        uint32_t dos_ptr = MK_FAR_PTR(seg, off);
        ddpt_cur = (unsigned char FAR*)DOS_ADDR_TO(dos_ptr);
    }
 #endif

    for (i = 0; i < DDPT_PARAM_NUM; ++i) {
        ddpt_save[i] = ddpt_cur[i];
        ddpt_cur[i]  = ddpt_param[ddpt_scnt][i];
    }
    return 0;
}


///
void ddpt_term(void)
{
    // DDPT を元に戻す.
    int i;
    for (i = 0; i < DDPT_PARAM_NUM; i++) {
        ddpt_cur[i] = ddpt_save[i];                 /* restore FDD param. */
    }
}


// -------------------------------------------------------------------------

/// エラーのあったセクタ情報を出力
///
void saveMrk(const char *fname, char *mrk, int cylNum, int sideNum, int sectNum)
{
    char    name[260];
    FILE    *fp;
    char    *p;
    int     n;
    int     cyl;
    int     side;
    int     sect;

    strcpy(name, fname);
    p = strrchr(name, '.');
    if (p != NULL)
        *p = 0;

    strcat(name, ".imh");

    fp = fopen(name, "wt");
    if (fp == NULL) {
        fprintf(stderr, "file '%s' open error\n", name);
        return;
    }

    fprintf(fp, "   ");
    for (sect = 0; sect < sectNum; ++sect) {
        fprintf(fp, "%x", sect & 15);
    }
    fprintf(fp, "\n");
    n = 0;
    for (cyl = 0; cyl < cylNum; ++cyl) {
        for (side = 0; side < sideNum; ++side) {
            fprintf(fp, "%2d ", n);
            for (sect = 0; sect < sectNum; ++sect) {
                int i = (cyl * sideNum + side) * sectNum + sect;
                int c = mrk[ i ];
                fprintf(fp, "%c", c);
            }
            fprintf(fp, "\n");
            ++n;
        }
    }
    fclose(fp);
}

///
///
int getFD2D(const char *name, int drvNo, int cylNum, int sideNum, int sectNum, int sectSz, int retryNum)
{
    enum {
        TRKBUF_SZ = SECT_MAX * SECT_SZ,
        MRK_SZ    = CYL_MAX*2*SECT_MAX
    };
    static unsigned char trkbuf[TRKBUF_SZ];
    static char mrk[ MRK_SZ ];
    int         errSectNum;
    int         result;
    int         cyl;
    int         side;
    int         sect;
    int         retry;
    int         i;
    FILE        *fp;

    assert( 0 < cylNum && cylNum <= CYL_MAX );
    assert( drvNo == 0   || drvNo == 1 );
    assert( sideNum == 1 || sideNum == 2 );
    assert( 0 < sectNum && sectNum <= SECT_MAX );
    assert( sectSz == 256 || sectSz == 512 || sectSz == 1024 );

    fp = fopen(name, "wb");
    if (fp == NULL) {
        fprintf(stderr, "file '%s' open error\n", name);
        return -1;
    }

    result = ddpt_init(sectSz);
    if (result) {
        fclose(fp);
        return -1;
    }

    //x result = bios_disk(_DISK_RESET, drvNo, 0, 0, 1, 1, NULL);
    result = bios_disk(_DISK_RESET, drvNo, 0, 0, 0, 0, NULL);
    if ((result & 0xff00) != 0) {
        fprintf(stderr, "Error reseting disk %d\n", result);
        ddpt_term();
        fclose(fp);
        return -1;
    }

    memset(mrk, 'x', MRK_SZ);
    errSectNum = 0;
    for (cyl = 0; cyl < cylNum; ++cyl) {
        memset(trkbuf, 0x0, TRKBUF_SZ);
        for (side = 0; side < sideNum; ++side) {
            int rdNum = 0;
            int erNum = 0;
            for (retry = 0; retry < retryNum; ++retry) {
                for (sect = 0; sect < sectNum; ++sect) {
                    char          *pMrk = &mrk[ (cyl*sideNum+side)*sectNum + sect ];
                    unsigned char *buf  = &trkbuf[sect * sectSz];
                    if (*pMrk == 'x') {
                        //x memset(buf, 0xFF, sectSz);
                        result = bios_disk(_DISK_READ, drvNo, side, cyl, sect+1, 1/*nsect*/, buf);
                        if (result == 0) {  // ok
                            *pMrk = '.';
                            ++rdNum;
                        }
                    }
                }
                if (rdNum == sectNum)
                    break;
            }
            printf("T%02d: read %2d sector(s).", cyl*2+side, rdNum);
            erNum = sectNum - rdNum;
            assert( erNum >= 0 );
            if (erNum)
                printf("\t(error:%2d sector(s))", erNum);
            printf("\n");
            //x fseek( fp, (cyl*sideNum + side)*sectNum*sectSize, 0 );
            fwrite(trkbuf, 1, sectNum*sectSz, fp);
            errSectNum += erNum;
        }
    }
    ddpt_term();
    fclose(fp);

    if (errSectNum == 0)
        printf("ok\n");
    else if (errSectNum > 0)
        printf("%3d error sectors\n", errSectNum);

    saveMrk(name, (char*)mrk, cylNum, sideNum, sectNum);

    return 0;
}


// -------------------------------------------------------------------------

///
///
int main(int argc, char *argv[])
{
    int     i;
    int     drvNo     =   1;
    int     cylNum    =  41;
    int     sideNum   =   2;
    int     sectNum   =  16;
    int     sectSz    = 256;
    int     retryNum  =   5;
    const char  *name = NULL;

    if (argc < 2)
        return usage();

    // オプション取得
    for (i = 1; i < argc; ++i) {
        const char *p = argv[i];
        if (stricmp(p, "-2d") == 0) {
            cylNum    = 40, sectNum   = 16, sectSz    =  256;
        } else if (stricmp(p, "-2d1") == 0) {
            cylNum    = 41, sectNum   = 16, sectSz    =  256;
        } else if (stricmp(p, "-2dd") == 0) {
            cylNum    = 80, sectNum   = 16, sectSz    =  256;
        } else if (stricmp(p, "-2hd") == 0) {
            cylNum    = 77, sectNum   = 26, sectSz    =  256;
        } else if (stricmp(p, "-2hd1024") == 0) {
            cylNum    = 77, sectNum   =  8, sectSz    = 1024;
        } else if (stricmp(p, "-xdf") == 0) {
            cylNum    = 77, sectNum   =  8, sectSz    = 1024;
        } else if (stricmp(p, "-1d") == 0) {
            cylNum    = 40, sectNum   = 16, sectSz    =  256;
            sideNum   = 1;
        } else if (stricmp(p, "-reset") == 0) {
            ddpt_init(512);
        } else if (*p == '-' && p[1] == 'r') {
            retryNum = (int)strtoul(p+2, 0, 10);
            if (retryNum <= 0)
                retryNum = 1;
            else if (retryNum > 16)
                retryNum = 16;
        } else if (*p == '-' && p[1] == 'c') {
            cylNum = (int)strtoul(p+2, 0, 10);
            if (cylNum <= 0)
                cylNum = 1;
            else if (cylNum > 85)
                cylNum = 85;
        } else if (*p == '-' && p[1] == 's' && p[2] == 'n') {
            sectNum = (int)strtoul(p+3, 0, 0);
            if (sectNum <= 0 || sectNum >= 32) {
                fprintf(stderr, "error: -sn%d (1..31)\n", sectNum);
                return 1;
            }

        } else if (*p == '-' && p[1] == 's' && p[2] == 's') {
            sectSz = (int)strtoul(p+3, 0, 0);
            if (sectSz != 256 && sectSz != 512 && sectSz != 1024) {
                fprintf(stderr, "error: -ss%d (-ss256 , -ss512 or -ss1024)\n", sectSz);
                return 1;
            }

        } else if (stricmp(p, "a:") == 0) {
            drvNo = 0;
        } else if (stricmp(p, "b:") == 0) {
            drvNo = 1;
        } else if (name == NULL) {
            name = p;
        }
    }

    if (name == NULL)
        return usage();

    return getFD2D(name, drvNo, cylNum, sideNum, sectNum, sectSz, retryNum);
}
