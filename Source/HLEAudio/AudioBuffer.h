/*
Copyright (C) 2007 StrmnNrmn

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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef AUDIOBUFFER_H__
#define AUDIOBUFFER_H__


//*****************************************************************************
//
//*****************************************************************************
struct Sample
{
	s16		L;
	s16		R;
};

//*****************************************************************************
//
//	A utility class for buffering up samples, upsampling to the desired
//	output frequency and copying them to the desired output buffer.
//
//	N.B. This class currently does no synchronisation - it's assumed that
//	the calling code will handle this
//
//*****************************************************************************
class CAudioBuffer
{
public:
	CAudioBuffer( u32 buffer_size );
	~CAudioBuffer();

	void			AddSamples( const Sample * samples, u32 num_samples, u32 frequency, u32 output_freq );
	void			Fill( Sample * samples, u32 num_samples );

	u32				GetSize() const;

private:
	Sample *		mBufferBegin;
	Sample *		mBufferEnd;

	const Sample * volatile	mReadPtr;
	Sample * volatile		mWritePtr;
};


#endif // AUDIOBUFFER_H__
