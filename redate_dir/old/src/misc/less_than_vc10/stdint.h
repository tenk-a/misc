/**
 *  @file   stdint.h
 *  @brief  êÆêîå^íËã` for vc
 *  @date   2000-10
 */

#ifndef STDINT_H_INCLUDED
#define STDINT_H_INCLUDED

#pragma once

typedef __int8	    	    int8_t;
typedef unsigned __int8     uint8_t;
typedef __int16     	    int16_t;
typedef unsigned __int16    uint16_t;
typedef __int32     	    int32_t;
typedef unsigned __int32    uint32_t;
typedef __int64     	    int64_t;
typedef unsigned __int64    uint64_t;

typedef __int64     	    intmax_t;
typedef unsigned __int64    uintmax_t;

#if defined(_WIN64)
typedef __int64     	    intptr_t;
typedef unsigned __int64    uintptr_t;
#else
typedef __int32     	    intptr_t;
typedef unsigned __int32    uintptr_t;
#endif

typedef __int8	    	    int_least8_t;
typedef unsigned __int8     uint_least8_t;
typedef __int16     	    int_least16_t;
typedef unsigned __int16    uint_least16_t;
typedef __int32     	    int_least32_t;
typedef unsigned __int32    uint_least32_t;
typedef __int64     	    int_least64_t;
typedef unsigned __int64    uint_least64_t;

typedef __int8	    	    int_fast8_t;
typedef unsigned __int8     uint_fast8_t;
typedef __int16     	    int_fast16_t;
typedef unsigned __int16    uint_fast16_t;
typedef __int32     	    int_fast32_t;
typedef unsigned __int32    uint_fast32_t;
typedef __int64     	    int_fast64_t;
typedef unsigned __int64    uint_fast64_t;



#if !defined(__cplusplus) || defined(__STDC_LIMIT_MACROS)
    // Å¶Å@ c++Ç≈ÇÕ __STDC_LIMIT_MACROS Ç™íËã`Ç≥ÇÍÇƒÇ¢ÇÈèÍçáÇÃÇ›

#define INT8_MIN    	    (-128)
#define INT8_MAX    	      127
#define UINT8_MAX   	     0xFF

#define INT16_MIN   	    (-32768)
#define INT16_MAX   	      32767
#define UINT16_MAX  	     0xFFFF

#define INT32_MIN   	    (-2147483648)
#define INT32_MAX   	      2147483647
#define UINT32_MAX  	      0xFFFFFFFF

#define INT64_MIN   	    (-9223372036854775808LL)
#define INT64_MAX   	      9223372036854775807LL
#define UINT64_MAX  	       0xFFFFFFFFFFFFFFFFLL

#define INTMAX_MIN  	    INT64_MIN
#define INTMAX_MAX  	    INT64_MAX
#define UINTMAX_MAX 	    UINT64_MAX

#if defined(_WIN64)
#define INTPTR_MIN  	    INT64_MIN
#define INTPTR_MAX  	    INT64_MAX
#define UINTPTR_MAX 	    UINT64_MAX
#define PTRDIFF_MIN 	    INT64_MIN
#define PTRDIFF_MAX 	    INT64_MAX
#else
#define INTPTR_MIN  	    INT32_MIN
#define INTPTR_MAX  	    INT32_MAX
#define UINTPTR_MAX 	    UINT32_MAX
#define PTRDIFF_MIN 	    INT32_MIN
#define PTRDIFF_MAX 	    INT32_MAX
#endif

#define INT_LEAST8_MIN	    INT8_MIN
#define INT_LEAST8_MAX	    INT8_MAX
#define UINT_LEAST8_MAX     UINT8_MAX

#define INT_LEAST16_MIN     INT16_MIN
#define INT_LEAST16_MAX     INT16_MAX
#define UINT_LEAST16_MAX    UINT16_MAX

#define INT_LEAST32_MIN     INT32_MIN
#define INT_LEAST32_MAX     INT32_MAX
#define UINT_LEAST32_MAX    UINT32_MAX

#define INT_LEAST64_MIN     INT64_MIN
#define INT_LEAST64_MAX     INT64_MAX
#define UINT_LEAST64_MAX    UINT64_MAX


#define INT_FAST8_MIN	    INT8_MIN
#define INT_FAST8_MAX	    INT8_MAX
#define UINT_FAST8_MAX	    UINT8_MAX

#define INT_FAST16_MIN	    INT16_MIN
#define INT_FAST16_MAX	    INT16_MAX
#define UINT_FAST16_MAX     UINT16_MAX

#define INT_FAST32_MIN	    INT32_MIN
#define INT_FAST32_MAX	    INT32_MAX
#define UINT_FAST32_MAX     UINT32_MAX

#define INT_FAST64_MIN	    INT64_MIN
#define INT_FAST64_MAX	    INT64_MAX
#define UINT_FAST64_MAX     UINT64_MAX

#define SIZE_MAX    	    UINT32_MAX

#define SIG_ATOMIC_MIN	    INT32_MIN
#define SIG_ATOMIC_MAX	    INT32_MAX


#if 0 //def _WCHAR_IS_UINT32

#if !defined(WCHAR_MIN)
#define WCHAR_MIN   	    0
#endif
#if !defined(WCHAR_MAX)
#define WCHAR_MAX   	    UINT32_MAX
#endif
#define WINT_MIN    	    UINT32_MIN
#define WINT_MAX    	    UINT32_MAX

#else

#if !defined(WCHAR_MIN)
#define WCHAR_MIN   	    0
#endif
#if !defined(WCHAR_MAX)
#define WCHAR_MAX   	    UINT16_MAX
#endif
#define WINT_MIN    	    UINT16_MIN
#define WINT_MAX    	    UINT16_MAX

#endif



#define INT8_C(val) 	    val
#define UINT8_C(val)	    val
#define INT16_C(val)	    val
#define UINT16_C(val)	    val
#define INT32_C(val)	    val
#define UINT32_C(val)	    val
#define INT64_C(val)	    val##LL
#define UINT64_C(val)	    val##ULL
#define INTMAX_C(val)	    val##LL
#define UINTMAX_C(val)	    val##ULL


#endif
#endif	// STDINT_H_INCLUDED
