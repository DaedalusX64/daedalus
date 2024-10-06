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

#ifndef UTILITY_ROMFILE_H_
#define UTILITY_ROMFILE_H_


#include <array>
#include "Utility/Paths.h"

class COutputStream;


const std::array<std::filesystem::path, 9> valid_extensions = {".v64", ".z64", ".n64", ".rom", ".bin", ".jap", ".pal", ".usa", ".zip"};

class ROMFile
{
public:
	static std::shared_ptr<ROMFile> Create( const std::filesystem::path& filename );

	ROMFile( const std::filesystem::path& filename );
	virtual ~ROMFile();

			bool		LoadData( u32 bytes_to_read, u8 *p_bytes, COutputStream & messages );

			//
			//	Streaming functions
			//
	virtual bool		Open( COutputStream & messages ) = 0;
	virtual bool		ReadChunk( u32 offset, u8 * p_dst, u32 length ) = 0;

	virtual bool		IsCompressed() const = 0;
			bool		RequiresSwapping() const;

	virtual u32			GetRomSize() const = 0;

	static	void		ByteSwap_2301( void * p_bytes, u32 length );
	static	void		ByteSwap_3210( void * p_bytes, u32 length );

protected:
			bool		SetHeaderMagic( u32 magic );
			void		CorrectSwap( u8 * p_bytes, u32 length );

private:
	virtual bool		LoadRawData( u32 bytes_to_read, u8 *p_bytes, COutputStream & messages ) = 0;

protected:
	std::filesystem::path mFilename;
	u32					mHeaderMagic;
};

#endif // UTILITY_ROMFILE_H_
