/*
Copyright (C) 2003 Azimer
Copyright (C) 2001,2006 StrmnNrmn
Copyright (C) 2019, DaedalusX64 Team

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
#include <stdio.h>

#include <pspkernel.h>
#include <pspaudiolib.h>
#include <pspaudio.h>

#include "Plugins/AudioPlugin.h"
#include "HLEAudio/audiohle.h"

#include "Config/ConfigOptions.h"
#include "Core/CPU.h"
#include "Core/Interrupt.h"
#include "Core/Memory.h"
#include "Core/ROM.h"
#include "Core/RSP_HLE.h"
#include "Debug/DBGConsole.h"
#include "HLEAudio/AudioBuffer.h"
#include "SysPSP/Utility/JobManager.h"
#include "SysPSP/Utility/CacheUtil.h"
#include "SysPSP/Utility/JobManager.h"
#include "Utility/FramerateLimiter.h"
#include "Utility/Thread.h"

EAudioPluginMode gAudioPluginEnabled = APM_DISABLED;

#define DEBUG_AUDIO 0

#if DEBUG_AUDIOPLUGIN
#define DPF_AUDIO(...)	do { printf(__VA_ARGS__); } while(0)
#else
#define DPF_AUDIO(...)	do { (void)sizeof(__VA_ARGS__); } while(0)
#endif


static const u32 kOutputFrequency {44100};
static const u32 kAudioBufferSize {1024 * 2}; // gpSP has a 4096kb sized audio buffer maybe we can use that?
static const u32 kNumChannels {2};
static const u32 kMaxBufferLengthMs {30}; // The maximum input we keep buffered in sync code, if too low we may skip otherwise too high, lag will persist.



class AudioPluginPSP : public CAudioPlugin
{
public:
  AudioPluginPSP();
  virtual ~AudioPluginPSP();

  virtual bool StartEmulation();
  virtual void StopEmulation();
  virtual void DacrateChanged (int system_type);
  virtual void LenChanged();
  virtual u32 ReadLength() {return 0;}
  virtual EProcessResult ProcessAList();

  // Non default AudioPlugin Functions
  void					StopAudio();						// Stops the Audio PlayBack (as if paused)
  void					StartAudio();						// Starts the Audio PlayBack (as if unpaused)
  void AddBuffer( void *ptr, u32 length);
  void FillBuffer(Sample * buffer, u32 num_samples);
private:
    CAudioBuffer mAudioBuffer;
    u32 mFrequency;
    s32 mSemaphore;
};

void AudioPluginPSP::StopAudio(){}

void AudioPluginPSP::FillBuffer(Sample * buffer, u32 num_samples)
{
	sceKernelWaitSema( mSemaphore, 1, NULL );

	mAudioBuffer.Drain( buffer, num_samples );

sceKernelSignalSema( mSemaphore, 1 );
}




void audioCallback( void * buf, unsigned int length, void * userdata )
{
	AudioPluginPSP * ac( reinterpret_cast< AudioPluginPSP * >( userdata ) );

	ac->FillBuffer( reinterpret_cast< Sample * >( buf ), length );
}
void AudioPluginPSP::StartAudio()
{
  AudioPluginPSP *ac = this;

pspAudioInit();
pspAudioSetChannelCallback( 0, audioCallback, this );
}

void AudioPluginPSP::AddBuffer( void *ptr, u32 length)
{
  if (length == 0)
    return;

    u32 num_samples {length / sizeof( Sample )};

    switch (gAudioPluginEnabled)
    {
      case APM_DISABLED:
        break;

        case APM_ENABLED_ASYNC:
          break;

        case APM_ENABLED_SYNC:
          mAudioBuffer.AddSamples( reinterpret_cast<const Sample *>(ptr), num_samples, mFrequency, kOutputFrequency );
        break;
    }

}




AudioPluginPSP::AudioPluginPSP()
: mAudioBuffer (kAudioBufferSize)
, mFrequency ( 44100 )
, mSemaphore ( sceKernelCreateSema( "AudioPluginPSP", 0, 1 ,1, nullptr) )
{}


AudioPluginPSP::~AudioPluginPSP()
{
sceKernelDeleteSema(mSemaphore);
}

void AudioPluginPSP::DacrateChanged(int system_type)
{
  u32 clock { (system_type == ST_NTSC) ? VI_NTSC_CLOCK : VI_PAL_CLOCK};
  u32 dacrate {Memory_AI_GetRegister(AI_DACRATE_REG)};
  u32 frequency = clock / (dacrate + 1);

  DBGConsole_Msg(0, "Audio frequency: %d", frequency);
  mFrequency = frequency;
}

void AudioPluginPSP::LenChanged()
{
  if (gAudioPluginEnabled > APM_DISABLED)
  {
      u32 address {Memory_AI_GetRegister(AI_DRAM_ADDR_REG) & 0xFFFFFF};
      u32 length { Memory_AI_GetRegister(AI_LEN_REG)};

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
      DAEDALUS_ERROR("Async is not implemented");
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


bool AudioPluginPSP::StartEmulation()
{
  return true;
}

void AudioPluginPSP::StopEmulation()
{
  Audio_Reset();
  StopAudio();
}


CAudioPlugin *CreateAudioPlugin()
{
  return new AudioPluginPSP();
}
