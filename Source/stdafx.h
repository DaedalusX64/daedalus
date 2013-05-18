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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(DAEDALUS_PSP)
#define _strcmpi stricmp
#elif defined(DAEDALUS_PS3) || defined(DAEDALUS_OSX) || defined(DAEDALUS_LINUX)
#define _strcmpi strcasecmp
#endif

#define DAEDALUS_USE(...)	do { (void)sizeof(__VA_ARGS__, 0); } while(0)

// Platform specifc #includes, externs, #defines etc
#ifdef DAEDALUS_W32
#include "DaedalusW32.h"
#endif

#include "Debug/DaedalusAssert.h"
#include "Utility/DaedalusTypes.h"

#endif // PRECOMPILED_H_
