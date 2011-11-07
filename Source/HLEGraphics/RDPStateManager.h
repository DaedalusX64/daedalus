/*
Copyright (C) 2006,2007 StrmnNrmn

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


#ifndef RDPSTATEMANAGER_H_
#define RDPSTATEMANAGER_H_

#include "RDP.h"
#include "TextureDescriptor.h"

#include <map>

class CTexture;

class CRDPStateManager
{
public:

	CRDPStateManager();
	~CRDPStateManager();

	void							Reset();

	inline const RDP_Tile &			GetTile( u32 idx ) const				{ return mTiles[ idx ]; }
	inline const RDP_TileSize &		GetTileSize( u32 idx ) const			{ return mTileSizes[ idx ]; }

	void							SetTile( const RDP_Tile & tile );
	void							SetTileSize( const RDP_TileSize & tile_size );
	void							LoadBlock( u32 idx, u32 address, bool swapped );
	void							LoadTile( const RDP_TileSize & tile_size );

// Retrive tile addr loading. used by Yoshi_MemRect
	inline u32						GetTileAddress( const u32 tmem ) { LoadDetailsMap::const_iterator it( mLoadMap.find( tmem ) ); const SLoadDetails &	load_details( it->second ); return load_details.Address; }

	const TextureInfo &				GetTextureDescriptor( const u32 idx ) const;

private:
	void							InvalidateAllTileTextureInfo();

private:
	RDP_Tile				mTiles[ 8 ];
	RDP_TileSize			mTileSizes[ 8 ];

	struct SLoadDetails
	{
		SImageDescriptor	Image;
		RDP_TileSize		TileSize;

		u32					Address;		// Base address of texture
		u32					Pitch;			// May be different from that derived from Image.Pitch
		bool				Swapped;
	};


	typedef	std::map< u32, SLoadDetails > LoadDetailsMap;
	LoadDetailsMap			mLoadMap;

	mutable TextureInfo		mTileTextureInfo[ 8 ];
	mutable bool			mTileTextureInfoValid[ 8 ];		// Set to false if this needs rebuilding

	mutable u32				mNumReused;
	mutable u32				mNumLoaded;
};

extern CRDPStateManager		gRDPStateManager;


#endif // RDPSTATEMANAGER_H_
