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

#endif // ATOMICPRIMITIVES_H_
