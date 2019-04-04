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

extern u32 gSoundSync;

static const u32	DESIRED_OUTPUT_FREQUENCY {44100};
static const u32	MAX_OUTPUT_FREQUENCY {DESIRED_OUTPUT_FREQUENCY * 4};

//static const u32	ADAPTIVE_FREQUENCY_ADJUST = 2000;


static const u32	PSP_NUM_SAMPLES {512};

// Global variables
static SceUID bufferEmpty {};
static s32 sound_channel {PSP_AUDIO_NEXT_CHANNEL};
static volatile s32 sound_volume {PSP_AUDIO_VOLUME_MAX};
static volatile u32 sound_status {0};

static volatile int pcmflip {0};
static s16 __attribute__((aligned(16))) pcmout1[PSP_NUM_SAMPLES * 2]; // # of stereo samples
static s16 __attribute__((aligned(16))) pcmout2[PSP_NUM_SAMPLES * 2];
static const u32 kOutputFrequency = 44100;
static const u32	kAudioBufferSize {1024 * 2}; // Large BUFFER_SIZE creates huge delay on sound //Corn

static bool audio_open {false};

EAudioPluginMode gAudioPluginEnabled = APM_DISABLED;

#define DEBUG_AUDIO 1

#if DEBUG_AUDIO
#define DPF_AUDIO(...)	do { printf(__VA_ARGS__); } while(0)
#else
#define DPF_AUDIO(...)	do { (void)sizeof(__VA_ARGS__); } while(0)
#endif


class AudioPluginPSP : public CAudioPlugin
{
public:
  AudioPluginPSP();
  virtual ~AudioPluginPSP();

	virtual bool			StartEmulation();
	virtual void			StopEmulation();

	virtual void			DacrateChanged( int system_type );
	virtual void			LenChanged();
  virtual u32				ReadLength()			{ return 0; }
	virtual EProcessResult	ProcessAList();

   void AddBuffer ( void * ptr, u32 length);

  void StopAudio();
  void StartAudio();

  CAudioBuffer * mAudioBufferUncached;

private:
  CAudioBuffer  mAudioBuffer;
  u32 mFrequency;
  ThreadHandle mAudioThread;
  volatile bool mKeepRunning; // Should the audio thread keep running?
  volatile u32 mBufferLenMs;

};


static AudioPluginPSP * ac;

AudioPluginPSP::AudioPluginPSP()
: mAudioBuffer ( kAudioBufferSize)
, mFrequency (44100)
, mAudioThread (kInvalidThreadHandle)
, mKeepRunning (false)
, mBufferLenMs(0)
{
	 // Allocate audio buffer with malloc_64 to avoid cached/uncached aliasing
 void * mem = malloc_64( sizeof( CAudioBuffer ) );
	 //mAudioBuffer = new( mem ) CAudioBuffer( kAudioBufferSize );
 mAudioBufferUncached = (CAudioBuffer*)MAKE_UNCACHED_PTR(mem);
	// // Ideally we could just invalidate this range?
	 //dcache_wbinv_range_unaligned( mAudioBuffer, mAudioBuffer+sizeof( CAudioBuffer ) );
}


AudioPluginPSP::~AudioPluginPSP()
{
  StopAudio();
}

bool	AudioPluginPSP::StartEmulation()
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

EProcessResult AudioPluginPSP::ProcessAList()
{
	Memory_SP_SetRegisterBits(SP_STATUS_REG, SP_STATUS_HALT);

	EProcessResult result = PR_NOT_STARTED;

	switch (gAudioPluginEnabled)
	{
		case APM_DISABLED:
			result = PR_COMPLETED;
			break;
		case APM_ENABLED_ASYNC:
			DAEDALUS_ERROR("Async audio is unimplemented");
			Audio_Ucode();
			result = PR_COMPLETED;
			break;
		case APM_ENABLED_SYNC:
			Audio_Ucode();
			result = PR_COMPLETED;
			break;
	}

	return result;
}

void AudioPluginPSP::AddBuffer( void * ptr, u32 length )
{
if (length == 0)
    return;

    if (mAudioThread == kInvalidThreadHandle)
    StartAudio();

    u32 num_samples = length / sizeof (Sample);
    mAudioBuffer.AddSamples( reinterpret_cast<const Sample *>(ptr), num_samples, mFrequency, kOutputFrequency);
    u32 remaining_samples = mAudioBuffer.GetNumBufferedSamples();
    mBufferLenMs = (1000 * remaining_samples) / kOutputFrequency;
    float ms = (float)num_samples * 1000.f / (float)mFrequency;
    #ifdef DAEDALUS_DEBUG_CONSOLE
      DPF_AUDIO("Queuing %d samples @%dHz - %.2fms - bufferlen now %d\n",
      num_samples, mFrequency, ms, mBufferLenMs);
    #endif
// As the sound is just sync we just add the samples normally

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


 void AudioPluginPSP::StartAudio()
 {
   if (mAudioThread != kInvalidThreadHandle)
 		return;

    mKeepRunning = true;

    bufferEmpty = sceKernelCreateSema("Buffer Empty", 0, 1, 1, 0);

  	// reserve audio channel
  	sound_channel = sceAudioChReserve(sound_channel, PSP_NUM_SAMPLES, PSP_AUDIO_FORMAT_STEREO);

  	sound_status = 0; // threads running
    // create audio playback thread to provide timing
  	mAudioThread = sceKernelCreateThread("audioOutput", audioOutput, 0x15, 0x1800, PSP_THREAD_ATTR_USER, NULL);

    if (mAudioThread == kInvalidThreadHandle)
    {
      #ifdef DAEDALUS_DEBUG_CONSOLE
      DBGConsole_Msg(0, "Failed to start audio thread!");
            #endif
      mKeepRunning = false;
    }
      	sceKernelStartThread(mAudioThread, 0, NULL);

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
  	mKeepRunning = true;
   }


 void AudioPluginPSP::StopAudio()
 {
 	if (mAudioThread == kInvalidThreadHandle)
 		return;

 	mKeepRunning = false;

  if (mAudioThread != kInvalidThreadHandle)
  {
    JoinThread(mAudioThread, -1);
    mAudioThread = kInvalidThreadHandle;
    sound_status = 0xDEADBEEF;
    sceKernelSignalSema(bufferEmpty, 1); // fillbuffer thread is probably waiting.
    sceKernelDelayThread(100*1000);
    sceAudioChRelease(sound_channel);
    sound_channel = PSP_AUDIO_NEXT_CHANNEL;
  }
  // Stop stream
 	mKeepRunning = false;

 	// Delete semaphore
 	sceKernelDeleteSema(bufferEmpty);
 }

CAudioPlugin * CreateAudioPlugin()
{
	return new AudioPluginPSP();
}
