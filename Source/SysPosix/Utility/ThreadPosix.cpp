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

#include <pthread.h>
#include <unistd.h>
#include <sched.h>

const ThreadHandle	kInvalidThreadHandle = -1;

namespace
{
DAEDALUS_STATIC_ASSERT( sizeof( pthread_t ) == sizeof( ThreadHandle ) );

const int	gThreadPriorities[ TP_NUM_PRIORITIES ] =
{
	0x19,		// TP_LOW
	0x18,		// TP_NORMAL
	0x17,		// TP_HIGH
	0x16,		// TP_TIME_CRITICAL
};

pthread_t HandleToPthread( ThreadHandle handle )
{
	return handle == kInvalidThreadHandle ? 0 : (pthread_t)handle;
}

ThreadHandle PthreadToHandle( pthread_t thread )
{
	return thread ? (ThreadHandle)thread : kInvalidThreadHandle;
}

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

//	The real thread is passed in as an argument. We call it and return the result
void * StartThreadFunc( void * arg )
{
	SDaedThreadDetails * thread_details( reinterpret_cast< SDaedThreadDetails * >( arg ) );

	int result = thread_details->ThreadFunction( thread_details->Argument );

	delete thread_details;

	pthread_exit( &result );
	return NULL;
}

} // anonymous namespace

ThreadHandle CreateThread( const char * name, DaedThread function, void * argument )
{
	pthread_t		thread;
	pthread_attr_t	thread_attr;

	pthread_attr_init( &thread_attr );

	SDaedThreadDetails *	thread_details = new SDaedThreadDetails( function, argument );

	s32	result = ::pthread_create( &thread, &thread_attr, &StartThreadFunc, thread_details );
	if(result == 0)
	{
		return PthreadToHandle( thread );
	}

	pthread_attr_destroy( &thread_attr );
	return kInvalidThreadHandle;
}

void SetThreadPriority( ThreadHandle handle, EThreadPriority pri )
{
	DAEDALUS_ERROR( "Thread priorities not supported" );
}

void ReleaseThreadHandle( ThreadHandle handle )
{
	// Nothing to do?
}

//	Wait the specified time for the thread to finish.
//	Returns false if the thread didn't terminate
bool JoinThread( ThreadHandle handle, s32 timeout )
{
	//u32		delay( timeout > 0 ? timeout : INFINITE );
	bool	signalled( true );

	pthread_join( HandleToPthread( handle ), NULL );

	return signalled;
}

void ThreadSleepMs( u32 ms )
{
	usleep( ms * 1000 );
}

void ThreadSleepTicks( u32 ticks )
{
	// FIXME: not sure what units this is in.
	usleep( ticks );
}

void ThreadYield()
{
	sched_yield();
}
