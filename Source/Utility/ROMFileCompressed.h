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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __DAEDALUS_ROMFILECOMPRESSED_H__
#define __DAEDALUS_ROMFILECOMPRESSED_H__

#ifdef DAEDALUS_ZLIB_SUPPORT

// This is required so that the linker doesn't expect __fastcall unzXYZ functions.
#define ZEXPORT DAEDALUS_ZLIB_CALL_TYPE

#include "unzip.h"

#include "ROMFile.h"

class ROMFileCompressed : public ROMFile
{
public:
	ROMFileCompressed( const char * filename );

	virtual ~ROMFileCompressed();

	virtual bool		Open( COutputStream & messages );

	virtual bool		IsCompressed() const			{ return true; }
	virtual u32			GetRomSize() const				{ return mRomSize; }
	virtual bool		LoadRawData( u32 bytes_to_read, u8 ** p_p_bytes, u32 * p_buffer_size, u32 * p_rom_size, COutputStream & messages );

	virtual bool		ReadChunk( u32 offset, u8 * p_dst, u32 length );

private:
			bool		Seek( u32 offset, u8 * p_scratch_block, u32 block_size );


private:
	unzFile				mZipFile;
	bool				mFoundRom;
	u32					mRomSize;

};

#endif // DAEDALUS_ZLIB_SUPPORT

#endif // __DAEDALUS_ROMFILECOMPRESSED_H__
