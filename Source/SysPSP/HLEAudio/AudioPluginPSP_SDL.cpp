
#include "Base/Types.h"
#include "HLEAudio/AudioPlugin.h"
#include "Config/ConfigOptions.h"
#include <stdio.h>
#include <SDL2/SDL.h>
#include "Config/ConfigOptions.h"
#include "Core/Memory.h"
#include "Debug/DBGConsole.h"
#include "HLEAudio/AudioBuffer.h"
#include "HLEAudio/HLEAudioInternal.h"
#include "Core/FramerateLimiter.h"
#include "System/Thread.h"
#include "System/Timing.h"

#ifdef DAEDALUS_PSP
#include "SysPSP/Utility/CacheUtil.h"


#include "SysPSP/PRX/MediaEngine/me.h"
#include "SysPSP/Utility/ModulePSP.h"

bool gLoadedMediaEnginePRX {false};

volatile me_struct *mei;

bool InitialiseMediaEngine()
{
	constexpr std::size_t size = 64;
	if( CModule::Load("Plugins/mediaengine.prx") < 0 )	return false;

	mei = (volatile struct me_struct *)std::malloc(size * sizeof(struct me_struct));
	mei = (volatile struct me_struct *)(make_uncached_ptr(mei));
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

}

#endif
EAudioPluginMode gAudioPluginEnabled( APM_DISABLED );

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

static SDL_AudioSpec	mLINUXAudioData;
SDL_AudioDeviceID audio_device;
static Uint8 *audio_pos; // global pointer to the audio buffer to be played
static Uint32 audio_len; // remaining length of the sample we have to play


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
	static void 			AudioCallback(void *userdata, Uint8 *stream, int len);
	static u32 				AudioThread(void * arg);

private:
	CAudioBuffer *			mAudioBuffer;
	CAudioBuffer * 			mAudioBufferUncached;
	u32								mFrequency;
	ThreadHandle 			mAudioThread;
	volatile bool			mKeepRunning;	// Should the audio thread keep running?
	volatile u32 			mBufferLenMs;
};

AudioPluginSDL::AudioPluginSDL()
// :	mAudioBuffer( kAudioBufferSize )
:	mFrequency( 44100 )
,	mAudioThread( kInvalidThreadHandle )
,	mKeepRunning( false )
// ,	mBufferLenMs( 0 )
{
	#ifdef DAEDALUS_PSP
	
	constexpr size_t size = 64;
	void * mem = std::malloc( sizeof(size * sizeof(CAudioBuffer) ) );
	 mAudioBuffer = new( mem ) CAudioBuffer( kAudioBufferSize );
	mAudioBufferUncached = (CAudioBuffer*)make_uncached_ptr(mem);
	dcache_wbinv_range_unaligned( mAudioBuffer, mAudioBuffer+sizeof( CAudioBuffer ) );
	InitialiseMediaEngine();
	#endif
}

AudioPluginSDL::~AudioPluginSDL()
{
	StopAudio();
	#ifdef DAEDALUS_PSP
		// mAudioBuffer->~CAudioBuffer();
  		// free(mAudioBuffer);
  #endif
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

	DBGConsole_Msg(0, "Audio frequency: %d", frequency);
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
	else
	{
		//This is disabled since it shutsdown SDL
		//StopAudio();
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
		#ifdef DAEDALUS_PSP
			sceKernelDcacheWritebackInvalidateAll();
				if(BeginME( mei, (int)&Audio_Ucode, (int)NULL, -1, NULL, -1, NULL) < 0){
						Audio_Ucode();
						result = PR_COMPLETED;
						break;
						#else
			DAEDALUS_ERROR("Async audio is unimplemented");
			Audio_Ucode();
			result = PR_COMPLETED;
			break;
			#endif
		case APM_ENABLED_SYNC:
			Audio_Ucode();
			result = PR_COMPLETED;
			break;
	}

}
	return result;
}

void AudioPluginSDL::AddBuffer(void * ptr, u32 length)
{
	if (length == 0)
		return;

	if (mAudioThread == kInvalidThreadHandle)
		StartAudio();

	u32 num_samples = length / sizeof( Sample );

	switch (gAudioPluginEnabled)
	{
		case APM_DISABLED:
		{
			
		}
		break;
		case APM_ENABLED_ASYNC:
		{
				mAudioBufferUncached->AddSamples( reinterpret_cast< const Sample * >( ptr ), num_samples, mFrequency, kOutputFrequency );
			
		}
			break;
		case APM_ENABLED_SYNC:
		{
		mAudioBuffer->AddSamples( reinterpret_cast<const Sample *>(ptr), num_samples, mFrequency, kOutputFrequency );
	
		}
		break;
	}
//	mAudioBuffer.AddSamples( reinterpret_cast<const Sample *>(ptr), num_samples, mFrequency, kOutputFrequency );

	// u32 remaining_samples = mAudioBuffer.GetNumBufferedSamples();
	// mBufferLenMs = (1000 * remaining_samples) / kOutputFrequency;
	// float ms = (float)num_samples * 1000.f / (float)mFrequency;
	// DPF_AUDIO("Queuing %d samples @%dHz - %.2fms - bufferlen now %d\n",
	// 	num_samples, mFrequency, ms, mBufferLenMs);
}

void AudioPluginSDL::AudioCallback(void *userdata, Uint8 *stream, int len)
{
	AudioPluginSDL* plugin = static_cast<	AudioPluginSDL*>(userdata);

	if(len <= 0){
		return;
	}

	plugin->mAudioBuffer->Drain( reinterpret_cast< Sample * >( stream ), 4096);

}

u32 AudioPluginSDL::AudioThread(void * arg)
{
	AudioPluginSDL * plugin = static_cast<	AudioPluginSDL *>(arg);

	// opening an audio device:
	SDL_AudioSpec audio_spec;
	SDL_zero(audio_spec);
	audio_spec.freq = 44100;
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

void AudioPluginSDL::AudioSyncFunction(void * arg)
{
	AudioPluginSDL * plugin = static_cast<AudioPluginSDL *>(arg);
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

void AudioPluginSDL::StartAudio()
{
	if (mAudioThread != kInvalidThreadHandle)
		return;

	mKeepRunning = true;

	mAudioThread = CreateThread("Audio", &AudioThread, this);
	if (mAudioThread == kInvalidThreadHandle)
	{
		DBGConsole_Msg(0, "Failed to start the audio thread!");
		mKeepRunning = false;
		FramerateLimiter_SetAuxillarySyncFunction(NULL, NULL);
	}

}

void AudioPluginSDL::StopAudio()
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

	SDL_CloseAudioDevice(audio_device);

}

std::unique_ptr<CAudioPlugin> CreateAudioPlugin()
{
	return std::make_unique<AudioPluginSDL>();
}