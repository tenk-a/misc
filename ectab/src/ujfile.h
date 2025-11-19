/**
 *  @file   ujfile.h
 *  @brief  utf8 text file read.(convert sjis,eucjp,utf16,utf32 to utf8)
 *  @author Masashi Kitamura (tenka@6809.net)
 *  @license Boost Software Lisence Version 1.0
 */

#ifndef UJFILE_H_INCLUDED__
#define UJFILE_H_INCLUDED__

#include <stddef.h>
#include <string.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct ujfile_opts_t {
    unsigned short  src_cp;         // > 0 win codepage  0=auto
    unsigned short  dst_cp;         // > 0 win codepage  0=auto
    unsigned char   remove_bom;     // Remove BOM if UNICODE.
    unsigned char   crlf_to_lf;     // 0:none  1:CRLF to LF  2:CR,CRLF to LF
    unsigned char   opt_getline;    // bit0:No line-break  bit1:CR as line-break
} ujfile_opts_t;

typedef struct ujfile_t {
    char*           malloc_buf;
    char*           end;
    char*           curpos;
    unsigned short  cur_cp;         // code page.
    unsigned short  src_cp;         //
    int             unget_c;
    unsigned char   has_bom;
    unsigned char   unkown_enc;
    ujfile_opts_t   opts;
 #ifndef NDEBUG
    char const*     fname;
 #endif
} ujfile_t;

ujfile_t*   ujfile_open(char const* fname, ujfile_opts_t const* opts);
void        ujfile_close(ujfile_t** pUj);
size_t      ujfile_getline(ujfile_t* uj, char* buf, size_t bufSz);
char const* ujfile_getCurrentLine(ujfile_t* uj, size_t* pSize, size_t* pCrlfSize);
size_t      ujfile_currentLineSize(ujfile_t* uj, size_t* crlfSize);

static inline int   ujfile_get1(ujfile_t* uj) { return (uj->curpos < uj->end) ? *uj->curpos++ : -1; }
static inline void  ujfile_unget1(ujfile_t* uj) { if (uj->curpos > uj->malloc_buf) --uj->curpos; }
static inline int   ujfile_eof(ujfile_t* uj) { return (uj->curpos >= uj->end); }

static inline size_t    ujfile_size(  ujfile_t* uj) { return uj->end - uj->malloc_buf; }
static inline char*     ujfile_buffer(ujfile_t* uj) { return uj->malloc_buf; }
static inline int       ujfile_curCP( ujfile_t* uj) { return uj->cur_cp; }  // code page
static inline int       ujfile_srcCP( ujfile_t* uj) { return uj->src_cp; }  // code page
static inline int       ujfile_hasBOM(ujfile_t* uj) { return uj->has_bom > 0; }
static inline int       ujfile_unkownEnc(ujfile_t* uj) { return uj->unkown_enc != 0; }
static inline ujfile_opts_t* ujfile_opts(ujfile_t* uj) { return &uj->opts; }

#ifndef UJFILE_FOPEN_CODE_PAGE
//#define UJFILE_FOPEN_CODE_PAGE    0
#define UJFILE_FOPEN_CODE_PAGE      65001   // convert to utf-8
#endif

// std c lib like.
ujfile_t*   ujfile_fopen(char const* fname, char const* mode/*"rbt"*/);
char*       ujfile_fgets(char* buf, size_t bufSz, ujfile_t* uj);
void        ujfile_fclose(ujfile_t* uj);
int         ujfile_fgetc(ujfile_t* uj);
int         ujfile_ungetc(int c, ujfile_t* uj);
ptrdiff_t   ujfile_fseek(ujfile_t* uj, ptrdiff_t offset, int origin);
static inline ptrdiff_t ujfile_ftell(ujfile_t* uj) { return uj->curpos - uj->malloc_buf; }
static inline int  ujfile_feof(ujfile_t* uj) { return ujfile_eof(uj); }
static inline int  ujfile_ferror(ujfile_t* uj) { return 0; }


#if defined(__cplusplus)
}
#endif

#endif  // CMISC_H
