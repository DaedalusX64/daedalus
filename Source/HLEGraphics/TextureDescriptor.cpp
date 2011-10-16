/*
Copyright (C) 2001,2007 StrmnNrmn

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

#include "TextureDescriptor.h"
#include "OSHLE/ultra_gbi.h"

#include "RDP.h"
#include "ConfigOptions.h"

#include "Core/Memory.h"

#include "Utility/Profiler.h"
#include "Utility/Hash.h"

#include "Core/ROM.h"

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
//*************************************************************************************
//
//*************************************************************************************
namespace
{
const char * const	pszImgFormat[8] = {"RGBA", "YUV", "CI", "IA", "I", "?1", "?2", "?3"};
const u32			pnImgSize[4]   = {4, 8, 16, 32};
}

//*************************************************************************************
//
//*************************************************************************************
const char * TextureInfo::GetFormatName() const
{
	return pszImgFormat[ Format ];
}

//*************************************************************************************
//
//*************************************************************************************
u32 TextureInfo::GetSizeInBits() const	
{
	return pnImgSize[ Size ];
}
#endif
//*************************************************************************************
//
//*************************************************************************************
void TextureInfo::SetTLutFormat( u32 format )
{
	TLutFmt = format >> G_MDSFT_TEXTLUT;
}

//*************************************************************************************
//
//*************************************************************************************
u32	TextureInfo::GetTLutFormat() const
{
	return TLutFmt << G_MDSFT_TEXTLUT;
}

//*************************************************************************************
//
//*************************************************************************************
const void *	TextureInfo::GetPalettePtr() const
{
	// Want to advance 16x16bpp palette entries in TMEM(i.e. 32 bytes into tmem for each palette), i.e. <<5.
	// some games uses <<7 like in MM but breaks Aerogauge //Corn
#ifndef DAEDALUS_TMEM
	//printf("%d\n",TLutIndex);
	//for(u32 i=0;i<0x100;i++) printf("%p ", gTextureMemory[ i ]);
	//printf("\n\n");

	if ( gTLUTalt_mode )
	{
		if(gTextureMemory[ TLutIndex << 4 ] == NULL)
		{	//If TMEM PAL address is NULL then assume that the base address is stored in
			//TMEM address 0x100 and calculate offset from there with TLutIndex
			//If this also returns NULL then return bogus non NULL address to avoid BSOD (Flying Dragon) //Corn
			if((void *)((u32)gTextureMemory[ 0 ] + ( TLutIndex << 7 )) == NULL) return (void *)g_pu8RamBase;
			else return (void *)((u32)gTextureMemory[ 0 ] + ( TLutIndex << 7 ));
		}
		else return (void *)gTextureMemory[ TLutIndex << 4 ];
	}
	else
	{
		if(gTextureMemory[ TLutIndex << 2 ] == NULL)
		{	//If TMEM PAL address is NULL then assume that the base address is stored in
			//TMEM address 0x100 and calculate offset from there with TLutIndex
			//If this also returns NULL then return bogus non NULL address to avoid BSOD (Extreme-G) //Corn
			if((void *)((u32)gTextureMemory[ 0 ] + ( TLutIndex << 5 )) == NULL)	return (void *)g_pu8RamBase;
			else return (void *)((u32)gTextureMemory[ 0 ] + ( TLutIndex << 5 ));
		}
		else return (void *)gTextureMemory[ TLutIndex << 2 ];
	}
#else

	return (void *)&gTextureMemory[ 0x200 + (TLutIndex << (gTLUTalt_mode? 5 : 3)) ];
#endif
}

//*************************************************************************************
//
//*************************************************************************************
u32	TextureInfo::GetWidthInBytes() const
{
	return pixels2bytes( Width, Size );
}

//*************************************************************************************
//
//*************************************************************************************
#if 1 //1->new hash(fast), 0-> old hash(expensive)
u32 TextureInfo::GenerateHashValue() const
{
	//Rewritten to use less recources //Corn
	//Number of places to do fragment hash from in texture
	//More rows will use more CPU...
	const u32 CHK_ROW = 5;

	DAEDALUS_PROFILE( "TextureInfo::GenerateHashValue" );
	
	// If CRC checking is disabled, always return 0
	if ( gCheckTextureHashFrequency == 0 ) return 0;

	//DAEDALUS_ASSERT( (GetLoadAddress() + Height * Pitch) < 4*1024*1024, "Address of texture is out of bounds" );

	u8 *ptr_b = g_pu8RamBase + GetLoadAddress();
	u32 hash_value = 0;

	//u32 step = Height * Width * (1<<Size) >> 1;	//Get size in bytes
	u32 step = Height * Pitch;	//Get size in bytes, seems to be more accurate

	if((u32)ptr_b & 0x3)	//Check if aligned to 4 bytes if not then align
	{
		ptr_b += 4 - ((u32)ptr_b & 0x3);
		step  -= 4 - ((u32)ptr_b & 0x3);
	}

	u32 *ptr = (u32*)ptr_b;	//use 32bit access
	step = step >> 2;	//convert to 32bit access

	//We want to sample the texture data as far apart as possible
	if (step < (CHK_ROW * 4))	//if texture is small hash all of it
	{
		for (u32 z = 0; z < step; z++) hash_value = ((hash_value << 1) | (hash_value >> 0x1F)) ^ ptr[z];
	}
	else	//if texture is big, hash only some parts inside it
	{
		step = (step - 4) / CHK_ROW;
		for (u32 y = 0; y < CHK_ROW; y++)
		{
			for (u32 z = 0; z < 4; z++)	hash_value = ((hash_value << 1) | (hash_value >> 0x1F)) ^ ptr[z];
			ptr += step;
		}
	}
	
	//If texture has a palette then make hash of that too
	//Might not be needed but it would catch if only the colors are changed in a palette texture
	//It is a bit expensive CPU wise so better leave out unless really needed
	//It assumes 4 byte alignment so we use u32 (faster than u8)
	//Used in OOT for the sky, really minor so is not worth the CPU time to always check for it
	/*if (GetFormat() == G_IM_FMT_CI)  
	{
		const u32* ptr = reinterpret_cast< const u32 * >( GetPalettePtr() );
		if ( GetSize() == G_IM_SIZ_4b )	for (u32 z = 0; z < 16; z++) hash_value ^= *ptr++;
		else							for (u32 z = 0; z < 256; z++) hash_value ^= *ptr++;
	}*/

	//printf("%08X %d S%d P%d H%d W%d B%d\n", hash_value, step, Size, Pitch, Height, Width, Height * Pitch);
	return hash_value;
}

#else
u32 TextureInfo::GenerateHashValue() const
{
	DAEDALUS_PROFILE( "TextureInfo::GenerateHashValue" );

	// If CRC checking is disabled, always return 0
	if ( gCheckTextureHashFrequency == 0 )
		return 0;
	
	u32 bytes_per_line( GetWidthInBytes() );

	//DBGConsole_Msg(0, "BytesPerLine: %d", bytes_per_line);
	
	// A very simple crc - just summation
	u32 hash_value( 0 );

	//DAEDALUS_ASSERT( (GetLoadAddress() + Height * Pitch) < 4*1024*1024, "Address of texture is out of bounds" );

	const u8 * p_bytes( g_pu8RamBase + GetLoadAddress() );
	u32 step;
	if (Height > 4)
	{
		step = (Height/2)-1;
	}else{
		step = 1;
	}

	for (u32 y = 0; y < Height; y+=step)		// Hash 3 Lines per texture
	{
		// Byte fiddling won't work, but this probably doesn't matter
		hash_value = murmur2_neutral_hash( p_bytes, bytes_per_line, hash_value );
		p_bytes += (Pitch * step);
	}

	if (GetFormat() == G_IM_FMT_CI)
	{
		u32 bytes;
		if ( GetSize() == G_IM_SIZ_4b )	bytes = 16  * 4;
		else							bytes = 256 * 4;

		p_bytes = reinterpret_cast< const u8 * >( GetPalettePtr() );
		hash_value = murmur2_neutral_hash( p_bytes, bytes, hash_value );
	}

	return hash_value;
}
#endif
//*************************************************************************************
//
//*************************************************************************************
ETextureFormat	TextureInfo::SelectNativeFormat() const
{
	switch (Format)
	{
	case G_IM_FMT_RGBA:
		switch (Size)
		{
		case G_IM_SIZ_16b:
			return TexFmt_5551;
		case G_IM_SIZ_32b:
			return TexFmt_8888;
		}
		break;
		
	case G_IM_FMT_YUV:
		break;

	case G_IM_FMT_CI:
		switch (Size)
		{
		case G_IM_SIZ_4b: // 4bpp
			switch (GetTLutFormat())
			{
			case G_TT_RGBA16:
				return TexFmt_CI4_8888;
			case G_TT_IA16:
				return TexFmt_CI4_8888;
			}
			break;
			
		case G_IM_SIZ_8b: // 8bpp
			switch(GetTLutFormat())
			{
			case G_TT_RGBA16:
				return TexFmt_CI8_8888;
			case G_TT_IA16:
				return TexFmt_CI8_8888;
			}
			break;
		}
		
		break;

	case G_IM_FMT_IA:
		switch (Size)
		{
		case G_IM_SIZ_4b:
			return TexFmt_4444;
		case G_IM_SIZ_8b:
			return TexFmt_4444;
		case G_IM_SIZ_16b:
			return TexFmt_8888;
		}
		break;

	case G_IM_FMT_I:
		switch (Size)
		{
		case G_IM_SIZ_4b:
			return TexFmt_4444;
		case G_IM_SIZ_8b:
			return TexFmt_8888;
		}
		break;
	}

	// Unhandled!
	DAEDALUS_ERROR( "Unhandled texture format" );
	return TexFmt_8888;

}
