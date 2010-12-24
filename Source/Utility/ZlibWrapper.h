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

#ifndef ZLIBWRAPPER_H__
#define ZLIBWRAPPER_H__

namespace Zlib
{
	typedef void *	gzFile;

	gzFile	DAEDALUS_ZLIB_CALL_TYPE gzopen( const char * filename, const char * mode );
	void	DAEDALUS_ZLIB_CALL_TYPE gzclose( gzFile fh );

	u32		DAEDALUS_ZLIB_CALL_TYPE gzwrite( gzFile fh, const void * buffer, u32 length );
	u32		DAEDALUS_ZLIB_CALL_TYPE gzread( gzFile fh, void * buffer, u32 length );

	//
	//	A buffered output stream
	//
	class COutStream
	{
		public:
			COutStream( const char * filename );
			~COutStream();
			
			bool					IsOpen() const;
			bool					Flush();
			bool					WriteData( const void * data, u32 length );
			void					Reset();

		private:
			static const u32		BUFFER_SIZE = 4096;
			u8						mBuffer[ BUFFER_SIZE ];
			u32						mBufferCount;
			gzFile const			mFile;
	};

	class CInStream
	{
		public:
			CInStream( const char * filename );
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
			gzFile const			mFile;
	};

}


#endif
