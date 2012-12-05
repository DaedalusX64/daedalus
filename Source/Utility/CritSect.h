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

#ifndef CRITSECT_H__
#define CRITSECT_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef DAEDALUS_PSP
#include <pspthreadman.h>
#endif

#if defined(DAEDALUS_W32)

class CCritSect
{
	public:

		CCritSect()
		{
			InitializeCriticalSection(&cs);
		}

		explicit CCritSect( const char * name )
		{
			use( name );		// Can't name?
			InitializeCriticalSection(&cs);
		};

		~CCritSect()
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

	protected:
		CRITICAL_SECTION cs;
};
#elif defined(DAEDALUS_PSP)

class CCritSect
{
	public:

		CCritSect()
			:	mSemaphore( sceKernelCreateSema( "CCritSect", 0, 1, 1, NULL ) )
		{
			DAEDALUS_ASSERT( mSemaphore >= 0, "Unable to create semaphore" );
		}

		explicit CCritSect( const char * name )
			:	mSemaphore( sceKernelCreateSema( name, 0, 1, 1, NULL ) )
		{
			DAEDALUS_ASSERT( mSemaphore >= 0, "Unable to create semaphore" );
		}

		~CCritSect()
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

#else

	// XXXX XXXX XXXX
class CCritSect
{
	public:

		CCritSect()
		{
		}

		explicit CCritSect( const char * name )
		{
		}

		~CCritSect()
		{
		}

		void Lock()
		{
		}

		void Unlock()
		{
		}
};

#endif


class CAutoCritSect
{
public:
	explicit CAutoCritSect( CCritSect & section ) : mSection( section )
	{
		mSection.Lock();
	}
	~CAutoCritSect()
	{
		mSection.Unlock();
	}

private:
	CCritSect &	mSection;
};

#define AUTO_CRIT_SECT( x )		CAutoCritSect daed_auto_crit_sect( x )


#endif //#ifndef CRITSECT_H__
