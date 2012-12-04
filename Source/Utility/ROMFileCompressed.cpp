/*
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

#include "stdafx.h"
#include "ROMFileCompressed.h"

#ifdef DAEDALUS_COMPRESSED_ROM_SUPPORT

#include "Math/MathUtil.h"

#include "Debug/DBGConsole.h"

#include "Utility/Stream.h"

//*****************************************************************************
//
//*****************************************************************************
ROMFileCompressed::ROMFileCompressed( const char * filename )
:	ROMFile( filename )
,	mZipFile( NULL )
,	mFoundRom( false )
,	mRomSize( 0 )
{
}

//*****************************************************************************
//
//*****************************************************************************
ROMFileCompressed::~ROMFileCompressed()
{
	if(mZipFile != NULL)
	{
		unzClose( mZipFile );
	}
}

//*****************************************************************************
//
//*****************************************************************************
bool ROMFileCompressed::Open( COutputStream & messages )
{
	DAEDALUS_ASSERT( mZipFile == NULL, "Opening the zipfile twice?" );

	mFoundRom = false;
	mZipFile = unzOpen(mFilename);
	if (mZipFile == NULL)
	{
		messages << "Couldn't open " << mFilename;
		return false;
	}

    s32				err;
	unz_file_info	file_info;
	char			rom_filename[MAX_PATH+1];

	err = unzGoToFirstFile(mZipFile);
	if (err != UNZ_OK)
	{
		messages << "error " << err << "with zipfile in unzGoToFirstFile";
	}
	else
	{
		do
		{
			err = unzGetCurrentFileInfo(mZipFile, &file_info,
										rom_filename, MAX_PATH,
										NULL, 0,
										NULL, 0);
			if (err != UNZ_OK)
			{
				messages << "error " << err << "with zipfile in unzGetCurrentFileInfo";
				break;
			}

			// If filesize is > 1MB, assume it's a rom
			if (file_info.uncompressed_size > 1*1024*1024)
			{
				// Open and try to find 4 byte header (0x80371240)
				err = unzOpenCurrentFile(mZipFile);
				if (err == UNZ_OK)
				{
					u32			magic;
					const u32	u32size( sizeof( u32 ) );

					u32 bytes_read( unzReadCurrentFile( mZipFile, &magic, u32size ) );
					if (bytes_read == u32size)
					{
						// Check the format:
						//DBGConsole_Msg(0, "%s %08x", rom_filename, dwVal);

						// Could be byteswapped - check all variations
						if (magic == 0x80371240 ||
							magic == 0x40123780 ||
							magic == 0x12408037)
						{
							unzCloseCurrentFile(mZipFile);
							mRomSize = file_info.uncompressed_size;
							mFoundRom = true;
							SetHeaderMagic( magic );
							break;
						}
					}
					unzCloseCurrentFile(mZipFile);
				}
			}

		} while (unzGoToNextFile(mZipFile) == UNZ_OK);
	}

	//
	// Ensure that we've opened the file and are read to start reading
	//
	if( mFoundRom )
	{
		err = unzOpenCurrentFile(mZipFile);
		if(err != UNZ_OK)
		{
			mFoundRom = false;
		}
	}

	return mFoundRom;
}

//*****************************************************************************
//
//*****************************************************************************
bool ROMFileCompressed::LoadRawData( u32 bytes_to_read, u8 *p_bytes, COutputStream & messages )
{
	DAEDALUS_ASSERT( mZipFile != NULL, "No open zipfile?" );
	DAEDALUS_ASSERT( mFoundRom, "Why are we loading data when no rom was found?" );

	if (p_bytes == NULL)
    {
        return false;
    }

	//
	//	It is assumed that the file is already openened and ready for reading here
	//
	s32 bytes_read( unzReadCurrentFile( mZipFile, p_bytes, bytes_to_read ) );
 	if( u32( bytes_read ) != bytes_to_read )
	{
		if( bytes_read < 0 )
		{
			messages << "error (" << bytes_read << ") with zipfile in unzReadCurrentFile";
		}
		else if( bytes_read == 0 )
		{
			// EOF?
		}
		else if( bytes_read < s32( bytes_to_read ) )
		{
			// Not enough bytes read
			messages << "Unable to read sufficent bytes from zip.\nRead " << bytes_read << ", wanted " << bytes_to_read;
		}

		unzCloseCurrentFile(mZipFile);
		return false;
	}

	//
	//	Close the file here to determine crc errors. No need to reopen presently
	//
	int err;
	err = unzCloseCurrentFile(mZipFile);
	if( err == UNZ_CRCERROR )
	{
		messages << "CRC Error in ZipFile";
		unzCloseCurrentFile(mZipFile);
		return false;
	}

	// Apply the bytesswapping before returning the buffer
	CorrectSwap( p_bytes, bytes_to_read );

    return true;
}

//*****************************************************************************
//	Utility function to seek to the specified location in the zipfile
//	Uses the specified buffer as a scratch pad to temporarilly decompress data.
//*****************************************************************************
bool	ROMFileCompressed::Seek( u32 offset, u8 * p_scratch_block, u32 block_size )
{
	DAEDALUS_ASSERT( mZipFile != NULL, "No open zipfile?" );

	int		err;
	u32		current_offset( unztell( mZipFile ) );

	DAEDALUS_ASSERT( (current_offset % block_size) == 0, "Performance: Trying to seek and current offset isn't a multiple of the block size" );
	DAEDALUS_ASSERT( (offset % block_size) == 0, "Performance: Trying to seek to an address which isn't the block size" );

	if(current_offset > offset)
	{
		DBGConsole_Msg( 0, "[CRomCache - seeking from %08x to %08x in zip", current_offset, offset );

		// Annoyingly, have to close and reopen the zip file
		unzCloseCurrentFile( mZipFile );		// Ignore errors here
		err = unzOpenCurrentFile( mZipFile );
		if(err != UNZ_OK)
		{
			// Why would this happen?
			return false;
		}

		current_offset = 0;
	}

	while( current_offset < offset )
	{
		u32 bytes_remaining( offset - current_offset );
		u32	bytes_to_read( Min( bytes_remaining, block_size ) );

		u32 bytes_read( unzReadCurrentFile( mZipFile, p_scratch_block, bytes_to_read ) );
		if(bytes_read != bytes_to_read)
		{
			return false;
		}

		current_offset += bytes_to_read;
	}

	DAEDALUS_ASSERT( u32( unztell( mZipFile ) ) == offset, "Failed to seek to the specified offset" );

	return true;
}

//*****************************************************************************
//
//*****************************************************************************
bool	ROMFileCompressed::ReadChunk( u32 offset, u8 * p_dst, u32 length )
{
	DAEDALUS_ASSERT( mZipFile != NULL, "No open zipfile?" );
	DAEDALUS_ASSERT( mFoundRom, "Why are we loading data when no rom was found?" );

	if( !Seek( offset, p_dst, length ) )
	{
		return false;
	}

	if( u32( unzReadCurrentFile( mZipFile, p_dst, length ) ) != length)
	{
		return false;
	}

	// Apply the bytesswapping before returning the buffer
	CorrectSwap( p_dst, length );
	return true;
}

#endif // DAEDALUS_COMPRESSED_ROM_SUPPORT
