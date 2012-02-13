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

#include "stdafx.h"
#include "ROMFileUncompressed.h"

#include "ROMFileMemory.h"
//*****************************************************************************
//
//*****************************************************************************
ROMFileUncompressed::ROMFileUncompressed( const char * filename )
:	ROMFile( filename )
,	mFH( NULL )
,	mRomSize( 0 )
{
}

//*****************************************************************************
//
//*****************************************************************************
ROMFileUncompressed::~ROMFileUncompressed()
{
	if( mFH != NULL )
	{
		fclose( mFH );
	}
}

//*****************************************************************************
//
//*****************************************************************************
bool ROMFileUncompressed::Open( COutputStream & messages )
{
	DAEDALUS_ASSERT( mFH == NULL, "Opening the file twice?" );

	// Open the file and read in the data
	mFH = fopen( mFilename, "rb" );
	if(mFH == NULL)
	{
		return false;
	}

	//
	//	Determine which byteswapping mode to use
	//
	u32		header;

	if( fread( &header, sizeof( u32 ), 1, mFH ) != 1 )
	{
		return false;
	}
	SetHeaderMagic( header );

	//
	//	Determine the rom size
	//
	fseek( mFH, 0, SEEK_END );
	mRomSize = ftell( mFH );
	fseek( mFH, 0, SEEK_SET );

	if (s32(mRomSize) == -1)
	{
		return false;
	}

	return true;
}

//*****************************************************************************
//
//*****************************************************************************
bool ROMFileUncompressed::LoadRawData( u32 bytes_to_read, u8 *p_bytes, COutputStream & messages )
{
	DAEDALUS_ASSERT( mFH != NULL, "Reading data when Open failed?" );

	if (p_bytes == NULL)
	{
		return false;
	}

	// Try and read in data - reset to the start of the file
	fseek( mFH, 0, SEEK_SET );

	//ToDo: Add Status bar here for loading ROM to memory.
	u32 bytes_read( fread( p_bytes, 1, bytes_to_read, mFH ) );
	if (bytes_read != bytes_to_read)
	{
		DAEDALUS_ASSERT("Bytes to read don't match bytes read!");
		return false;
	}

	// Apply the bytesswapping before returning the buffer
	CorrectSwap( p_bytes, bytes_to_read );

	return true;
}

//*****************************************************************************
//
//*****************************************************************************
bool	ROMFileUncompressed::ReadChunk( u32 offset, u8 * p_dst, u32 length )
{
	DAEDALUS_ASSERT( mFH != NULL, "Reading data when Open failed?" );

	// Try and read in data - reset to the specified offset
	fseek( mFH, offset, SEEK_SET );

	if( fread( p_dst, length, 1, mFH ) != 1 )
	{
		return false;
	}

	// Apply the bytesswapping before returning the buffer
	CorrectSwap( p_dst, length );
	return true;
}
