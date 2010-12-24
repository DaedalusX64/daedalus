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

// This file should not be compiled using precompiled headers.
// This is a compatibility workaround for the __fastcall calling convention used in DAEDALUS_W32

#include "stdafx.h"
#include "ZlibWrapper.h"

#include <zlib.h>

#include "Math/MathUtil.h"

//
//	Although this looks pretty redundant, it allows us to get around
//	various issues relating to compiling the project with __fastcall
//	as the default calling convention.
//

namespace Zlib
{
typedef void *	gzFile;

//*****************************************************************************
//
//*****************************************************************************
gzFile	DAEDALUS_ZLIB_CALL_TYPE gzopen( const char * filename, const char * mode )
{
	return ::gzopen( filename, mode );
}

//*****************************************************************************
//
//*****************************************************************************
void	DAEDALUS_ZLIB_CALL_TYPE gzclose( gzFile fh )
{
	::gzclose( fh );
}

//*****************************************************************************
//
//*****************************************************************************
u32		DAEDALUS_ZLIB_CALL_TYPE gzwrite( gzFile fh, const void * buffer, u32 length )
{
	return ::gzwrite( fh, buffer, length );
}

//*****************************************************************************
//
//*****************************************************************************
u32		DAEDALUS_ZLIB_CALL_TYPE gzread( gzFile fh, void * buffer, u32 length )
{
	return ::gzread( fh, buffer, length );
}

//*****************************************************************************
//
//*****************************************************************************
u32		DAEDALUS_ZLIB_CALL_TYPE gzseek( gzFile fh, u32 offset, int whence)
{
	return ::gzseek( fh, offset, whence );
}

//*****************************************************************************
//
//*****************************************************************************
COutStream::COutStream( const char * filename )
:	mBufferCount( 0 )
,	mFile( Zlib::gzopen( filename, "wb" ) )
{
}

//*****************************************************************************
//
//*****************************************************************************
COutStream::~COutStream()
{
	if ( mFile != NULL )
	{
		Flush();
		Zlib::gzclose( mFile );
	}
}

//*****************************************************************************
//
//*****************************************************************************
bool	COutStream::IsOpen() const
{
	return mFile != NULL;
}

//*****************************************************************************
//
//*****************************************************************************
bool	COutStream::Flush()
{
	DAEDALUS_ASSERT( mBufferCount <= BUFFER_SIZE, "How come the buffer has overflowed?" );

	const u32	count( mBufferCount );

	mBufferCount = 0;

	return Zlib::gzwrite( mFile, mBuffer, count ) == count;
}

//*****************************************************************************
//
//*****************************************************************************
bool	COutStream::WriteData( const void * data, u32 length )
{
	if ( mFile != NULL )
	{
		const u8 *	current_ptr( reinterpret_cast< const u8 * >( data ) );
		u32			bytes_remaining( length );
		while( bytes_remaining > 0 )
		{
			u32		buffer_bytes_remaining( BUFFER_SIZE - mBufferCount );
			u32		bytes_to_process( pspFpuMin( bytes_remaining, buffer_bytes_remaining ) );

			//
			// Append as many bytes as possible
			//
			if( bytes_to_process > 0 )
			{
				memcpy( mBuffer + mBufferCount, current_ptr, bytes_to_process );

				current_ptr += bytes_to_process;
				bytes_remaining -= bytes_to_process;
				mBufferCount += bytes_to_process;
			}

			//
			// Flush the buffer if full
			//
			if( mBufferCount >= BUFFER_SIZE )
			{
				if( !Flush() )
				{
					return false;
				}
			}
		}
		return true;
	}

	return false;
}

//*****************************************************************************
//
//*****************************************************************************
void	COutStream::Reset()
{
	Flush();

	mBufferCount = 0;

	gzseek(mFile, 0, SEEK_SET);
}

//*****************************************************************************
//
//*****************************************************************************
CInStream::CInStream( const char * filename )
:	mBufferOffset( 0 )
,	mBytesAvailable( 0 )
,	mFile( Zlib::gzopen( filename, "rb" ) )
{
}

//*****************************************************************************
//
//*****************************************************************************
CInStream::~CInStream()
{
	if ( mFile != NULL )
	{
		Zlib::gzclose( mFile );
	}
}

//*****************************************************************************
//
//*****************************************************************************
bool	CInStream::IsOpen() const
{
	return mFile != NULL;
}

//*****************************************************************************
//
//*****************************************************************************
bool	CInStream::Fill()
{
	DAEDALUS_ASSERT( mBytesAvailable == 0, "How come we're refilling with a non-empty buffer?" );

	mBufferOffset = 0;

	s32	bytes_read( Zlib::gzread( mFile, mBuffer, BUFFER_SIZE ) );
	if( bytes_read > 0 )
	{
		mBytesAvailable = bytes_read;
	}
	else
	{
		// EOF? Make sure we don't interpret negative result as unsigned value.
		mBytesAvailable = 0;
	}

	return mBytesAvailable > 0;
}

//*****************************************************************************
//
//*****************************************************************************
bool	CInStream::ReadData( void * data, u32 length )
{
	if ( mFile != NULL )
	{
		u8 *		current_ptr( reinterpret_cast< u8 * >( data ) );
		u32			bytes_remaining( length );

		while( bytes_remaining > 0 )
		{
			u32		bytes_to_process( pspFpuMin( bytes_remaining, u32(mBytesAvailable) ) );

			if( bytes_to_process > 0 )
			{
				DAEDALUS_ASSERT( mBufferOffset + bytes_to_process <= u32(BUFFER_SIZE), "Reading too many bytes" );

				memcpy( current_ptr, mBuffer + mBufferOffset, bytes_to_process );

				current_ptr += bytes_to_process;
				bytes_remaining -= bytes_to_process;
				mBufferOffset += bytes_to_process;
				mBytesAvailable -= bytes_to_process;
			}

			if( mBytesAvailable == 0 )
			{
				if( !Fill() )
				{
					return false;
				}
			}
		}

		return true;
	}

	return false;
}

//*****************************************************************************
//
//*****************************************************************************
void	CInStream::Reset()
{
	mBufferOffset = 0;
	mBytesAvailable = 0;

	gzseek(mFile, 0, SEEK_SET);
}
}

