/*
Copyright (C) 2006 StrmnNrmn

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


#ifndef MATH_MATHUTIL_H_
#define MATH_MATHUTIL_H_

#include "Math/Math.h"
#include <algorithm>


template< typename T > constexpr inline T Saturate( s32 x );

template<> constexpr inline s16 Saturate( s32 x )
{
	return static_cast<s16>( std::clamp< s32 >( x, -32768, 32767 ) );
}

template<> constexpr inline s8 Saturate( s32 x )
{
	return static_cast<s8>( std::clamp< s32 >( x, -128, 127 ) );
}


template< typename T >
inline const T * RoundPointerDown( const T * p, uintptr_t r )
{
	const uintptr_t mask = r-1;
	return reinterpret_cast< const T * >( reinterpret_cast< uintptr_t >( p ) & ~mask );
}

template< typename T >
inline const T * RoundPointerUp( const T * p, uintptr_t r )
{
	const uintptr_t mask = r-1;
	return reinterpret_cast< const T * >( ( reinterpret_cast< uintptr_t >( p ) + mask ) & ~mask );
}

inline bool IsPointerAligned( const void * p, uintptr_t r )
{
	const uintptr_t mask = r-1;
	 return (reinterpret_cast< uintptr_t >( p ) & mask) == 0;
}

inline u32 AlignPow2( u32 x, u32 r )
{
	return (x + (r-1)) & ~(r-1);
}

inline bool IsAligned( u32 x, u32 r )
{
	 return (x & (r-1)) == 0;
}

inline u32 GetNextPowerOf2( u32 x )
{
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;


	return x;
}

#endif // MATH_MATHUTIL_H_
