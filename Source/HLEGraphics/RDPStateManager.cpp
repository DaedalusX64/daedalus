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
#include "DebugDisplayList.h"

#include "Math/MathUtil.h"

#include "OSHLE/ultra_gbi.h"


extern SImageDescriptor g_TI;		//Texture data from Timg ucode

//*****************************************************************************
//
//*****************************************************************************
CRDPStateManager::CRDPStateManager()
{
	memset( mTMEM_Load, 0, sizeof(mTMEM_Load) );
	InvalidateAllTileTextureInfo();
}

//*****************************************************************************
//
//*****************************************************************************
CRDPStateManager::~CRDPStateManager()
{
}

//*****************************************************************************
//
//*****************************************************************************
void CRDPStateManager::Reset()
{
	memset( mTMEM_Load, 0, sizeof(mTMEM_Load) );
	InvalidateAllTileTextureInfo();
}

//*****************************************************************************
//
//*****************************************************************************
void	CRDPStateManager::SetTile( const RDP_Tile & tile )
{
	u32 idx( tile.tile_idx );

	if( mTiles[ idx ] != tile )
	{
		mTiles[ idx ] = tile;
		mTileTextureInfoValid[ idx ] = false;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void	CRDPStateManager::SetTileSize( const RDP_TileSize & tile_size )
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

//*****************************************************************************
//
//*****************************************************************************
void	CRDPStateManager::LoadBlock( u32 idx, u32 address, bool swapped )
{
	InvalidateAllTileTextureInfo();		// Can potentially invalidate all texture infos

	u32	tmem_lookup( mTiles[ idx ].tmem >> 4 );

	//Invalidate load info from current TMEM address to the end of TMEM (fixes Fzero and SSV) //Corn
	for( u32 i = tmem_lookup; i < 32; ++i )
	{
		mTMEM_Load[ i ].Valid = false;
	}
		
	mTMEM_Load[ tmem_lookup ].Address = address;
	mTMEM_Load[ tmem_lookup ].Pitch = ~0;
	mTMEM_Load[ tmem_lookup ].Swapped = swapped;
	mTMEM_Load[ tmem_lookup ].Valid = true;
}

//*****************************************************************************
//
//*****************************************************************************
void	CRDPStateManager::LoadTile( u32 idx, u32 address )
{
	InvalidateAllTileTextureInfo();		// Can potentially invalidate all texture infos

	u32	tmem_lookup( mTiles[ idx ].tmem >> 4 );

	mTMEM_Load[ tmem_lookup ].Address = address;
	mTMEM_Load[ tmem_lookup ].Pitch = g_TI.GetPitch();
	mTMEM_Load[ tmem_lookup ].Swapped = false;
	mTMEM_Load[ tmem_lookup ].Valid = true;
}
//*****************************************************************************
//
//*****************************************************************************
/*void	CRDPStateManager::LoadTlut( u32 idx, u32 address )
{
	InvalidateAllTileTextureInfo();		// Can potentially invalidate all texture infos

	u32	tmem_lookup( mTiles[ idx ].tmem >> 4 );
	
	mTMEM_Load[ tmem_lookup ].Address = address;
	mTMEM_Load[ tmem_lookup ].Pitch = g_TI.GetPitch();
	mTMEM_Load[ tmem_lookup ].Swapped = false;
	mTMEM_Load[ tmem_lookup ].Valid = true;
}*/
//*****************************************************************************
//
//*****************************************************************************
void	CRDPStateManager::InvalidateAllTileTextureInfo()
{
	for( u32 i = 0; i < 8; ++i )
	{
		mTileTextureInfoValid[ i ] = false;
	}
}

namespace
{
	//
	//	Limit the tile's width/height to the number of bits specified by mask_s/t.
	//	See the detailed noted in PSPRenderer::EnableTexturing for issues relating to this.
	//
	u16		GetTextureDimension( u16 tile_dimension, u8 mask )
	{
		if( mask != 0 )
		{
			return Min< u16 >( 1 << mask, tile_dimension );
		}

		return tile_dimension;
	}
}

//*****************************************************************************
//
//*****************************************************************************
const TextureInfo & CRDPStateManager::GetTextureDescriptor( u32 idx ) const
{
	DAEDALUS_ASSERT( idx < ARRAYSIZE( mTileTextureInfoValid ), "Invalid index %d", idx );
	if( !mTileTextureInfoValid[ idx ] )
	{
		TextureInfo &			ti( mTileTextureInfo[ idx ] );

		const RDP_Tile &		rdp_tile( mTiles[ idx ] );
		const RDP_TileSize &	rdp_tilesize( mTileSizes[ idx ] );
		u32						tmem_lookup( rdp_tile.tmem >> 4 );

		u32		address( mTMEM_Load[ tmem_lookup ].Address );
		u32		pitch( mTMEM_Load[ tmem_lookup ].Pitch );
		bool	swapped( mTMEM_Load[ tmem_lookup ].Swapped );

		if(	!mTMEM_Load[ tmem_lookup ].Valid )
		{
			//If we can't find the load details on current tile TMEM address we assume load was done on TMEM address 0 //Corn
			//
			address = mTMEM_Load[ 0 ].Address + (rdp_tile.tmem << 3);	//Calculate offset in bytes and add to base address
			pitch = mTMEM_Load[ 0 ].Pitch;
			swapped = mTMEM_Load[ 0 ].Swapped;
		}

		// If it was a block load - the pitch is determined by the tile size
		// else if it was a tile - the pitch is set when the tile is loaded
		if ( pitch == u32(~0) )
		{
			if( rdp_tile.size == G_IM_SIZ_32b )	pitch = rdp_tile.line << 4;
			else pitch = rdp_tile.line << 3;
		}

		//	Limit the tile's width/height to the number of bits specified by mask_s/t.
		//	See the detailed notes in PSPRenderer::EnableTexturing for issues relating to this.
		//
		u16		tile_width( GetTextureDimension( rdp_tilesize.GetWidth(), rdp_tile.mask_s ) );
		u16		tile_height( GetTextureDimension( rdp_tilesize.GetHeight(), rdp_tile.mask_t ) );

#ifdef DAEDALUS_ENABLE_ASSERTS
		u32		num_pixels( tile_width * tile_height );
		u32		num_bytes( pixels2bytes( num_pixels, rdp_tile.size ) );
		DAEDALUS_DL_ASSERT( num_bytes <= 4096, "Suspiciously large texture load: %d bytes (%dx%d, %dbpp)", num_bytes, tile_width, tile_height, (1<<(rdp_tile.size+2)) );
#endif

#ifndef DAEDALUS_TMEM
		//If indexed TMEM PAL address is NULL then assume that the base address is stored in
		//TMEM address 0x100 (gTextureMemory[ 0 ]) and calculate offset from there with TLutIndex(palette index)
		//This trick saves us from the need to copy the real palette to TMEM and we just pass the pointer //Corn
		//
		if(rdp_tile.size == G_IM_SIZ_4b)
		{
			if ( g_ROM.TLUT_HACK )
			{
				if(gTextureMemory[ rdp_tile.palette << 2 ] == NULL)
				{
					ti.SetTlutAddress( (u32)gTextureMemory[ 0 ] + (rdp_tile.palette << 7) );
				}
				else
				{
					ti.SetTlutAddress( (u32)gTextureMemory[ rdp_tile.palette << 2] );
				}
			}
			else
			{
				if(gTextureMemory[ rdp_tile.palette << 0 ] == NULL)
				{
					ti.SetTlutAddress( (u32)gTextureMemory[ 0 ] + (rdp_tile.palette << 5) );
				}
				else
				{
					ti.SetTlutAddress( (u32)gTextureMemory[ rdp_tile.palette << 0] );
				}
			}
		}
		else
		{	//Force index 0 for all but 4b palettes
			ti.SetTlutAddress( (u32)gTextureMemory[ 0 ] );
		}

#else
		// Proper way, doesn't need Harvest Moon hack, Nb. 4b check is for Majora's Mask
		u32 tlut( (u32)(&gTextureMemory[0]) );
		ti.SetTLutIndex( rdp_tile.palette ); 
		ti.SetTlutAddress( rdp_tile.size == G_IM_SIZ_4b ? tlut + (rdp_tile.palette << 5) : tlut );
#endif

		ti.SetTmemAddress( rdp_tile.tmem );
		ti.SetLoadAddress( address );
		ti.SetFormat( rdp_tile.format );
		ti.SetSize( rdp_tile.size );

		// May not work if Left is not even?
#ifdef DAEDALUS_ENABLE_ASSERTS
		u32	tile_left( rdp_tilesize.left >> 2 );
		DAEDALUS_DL_ASSERT( (rdp_tile.size > 0) || (tile_left&1) == 0, "Expecting an even Left for 4bpp formats" );
#endif
		ti.SetWidth( tile_width );
		ti.SetHeight( tile_height );
		ti.SetPitch( pitch );
		ti.SetTLutFormat( gRDPOtherMode.text_tlut << G_MDSFT_TEXTLUT );
		ti.SetTile( idx );
		ti.SetSwapped( swapped );
		ti.SetMirrorS( rdp_tile.mirror_s );
		ti.SetMirrorT( rdp_tile.mirror_t );

		// Hack to fix the sun in Zelda
		//
		if( g_ROM.ZELDA_HACK )
		{
			if(gRDPOtherMode.L == 0x0c184241 && ti.GetFormat() == G_IM_FMT_I /*&& ti.GetWidth() == 64*/)	
			{
				//ti.SetHeight( tile_height );	// (fix me)
				ti.SetWidth( tile_width >> 1 );	
				ti.SetPitch( pitch >> 1 );	
			}
		}
		// Hack - Extreme-G specifies RGBA/8 textures, but they're really CI8
		if( ti.GetFormat() == G_IM_FMT_RGBA && ti.GetSize() <= G_IM_SIZ_8b ) ti.SetFormat( G_IM_FMT_CI );

		// Force RGBA
		if( ti.GetFormat() == G_IM_FMT_CI && ti.GetTLutFormat() == G_TT_NONE ) ti.SetTLutFormat( G_TT_RGBA16 );

		mTileTextureInfoValid[ idx ] = true;
	}

	return mTileTextureInfo[ idx ];
}

//*****************************************************************************
//
//*****************************************************************************
// Effectively a singleton...needs refactoring
CRDPStateManager		gRDPStateManager;

