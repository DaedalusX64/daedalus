/*
Copyright (C) 2008 StrmnNrmn

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

#ifndef SYSW32_INCLUDE_DAEDALUSW32_H_
#define SYSW32_INCLUDE_DAEDALUSW32_H_

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <intrin.h>

#include <algorithm>
#include <vector>
#include <map>
#include <string>

// Pull this in after all stl headers
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif
#include <crtdbg.h>

#ifdef _DEBUG
#ifndef NEW_INLINE_WORKAROUND
#define NEW_INLINE_WORKAROUND new ( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new NEW_INLINE_WORKAROUND
#endif // NEW_INLINE_WORKAROUND
#endif // _DEBUG

//We link glew statically, so define this
#define GLEW_STATIC

#endif // SYSW32_INCLUDE_DAEDALUSW32_H_
