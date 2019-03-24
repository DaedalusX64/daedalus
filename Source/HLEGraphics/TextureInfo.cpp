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
#include "TextureInfo.h"

#include "Config/ConfigOptions.h"
#include "Core/Memory.h"
#include "Core/ROM.h"
#include "OSHLE/ultra_gbi.h"
#include "Utility/Hash.h"
#include "Utility/Profiler.h"

static const char * const	gImageFormatNames[8] {"RGBA", "YUV", "CI", "IA", "I", "?1", "?2", "?3"};
static const u32			gImageSizesInBits[4] {4, 8, 16, 32};

const char * TextureInfo::GetFormatName() const
{
	return gImageFormatNames[ Format ];
}

u32 TextureInfo::GetSizeInBits() const
{
	return gImageSizesInBits[ Size ];
}

// Fast hash for checking is data in a texture source has changed //Corn
u32 TextureInfo::GenerateHashValue() const
{
	//Rewritten to use less recources //Corn
#ifdef DAEDALUS_ENABLE_PROFILING
	DAEDALUS_PROFILE( "TextureInfo::GenerateHashValue" );
#endif
	// If CRC checking is disabled, always return 0
	if ( gCheckTextureHashFrequency == 0 ) return 0;

	//DAEDALUS_ASSERT( (GetLoadAddress() + Height * Pitch) < 4*1024*1024, "Address of texture is out of bounds" );

	//Number of places to do fragment hash from in texture
	//More rows will use more CPU...
	u32 CHK_ROW {5};
	u32 hash_value {};
	u8 *ptr_u8 {g_pu8RamBase + GetLoadAddress()};

	if( g_ROM.GameHacks == YOSHI )
	{
		CHK_ROW = 49;
		if (GetFormat() == G_IM_FMT_CI)
		{
			//Check palette changes too but only first 16 palette values//Corn
			const u32* ptr_u32 {(u32*)GetTlutAddress()};
			for (u32 z {}; z < 8; z++) hash_value = ((hash_value << 1) | (hash_value >> 0x1F)) ^ *ptr_u32++;
		}
	}
	else if( g_ROM.GameHacks == WORMS_ARMAGEDDON )
	{
		CHK_ROW = 1000;
		if (GetFormat() == G_IM_FMT_CI)
		{
			//Check palette changes too but only first 16 palette values//Corn
			const u32* ptr_u32 {(u32*)GetTlutAddress()};
			for (u32 z {}; z < 8; z++) hash_value = ((hash_value << 1) | (hash_value >> 0x1F)) ^ *ptr_u32++;
		}
	}

	u32 step {(u32)(Height * Pitch)};	//Get size in bytes, seems to be more accurate (alternative -> Height * Width * (1<<Size) >> 1;)

	if((u32)ptr_u8 & 0x3)	//Check if aligned to 4 bytes if not then align
	{
		ptr_u8 += 4 - ((u32)ptr_u8 & 0x3);
		step   -= 4 - ((u32)ptr_u8 & 0x3);
	}

	u32 *ptr_u32 {(u32*)ptr_u8};	//use 32bit access
	step = step >> 2;	//convert to 32bit access

	//We want to sample the texture data as far apart as possible
	if (step < (CHK_ROW << 2))	//if texture is small hash all of it
	{
		for (u32 z {}; z < step; z++)
		{
			hash_value = ((hash_value << 1) | (hash_value >> 0x1F)) ^ ptr_u32[z];
		}
	}
	else	//if texture is big, hash only some parts inside it
	{
		step = (step - 4) / CHK_ROW;
		for (u32 y {}; y < CHK_ROW; y++)
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
		const u32* ptr_u32 = (u32*)GetTlutAddress();
		for (u32 z = 0; z < ((GetSize() == G_IM_SIZ_4b)? 8 : 128); z++) hash_value ^= *ptr_u32++;
	}*/

	//printf("%08X %d S%d P%d H%d W%d B%d\n", hash_value, step, Size, Pitch, Height, Width, Height * Pitch);
	return hash_value;
}
