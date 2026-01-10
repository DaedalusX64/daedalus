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
#include <kos.h>
#include "Precompiled.h"
#include "DaedThread.h"

#define MAX_NUM_THREADS 4
namespace daedalus
{
const s32	INVALID_THREAD_HANDLE( -1 );
static kthread_t *g_pThreadHandle[MAX_NUM_THREADS] = {NULL};

//*****************************************************************************
//
//*****************************************************************************
// XXXX p_function might go out of scope, so I'm doing this. There must be a nicer way. It's not thread safe at the moment
static DaedThread	PFunction;

//*****************************************************************************
//	The real thread is passed in as an argument. We call it and return the result
//*****************************************************************************
static void StartThreadFunc(void *argp)
{
    ((DaedThread)argp)();
}

s32		CreateThread( DaedThread p_function, bool start_on_creation )
{
    // Nächsten freien ThreadHandle suchen
    int i = 0;
    for(i = 0; i < MAX_NUM_THREADS; i++)
        if(g_pThreadHandle == NULL)
            break;

    // Überprüfen, ob ein freier ThreadHandle gefunden wurde
    if(!(i < MAX_NUM_THREADS))
        return -1;
        
    // ThreadHandle reservieren und Thread erstellen
    g_pThreadHandle[i] = thd_create(StartThreadFunc, (void *)p_function);
    return i;
}

//*****************************************************************************
//
//*****************************************************************************
void	SetThreadPriority( s32 handle, EThreadPriority pri )
{
	// Nothing to do?
}

//*****************************************************************************
//
//*****************************************************************************
void	ReleaseThreadHandle( s32 handle )
{
	// Nothing to do?
}

//*****************************************************************************
//	Wait the specified time for the thread to finish.
//	Returns false if the thread didn't terminate
//*****************************************************************************
bool	WaitForThreadTermination( s32 handle, s32 timeout )
{
    // Timeouts not supported - simple wait
    thd_wait(g_pThreadHandle[handle]);
    g_pThreadHandle[handle] = NULL;

	// XXXX
	return true;
}


}
