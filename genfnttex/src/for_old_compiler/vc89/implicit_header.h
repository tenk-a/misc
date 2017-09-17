/**
 *  @file implicit_header.h
 *  @brief  c/c++ language
 */
#ifndef IMPLICIT_HEADER_H
#define IMPLICIT_HEADER_H

#if defined(_MSC_VER)
/*
    6.0     6.0     1200
    2002    7.0     1300
    2003    7.1     1310
    2005    8.0     1400
    2008    9.0     1500
    2010    10.0    1600
    2012    11.0    1700
    2013    12.0    1800
    2015    14.0    1900
    2017    14.1    1910
*/
 #ifndef __WRaP_M_CAT
  #define __WRaP_M_CAT(a,b) 	__WRaP_M_CAT_S2(a,b)
  #define __WRaP_M_CAT_S2(a,b)	__WRaP_M_CAT_S3(a##b)
  #define __WRaP_M_CAT_S3(x)	x
 #endif

 /*
 #ifndef __WRaP_NATIVE_C_HEADER_PATH
  #if _MSC_VER >= 1400
   #define __WRaP_NATIVE_C_HEADER_PATH(x)   <../../vc/include/##x>
  #elif _MSC_VER >= 1300
   #define __WRaP_NATIVE_C_HEADER_PATH(x)   <../../vc7/include/##x>
  #elif _MSC_VER >= 1200
   #define __WRaP_NATIVE_C_HEADER_PATH(x)   <../../vc6/include/##x>
  #endif
 #endif
 */

 #if defined(__cplusplus)
  #if 1 //__cplusplus < 201101L     	// less c++11
   #if _MSC_VER < 1400
    #if !defined(override)
     #define override
    #endif
   #endif
   #if _MSC_VER < 1600
    #if !defined(__func__)
     #define __func__	    	    __FUNCTION__
    #endif
    #if !defined(_Pragma)
     #define _Pragma(...)   	    __pragma(__VA_ARGS__)
    #endif
    #if !defined(noexcept)
     #define noexcept	    	    throw()
    #endif
    #if !defined(nullptr)
     #define nullptr	    	    (0)
    #endif
    #if !defined(static_assert)
     #define static_assert(c,m)     typedef char __WRaP_M_CAT(__static_assert_failed_L,__LINE__)[(c) ? 1/*OK*/ : -1/*NG*/]
    #endif
    typedef wchar_t 	    	    char16_t;
    typedef unsigned	    	    char32_t;
   #endif
   #if _MSC_VER < 1700
    #if !defined(override)
     #define final  	    	    sealed
    #endif
   #endif
   #if _MSC_VER < 1800
    #if !defined(alignas)
     #define alignas(a)     	    __declspec(align(a))
    #endif
    #if !defined(alignof)
     #define alignof(a)     	    __alignof(a)
    #endif
   #endif
   #if _MSC_VER < 1900
    #if !defined(thread_local)
     #define thread_local   	    __declspec(thread)
    #endif
   #endif
   #if _MSC_VER < 1910
    #if !defined(__has_include)
     #define __has_include(x)	    	0
    #endif
   #endif
  #endif
 #else	// for c
  #if _MSC_VER < 1900
   #if !defined inline
    #define inline  	    	    __inline
   #endif
   #if !defined(_Alignof)
    #define _Alignof(t)     	    __alignof(t)
   #endif
  #endif
  #if 1
   #if !defined restrict
    #define restrict	    	    __restrict
   #endif
   #if !defined _Bool
    #define _Bool   	    	    char
   #endif
   #ifndef _Noreturn
    #define _Noreturn	    	    __declspec(noreturn)
   #endif
   #ifndef _Alignas
    #define _Alignas(a)     	    __declspec(align(a))
   #endif
   #ifndef _Thread_local
    #define _Thread_local   	    __declspec(thread)
   #endif
   #ifndef _Static_assert
    #define _Static_assert  	    typedef char __WRaP_M_CAT(__static_assert_failed_L,__LINE__)[(c) ? 1/*OK*/ : -1/*NG*/]
   #endif
   #define __STDC_NO_VLA__
   #define __STDC_NO_COMPLEX__
  #endif
 #endif
#else

//#ifdef __cplusplus
//#if __cplusplus < 201103L
//#endif
//#endif

#endif


#endif	// IMPLICIT_HEADER_H
