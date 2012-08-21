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
	//Debug Palette pointers
	#if 0 
		printf("0x%02x\n",TLutIndex << (g_ROM.TLUT_HACK? 4:2));
		for(u32 i=0;i<0x40;i+=8) printf("0x%02x -> %08x %08x %08x %08x %08x %08x %08x %08x\n", i<<2,
			(u32)gTextureMemory[ i+0 ], (u32)gTextureMemory[ i+1 ], (u32)gTextureMemory[ i+2 ], (u32)gTextureMemory[ i+3 ],
			(u32)gTextureMemory[ i+4 ], (u32)gTextureMemory[ i+5 ], (u32)gTextureMemory[ i+6 ], (u32)gTextureMemory[ i+7 ]);
		printf("\n\n");
	#endif

	if ( g_ROM.TLUT_HACK )
	{
		if(gTextureMemory[ TLutIndex << 2 ] == NULL)
		{	//If TMEM PAL address is NULL then assume that the base address is stored in
			//TMEM address 0x100 and calculate offset from there with TLutIndex
			//Flying Dragon returns NULL here //Corn
			return (void *)((u32)gTextureMemory[ 0 ] + ( TLutIndex << 7 ));
		}
		else return (void *)gTextureMemory[ TLutIndex << 2 ];
	}
	else
	{
		if(gTextureMemory[ TLutIndex ] == NULL)
		{	//If TMEM PAL address is NULL then assume that the base address is stored in
			//TMEM address 0x100 and calculate offset from there with TLutIndex
			//Extreme-G returns NULL here //Corn
			return (void *)((u32)gTextureMemory[ 0 ] + ( TLutIndex << 5 ));
		}
		else return (void *)gTextureMemory[ TLutIndex ];
	}
#else

	return (void *)&gTextureMemory[ 0x200 + (TLutIndex << (g_ROM.TLUT_HACK ? 5 : 3)) ];
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

	DAEDALUS_PROFILE( "TextureInfo::GenerateHashValue" );
	
	// If CRC checking is disabled, always return 0
	if ( gCheckTextureHashFrequency == 0 ) return 0;

	//DAEDALUS_ASSERT( (GetLoadAddress() + Height * Pitch) < 4*1024*1024, "Address of texture is out of bounds" );

	//Number of places to do fragment hash from in texture
	//More rows will use more CPU...
	u32 CHK_ROW = 5;

	if( g_ROM.GameHacks == YOSHI ) CHK_ROW = 49;

	u8 *ptr_u8 = g_pu8RamBase + GetLoadAddress();
	u32 hash_value = 0;

	//u32 step = Height * Width * (1<<Size) >> 1;	//Get size in bytes
	u32 step = Height * Pitch;	//Get size in bytes, seems to be more accurate

	if((u32)ptr_u8 & 0x3)	//Check if aligned to 4 bytes if not then align
	{
		ptr_u8 += 4 - ((u32)ptr_u8 & 0x3);
		step  -= 4 - ((u32)ptr_u8 & 0x3);
	}

	u32 *ptr_u32 = (u32*)ptr_u8;	//use 32bit access
	step = step >> 2;	//convert to 32bit access

	//We want to sample the texture data as far apart as possible
	if (step < (CHK_ROW << 2))	//if texture is small hash all of it
	{
		for (u32 z = 0; z < step; z++) hash_value = ((hash_value << 1) | (hash_value >> 0x1F)) ^ ptr_u32[z];
	}
	else	//if texture is big, hash only some parts inside it
	{
		step = (step - 4) / CHK_ROW;
		for (u32 y = 0; y < CHK_ROW; y++)
		{
			hash_value = ((hash_value << 1) | (hash_value >> 0x1F)) ^ ptr_u32[0];
			hash_value = ((hash_value << 1) | (hash_value >> 0x1F)) ^ ptr_u32[1];
			hash_value = ((hash_value << 1) | (hash_value >> 0x1F)) ^ ptr_u32[2];
			hash_value = ((hash_value << 1) | (hash_value >> 0x1F)) ^ ptr_u32[3];
			ptr_u32 += step;
		}
	}
	
	//If texture has a palette then make hash of that too
	//Might not be needed but it would catch if only the colors are changed in a palette texture
	//It is a bit expensive CPU wise so better leave out unless really needed
	//It assumes 4 byte alignment so we use u32 (faster than u8)
	//Used in OOT for the sky, really minor so is not worth the CPU time to always check for it
	/*if (GetFormat() == G_IM_FMT_CI)  
	{
		const u32* ptr_u32 = (u32*)GetPalettePtr();
		for (u32 z = 0; z < ((GetSize() == G_IM_SIZ_4b)? 16 : 256); z++) hash_value ^= *ptr_u32++;
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
		hash_value = murmur2_neutral_hash( GetPalettePtr(), (GetSize() == G_IM_SIZ_4b)? 16*4 : 256*4, hash_value );
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
			return g_ROM.T1_HACK? TexFmt_4444 : TexFmt_5551;
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
