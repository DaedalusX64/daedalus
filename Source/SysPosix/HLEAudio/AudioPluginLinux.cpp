#include "stdafx.h"
#include "Plugins/AudioPlugin.h"
#include "Config/ConfigOptions.h"

#include <stdio.h>
#include "Core/Memory.h"
#include "Debug/DBGConsole.h"
#include "HLEAudio/AudioBuffer.h"
#include "HLEAudio/audiohle.h"
#include "Utility/FramerateLimiter.h"
#include "Utility/Thread.h"
#include "Utility/Timing.h"

EAudioPluginMode gAudioPluginEnabled = APM_DISABLED;


// How much input we try to keep buffered in the synchronisation code.
// Setting this too low and we run the risk of skipping.
// Setting this too high and we run the risk of being very laggy.
static const u32 kMaxBufferLengthMs = 30;


// AudioQueue buffer object count and length.
// Increasing either of these numbers increases the amount of buffered
// audio which can help reduce crackling (empty buffers) at the cost of lag.
static const u32 kNumBuffers = 3;
static const u32 kAudioQueueBufferLength = 1 * 1024;

class AudioPluginSDL : public CAudioPlugin
{
public:
	AudioPluginSDL();
	virtual ~AudioPluginSDL();

	virtual bool			StartEmulation();
	virtual void			StopEmulation();

	virtual void			DacrateChanged(int system_type);
	virtual void			LenChanged();
	virtual u32				ReadLength()			{ return 0; }
	virtual EProcessResult	ProcessAList();

	void					AddBuffer(void * ptr, u32 length);	// Uploads a new buffer and returns status

	void					StopAudio();						// Stops the Audio PlayBack (as if paused)
	void					StartAudio();						// Starts the Audio PlayBack (as if unpaused)

	static void AudioSyncFunction(void * arg);

private:
	//CAudioBuffer			mAudioBuffer;
	//	ThreadHandle 			mAudioThread;
	u32						mFrequency;
	volatile bool			mKeepRunning;	// Should the audio thread keep running?
	volatile u32 			mBufferLenMs;
};


AudioPluginSDL::AudioPluginSDL()
//: mAudioBuffer( kAudioBufferSize )
: mFrequency ( 44100 )
// , mAudioThread (kInvalidThreadHandle )
, mKeepRunning ( false )
, mBufferLenMs ( 0 )
{

}

AudioPluginSDL::~AudioPluginSDL()
{
	StopAudio();
}

bool AudioPluginSDL::StartEmulation()
{
	return true;
}

void AudioPluginSDL::StopEmulation()
{
	Audio_Reset();
	StopAudio();
}

void AudioPluginSDL::DacrateChanged(int system_type)
{
	u32 clock = (system_type ==ST_NTSC) ? VI_NTSC_CLOCK : VI_PAL_CLOCK;
	u32 dacrate = Memory_AI_GetRegister(AI_DACRATE_REG);
	u32 frequency = clock / (dacrate + 1);

	DBGConsole_Msg(0, "Audio Frequency: %d", frequency);
	mFrequency = frequency;

}

void AudioPluginSDL::LenChanged()
{
	if(gAudioPluginEnabled > APM_DISABLED)
	{
		u32 address = Memory_AI_GetRegister(AI_DRAM_ADDR_REG) & 0xFFFFFF;
		u32 length = Memory_AI_GetRegister(AI_LEN_REG);

	///	AddBuffer( g_pu8RamBase + address, length);
	}
	else
	{
		StopAudio();
	}
}

	EProcessResult AudioPluginSDL::ProcessAList()
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

		void AudioPluginSDL::AudioSyncFunction(void * arg)
		{
			AudioPluginSDL * plugin = static_cast<AudioPluginSDL *>(arg);

			#if DEBUG_AUDIO
			statuc u64 last_time = 0;
			u64 now;
			NTiming::GetPreciseTime(&now);
			if (last_time == 0) last_time = now;
			DPF_AUDIO("VBL: %dms,elapsed. Audio buffer len %dms\n", (s32)NTiming::ToMilliseconds(now-last_time), plugin->mBufferLenMs);
			last_time = now;
			#endif

			u32 buffer_len = plugin->mBufferLenMs;
			if (buffer_len > kMaxBufferLengthMs)
			{
				ThreadSleepMs(buffer_len - kMaxBufferLengthMs);
			}

		}


 void AudioPluginSDL::StartAudio()
 {
		FramerateLimiter_SetAuxillarySyncFunction(&AudioSyncFunction, this);

		mKeepRunning = true;
		//XXX  create audio Thread
}

void AudioPluginSDL::StopAudio()
{
		// Tell thread to stop
		mKeepRunning = false;

		// Stop audio thread

		//Disable sync
		FramerateLimiter_SetAuxillarySyncFunction(NULL,NULL);
}

		CAudioPlugin * CreateAudioPlugin()
		{
			return new AudioPluginSDL;
		}
