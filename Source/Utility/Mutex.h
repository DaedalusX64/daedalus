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

#ifndef UTILITY_MUTEX_H_
#define UTILITY_MUTEX_H_

#ifdef DAEDALUS_PSP
#include <pspthreadman.h>
#endif
#if defined(DAEDALUS_OSX) || defined(DAEDALUS_LINUX)
#include <pthread.h>
#endif

#if defined(DAEDALUS_W32)

class Mutex
{
public:

	Mutex()
	{
		InitializeCriticalSection(&cs);
	}

	explicit Mutex( const char * name )
	{
		InitializeCriticalSection(&cs);
	};

	~Mutex()
	{
		DeleteCriticalSection(&cs);
	}

	void Lock()
	{
		EnterCriticalSection(&cs);
	}

	void Unlock()
	{
		LeaveCriticalSection(&cs);
	}

public:
	CRITICAL_SECTION cs;
};
#elif defined(DAEDALUS_PSP)

class Mutex
{
public:

	Mutex()
		:	mSemaphore( sceKernelCreateSema( "Mutex", 0, 1, 1, NULL ) )
	{
		DAEDALUS_ASSERT( mSemaphore >= 0, "Unable to create semaphore" );
	}

	explicit Mutex( const char * name )
		:	mSemaphore( sceKernelCreateSema( name, 0, 1, 1, NULL ) )
	{
		DAEDALUS_ASSERT( mSemaphore >= 0, "Unable to create semaphore" );
	}

	~Mutex()
	{
		sceKernelDeleteSema( mSemaphore );
	}

	void Lock()
	{
		sceKernelWaitSema( mSemaphore, 1, NULL );
	}

	void Unlock()
	{
		sceKernelSignalSema( mSemaphore, 1 );
	}

private:
	s32	mSemaphore;
};

#elif defined(DAEDALUS_OSX) || defined(DAEDALUS_LINUX)

class Mutex
{
public:

	Mutex()
	{
		pthread_mutex_init(&mMutex, NULL);
	}

	explicit Mutex( const char * name )
	{
		pthread_mutex_init(&mMutex, NULL);
	}

	~Mutex()
	{
		pthread_mutex_destroy(&mMutex);
	}

	void Lock()
	{
		pthread_mutex_lock(&mMutex);
	}

	void Unlock()
	{
		pthread_mutex_unlock(&mMutex);
	}

public:
	pthread_mutex_t  mMutex;
};


#else

#error Unhandled platform

#endif


class MutexLock
{
public:
	explicit MutexLock( Mutex * mutex )
	:	mOwnedMutex( mutex )
	{
		if (mOwnedMutex)
			mOwnedMutex->Lock();
	}
	~MutexLock()
	{
		if (mOwnedMutex)
			mOwnedMutex->Unlock();
	}

	void Set(Mutex * mutex)
	{
		if (mOwnedMutex)
			mOwnedMutex->Unlock();
		mOwnedMutex = mutex;
		if (mOwnedMutex)
			mOwnedMutex->Lock();
	}

	bool HasLock(const Mutex & mutex) const
	{
		return mOwnedMutex == &mutex;
	}

private:
	Mutex *	mOwnedMutex;
};

#define AUTO_CRIT_SECT( x )		MutexLock daed_auto_crit_sect( &x )


#endif // UTILITY_MUTEX_H_
