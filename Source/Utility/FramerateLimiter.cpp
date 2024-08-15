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


#include "Base/Types.h"
#include "FramerateLimiter.h"

#include "System/Timing.h"
#include "System/Thread.h"

#include "Core/Memory.h"
#include "Core/ROM.h"

static u32				gTicksBetweenVbls = 0;			// How many ticks we want to delay between vertical blanks
static u32				gTicksPerSecond = 0;			// How many ticks there are per second
static u64				gLastVITime = 0;				// The time of the last vertical blank
static u32				gLastOrigin = 0;				// The origin that we saw on the last vertical blank
static u32				gVblsSinceFlip = 0;				// The number of vertical blanks that have occurred since the last n64 flip
static u32				gCurrentAverageTicksPerVbl = 0;
static FramerateSyncFn 	gAuxSyncFn = NULL;
static void *			gAuxSyncArg = NULL;

static const u32		gTvFrequencies[] =
{
	50,		// OS_TV_PAL,
	60,		// OS_TV_NTSC,
	50		// OS_TV_MPAL
};

void FramerateLimiter_SetAuxillarySyncFunction(FramerateSyncFn fn, void * arg)
{
	gAuxSyncFn  = fn;
	gAuxSyncArg = arg;
}

bool FramerateLimiter_Reset()
{
	u64 frequency;

	gLastVITime = 0;
	gLastOrigin = 0;
	gVblsSinceFlip = 0;

	//gAuxSyncFn  = NULL;	// Should we reset this? Will audio re-init?
	//gAuxSyncArg = NULL;

	if(NTiming::GetPreciseFrequency(&frequency))
	{
		#ifdef DAEDALUS_ENABLE_ASSERTS
		DAEDALUS_ASSERT(g_ROM.TvType <= sizeof(gTvFrequencies) / sizeof(u32), "Unknown TV type: %d", g_ROM.TvType);
		#endif

		gTicksBetweenVbls = (u32)(frequency / (u64)gTvFrequencies[ g_ROM.TvType ]);
		gTicksPerSecond = (u32)frequency;
	}
	else
	{
		gTicksBetweenVbls = 0;
		gTicksPerSecond = 0;
	}
	return true;
}

static u32 FramerateLimiter_UpdateAverageTicksPerVbl( u32 elapsed_ticks )
{
	static u32 s[4];
	static u32 ptr = 0;

	s[ptr++] = elapsed_ticks;
	ptr &= 0x3;

	//Average 4 frames
	return (s[0] + s[1] + s[2] + s[3] + 2) >> 2;
}

void FramerateLimiter_Limit()
{
	gVblsSinceFlip++;

	// Only do framerate limiting on frames that correspond to a flip
	u32 current_origin = Memory_VI_GetRegister(VI_ORIGIN_REG);

	if (gAuxSyncFn)
	{
		gAuxSyncFn(gAuxSyncArg);
	}

	if( current_origin == gLastOrigin )
		return;

	u64	now;
	NTiming::GetPreciseTime(&now);

	u32 elapsed_ticks = (u32)(now - gLastVITime);

	gCurrentAverageTicksPerVbl = FramerateLimiter_UpdateAverageTicksPerVbl( elapsed_ticks / gVblsSinceFlip );

	if( gSpeedSyncEnabled && !gAuxSyncFn )
	{
		u32 required_ticks = gTicksBetweenVbls * gVblsSinceFlip;

		if( gSpeedSyncEnabled == 2 ) required_ticks = required_ticks << 1;	// Slow down to 1/2 speed //Corn

		// FIXME the constant here will need to be adjusted for different platforms.
		s32	delay_ticks = required_ticks - elapsed_ticks - 50;	//Remove ~50 ticks for additional processing

		if( delay_ticks > 0 )
		{
			//printf( "Delay ticks: %d\n", delay_ticks );
			ThreadSleepTicks( delay_ticks & 0xFFFF );
			NTiming::GetPreciseTime(&now);
		}
	}

	gLastOrigin = current_origin;
	gLastVITime = now;
	gVblsSinceFlip = 0;
}

f32	FramerateLimiter_GetSync()
{
	if( gCurrentAverageTicksPerVbl == 0 )
	{
		return 0.0f;
	}
	return f32( gTicksBetweenVbls ) / f32( gCurrentAverageTicksPerVbl );
}

u32 FramerateLimiter_GetTvFrequencyHz()
{
	return gTvFrequencies[ g_ROM.TvType ];
}
