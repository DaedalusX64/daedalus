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

#include "stdafx.h"
#include "FramerateLimiter.h"

#include "Utility/Timing.h"
#include "Utility/Thread.h"

#include "Core/Memory.h"
#include "Core/ROM.h"

#include "Debug/DBGConsole.h"

#include "OSHLE/ultra_os.h"		// System type

//*****************************************************************************
//
//*****************************************************************************
namespace
{
u64				gTicksBetweenVbls( 0 );				// How many ticks we want to delay between vertical blanks
u64				gTicksPerSecond( 0 );				// How many ticks there are per second

u64				gLastVITime( 0 );					// The time of the last vertical blank
u32				gLastOrigin( 0 );					// The origin that we saw on the last vertical blank
u32				gVblsSinceFlip( 0 );				// The number of vertical blanks that have occurred since the last n64 flip

const u32		NUM_SYNC_SAMPLES( 8 );				// These are all for keeping track of the current sync rate
u64				gRecentTicksPerVbl[ NUM_SYNC_SAMPLES ];
u32				gRecentTicksPerVblIdx( 0 );
u64				gCurrentAverageTicksPerVbl( 0 );

const u32		gTvFrequencies[] = 
{
	50,		//OS_TV_PAL,
	60,		//OS_TV_NTSC,
	50		// OS_TV_MPAL
};

}

//*****************************************************************************
//
//*****************************************************************************
void FramerateLimiter_Reset()
{
	u64 frequency;

	gLastVITime = 0;
	gLastOrigin = 0;
	gVblsSinceFlip = 0;

	if(NTiming::GetPreciseFrequency(&frequency))
	{
		DAEDALUS_ASSERT(g_ROM.TvType <= sizeof(gTvFrequencies) / sizeof(u32), "Unknow TV type: %d", g_ROM.TvType);

		gTicksBetweenVbls = frequency / (u64)gTvFrequencies[ g_ROM.TvType ];
		gTicksPerSecond = frequency;
	}
	else
	{
		gTicksBetweenVbls = 0;
		gTicksPerSecond = 0;
	}


	// Initialise the array - assume perfect timekeeping
	for (u32 i = 0; i < NUM_SYNC_SAMPLES; i++)
	{
		gRecentTicksPerVbl[i] = gTicksBetweenVbls;
	}
	gRecentTicksPerVblIdx = 0;
	gCurrentAverageTicksPerVbl = gTicksBetweenVbls;

}

//*****************************************************************************
//
//*****************************************************************************
template< typename T > T Average( const T * arr, const u32 count )
{
	T sum = 0;
	for( u32 i = 0; i < count; ++i )
	{
		sum += arr[ i ];
	}
	return sum / T( count );
}

u64 FramerateLimiter_UpdateAverageTicksPerVbl( u64 elapsed_ticks )
{
	gRecentTicksPerVbl[gRecentTicksPerVblIdx] = elapsed_ticks;
	gRecentTicksPerVblIdx = (gRecentTicksPerVblIdx + 1) % NUM_SYNC_SAMPLES;

	return Average( gRecentTicksPerVbl, NUM_SYNC_SAMPLES );
}

//*****************************************************************************
//
//*****************************************************************************
void FramerateLimiter_Limit()
{
	gVblsSinceFlip++;

	// Only do framerate limiting on frames that correspond to a flip
	u32 current_origin = Memory_VI_GetRegister(VI_ORIGIN_REG);
	if( current_origin == gLastOrigin )
	{
		return;
	}
	gLastOrigin = current_origin;

	u64		now;

	NTiming::GetPreciseTime(&now);
	if( gLastVITime != 0 )
	{
		u64 elapsed_ticks = now - gLastVITime;

		gCurrentAverageTicksPerVbl = FramerateLimiter_UpdateAverageTicksPerVbl( elapsed_ticks / gVblsSinceFlip );

		if( gSpeedSyncEnabled )
		{
			//
			// This is a bit of a hack - busy wait until we're exactly on schedule
			//	Ideally we should call Sleep() or similar here to avoid this.
			//
			u64		required_ticks( gTicksBetweenVbls * gVblsSinceFlip );
			while( elapsed_ticks < required_ticks )
			{
				u64		delay_ticks( required_ticks - elapsed_ticks );
				if( gTicksPerSecond != 0 )
				{
					u64		delay_ms( 1000 * delay_ticks / gTicksPerSecond );
					u32		sleep_ms = u32( delay_ms );			// Need to deal with potential overflow?

					if( sleep_ms > 1 )
					{
						//printf( "Delay ticks: %d, ms: %d, sleep: %d\n", u32( delay_ticks ), u32( delay_ms ), sleep_ms );
						ThreadSleepMs( sleep_ms );
						return;	//Return early //Corn
					}
				}

				NTiming::GetPreciseTime(&now);
				elapsed_ticks = now - gLastVITime;
			}
		}
	}

	gLastVITime = now;
	gVblsSinceFlip = 0;
}

//*****************************************************************************
//	Get the current sync
//*****************************************************************************
f32	FramerateLimiter_GetSync()
{
	if( gCurrentAverageTicksPerVbl == 0 )
	{
		return 0.0f;
	}
	return f32( gTicksBetweenVbls ) / f32( gCurrentAverageTicksPerVbl );
}

//*****************************************************************************
//	Get the current sync rate to match audio sample rate //Corn 
//*****************************************************************************
u32	FramerateLimiter_GetSyncI()
{
	static f32 sum=0.0f;

	if( gTicksBetweenVbls == 0 )
	{
		return 44100;
	}

	//Filter variations a bit
	sum = sum * 0.90f + 0.095f * 44100.0f * (f32)gCurrentAverageTicksPerVbl  / (f32)gTicksBetweenVbls;
	//printf("%d\n",u32(sum));
	return u32(sum);
}

//*****************************************************************************
//
//*****************************************************************************
void	FramerateLimiter_SetLimit( bool limit )
{
	gSpeedSyncEnabled = limit;
	FramerateLimiter_Reset();
}

//*****************************************************************************
//
//*****************************************************************************
bool	FramerateLimiter_GetLimit()
{
	return gSpeedSyncEnabled;
}

//*****************************************************************************
//
//*****************************************************************************
u32		FramerateLimiter_GetTvFrequencyHz()
{
	return gTvFrequencies[ g_ROM.TvType ];
}
