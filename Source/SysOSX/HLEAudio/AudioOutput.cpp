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
#include "AudioOutput.h"

#include <stdio.h>

#include <AudioToolbox/AudioQueue.h>
#include <CoreAudio/CoreAudioTypes.h>
#include <CoreFoundation/CFRunLoop.h>

#include "Debug/DBGConsole.h"
#include "HLEAudio/AudioBuffer.h"
#include "Utility/FramerateLimiter.h"
#include "Utility/Thread.h"
#include "Utility/Timing.h"

#define DEBUG_AUDIO  0

#if DEBUG_AUDIO
#define DPF_AUDIO(...)	do { printf(__VA_ARGS__); } while(0)
#else
#define DPF_AUDIO(...)	do {} while(0)
#endif

static const u32 kOutputFrequency = 44100;
static const u32 kAudioBufferSize = 1024 * 1024;	// Circular buffer length. Converts N64 samples out our output rate.
static const u32 kNumChannels = 2;

// AudioQueue buffer object count and length.
// Increasing either of these numbers increases the amount of buffered
// audio which can help reduce crackling (empty buffers) at the cost of lag.
static const u32 kNumBuffers = 3;
static const u32 kAudioQueueBufferLength = 1 * 1024;

static AudioQueueRef		gQueue;
static AudioQueueBufferRef	gBuffers[kNumBuffers];
static ThreadHandle 		gAudioThread = kInvalidThreadHandle;
static u32 					gBufferLenMs = 0;

static void AudioSyncFunction()
{
#if DEBUG_AUDIO
	static u64 last_time = 0;
	u64 now;
	NTiming::GetPreciseTime(&now);
	if (last_time == 0) last_time = now;
	DPF_AUDIO("VBL: %dms elapsed. Audio buffer len %dms\n", (s32)NTiming::ToMilliseconds(now-last_time), gBufferLenMs);
	last_time = now;
#endif

	const u32 kMaxBufferLengthMs = 30;
	if (gBufferLenMs > kMaxBufferLengthMs)
	{
		ThreadSleepMs(gBufferLenMs - kMaxBufferLengthMs);
	}
}

static void AudioCallback(void * user_data, AudioQueueRef queue, AudioQueueBufferRef buffer)
{
	CAudioBuffer * audio_buffer = static_cast<CAudioBuffer *>(user_data);

	u32 num_samples = buffer->mAudioDataBytesCapacity / sizeof(Sample);
	u32 samples_written = audio_buffer->Fill(static_cast<Sample *>(buffer->mAudioData), num_samples);

	u32 remaining_samples = audio_buffer->GetNumBufferedSamples();
	gBufferLenMs = (1000 * remaining_samples) / kOutputFrequency;


	float ms = (float)samples_written * 1000.f / (float)kOutputFrequency;
	DPF_AUDIO("Playing %d samples @%dHz - %.2fms - bufferlen now %d\n",
			samples_written, kOutputFrequency, ms, gBufferLenMs);

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
}

static u32 AudioThread(void * arg)
{
	CAudioBuffer * audio_buffer = static_cast<CAudioBuffer *>(arg);

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

	AudioQueueNewOutput(&format, &AudioCallback, audio_buffer, CFRunLoopGetCurrent(), kCFRunLoopCommonModes, 0, &gQueue);

	for (u32 i = 0; i < kNumBuffers; ++i)
	{
		AudioQueueAllocateBuffer(gQueue, kAudioQueueBufferLength, &gBuffers[i]);

		gBuffers[i]->mAudioDataByteSize = kAudioQueueBufferLength;

		AudioCallback(audio_buffer, gQueue, gBuffers[i]);
	}

	AudioQueueStart(gQueue, NULL);

	CFRunLoopRun();

	return 0;
}

static void AudioExit()
{
	AudioQueueStop(gQueue, false);
	AudioQueueDispose(gQueue, false);
	CFRunLoopStop(CFRunLoopGetCurrent());

	if(gAudioThread != kInvalidThreadHandle)
	{
		JoinThread(gAudioThread, -1);
	}

	for (u32 i = 0; i < kNumBuffers; ++i)
	{
		AudioQueueFreeBuffer(gQueue, gBuffers[i]);
		gBuffers[i] = NULL;
	}

}

AudioOutput::AudioOutput()
:	mAudioBufferUncached( NULL )	// FIXME(strmnrmn): remove this.
,	mAudioBuffer( new CAudioBuffer( kAudioBufferSize ) )
,	mAudioPlaying( false )
,	mFrequency( 44100 )
{
}

AudioOutput::~AudioOutput( )
{
	StopAudio();

	delete mAudioBuffer;
}

void AudioOutput::SetFrequency( u32 frequency )
{
	DBGConsole_Msg(0, "Audio frequency: %d", frequency);
	mFrequency = frequency;
}

u32 AudioOutput::AddBuffer( u8 *start, u32 length )
{
	if (length == 0)
		return 0;

	if (!mAudioPlaying)
		StartAudio();

	u32 num_samples = length / sizeof( Sample );

	mAudioBuffer->AddSamples( reinterpret_cast<const Sample *>(start), num_samples, mFrequency, kOutputFrequency );

	u32 remaining_samples = mAudioBuffer->GetNumBufferedSamples();
	gBufferLenMs = (1000 * remaining_samples) / kOutputFrequency;
	float ms = (float)num_samples * 1000.f / (float)mFrequency;
	DPF_AUDIO("Queuing %d samples @%dHz - %.2fms - bufferlen now %d\n",
		num_samples, mFrequency, ms, gBufferLenMs);

	return 0;
}

void AudioOutput::StartAudio()
{
	if (mAudioPlaying)
		return;

	// Install the sync function.
	FramerateLimiter_SetAuxillarySyncFunction(&AudioSyncFunction);

	mAudioPlaying = true;

	gAudioThread = CreateThread( "CPU", &AudioThread, mAudioBuffer );
	if (gAudioThread == kInvalidThreadHandle)
	{
		DBGConsole_Msg(0, "Failed to start the audio thread!");
	}
}

void AudioOutput::StopAudio()
{
	if (!mAudioPlaying)
		return;

	// Remove the sync function.
	FramerateLimiter_SetAuxillarySyncFunction(&AudioSyncFunction);

	mAudioPlaying = false;

	AudioExit();
}

