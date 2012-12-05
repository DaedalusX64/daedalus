/*
Copyright (C) 2007 StrmnNrmn

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

#ifndef ATOMICPRIMITIVES_H_
#define ATOMICPRIMITIVES_H_

#if defined( DAEDALUS_PSP )

extern "C"
{
	u32 _AtomicIncrement( volatile u32 * ptr );
	u32 _AtomicDecrement( volatile u32 * ptr );
	u32 _AtomicBitSet( volatile u32 * ptr, u32 and_bits, u32 or_bits );
}

inline u32 AtomicIncrement( volatile u32 * ptr )
{
	return _AtomicIncrement( ptr );
}

inline u32 AtomicDecrement( volatile u32 * ptr )
{
	return _AtomicDecrement( ptr );
}

inline u32 AtomicBitSet( volatile u32 * ptr, u32 and_bits, u32 or_bits )
{
	return _AtomicBitSet( ptr, and_bits, or_bits );
}

#elif defined( DAEDALUS_W32 )

#include <intrin.h>

#pragma intrinsic (_InterlockedIncrement)
#pragma intrinsic (_InterlockedDecrement)
#pragma intrinsic (_InterlockedIncrement)


inline u32 AtomicIncrement( volatile u32 * ptr )
{
	return _InterlockedIncrement( reinterpret_cast< volatile LONG * >( ptr ) );
}

inline u32 AtomicDecrement( volatile u32 * ptr )
{
	return _InterlockedDecrement( reinterpret_cast< volatile LONG * >( ptr ) );
}

inline u32 AtomicBitSet( volatile u32 * ptr, u32 and_bits, u32 or_bits )
{
	u32 new_value;
	u32 orig_value;
	do
	{
		orig_value = *ptr;
		new_value = (orig_value & and_bits) | or_bits;
	}
	while ( _InterlockedCompareExchange( reinterpret_cast< volatile LONG * >( ptr ), new_value, orig_value ) != orig_value );

	return new_value;
}

#elif defined( DAEDALUS_OSX )

inline u32 AtomicIncrement( volatile u32 * ptr )
{
	DAEDALUS_ASSERT(false, "FIXME");
	return *ptr++;
}

inline u32 AtomicDecrement( volatile u32 * ptr )
{
	DAEDALUS_ASSERT(false, "FIXME");
	return *ptr--;
}

inline u32 AtomicBitSet( volatile u32 * ptr, u32 and_bits, u32 or_bits )
{
	u32 r = *ptr;
	r &= and_bits;
	r |= or_bits;
	*ptr = r;
	return r;
}


#else

#error Unhandled platform

#endif

#endif // ATOMICPRIMITIVES_H_

