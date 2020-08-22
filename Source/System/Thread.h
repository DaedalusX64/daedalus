/*
Copyright (C) 2001 StrmnNrmn

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

#pragma once

#ifndef UTILITY_THREAD_H_
#define UTILITY_THREAD_H_

#include "Base/Types.h"


typedef u32 ( DAEDALUS_THREAD_CALL_TYPE * DaedThread )( void * arg );

typedef intptr_t ThreadHandle;
extern const ThreadHandle	kInvalidThreadHandle;

enum EThreadPriority
{
	TP_LOW = 0,
	TP_NORMAL,
	TP_HIGH,
	TP_TIME_CRITICAL,
	TP_NUM_PRIORITIES,
};

//
//	Returns a thread handle - you must check it for error status (== kInvalidThreadHandle)
//
ThreadHandle	CreateThread( const char * name, DaedThread function, void * argument );

//
//	Adjusts a thread's priority
//
void	SetThreadPriority( ThreadHandle handle, EThreadPriority pri );

//
//	Releases a handle to a thread. This doesn't stop the thread - it just frees resources
//
void	ReleaseThreadHandle( ThreadHandle handle );

//
//	Terminates a running thread
//
void	TerminateThread( ThreadHandle handle );

//
//	Wait the specified time for the thread to finish.
//	Returns true if the thread has terminated, false if it is still running
//	If timeout < 0 this function will block until the thread has finished, otherwise it is a time in MS to wait for
//
bool	JoinThread( ThreadHandle handle, s32 timeout );

//
//	Sleep for the specified number of milliseconds
//
void	ThreadSleepMs( u32 ms );

//
//	Sleep for the specified number of ticks
//
void	ThreadSleepTicks( u32 ticks );

//
//	Yield for a short time
//
void	ThreadYield();

#endif // UTILITY_THREAD_H_
