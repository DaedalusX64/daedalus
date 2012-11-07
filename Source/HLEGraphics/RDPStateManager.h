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

	//inline const u32				GetTmemAdr( u32 idx ) const				{ return mTiles[ idx ].tmem; }
	inline const RDP_Tile &			GetTile( u32 idx ) const				{ return mTiles[ idx ]; }
	inline const RDP_TileSize &		GetTileSize( u32 idx ) const			{ return mTileSizes[ idx ]; }

	void							SetTile( const RDP_Tile & tile );
	void							SetTileSize( const RDP_TileSize & tile_size );
	void							LoadBlock( u32 idx, u32 address, bool swapped );
	void							LoadTile( u32 idx, u32 address );
	//void							LoadTlut( u32 idx, u32 address )

	// Retrive tile addr loading. used by Yoshi_MemRect
	inline u32						GetTileAddress( const u32 tmem ) const { return mTmemLoadInfo[ tmem >> 4 ].Address; }

	const TextureInfo &				GetTextureDescriptor( const u32 idx ) const;

private:
	void					InvalidateAllTileTextureInfo()		{ for( u32 i = 0; i < 8; ++i ) mTileTextureInfoValid[ i ] = false; }
	inline u32				EntryIsValid( const u32 tmem )const	{ return (Valid_Entry >> tmem) & 1; }	//Return 1 if entry is valid else 0
	inline void				SetValidEntry( const u32 tmem )		{ Valid_Entry |= (1 << tmem); }	//Set TMEM address entry as valid
	inline void				ClearEntries( const u32 tmem )		{ Valid_Entry &= ((u32)~0 >> (31-tmem)); }	//Clear all entries after the specified TMEM address
	inline void				ClearAllEntries()					{ Valid_Entry = 0; }	//Clear all entries

private:
	struct TimgLoadDetails
	{
		u32					Address;		// Base address of texture (same address as from Timg ucode)
		u32					Pitch;			// May be different from that derived from Image.Pitch
		bool				Swapped;
	};

	RDP_Tile				mTiles[ 8 ];
	RDP_TileSize			mTileSizes[ 8 ];
	TimgLoadDetails			mTmemLoadInfo[ 32 ];	//Subdivide TMEM area into 32 slots and keep track of texture loads (LoadBlock/LoadTile/LoadTlut) //Corn
	u32						Valid_Entry;		//Use bits to signal valid entries in TMEM

	mutable TextureInfo		mTileTextureInfo[ 8 ];
	mutable bool			mTileTextureInfoValid[ 8 ];		// Set to false if this needs rebuilding
};

extern CRDPStateManager		gRDPStateManager;


#endif // RDPSTATEMANAGER_H_
