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

#ifndef HLEGRAPHICS_TEXTURECACHE_H_
#define HLEGRAPHICS_TEXTURECACHE_H_

#include "CachedTexture.h"

#include "Utility/Singleton.h"
#include "Utility/RefCounted.h"
#include "Utility/Mutex.h"

#include <vector>

struct TextureInfo;


class CTextureCache : public CSingleton< CTextureCache >
{
public:
	CTextureCache();
	virtual ~CTextureCache();

	CRefPtr<CNativeTexture>	GetOrCreateTexture(const TextureInfo & ti);

	void		PurgeOldTextures();
	void		DropTextures();


#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	Mutex * 	GetDebugMutex()		{ return &mDebugMutex; }
	struct STextureInfoSnapshot
	{
		STextureInfoSnapshot( const TextureInfo & info, CNativeTexture * texture )
		: Info( info ), Texture( texture )
		{
		}

		TextureInfo					Info;
		CRefPtr<CNativeTexture>		Texture;
	};

	// You must have a valid lock to call Snapshot.
	void		Snapshot(const MutexLock & lock, std::vector< STextureInfoSnapshot > & snapshot) const;
#else
	// Don't bother locking if we're not debugging.
	Mutex * 	GetDebugMutex()		{ return NULL; }
#endif

private:
	CachedTexture * GetOrCreateCachedTexture(const TextureInfo & ti);

	//
	//	We implement a 2-way skewed associative cache.
	//	Each TextureInfo is hashed using two different methods, to reduce the chance of collisions
	//
	static const u32 HASH_TABLE_BITS {9};
	static const u32 HASH_TABLE_SIZE {1<<HASH_TABLE_BITS};

	inline static u32 MakeHashIdxA( const TextureInfo & ti );
	inline static u32 MakeHashIdxB( const TextureInfo & ti );

	// FIXME(strmnnrmn): we should have a struct of TextureInfo+CachedTexture instead -
	// doing binary search on this array needs a memory indirect for every probe.
	typedef std::vector< CachedTexture * >	TextureVec;
	TextureVec			mTextures;
	CachedTexture *		mpCacheHashTable[HASH_TABLE_SIZE];
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	Mutex				mDebugMutex;
#endif
};

#endif // HLEGRAPHICS_TEXTURECACHE_H_
