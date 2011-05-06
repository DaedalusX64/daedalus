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

#ifndef THREAD_H_
#define THREAD_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


typedef u32 ( DAEDALUS_THREAD_CALL_TYPE * DaedThread )( void * arg );

typedef s32 ThreadHandle;
extern const ThreadHandle	INVALID_THREAD_HANDLE;

enum EThreadPriority
{
	TP_LOW = 0,
	TP_NORMAL,
	TP_HIGH,
	TP_TIME_CRITICAL,
	TP_NUM_PRIORITIES,
};

//
//	Returns a thread handle - you must check it for error status (== INVALID_THREAD_HANDLE)
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
bool	WaitForThreadTermination( ThreadHandle handle, s32 timeout );

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

#endif // THREAD_H_
