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
#include "DLDebug.h"

#include "Core/Memory.h"
#include "Core/ROM.h"
#include "Debug/DBGConsole.h"
#include "HLEGraphics/uCodes/UcodeDefs.h"
#include "Math/MathUtil.h"
#include "OSHLE/ultra_gbi.h"
#include "Utility/Alignment.h"
#include "Utility/Endian.h"
#include "Utility/FastMemcpy.h"
#include "Utility/Macros.h"

extern SImageDescriptor g_TI;		//Texture data from Timg ucode

CRDPStateManager gRDPStateManager;

static const char * const kTLUTTypeName[] = {"None", "?", "RGBA16", "IA16"};

RDP_OtherMode		gRDPOtherMode;

#define MAX_TMEM_ADDRESS 4096

//Granularity down to 24bytes is good enuff also only need to address the upper half of TMEM for palettes//Corn
u32* gTlutLoadAddresses[ MAX_TMEM_ADDRESS >> 6 ];


#ifdef DAEDALUS_ACCURATE_TMEM
ALIGNED_GLOBAL(u8, gTMEM[ MAX_TMEM_ADDRESS ], 16);	// 4Kb
#endif


#ifdef DAEDALUS_ACCURATE_TMEM

// CopyLineQwords** passes in an offset in 32bit words, (i.e. does mem_address/4)
// So it's aligned by definition, we have asserts in place just in case!

// Due to how TMEM is organized, erg the last 3 bits in the address are always "0"
// It should be safe to assume copies will always be in qwords
#define FAST_TMEM_COPY

static inline void CopyLineQwords(void * dst, const void * src, u32 qwords)
{
#ifdef FAST_TMEM_COPY
	u32* src32 = (u32*)src;
	u32* dst32 = (u32*)dst;

	DAEDALUS_ASSERT( ((uintptr_t)src32&0x3)==0, "src is not aligned!");

	while(qwords--)
	{
		dst32[0] = BSWAP32(src32[0]);
		dst32[1] = BSWAP32(src32[1]);
		dst32 += 2;
		src32 += 2;
	}
#else
	u8* src8 = (u8*)src;
	u8* dst8 = (u8*)dst;
	u32 bytes = qwords * 8;
	while(bytes--)
	{
		*dst8++ = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
	}
#endif
}

static void CopyLineQwordsSwap(void * dst, const void * src, u32 qwords)
{
#ifdef FAST_TMEM_COPY
	u32* src32 {(u32*)src};
	u32* dst32 {(u32*)dst};

	DAEDALUS_ASSERT( ((uintptr_t)src32&0x3 )==0, "src is not aligned!");

	while(qwords--)
	{
		dst32[1]  = BSWAP32(src32[0]);
		dst32[0]  = BSWAP32(src32[1]);
		dst32 += 2;
		src32 += 2;
	}
#else
	u8* src8 {(u8*)src};
	u8* dst8 {(u8*)dst};
	u32 bytes {qwords * 8};
	while(bytes--)
	{
		*(u8*)((uintptr_t)dst8++ ^ 0x4)  = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
	}
#endif
}

static void CopyLineQwordsSwap32(void * dst, const void * src, u32 qwords)
{
#ifdef FAST_TMEM_COPY
	u32* src32 {(u32*)src};
	u32* dst32 {(u32*)dst};

	DAEDALUS_ASSERT( ((uintptr_t)src32&0x3 )==0, "src is not aligned!");

	u32 size128 {qwords >>1};

	while(size128--)
	{
		dst32[2]  = BSWAP32(src32[0]);
		dst32[3]  = BSWAP32(src32[1]);
		dst32[0]  = BSWAP32(src32[2]);
		dst32[1]  = BSWAP32(src32[3]);
		dst32 += 4;
		src32 += 4;
	}

	// Copy any remaining quadword
	qwords&=0x1;
	while(qwords--)
	{
		*(u32*)((uintptr_t)dst32++ ^ 0x8) = BSWAP32(src32[0]);
		*(u32*)((uintptr_t)dst32++ ^ 0x8) = BSWAP32(src32[1]);
		src32+=2;
	}
#else

	u8* src8 {(u8*)src};
	u8* dst8 {(u8*)dst};
	u32 bytes {qwords * 8};
	while(bytes--)
	{
		*(u8*)((uintptr_t)dst8++ ^ 0x8)  = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
	}
#endif
}

static inline void CopyLine(void * dst, const void * src, u32 bytes)
{
#ifdef FAST_TMEM_COPY
	u32* src32 {(u32*)src};
	u32* dst32 {(u32*)dst};

	DAEDALUS_ASSERT((bytes&0x3)==0, "CopyLine: Remaning bytes! (%d)",bytes);

	u32 size32 {bytes >> 2};
	u32 src_alignment {(uintptr_t)src32&0x3};

	if(src_alignment == 0)
	{
		while (size32--)
		{
			*dst32++ = BSWAP32(src32[0]);
			src32++;
		}
	}
	else
	{
		// Zelda and DK64 have unaligned copies. so let's optimize 'em
		src32 = (u32*)((uintptr_t)src & ~0x3);
		u32 src_tmp {*src32++};
		u32 dst_tmp {};

		// calculate offset 3..1..2
		u32 offset {4-src_alignment};
		u32 lshift {src_alignment<<3};
		u32 rshift {offset<<3};

		while(size32--)
		{
			dst_tmp = src_tmp << lshift;
			src_tmp = *src32++;
			dst_tmp|= src_tmp >> rshift;
			*dst32++ = BSWAP32(dst_tmp);
		}
		src32 -= offset;
	}
#else
	u8* src8 {(u8*)src};
	u8* dst8 {(u8*)dst};
	while(bytes--)
	{
		*dst8++ = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
	}
#endif
}

static inline void CopyLine16(u16 * dst16, const u16 * src16, u32 words)
{
#ifdef FAST_TMEM_COPY
	DAEDALUS_ASSERT( ((uintptr_t)src16&0x1 )==0, "src is not aligned!");

	u32 dwords {words >> 1};
	while(dwords--)
	{
		dst16[0] = src16[1];
		dst16[4] = src16[0];
		dst16+=8;
		src16+=2;
	}

	// Copy any remaining word
	words&= 0x1;
#endif
	while(words--)
	{
		*dst16 = *(u16*)((uintptr_t)src16++ ^ U16_TWIDDLE);
		dst16+=4;
	}
}

static inline void CopyLineSwap(void * dst, const void * src, u32 bytes)
{
	u32* src32 {(u32*)src};
	u32* dst32 {(u32*)dst};

#ifdef FAST_TMEM_COPY
	DAEDALUS_ASSERT((bytes&0x7)==0, "CopyLineSwap: Remaning bytes! (%d)",bytes);

	if( ((uintptr_t)src32&0x3 )==0)
	{
		u32 size64 {bytes >> 3};

		while (size64--)
		{
			dst32[0] = BSWAP32(src32[1]);
			dst32[1] = BSWAP32(src32[0]);
			dst32 += 2;
			src32 += 2;
		}
	}
	else
#endif
	{
		// Optimize me: Bomberman, Zelda, and Quest 64 have unaligned copies here
		//DBGConsole_Msg(0, "[WWarning CopyLineSwap: Performing slow copy]" );

		u8* src8 {(u8*)src32};
		u8* dst8 {(u8*)dst32};
		while(bytes--)
		{
			// Alternate 32 bit words are swapped
			*(u8*)((uintptr_t)dst8++ ^ 0x4) = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
		}
	}
}

static inline void CopyLineSwap32(void * dst, const void * src, u32 bytes)
{
	u32* src32 {(u32*)(src)};
	u32* dst32 {(u32*)(dst)};

#ifdef FAST_TMEM_COPY
	DAEDALUS_ASSERT((bytes&0x7)==0, "CopyLineSwap32: Remaning bytes! (%d)",bytes);

	if( ((uintptr_t)src32&0x3 )==0)
	{
		u32 size128 {bytes >> 4};

		while (size128--)
		{
			dst32[0] = BSWAP32(src32[2]);
			dst32[1] = BSWAP32(src32[3]);
			dst32[2] = BSWAP32(src32[0]);
			dst32[3] = BSWAP32(src32[1]);
			dst32 += 4;
			src32 += 4;
		}

		// Copy any remaining quadword
		bytes&=0xF;
		while(bytes--)
		{
			*(u32*)((uintptr_t)dst32++ ^ 0x8) = BSWAP32(src32[0]);
			*(u32*)((uintptr_t)dst32++ ^ 0x8) = BSWAP32(src32[1]);
			src32+=2;
		}
	}
	else
#endif
	{
		// Have yet to see game with unaligned copies here
		//DBGConsole_Msg(0, "[WWarning CopyLineSwap32: Performing slow copy]" );

		u8* src8 {(u8*)src32};
		u8* dst8 {(u8*)dst32};
		while(bytes--)
		{
			// Alternate 64 bit words are swapped
			*(u8*)((uintptr_t)dst8++ ^ 0x8) = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
		}
	}
}
#endif


CRDPStateManager::CRDPStateManager()
:	EmulateMirror(true)
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

	memset(mTiles, 0, sizeof(mTiles));
	memset(mTileSizes, 0, sizeof(mTileSizes));
	memset(mTileTextureInfo, 0, sizeof(mTileTextureInfo));
}

void CRDPStateManager::SetTile( const RDP_Tile & tile )
{
	u32 idx {tile.tile_idx};

	if( mTiles[ idx ] != tile )
	{
		mTiles[ idx ] = tile;
		mTileTextureInfoValid[ idx ] = false;
	}
}

void CRDPStateManager::SetTileSize( const RDP_TileSize & tile_size )
{
	u32 idx {tile_size.tile_idx};

	if( mTileSizes[ idx ] != tile_size )
	{
		mTileSizes[ idx ] = tile_size;
		mTileTextureInfoValid[ idx ] = false;
	}
}

void CRDPStateManager::LoadBlock(const SetLoadTile & load)
{
	u32 uls			{load.sl};	//Left
	u32 ult			{load.tl};	//Top
	u32 dxt			{load.th};	// 1.11 fixed point
	u32 tile_idx {load.tile};
	u32 address		{g_TI.GetAddress( uls, ult )};
	//u32 ByteSize	= (load.sh + 1) << (g_TI.Size == G_IM_SIZ_32b);

	bool	swapped = (dxt) ? false : true;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	DL_PF("    Tile[%d] (%d,%d - %d) DXT[0x%04x] = [%d]Bytes/line => [%d]Pixels/line Address[0x%08x]",
		tile_idx,
		uls, ult,
		load.sh,
		dxt,
		(g_TI.Width << g_TI.Size >> 1),
		bytes2pixels( (g_TI.Width << g_TI.Size >> 1), g_TI.Size ),
		address);
#endif

	InvalidateAllTileTextureInfo();		// Can potentially invalidate all texture infos

	const RDP_Tile & rdp_tile {mTiles[tile_idx]};

	u32	tmem_lookup {(u32)(rdp_tile.tmem >> 4)};

	//Invalidate load info after current TMEM address to the end of TMEM (fixes Fzero and SSV) //Corn
	ClearEntries( tmem_lookup );
	SetValidEntry( tmem_lookup );

	TimgLoadDetails & info = mTmemLoadInfo[ tmem_lookup ];
	info.Address = address;
	info.Pitch	 = ~0;
	info.Swapped = swapped;


#ifdef DAEDALUS_ACCURATE_TMEM
	u32 lrs    {load.sh};
	u32 bytes  {((lrs+1) << g_TI.Size) >> 1};

	DAEDALUS_DL_ASSERT( bytes <= 4096, "Suspiciously large loadblock: %d bytes", bytes );
	DAEDALUS_DL_ASSERT( bytes, "LoadBLock: No bytes??" );

	u32 qwords {(bytes+7) / 8};
	u32 tmem_offset {(rdp_tile.tmem << 3)};
	u32 ram_offset  {address};

	if (( (address + bytes) > MAX_RAM_ADDRESS) || (tmem_offset + bytes) > MAX_TMEM_ADDRESS )
	{
		DBGConsole_Msg(0, "[WWarning LoadBlock address is invalid]" );
		return;
	}

	u32* dst {(u32*)(gTMEM + tmem_offset)};
	u32* src {(u32*)(g_pu8RamBase + ram_offset)};

	if (dxt == 0)
	{
		CopyLineQwords(dst, src, qwords);
	}
	else
	{
		void (*CopyLineQwordsMode)(void*, const void*, u32);

		if(g_TI.Size == G_IM_SIZ_32b)
			CopyLineQwordsMode = CopyLineQwordsSwap32;
		else
			CopyLineQwordsMode = CopyLineQwordsSwap;

		u32 qwords_per_line {(2048 + dxt-1) / dxt};

		DAEDALUS_ASSERT(qwords_per_line == (u32)ceilf(2048.f / (float)dxt), "Broken DXT calc");

		u32 odd_row {};
		for (u32 i {}; i < qwords; /* updated in loop */)
		{
			u32 qwords_to_copy {Min(qwords-i, qwords_per_line)};

			if (odd_row)
			{
				CopyLineQwordsMode(dst, src, qwords_to_copy);
			}
			else
			{
				CopyLineQwords(dst, src, qwords_to_copy);
			}

			i  += qwords_to_copy;
			qwords_to_copy *= 2;				// 2 32bit words per qword
			dst			+= qwords_to_copy;
			src			+= qwords_to_copy;
			odd_row     ^= 0x1;					// Odd lines are word swapped
		}
	}

	//InvalidateTileHashes();
#endif // DAEDALUS_ACCURATE_TMEM
}

void CRDPStateManager::LoadTile(const SetLoadTile & load)
{
	u32 uls      {load.sl};	//Left
	u32 ult      {load.tl};	//Top
	u32 tile_idx {load.tile};
	u32 address  {g_TI.GetAddress( uls / 4, ult / 4 )};
	u32 pitch    {g_TI.GetPitch()};

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	DL_PF("    Tile[%d] (%d,%d)->(%d,%d) [%d x %d] Address[0x%08x]",
		tile_idx,
		load.sl / 4, load.tl / 4, load.sh / 4 + 1, load.th / 4 + 1,
		(load.sh - load.sl) / 4 + 1, (load.th - load.tl) / 4 + 1,
		address);
#endif
	InvalidateAllTileTextureInfo();		// Can potentially invalidate all texture infos

	const RDP_Tile & rdp_tile = mTiles[tile_idx];

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	DAEDALUS_DL_ASSERT( (rdp_tile.size > 0) || (uls & 4) == 0, "Expecting an even Left for 4bpp formats (left is %f)", uls / 4.f );
#endif
	u32	tmem_lookup {(u32)(rdp_tile.tmem >> 4)};
	SetValidEntry( tmem_lookup );

	TimgLoadDetails & info = mTmemLoadInfo[ tmem_lookup ];
	info.Address = address;
	info.Pitch = pitch;
	info.Swapped = false;

#ifdef DAEDALUS_ACCURATE_TMEM
	u32 lrs    {load.sh};
	u32 lrt    {load.th};

	u32 ram_address {address;
	u32 h           {((lrt-ult)>>2) + 1};
	u32 w           {((lrs-uls)>>2) + 1};
	u32 bytes       {((h * w) << g_TI.Size) >> 1};

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	DAEDALUS_USE(bytes);
	DAEDALUS_DL_ASSERT( bytes <= MAX_TMEM_ADDRESS,
		"Suspiciously large texture load: %d bytes (%dx%d, %dbpp)",
		bytes, w, h, (1<<(g_TI.Size+2)) );
#endif

	u32  tmem_offset {rdp_tile.tmem << 3};
	u32  ram_offset  {ram_address};
	u32 bytes_per_tmem_line {rdp_tile.line << 3};

	void (*CopyLineMode)(void*, const void*, u32);

	if (g_TI.Size == G_IM_SIZ_32b)
	{
		bytes_per_tmem_line *= 2;
		CopyLineMode = CopyLineSwap32;
	}
	else
	{
		CopyLineMode = CopyLineSwap;
	}

	u32 bytes_to_copy {(bytes_per_tmem_line * h)};
	if ((address + bytes_to_copy) > MAX_RAM_ADDRESS || (tmem_offset + bytes_to_copy) > MAX_TMEM_ADDRESS)
	{
		DBGConsole_Msg(0, "[WWarning LoadTile address is invalid]" );
		return;
	}

	u8* dst {gTMEM + tmem_offset};
	u8* src {g_pu8RamBase + ram_offset};

	for (u32 y {}; y < h; ++y)
	{
		if (y&1)
		{
			CopyLineMode(dst, src, bytes_per_tmem_line);
		}
		else
		{
			CopyLine(dst, src, bytes_per_tmem_line);
		}

		// There might be uninitialised padding bytes here, but we don't care.

		dst += bytes_per_tmem_line;
		src += pitch;
	}

	//InvalidateTileHashes();
#endif // DAEDALUS_ACCURATE_TMEM
}

void CRDPStateManager::LoadTlut(const SetLoadTile & load)
{
	// Tlut fmt is sometimes wrong (in 007) and is set after tlut load, but before tile load
	// Format is always 16bpp - RGBA16 or IA16:
	//DAEDALUS_DL_ASSERT(g_TI.Size == G_IM_SIZ_16b, "Crazy tlut load - not 16bpp");

	u32    uls        {load.sl};		//Left
	u32    ult        {load.tl};		//Top
	u32    lrs        {load.sh};		//Right
	u32	   lrt		  {load.th};	    //Bottom
	u32    tile_idx   {load.tile};
	u32    ram_offset {g_TI.GetAddress16bpp(uls >> 2, ult >> 2)};
	u8*	   address	  {g_pu8RamBase + ram_offset};

	const RDP_Tile & rdp_tile {mTiles[tile_idx]};

	u32 count {((lrs - uls)>>2) + 1};

	#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	DAEDALUS_USE(count);
	DAEDALUS_USE(lrt);
#endif

	//Store address of PAL (assuming PAL is only stored in upper half of TMEM) //Corn
	gTlutLoadAddresses[ (rdp_tile.tmem>>2) & 0x3F ] = (u32*)address;
	#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	DL_PF("    TLut Addr[0x%08x] TMEM[0x%03x] Tile[%d] Count[%d] Format[%s] (%d,%d)->(%d,%d)",
		address, rdp_tile.tmem, tile_idx, count, kTLUTTypeName[gRDPOtherMode.text_tlut], uls >> 2, ult >> 2, lrs >> 2, lrt >> 2);
#endif
#ifdef DAEDALUS_ACCURATE_TMEM
	DAEDALUS_DL_ASSERT( (rdp_tile.tmem + count) <= (MAX_TMEM_ADDRESS/8), "LoadTlut address is invalid" );

	u16* dst {(u16*)(((u64*)gTMEM) + rdp_tile.tmem)};
	u16* src {(u16*)(address)};

	CopyLine16(dst, src, count);
#endif
}

// Limit the tile's width/height to the number of bits specified by mask_s/t.
// See the detailed noted in BaseRenderer::UpdateTileSnapshots for issues relating to this.
static inline u16 GetTextureDimension( u16 tile_dimension, u8 mask, bool clamp )
{
	if (mask)
	{
		u16 mask_dimension {(u16)(1 << mask)};

		// If clamp is enabled, the maximum addressable texel is the
		// smaller of the mask dimension and the tile dimension.
		if (clamp)
		{
			return Min< u16 >( mask_dimension, tile_dimension );
		}

		return mask_dimension;
	}

	return tile_dimension;
}

const TextureInfo & CRDPStateManager::GetUpdatedTextureDescriptor( u32 idx )
{
	DAEDALUS_ASSERT( idx < ARRAYSIZE( mTileTextureInfoValid ), "Invalid index %d", idx );
	if( !mTileTextureInfoValid[ idx ] )
	{
		TextureInfo &			ti           {mTileTextureInfo[ idx ]};
		const RDP_Tile &		rdp_tile     {mTiles[ idx ]};
		const RDP_TileSize &	rdp_tilesize {mTileSizes[ idx ]};
		const u32				tmem_lookup  {(u32)(rdp_tile.tmem >> 4)};
		const TimgLoadDetails &	info  {mTmemLoadInfo[ tmem_lookup ]};

		u32		address {info.Address};
		u32		pitch   {info.Pitch};
		bool	swapped {info.Swapped};

		//Check if tmem_lookup has a valid entry, if not we assume load was done on TMEM[0] and we add the offset //Corn
		//Games that uses this is Fzero/Space station Silicon Valley/Animal crossing.
		if(	EntryIsValid( tmem_lookup ) == 0 )
		{
			const TimgLoadDetails & info_base {mTmemLoadInfo[ 0 ]};

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
			else 								pitch = rdp_tile.line << 3;
		}

		//	Limit the tile's width/height to the number of bits specified by mask_s/t.
		//	See the detailed notes in BaseRenderer::UpdateTileSnapshots for issues relating to this.
		//
		u16		tile_width  {GetTextureDimension( rdp_tilesize.GetWidth(),  rdp_tile.mask_s, rdp_tile.clamp_s )};
		u16		tile_height {GetTextureDimension( rdp_tilesize.GetHeight(), rdp_tile.mask_t, rdp_tile.clamp_t )};

#ifdef DAEDALUS_ACCURATE_TMEM
		ti.SetTlutAddress( TLUT_BASE );
#else
		//
		//If indexed TMEM PAL address is NULL then assume that the base address is stored in
		//TMEM address 0x100 (gTlutLoadAddresses[ 0 ]) and calculate offset from there with TLutIndex(palette index)
		//This trick saves us from the need to copy the real palette to TMEM and we just pass the pointer //Corn
		//
		u32	tlut {TLUT_BASE};
		if(rdp_tile.size == G_IM_SIZ_4b)
		{
			u32 tlut_idx0 {(u32)(g_ROM.TLUT_HACK << 1)};
			u32 tlut_idx1 {(u32)gTlutLoadAddresses[ rdp_tile.palette << tlut_idx0 ]};

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
#endif

#ifdef DAEDALUS_ACCURATE_TMEM
		ti.SetLine( rdp_tile.line );
		// NB: ACCURATE_TMEM doesn't care about pitch - it's already been loaded into tmem.
		// We only care about line.
#endif

		ti.SetTmemAddress( rdp_tile.tmem );
		ti.SetLoadAddress( address );
		ti.SetPalette( rdp_tile.palette );
		ti.SetFormat( rdp_tile.format );
		ti.SetSize( rdp_tile.size );

		ti.SetWidth( tile_width );
		ti.SetHeight( tile_height );
		ti.SetPitch( pitch );
		ti.SetTLutFormat( (ETLutFmt)gRDPOtherMode.text_tlut );
		ti.SetSwapped( swapped );

		ti.SetEmulateMirrorS( EmulateMirror && rdp_tile.mirror_s );
		ti.SetEmulateMirrorT( EmulateMirror && rdp_tile.mirror_t );

		// Hack - Extreme-G specifies RGBA/8 textures, but they're really CI8
		if( ti.GetFormat() == G_IM_FMT_RGBA && ti.GetSize() <= G_IM_SIZ_8b ) ti.SetFormat( G_IM_FMT_CI );

		// Force RGBA
		if( ti.GetFormat() == G_IM_FMT_CI && ti.GetTLutFormat() == kTT_NONE ) ti.SetTLutFormat( kTT_RGBA16 );

		mTileTextureInfoValid[ idx ] = true;
	}

	return mTileTextureInfo[ idx ];
}
