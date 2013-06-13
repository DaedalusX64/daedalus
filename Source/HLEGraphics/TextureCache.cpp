/*
Copyright (C) 2001 StrmnNrmn

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

// Manages textures for RDP code
// Uses a HashTable (hashing on TImg) to allow quick access
//  to previously used textures

#include "stdafx.h"

#include "TextureCache.h"
#include "TextureInfo.h"

#include "Utility/Profiler.h"

#include "DLDebug.h"

#include <vector>
#include <algorithm>

//#define PROFILE_TEXTURE_CACHE

template<> bool CSingleton< CTextureCache >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = new CTextureCache();
	return mpInstance != NULL;
}

CTextureCache::CTextureCache()
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
:	mDebugMutex("TextureCache")
#endif
{
	memset( mpCacheHashTable, 0, sizeof(mpCacheHashTable) );
}

CTextureCache::~CTextureCache()
{
	DropTextures();
}

inline u32 CTextureCache::MakeHashIdxA( const TextureInfo & ti )
{
	u32 address( ti.GetLoadAddress() );
	u32 hash( (address >> (HASH_TABLE_BITS*2)) ^ (address >> HASH_TABLE_BITS) ^ address );

	hash ^= ti.GetPalette() >> 2;			// Useful for palettised fonts, e.g in Starfox

	return hash & (HASH_TABLE_SIZE-1);
}

inline u32 CTextureCache::MakeHashIdxB( const TextureInfo & ti )
{
	return ti.GetHashCode() & (HASH_TABLE_SIZE-1);
}

// Purge any textures that haven't been used recently
void CTextureCache::PurgeOldTextures()
{
	MutexLock lock(GetDebugMutex());

	//
	//	Erase expired textures in reverse order, which should require less
	//	copying when large clumps of textures are released simultaneously.
	//
	for( s32 i = mTextures.size() - 1; i >= 0; --i )
	{
		CachedTexture * texture = mTextures[ i ];
		if ( texture->HasExpired() )
		{
			u32	ixa = MakeHashIdxA( texture->GetTextureInfo() );
			u32 ixb = MakeHashIdxB( texture->GetTextureInfo() );

			if( mpCacheHashTable[ixa] == texture )
			{
				mpCacheHashTable[ixa] = NULL;
			}
			if( mpCacheHashTable[ixb] == texture )
			{
				mpCacheHashTable[ixb] = NULL;
			}

			mTextures.erase( mTextures.begin() + i );

			delete texture;
		}
	}
}

void CTextureCache::DropTextures()
{
	MutexLock lock(GetDebugMutex());

	for( u32 i = 0; i < mTextures.size(); ++i)
	{
		delete mTextures[i];
	}
	mTextures.clear();
	for( u32 i = 0; i < HASH_TABLE_SIZE; ++i )
	{
		mpCacheHashTable[i] = NULL;
	}
}

#ifdef PROFILE_TEXTURE_CACHE
#define RECORD_CACHE_HIT( a, b )		TextureCacheStat( a, b, mTextures.size() )

static void TextureCacheStat( u32 l1_hit, u32 l2_hit, u32 size )
{
	static u32 total_lookups = 0;
	static u32 total_l1_hits = 0;
	static u32 total_l2_hits = 0;

	total_l1_hits += l1_hit;
	total_l2_hits += l2_hit;
	++total_lookups;

	if( total_lookups == 1000 )
	{
		printf( "L1 hits[%d] L2 hits[%d] Miss[%d] (%d entries)\n", total_l1_hits, total_l2_hits, total_lookups - (total_l1_hits+total_l2_hits), size );
		total_lookups = total_l1_hits = total_l2_hits = 0;
	}
}
#else

#define RECORD_CACHE_HIT( a, b )

#endif

struct SSortTextureEntries
{
public:
	bool operator()( const TextureInfo & a, const TextureInfo & b ) const
	{
		return a < b;
	}
	bool operator()( const CachedTexture * a, const TextureInfo & b ) const
	{
		return a->GetTextureInfo() < b;
	}
	bool operator()( const TextureInfo & a, const CachedTexture * b ) const
	{
		return a < b->GetTextureInfo();
	}
	bool operator()( const CachedTexture * a, const CachedTexture * b ) const
	{
		return a->GetTextureInfo() < b->GetTextureInfo();
	}
};

// If already in table, return cached copy
// Otherwise, create surfaces, and load texture into memory
CachedTexture * CTextureCache::GetOrCreateCachedTexture(const TextureInfo & ti)
{
	DAEDALUS_PROFILE( "CTextureCache::GetOrCreateCachedTexture" );

	// NB: this is a no-op in normal builds.
	MutexLock lock(GetDebugMutex());

	//
	// Retrieve the texture from the cache (if it already exists)
	//
	u32	ixa = MakeHashIdxA( ti );
	if( mpCacheHashTable[ixa] && mpCacheHashTable[ixa]->GetTextureInfo() == ti )
	{
		RECORD_CACHE_HIT( 1, 0 );
		mpCacheHashTable[ixa]->UpdateIfNecessary();

		return mpCacheHashTable[ixa];
	}

	u32 ixb = MakeHashIdxB( ti );
	if( mpCacheHashTable[ixb] && mpCacheHashTable[ixb]->GetTextureInfo() == ti )
	{
		RECORD_CACHE_HIT( 1, 0 );
		mpCacheHashTable[ixb]->UpdateIfNecessary();

		return mpCacheHashTable[ixb];
	}

	CachedTexture *	texture = NULL;
	TextureVec::iterator	it = std::lower_bound( mTextures.begin(), mTextures.end(), ti, SSortTextureEntries() );
	if( it != mTextures.end() && (*it)->GetTextureInfo() == ti )
	{
		texture = *it;
		RECORD_CACHE_HIT( 0, 1 );
	}
	else
	{
		texture = CachedTexture::Create( ti );
		if (texture != NULL)
		{
			mTextures.insert( it, texture );
		}

		RECORD_CACHE_HIT( 0, 0 );
	}

	// Update the hashtable
	if( texture )
	{
		texture->UpdateIfNecessary();

		mpCacheHashTable[ixa] = texture;
		mpCacheHashTable[ixb] = texture;
	}

	return texture;
}

CRefPtr<CNativeTexture> CTextureCache::GetOrCreateTexture(const TextureInfo & ti)
{
	CachedTexture * base_texture = GetOrCreateCachedTexture(ti);
	if (!base_texture)
		return NULL;

	return base_texture->GetTexture();
}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
void CTextureCache::Snapshot(const MutexLock & lock, std::vector< STextureInfoSnapshot > & snapshot) const
{
	DAEDALUS_ASSERT(lock.HasLock(mDebugMutex), "No debug lock");

	snapshot.erase( snapshot.begin(), snapshot.end() );

	for( TextureVec::const_iterator it = mTextures.begin(); it != mTextures.end(); ++it )
	{
		STextureInfoSnapshot	info( (*it)->GetTextureInfo(), (*it)->GetTexture() );
		snapshot.push_back( info );
	}
}
#endif // DAEDALUS_DEBUG_DISPLAYLIST
