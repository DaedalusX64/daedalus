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


#ifndef HLEGRAPHICS_RDPSTATEMANAGER_H_
#define HLEGRAPHICS_RDPSTATEMANAGER_H_

#include "RDP.h"
#include "TextureInfo.h"

#include <map>

struct SetLoadTile;

class CRDPStateManager
{
public:
	CRDPStateManager();
	~CRDPStateManager();

	void							Reset();

	void 							SetEmulateMirror(bool enable)			{ EmulateMirror = enable; }


	//inline const u32				GetTmemAdr( u32 idx ) const				{ return mTiles[ idx ].tmem; }
	inline const RDP_Tile &			GetTile( u32 idx ) const				{ return mTiles[ idx ]; }
	inline const RDP_TileSize &		GetTileSize( u32 idx ) const			{ return mTileSizes[ idx ]; }

	inline bool IsTileInitialised( u32 idx ) const
	{
		// mTile is zeroed to RGBA/4 on init. If we're RGBA/4, assume we're not initialised.
		return mTiles[idx].format != 0 || mTiles[idx].size != 0;
	}

	void							SetTile( const RDP_Tile & tile );
	void							SetTileSize( const RDP_TileSize & tile_size );

	void							LoadBlock(const SetLoadTile & load);
	void							LoadTile(const SetLoadTile & load);
	void							LoadTlut(const SetLoadTile & load);

	// Retrive tile addr loading. used by Yoshi_MemRect
	inline u32						GetTileAddress( u32 tmem ) const { return mTmemLoadInfo[ tmem >> 4 ].Address; }

	const TextureInfo &				GetUpdatedTextureDescriptor( u32 idx );

private:
	inline void				InvalidateAllTileTextureInfo()		{ memset( mTileTextureInfoValid, 0, sizeof(mTileTextureInfoValid) ); }
	inline u32				EntryIsValid( const u32 tmem )const	{ return (mValidEntryBits >> tmem) & 1; }	//Return 1 if entry is valid else 0
	inline void				SetValidEntry( const u32 tmem )		{ mValidEntryBits |= (1 << tmem); }	//Set TMEM address entry as valid
	inline void				ClearEntries( const u32 tmem )		{ mValidEntryBits &= ((u32)~0 >> (31-tmem)); }	//Clear all entries after the specified TMEM address
	inline void				ClearAllEntries()					{ mValidEntryBits = 0; }	//Clear all entries

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
	u32						mValidEntryBits;		//Use bits to signal valid entries in TMEM

	TextureInfo				mTileTextureInfo[ 8 ];
	bool					mTileTextureInfoValid[ 8 ];		// Set to false if this needs rebuilding

	bool					EmulateMirror;
};

extern CRDPStateManager		gRDPStateManager;
extern RDP_OtherMode		gRDPOtherMode;

extern u32* gTlutLoadAddresses[ 4096 >> 6 ];
#define TLUT_BASE ((u32)(gTlutLoadAddresses[0]))


#endif // HLEGRAPHICS_RDPSTATEMANAGER_H_
