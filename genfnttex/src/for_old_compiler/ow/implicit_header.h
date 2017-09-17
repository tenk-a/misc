/**
 *  @file implicit_header.h
 *  @brief  c/c++ language
 *  @note
 *  	mada tekikou
 */
#ifndef IMPLICIT_INCLUDE_HEADER_H
#define IMPLICIT_INCLUDE_HEADER_H

#if defined(__WATCOMC__)
 #ifndef __WRaP_M_CAT
  #define __WRaP_M_CAT(a,b) 	__WRaP_M_CAT_S2(a,b)
  #define __WRaP_M_CAT_S2(a,b)	a##b
 #endif

 #if defined(__cplusplus)
    #if !defined(override)
     #define override
    #endif
    #if !defined(__func__)
     #define __func__	    	    __FUNCTION__
    #endif
    #if !defined(_Pragma)
     #define _Pragma(...)   	    //__pragma(__VA_ARGS__)
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
    #if !defined(override)
     #define final
    #endif
    #if !defined(alignas)
     #define alignas(a)     	    //__declspec(align(a))
    #endif
    #if !defined(alignof)
     #define alignof(a)     	    //__alignof(a)
    #endif
    #if !defined(thread_local)
     #define thread_local   	    //__declspec(thread)
    #endif
    #if !defined(__has_include)
     #define __has_include(x)	    0
    #endif
 #endif
#endif


#endif	// IMPLICIT_INCLUDE_HEADER_H
