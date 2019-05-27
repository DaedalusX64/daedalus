/*
Copyright (C) 2003 Azimer
Copyright (C) 2012 StrmnNrmn

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
#include "Plugins/AudioPlugin.h"

#include <stdio.h>

#include <AudioToolbox/AudioQueue.h>
#include <CoreAudio/CoreAudioTypes.h>
#include <CoreFoundation/CFRunLoop.h>

#include "Config/ConfigOptions.h"
#include "Core/Memory.h"
#include "Debug/DBGConsole.h"
#include "HLEAudio/AudioBuffer.h"
#include "HLEAudio/audiohle.h"
#include "Utility/FramerateLimiter.h"
#include "Utility/Thread.h"
#include "Utility/Timing.h"

EAudioPluginMode gAudioPluginEnabled = APM_DISABLED;

#define DEBUG_AUDIO  0

#if DEBUG_AUDIO
#define DPF_AUDIO(...)	do { printf(__VA_ARGS__); } while(0)
#else
#define DPF_AUDIO(...)	do { (void)sizeof(__VA_ARGS__); } while(0)
#endif

static const u32 kOutputFrequency = 44100;
static const u32 kAudioBufferSize = 1024 * 1024;	// Circular buffer length. Converts N64 samples out our output rate.
static const u32 kNumChannels = 2;

// How much input we try to keep buffered in the synchronisation code.
// Setting this too low and we run the risk of skipping.
// Setting this too high and we run the risk of being very laggy.
static const u32 kMaxBufferLengthMs = 30;

// AudioQueue buffer object count and length.
// Increasing either of these numbers increases the amount of buffered
// audio which can help reduce crackling (empty buffers) at the cost of lag.
static const u32 kNumBuffers = 3;
static const u32 kAudioQueueBufferLength = 1 * 1024;

class AudioPluginOSX : public CAudioPlugin
{
public:
	AudioPluginOSX();
	virtual ~AudioPluginOSX();

	virtual bool			StartEmulation();
	virtual void			StopEmulation();

	virtual void			DacrateChanged(int system_type);
	virtual void			LenChanged();
	virtual u32				ReadLength()			{ return 0; }
	virtual EProcessResult	ProcessAList();

	void					AddBuffer(void * ptr, u32 length);	// Uploads a new buffer and returns status

	void					StopAudio();						// Stops the Audio PlayBack (as if paused)
	void					StartAudio();						// Starts the Audio PlayBack (as if unpaused)

	static void				AudioSyncFunction(void * arg);
	static void 			AudioCallback(void * arg, AudioQueueRef queue, AudioQueueBufferRef buffer);
	static u32 				AudioThread(void * arg);

private:
	CAudioBuffer			mAudioBuffer;
	u32						mFrequency;
	ThreadHandle 			mAudioThread;
	volatile bool			mKeepRunning;	// Should the audio thread keep running?

	volatile u32 			mBufferLenMs;
};

AudioPluginOSX::AudioPluginOSX()
:	mAudioBuffer( kAudioBufferSize )
,	mFrequency( 44100 )
,	mAudioThread( kInvalidThreadHandle )
,	mKeepRunning( false )
,	mBufferLenMs( 0 )
{
}

AudioPluginOSX::~AudioPluginOSX()
{
	StopAudio();
}

bool AudioPluginOSX::StartEmulation()
{
	return true;
}

void AudioPluginOSX::StopEmulation()
{
	Audio_Reset();
	StopAudio();
}

void AudioPluginOSX::DacrateChanged(int system_type)
{
	u32 clock      = (system_type == ST_NTSC) ? VI_NTSC_CLOCK : VI_PAL_CLOCK;
	u32 dacrate   = Memory_AI_GetRegister(AI_DACRATE_REG);
	u32	frequency = clock / (dacrate + 1);

	DBGConsole_Msg(0, "Audio frequency: %d", frequency);
	mFrequency = frequency;
}

void AudioPluginOSX::LenChanged()
{kOutputFre
	if (gAudioPluginEnabled > APM_DISABLED)
	{
		u32	address = Memory_AI_GetRegister(AI_DRAM_ADDR_REG) & 0xFFFFFF;
		u32	length  = Memory_AI_GetRegister(AI_LEN_REG);

		AddBuffer( g_pu8RamBase + address, length );
	}
	else
	{
		StopAudio();
	}
}

EProcessResult AudioPluginOSX::ProcessAList()
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

void AudioPluginOSX::AddBuffer(void * ptr, u32 length)
{
	if (length == 0)
		return;

	if (mAudioThread == kInvalidThreadHandle)
		StartAudio();

	u32 num_samples = length / sizeof( Sample );

	mAudioBuffer.AddSamples( reinterpret_cast<const Sample *>(ptr), num_samples, mFrequency, kOutputFrequency );

	u32 remaining_samples = mAudioBuffer.GetNumBufferedSamples();
	mBufferLenMs = (1000 * remaining_samples) / kOutputFrequency;
	float ms = (float)num_samples * 1000.f / (float)mFrequency;
	DPF_AUDIO("Queuing %d samples @%dHz - %.2fms - bufferlen now %d\n",
		num_samples, mFrequency, ms, mBufferLenMs);
}

void AudioPluginOSX::AudioCallback(void * arg, AudioQueueRef queue, AudioQueueBufferRef buffer)
{

	AudioPluginOSX * plugin = static_cast<AudioPluginOSX *>(arg);

	u32 num_samples     = buffer->mAudioDataBytesCapacity / sizeof(Sample);
	u32 samples_written = plugin->mAudioBuffer.Drain(static_cast<Sample *>(buffer->mAudioData), num_samples);

	u32 remaining_samples = plugin->mAudioBuffer.GetNumBufferedSamples();
	plugin->mBufferLenMs = (1000 * remaining_samples) / kOutputFrequency;

	float ms = (float)samples_written * 1000.f / (float)kOutputFrequency;
	DPF_AUDIO("Playing %d samples @%dHz - %.2fms - bufferlen now %d\n",
			samples_written, kOutputFrequency, ms, plugin->mBufferLenMs);

	if (samples_written == 0)
	{
		// Would be nice to sleep here until we have something to play,
		// but AudioQueue doesn't seem to like that.
		// Leave the buffer untouched, and requeue for now.
		DPF_AUDIO("********************* Audio buffer is empty ***********************\n");
	}
	else
	{
		buffer->mAudioDataByteSize = samples_written * sizeof(Sample);
	}

	AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);

	if (!plugin->mKeepRunning)
	{
		CFRunLoopStop(CFRunLoopGetCurrent());
	}

}

u32 AudioPluginOSX::AudioThread(void * arg)
{
	AudioPluginOSX * plugin = static_cast<AudioPluginOSX *>(arg);

	AudioStreamBasicDescription format;

	format.mSampleRate       = kOutputFrequency;
	format.mFormatID         = kAudioFormatLinearPCM;
	format.mFormatFlags      = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
	format.mBitsPerChannel   = 8 * sizeof(s16);
	format.mChannelsPerFrame = kNumChannels;
	format.mBytesPerFrame    = sizeof(s16) * kNumChannels;
	format.mFramesPerPacket  = 1;
	format.mBytesPerPacket   = format.mBytesPerFrame * format.mFramesPerPacket;
	format.mReserved         = 0;

	AudioQueueRef			queue;
	AudioQueueBufferRef		buffers[kNumBuffers];
	AudioQueueNewOutput(&format, &AudioCallback, plugin, CFRunLoopGetCurrent(), kCFRunLoopCommonModes, 0, &queue);

	for (u32 i = 0; i < kNumBuffers; ++i)
	{
		AudioQueueAllocateBuffer(queue, kAudioQueueBufferLength, &buffers[i]);

		buffers[i]->mAudioDataByteSize = kAudioQueueBufferLength;

		AudioCallback(plugin, queue, buffers[i]);
	}

	AudioQueueStart(queue, NULL);

	CFRunLoopRun();

	AudioQueueStop(queue, false);
	AudioQueueDispose(queue, false);

	for (u32 i = 0; i < kNumBuffers; ++i)
	{
		AudioQueueFreeBuffer(queue, buffers[i]);
		buffers[i] = NULL;
	}

	return 0;
}

void AudioPluginOSX::AudioSyncFunction(void * arg)
{
	AudioPluginOSX * plugin = static_cast<AudioPluginOSX *>(arg);
#if DEBUG_AUDIO
	static u64 last_time = 0;
	u64 now;
	NTiming::GetPreciseTime(&now);
	if (last_time == 0) last_time = now;
	DPF_AUDIO("VBL: %dms elapsed. Audio buffer len %dms\n", (s32)NTiming::ToMilliseconds(now-last_time), plugin->mBufferLenMs);
	last_time = now;
#endif

	u32 buffer_len = plugin->mBufferLenMs;	// NB: copy this volatile to a local var so that we have a consistent view for the remainder of this function.
	if (buffer_len > kMaxBufferLengthMs)
	{
		ThreadSleepMs(buffer_len - kMaxBufferLengthMs);
	}
}

void AudioPluginOSX::StartAudio()
{
	if (mAudioThread != kInvalidThreadHandle)
		return;

	// Install the sync function.
	FramerateLimiter_SetAuxillarySyncFunction(&AudioSyncFunction, this);

	mKeepRunning = true;

	mAudioThread = CreateThread("Audio", &AudioThread, this);
	if (mAudioThread == kInvalidThreadHandle)
	{
		DBGConsole_Msg(0, "Failed to start the audio thread!");
		mKeepRunning = false;
		FramerateLimiter_SetAuxillarySyncFunction(NULL, NULL);
	}
}

void AudioPluginOSX::StopAudio()
{
	if (mAudioThread == kInvalidThreadHandle)
		return;

	// Tell the thread to stop running.
	mKeepRunning = false;

	if (mAudioThread != kInvalidThreadHandle)
	{
		JoinThread(mAudioThread, -1);
		mAudioThread = kInvalidThreadHandle;
	}

	// Remove the sync function.
	FramerateLimiter_SetAuxillarySyncFunction(NULL, NULL);
}

CAudioPlugin * CreateAudioPlugin()
{
	return new AudioPluginOSX();
}
