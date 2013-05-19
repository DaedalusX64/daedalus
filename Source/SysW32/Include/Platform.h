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

#ifndef SYSW32_INCLUDE_PLATFORM_H_
#define SYSW32_INCLUDE_PLATFORM_H_

//
//	Make sure this platform is defined correctly
//
#ifndef DAEDALUS_W32
#define DAEDALUS_W32
#endif

#define __PRETTY_FUNCTION__ __FUNCTION__
#define _CRT_SECURE_NO_DEPRECATE
#define _DO_NOT_DECLARE_INTERLOCKED_INTRINSICS_IN_MEMORY
#define __has_feature(x) 0


#define DAEDALUS_ENABLE_DYNAREC
#undef DAEDALUS_BREAKPOINTS_ENABLED
#define DAEDALUS_ENABLE_OS_HOOKS
#define DAEDALUS_COMPRESSED_ROM_SUPPORT
#define DAEDALUS_GL
#define DAEDALUS_ACCURATE_TMEM

#define DAEDALUS_ENDIAN_MODE DAEDALUS_ENDIAN_LITTLE


// Calling convention for the R4300 instruction handlers.
// These are called from dynarec so we need to ensure they're __fastcall,
// even if the project is not compiled with that option in the project settings.
#define R4300_CALL_TYPE						__fastcall

// Thread functions need to be __stdcall to work with the W32 api
#define DAEDALUS_THREAD_CALL_TYPE			__stdcall

// Vararg functions need to be __cdecl
#define DAEDALUS_VARARG_CALL_TYPE			__cdecl

// Zlib is compiled as __cdecl
#define	DAEDALUS_ZLIB_CALL_TYPE				__cdecl

// Breakpoint
#define DAEDALUS_HALT					__asm { int 3 }


#endif // SYSW32_INCLUDE_PLATFORM_H_
