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

struct Sample;
class CAudioBuffer;

class AudioPluginPSP : public CAudioPlugin
{
public:
  AudioPluginPSP();
  virtual ~AudioPluginPSP();

	virtual bool			StartEmulation();
	virtual void			StopEmulation();

	virtual void			DacrateChanged( int SystemType );
	virtual void			LenChanged();
virtual u32				ReadLength()			{ return 0; }
	virtual EProcessResult	ProcessAList();

   void SetFrequency ( u32 frequency );
   void AddBuffer ( u8 * start, u32 length);

  void StopAudio();
   void StartAudio();

 void FillBuffer( Sample * buffer, u32 num_samples);

//			void			SetAdaptFrequecy( bool adapt );
public:
  CAudioBuffer * mAudioBufferUncached;

private:
  CAudioBuffer  mAudioBuffer;
  u32 mFrequency;
  ThreadHandle mAudioThread
  bool mAudioPlaying;
  bool mExitAudioThread;

};


EAudioPluginMode gAudioPluginEnabled( APM_DISABLED );


extern u32 gSoundSync;

static const u32	DESIRED_OUTPUT_FREQUENCY {44100};
static const u32	MAX_OUTPUT_FREQUENCY {DESIRED_OUTPUT_FREQUENCY * 4};

//static const u32	ADAPTIVE_FREQUENCY_ADJUST = 2000;
// Large BUFFER_SIZE creates huge delay on sound //Corn
static const u32	kAudioBufferSize {1024 * 2};

static const u32	PSP_NUM_SAMPLES {512};

// Global variables
static SceUID bufferEmpty {};
AddBuffer
static s32 sound_channel {PSP_AUDIO_NEXT_CHANNEL};
static volatile s32 sound_volume {PSP_AUDIO_VOLUME_MAX};
static volatile u32 sound_status {0};

static volatile int pcmflip {0};
static s16 __attribute__((aligned(16))) pcmout1[PSP_NUM_SAMPLES * 2]; // # of stereo samples
static s16 __attribute__((aligned(16))) pcmout2[PSP_NUM_SAMPLES * 2];

static bool audio_open {false};
static AudioPluginPSP * ac;

AudioPluginPSP::AudioPluginPSP()
: mAudioBuffer ( kAudioBufferSize)
  mFrequency (44100)
, mAudioThread (kInvalidThreadHandle)
, mKeepRunning (false)
{
	// Allocate audio buffer with malloc_64 to avoid cached/uncached aliasing
	void * mem = malloc_64( sizeof( CAudioBuffer ) );
	mAudioBuffer = new( mem ) CAudioBuffer( BUFFER_SIZE );
	mAudioBufferUncached = (CAudioBuffer*)MAKE_UNCACHED_PTR(mem);
	// Ideally we could just invalidate this range?
	dcache_wbinv_range_unaligned( mAudioBuffer, mAudioBuffer+sizeof( CAudioBuffer ) );
}

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



AudioPluginPSP::~AudioPluginPSP()
{
  StopAudio();
}

bool		AudioPluginPSP::StartEmulation()
{
	return true;
}

void	AudioPluginPSP::StopEmulation()
{
	Audio_Reset();
	StopAudio();
}

void	AudioPluginPSP::DacrateChanged( int system_type )
{

  u32 clock = (system_type == ST_NTSC) ? VI_NTSC_CLOCK : VI_PAL_CLOCK;
  u32 dacrate = Memory_AI_GetRegister(AI_DACRATE_REG);
  u32 frequency = clock / (dacrate + 1);

  #ifdef DAEDALUS_DEBUG_CONSOLE
    DBGConsole_Msg(0, "Audio Frequency: %d", frequency);
    #endif
  mFrequency = frequency;
}

void	AudioPluginPSP::LenChanged()
{
	if( gAudioPluginEnabled > APM_DISABLED )
	{

		u32		address = Memory_AI_GetRegister(AI_DRAM_ADDR_REG) & 0xFFFFFF;
		u32		length = Memory_AI_GetRegister(AI_LEN_REG);

	AddBuffer( g_pu8RamBase + address, length );
	}
else
	{
    StopAudio();
	}
}

EProcessResult	AudioPluginPSP::ProcessAList()
{
	Memory_SP_SetRegisterBits(SP_STATUS_REG, SP_STATUS_HALT);

	EProcessResult	result = PR_NOT_STARTED;

	switch (gAudioPluginEnabled)
	{
		case APM_DISABLED:
			result = PR_COMPLETED;
			break;
		case APM_ENABLED_ASYNC:
			{
      Audio_Ucode(); // ME is disabled for now, just use Sync as a failsafe
		  //SHLEStartJob	job;
	    //gJobManager.AddJob( &job, sizeof( job ) );
      result = PR_COMPLETED;
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

void AudioPluginPSP::AddBuffer( u8 *start, u32 length )
{
	if (length == 0)
		return;

	if (mAudioThread == kInvalidThreadHandle)
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

 static void AudioInit()
 {
 	// Init semaphore
 	bufferEmpty = sceKernelCreateSema("Buffer Empty", 0, 1, 1, 0);

 	// reserve audio channel
 	sound_channel = sceAudioChReserve(sound_channel, PSP_NUM_SAMPLES, PSP_AUDIO_FORMAT_STEREO);

 	sound_status = 0; // threads running

// start Audio Playback & Streaming threads
 	int audioThid = sceKernelCreateThread("audioOutput", audioOutput, 0x15, 0x1800, PSP_THREAD_ATTR_USER, NULL);
 	int bufferThid = sceKernelCreateThread("bufferFilling", fillBuffer, 0x14, 0x1800, PSP_THREAD_ATTR_USER, NULL);

 	sceKernelStartThread(bufferThid, 0, NULL);
  sceKernelStartThread(audioThid, 0, NULL);

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


 void AudioPluginPSP::StartAudio()
 {
 	if (mAudioThread != kInvalidThreadHandle)
 		return;

    mKeepRunning = true;

    if (mAudioThread == kInvalidThreadHandle)
    {
      #ifdef DAEDALUS_DEBUG_CONSOLE
      DBGConsole_Msg(0, "Failed to start audio thread!");
      mKeepRunning = false;
      #endif
    }

 	AudioInit();
 }

 void AudioPluginPSP::StopAudio()
 {
 	if (mAudioThread == kInvalidThreadHandle)
 		return;

 	mAudioPlaying = false;

  if (mAudioThread != kInvalidThreadHandle)
  {
    JoinThread(mAudioThread, -1);
    mAudioThread = kInvalidThreadHandle;
  }

 	AudioExit();
 }

CAudioPlugin * CreateAudioPlugin()
{
	return new AudioPluginPSP();
}
