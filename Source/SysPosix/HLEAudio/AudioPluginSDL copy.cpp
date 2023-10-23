#include <SDL2/SDL.h>



#include "Base/Types.h"
#include "Config/ConfigOptions.h"
#include "Core/FramerateLimiter.h"
#include "Core/Memory.h"
#include "Debug/DBGConsole.h"
#include "HLEAudio/AudioPlugin.h"
#include "HLEAudio/HLEAudioInternal.h"
#include "System/Timing.h"
#include <pthread.h>
#include <iostream>

EAudioPluginMode gAudioPluginEnabled = APM_ENABLED_ASYNC;

pthread_t Asyncthread;
int  Asyncthreadreturn;

void* Audio_UcodeEntry(void* arg) {
    Audio_Ucode();
    return nullptr;
}

SDL_AudioDeviceID audio_device;

struct Sample {
  s16 L;
  s16 R;
};

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

	static void				AudioSyncFunction(void * arg);
	static int 				AudioThread(void * arg);

private:
	u32						mFrequency;
	SDL_Thread*  			mAudioThread;
};

AudioPluginSDL::AudioPluginSDL()
:	mFrequency( 44100 )
{}

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
	u32 clock      = (system_type == ST_NTSC) ? VI_NTSC_CLOCK : VI_PAL_CLOCK;
	u32 dacrate   = Memory_AI_GetRegister(AI_DACRATE_REG);
	u32	frequency = clock / (dacrate + 1);

	mFrequency = frequency;
}

void AudioPluginSDL::LenChanged()
{
	if (gAudioPluginEnabled > APM_DISABLED)
	{
		u32	address = Memory_AI_GetRegister(AI_DRAM_ADDR_REG) & 0xFFFFFF;
		u32	length  = Memory_AI_GetRegister(AI_LEN_REG);

		AddBuffer( g_pu8RamBase + address, length );
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
			Asyncthreadreturn = pthread_create(&Asyncthread, NULL, &Audio_UcodeEntry, (void*)nullptr);
            result = PR_COMPLETED;
			break;
		case APM_ENABLED_SYNC:
			Audio_Ucode();
			result = PR_COMPLETED;
			break;
	}

	return result;
}

void AudioPluginSDL::AddBuffer(void * ptr, u32 length)
{
    if (length == 0)
        return;

    if (mAudioThread == nullptr)
        StartAudio();

    u32 num_samples = length / sizeof(Sample);

    // Queue the samples into SDL's audio buffer
    if(SDL_QueueAudio(audio_device, ptr, num_samples * sizeof(Sample)) != 0) {
        DBGConsole_Msg(0, "SDL_QueueAudio error: %s", SDL_GetError());
        return;
    }
}

int AudioPluginSDL::AudioThread(void * arg)
{
	AudioPluginSDL * plugin = static_cast<	AudioPluginSDL *>(arg);

	SDL_AudioSpec audio_spec;
	audio_spec.freq = plugin->mFrequency;
	audio_spec.format = AUDIO_S16SYS;
	audio_spec.channels = 2;
	audio_spec.samples = 4096;
	audio_spec.callback = NULL;
	audio_spec.userdata = plugin;

	audio_device = SDL_OpenAudioDevice(
			NULL, 0, &audio_spec, NULL, 0);

	SDL_QueueAudio(0, &audio_spec, 4096);

	SDL_PauseAudioDevice(audio_device, 0);

	return 0;
}

void AudioPluginSDL::StartAudio()
{

	mAudioThread = SDL_CreateThread(&AudioThread, "Audio", this);
	
	if (mAudioThread == nullptr)
	{
		DBGConsole_Msg(0, "Failed to start the audio thread!");
		FramerateLimiter_SetAuxillarySyncFunction(nullptr, nullptr);
	}
}

void AudioPluginSDL::StopAudio()
{
	if (mAudioThread == nullptr)
		return;

	if (mAudioThread != nullptr)
	{
		int threadReturnValue;
		SDL_WaitThread(mAudioThread, &threadReturnValue);
		mAudioThread = nullptr;
	}

    // Clear the remaining audio data in SDL's audio buffer
    SDL_ClearQueuedAudio(audio_device);
	SDL_CloseAudioDevice(audio_device);
}

std::unique_ptr<CAudioPlugin> CreateAudioPlugin()
{
	return std::make_unique<AudioPluginSDL>();
}