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

#ifndef UTILITY_ATOMICPRIMITIVES_H_
#define UTILITY_ATOMICPRIMITIVES_H_

#include <stdlib.h>
#include "Base/Types.h"

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
	return _InterlockedIncrement( reinterpret_cast< volatile long * >( ptr ) );
}

inline u32 AtomicDecrement( volatile u32 * ptr )
{
	return _InterlockedDecrement( reinterpret_cast< volatile long * >( ptr ) );
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
	while ( _InterlockedCompareExchange( reinterpret_cast< volatile long * >( ptr ), new_value, orig_value ) != orig_value );

	return new_value;
}

// POSIX Atomics, Probably can just set this as default for platforms that don't need custom atomics for now. 
// This eventually will be replaced with a more modern set.
#else

// inline u32 AtomicIncrement( volatile u32 * ptr )
// {
// 	DAEDALUS_ASSERT(false, "FIXME");
// 	return *ptr++;
// }

// inline u32 AtomicDecrement( volatile u32 * ptr )
// {
// 	DAEDALUS_ASSERT(false, "FIXME");
// 	return *ptr--;
// }

inline u32 AtomicBitSet( volatile u32 * ptr, u32 and_bits, u32 or_bits )
{
    u32 new_value;
    u32 orig_value;

    do {
        orig_value = __sync_fetch_and_or(ptr, or_bits);  // Atomically OR or_bits into *ptr
        new_value = (orig_value & and_bits) | or_bits;   // Calculate the new value

    } while (__sync_val_compare_and_swap(ptr, orig_value, new_value) != orig_value);

    return new_value;
}

#endif

#endif // UTILITY_ATOMICPRIMITIVES_H_
