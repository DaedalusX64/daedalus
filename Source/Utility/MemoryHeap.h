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

#pragma once

#ifndef UTILITY_MEMORYHEAP_H_
#define UTILITY_MEMORYHEAP_H_

#include "Base/Types.h"

class CMemoryHeap
{
public:
	static CMemoryHeap * Create( u32 size );						// Allocate and manage a new region of this size
	static CMemoryHeap * Create( void * base_ptr, u32 size );		// Manage this region of pre-allocated memory

	virtual ~CMemoryHeap();

	virtual void *		Alloc( u32 size ) = 0;
	virtual void		Free( void * ptr ) = 0;

	virtual bool		IsFromHeap( void * ptr ) const = 0;			// Does this chunk of memory belong to this heap?
#ifdef DAEDALUS_DEBUG_MEMORY
	//virtual u32		GetAvailableMemory() const = 0;
	virtual void		DisplayDebugInfo() const = 0;
#endif
};

#endif // UTILITY_MEMORYHEAP_H_
