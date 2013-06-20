#include "stdafx.h"
#include "Utility/Cond.h"
#include "Utility/Mutex.h"

const double kTimeoutInfinity = 0.f;

// Cond wrapper derived from GLFW 2.7, see http://www.glfw.org/.

enum 
{
    COND_SIGNAL     = 0,
    COND_BROADCAST  = 1
};

struct _cond
{
    HANDLE events[ 2 ];
    u32 waiters_count;
    CRITICAL_SECTION waiters_count_lock;
};

Cond * CondCreate()
{
   _cond   *cond;
    cond = (_cond *) malloc( sizeof(_cond) );
    if( !cond )
    {
        return NULL;
    }

    cond->waiters_count = 0;
    cond->events[ COND_SIGNAL ]    = CreateEvent( NULL, false, false, NULL );
    cond->events[ COND_BROADCAST ] = CreateEvent( NULL, true, false, NULL );
    InitializeCriticalSection( &cond->waiters_count_lock );

    return (Cond*)cond;
}

void CondDestroy(Cond * cond)
{
    CloseHandle( ((_cond *)cond)->events[ COND_SIGNAL ] );
    CloseHandle( ((_cond *)cond)->events[ COND_BROADCAST ] );

    DeleteCriticalSection( &((_cond*)cond)->waiters_count_lock );
    free( (void *)cond );
}

void CondWait(Cond * cond, Mutex * mutex, double timeout)
{
	_cond *cv = (_cond *)cond;
	s32 result, last_waiter;
	u32 ms;

    EnterCriticalSection( &cv->waiters_count_lock );
    cv->waiters_count ++;
    LeaveCriticalSection( &cv->waiters_count_lock );
    LeaveCriticalSection( &mutex->cs );

	if (timeout <= 0)
	{
		ms = INFINITE;
	}
	else
	{
		ms = (u32) (1000.0 * timeout + 0.5);
		ms = ( ms <= 0 ) ? 1 : ms;
	}

    result = WaitForMultipleObjects( 2, cv->events, false, ms );
    EnterCriticalSection( &cv->waiters_count_lock );
    cv->waiters_count--;
    last_waiter = (result == WAIT_OBJECT_0 + COND_BROADCAST) &&(cv->waiters_count == 0);
    LeaveCriticalSection( &cv->waiters_count_lock );

    if( last_waiter )
    {
        ResetEvent( cv->events[ COND_BROADCAST ] );
    }

    EnterCriticalSection( &mutex->cs );
}

void CondSignal(Cond * cond)
{
    _cond *cv = (_cond*)cond;
    s32 have_waiters;

    // Avoid race conditions
    EnterCriticalSection( &cv->waiters_count_lock );
    have_waiters = cv->waiters_count > 0;
    LeaveCriticalSection( &cv->waiters_count_lock );

    if( have_waiters )
    {
        SetEvent( cv->events[ COND_SIGNAL ] );
    }
}
