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


#include "Base/Types.h"
#include "System/Timing.h"


#include <chrono>

namespace NTiming {
using u64 = uint64_t;

// Get the frequency in microseconds per second (which is 1,000,000)
bool GetPreciseFrequency(u64* p_freq)
{
    *p_freq = std::chrono::microseconds::period::den;  // 1,000,000 microseconds in a second
    return true;
}

// Get the precise current time in microseconds since epoch
bool GetPreciseTime(u64* p_time)
{
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
    *p_time = duration.count();
    return true;
}

// Convert ticks (microseconds) to milliseconds
u64 ToMilliseconds(u64 ticks)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::microseconds(ticks)).count();
}

} // namespace NTiming
