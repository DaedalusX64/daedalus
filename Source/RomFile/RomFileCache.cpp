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
#include "Utility/MathUtil.h"
#include "RomFile/RomFile.h"
#include "RomFile/RomFileCache.h"
#include "RomFile/RomFileMemory.h"


#ifdef DAEDALUS_PSP
extern bool PSP_IS_SLIM;
#endif

namespace
{
	#ifdef DAEDALUS_PSP
	constexpr u32 chunkSize = 4 * 1024;
	#if PSP_IS_SLIM
			// 32MB Cache
		constexpr u32 cacheSize = 2048;
	#else
	// 2MB Cache (Phat)
		constexpr u32 cacheSize = 256;
	#endif

	#else
	constexpr u32 cacheSize = 1024;
	constexpr u32 chunkSize = 2 * 1024;
	#endif
	constexpr u32 storageBytes = cacheSize * chunkSize;

	const u32	INVALID_ADDRESS = u32( ~0 );
}

struct SChunkInfo
{
	u32				StartOffset;
	mutable u32		LastUseIdx;

	bool		ContainsAddress( u32 address ) const
	{
		return address >= StartOffset && address < StartOffset + chunkSize;
	}

	bool		InUse() const
	{
		return StartOffset != INVALID_ADDRESS;
	}

};


ROMFileCache::ROMFileCache()
:	mpROMFile( nullptr )
,	mChunkMapEntries( 0 )
,	mpChunkMap( NULL )
,	mMRUIdx( 0 )
{

	// STORAGE_BYTES = CACHE_SIZE * CHUNK_SIZE;
#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( (1<<(sizeof(CacheIdx)*8)) > cacheSize, "Need to increase size of CacheIdx typedef to allow sufficient entries to be indexed" );
#endif
	mpStorage   = (u8*)CROMFileMemory::Get()->Alloc( storageBytes );
	// std::unique_ptr<u8[]> mpStorage = std::make_unique<u8[]>(storageBytes);
	mpChunkInfo = std::make_unique<SChunkInfo[]>(cacheSize);
}

ROMFileCache::~ROMFileCache()
{
	CROMFileMemory::Get()->Free( mpStorage );
}

bool	ROMFileCache::Open( std::shared_ptr<ROMFile> p_rom_file )
{
	mpROMFile = p_rom_file;

	u32		rom_size( p_rom_file->GetRomSize() );
	u32		rom_chunks( AlignPow2( rom_size, chunkSize ) / chunkSize );

	mChunkMapEntries = rom_chunks;
	mpChunkMap = std::make_unique<CacheIdx[]>(rom_chunks);

	// Invalidate all entries
	for(u32 i = 0; i < rom_chunks; ++i)
	{
		mpChunkMap[ i ] = INVALID_IDX;
	}

	for(u32 i = 0; i < cacheSize; ++i)
	{
		mpChunkInfo[ i ].StartOffset = INVALID_ADDRESS;
		mpChunkInfo[ i ].LastUseIdx = 0;
	}

	return true;
}

void	ROMFileCache::Close()
{
	// delete [] mpChunkMap;
	mpChunkMap = nullptr;
	mChunkMapEntries = 0;
	mpROMFile = nullptr;
}

inline u32 AddressToChunkMapIndex( u32 address )
{
	return address / chunkSize;
}


inline u32 GetChunkStartAddress( u32 address )
{
	return ( address / chunkSize ) * chunkSize;
}


void	ROMFileCache::PurgeChunk( CacheIdx cache_idx )
{
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( cache_idx < cacheSize, "Invalid chunk index" );
	#endif

	SChunkInfo & chunk_info =  mpChunkInfo[ cache_idx ];
	u32	current_chunk_address = chunk_info.StartOffset;
	if( chunk_info.InUse() )
	{
		//DBGConsole_Msg( 0, "[CRomCache - purging %02x %08x-%08x", cache_idx, chunk_info.StartOffset, chunk_info.StartOffset + CHUNK_SIZE );
		u32	chunk_map_idx = AddressToChunkMapIndex( current_chunk_address ) ;
		#ifdef DAEDALUS_ENABLE_ASSERTS
		DAEDALUS_ASSERT( chunk_map_idx < mChunkMapEntries, "Chunk address is out of range?" );
		DAEDALUS_ASSERT( mpChunkMap[ chunk_map_idx ] == cache_idx, "Chunk map inconsistancy" );
		#endif
		// Scrub down the chunk map to show it's no longer cached
		mpChunkMap[ chunk_map_idx ] = INVALID_IDX;
	}
	else
	{
		//DBGConsole_Msg( 0, "[CRomCache - purging %02x (unused)", cache_idx );
	}

	// Scrub these down
	chunk_info.StartOffset = INVALID_ADDRESS;
	chunk_info.LastUseIdx = 0;
}

ROMFileCache::CacheIdx	ROMFileCache::GetCacheIndex( u32 address )
{
	u32		chunk_map_idx = AddressToChunkMapIndex( address );
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( chunk_map_idx < mChunkMapEntries, "Chunk address is out of range?" );
	#endif
	//
	//	Check if this chunk is already cached, load if necessary
	//
	CacheIdx	idx = mpChunkMap[ chunk_map_idx ];
	if(idx == INVALID_IDX)
	{
		CacheIdx	selected_idx = 0;
		u32			oldest_timestamp( mpChunkInfo[ 0 ].LastUseIdx );

		for(CacheIdx i = 1; i < cacheSize; ++i)
		{
			u32	timestamp = mpChunkInfo[ i ].LastUseIdx;
			if(timestamp < oldest_timestamp)
			{
				oldest_timestamp = timestamp;
				selected_idx = i;
			}
		}

		//
		//	Purge the current chunk
		//
		PurgeChunk( selected_idx );

		//	And load up the new one
		SChunkInfo &chunk_info =  mpChunkInfo[ selected_idx ];
		chunk_info.StartOffset = GetChunkStartAddress( address );
		chunk_info.LastUseIdx = ++mMRUIdx;

		#ifdef DAEDALUS_ENABLE_ASSERTS
		DAEDALUS_ASSERT( chunk_map_idx < mChunkMapEntries, "Chunk address is out of range?" );
		#endif

		mpChunkMap[ chunk_map_idx ] = selected_idx;

		u32		storage_offset =  selected_idx * chunkSize;
		u8 *	p_dst =  mpStorage + storage_offset;

		//DBGConsole_Msg( 0, "[CRomCache - loading %02x, %08x-%08x", selected_idx, chunk_info.StartOffset, chunk_info.StartOffset + chunkSize );
		mpROMFile->ReadChunk( chunk_info.StartOffset, p_dst, chunkSize );

		idx = selected_idx;
	}

	return idx;
}

bool	ROMFileCache::GetChunk( u32 rom_offset, u8 ** p_p_chunk_base, u32 * p_chunk_offset, u32 * p_chunk_size )
{
	u32		chunk_map_idx = AddressToChunkMapIndex( rom_offset );

	if(chunk_map_idx < mChunkMapEntries)
	{
		CacheIdx idx = GetCacheIndex( rom_offset );
		#ifdef DAEDALUS_ENABLE_ASSERTS
		DAEDALUS_ASSERT( idx < cacheSize, "Invalid chunk index!" );
		#endif

		const SChunkInfo &chunk_info =  mpChunkInfo[ idx ];

		#ifdef DAEDALUS_ENABLE_ASSERTS
		DAEDALUS_ASSERT( AddressToChunkMapIndex( chunk_info.StartOffset ) == chunk_map_idx, "Inconsistant map indices" );
		DAEDALUS_ASSERT( chunk_info.ContainsAddress( rom_offset ), "Address is out of range for chunk" );
		#endif

		u32	storage_offset =  idx * chunkSize;

		*p_p_chunk_base = mpStorage + storage_offset;
		*p_chunk_offset = chunk_info.StartOffset;
		*p_chunk_size = chunkSize;					// XXXX if last chunk, adjust this?

		chunk_info.LastUseIdx = ++mMRUIdx;
		return true;
	}
	else
	{
		// Invalid address, no chunk available
		return false;
	}
}
