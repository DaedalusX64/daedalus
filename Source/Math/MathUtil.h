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


#ifndef MATHUTIL_H_
#define MATHUTIL_H_

#include "Math/Math.h"

template< typename T > const T & Max( const T & a, const T & b )
{
	if(a > b)
		return a;
	return b;
}

template< typename T > const T & Min( const T & a, const T & b )
{
	if(a < b)
		return a;
	return b;
}

template< typename T > void Swap( T & a, T & b )
{
	T t = a;
	a = b;
	b = t;
}

inline float Interpolate( float a, float b, float r )
{
	return a + r * (b - a);
}

template< typename T > T Square( T x )
{
	return x * x;
}

template< typename T > T Clamp( T x, T lo, T hi )
{
	if(x < lo)
		return lo;
	else if(x > hi)
		return hi;
	else
		return x;
}

template< typename T > T Saturate( s32 x );

template<> inline s16 Saturate( s32 x )
{
	return s16( Clamp< s32 >( x, -32768, 32767 ) );
}

template<> inline s8 Saturate( s32 x )
{
	return s8( Clamp< s32 >( x, -128, 127 ) );
}


template< typename T > const T * RoundPointerDown( const T * p, u32 r )
{
	const u32 mask( r-1 );
	return reinterpret_cast< const T * >( reinterpret_cast< u32 >( p ) & ~mask );
}

template< typename T > const T * RoundPointerUp( const T * p, u32 r )
{
	const u32 mask( r-1 );
	return reinterpret_cast< const T * >( ( reinterpret_cast< u32 >( p ) + mask ) & ~mask );
}

inline bool IsPointerAligned( const void * p, u32 r )
{
	 return (reinterpret_cast< u32 >( p ) & (r-1)) == 0;
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
	u32 n = 1;
	while ( n < x )
	{
		n = n<<1;
	}

/*
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
*/

	return n;
}

#endif	// MATHUTIL_H_
