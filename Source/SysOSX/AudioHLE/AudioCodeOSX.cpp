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

#include "Utility/FramerateLimiter.h"
#include "HLEAudio/AudioCode.h"
#include "HLEAudio/AudioBuffer.h"

#include "Debug/DBGConsole.h"

#include "Utility/Thread.h"

#include <stdio.h>
#include <new>

#include "ConfigOptions.h"

static const u32	DESIRED_OUTPUT_FREQUENCY = 44100;
static const u32	MAX_OUTPUT_FREQUENCY = DESIRED_OUTPUT_FREQUENCY * 4;
static const u32	BUFFER_SIZE = 1024 * 8;

//*****************************************************************************
//
//*****************************************************************************
AudioCode::AudioCode()
:	mAudioBuffer( new CAudioBuffer( BUFFER_SIZE ) )
,	mAudioPlaying( false )
,	mFrequency( 44100 )
,	mOutputFrequency( DESIRED_OUTPUT_FREQUENCY )
//,	mAdaptFrequency( false )
,	mBufferLength( 0 )
{
}

AudioCode::~AudioCode( )
{
	StopAudio();

	mAudioBuffer->~CAudioBuffer();
	free( mAudioBuffer );
}

void AudioCode::SetFrequency( u32 frequency )
{
	DBGConsole_Msg( 0, "Audio frequency: %d", frequency );
	mFrequency = frequency;
}

u32 AudioCode::AddBuffer( u8 *start, u32 length )
{
	DAEDALUS_ASSERT(false, "unimplemented");
	return 0;
}

void AudioCode::StartAudio()
{
	if (mAudioPlaying)
		return;

	mAudioPlaying = true;
}

void AudioCode::StopAudio()
{
	if (!mAudioPlaying)
		return;

	mAudioPlaying = false;
}

u32 AudioCode::GetReadStatus()
{
	return 0;
}

