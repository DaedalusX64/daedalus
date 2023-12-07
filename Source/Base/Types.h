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

#include <cstdint>

using u8 =  std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

using f32 = float;
using f64 = double;

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

};

// DAEDALUS_STATIC_ASSERT( sizeof( REG64 ) == sizeof( u64 ) );

union REG32
{
	f32		_f32;
	s32		_s32;
	u32		_u32;
};

// DAEDALUS_STATIC_ASSERT( sizeof( REG32 ) == sizeof( u32 ) );


// Alignment Size Specifiers
#define DATA_ALIGN	16
#define CACHE_ALIGN	64

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

#endif // UTILITY_DAEDALUSTYPES_H_
