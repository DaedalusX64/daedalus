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

#include "Math/MathUtil.h"

#include "SysPSP/Graphics/RomMemoryManger.h"
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
/*bool ROMFileUncompressed::LoadRawData( u32 bytes_to_read, u8 ** p_p_bytes, u32 * p_buffer_size, u32 * p_rom_size, COutputStream & messages )
{
	DAEDALUS_ASSERT( mFH != NULL, "Reading data when Open failed?" );

	u8 *	p_bytes;
	u32		size_aligned;

	// If a size of 0 was specified, this indicates we should read entire file
	// XXXX Should set bytes_to_read to max( bytes_to_read, filesize )?
	if (bytes_to_read == 0)
	{
		// Now, allocate memory for rom - round up to a 4 byte boundry
		size_aligned  = AlignPow2( mRomSize, 4 );
		p_bytes		  = (u8*)CRomMemoryManager::Get()->Alloc( size_aligned );
	}
	else
	{
		// Now, allocate memory for rom - round up to a 4 byte boundry
		size_aligned  = AlignPow2( bytes_to_read, 4 );
		p_bytes		  = new u8[size_aligned];
	}

	if (p_bytes == NULL)
	{
		return false;
	}

	// Try and read in data - reset to the start of the file
	fseek( mFH, 0, SEEK_SET );

	u32 bytes_read( fread( p_bytes, 1, bytes_to_read, mFH ) );
	if (bytes_read != bytes_to_read)
	{
		delete [] p_bytes;
		return false;
	}

	// Apply the bytesswapping before returning the buffer
	CorrectSwap( p_bytes, bytes_to_read );

	*p_p_bytes = p_bytes;
	*p_buffer_size = bytes_to_read;
	*p_rom_size = mRomSize;

	return true;
}*/
bool ROMFileUncompressed::LoadRawData( u32 bytes_to_read, u8 ** p_p_bytes, u32 * p_buffer_size, u32 * p_rom_size, COutputStream & messages )
{
	DAEDALUS_ASSERT( mFH != NULL, "Reading data when Open failed?" );

	// If a size of 0 was specified, this indicates we should read entire file
	// XXXX Should set bytes_to_read to max( bytes_to_read, filesize )?
	if (bytes_to_read == 0)
	{
		bytes_to_read = mRomSize;	
		u32		size_aligneds( AlignPow2( bytes_to_read, 4 ) );

		// hack.. for some reason when we call this func (multiple times) to read the header info, causes to crash
		// size_aligned is 64 when this happens..mmm not sure why..maybe cuz is called several times in a row?? so only alloc when reading the ROM entirely
		u8 *	p_bytess( (u8*)CRomMemoryManager::Get()->Alloc( size_aligneds ) );

		// Now, allocate memory for rom - round up to a 4 byte boundry
		
		//u8 *	p_bytes( (u8*)tmp );
		if (p_bytess == NULL)
		{
			return false;
		}

		// Try and read in data - reset to the start of the file
		fseek( mFH, 0, SEEK_SET );

		u32 bytes_read( fread( p_bytess, 1, bytes_to_read, mFH ) );
		if (bytes_read != bytes_to_read)
		{
			CRomMemoryManager::Get()->Free( p_bytess );
			return false;
		}

		// Apply the bytesswapping before returning the buffer
		CorrectSwap( p_bytess, bytes_to_read );

		*p_p_bytes = p_bytess;
		*p_buffer_size = bytes_to_read;
		*p_rom_size = mRomSize;

		return true;
	}
	else
	{
		// Now, allocate memory for rom - round up to a 4 byte boundry
		u32		size_aligned( AlignPow2( bytes_to_read, 4 ) );
		u8 *	p_bytes( new u8[size_aligned] );
		if (p_bytes == NULL)
		{
			return false;
		}
		// Try and read in data - reset to the start of the file
		fseek( mFH, 0, SEEK_SET );

		u32 bytes_read( fread( p_bytes, 1, bytes_to_read, mFH ) );
		if (bytes_read != bytes_to_read)
		{
			delete [] p_bytes;
			return false;
		}

		// Apply the bytesswapping before returning the buffer
		CorrectSwap( p_bytes, bytes_to_read );

		*p_p_bytes = p_bytes;
		*p_buffer_size = bytes_to_read;
		*p_rom_size = mRomSize;

		return true;
	}

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
