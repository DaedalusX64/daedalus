/*
Copyright (C) 2005 StrmnNrmn

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

#include "stdafx.h"
#include "System/Timing.h"

namespace NTiming {

bool GetPreciseFrequency( u64 * p_freq )
{
	LARGE_INTEGER	freq;
	if(::QueryPerformanceFrequency( &freq ))
	{
		*p_freq = freq.QuadPart;
		return true;
	}

	*p_freq = 1;
	return false;
}

bool GetPreciseTime( u64 * p_time )
{
	LARGE_INTEGER	time;
	if(::QueryPerformanceCounter( &time ))
	{
		*p_time = time.QuadPart;
		return true;
	}

	*p_time = 0;
	return false;
}

u64 ToMilliseconds( u64 ticks )
{
	static u64 tick_resolution = 0;
	if (tick_resolution == 0)
		GetPreciseFrequency(&tick_resolution);
	return (ticks*1000LL) / tick_resolution;
}

} // NTiming
