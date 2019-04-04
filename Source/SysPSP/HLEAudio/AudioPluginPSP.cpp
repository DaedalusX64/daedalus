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

#include <stdio.h>
#include <new>

#include <pspkernel.h>
#include <pspaudio.h>


#include "AudioOutput.h"
#include "HLEAudio/audiohle.h"
#include "Plugins/AudioPlugin.h"

#include "Config/ConfigOptions.h"
#include "Debug/DBGConsole.h"
#include "HLEAudio/AudioBuffer.h"
#include "SysPSP/Utility/CacheUtil.h"
#include "Utility/FramerateLimiter.h"
#include "Utility/Thread.h"
#include "Core/CPU.h"
#include "Core/Interrupt.h"
#include "Core/Memory.h"
#include "Core/ROM.h"
#include "Core/RSP_HLE.h"

#define RSP_AUDIO_INTR_CYCLES     1

/* This sets default frequency what is used if rom doesn't want to change it.
   Probably only game that needs this is Zelda: Ocarina Of Time Master Quest
   *NOTICE* We should try to find out why Demos' frequencies are always wrong
   They tend to rely on a default frequency, apparently, never the same one ;)*/

#define DEFAULT_FREQUENCY 44100	// Taken from Mupen64 : )

static AudioOutput * ac;

class AudioPluginPSP : public CAudioPlugin
{
public:
  AudioPluginPSP();
  virtual ~AudioPluginPSP();

	virtual bool			StartEmulation();
	virtual void			StopEmulation();

	virtual void			DacrateChanged( int SystemType );
	virtual void			LenChanged();
	virtual u32				ReadLength();
	virtual EProcessResult	ProcessAList();

//			void			SetAdaptFrequecy( bool adapt );

private:
	AudioOutput *			mAudioOutput;
};


EAudioPluginMode gAudioPluginEnabled( APM_DISABLED );


AudioPluginPSP::~AudioPluginPSP() {}

AudioPluginPSP::AudioPluginPSP()
:	mAudioOutput( new AudioOutput )
{}


extern u32 gSoundSync;

static const u32	DESIRED_OUTPUT_FREQUENCY {44100};
static const u32	MAX_OUTPUT_FREQUENCY {DESIRED_OUTPUT_FREQUENCY * 4};

//static const u32	ADAPTIVE_FREQUENCY_ADJUST = 2000;
// Large BUFFER_SIZE creates huge delay on sound //Corn
static const u32	BUFFER_SIZE {1024 * 2};

static const u32	PSP_NUM_SAMPLES {512};

// Global variables
static SceUID bufferEmpty {};

static s32 sound_channel {PSP_AUDIO_NEXT_CHANNEL};
static volatile s32 sound_volume {PSP_AUDIO_VOLUME_MAX};
static volatile u32 sound_status {0};

static volatile int pcmflip {0};
static s16 __attribute__((aligned(16))) pcmout1[PSP_NUM_SAMPLES * 2]; // # of stereo samples
static s16 __attribute__((aligned(16))) pcmout2[PSP_NUM_SAMPLES * 2];

static bool audio_open {false};



static int fillBuffer(SceSize args, void *argp)
{
	s16 *fillbuf {0};

	while(sound_status != 0xDEADBEEF)
	{
		sceKernelWaitSema(bufferEmpty, 1, 0);
		fillbuf = pcmflip ? pcmout2 : pcmout1;

		ac->mAudioBufferUncached->Drain( reinterpret_cast< Sample * >( fillbuf ), PSP_NUM_SAMPLES );
	}
	sceKernelExitDeleteThread(0);
	return 0;
}

static int audioOutput(SceSize args, void *argp)
{
	s16 *playbuf {0};

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

static void AudioInit()
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

static void AudioExit()
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

AudioOutput::AudioOutput()
:	mAudioPlaying( false )
,	mFrequency( 44100 )
{
	// Allocate audio buffer with malloc_64 to avoid cached/uncached aliasing
	void * mem = malloc_64( sizeof( CAudioBuffer ) );
	mAudioBuffer = new( mem ) CAudioBuffer( BUFFER_SIZE );
	mAudioBufferUncached = (CAudioBuffer*)MAKE_UNCACHED_PTR(mem);
	// Ideally we could just invalidate this range?
	dcache_wbinv_range_unaligned( mAudioBuffer, mAudioBuffer+sizeof( CAudioBuffer ) );
}

AudioOutput::~AudioOutput( )
{
	StopAudio();

	mAudioBuffer->~CAudioBuffer();
	free( mAudioBuffer );
}

void AudioOutput::SetFrequency( u32 frequency )
{
	DBGConsole_Msg( 0, "Audio frequency: %d", frequency );
	mFrequency = frequency;
}


bool		AudioPluginPSP::StartEmulation()
{
	return true;
}
void	AudioPluginPSP::StopEmulation()
{
	Audio_Reset();
	mAudioOutput->StopAudio();
}

void	AudioPluginPSP::DacrateChanged( int SystemType )
{
//	printf( "DacrateChanged( %s )\n", (SystemType == ST_NTSC) ? "NTSC" : "PAL" );
	u32 type {(u32)((SystemType == ST_NTSC) ? VI_NTSC_CLOCK : VI_PAL_CLOCK)};
	u32 dacrate {Memory_AI_GetRegister(AI_DACRATE_REG)};
	u32	frequency {type / (dacrate + 1)};

	mAudioOutput->SetFrequency( frequency );
}


void	AudioPluginPSP::LenChanged()
{
	if( gAudioPluginEnabled > APM_DISABLED )
	{
		//mAudioOutput->SetAdaptFrequency( gAdaptFrequency );

		u32		address( Memory_AI_GetRegister(AI_DRAM_ADDR_REG) & 0xFFFFFF );
		u32		length(Memory_AI_GetRegister(AI_LEN_REG));

		mAudioOutput->AddBuffer( g_pu8RamBase + address, length );
	}
	else
	{
		mAudioOutput->StopAudio();
	}
}


u32		AudioPluginPSP::ReadLength()
{
	return 0;
}


EProcessResult	AudioPluginPSP::ProcessAList()
{
	Memory_SP_SetRegisterBits(SP_STATUS_REG, SP_STATUS_HALT);

	EProcessResult	result( PR_NOT_STARTED );

	switch( gAudioPluginEnabled )
	{
		case APM_DISABLED:
			result = PR_COMPLETED;
			break;
		case APM_ENABLED_ASYNC:
			{
		//		SHLEStartJob	job;
	//			gJobManager.AddJob( &job, sizeof( job ) );
			}
			result = PR_STARTED;
			break;
		case APM_ENABLED_SYNC:
			Audio_Ucode();
			result = PR_COMPLETED;
			break;
	}

	return result;
}

void AudioOutput::AddBuffer( u8 *start, u32 length )
{
	if (length == 0)
		return;

	if (!mAudioPlaying)
		StartAudio();

	u32 num_samples {length / sizeof( Sample )};


	switch( gAudioPluginEnabled )
	{
	case APM_DISABLED:
		break;

	case APM_ENABLED_ASYNC:
		{
//			SAddSamplesJob	job( mAudioBufferUncached, reinterpret_cast< const Sample * >( start ), num_samples, mFrequency, 44100 );

	//		gJobManager.AddJob( &job, sizeof( job ) );
		}
		break;

	case APM_ENABLED_SYNC:
		{
			mAudioBufferUncached->AddSamples( reinterpret_cast< const Sample * >( start ), num_samples, mFrequency, 44100 );
		}
		break;
	}
}

void AudioOutput::StartAudio()
{
	if (mAudioPlaying)
		return;

	mAudioPlaying = true;

	ac = this;

	AudioInit();
}

void AudioOutput::StopAudio()
{
	if (!mAudioPlaying)
		return;

	mAudioPlaying = false;

	AudioExit();
}



CAudioPlugin * CreateAudioPlugin()
{
	return new AudioPluginPSP();
}
