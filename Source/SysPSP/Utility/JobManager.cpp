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


#include "stdafx.h"
#include "JobManager.h"

#include <string.h>
#include <stdio.h>

#include <pspsdk.h>
#include <pspkernel.h>

#include "Debug/DBGConsole.h"
#include "SysPSP/PRX/MediaEngine/me.h"
#include "SysPSP/Utility/CacheUtil.h"
#include "SysPSP/Utility/ModulePSP.h"
#include "Utility/Mutex.h"
#include "Utility/Thread.h"
#include "Utility/FastMemcpy.h"

#ifdef DAEDALUS_PSP_USE_ME
#include <queue>

std::queue<void *> q;

bool gLoadedMediaEnginePRX {false};

volatile me_struct *mei;
#endif
CJobManager gJobManager( 1024, TM_ASYNC_ME );

bool InitialiseJobManager()
{
#ifdef DAEDALUS_PSP_USE_ME

	if( CModule::Load("mediaengine.prx") < 0 )	return false;

	mei = (volatile struct me_struct *)malloc_64(sizeof(struct me_struct));
	mei = (volatile struct me_struct *)(MAKE_UNCACHED_PTR(mei));
	sceKernelDcacheWritebackInvalidateAll();

	if (InitME(mei) == 0)
	{
		gLoadedMediaEnginePRX = true;
		return true;
	}
	else
	{
		printf(" Couldn't initialize MediaEngine Instance\n");
		return false;
	}
#else
	return false;
#endif
}


//*****************************************************************************
//
//*****************************************************************************
CJobManager::CJobManager( u32 job_buffer_size, ETaskMode task_mode )
:	mJobBuffer( malloc_64( job_buffer_size ) )
,	mRunBuffer( malloc_64( job_buffer_size ) )
,	mJobBufferSize( job_buffer_size )
,	mTaskMode( task_mode )
,	mThread( kInvalidThreadHandle )
,	mWorkReady( sceKernelCreateSema( "JMWorkReady", 0, 0, 1, NULL ) )	// Initval is 0 - i.e. no work ready
,	mWorkEmpty( sceKernelCreateSema( "JMWorkEmpty", 0, 1, 1, NULL ) )	// Initval is 1 - i.e. work done
,	mWantQuit( false )
{
	memset( mRunBuffer, 0, mJobBufferSize );
}

//*****************************************************************************
//
//*****************************************************************************
CJobManager::~CJobManager()
{
	Stop();

	sceKernelDeleteSema(mWorkReady);
	sceKernelDeleteSema(mWorkEmpty);

	if( mJobBuffer != NULL )
	{
		free( mJobBuffer );
	}

	if( mRunBuffer != NULL )
	{
		free( mRunBuffer );
	}
}

//*****************************************************************************
//
//*****************************************************************************
void CJobManager::Start()
{
	if( mThread == kInvalidThreadHandle )
	{

		mWantQuit = false;
		mThread = CreateThread( "JobManager", JobMain, this );

		DAEDALUS_ASSERT( mThread != kInvalidThreadHandle, "Unable to start JobManager thread!" );
	}
}

//*****************************************************************************
//
//*****************************************************************************
void CJobManager::Stop()
{
	if( mThread != kInvalidThreadHandle )
	{
		mWantQuit = true;
		sceKernelWaitThreadEnd(mThread, 0);
		mThread = kInvalidThreadHandle;
	}
}

//*****************************************************************************
//
//*****************************************************************************
u32 CJobManager::JobMain( void * arg )
{
	CJobManager *	job_manager( static_cast< CJobManager * >( arg ) );

	job_manager->Run();

	return 0;
}

//*****************************************************************************
//
//*****************************************************************************
bool CJobManager::AddJob( SJob * job, u32 job_size )
{
	bool	success( false );

	if( job == NULL ){
		success = true;
		return success;
	}

	if( mTaskMode == TM_SYNC )
	{
		if( job->InitJob ) job->InitJob( job );
		if( job->DoJob )   job->DoJob( job );
		if( job->FiniJob ) job->FiniJob( job );
		return true;
	}
	if(!mThread != kInvalidThreadHandle)
	Start();

	//printf( "Adding job...waiting for empty\n" );
	sceKernelWaitSema( mWorkEmpty, 1, NULL );

	// Add job to queue
	if( job_size <= mJobBufferSize )
	{
		// Add job to queue
		memcpy_vfpu( mJobBuffer, job, job_size );

			SJob *	run( static_cast< SJob * >( mJobBuffer ) );

			//clear Cache -> this one is very important without it the CheckME(mei) will not return with the ME status.
			sceKernelDcacheWritebackInvalidateAll();

			// Execute job initialise
			if( run->InitJob )
				run->InitJob( run );

			// Start the job on the ME - inv_all dcache on entry, wbinv_all on exit
			BeginME( mei, (int)run->DoJob, (int)run, -1, NULL, -1, NULL );

		success = true;
	}

	//printf( "Adding job...signaling\n" );
	sceKernelSignalSema( mWorkReady, 1 );

	//printf( "Adding job...done\n" );
	return success;
}

//*****************************************************************************
//
//*****************************************************************************
void CJobManager::Run()
{

	while( true )
	{

		// Check for work with a timeout, in case we want to quit and no more work comes in
		sceKernelWaitSema( mWorkReady, 1, NULL );

		SJob *	run( static_cast< SJob * >( mJobBuffer ) );

				sceKernelDcacheWritebackInvalidateAll();

				while (!mei->done);

				//Mark Job(run) from Mrunbuffer as Finished
				run->FiniJob( run );
				run->FiniJob = NULL; // so it doesn't get run again later

				// signal ready for a new job
				sceKernelSignalSema( mWorkEmpty, 1 );

		// This thread needs to be terminated, so break this loop & kill the me
		if( mWantQuit ){
			KillME(mei);
			break;
		}

	}
	sceKernelExitDeleteThread(0);
}
