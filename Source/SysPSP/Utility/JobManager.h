/*
Copyright (C) 2007 StrmnNrmn

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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef JOBMANAGER_H_
#define JOBMANAGER_H_


enum ETaskMode
{
	TM_SYNC,		// Synchronous
	TM_ASYNC_SC,	// Asynchronous on SC
	TM_ASYNC_ME,	// Asynchronous on ME
};

struct SJob;
typedef int (*JobFunction)( SJob * job );

struct SJob
{
	JobFunction			InitJob;
	JobFunction			DoJob;
	JobFunction			FiniJob;
};

class CJobManager
{
public:
	CJobManager( u32 job_buffer_size, ETaskMode task_mode );
	~CJobManager();

	void			Start();
	void			Stop();

	ETaskMode		GetTaskMode() const		{ return mTaskMode; }

	bool			AddJob( SJob * job, u32 job_size );

private:
	static u32		JobMain( void * arg );

	void			Run();

private:
	void *			mJobBuffer;
	void *			mRunBuffer;
	u32				mJobBufferSize;

	ETaskMode		mTaskMode;
	s32				mThread;
	s32				mWorkReady;
	s32				mWorkEmpty;

	volatile bool	mWantQuit;
};

extern CJobManager gJobManager;

#endif // JOBMANAGER_H_
