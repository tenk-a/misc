/**
 *  @file   cmisc.h
 *  @brief  misc for c
 *  @author Masashi Kitamura (tenka@6809.net)
 *  @license Boost Software Lisence Version 1.0
 */

#ifndef CMISC_H_INCLUDE__
#define CMISC_H_INCLUDE__

#include <stddef.h>
#include <string.h>

//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

#if defined(_WIN32) || defined(_DOS)
#define FILE_DIR_SEP        '\\'
#define FILE_IS_DIR_SEP(c)  ((c) == '/' || (c) == '\\')
#else
#define FILE_DIR_SEP    '/'
#define FILE_IS_DIR_SEP(c)  ((c) == '/')
#endif

void        str_replace(char str[], char old_c, char new_c);
char*       str_trimSpcR(char str[], unsigned flags);
//char*     strUpLow(char str[], unsigned flags);

char*       fname_base(char const* p);
char const* fname_ext( char const* p);
char*       fname_removeDirSep(char path[]);
char*       fname_addDirSep(char path[], size_t size);

size_t      file_size( char const* fpath);
int         file_exist(char const* fpath);
size_t      file_load( char const* fname, void* dst, size_t bytes, size_t max_bytes);
void*       file_loadMalloc(char const* fname, size_t* pSize, size_t max_size);

int         file_recursive_mkdir(char const* fpath, int pmode);

struct mbc_enc_st;
size_t      strConvTab(struct mbc_enc_st const* mbc, char *dst, size_t dstSz
                , const char *src, int dstTabSz, int srcTabSz, unsigned flags);

//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -


#endif  // CMISC_H
