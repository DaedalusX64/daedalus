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
#include "HLEAudio/AudioCode.h"
#include "HLEAudio/audiohle.h"

#include "SysPSP/Utility/JobManager.h"

#include "Core/Interrupt.h"
#include "Core/Memory.h"
#include "Core/ROM.h"
#include "Core/CPU.h"
#include "Core/RSP_HLE.h"

#include "ConfigOptions.h"


/* This sets default frequency what is used if rom doesn't want to change it.
   Probably only game that needs this is Zelda: Ocarina Of Time Master Quest
   ToDo: We should try to find out why Demos' frequencies are always wrong
   They tend to rely on a default frequency, apparently, never the same one.*/

#define DEFAULT_FREQUENCY 33600	// Taken from Mupen64 : )


class CAudioPluginOSX : public CAudioPlugin
{
private:
	CAudioPluginOSX();
public:
	static CAudioPluginOSX *		Create();

	virtual ~CAudioPluginOSX();
	virtual bool			StartEmulation();
	virtual void			StopEmulation();
//ToDo: Bring this properly to PC platform...
	virtual void			AddBufferHLE(u8 *addr, u32 len);

	virtual void			DacrateChanged( ESystemType system_type );
	virtual void			LenChanged();
	virtual u32				ReadLength();
	virtual EProcessResult	ProcessAList();
	virtual void			RomClosed();

//			void			SetAdaptFrequecy( bool adapt );

private:
	AudioCode *			mAudioCode;
};


EAudioPluginMode gAudioPluginEnabled( APM_DISABLED );
//bool gAdaptFrequency( false );

CAudioPluginOSX::CAudioPluginOSX()
:	mAudioCode( new AudioCode )
{
	//mAudioCode->SetAdaptFrequency( gAdaptFrequency );
	//gAudioPluginEnabled = APM_ENABLED_SYNC; // for testing
}

CAudioPluginOSX::~CAudioPluginOSX()
{
	delete mAudioCode;
}

CAudioPluginOSX *	CAudioPluginOSX::Create()
{
	return new CAudioPluginOSX();
}

/*
void CAudioPluginOSX::SetAdaptFrequecy( bool adapt )
{
	mAudioCode->SetAdaptFrequency( adapt );
}
*/

bool CAudioPluginOSX::StartEmulation()
{
	return true;
}

void CAudioPluginOSX::StopEmulation()
{
	Audio_Reset();
	mAudioCode->StopAudio();
}

//ToDo: Port to PC side of things...
void CAudioPluginOSX::AddBufferHLE(u8 *addr, u32 len)
{
	mAudioCode->AddBuffer(addr,len);
}

void CAudioPluginOSX::DacrateChanged( ESystemType system_type )
{
		// XXX only checked once mostly when scene changes
#ifndef DAEDALUS_SILENT
		printf( "DacrateChanged( %d )\n", system_type );
#endif
		u32 dacrate = Memory_AI_GetRegister(AI_DACRATE_REG);

		u32		frequency;
		switch (system_type)
		{
			case ST_NTSC: frequency = VI_NTSC_CLOCK / (dacrate + 1); break;
			case ST_PAL:  frequency = VI_PAL_CLOCK  / (dacrate + 1); break;
			case ST_MPAL: frequency = VI_MPAL_CLOCK / (dacrate + 1); break;
			default: frequency = DEFAULT_FREQUENCY;	break;	// This shouldn't happen
		}

		DAEDALUS_ASSERT( system_type != DEFAULT_FREQUENCY || frequency != 0, "Setting unknown frequency (%d)", frequency );

		mAudioCode->SetFrequency( frequency );
}

void CAudioPluginOSX::LenChanged()
{
	if( gAudioPluginEnabled > APM_DISABLED )
	{
		//mAudioCode->SetAdaptFrequency( gAdaptFrequency );

		u32		address( Memory_AI_GetRegister(AI_DRAM_ADDR_REG) & 0xFFFFFF );
		u32		length(Memory_AI_GetRegister(AI_LEN_REG));

		u32		result( mAudioCode->AddBuffer( g_pu8RamBase + address, length ) );

		use(result);
	}
	else
	{
		mAudioCode->StopAudio();
	}
}

u32 CAudioPluginOSX::ReadLength()
{
	return 0;
}

struct SHLEStartJob : public SJob
{
	SHLEStartJob()
	{
		 InitJob = NULL;
		 DoJob = &DoHLEStartStatic;
		 FiniJob = &DoHLEFinishedStatic;
	}

	static int DoHLEStartStatic( SJob * arg )
	{
		 SHLEStartJob *  job( static_cast< SHLEStartJob * >( arg ) );
		 return job->DoHLEStart();
	}

	static int DoHLEFinishedStatic( SJob * arg )
	{
		 SHLEStartJob *  job( static_cast< SHLEStartJob * >( arg ) );
		 return job->DoHLEFinish();
	}

	int DoHLEStart()
	{
		 Audio_Ucode();
		 return 0;
	}

	int DoHLEFinish()
	{
		 CPU_AddEvent(RSP_AUDIO_INTR_CYCLES, CPU_EVENT_AUDIO);
		 return 0;
	}
};

EProcessResult CAudioPluginOSX::ProcessAList()
{
	Memory_SP_SetRegisterBits(SP_STATUS_REG, SP_STATUS_HALT);

	EProcessResult	result( PR_NOT_STARTED );

	switch( gAudioPluginEnabled )
	{
		case APM_DISABLED:
			result = PR_COMPLETED;
			break;
		case APM_ENABLED_ASYNC:
			{
				DAEDALUS_ASSERT(false, "Unimplemented");
#if 0
				SHLEStartJob	job;
				gJobManager.AddJob( &job, sizeof( job ) );
#endif
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

void CAudioPluginOSX::RomClosed()
{
	mAudioCode->StopAudio();
}

CAudioPlugin * CreateAudioPlugin()
{
	return CAudioPluginOSX::Create();
}
