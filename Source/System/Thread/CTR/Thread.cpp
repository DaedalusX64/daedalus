
#include "System/Thread/Thread.h"
#include "System/Timing/Timing.h"

#include <3ds.h>

static const int	gThreadPriorities[ TP_NUM_PRIORITIES ] =
{
	0x19,		// TP_LOW
	0x18,		// TP_NORMAL
	0x17,		// TP_HIGH
	0x16,		// TP_TIME_CRITICAL
};


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
