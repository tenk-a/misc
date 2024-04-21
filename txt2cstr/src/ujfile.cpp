/**
 *  @file   cmisc.c
 *  @brief  misc for c
 *  @author Masashi Kitamura (tenka@6809.net)
 *  @license Boost Software Lisence Version 1.0
 */

#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "ujfile.h"
#include "mbc.h"

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

static size_t fileBytes(const char* fpath)
{
 #if defined(_WIN32) || defined(_DOS)
    struct _stat st;
    int   rc = _stat(fpath, &st);
 #else
    struct stat st;
    int   rc = stat(fpath, &st);
 #endif
    return (rc == 0) ? (size_t)st.st_size : (size_t)-1;
}

static size_t fileLoad(char const* fname, void* dst, size_t bytes)
{
	size_t rbytes;
    if (dst == NULL || bytes == 0)
        return 0;
	if (fname) {
     #if defined(_WIN32) || defined(_DOS)
        int fd = _open(fname, _O_RDONLY|_O_BINARY);
        if (fd == -1)
            return 0;
        rbytes = _read(fd, dst, bytes);
        _close(fd);
     #else
        int fd = open(fname, O_RDONLY);
        if (fd == -1)
            return 0;
        rbytes = read(fd, dst, bytes);
        close(fd);
     #endif
	} else {
        rbytes = read(0, dst, bytes);
		
	}
    return rbytes;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

static ujfile_t* ujfile_set(ujfile_t* uj, char* malloc_buf, size_t size, char const* fname, ujfile_opts_t const* opts);


/** utf8 text file open. (sjis,eucjp,utf16,utf32 to utf8)
 */
ujfile_t* ujfile_fopen(char const* fname, char const* mode)
{
	ujfile_opts_t opts = { 0, MBC_CP_UTF8, 1, 0 };
	while (*mode) {
		int c = *mode++;
		if (c == 'b')
			opts.crlf_to_lf = 0;
		else if (c == 't')
			opts.crlf_to_lf = 1;
	}
	return ujfile_open(fname, &opts);
}

/** Text file open. (sjis,eucjp,utf16,utf32 to utf8,sjis,eucjp)
 */
ujfile_t* ujfile_open(char const* fname, ujfile_opts_t const* opts)
{
	size_t size = fileBytes(fname);
	if (size != (size_t)-1) {
		ujfile_t* uj = (ujfile_t*)calloc(1, sizeof(ujfile_t));
		if (uj != NULL) {
			char* buf = (char*)malloc(size + 1);
			uj->malloc_buf = buf;
			if (buf != NULL) {
				size_t rd = fileLoad(fname, buf, size);
				if (rd == size) {
					buf[size] = 0;
					if (ujfile_set(uj, buf, size, fname, opts) != NULL)
						return uj;
				}
			}
			ujfile_fclose(uj);
		}
	}
	return NULL;
}

/** File
 */
static ujfile_t* 	ujfile_set(ujfile_t* uj, char* malloc_buf, size_t size, char const* fname, ujfile_opts_t const* opts)
{
	int			dstCP, curCP;
	unsigned	bomSize = 0;
	char*		src   = malloc_buf;
	mbc_enc_t	curEnc;
	assert(uj != NULL);
	if (opts)
		uj->opts = *opts;
	dstCP 		= opts->dst_cp;
	curEnc		= (opts->src_cp) ? mbc_cpToEnc((mbc_cp_t)opts->src_cp)
		            : mbc_autoEncodeCheck(src, size, 0, NULL, 0);
	if (curEnc == NULL) {
		curEnc  = mbc_cpToEnc(MBC_CP_UTF8);
		uj->unkown_enc = 1;
	}
	curCP 		= curEnc->cp;
	uj->src_cp  = curCP;
	uj->cur_cp  = curCP;

	if (dstCP == 0 && (curCP == MBC_CP_UTF16LE || curCP == MBC_CP_UTF16BE
					|| curCP == MBC_CP_UTF32LE || curCP == MBC_CP_UTF32BE)
	) {
		dstCP = MBC_CP_UTF8;
	}

	if (mbc_cpIsUnicode((mbc_cp_t)curCP)) {
		bomSize = mbc_getBOMbytes(src, size);
		if (opts->remove_bom) {
			src  += bomSize;
			size -= bomSize;
		}
	}
	uj->has_bom = (unsigned char)bomSize;
	if (curCP != dstCP && dstCP > 0 ) {
		mbc_enc_t	dstEnc  = mbc_cpToEnc((mbc_cp_t)dstCP);
		size_t		dstSize = 0;
		char*		dst 	= mbc_strConvMalloc(dstEnc, curEnc, src, size, &dstSize);
		free(malloc_buf);
		if (dst == NULL)
			return NULL;
		malloc_buf	= dst;
		src			= dst;
        size        = dstSize;
        uj->cur_cp  = curCP = dstCP;
	}
	if (opts->crlf_to_lf) {
		int			crToLf = (opts->crlf_to_lf >= 2);
		char*       d = src;
		char const* s = d;
		char const* e = s + size;
		while (s < e) {
			char c = *s++;
			if (c == '\r') {
				if (s < e && *s == '\n') {
					++s;
					c = '\n';
				} else if (crToLf) {
					c = '\n';
				}
			}
			*d++ = c;
		}
		if (d < s)
			*d = 0;
		size = d - malloc_buf;
	}
	uj->malloc_buf	= malloc_buf;
	uj->curpos		= src;
	uj->end 		= src + size;
 #ifndef NDEBUG
	uj->fname 		= fname;
 #endif
	return uj;
}

/** File close (ptr to null)
 */
void 	ujfile_close(ujfile_t** uj)
{
	if (uj) {
		ujfile_fclose(*uj);
		*uj = NULL;
	}
}

/** File close
 */
void 	ujfile_fclose(ujfile_t* uj)
{
	if (uj) {
		if (uj->malloc_buf)
			free(uj->malloc_buf);
		free(uj);
	}
}

/** Get current line size(bytes)
 */
size_t	ujfile_currentLineSize(ujfile_t* uj, size_t* crlfSize)
{
    unsigned crAsLf = (uj->opts.opt_getline & 2) != 0;
	char const* b   = uj->curpos;
	char const* s   = b;
	char const* e   = uj->end;
	if (crlfSize)
		*crlfSize = 0;
	while (s < e) {
		unsigned c = *s++;
	    if (c == '\n') {
			if (crlfSize)
				*crlfSize = 1;
	    	break;
	    } else if (c == '\r') {
			c = *s++;
	    	if (c == '\n') {
				if (crlfSize)
					*crlfSize = 2;
    	    	break;
	    	} else {
				--s;
				if (crAsLf) {
					if (crlfSize)
						*crlfSize = 1;
	    	    	break;
	    	    }
	    	}
	    }
    }
    return s - b;
}

/** Get cullent line (curpos update)
 */
char const*	ujfile_getCurrentLine(ujfile_t* uj, size_t* pSize, size_t* pCrlfSize)
{
	size_t size  = ujfile_currentLineSize(uj, pCrlfSize);
	char* curpos = uj->curpos;
	//if (update)
	uj->curpos += size;
	if (pSize)
		*pSize = size;
	return curpos;
}

/** get line ('\n','\r\n'). 
 *  @return read bytes  error:(size_t)-1
 *  	    
 */
size_t ujfile_getline(ujfile_t* uj, char* buf, size_t bufSz)
{
	char*	 dst          = buf;
	size_t	 crlfSize     = 0;
	int 	 hasLineBreak = (uj->opts.opt_getline & 1) == 0;
	size_t	 bufSzM1      = bufSz - 1;
	size_t	 l;
	assert(bufSz > 1);

	l = ujfile_currentLineSize(uj, &crlfSize);
	if (l - crlfSize + hasLineBreak < bufSzM1) {
		size_t n       = l - crlfSize;
		if (n)
			memcpy(dst, uj->curpos, n);
		uj->curpos += l;
		dst += n;
		if (hasLineBreak)
			*dst++ = '\n';
		*dst = 0;
		return dst - buf;
	} else {
		memcpy(dst, uj->curpos, bufSzM1);
		uj->curpos += bufSzM1;
		dst += bufSzM1;
		*dst = 0;
	}
	return (size_t)-1;
}

/** like fgets
 */
char*	ujfile_fgets(char* buf, size_t size, ujfile_t* uj)
{
	size_t n = ujfile_getline(uj, buf, size);
	return (n != (size_t)-1) ? buf : NULL;
}

