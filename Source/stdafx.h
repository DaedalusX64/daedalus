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

#pragma once

#ifndef STDAFX_H_
#define STDAFX_H_

//*****************************************************************************
//	These are our various compilation options
//*****************************************************************************
#include "BuildOptions.h"

// Platform specifc #includes, externs, #defines etc
#ifdef DAEDALUS_W32
#include "DaedalusW32.h"
#endif

#include "Base/Types.h"

#endif // STDAFX_H_
