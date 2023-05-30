/*
Copyright (C) 2001 StrmnNrmn

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

#ifndef UTILITY_DAEDALUSTYPES_H_
#define UTILITY_DAEDALUSTYPES_H_

#include "Base/Assert.h"

#if !defined(DAEDALUS_W32) || _MSC_VER >= 1600
#include <stdint.h>
using u8 =  uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using f32 = float;
using f64 = double;

#else

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned long;
using u64 = unsigned long long;

using s8  = signed char;
using s16 = short;
using s32 = long;
using s64 = long long;

using f32 = float;
using f64 = double;

#endif

// DAEDALUS_STATIC_ASSERT( sizeof( u8 ) == 1 );
// DAEDALUS_STATIC_ASSERT( sizeof( s8 ) == 1 );
// DAEDALUS_STATIC_ASSERT( sizeof( u16 ) == 2 );
// DAEDALUS_STATIC_ASSERT( sizeof( s16 ) == 2 );
// DAEDALUS_STATIC_ASSERT( sizeof( u32 ) == 4 );
// DAEDALUS_STATIC_ASSERT( sizeof( s32 ) == 4 );
// DAEDALUS_STATIC_ASSERT( sizeof( u64 ) == 8 );
// DAEDALUS_STATIC_ASSERT( sizeof( s64 ) == 8 );
// DAEDALUS_STATIC_ASSERT( sizeof( f32 ) == 4 );
// DAEDALUS_STATIC_ASSERT( sizeof( f64 ) == 8 );


union REG64
{
	f64		_f64;
	s64		_s64;
	u64		_u64;

#ifdef DAEDALUS_ENDIAN_BIG
	struct { f32 _f32_1, _f32_0; };
	struct { s32 _s32_1, _s32_0; };
	struct { u32 _u32_1, _u32_0; };
#elif DAEDALUS_ENDIAN_LITTLE
	struct { f32 _f32_0, _f32_1; };
	struct { s32 _s32_0, _s32_1; };
	struct { u32 _u32_0, _u32_1; };
#else
#error No DAEDALUS_ENDIAN_MODE specified
#endif

/*	struct { u32 _f64_unused; f32 _f64_sim;};
	struct { s16 _s16_3, _s16_2, _s16_1, _s16_0; };
	struct { u16 _u16_3, _u16_2, _u16_1, _u16_0; };

	f32		_f32[2];
	s32		_s32[2];
	u32		_u32[2];
	s16		_s16[4];
	u16		_u16[4];
	s8		_s8[8];
	u8		_u8[8];*/
};

// DAEDALUS_STATIC_ASSERT( sizeof( REG64 ) == sizeof( u64 ) );
union REG32
{
	f32		_f32;
	s32		_s32;
	u32		_u32;

	/*s16		_s16[2];
	u16		_u16[2];
	s8		_s8[4];
	u8		_u8[4];*/

};

// DAEDALUS_STATIC_ASSERT( sizeof( REG32 ) == sizeof( u32 ) );


// Alignment Size Specifiers
#define DATA_ALIGN	16
#define CACHE_ALIGN	64

#endif // UTILITY_DAEDALUSTYPES_H_
