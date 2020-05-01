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
#include <queue>

#ifdef DAEDALUS_PSP_USE_ME
bool gLoadedMediaEnginePRX {false};

volatile me_struct *mei;

std::queue<void *> meq;
#endif
CJobManager gJobManager( 256, TM_ASYNC_ME );

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
		#ifdef DAEDALUS_DEBUG_CONSOLE
		printf(" Couldn't initialize MediaEngine Instance\n");
		#endif
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
,	mJobBufferSize( job_buffer_size )
,	mTaskMode( task_mode )
,	mThread( kInvalidThreadHandle )
,	mWorkReady( sceKernelCreateSema( "JMWorkReady", 0, 0, 1, 0) )	// Initval is 0 - i.e. no work ready
,	mWorkEmpty( sceKernelCreateSema( "JMWorkEmpty", 0, 1, 1, 0 ) )	// Initval is 1 - i.e. work done
,	mWantQuit( false )
{
//	memset( mRunBuffer, 0, mJobBufferSize );
}

//*****************************************************************************
//
//*****************************************************************************
CJobManager::~CJobManager()
{

	sceKernelDeleteSema(mWorkReady);
	sceKernelDeleteSema(mWorkEmpty);

	if( mJobBuffer != nullptr )
	{
		free( mJobBuffer );
	}

}

//*****************************************************************************
//
//*****************************************************************************
u32 CJobManager::JobMain( void * arg )
{
	CJobManager *	job_manager( static_cast< CJobManager * >( arg ) );

	return 0;
}

static int mefunloop( SJob * job ){
		while(!meq.empty()){
		dcache_inv_range(&meq,sizeof(meq));
		SJob *	run( static_cast< SJob * >( meq.front() ));
		if( run->InitJob ) run->InitJob( run );
		if( run->DoJob )   run->DoJob( run );
		meq.pop();
		dcache_wbinv_all();
		}
	
	return 0;
}


//*****************************************************************************
//
//*****************************************************************************
bool CJobManager::AddJob( SJob * job, u32 job_size )
{
	bool	success( false );

	if( job == nullptr ){
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

	// Add job to queue
	if( job_size <= mJobBufferSize )
	{
		// Add job to queue
		if (!job == 0){
		memcpy_vfpu( mJobBuffer, job, job_size );
		meq.push(mJobBuffer);
		}
		else{
			return true;
		}

		//clear the Cache
		sceKernelDcacheWritebackAll();

		success = true;
	}

	// Start the job on the ME - inv_all dcache on entry, wbinv_all on exit
	// if the me is busy run the job on the main cpu so we don't stall
	if(CheckME(mei));
	BeginME( mei, (int)mefunloop, (int)NULL, -1, NULL, -1, NULL);

	//printf( "Adding job...done\n" );
	return success;
}
