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

#include <algorithm>


template< typename T > constexpr inline T Saturate( s32 x );

template<> constexpr inline s16 Saturate<s16>(s32 x)
{
	return static_cast<s16>(x > 32767 ? 32767 : (x < -32768 ? -32768 : x));
}

template<> constexpr inline s8 Saturate<s8>(s32 x)
{
	return static_cast<s8>(x > 127 ? 127 : (x < -128 ? -128 : x));
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

inline u8 clamp_u8(s16 x)
{
    return (x & (0xff00)) ? ((-x) >> 15) & 0xff : x;
}

inline s16 clamp_s12(s16 x)
{
    if (x < -0x800) { x = -0x800; } else if (x > 0x7f0) { x = 0x7f0; }
    return x;
}

inline s16 clamp_s16(s32 x)
{
    if (x > 32767) { x = 32767; } else if (x < -32768) { x = -32768; }
    return x;
}

inline u16 clamp_RGBA_component(s16 x)
{
    if (x > 0xff0) { x = 0xff0; } else if (x < 0) { x = 0; }
    return (x & 0xf80);
}
inline u8 clamp_f32_to_u8(float x)
{
    s32 val = static_cast<s32>(x);
    if (val < 0) return 0;
    if (val > 255) return 255;
    return static_cast<u8>(val);
}
#endif // MATH_MATHUTIL_H_
