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

#if defined(DAEDALUS_POSIX) || defined(DAEDALUS_PSP) || defined(DAEDALUS_CTR)
#include <pthread.h>
#endif

#include <mutex>
class Mutex
{
public:

	Mutex()
	{
		// pthread_mutex_init(&mMutex, NULL);
		std::mutex mMutex;
	}

	explicit Mutex( const char * name )
	{
		std::mutex mMutex;
		// pthread_mutex_init(&mMutex, NULL);
	}

	~Mutex()
	{
		// pthread_mutex_destroy(&mMutex);
	}

	void Lock()
	{
		// pthread_mutex_lock(&mMutex);
			mMutex.lock();
	}

	void Unlock()
	{
		// pthread_mutex_unlock(&mMutex);
		 	mMutex.unlock();
	}

public:
	std::mutex  mMutex;
};


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
