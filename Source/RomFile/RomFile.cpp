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


#include "Base/Types.h"


#include "Debug/DBGConsole.h"

#include "RomFile/RomFile.h"
#include "RomFile/RomFileCompressed.h"
#include "RomFile/RomFileUncompressed.h"
#include "Utility/Stream.h"


#include <algorithm>
#include <string.h>
#include <filesystem>


std::shared_ptr<ROMFile> ROMFile::Create( const std::filesystem::path& filename )
{

	if (filename.extension() == ".zip")
	{
#ifdef DAEDALUS_COMPRESSED_ROM_SUPPORT
		return std::make_unique<ROMFileCompressed>( filename.c_str() );
#else
		return nullptr;
#endif
	}
	else
	{
		return std::make_unique<ROMFileUncompressed>( filename.c_str() );
	}
}

ROMFile::ROMFile( const std::filesystem::path& filename )
:	mFilename ( filename )
,	mHeaderMagic ( 0 )
{

}

ROMFile::~ROMFile() {}

bool ROMFile::LoadData( u32 bytes_to_read, u8 *p_bytes, COutputStream & messages )
{
	if( !LoadRawData( bytes_to_read, p_bytes, messages ) )
	{
		messages << "Unable to get rom info from '" << mFilename.string().c_str() << "'";
		return false;
	}

	return true;
}

bool ROMFile::RequiresSwapping() const
{
	DAEDALUS_ASSERT( mHeaderMagic != 0, "The header magic hasn't been set" );
	return mHeaderMagic != 0x80371240;
}

bool ROMFile::SetHeaderMagic( u32 magic )
{
	mHeaderMagic = magic;

	switch (mHeaderMagic)
	{
	case 0x80371240:
	case 0x40123780:
	case 0x12408037:
		break;
	default:
		DAEDALUS_ERROR( "Unhandled swapping mode: %08x - ROM format is not supported for %s", mHeaderMagic, mFilename.c_str() );
		return false;
	}
	return true;
}

void ROMFile::CorrectSwap( u8 * p_bytes, u32 length )
{
	switch (mHeaderMagic)
	{
	case 0x80371240:
		// Pre byteswapped - no need to do anything
		break;
	case 0x40123780:
		ByteSwap_3210( p_bytes, length );
		break;
	case 0x12408037:
		ByteSwap_2301( p_bytes, length );
		break;
	default:
		DAEDALUS_ERROR( "Unhandled swapping mode: %08x", mHeaderMagic );
		break;
	}
}

// Swap bytes from 37 80 40 12
// to              40 12 37 80
void ROMFile::ByteSwap_2301( void * p_bytes, u32 length )
{
	u32* p = (u32*)p_bytes;
	u32* maxp = (u32*)((u8*)p_bytes + length);
	for(; p < maxp; p++)
	{
		std::swap(((u16*)p)[0], ((u16*)p)[1]);
	}
}


// Swap bytes from 80 37 12 40
// to              40 12 37 80
void ROMFile::ByteSwap_3210( void * p_bytes, u32 length )
{
	u8* p = (u8*)p_bytes;
	u8* maxp = (u8*)(p + length);
	for(; p < maxp; p+=4)
	{
		std::swap(p[0], p[3]);
		std::swap(p[1], p[2]);
	}
}
