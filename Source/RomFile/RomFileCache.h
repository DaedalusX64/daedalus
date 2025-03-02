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

#ifndef UTILITY_ROMFILECACHE_H_
#define UTILITY_ROMFILECACHE_H_

#include "Base/Types.h"

class ROMFile;
struct SChunkInfo;

class ROMFileCache
{	using CacheIdx = u16;

	public:
		ROMFileCache();
		~ROMFileCache();

		bool				Open( std::shared_ptr<ROMFile> p_rom_file );
		void				Close();

		bool				GetChunk( u32 rom_offset, u8 ** p_p_chunk_base, u32 * p_chunk_offset, u32 * p_chunk_size );

	private:
		void				PurgeChunk( CacheIdx cache_idx );

		CacheIdx			GetCacheIndex( u32 address );

	private:
		std::shared_ptr<ROMFile> 		mpROMFile;

		u8 *				mpStorage;			// Underlying storage. This is carved up between different chunks
		std::unique_ptr<SChunkInfo[]>		mpChunkInfo;		// Info about which region a chunk is allocated to

		u32					mChunkMapEntries;	// i.e. Number of chunks in the rom
		std::unique_ptr<CacheIdx[]>			mpChunkMap;			// Map allowing quick lookups from address -> chunkidx

		u32					mMRUIdx;			// Most recently used index

		static const CacheIdx	INVALID_IDX = CacheIdx(-1);
};

#endif // UTILITY_ROMFILECACHE_H_
