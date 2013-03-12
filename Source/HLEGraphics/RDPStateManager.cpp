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

#include "stdafx.h"
#include "RDPStateManager.h"

#include "Core/ROM.h"
#include "Core/Memory.h"
#include "DLDebug.h"

#include "Math/MathUtil.h"

#include "OSHLE/ultra_gbi.h"

#include "HLEGraphics/uCodes/UcodeDefs.h"


extern SImageDescriptor g_TI;		//Texture data from Timg ucode

CRDPStateManager gRDPStateManager;

static const char * const kTLUTTypeName[] = {"None", "?", "RGBA16", "IA16"};

CRDPStateManager::CRDPStateManager()
{
	ClearAllEntries();
	InvalidateAllTileTextureInfo();
}

CRDPStateManager::~CRDPStateManager()
{
}

void CRDPStateManager::Reset()
{
	ClearAllEntries();
	InvalidateAllTileTextureInfo();
}

void CRDPStateManager::SetTile( const RDP_Tile & tile )
{
	u32 idx( tile.tile_idx );

	if( mTiles[ idx ] != tile )
	{
		mTiles[ idx ] = tile;
		mTileTextureInfoValid[ idx ] = false;
	}
}

void CRDPStateManager::SetTileSize( const RDP_TileSize & tile_size )
{
	u32 idx( tile_size.tile_idx );

	if( mTileSizes[ idx ] != tile_size )
	{
		// XXXX might be able to remove this with recent tile loading fixes?
		// Wetrix hack
		if( (tile_size.top > tile_size.bottom) | (tile_size.left > tile_size.right) )
		{
			DAEDALUS_DL_ERROR( "Specifying negative width/height for tile descriptor" );
			return;
		}

		mTileSizes[ idx ] = tile_size;
		mTileTextureInfoValid[ idx ] = false;
	}
}

void CRDPStateManager::LoadBlock(const SetLoadTile & load)
{
	u32 uls			= load.sl;	//Left
	u32 ult			= load.tl;	//Top
	u32 dxt			= load.th;	// 1.11 fixed point
	u32 tile_idx	= load.tile;
	u32 address		= g_TI.GetAddress( uls, ult );
	//u32 ByteSize	= (load.sh + 1) << (g_TI.Size == G_IM_SIZ_32b);

	bool	swapped = (dxt) ? false : true;

	DL_PF("    Tile[%d] (%d,%d - %d) DXT[0x%04x] = [%d]Bytes/line => [%d]Pixels/line Address[0x%08x]",
		tile_idx,
		uls, ult,
		load.sh,
		dxt,
		(g_TI.Width << g_TI.Size >> 1),
		bytes2pixels( (g_TI.Width << g_TI.Size >> 1), g_TI.Size ),
		address);

	InvalidateAllTileTextureInfo();		// Can potentially invalidate all texture infos

	u32	tmem_lookup( mTiles[ tile_idx ].tmem >> 4 );

	//Invalidate load info after current TMEM address to the end of TMEM (fixes Fzero and SSV) //Corn
	ClearEntries( tmem_lookup );
	SetValidEntry( tmem_lookup );

	TimgLoadDetails & info = mTmemLoadInfo[ tmem_lookup ];
	info.Address = address;
	info.Pitch	 = ~0;
	info.Swapped = swapped;
}

void CRDPStateManager::LoadTile(const SetLoadTile & load)
{
	u32 uls      = load.sl;	//Left
	u32 ult      = load.tl;	//Top
	u32 tile_idx = load.tile;
	u32 address  = g_TI.GetAddress( uls / 4, ult / 4 );

	DL_PF("    Tile[%d] (%d,%d)->(%d,%d) [%d x %d] Address[0x%08x]",
		tile_idx,
		load.sl / 4, load.tl / 4, load.sh / 4 + 1, load.th / 4 + 1,
		(load.sh - load.sl) / 4 + 1, (load.th - load.tl) / 4 + 1,
		address);

	InvalidateAllTileTextureInfo();		// Can potentially invalidate all texture infos

	u32	tmem_lookup( mTiles[ tile_idx ].tmem >> 4 );

	SetValidEntry( tmem_lookup );

	TimgLoadDetails & info = mTmemLoadInfo[ tmem_lookup ];
	info.Address = address;
	info.Pitch = g_TI.GetPitch();
	info.Swapped = false;
}

void CRDPStateManager::LoadTlut(const SetLoadTile & load)
{
	// Tlut fmt is sometimes wrong (in 007) and is set after tlut load, but before tile load
	// Format is always 16bpp - RGBA16 or IA16:
	//DAEDALUS_DL_ASSERT(g_TI.Size == G_IM_SIZ_16b, "Crazy tlut load - not 16bpp");

	u32    uls        = load.sl;		//Left
	u32    ult        = load.tl;		//Top
	u32    lrs        = load.sh;		//Right
	u32    tile_idx   = load.tile;
	u32    ram_offset = g_TI.GetAddress16bpp(uls >> 2, ult >> 2);
	void * address    = g_pu8RamBase + ram_offset;

	const RDP_Tile & rdp_tile = gRDPStateManager.GetTile( tile_idx );

	u32 count = ((lrs - uls)>>2) + 1;
	use(count);

#ifdef DAEDALUS_FAST_TMEM
	//Store address of PAL (assuming PAL is only stored in upper half of TMEM) //Corn
	gTlutLoadAddresses[ (rdp_tile.tmem>>2) & 0x3F ] = (u32*)address;
#else
	//This corresponds to the number of palette entries (16 or 256) 16bit
	//Seems partial load of palette is allowed -> count != 16 or 256 (MM, SSB, Starfox64, MRC) //Corn
	u32 offset = rdp_tile.tmem - 256;				// starting location in the palettes
	DAEDALUS_ASSERT( count <= 256, "Check me: TMEM - count is %d", count );

	//Copy PAL to the PAL memory
	u16 * palette = (u16*)address;

	for (u32 i=0; i<count; i++)
	{
		gPaletteMemory[ i+offset ] = palette[ i ];
	}

	//printf("Addr %08X : TMEM %03X : Tile %d : PAL %d\n", ram_offset, tmem, tile_idx, count);
#endif

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	u32 lrt = load.th;	//Bottom

	DL_PF("    TLut Addr[0x%08x] TMEM[0x%03x] Tile[%d] Count[%d] Format[%s] (%d,%d)->(%d,%d)",
		address, rdp_tile.tmem, tile_idx, count, kTLUTTypeName[gRDPOtherMode.text_tlut], uls >> 2, ult >> 2, lrs >> 2, lrt >> 2);

#endif
}

// Limit the tile's width/height to the number of bits specified by mask_s/t.
// See the detailed noted in BaseRenderer::UpdateTileSnapshots for issues relating to this.
static inline u16 GetTextureDimension( u16 tile_dimension, u8 mask )
{
	if( mask != 0 )
	{
		return Min< u16 >( 1 << mask, tile_dimension );
	}

	return tile_dimension;
}

const TextureInfo & CRDPStateManager::GetTextureDescriptor( u32 idx ) const
{
	DAEDALUS_ASSERT( idx < ARRAYSIZE( mTileTextureInfoValid ), "Invalid index %d", idx );
	if( !mTileTextureInfoValid[ idx ] )
	{
		TextureInfo &			ti( mTileTextureInfo[ idx ] );

		const RDP_Tile &		rdp_tile( mTiles[ idx ] );
		const RDP_TileSize &	rdp_tilesize( mTileSizes[ idx ] );
		const u32				tmem_lookup( rdp_tile.tmem >> 4 );
		const TimgLoadDetails&	info( mTmemLoadInfo[ tmem_lookup ] );

		u32		address( info.Address );
		u32		pitch( info.Pitch );
		bool	swapped( info.Swapped );
		u32		tlut( TLUT_BASE );

		//Check if tmem_lookup has a valid entry, if not we assume load was done on TMEM[0] and we add the offset //Corn
		//Games that uses this is Fzero/Space station Silicon Valley/Animal crossing.
		if(	EntryIsValid( tmem_lookup ) == 0 )
		{
			const TimgLoadDetails&	info_base ( mTmemLoadInfo[ 0 ] );

			//Calculate offset in bytes and add to base address
			address = info_base.Address + (rdp_tile.tmem << 3);
			pitch	= info_base.Pitch;
			swapped = info_base.Swapped;
		}

		// If it was a Block Load - the pitch is determined by the tile size.
		// Else if it was a Tile Load - the pitch is set when the tile is loaded.
		if ( pitch == u32(~0) )
		{
			if( rdp_tile.size == G_IM_SIZ_32b )	pitch = rdp_tile.line << 4;
			else pitch = rdp_tile.line << 3;
		}

		//	Limit the tile's width/height to the number of bits specified by mask_s/t.
		//	See the detailed notes in BaseRenderer::UpdateTileSnapshots for issues relating to this.
		//
		u16		tile_width( GetTextureDimension( rdp_tilesize.GetWidth(), rdp_tile.mask_s ) );
		u16		tile_height( GetTextureDimension( rdp_tilesize.GetHeight(), rdp_tile.mask_t ) );

#ifdef DAEDALUS_ENABLE_ASSERTS
		u32		num_pixels( tile_width * tile_height );
		u32		num_bytes( pixels2bytes( num_pixels, rdp_tile.size ) );
		DAEDALUS_DL_ASSERT( num_bytes <= 4096, "Suspiciously large texture load: %d bytes (%dx%d, %dbpp)", num_bytes, tile_width, tile_height, (1<<(rdp_tile.size+2)) );

		// May not work if Left (10.2 format) is not even?
		DAEDALUS_DL_ASSERT( (rdp_tile.size > 0) || (rdp_tilesize.left & 4) == 0, "Expecting an even Left for 4bpp formats" );
#endif

#ifdef DAEDALUS_FAST_TMEM
		//If indexed TMEM PAL address is NULL then assume that the base address is stored in
		//TMEM address 0x100 (gTlutLoadAddresses[ 0 ]) and calculate offset from there with TLutIndex(palette index)
		//This trick saves us from the need to copy the real palette to TMEM and we just pass the pointer //Corn
		//
		if(rdp_tile.size == G_IM_SIZ_4b)
		{
			u32 tlut_idx0( g_ROM.TLUT_HACK << 1 );
			u32 tlut_idx1( (u32)gTlutLoadAddresses[ rdp_tile.palette << tlut_idx0 ] );

			//If pointer == NULL(=invalid entry) add offset to base address (TMEM[0] + offset)
			if(tlut_idx1 == 0)
			{
				tlut += (rdp_tile.palette << (5 + tlut_idx0) );
			}
			else
			{
				tlut = tlut_idx1;
			}
		}
		ti.SetTlutAddress( tlut );
#else
		ti.SetTlutAddress( rdp_tile.size == G_IM_SIZ_4b ? tlut + (rdp_tile.palette << 5) : tlut );
#endif

		ti.SetTmemAddress( rdp_tile.tmem );
		ti.SetLoadAddress( address );
		ti.SetTLutIndex( rdp_tile.palette );
		ti.SetFormat( rdp_tile.format );
		ti.SetSize( rdp_tile.size );

		// Hack to fix the sun in Zelda OOT/MM
		if( g_ROM.ZELDA_HACK && (gRDPOtherMode.L == 0x0c184241) && (rdp_tile.format == G_IM_FMT_I) )	 //&& (ti.GetWidth() == 64)
		{
			tile_width >>= 1;
			pitch >>= 1;
		}

		ti.SetWidth( tile_width );
		ti.SetHeight( tile_height );
		ti.SetPitch( pitch );
		ti.SetTLutFormat( (ETLutFmt)gRDPOtherMode.text_tlut );
		ti.SetSwapped( swapped );
		ti.SetMirrorS( rdp_tile.mirror_s );
		ti.SetMirrorT( rdp_tile.mirror_t );

		// Hack - Extreme-G specifies RGBA/8 textures, but they're really CI8
		if( ti.GetFormat() == G_IM_FMT_RGBA && ti.GetSize() <= G_IM_SIZ_8b ) ti.SetFormat( G_IM_FMT_CI );

		// Force RGBA
		if( ti.GetFormat() == G_IM_FMT_CI && ti.GetTLutFormat() == kTT_NONE ) ti.SetTLutFormat( kTT_RGBA16 );

		mTileTextureInfoValid[ idx ] = true;
	}

	return mTileTextureInfo[ idx ];
}
