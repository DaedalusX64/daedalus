/*
Copyright (C) 2009 StrmnNrmn

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
#include "Microcode.h"

#include "Core/ROM.h"
#include "Core/Memory.h"

#include "Debug/DBGConsole.h"
#include "Math/Math.h"	// pspFastRand()
#include "Utility/Hash.h"

// Limit cache ucode entries to 12
// This is done for performance reasons
//
// Increase this number to cache more ucode entries
// At the expense of more time required each pass
//
#define MAX_UCODE_CACHE_ENTRIES 12


//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                    uCode Config                      //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

// NoN			No Near clipping
// Rej			Reject polys with one or more points outside screenspace

//F3DEX: Extended fast3d. Vertex cache is 32, up to 18 DL links
//F3DLX: Compatible with F3DEX, GBI, but not sub-pixel accurate. Clipping can be explicitly enabled/disabled
//F3DLX.Rej: No clipping, rejection instead. Vertex cache is 64
//F3FLP.Rej: Like F3DLX.Rej. Vertex cache is 80
//L3DEX: Line processing, Vertex cache is 32.

//
// Used for custom ucodes' array
//
struct MicrocodeData
{
	u32				ucode;
	u32				code_hash;
};


#ifdef DAEDALUS_ENABLE_ASSERTS
const u32	MAX_RAM_ADDRESS = (8*1024*1024);
#endif

//
// Used to keep track of used ucode entries
//
UcodeInfo used[ MAX_UCODE_CACHE_ENTRIES ];
UcodeInfo current;
//*****************************************************************************
//
//*****************************************************************************
static const MicrocodeData gMicrocodeData[] = 
{
	//	The only games that need defining are custom ucodes and incorrectly detected ones
	//	If you believe a title should be here post the line for it from ucodes.txt @ http://www.daedalusx64.com
	//	Note - Games are in alphabetical order by game title
	//
	{ GBI_0_CK,  0x10372b79	},// "RSP Gfx ucode F3DEXBG.NoN fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo.", "Conker's Bad Fur Day"}, 
	{ GBI_0_LL,  0x9a824412	},//"", "Dark Rift"},
	{ GBI_0_DKR, 0xa3f481d8	},//"", "Diddy Kong Racing (v1.0)"}, 
	{ GBI_0_DKR, 0xd5d68f00	},//"", "Diddy Kong Racing (v1.1)"}, 
	{ GBI_0_GE,  0x96c35300	},//"RSP SW Version: 2.0G, 09-30-96", "GoldenEye 007"}, 
	{ GBI_0_JFG, 0x58823aab	},//"", "Jet Force Gemini"},														
	{ GBI_0_LL,  0x85185534	},//"", "Last Legion UX"},							
	{ GBI_0_PD,  0x84c127f1	},//"", "Perfect Dark (v1.1)"}, 
	{ GBI_0_SE,  0xd010d659	},//"RSP SW Version: 2.0D, 04-01-96", "Star Wars - Shadows of the Empire (v1.0)"}, 
	{ GBI_0_LL,  0xf9ec7828	},//"", "Toukon Road - Brave Spirits"},											
	{ GBI_0_WR,  0xbb5a808d	},//"RSP SW Version: 2.0D, 04-01-96", "Wave Race 64"},
	//{ GBI_0_UNK, 0x10b092bf, "", "World Driver Championship"},		
	//{ GBI_0_UNK, 0x5719c8de, "", "Star Wars - Rogue Squadron"}, 
};

//*****************************************************************************
//
//*****************************************************************************
static bool	GBIMicrocode_DetectVersionString( u32 data_base, u32 data_size, char * str, u32 str_len )
{
	DAEDALUS_ASSERT( data_base < MAX_RAM_ADDRESS + 0x1000 ,"GBIMicrocode out of bound %08X", data_base );

	const s8 * ram( g_ps8RamBase );

	for ( u32 i = 0; i+2 < data_size; i++ )
	{
		if ( ram[ (data_base + i+0) ^ 3 ] == 'R' &&
			 ram[ (data_base + i+1) ^ 3 ] == 'S' &&
			 ram[ (data_base + i+2) ^ 3 ] == 'P' )
		{
			char * p = str;
			char * e = str+str_len;

			// Loop while we haven't filled our buffer, and there's space for our terminator
			while (p+1 < e)
			{
				char c( ram[ (data_base + i) ^ 3 ] );
				if( c < ' ')
					break;

				*p++ = c;
				++i;
			}
			*p++ = 0;
			return true;
		}
	}
	return false;
}
u32 mCurrentUcode = 0;
//*****************************************************************************
//
//*****************************************************************************
static void GBIMicrocode_Cache( u32 index, u32 code_base, u32 data_base, u32 ucode_version)
{
	used[ index ].code_base = code_base;
	used[ index ].data_base = data_base;
	used[ index ].ucode 	= ucode_version;
	used[ index ].used 		= true;
}

//*****************************************************************************
//
//*****************************************************************************
u32	GBIMicrocode_DetectVersion( u32 code_base, u32 code_size, u32 data_base, u32 data_size)
{

	u32 index;
	if( code_size == 0 ) code_size = 0x1000;

	DAEDALUS_ASSERT( code_base, "Warning : Last Ucode might be ignored!" );

	// Cheap way to cache ucodes, don't check for strings (too slow!) but check last used ucode entries which is alot faster than string comparison.
	// This only needed for GBI1/SDEX1 ucodes that use LoadUcode, else we only check when t.ucode changes, which most of the time only happens once :)
	//
	for( index = 0; index < MAX_UCODE_CACHE_ENTRIES; index++ )
	{
		if( used[ index ].used == false )	
			break;
			
		// Check if the microcode is the same to the last used ucodes (We cache up to 12 entries)
		//
		if( used[ index ].code_base == code_base && used[ index ].data_base == data_base )
		{
			return used[ index ].ucode;
		}
	}

	//
	// If the max of ucode entries is reached, spread it randomly
	// Otherwise we'll keep overriding the last entry
	// 
	if( index >= MAX_UCODE_CACHE_ENTRIES )
	{
		DBGConsole_Msg(0, "Warning : Reached max of ucode entries!");

		index = pspFastRand()%MAX_UCODE_CACHE_ENTRIES;
		DBGConsole_Msg(0, "Spreading entry to (%d)",index );
	}

	//
	//	Try to find the version string in the microcode data. This is faster than calculating a crc of the code
	//
	char str[256] = "";
	GBIMicrocode_DetectVersionString( data_base, data_size, str, 256 );

	// It wasn't the same as the last time around, we'll hash it and check the array. 
	// Don't bother checking for matches when ucode was found in array
	// This only used for custom ucodes
	//
	u32 code_hash( murmur2_neutral_hash( &g_pu8RamBase[ code_base ], code_size, 0 ) );

	for ( u32 i = 0; i < ARRAYSIZE(gMicrocodeData); i++ )
	{
		if ( code_hash == gMicrocodeData[i].code_hash )
		{
			//
			// Retain used ucode info which will be cached
			//
			GBIMicrocode_Cache( index, code_base, data_base, gMicrocodeData[ i ].ucode);

			DBGConsole_Msg(0, "Ucode has been Detected in Array :[M\"%s\", Ucode %d]", str, gMicrocodeData[ i ].ucode);
			return gMicrocodeData[ i ].ucode;
		}
	}
	//
	// If it wasn't found in the array
	// See if we can identify it by string, if no match was found. Set default ucode (Fast3D)
	//
	const char  *ucodes[] = { "F3", "L3", "S2DEX" };
	char 		*match = 0;
	u32 		ucode_version = 0;

	for(u32 j = 0; j<3;j++)
	{
		if( (match = strstr(str, ucodes[j])) )
			break;
	}

	if( match )
	{
		if( strstr(match, "fifo") || strstr(match, "xbus") )
		{
			ucode_version = GBI_2;
		}
		else
		{
			if( !strncmp(match, "S2DEX", 5) )
				ucode_version = S2DEX_GBI_1;
			else
				ucode_version = GBI_1;	
		}
	}
	else
	{
		ucode_version = GBI_0;
	}

	//
	// Retain used ucode info which will be cached
	//
	GBIMicrocode_Cache( index, code_base, data_base, ucode_version);

	DBGConsole_Msg(0,"Detected Ucode is: [M Ucode %d, 0x%08x, \"%s\", \"%s\"]",ucode_version, code_hash, str, g_ROM.settings.GameName.c_str() );

// This is no longer needed as we now have an auto ucode detector, I'll leave it as reference ~Salvy
//
/*
	FILE * fh = fopen( "ucodes.txt", "a" );
	if ( fh )
	{
		fprintf( fh,  "{ ucode=%d, 0x%08x, \"%s\", \"%s\"}, \n", ucode_version, code_hash, str, g_ROM.settings.GameName.c_str() );
		fclose(fh);
	}
*/

	return ucode_version;
}
