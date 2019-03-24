/*
Copyright (C) 2007 StrmnNrmn
Copyright (C) 2007 crazyc (?)

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

#ifndef SYSPSP_UTILITY_CACHEUTIL_H_
#define SYSPSP_UTILITY_CACHEUTIL_H_

#include "Math/MathUtil.h"

// Taken from MediaEnginePRX, assume they're orignally from
// http://forums.ps2dev.org/viewtopic.php?p=58333#58333
// Primarily for use in code which will be run on the ME, were we can't access kernel functions

#include <malloc.h>

#ifdef DAEDALUS_PSP
inline void *malloc_64(int size)
{
	int mod_64 {size & 0x3f};
	if (mod_64 != 0) size += 64 - mod_64;
	return((void *)memalign(64, size));
}


inline void dcache_wbinv_all()
{
   for(int i {0}; i < 8192; i += 64)
__builtin_allegrex_cache(0x14, i);
}

inline void dcache_wbinv_range(const void *addr, int size)
{
   int i {0}, j = (int)addr;
   for(i = j; i < size+j; i += 64)
      __builtin_allegrex_cache(0x1b, i);
}

inline void dcache_wbinv_range_unaligned(const void *lower, const void *upper)
{
	lower = RoundPointerDown( lower, 64 );
	upper = RoundPointerUp( upper, 64 );

	dcache_wbinv_range( lower, (int)upper - (int)lower );
}

inline void dcache_inv_range(void *addr, int size)
{
   int i {0}, j = (int)addr;
   for(i = j; i < size+j; i += 64)
      __builtin_allegrex_cache(0x1b, i);
}
#endif

#endif // SYSPSP_UTILITY_CACHEUTIL_H_
