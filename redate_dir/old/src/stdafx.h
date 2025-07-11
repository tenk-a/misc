/**
 *  @file   stdafx.h
 *  @brief  全体で使われる、プリコンパイルされるヘッダ
 */
#ifndef STDAFX_H
#define STDAFX_H

#pragma once


//#define TARGET    0


#if defined _MSC_VER && _MSC_VER < 1600
#include "misc/less_than_vc10/stdint.h"
#else
#include <stdint.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>
#include <functional>

#undef max
#undef min
using	    std::vector;
//#define   vector  	km_vector

//#include "misc/noncopyable.h"
#include "misc/CString.h"
#include "misc/CString_Misc.h"
#include "misc/misc_cstr.h"
//#include "misc/intrusive_ptr.h"
//#include "misc/RefCountForIntrusivePtr.h"
#include "misc/disk.h"
//#include "misc/ZeroFillNew.h"
//#include "misc/misc.h"

#undef max
#undef min

#endif
