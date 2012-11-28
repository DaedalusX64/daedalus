/*
Copyright (C) 2003 Azimer
Copyright (C) 2001,2006 StrmnNrmn

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

//
//	N.B. This source code is derived from Azimer's Audio plugin (v0.55?)
//	and modified by StrmnNrmn to work with Daedalus PSP. Thanks Azimer!
//	Drop me a line if you get chance :)
//

#include "stdafx.h"

#include "Utility/FramerateLimiter.h"
#include "HLEAudio/AudioCode.h"
#include "HLEAudio/AudioBuffer.h"

#include "Debug/DBGConsole.h"

#include "Utility/Thread.h"

#include <pspkernel.h>
#include <pspaudio.h>

#include <stdio.h>
#include <new>

#include "SysPSP/Utility/JobManager.h"
#include "SysPSP/Utility/CacheUtil.h"

#include "ConfigOptions.h"

static const u32	DESIRED_OUTPUT_FREQUENCY = 44100;
static const u32	MAX_OUTPUT_FREQUENCY = DESIRED_OUTPUT_FREQUENCY * 4;

//static const u32	ADAPTIVE_FREQUENCY_ADJUST = 2000;
// Large BUFFER_SIZE creates huge delay on sound //Corn
static const u32	BUFFER_SIZE = 1024 * 2;

static const u32	PSP_NUM_SAMPLES = 512;

// Global variables
static SceUID bufferEmpty;

static s32 sound_channel = PSP_AUDIO_NEXT_CHANNEL;
static volatile s32 sound_volume = PSP_AUDIO_VOLUME_MAX;
static volatile u32 sound_status = 0;

static volatile int pcmflip = 0;
static s16 __attribute__((aligned(16))) pcmout1[PSP_NUM_SAMPLES * 2]; // # of stereo samples
static s16 __attribute__((aligned(16))) pcmout2[PSP_NUM_SAMPLES * 2];

static bool audio_open = false;

static AudioCode * ac;

namespace
{

/*
 * Audio Threads
 *
 */

static int fillBuffer(SceSize args, void *argp)
{
	s16 *fillbuf;

	while(sound_status != 0xDEADBEEF)
	{
		sceKernelWaitSema(bufferEmpty, 1, 0);
		fillbuf = pcmflip ? pcmout2 : pcmout1;

		ac->mAudioBufferUncached->Fill( reinterpret_cast< Sample * >( fillbuf ), PSP_NUM_SAMPLES );
		/*
		if( ac->mAdaptFrequency && ac->mAudioBuffer->GetSize() < PSP_NUM_SAMPLES )
		{
			ac->DodgeBufferUnderflow();
		}*/

	}
	sceKernelExitDeleteThread(0);
	return 0;
}

static int audioOutput(SceSize args, void *argp)
{
	s16 *playbuf;

	while(sound_status != 0xDEADBEEF)
	{
		playbuf = pcmflip ? pcmout1 : pcmout2;
		pcmflip ^= 1;
		sceKernelSignalSema(bufferEmpty, 1);
		sceAudioOutputBlocking(sound_channel, sound_volume, playbuf);
	}
	sceKernelExitDeleteThread(0);
	return 0;
}

/*
 *  Initialization
 *
 */

static void AudioInit(void)
{
	// Init semaphore
	bufferEmpty = sceKernelCreateSema("Buffer Empty", 0, 1, 1, 0);

	// reserve audio channel
	sound_channel = sceAudioChReserve(sound_channel, PSP_NUM_SAMPLES, PSP_AUDIO_FORMAT_STEREO);

	sound_status = 0; // threads running

	// create audio playback thread to provide timing
	int audioThid = sceKernelCreateThread("audioOutput", audioOutput, 0x15, 0x1800, PSP_THREAD_ATTR_USER, NULL);
	if(audioThid < 0)
	{
		printf("FATAL: Cannot create audioOutput thread\n");
		return; // no audio
	}
	sceKernelStartThread(audioThid, 0, NULL);

	// Start streaming thread
	int bufferThid = sceKernelCreateThread("bufferFilling", fillBuffer, 0x14, 0x1800, PSP_THREAD_ATTR_USER, NULL);
	if(bufferThid < 0)
	{
		sound_status = 0xDEADBEEF; // kill the audioOutput thread
		sceKernelDelayThread(100*1000);
		sceAudioChRelease(sound_channel);
		sound_channel = PSP_AUDIO_NEXT_CHANNEL;
		printf("FATAL: Cannot create bufferFilling thread\n");
		return;
	}
	sceKernelStartThread(bufferThid, 0, NULL);

	// Everything OK
	audio_open = true;
}


/*
 *  Deinitialization
 */

static void AudioExit(void)
{
	// Stop stream
	if (audio_open)
	{
		sound_status = 0xDEADBEEF;
		sceKernelSignalSema(bufferEmpty, 1); // fillbuffer thread is probably waiting.
		sceKernelDelayThread(100*1000);
		sceAudioChRelease(sound_channel);
		sound_channel = PSP_AUDIO_NEXT_CHANNEL;
	}

	audio_open = false;

	// Delete semaphore
	sceKernelDeleteSema(bufferEmpty);
}

}

//*****************************************************************************
//
//*****************************************************************************
AudioCode::AudioCode()
:	mAudioPlaying( false )
,	mFrequency( 44100 )
,	mOutputFrequency( DESIRED_OUTPUT_FREQUENCY )
//,	mAdaptFrequency( false )
,	mBufferLength( 0 )
{
	// Allocate audio buffer with malloc_64 to avoid cached/uncached aliasing
	void * mem = malloc_64( sizeof( CAudioBuffer ) );
	mAudioBuffer = new( mem ) CAudioBuffer( BUFFER_SIZE );
	mAudioBufferUncached = (CAudioBuffer*)MAKE_UNCACHED_PTR(mem);
	// Ideally we could just invalidate this range?
	dcache_wbinv_range_unaligned( mAudioBuffer, mAudioBuffer+sizeof( CAudioBuffer ) );
}

//*****************************************************************************
//
//*****************************************************************************
AudioCode::~AudioCode( )
{
	StopAudio();

	mAudioBuffer->~CAudioBuffer();
	free( mAudioBuffer );
}

//*****************************************************************************
//
//*****************************************************************************
void AudioCode::SetFrequency( u32 frequency )
{
	DBGConsole_Msg( 0, "Audio frequency: %d", frequency );
	mFrequency = frequency;
}

//*****************************************************************************
//
//*****************************************************************************
/*
void	AudioCode::SetAdaptFrequency( bool adapt )
{
	mAdaptFrequency = adapt;
	mOutputFrequency = DESIRED_OUTPUT_FREQUENCY;
}
*/

struct SAddSamplesJob : public SJob
{
	CAudioBuffer *		mBuffer;
	const Sample *		mSamples;
	u32					mNumSamples;
	u32					mFrequency;
	u32					mOutputFreq;
	volatile u32 *		mBufferLength;

	SAddSamplesJob( CAudioBuffer * buffer, const Sample * samples, u32 num_samples, u32 frequency, u32 output_freq, volatile u32 * buffer_len )
		:	mBuffer( buffer )
		,	mSamples( samples )
		,	mNumSamples( num_samples )
		,	mFrequency( frequency )
		,	mOutputFreq( output_freq )
		,	mBufferLength( buffer_len )
	{
		InitJob = NULL;
		DoJob = &DoAddSamplesStatic;
		FiniJob = &DoJobComplete;
	}

	static int DoAddSamplesStatic( SJob * arg )
	{
		SAddSamplesJob *	job( static_cast< SAddSamplesJob * >( arg ) );
		return job->DoAddSamples();
	}

	static int DoJobComplete( SJob * arg )
	{
		SAddSamplesJob *	job( static_cast< SAddSamplesJob * >( arg ) );
		return job->DoJobComplete();
	}

	int DoAddSamples()
	{
		mBuffer->AddSamples( mSamples, mNumSamples, mFrequency, mOutputFreq );
		return 0;
	}

	int DoJobComplete()
	{
		*mBufferLength = 0;		// Clear the length indicator

		/*u32	output_samples( ( PSP_NUM_SAMPLES * mOutputFreq ) / mFrequency );
		if( ac->mAdaptFrequency && mBuffer->GetSize() > (BUFFER_SIZE - output_samples) )
		{
			ac->DodgeBufferOverflow();
		}*/
		return 0;
	}

};

//*****************************************************************************
//
//*****************************************************************************
extern u32 gSoundSync;
u32 AudioCode::AddBuffer( u8 *start, u32 length )
{
	if (length == 0)
		return 0;

	if (!mAudioPlaying)
		StartAudio();

	u32		num_samples( length / sizeof( Sample ) );

	mBufferLength = num_samples;

	//Adapt Audio to sync% //Corn
	if(gAudioRateMatch)
	{
		if(gSoundSync > 88200) mOutputFrequency = 88200;	//limit upper rate
		else if(gSoundSync < 44100) mOutputFrequency = 44100;	//limit lower rate
		else mOutputFrequency = gSoundSync;
	}
	else
		mOutputFrequency = DESIRED_OUTPUT_FREQUENCY;

	switch( gAudioPluginEnabled )
	{
	case APM_DISABLED:
		break;

	case APM_ENABLED_ASYNC:
		{
			SAddSamplesJob	job( mAudioBufferUncached, reinterpret_cast< const Sample * >( start ), num_samples, mFrequency, mOutputFrequency, &mBufferLength );

			gJobManager.AddJob( &job, sizeof( job ) );
		}
		break;

	case APM_ENABLED_SYNC:
		{
			mAudioBufferUncached->AddSamples( reinterpret_cast< const Sample * >( start ), num_samples, mFrequency, mOutputFrequency );
		}
		break;
	}

	return 0;
}

//*****************************************************************************
//
//*****************************************************************************
void AudioCode::StartAudio()
{
	if (mAudioPlaying)
		return;

	mAudioPlaying = true;

	ac = this;

	AudioInit();
}

//*****************************************************************************
//
//*****************************************************************************
void AudioCode::StopAudio()
{
	if (!mAudioPlaying)
		return;

	mAudioPlaying = false;

	AudioExit();
}

//*****************************************************************************
//
//*****************************************************************************
u32 AudioCode::GetReadStatus()
{
	//dcache_wbinv_all();
	//printf( "Length is currently %d\n", mBufferLength );
	//return mBufferLength;
	return 0;
}

//*****************************************************************************
//
//*****************************************************************************
/*
void	AudioCode::DodgeBufferUnderflow()
{
	//DAEDALUS_ASSERT( mAdaptFrequency, "Shouldn't be dodging" );

	u32 adjust( ADAPTIVE_FREQUENCY_ADJUST );
	if(mOutputFrequency + adjust < MAX_OUTPUT_FREQUENCY)
	{
		mOutputFrequency += adjust;
		//DBGConsole_Msg( 0, "Adjusting freq to %d", mOutputFrequency );
	}
	else
	{
		mOutputFrequency = MAX_OUTPUT_FREQUENCY;
	}
}
*/
//*****************************************************************************
//
//*****************************************************************************
/*
void	AudioCode::DodgeBufferOverflow()
{
	//DAEDALUS_ASSERT( mAdaptFrequency, "Shouldn't be dodging" );

	u32 adjust( ADAPTIVE_FREQUENCY_ADJUST );
	if( mOutputFrequency - adjust > (DESIRED_OUTPUT_FREQUENCY>>1) )
	{
		mOutputFrequency -= adjust;
		//DBGConsole_Msg( 0, "Restoring freq to %d", mOutputFrequency );
	}
	else
	{
		mOutputFrequency = DESIRED_OUTPUT_FREQUENCY>>1;
	}

}
*/