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

#include "stdafx.h"
#include "AudioOutput.h"

#include <stdio.h>
#include <new>

#include <3ds.h>

#include "Config/ConfigOptions.h"
#include "Debug/DBGConsole.h"
#include "HLEAudio/AudioBuffer.h"
#include "Utility/FramerateLimiter.h"
#include "Utility/Thread.h"

extern u32 gSoundSync;

static const u32 DESIRED_OUTPUT_FREQUENCY = 44100;

static const u32 CTR_BUFFER_SIZE  = 1024 * 2;
static const u32 CTR_BUFFER_COUNT = 6;
static const u32 CTR_NUM_SAMPLES  = 512;


static ndspWaveBuf waveBuf[CTR_BUFFER_COUNT];
static unsigned int waveBuf_id;

bool audioOpen = false;

static AudioOutput * ac;

CAudioBuffer *mAudioBuffer;

static void audioCallback(void *arg)
{
	(void)arg;

	u32 samples_written = 0;

	if(waveBuf[waveBuf_id].status == NDSP_WBUF_DONE)
	{
		samples_written = mAudioBuffer->Drain( reinterpret_cast< Sample * >( waveBuf[waveBuf_id].data_pcm16 ), CTR_NUM_SAMPLES );

		if(samples_written != 0)
			waveBuf[waveBuf_id].nsamples = samples_written;

		DSP_FlushDataCache(waveBuf[waveBuf_id].data_pcm16, CTR_NUM_SAMPLES << 2);
		ndspChnWaveBufAdd( 0, &waveBuf[waveBuf_id] );

		waveBuf_id = (waveBuf_id + 1) % CTR_BUFFER_COUNT;
	}
}

static void AudioInit()
{
	if (ndspInit() != 0)
		return;

	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
	ndspChnSetRate(0, DESIRED_OUTPUT_FREQUENCY);

	for(u32 i = 0; i < CTR_BUFFER_COUNT; i++)
	{
		waveBuf[i].data_vaddr = linearAlloc(CTR_NUM_SAMPLES * 4);
		waveBuf[i].nsamples = CTR_NUM_SAMPLES;
		waveBuf[i].status = 0;

		memset(waveBuf[i].data_pcm16, 0, CTR_NUM_SAMPLES * 4);

		ndspChnWaveBufAdd(0, &waveBuf[i]);
	}

	waveBuf_id = 0;

	ndspSetCallback(&audioCallback, nullptr);

	// Everything OK
	audioOpen = true;
}

static void AudioExit()
{
	// Stop stream
	ndspChnWaveBufClear(0);
	ndspExit();

	for(u32 i = 0; i < CTR_BUFFER_COUNT; i++)
	{
		linearFree((void*)waveBuf[i].data_vaddr);
	}

	audioOpen = false;
}

AudioOutput::AudioOutput()
:	mAudioPlaying( false )
,	mFrequency( 44100 )
{
	mAudioBuffer = new CAudioBuffer( CTR_BUFFER_SIZE );
}

AudioOutput::~AudioOutput( )
{
	StopAudio();
	delete mAudioBuffer;
}

void AudioOutput::SetFrequency( u32 frequency )
{
	mFrequency = frequency;
}

void AudioOutput::AddBuffer( u8 *start, u32 length )
{
	if (length == 0)
		return;

	if (!mAudioPlaying)
		StartAudio();

	u32 num_samples = length / sizeof( Sample );

	u32 output_freq = DESIRED_OUTPUT_FREQUENCY;

	/*if (gAudioRateMatch)
	{
		if (gSoundSync > DESIRED_OUTPUT_FREQUENCY * 2)	
			output_freq = DESIRED_OUTPUT_FREQUENCY * 2;	//limit upper rate
		else if (gSoundSync < DESIRED_OUTPUT_FREQUENCY)
			output_freq = DESIRED_OUTPUT_FREQUENCY;	//limit lower rate
		else
			output_freq = gSoundSync;
	}*/

	mAudioBuffer->AddSamples( reinterpret_cast< const Sample * >( start ), num_samples, mFrequency, output_freq );
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
