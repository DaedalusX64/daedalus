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

#ifndef TEXTURECACHE_H__
#define TEXTURECACHE_H__

#include "CachedTexture.h"

#include "Utility/Singleton.h"
#include "Utility/RefCounted.h"

#include <vector>

struct TextureInfo;


class CTextureCache : public CSingleton< CTextureCache >
{
public:
	virtual ~CTextureCache();

	CRefPtr<CNativeTexture>	GetOrCreateTexture(const TextureInfo & ti);
	CRefPtr<CNativeTexture>	GetOrCreateWhiteTexture(const TextureInfo & ti);

	void		PurgeOldTextures();
	void		DropTextures();

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	struct STextureInfoSnapshot
	{
		explicit STextureInfoSnapshot( CachedTexture * texture ) : Texture( texture ) {}

		CRefPtr<CachedTexture>		Texture;
	};
	void		Snapshot( std::vector< STextureInfoSnapshot > & snapshot ) const;
#endif

private:
	CRefPtr<CachedTexture> GetOrCreateCachedTexture(const TextureInfo & ti);

	//
	//	We implement a 2-way skewed associative cache.
	//	Each TextureInfo is hashed using two different methods, to reduce the chance of collisions
	//
	static const u32 HASH_TABLE_BITS = 9;
	static const u32 HASH_TABLE_SIZE = 1<<HASH_TABLE_BITS;

	inline static u32 MakeHashIdxA( const TextureInfo & ti );
	inline static u32 MakeHashIdxB( const TextureInfo & ti );

	// FIXME(strmnnrmn): we should have a struct of TextureInfo+CachedTexture instead -
	// doing binary search on this array needs a memory indirect for every probe.
	typedef std::vector< CRefPtr<CachedTexture> >	TextureVec;
	TextureVec						mTextures;
	mutable CRefPtr<CachedTexture>	mpCacheHashTable[HASH_TABLE_SIZE];
};

#endif	// TEXTURECACHE_H__
