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

#pragma once

#ifndef UTILITY_ROMFILEUNCOMPRESSED_H_
#define UTILITY_ROMFILEUNCOMPRESSED_H_

#include <fstream>

#include "RomFile/RomFile.h"


class ROMFileUncompressed : public ROMFile
{
public:
	ROMFileUncompressed( const std::filesystem::path filename );
	virtual ~ROMFileUncompressed();

	virtual bool		Open( COutputStream & messages );

	virtual bool		IsCompressed() const			{ return false; }
	virtual u32			GetRomSize() const				{ return mRomSize; }
	virtual bool		LoadRawData( u32 bytes_to_read, u8 *p_bytes, COutputStream & messages );

	virtual bool		ReadChunk( u32 offset, u8 * p_dst, u32 length );

private:
	std::fstream	mFH;
	u32					mRomSize;
};

#endif // UTILITY_ROMFILEUNCOMPRESSED_H_
