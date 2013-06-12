/*
Copyright (C) 2003 Azimer
Copyright (C) 2008,2009 Howard0Su

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
#include "Plugins/AudioPlugin.h"

#include <mmsystem.h>
#include <dsound.h>

#include "Config/ConfigOptions.h"
#include "Core/CPU.h"
#include "Core/Interrupt.h"
#include "Core/Memory.h"
#include "Core/ROM.h"
#include "Core/RSP_HLE.h"
#include "HLEAudio/audiohle.h"
#include "Utility/FastMemcpy.h"
#include "Utility/Thread.h"

//This is disabled, it doesn't work well, causes random deadlocks/Lock failures :(
//Would be nice to get it working correctly, since running audio in the main thread is abit jerky
//#define AUDIO_THREADED

#define NUMCAPTUREEVENTS	3
#define BufferSize			0x3000

#define Buffer_Empty		0
#define Buffer_Playing		1
#define Buffer_HalfFull		2
#define Buffer_Full			3

inline void Soundmemcpy(void * dest, const void * src, size_t count)
{
	memcpy_byteswap(dest, src, count);
}


class CAudioPluginW32 : public CAudioPlugin
{
private:
	CAudioPluginW32();
public:
	static CAudioPluginW32 *		Create();


	virtual ~CAudioPluginW32();
	virtual bool			StartEmulation();
	virtual void			StopEmulation();

	virtual void			DacrateChanged( int SystemType );
	virtual void			LenChanged();
	virtual u32				ReadLength();
	virtual EProcessResult	ProcessAList();
	virtual void			Update( bool wait );
private:
	u32 Frequency, Dacrate, Snd1Len, SpaceLeft, SndBuffer[3], Playing;
	u8 *Snd1ReadPos;
	bool AIReady;
#ifdef AUDIO_THREADED
	static u32 __stdcall AudioThread(void * arg);
#endif
	LPDIRECTSOUNDBUFFER  lpdsbuf;
	LPDIRECTSOUND        lpds;
	LPDIRECTSOUNDNOTIFY  lpdsNotify;
	HANDLE               rghEvent[NUMCAPTUREEVENTS+1];
	DSBPOSITIONNOTIFY    rgdscbpn[NUMCAPTUREEVENTS+1];

	void SetupDSoundBuffers();
	bool CAudioPluginW32::FillBufferWithSilence( LPDIRECTSOUNDBUFFER lpDsb );

	void FillSectionWithSilence( int buffer );
	void FillBuffer            ( int buffer );
};
//*****************************************************************************
//
//*****************************************************************************
EAudioPluginMode gAudioPluginEnabled( APM_ENABLED_SYNC );

//*****************************************************************************
//
//*****************************************************************************
CAudioPluginW32::CAudioPluginW32()
:	Dacrate(0)
,	AIReady(false)
,	lpds(NULL)
,	lpdsbuf(NULL)
,	lpdsNotify(NULL)
{
}

//*****************************************************************************
//
//*****************************************************************************
CAudioPluginW32::~CAudioPluginW32()
{
}

//*****************************************************************************
//
//*****************************************************************************
CAudioPluginW32 *	CAudioPluginW32::Create()
{
	return new CAudioPluginW32();
}

//*****************************************************************************
//
//*****************************************************************************
bool		CAudioPluginW32::StartEmulation()
{
	HRESULT hr;
	int count;

	if ( FAILED( hr = DirectSoundCreate( NULL, &lpds, NULL ) ) ) {
		return false;
	}

	if ( FAILED( hr = IDirectSound8_SetCooperativeLevel(lpds, GetConsoleWindow(), DSSCL_PRIORITY   ))) {
		return false;
	}
	for ( count = 0; count < NUMCAPTUREEVENTS; count++ ) {
		rghEvent[count] = CreateEvent( NULL, false, false, NULL );
		if (rghEvent[count] == NULL ) { return false; }
	}
	Dacrate = 0;
	Playing = false;
	AIReady = false;
	SndBuffer[0] = Buffer_Empty;
	SndBuffer[1] = Buffer_Empty;
	SndBuffer[2] = Buffer_Empty;
#ifdef AUDIO_THREADED
	if (CreateThread( "Audio", AudioThread, this ) == kInvalidThreadHandle)
	{
		DAEDALUS_ERROR("Failed to start the audio thread!");
		gAudioPluginEnabled = APM_DISABLED;
	}
#endif
	return true;

}

//*****************************************************************************
//
//*****************************************************************************
void	CAudioPluginW32::StopEmulation()
{
	Audio_Reset();

#ifndef AUDIO_THREADED
	//ToDo: Terminate thread
#endif

	if (lpdsbuf)
	{
		IDirectSoundBuffer8_Stop(lpdsbuf);
		IDirectSoundBuffer8_Release(lpdsbuf);
		lpdsbuf = NULL;
	}
	if ( lpds )
	{
		IDirectSound8_Release(lpds);
		lpds = NULL;
	}
}
#ifdef AUDIO_THREADED
u32	DAEDALUS_THREAD_CALL_TYPE CAudioPluginW32::AudioThread(void * arg)
{
	CAudioPluginW32 * plugin = static_cast<CAudioPluginW32 *>(arg);
	while(1)
	{
		plugin->Update(true);
		ThreadSleepMs(1);
	}

	return 0;
}
#endif

//*****************************************************************************
//
//*****************************************************************************
void	CAudioPluginW32::DacrateChanged( int SystemType )
{
	if (Dacrate != Memory_AI_GetRegister(AI_DACRATE_REG))
	{
		u32 type = (SystemType == ST_NTSC) ? VI_NTSC_CLOCK : VI_PAL_CLOCK;
		Dacrate = Memory_AI_GetRegister(AI_DACRATE_REG);
		Frequency = type / (Dacrate + 1);
		SetupDSoundBuffers();
	}
}

//*****************************************************************************
//
//*****************************************************************************
void	CAudioPluginW32::LenChanged()
{
	if( gAudioPluginEnabled == APM_DISABLED )
		return;

	u32 len, count;

	len = Memory_AI_GetRegister(AI_LEN_REG);
	if (len == 0) { return; }
	Memory_AI_SetRegisterBits(AI_STATUS_REG, AI_STATUS_FIFO_FULL);
	Snd1Len = (len & 0x3FFF8);
	Snd1ReadPos = g_pu8RamBase + (Memory_AI_GetRegister(AI_DRAM_ADDR_REG) & 0x00FFFFF8);
	AIReady = true;

	int offset = 0;
	if (Playing) {
		for (count = 0; count < 3; count ++) {
			if (SndBuffer[count] == Buffer_Playing) {
				offset = (count + 1) & 3;
			}
		}
	} else {
		offset = 0;
	}

	for (count = 0; count < 3; count ++) {
		if (SndBuffer[(count + offset) & 3] == Buffer_HalfFull) {
			FillBuffer((count + offset) & 3);
			count = 3;
		}
	}
	for (count = 0; count < 3; count ++) {
		if (SndBuffer[(count + offset) & 3] == Buffer_Full) {
			FillBuffer((count + offset + 1) & 3);
			FillBuffer((count + offset + 2) & 3);
			count = 20;
		}
	}
	if (count < 10) {
		FillBuffer((0 + offset) & 3);
		FillBuffer((1 + offset) & 3);
		FillBuffer((2 + offset) & 3);
	}
#ifndef AUDIO_THREADED
	Update(false);
#endif
}

//*****************************************************************************
//
//*****************************************************************************
u32		CAudioPluginW32::ReadLength()
{
	return Snd1Len;
}


//*****************************************************************************
//
//*****************************************************************************
void	CAudioPluginW32::Update( bool Wait )
{
	u32 status, count, dwEvt;

	// Bug fix DK64
	if(!AIReady)
		return;

	if (Wait&&Playing) {
		dwEvt = MsgWaitForMultipleObjects(NUMCAPTUREEVENTS,rghEvent,false,
			INFINITE,QS_ALLINPUT);
	} else {
		dwEvt = MsgWaitForMultipleObjects(NUMCAPTUREEVENTS,rghEvent,false,
			0,QS_ALLINPUT);
	}
	dwEvt -= WAIT_OBJECT_0;
	if (dwEvt == NUMCAPTUREEVENTS) {
		return;
	}

	switch (dwEvt)
	{
	case 0:
		SndBuffer[0] = Buffer_Empty;
		FillSectionWithSilence(0);
		SndBuffer[1] = Buffer_Playing;
		FillBuffer(2);
		FillBuffer(0);
		break;
	case 1:
		SndBuffer[1] = Buffer_Empty;
		FillSectionWithSilence(1);
		SndBuffer[2] = Buffer_Playing;
		FillBuffer(0);
		FillBuffer(1);
		break;
	case 2:
		SndBuffer[2] = Buffer_Empty;
		FillSectionWithSilence(2);
		SndBuffer[0] = Buffer_Playing;
		FillBuffer(1);
		FillBuffer(2);
		//IDirectSoundBuffer_Play(lpdsbuf, 0, 0, 0 );
		break;
	}

	if (!Playing)
	{
		for (count = 0; count < 3; count ++)
		{
			if (SndBuffer[count] == Buffer_Full)
			{
				Playing = true;
				IDirectSoundBuffer8_Play(lpdsbuf, 0, 0, DSBPLAY_LOOPING );
				return;
			}
		}
	}
	else
	{
		IDirectSoundBuffer8_GetStatus(lpdsbuf,(LPDWORD)&status);
		if ((status & DSBSTATUS_PLAYING) == 0)
		{
			IDirectSoundBuffer8_Play(lpdsbuf, 0, 0, DSBPLAY_LOOPING );
		}
	}

	// Speed Sync
	if (Playing && (SndBuffer[2] == Buffer_Full) ||(SndBuffer[1] == Buffer_Full) ||(SndBuffer[0] == Buffer_Full))
	{
		ThreadSleepMs(10);
	}
}

EProcessResult	CAudioPluginW32::ProcessAList()
{
	Memory_SP_SetRegisterBits(SP_STATUS_REG, SP_STATUS_HALT);

	EProcessResult	result( PR_NOT_STARTED );

	switch( gAudioPluginEnabled )
	{
	case APM_DISABLED:
		result = PR_COMPLETED;
		break;

	case APM_ENABLED_SYNC:
		Audio_Ucode();
		result = PR_COMPLETED;
		break;
	}

	return result;
}

//*****************************************************************************
//
//*****************************************************************************
CAudioPlugin *		CreateAudioPlugin()
{
	return CAudioPluginW32::Create();
}

void CAudioPluginW32::SetupDSoundBuffers(void) {
	LPDIRECTSOUNDBUFFER lpdsb;
	DSBUFFERDESC        dsPrimaryBuff, dsbdesc;
	WAVEFORMATEX        wfm;
	HRESULT             hr;

	if (lpdsbuf) { IDirectSoundBuffer8_Release(lpdsbuf); }

	memset( &dsPrimaryBuff, 0, sizeof( DSBUFFERDESC ) );
	dsPrimaryBuff.dwSize        = sizeof( DSBUFFERDESC );
	dsPrimaryBuff.dwFlags       = DSBCAPS_PRIMARYBUFFER;
	dsPrimaryBuff.dwBufferBytes = 0;
	dsPrimaryBuff.lpwfxFormat   = NULL;

	memset( &wfm, 0, sizeof( WAVEFORMATEX ) );
	wfm.wFormatTag = WAVE_FORMAT_PCM;
	wfm.nChannels = 2;
	wfm.nSamplesPerSec = 44100;
	wfm.wBitsPerSample = 16;
	wfm.nBlockAlign = wfm.wBitsPerSample / 8 * wfm.nChannels;
	wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;

	hr = IDirectSound8_CreateSoundBuffer(lpds,&dsPrimaryBuff, &lpdsb, NULL);

	if (SUCCEEDED ( hr ) ) {
		IDirectSoundBuffer8_SetFormat(lpdsb, &wfm );
		IDirectSoundBuffer8_Play(lpdsb, 0, 0, DSBPLAY_LOOPING );
	}

	wfm.nSamplesPerSec = Frequency;
	wfm.wBitsPerSample = 16;
	wfm.nBlockAlign = wfm.wBitsPerSample / 8 * wfm.nChannels;
	wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;

	memset( &dsbdesc, 0, sizeof( DSBUFFERDESC ) );
	dsbdesc.dwSize = sizeof( DSBUFFERDESC );
	dsbdesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY;
	dsbdesc.dwBufferBytes = BufferSize * 3;
	dsbdesc.lpwfxFormat = &wfm;

	if ( FAILED( hr = IDirectSound8_CreateSoundBuffer(lpds, &dsbdesc, &lpdsbuf, NULL ) ) ) {
		DAEDALUS_ERROR("Failed in creation of Play buffer 1");
	}
	FillBufferWithSilence( lpdsbuf );

	rgdscbpn[0].dwOffset = ( BufferSize ) - 1;
	rgdscbpn[0].hEventNotify = rghEvent[0];
	rgdscbpn[1].dwOffset = ( BufferSize * 2) - 1;
	rgdscbpn[1].hEventNotify = rghEvent[1];
	rgdscbpn[2].dwOffset = ( BufferSize * 3) - 1;
	rgdscbpn[2].hEventNotify = rghEvent[2];
	rgdscbpn[3].dwOffset = DSBPN_OFFSETSTOP;
	rgdscbpn[3].hEventNotify = rghEvent[3];

	if ( FAILED( hr = IDirectSound8_QueryInterface(lpdsbuf, IID_IDirectSoundNotify, ( VOID ** )&lpdsNotify ) ) ) {
		return;
	}

	// Set capture buffer notifications.
	if ( FAILED( hr = IDirectSoundNotify_SetNotificationPositions(lpdsNotify, NUMCAPTUREEVENTS, rgdscbpn ) ) ) {
		return;
	}

	IDirectSoundBuffer_Play(lpdsbuf, 0, 0, 0 );
}

bool CAudioPluginW32::FillBufferWithSilence( LPDIRECTSOUNDBUFFER lpDsb ) {
	WAVEFORMATEX    wfx;
	u32           dwSizeWritten;

	u8*   pb1;
	u32   cb1;

	if ( FAILED( IDirectSoundBuffer8_GetFormat(lpDsb, &wfx, sizeof( WAVEFORMATEX ), (LPDWORD)&dwSizeWritten ) ) ) {
		return false;
	}

	if ( SUCCEEDED( IDirectSoundBuffer8_Lock(lpDsb,0,0,(LPVOID*)&pb1,(LPDWORD)&cb1,NULL,NULL,DSBLOCK_ENTIREBUFFER))) {
		FillMemory( pb1, cb1, ( wfx.wBitsPerSample == 8 ) ? 128 : 0 );

		IDirectSoundBuffer8_Unlock(lpDsb, pb1, cb1, NULL, 0 );
		return TRUE;
	}

	return false;
}

void CAudioPluginW32::FillSectionWithSilence( int buffer ) {
	u32 dwBytesLocked;
	VOID *lpvData;

	if (FAILED( IDirectSoundBuffer8_Lock(lpdsbuf, BufferSize * buffer,BufferSize, &lpvData, (LPDWORD)&dwBytesLocked,
		NULL, NULL, 0  ) ) )
	{
		IDirectSoundBuffer8_Unlock(lpdsbuf, lpvData, dwBytesLocked, NULL, 0 );
		DAEDALUS_ERROR("FAILED lock");
		return;
	}
	FillMemory( lpvData, dwBytesLocked, 0 );
	IDirectSoundBuffer8_Unlock(lpdsbuf, lpvData, dwBytesLocked, NULL, 0 );
}

void CAudioPluginW32::FillBuffer ( int buffer ) {
	u32 dwBytesLocked;
	VOID *lpvData;

	if (Snd1Len == 0)
	{
		//Memory_AI_ClrRegisterBits(AI_STATUS_REG, AI_STATUS_FIFO_FULL);
		//Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_AI);
		//R4300_Interrupt_UpdateCause3();
		return;
	}

	if (SndBuffer[buffer] == Buffer_Empty)
	{
		if (Snd1Len >= BufferSize)
		{
			if (FAILED( IDirectSoundBuffer8_Lock(lpdsbuf, BufferSize * buffer,BufferSize, &lpvData, (LPDWORD)&dwBytesLocked,
				NULL, NULL, 0  ) ) )
			{
				IDirectSoundBuffer8_Unlock(lpdsbuf, lpvData, dwBytesLocked, NULL, 0 );
				DAEDALUS_ERROR("FAILED lock");
				return;
			}
			Soundmemcpy(lpvData,Snd1ReadPos,dwBytesLocked);
			SndBuffer[buffer] = Buffer_Full;
			Snd1ReadPos += dwBytesLocked;
			Snd1Len -= dwBytesLocked;
			IDirectSoundBuffer8_Unlock(lpdsbuf, lpvData, dwBytesLocked, NULL, 0 );
		}
		else
		{
			if (FAILED( IDirectSoundBuffer8_Lock(lpdsbuf, BufferSize * buffer,Snd1Len, &lpvData, (LPDWORD)&dwBytesLocked,
				NULL, NULL, 0  ) ) )
			{
				IDirectSoundBuffer8_Unlock(lpdsbuf, lpvData, dwBytesLocked, NULL, 0 );
				DAEDALUS_ERROR("FAILED lock");
				return;
			}
			Soundmemcpy(lpvData,Snd1ReadPos,dwBytesLocked);
			SndBuffer[buffer] = Buffer_HalfFull;
			Snd1ReadPos += dwBytesLocked;
			SpaceLeft = BufferSize - Snd1Len;
			Snd1Len = 0;
			IDirectSoundBuffer8_Unlock(lpdsbuf, lpvData, dwBytesLocked, NULL, 0 );
		}
	} else if (SndBuffer[buffer] == Buffer_HalfFull)
	{
		if (Snd1Len >= SpaceLeft)
		{
			if (FAILED( IDirectSoundBuffer8_Lock(lpdsbuf, (BufferSize * (buffer + 1)) - SpaceLeft ,SpaceLeft, &lpvData,
				(LPDWORD)&dwBytesLocked, NULL, NULL, 0  ) ) )
			{
				IDirectSoundBuffer8_Unlock(lpdsbuf, lpvData, dwBytesLocked, NULL, 0 );
				DAEDALUS_ERROR("FAILED lock");
				return;
			}
			Soundmemcpy(lpvData,Snd1ReadPos,dwBytesLocked);
			SndBuffer[buffer] = Buffer_Full;
			Snd1ReadPos += dwBytesLocked;
			Snd1Len -= dwBytesLocked;
			IDirectSoundBuffer8_Unlock(lpdsbuf, lpvData, dwBytesLocked, NULL, 0 );
		}
		else
		{
			if (FAILED( IDirectSoundBuffer8_Lock(lpdsbuf, (BufferSize * (buffer + 1)) - SpaceLeft,Snd1Len, &lpvData, (LPDWORD)&dwBytesLocked,
				NULL, NULL, 0  ) ) )
			{
				IDirectSoundBuffer8_Unlock(lpdsbuf, lpvData, dwBytesLocked, NULL, 0 );
				DAEDALUS_ERROR("FAILED lock");
				return;
			}
			Soundmemcpy(lpvData,Snd1ReadPos,dwBytesLocked);
			SndBuffer[buffer] = Buffer_HalfFull;
			Snd1ReadPos += dwBytesLocked;
			SpaceLeft = SpaceLeft - Snd1Len;
			Snd1Len = 0;
			IDirectSoundBuffer8_Unlock(lpdsbuf, lpvData, dwBytesLocked, NULL, 0 );
		}
	}
	if (Snd1Len == 0)
	{
		Memory_AI_ClrRegisterBits(AI_STATUS_REG, AI_STATUS_FIFO_FULL);
		Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_AI);
		R4300_Interrupt_UpdateCause3();
	}
}
