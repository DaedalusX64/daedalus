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
#include "TextureDescriptor.h"

#include "Utility/Profiler.h"

#include "DebugDisplayList.h"

#include <vector>
#include <algorithm>

//#define PROFILE_TEXTURE_CACHE

//*************************************************************************************
//
//*************************************************************************************
class ITextureCache : public CTextureCache
{
public:
	ITextureCache();
	~ITextureCache();
	
			void			PurgeOldTextures();
			void			DropTextures();
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
			void			SetDumpTextures( bool dump_textures )	{ mDumpTextures = dump_textures; }
			bool			GetDumpTextures( ) const				{ return mDumpTextures; }
#endif	
			CRefPtr<CTexture>GetTexture(const TextureInfo * pti);

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	virtual	void			DisplayStats();

			void			Snapshot( std::vector< STextureInfoSnapshot > & snapshot ) const;

protected:
			void			GetUsedTextureStats( u32 * p_num_textures, u32 * p_video_memory_used, u32 * p_system_memory_used ) const;
#endif

protected:
	struct SSortTextureEntries
	{
	public:
		bool operator()( const TextureInfo & a, const TextureInfo & b ) const
		{
			return a < b;
		}
		bool operator()( const CRefPtr<CTexture> & a, const TextureInfo & b ) const
		{
			return a->GetTextureInfo() < b;
		}
		bool operator()( const TextureInfo & a, const CRefPtr<CTexture> & b ) const
		{
			return a < b->GetTextureInfo();
		}
			bool operator()( const CRefPtr<CTexture> & a, const CRefPtr<CTexture> & b ) const
		{
			return a->GetTextureInfo() < b->GetTextureInfo();
		}
	};

	typedef std::vector< CRefPtr<CTexture> >	TextureVec;
	TextureVec				mTextures;

	//
	//	We implement a 2-way skewed associative cache.
	//	Each TextureInfo is hashed using two different methods, to reduce the chance of collisions
	//
	static const u32 HASH_TABLE_BITS = 9;
	static const u32 HASH_TABLE_SIZE = 1<<HASH_TABLE_BITS;

	inline static u32 MakeHashIdxA( const TextureInfo & ti )
	{
		u32 address( ti.GetLoadAddress() );
		u32 hash( (address >> (HASH_TABLE_BITS*2)) ^ (address >> HASH_TABLE_BITS) ^ address );
		
		hash ^= ti.GetTLutIndex() >> 2;			// Useful for palettised fonts, e.g in Starfox

		return hash & (HASH_TABLE_SIZE-1);
	}
	inline static u32 MakeHashIdxB( const TextureInfo & ti )
	{
		return ti.GetHashCode() & (HASH_TABLE_SIZE-1);
	}

	mutable CRefPtr<CTexture>	mpCacheHashTable[HASH_TABLE_SIZE];

	bool					mDumpTextures;
};


// Interface

//*****************************************************************************
//
//*****************************************************************************
template<> bool CSingleton< CTextureCache >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);
	
	mpInstance = new ITextureCache();
	return mpInstance != NULL;
}

//*************************************************************************************
//
//*************************************************************************************
ITextureCache::ITextureCache()
:	mDumpTextures(false)
{
}

//*************************************************************************************
//
//*************************************************************************************
ITextureCache::~ITextureCache()
{
	DropTextures();
}


//*************************************************************************************
// Purge any textures that haven't been used recently
//*************************************************************************************
void ITextureCache::PurgeOldTextures()
{
	//
	//	Erase expired textures in reverse order, which should require less
	//	copying when large clumps of textures are released simultaneously.
	//
	for( s32 i = mTextures.size() - 1; i >= 0; --i )
	{
		CRefPtr<CTexture> &	texture( mTextures[ i ] );
		if ( texture->HasExpired() )
		{
			//printf("Texture load address -> %d\n",mTextures[ i ]->GetTextureInfo().GetLoadAddress());

			u32	ixa( MakeHashIdxA( texture->GetTextureInfo() ) );
			u32 ixb( MakeHashIdxB( texture->GetTextureInfo() ) );

			if( mpCacheHashTable[ixa] == texture )
			{
				mpCacheHashTable[ixa] = NULL;
			}
			if( mpCacheHashTable[ixb] == texture )
			{
				mpCacheHashTable[ixb] = NULL;
			}

			mTextures.erase( mTextures.begin() + i );
		}
	}
}

//*************************************************************************************
//
//*************************************************************************************
void ITextureCache::DropTextures()
{
	mTextures.clear();
	for( u32 i = 0; i < HASH_TABLE_SIZE; ++i )
	{
		mpCacheHashTable[i] = NULL;
	}
}

//*************************************************************************************
//
//*************************************************************************************
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

//*************************************************************************************
// If already in table, return cached copy
// Otherwise, create surfaces, and load texture into memory
//*************************************************************************************
CRefPtr<CTexture> ITextureCache::GetTexture(const TextureInfo * pti)
{
	DAEDALUS_PROFILE( "ITextureCache::GetTexture" );

	//
	// Retrieve the texture from the cache (if it already exists)
	//
	u32	ixa( MakeHashIdxA( *pti ) );
	if( mpCacheHashTable[ixa] && mpCacheHashTable[ixa]->GetTextureInfo() == *pti )
	{
		RECORD_CACHE_HIT( 1, 0 );
		return mpCacheHashTable[ixa];
	}

	u32 ixb( MakeHashIdxB( *pti ) );
	if( mpCacheHashTable[ixb] && mpCacheHashTable[ixb]->GetTextureInfo() == *pti )
	{
		RECORD_CACHE_HIT( 1, 0 );
		return mpCacheHashTable[ixb];
	}

	CRefPtr<CTexture>		texture;
	TextureVec::iterator	it( std::lower_bound( mTextures.begin(), mTextures.end(), *pti, SSortTextureEntries() ) );
	if( it != mTextures.end() && (*it)->GetTextureInfo() == *pti )
	{
		texture = *it;
		RECORD_CACHE_HIT( 0, 1 );
	}
	else
	{
		texture = CTexture::Create( *pti );
		if (texture != NULL)
		{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
			if ( mDumpTextures )
			{
				texture->DumpTexture();
			}
#endif
			mTextures.insert( it, texture );
		}

		RECORD_CACHE_HIT( 0, 0 );
	}

	// Update the hashtable
	if( texture )
	{
		mpCacheHashTable[ixa] = texture;
		mpCacheHashTable[ixb] = texture;
	}

	return texture;
}
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
//*************************************************************************************
//
//*************************************************************************************
void	ITextureCache::GetUsedTextureStats( u32 * p_num_textures,
											u32 * p_video_memory_used,
											u32 * p_system_memory_used ) const
{
	u32	num_textures( 0 );
	u32	video_memory_used( 0 );
	u32	system_memory_used( 0 );

	for( TextureVec::const_iterator it = mTextures.begin(); it != mTextures.end(); ++it )
	{
		const CRefPtr<CTexture> & texture( *it );
		num_textures++;
		video_memory_used += texture->GetVideoMemoryUsage();
		system_memory_used += texture->GetSystemMemoryUsage();
	}

	*p_num_textures = num_textures;
	*p_video_memory_used = video_memory_used;
	*p_system_memory_used = system_memory_used;
}

//*************************************************************************************
//	Display info on the number of textures in the cache
//*************************************************************************************
void	ITextureCache::DisplayStats()
{
	u32		used_textures;
	u32		used_texture_video_mem;
	u32		used_texture_system_mem;

	GetUsedTextureStats( &used_textures, &used_texture_video_mem, &used_texture_system_mem );

	printf( "Texture Cache Stats\n" );
	printf( " Used: %3d v:%4dKB s:%4dKB\n", used_textures, used_texture_video_mem / 1024, used_texture_system_mem / 1024 );
}

//*************************************************************************************
//
//*************************************************************************************
void	ITextureCache::Snapshot( std::vector< STextureInfoSnapshot > & snapshot ) const
{
	snapshot.erase( snapshot.begin(), snapshot.end() );

	for( TextureVec::const_iterator it = mTextures.begin(); it != mTextures.end(); ++it )
	{
		STextureInfoSnapshot	info( *it );
		snapshot.push_back( info );
	}
}

CTextureCache::STextureInfoSnapshot::STextureInfoSnapshot( CTexture * p_texture )
:	Texture( p_texture )
{
}

CTextureCache::STextureInfoSnapshot::STextureInfoSnapshot( const STextureInfoSnapshot & rhs )
:	Texture( rhs.Texture )
{
}

CTextureCache::STextureInfoSnapshot & CTextureCache::STextureInfoSnapshot::operator=( const STextureInfoSnapshot & rhs )
{
	if( this != &rhs )
	{
		Texture = rhs.Texture;
	}

	return *this;
}

CTextureCache::STextureInfoSnapshot::~STextureInfoSnapshot()
{
}


#endif	//DAEDALUS_DEBUG_DISPLAYLIST
