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

#include "Core/Memory.h"
#include "Core/ROM.h"
#include "Core/R4300OpCode.h"

#include "Debug/Dump.h"
#include "Debug/DBGConsole.h"

#include "Utility/CRC.h"
#include "Utility/Hash.h"
#include "Utility/IO.h"
#include "Utility/Preferences.h"


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

struct MicrocodeData
{
	GBIVersion		gbi_version;
	UCodeVersion	ucode_version;
	u32				code_hash;
	const char *	ucode_name;
	const char *	rom_name;
};

typedef CFixedString<255> MicrocodeString;

struct MicrocodeCacheEntry
{
	bool Matches( const char * str, const char * title, GBIVersion * gbi_version, UCodeVersion * ucode_version ) const
	{
		if( strcmp( str, VersionString ) == 0 && strcmp( title, RomTitle ) == 0)
		{
			*gbi_version = Data.gbi_version;
			*ucode_version = Data.ucode_version;
			return true;
		}

		return false;
	}

	void Set( const char * str, const char * title, const MicrocodeData data )
	{
		VersionString = str;
		RomTitle = title;
		Data = data;
	}

	MicrocodeString		VersionString;
	MicrocodeString		RomTitle;
	MicrocodeData 		Data;
};

static const u32				NUM_MICROCODE_CACHE_ENTRIES = 4;
static MicrocodeCacheEntry		gMicrocodeCache[ NUM_MICROCODE_CACHE_ENTRIES ];
static u32						gCurrentMicrocodeCacheEntry = 0;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
static u32						gMicrocodeHistoryCount = 0;
static const u32				MICROCODE_HISTORY_MAX = 10;
static MicrocodeString			gMicrocodeHistory[ MICROCODE_HISTORY_MAX ];
#endif

//*****************************************************************************
//
//*****************************************************************************
static const MicrocodeData gMicrocodeData[] = 
{
	//The only games that need defining are custom ucodes and incorrectly detected ones
	// If you believe a title should be here post the line for it from ucodes.txt @ http://www.daedalusx64.com
	//Note - Games are in alphabetical order by game title

	{ GBI_0_CK, F3DEX, 0x10372b79, "RSP Gfx ucode F3DEXBG.NoN fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo.", "Conker's Bad Fur Day"}, 
	{ GBI_0_LL, F3DEX,  0x9a824412, "", "Dark Rift"},
	{ GBI_0_DKR, F3DEX, 0xa3f481d8, "", "Diddy Kong Racing (v1.0)"}, 
	{ GBI_0_DKR, F3DEX, 0xd5d68f00, "", "Diddy Kong Racing (v1.1)"}, 
	{ GBI_0_GE, FAST3D, 0x96c35300, "RSP SW Version: 2.0G, 09-30-96", "GoldenEye 007"}, 
	{ GBI_0_JFG, F3DEX, 0x58823aab, "", "Jet Force Gemini"},    // Mickey's Speedway USA uses the same string mapping
	{ GBI_0_LL, FAST3D, 0x85185534, "", "Last Legion UX"},		// Toukon 2 uses the same string mapping
	{ GBI_0_PD, FAST3D, 0x84c127f1, "", "Perfect Dark (v1.1)"}, 
	{ GBI_0_SE, FAST3D, 0xd010d659, "RSP SW Version: 2.0D, 04-01-96", "Star Wars - Shadows of the Empire (v1.0)"}, 
	{ GBI_0_LL, FAST3D, 0xf9ec7828, "", "Toukon Road - Brave Spirits"}, //f3dex
	{ GBI_0_WR, FAST3D, 0xbb5a808d, "RSP SW Version: 2.0D, 04-01-96", "Wave Race 64"},
	{ GBI_0_UNK, F3DEX, 0x10b092bf, "", "World Driver Championship"}, // Stunt Racer 64 uses the same string
	{ GBI_0_UNK, F3DEX, 0x5719c8de, "", "Star Wars - Rogue Squadron"}, 


};

//*****************************************************************************
//
//*****************************************************************************
bool	GBIMicrocode_DetectVersionString( u32 data_base, u32 data_size, char * str, u32 str_len )
{
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

//*****************************************************************************
//
//*****************************************************************************
void	GBIMicrocode_DetectVersion( u32 code_base, u32 code_size,
									u32 data_base, u32 data_size,
									GBIVersion * gbi_version, UCodeVersion * ucode_version )
{
	u32 i;
	char str[256] = "";
	char* title = (char*)g_ROM.settings.GameName.c_str();
	if( code_size == 0 ) code_size = 0x1000;

	//
	//	Try to find the version string in the microcode data. This is faster than calculating a crc of the code
	//
	GBIMicrocode_DetectVersionString( data_base, data_size, str, 256 );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if( gMicrocodeHistoryCount < MICROCODE_HISTORY_MAX )
	{
		gMicrocodeHistory[gMicrocodeHistoryCount++] = str;
	}
#endif

	//
	// Check if the microcode is the same as last time around (if we didn't find a version string, we allow matches with the empty string)
	//
	for( i = 0; i < NUM_MICROCODE_CACHE_ENTRIES; ++i )
	{
		if ( gMicrocodeCache[ i ].Matches( str, title, gbi_version, ucode_version ) )
		{
			return;
		}
	}

	//
	//	It wasn't the same as last time around, so we'll hash it and check the array
	//
	u32 code_hash( murmur2_neutral_hash( &g_pu8RamBase[ code_base ], code_size, 0 ) );

	MicrocodeData microcode;
	bool CheckArray = false;
	for ( i = 0; i < ARRAYSIZE(gMicrocodeData); i++ )
	{	
		if ( code_hash == gMicrocodeData[i].code_hash )
		{
			microcode = gMicrocodeData[ i ];
			CheckArray = true;
			break;
		}
	}

	//
	// If the microcode wasn't found in the array, we try to auto detect it
	//
	
	if ( !CheckArray )
	{
		const char *ucodes[] = { "F3", "L3", "S2DEX" };
		char *match = 0;

		for(i=0; i<3;i++)
		{
			if( (match = strstr(str, ucodes[i])) )
				break;
		}

		if(match)
		{
			if( !strncmp(match, "S2DEX", 5))
			{
				if( strstr(match, "fifo") || strstr(match, "xbus")){
					*ucode_version = S2DEX;
					*gbi_version = S2DEX_GBI_2;
				}
				else{
					*ucode_version = S2DEX;
					*gbi_version = S2DEX_GBI_1;
				}
			}
			else
			{
				if( strstr(match, "fifo") || strstr(match, "xbus"))
					*gbi_version = GBI_2;
				else
					*gbi_version = GBI_1;

				if( !strncmp(match, "F3DEX", 5))
				{
					*ucode_version = F3DEX;
				}
				if( !strncmp(match, "F3DZE", 5))
				{
					*ucode_version = F3DEX;
				}
				else if( !strncmp(match, "F3DLP", 5))
				{
					*ucode_version = F3DLP;
				}
				else if( !strncmp(match, "F3DLX", 5))
				{
					*ucode_version = F3DLX;
				}
				else if( !strncmp(match, "L3DEX", 5)){
					*ucode_version = F3DEX;
				}
			}
		}
		else
		{
			*ucode_version = FAST3D;
			*gbi_version = GBI_0;
		}

		microcode.gbi_version = *gbi_version;
		microcode.ucode_version = *ucode_version;
		microcode.ucode_name = str;
		microcode.rom_name = title;
		microcode.code_hash = code_hash;
	}

	DBGConsole_Msg(0, "Detected Ucode is: [M{ %u, %u, 0x%08x, \"%s\", \"%s\"}] \n", microcode.gbi_version, microcode.ucode_version, code_hash, str, title );
#ifndef DAEDALUS_PUBLIC_RELEASE
	if (gGlobalPreferences.LogMicrocodes)
	{
		FILE * fh = fopen( "ucodes.txt", "a" );
		if ( fh )
		{
			fprintf( fh,  "{ %u, %u, 0x%08x, \"%s\", \"%s\"}, \n", microcode.gbi_version, microcode.ucode_version, code_hash, str, title );
			fclose(fh);
		}
	}
#endif

	gMicrocodeCache[ gCurrentMicrocodeCacheEntry ].Set( str, title, microcode );
	gCurrentMicrocodeCacheEntry = ( gCurrentMicrocodeCacheEntry + 1 ) % NUM_MICROCODE_CACHE_ENTRIES;


	return;
}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
//*****************************************************************************
//
//*****************************************************************************
u32 GBIMicrocode_GetMicrocodeHistoryStringCount()
{
	return gMicrocodeHistoryCount = 0;
}

//*****************************************************************************
//
//*****************************************************************************
const char * GBIMicrocode_GetMicrocodeHistoryString( u32 i )
{
	if( i < gMicrocodeHistoryCount )
		return gMicrocodeHistory[ i ];

	return NULL;
}

//*****************************************************************************
//
//*****************************************************************************
void GBIMicrocode_ResetMicrocodeHistory()
{
	gMicrocodeHistoryCount = 0;
}
#endif
