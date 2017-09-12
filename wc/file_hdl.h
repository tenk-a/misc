/**
 *  @file   file_hdl.h
 *  @brief  ファイルハンドルを用いる ファイルapi の薄いラッパー.
 *  @author tenk* (Masashi Kitamura)
 *  @note
 *  -   win と linux(unix)系を対象(想定).
 *  -   基本的に unix 系の関数仕様に似せている.
 *      このため成功/失敗は 0/負値 にしているので注意.
 *  -   コピペ利用の元ネタとしても想定しているため、型名等はos固有のまま記述.
 *  -   winのHANDLEで標準入出力は0,1,2ではないので、専用名を使う必要あり.
 */

#ifndef FILE_HDL_H
#define FILE_HDL_H

#include <stddef.h>
#if defined _WIN32
 #include <windows.h>
 #include <tchar.h>
 #include <malloc.h>
 #ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable:4996)
  #pragma warning(disable:4189)
 #endif
#else   // linux,unix系.
 #define _LARGEFILE64_SOURCE        // include順番を気をつけないと乗っ取れない...
 #define _FILE_OFFSET_BITS  64      // include順番を気をつけないと乗っ取れない...
// #include <unistd.h>
 #include <fcntl.h>
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <sys/time.h>
 #include <stdint.h>
 #include <string.h>
 #define  TCHAR     char
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>


#ifdef FILE_HDL_NS
namespace FILE_HDL_NS {
#endif

#if defined _WIN32  // --------------------------------------------------------

#if !defined(__cplusplus) && defined(_MSC_VER)
 #define inline __inline
#endif

// 使う側が移植性を考慮する場合に利用する型名等.
typedef HANDLE              FILE_HDL;
typedef unsigned __int64    file_size64_t;
typedef __int64             file_off_t;
typedef unsigned __int64    file_time_t;
static const  HANDLE        FILE_ERR_HDL = INVALID_HANDLE_VALUE;
static inline HANDLE        FILE_HDL_STDIN()  { return GetStdHandle(STD_INPUT_HANDLE); }
static inline HANDLE        FILE_HDL_STDOUT() { return GetStdHandle(STD_OUTPUT_HANDLE); }
static inline HANDLE        FILE_HDL_STDERR() { return GetStdHandle(STD_ERROR_HANDLE); }

// ファイルオープン, r,w,rp,wp はfopenの"rb","wb","rb+","wb+"に相応.
static inline HANDLE    file_open_r (const TCHAR* nm) { assert(nm); return CreateFile(nm, GENERIC_READ , FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0); }
static inline HANDLE    file_open_w (const TCHAR* nm) { assert(nm); return CreateFile(nm, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0); }
static inline HANDLE    file_open_rp(const TCHAR* nm) { assert(nm); return CreateFile(nm, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0); }
static inline HANDLE    file_open_wp(const TCHAR* nm) { assert(nm); return CreateFile(nm, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0); }

static inline void  file_close(HANDLE hdl)                         { if (hdl != INVALID_HANDLE_VALUE) CloseHandle(hdl); }
static inline size_t    file_read(HANDLE h, void* b, size_t sz)        { DWORD r=0; assert(h!=INVALID_HANDLE_VALUE&&b&&sz); if (!ReadFile(h,b,sz,&r,0)) r=0; return r; }
static inline size_t    file_write(HANDLE h, const void* b, size_t sz) { DWORD r=0; assert(h!=INVALID_HANDLE_VALUE&&b&&sz); if (!WriteFile(h,b,sz,&r,0)) r=0; return r; }
static inline int       file_flush(HANDLE h)                           { assert(h!=INVALID_HANDLE_VALUE); return FlushFileBuffers(h) ? 0 : -1; }
#if (_WIN32_WINNT >= 0x0500)
static inline __int64   file_seek(HANDLE h, __int64 ofs, int mode)     { assert(h!=INVALID_HANDLE_VALUE); return SetFilePointerEx(h, *(LARGE_INTEGER*)&ofs, (LARGE_INTEGER*)&ofs, mode) ? ofs : 0; }
static inline unsigned __int64 file_getSize(HANDLE h)                  { unsigned __int64 l = 0; assert(h!=INVALID_HANDLE_VALUE); return GetFileSizeEx(h, (LARGE_INTEGER*)&l) ? l : 0; }
#else
static inline __int64   file_seek(HANDLE h, __int64 ofs, int mode)     { assert(h!=INVALID_HANDLE_VALUE); return SetFilePointer(h, (LONG)ofs, 0, mode); }
static inline unsigned __int64 file_getSize(HANDLE h)                  { DWORD m = 0, l; assert(h!=INVALID_HANDLE_VALUE); l = GetFileSize(h, (DWORD*)&m); return ((unsigned __int64)m<<32)|l; }
#endif
static inline __int64   file_tell(HANDLE h)                            { assert(h!=INVALID_HANDLE_VALUE); return file_seek(h, 0, FILE_CURRENT); }

// 時間の取得. 成功したら 0, 失態したら負を返す.
static inline int       file_getTime(HANDLE h, unsigned __int64* pCreat, unsigned __int64* pLastAcs, unsigned __int64* pLastWrt) {
    assert(h != INVALID_HANDLE_VALUE);
    return GetFileTime(h, (FILETIME*)pCreat, (FILETIME*)pLastAcs, (FILETIME*)pLastWrt) ? 0 : -1;
}

// 時間の設定. 成功したら 0, 失態したら負を返す.
static inline int       file_setTime(HANDLE h, unsigned __int64 creatTim, unsigned __int64 lastAcs, unsigned __int64 lastWrt) {
    assert(h != INVALID_HANDLE_VALUE);
    return SetFileTime(h, (FILETIME*)&creatTim, (FILETIME*)&lastAcs, (FILETIME*)&lastWrt) ? 0 : -1;
}

#else   // linux(unix) //----------------------------------------------------

// 使う側が移植性を考慮する場合に利用する型名等.
typedef int             FILE_HDL;
typedef uint64_t        file_size64_t;
typedef off64_t         file_off_t;
typedef time_t          file_time_t;

enum { FILE_ERR_HDL = -1 };
static inline int FILE_HDL_STDIN()  { return 0; }
static inline int FILE_HDL_STDOUT() { return 1; }
static inline int FILE_HDL_STDERR() { return 2; }

// ファイルオープン, r,w,rp,wp はfopenの"rb","wb","rb+","wb+"に相応.
static inline int       file_open_r (const char* nm) { assert(nm); return open(nm, O_RDONLY, 0766); }
static inline int       file_open_w (const char* nm) { assert(nm); return open(nm, O_WRONLY|O_CREAT|O_TRUNC, 0766); }
static inline int       file_open_rp(const char* nm) { assert(nm); return open(nm, O_RDWR, 0766); }
static inline int       file_open_wp(const char* nm) { assert(nm); return open(nm, O_RDWR|O_CREAT, 0766); }

static inline void      file_close(int hdl)                           { if (hdl != -1) close(hdl); }
static inline size_t    file_read(int h, void* buf, size_t sz)        { assert(h!=-1 && buf && sz); return read(h, buf, sz); }
static inline size_t    file_write(int h, const void* buf, size_t sz) { assert(h!=-1 && buf && sz); return write(h, buf, sz); }
static inline int       file_flush(int h)                             { return 0; } // ダミー.

// ほんとに 64ビットを有効になっているかは環境の設定しだい.
static inline int64_t   file_seek(int h, int64_t ofs, int mode) { assert(h!=-1); return lseek64(h,ofs,mode); }
static inline int64_t   file_tell(int h)                        { assert(h!=-1); return file_seek(h, 0, 1/*seek_cur*/); }
static inline uint64_t file_getSize(int h)                  { assert(h!=-1); struct stat st; return fstat(h, &st)==0 ? st.st_size : 0; }

/// 時間の取得. 値はシステムに依存.
static inline int       file_getTime(int h, time_t* pCreat, time_t* pLastAcs, time_t* pLastWrt) {
    struct stat st;
    int         rc;
    assert(h != 0);
    rc = fstat(h, &st);
    if (rc == 0) {
        if (pLastWrt) *pLastWrt = st.st_mtime;
        if (pLastAcs) *pLastAcs = st.st_atime;
        if (pCreat  ) *pCreat   = st.st_ctime;
    }
    return rc;
}

/// 時間の設定. creatTimは無視するので注意.
static inline int       file_setTime(int h, time_t creatTim, time_t lastAcs, time_t lastWrt) {
    struct timeval tv[2];
    assert(h != 0);
    //creatTim;
    tv[0].tv_sec  = lastAcs;
    tv[0].tv_usec = 0;
    tv[1].tv_sec  = lastWrt;
    tv[1].tv_usec = 0;
    return futimes(h, tv);
}

#endif  //  -------------------------------------------------------

#ifdef FILE_HDL_NS
}
#endif


// ---------------------------------------------------------------------------
// テキスト出力

#ifdef FILE_HDL_NS
namespace FILE_HDL_NS {
#endif

// 文字列出力. ※ winでは \n を\r\nにして出力. ※簡易版としてなのでencodeはos依存.
static inline size_t    file_puts(const TCHAR* str, FILE_HDL h) {
 #if defined _WIN32
  #ifdef UNICODE
   #if 0    // UTF-16のまま出力するとき.
    typedef TCHAR   Char;
    const Char* s   = str;
   #else    // CP932へ変換してから出力するとき.
    enum { N = 2048 };
    typedef char    Char;
    enum { CP = 0 /* CP_UTF8 */ };
    Char            buf[ N + 1 ];
    size_t          slen = wcslen(str);
    size_t          blen = WideCharToMultiByte(CP, 0, str, slen, NULL, 0, NULL, NULL);
    Char*           s    = buf;
    if (blen > N) {
        s = (Char*)alloca(blen+1);
        if (s == NULL) {
            blen = N;
            s    = buf;
        }
    }
    WideCharToMultiByte(CP, 0, str, slen, s, blen, NULL, NULL);
    s[blen] = 0;
   #endif
  #else
    typedef TCHAR   Char;
    const Char* s   = str;
  #endif
    {
        enum { L = 4096 };
        Char         buf[L + 4];
        Char*        d  = buf;
        Char*        e  = d + L;
        size_t   sz = 0;
        while (*s) {
            Char  c = *s++;
            *d++ = c;
            if (c == '\r' && *s == '\n') {
                *d++ = *s++;
            } else if (c == '\n') {
                d[-1] = '\r';
                *d++  = c;
            }
            if (d >= e) {
                size_t l = d - buf;
                size_t r = file_write(h, buf, l*sizeof(Char));
                if (r != l * sizeof(Char))
                    return 0;
                sz += l;
                d = buf;
            }
        }
        if (d > buf) {
            size_t l = d - buf;
            size_t r = file_write(h, buf, l*sizeof(Char));
            if (r != l * sizeof(Char))
                return 0;
            sz += l;
        }
        return sz;
    }
 #else
    return file_write(h, s, strlen(s));
 #endif
}


// printf書式の文字列出力.
static inline size_t    file_vprintf(FILE_HDL h, const TCHAR* fmt, va_list a) {
    TCHAR   buf[ 2048 ];
    TCHAR*  s = buf;
    TCHAR*  p = s;
    size_t  bufSz = 2048;
    int     n;
    assert(h!=FILE_ERR_HDL && fmt);
    do {
        s = p;
     #if defined UNICODE
       #ifdef _WIN32
        n = _vsnwprintf(s, bufSz, fmt, a);
       #else
        n = vsnwprintf (s, bufSz, fmt, a);
       #endif
     #else
       #ifdef _WIN32
        n = _vsnprintf (s, bufSz, fmt, a);
       #else
        n = vsnprintf (s, bufSz, fmt, a);
       #endif
     #endif
    } while (n >= (int)(bufSz)-1 && (p = (TCHAR*)alloca((bufSz * 2)*sizeof(TCHAR))) != 0 && (bufSz *= 2));
    n = (n < 0) ? 0 : (n > (int)bufSz-1) ? (int)bufSz-1 : n;
    s[n] = 0;
    return file_puts(s, h);
}


// printf書式の文字列出力.
static inline size_t    file_printf(FILE_HDL h, const TCHAR* fmt, ...) {
    size_t  n;
    va_list  a;
    va_start(a,fmt);
    n = file_vprintf(h, fmt, a);
    va_end(a);
    return n;
}


#ifdef _WIN32   // win-apiの wsprintf を用いた簡易な標準出力printf. 結果の文字数が1024バイト未満のこと.
#define FILE_STDOUT_PRINTF(...) do { char bUf[1030]; wsprintfA(bUf, __VA_ARGS__); file_puts(bUf, FILE_HDL_STDOUT()); } while (0)
#else
#define FILE_STDOUT_PRINTF      printf
#endif

// ---------------------------------------------------------------------------
// c++ の時の、ハンドルをラップしたクラス

#ifdef __cplusplus

class FileHdl {
public:
    enum OpenMode { R,W,RP,WP };

    FileHdl() : hdl_(FILE_ERR_HDL) {}
    explicit FileHdl(const TCHAR* nm, OpenMode md=R) : hdl_(FILE_ERR_HDL) { open(nm, md); }
    ~FileHdl() { close(); }

    bool            is_open() const { return hdl_ != FILE_ERR_HDL; }
    bool            open_r (const TCHAR* nm) { hdl_ = file_open_r (nm); return hdl_ != FILE_ERR_HDL; }
    bool            open_w (const TCHAR* nm) { hdl_ = file_open_w (nm); return hdl_ != FILE_ERR_HDL; }
    bool            open_rp(const TCHAR* nm) { hdl_ = file_open_rp(nm); return hdl_ != FILE_ERR_HDL; }
    bool            open_wp(const TCHAR* nm) { hdl_ = file_open_wp(nm); return hdl_ != FILE_ERR_HDL; }

    bool            open(const TCHAR* nm, OpenMode md=R) {
                        assert(hdl_ == FILE_ERR_HDL);
                        switch (md) {
                        case R : hdl_ = file_open_r (nm); break;
                        case W : hdl_ = file_open_w (nm); break;
                        case RP: hdl_ = file_open_rp(nm); break;
                        case WP: hdl_ = file_open_wp(nm); break;
                        default: assert(0);
                        }
                        return hdl_ != FILE_ERR_HDL;
                    }

    void            close() { if (hdl_ != FILE_ERR_HDL) {file_close(hdl_);}  hdl_ = FILE_ERR_HDL; }

    file_size64_t   size() { return file_getSize(hdl_); }
    size_t          read(void* b, size_t sz) { return file_read(hdl_, b, sz); }
    size_t          write(const void* b, size_t sz) { return file_write(hdl_, b, sz); }
    int             flush() { return file_flush(hdl_); }
    file_size64_t   seek(file_size64_t ofs, int mode) { return file_seek(hdl_, ofs, mode); }
    file_size64_t   tell() { return file_tell(hdl_); }

    file_size64_t   getTime(file_time_t* pCreat, file_time_t* pLastAcs, file_time_t* pLastWrt) {
                        return file_getTime(hdl_, pCreat, pLastAcs, pLastWrt);
                    }
    int             setTime(file_time_t creatTim, file_time_t lastAcs, file_time_t lastWrt) {
                        return file_setTime(hdl_, creatTim, lastAcs, lastWrt);
                    }

    size_t          puts(const TCHAR* str) { return file_puts(str, hdl_); }
    template<class STRING>
    size_t          puts(const STRING& str) { return file_puts(str.c_str(), hdl_); }

    size_t          printf(const TCHAR* fmt, ...) {
                        va_list  a;
                        va_start(a,fmt);
                        size_t n = file_vprintf(hdl_, fmt, a);
                        va_end(a);
                        return n;
                    }

    FILE_HDL        get_handle() const { return hdl_;}

private:
    FILE_HDL        hdl_;
};

#endif  // __cplusplus


#ifdef FILE_HDL_NS
}
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif  // FILE_HDL_H
