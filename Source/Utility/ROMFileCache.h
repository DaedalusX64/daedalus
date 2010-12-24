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

#ifndef __DAEDALUS_ROMFILECACHE_H__
#define __DAEDALUS_ROMFILECACHE_H__

class ROMFile;
struct SChunkInfo;

class ROMFileCache
{
		typedef u16			CacheIdx;

	public:
		ROMFileCache();
		~ROMFileCache();

		bool				Open( ROMFile * p_rom_file );
		void				Close();

		bool				GetChunk( u32 rom_offset, u8 ** p_p_chunk_base, u32 * p_chunk_offset, u32 * p_chunk_size );

	private:
		void				PurgeChunk( CacheIdx cache_idx );

		CacheIdx			GetCacheIndex( u32 address );

		static u32			AddressToChunkMapIndex( u32 address );
		static u32			GetChunkStartAddress( u32 address );
		
	private:
		ROMFile *			mpROMFile;

		u8 *				mpStorage;			// Underlying storage. This is carved up between different chunks
		SChunkInfo *		mpChunkInfo;		// Info about which region a chunk is allocated to

		u32					mChunkMapEntries;	// i.e. Number of chunks in the rom
		CacheIdx *			mpChunkMap;			// Map allowing quick lookups from address -> chunkidx

		u32					mMRUIdx;			// Most recently used index

		static const CacheIdx	INVALID_IDX = CacheIdx(-1);
};

#endif // __DAEDALUS_ROMFILECACHE_H__
