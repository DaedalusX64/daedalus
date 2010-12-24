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

#ifndef _AUDIOCODE_DOT_H_
#define _AUDIOCODE_DOT_H_

//*****************************************************************************
//
//*****************************************************************************
struct Sample;
class CAudioBuffer;

//*****************************************************************************
//
//*****************************************************************************
class AudioCode
{
	public:
		AudioCode();
		~AudioCode();

		// Buffer Functions for the Audio Code
		void				SetFrequency( u32 frequency );			// Sets the Nintendo64 Game Audio Frequency
		u32					AddBuffer( u8 * start, u32 length );	// Uploads a new buffer and returns status

		//void				SetAdaptFrequency( bool adapt );

		// Management functions
		void				StopAudio( );						// Stops the Audio PlayBack (as if paused)
		void				StartAudio( );						// Starts the Audio PlayBack (as if unpaused)

		u32					GetReadStatus( );					// Returns the status on the read pointer

	public:
		void				FillBuffer( Sample * buffer, u32 num_samples );

		/*void				DodgeBufferUnderflow();
		void				DodgeBufferOverflow();*/

	public:
		CAudioBuffer *		mAudioBufferUncached;

	private:
		CAudioBuffer *		mAudioBuffer;
		bool				mAudioPlaying;
		bool				mExitAudioThread;
		u32					mFrequency;

		u32					mOutputFrequency;
		//bool				mAdaptFrequency;

		volatile u32		mBufferLength;
};

#endif
