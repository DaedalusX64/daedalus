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
#include "Utility/Thread.h"
#include "Utility/Timing.h"

#include <3ds.h>

static const int	gThreadPriorities[ TP_NUM_PRIORITIES ] =
{
	0x19,		// TP_LOW
	0x18,		// TP_NORMAL
	0x17,		// TP_HIGH
	0x16,		// TP_TIME_CRITICAL
};

const ThreadHandle	kInvalidThreadHandle = -1;

struct SDaedThreadDetails
{
	SDaedThreadDetails( DaedThread function, void * argument )
		:	ThreadFunction( function )
		,	Argument( argument )
	{
	}

	DaedThread		ThreadFunction;
	void *			Argument;
};

// The real thread is passed in as an argument. We call it and return the result
static void StartThreadFunc(  void *argp )
{
	SDaedThreadDetails * thread_details( static_cast< SDaedThreadDetails * >( argp ) );
	thread_details->ThreadFunction( thread_details->Argument );
}

ThreadHandle CreateThread( const char * name, DaedThread function, void * argument )
{
	SDaedThreadDetails thread_details( function, argument );

	Thread thid = threadCreate(StartThreadFunc, &thread_details, 0x10000, gThreadPriorities[TP_NORMAL], -2, false);

	return thid ? (ThreadHandle)thid : kInvalidThreadHandle;
}

void SetThreadPriority( s32 handle, EThreadPriority pri )
{
	// Nothing to do
}

void ReleaseThreadHandle( s32 handle )
{
	threadFree((Thread)handle);
}

// Wait the specified time for the thread to finish.
// Returns false if the thread didn't terminate
bool JoinThread( s32 handle, s32 timeout )
{
	Result ret = threadJoin((Thread)handle, timeout);

	return (ret >= 0);
}

void ThreadSleepMs( u32 ms )
{
	svcSleepThread( ms * 1000 );		// Delay is specified in microseconds
}

void ThreadSleepTicks( u32 ticks )
{
	svcSleepThread( NTiming::ToMilliseconds(ticks) * 1000 );		// Delay is specified in ticks
}

void ThreadYield()
{
	svcSleepThread( 1 );				// Is 0 valid?
}
