/*
Copyright (C) 2001-2007 StrmnNrmn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef PRECOMPILED_H_
#define PRECOMPILED_H_

//*****************************************************************************
//	These are our various compilation options
//*****************************************************************************
#include "BuildOptions.h"

#include "Endian.h"

//*****************************************************************************
//
//*****************************************************************************
#ifndef _MSC_VER
#define __forceinline	inline __attribute__((always_inline))
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DAEDALUS_USE(...)	do { (void)sizeof(__VA_ARGS__, 0); } while(0)

// Platform specifc #includes, externs, #defines etc
#ifdef DAEDALUS_W32
#include "DaedalusW32.h"
#endif

#include "Debug/DaedalusAssert.h"
#include "Utility/DaedalusTypes.h"
#include "Utility/String.h"

//*****************************************************************************
//
//*****************************************************************************
#ifdef DAEDALUS_ENABLE_ASSERTS

#define NODEFAULT		DAEDALUS_ERROR( "No default - we shouldn't be here" )

#else

#ifdef _MSC_VER
#define NODEFAULT		__assume( 0 )
#else
#define NODEFAULT		//DAEDALUS_EXPECT_LIKELY(1)?
#endif

#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(arr)   (sizeof(arr) / sizeof(arr[0]))
#endif

#endif // PRECOMPILED_H_
