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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef PLATFORM_H_
#define PLATFORM_H_

//
//	Make sure this platform is defined correctly
//
#ifndef DAEDALUS_W32
#define DAEDALUS_W32
#endif

#define _CRT_SECURE_NO_DEPRECATE
#define _DO_NOT_DECLARE_INTERLOCKED_INTRINSICS_IN_MEMORY


#define DAEDALUS_ENABLE_DYNAREC
#define DAEDALUS_TRAP_PLUGIN_EXCEPTIONS
#undef DAEDALUS_BREAKPOINTS_ENABLED
#define DAEDALUS_ENABLE_OS_HOOKS
#define DAEDALUS_SIMULATE_DOUBLE
#define DAEDALUS_COMPRESSED_ROM_SUPPORT

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

#endif // PLATFORM_H_
