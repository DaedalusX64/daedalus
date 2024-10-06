/*
Copyright (C) 2006 StrmnNrmn

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

#ifndef UTILITY_ZLIBWRAPPER_H_
#define UTILITY_ZLIBWRAPPER_H_

#include "Utility/Paths.h"

//
//	A buffered output stream
//
class COutStream
{
	public:
		COutStream( const std::filesystem::path filename );
		~COutStream();

		bool					IsOpen() const;
		bool					Flush();
		bool					WriteData( const void * data, u32 length );
		void					Reset();

	private:
		static const u32		BUFFER_SIZE = 4096;
		u8						mBuffer[ BUFFER_SIZE ];
		u32						mBufferCount;
		void * const			mFile;
};

class CInStream
{
	public:
		CInStream( const std::filesystem::path filename );
		~CInStream();

		bool					IsOpen() const;
		bool					ReadData( void * data, u32 length );
		void					Reset();

	private:
		bool					Fill();

	private:
		static const u32		BUFFER_SIZE = 4096;
		u8						mBuffer[ BUFFER_SIZE ];
		u32						mBufferOffset;
		s32						mBytesAvailable;
		void * const			mFile;
};

#endif // UTILITY_ZLIBWRAPPER_H_
