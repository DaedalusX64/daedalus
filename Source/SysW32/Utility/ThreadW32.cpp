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
#include "System/Thread.h"

static const int	gThreadPriorities[ TP_NUM_PRIORITIES ] =
{
	THREAD_PRIORITY_BELOW_NORMAL,		// TP_LOW
	THREAD_PRIORITY_NORMAL,				// TP_NORMAL
	THREAD_PRIORITY_ABOVE_NORMAL,		// TP_HIGH
	THREAD_PRIORITY_TIME_CRITICAL,		// TP_TIME_CRITICAL
};

const ThreadHandle	kInvalidThreadHandle = 0;

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
static DWORD WINAPI StartThreadFunc( LPVOID arg )
{
	SDaedThreadDetails * thread_details( reinterpret_cast< SDaedThreadDetails * >( arg ) );

	DWORD result = thread_details->ThreadFunction( thread_details->Argument );

	delete thread_details;

	return result;
}

ThreadHandle CreateThread( const char * name, DaedThread function, void * argument )
{
	DWORD					id;
	SDaedThreadDetails *	thread_details( new SDaedThreadDetails( function, argument ) );
	HANDLE					h( ::CreateThread( NULL, 0, StartThreadFunc, thread_details, CREATE_SUSPENDED, &id ) );

	if(h != NULL)
	{
		ResumeThread( h );
		return reinterpret_cast< s32 >( h );
	}

	return 0;
}

void SetThreadPriority( ThreadHandle handle, EThreadPriority pri )
{
	HANDLE	h( reinterpret_cast< HANDLE >( handle ) );
	if(h != NULL)
	{
		::SetThreadPriority( h, gThreadPriorities[ pri ] );
	}
}

void ReleaseThreadHandle( ThreadHandle handle )
{
	HANDLE	h( reinterpret_cast< HANDLE >( handle ) );
	if(h != NULL)
	{
		CloseHandle(h);
	}
}

// Wait the specified time for the thread to finish.
// Returns false if the thread didn't terminate
bool JoinThread( ThreadHandle handle, s32 timeout )
{
	u32		delay( timeout > 0 ? timeout : INFINITE );
	bool	signalled( true );

	HANDLE	h( reinterpret_cast< HANDLE >( handle ) );
	if(h != NULL)
	{
		// Wait forever for it to finish.
		switch(WaitForSingleObject(h, delay))
		{
		case WAIT_ABANDONED:
			signalled = true;
			break;
		case WAIT_OBJECT_0:
			signalled = true;
			break;
		case WAIT_TIMEOUT:
			signalled = false;
			break;
		default:
			DAEDALUS_ERROR( "Unhandled result from WaitForSingleObject" );
			break;
		}
	}

	return signalled;
}

void ThreadSleepMs( u32 ms )
{
	::Sleep( ms );
}

void ThreadSleepTicks( u32 ticks )
{
	// FIXME: not sure what units this is in.
	::Sleep( ticks );
}

void ThreadYield()
{
	::Sleep( 0 );
}

void TerminateThread(long handle)
{
	::TerminateThread((HANDLE)handle, 0);
}
