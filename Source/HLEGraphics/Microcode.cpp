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
#include "Ucode.h"

#include "Core/Memory.h"
#include "Core/ROM.h"
#include "Core/R4300OpCode.h"

#include "Debug/Dump.h"
#include "Debug/DBGConsole.h"

#include "Utility/CRC.h"
#include "Utility/Hash.h"
#include "Utility/IO.h"
#include "Utility/Preferences.h"
#include "Math/Math.h"	// VFPU Math

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

// define this to use old ucode cache code (slower)
//#define DAEDALUS_UCODE_CACHE



struct MicrocodeData
{
	u32				ucode;
	u32				code_hash;
	const char *	ucode_name;
	const char *	rom_name;
};

typedef CFixedString<255> MicrocodeString;

#ifdef DAEDALUS_UCODE_CACHE
struct MicrocodeCacheEntry
{
	bool Matches( const char * str, const char * title, u32 ucode_version ) const
	{
		if( strcmp( str, VersionString ) == 0 && strcmp( title, RomTitle ) == 0)
		{
			ucode_version = Data.ucode;
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
#endif
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
static u32						gMicrocodeHistoryCount = 0;
static const u32				MICROCODE_HISTORY_MAX = 10;
static MicrocodeString			gMicrocodeHistory[ MICROCODE_HISTORY_MAX ];
#endif

UcodeInfo last;
UcodeInfo used[MAX_UCODE];
//*****************************************************************************
//
//*****************************************************************************
static const MicrocodeData gMicrocodeData[] = 
{
	//	The only games that need defining are custom ucodes and incorrectly detected ones
	//	If you believe a title should be here post the line for it from ucodes.txt @ http://www.daedalusx64.com
	//	Note - Games are in alphabetical order by game title
	//
	{ GBI_0_CK,  0x10372b79, "RSP Gfx ucode F3DEXBG.NoN fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo.", "Conker's Bad Fur Day"}, 
	{ GBI_0_LL,  0x9a824412, "", "Dark Rift"},
	{ GBI_0_DKR, 0xa3f481d8, "", "Diddy Kong Racing (v1.0)"}, 
	{ GBI_0_DKR, 0xd5d68f00, "", "Diddy Kong Racing (v1.1)"}, 
	{ GBI_0_GE,  0x96c35300, "RSP SW Version: 2.0G, 09-30-96", "GoldenEye 007"}, 
	{ GBI_0_JFG, 0x58823aab, "", "Jet Force Gemini"},														// Mickey's Speedway USA uses the same string mapping
	{ GBI_0_LL,  0x85185534, "", "Last Legion UX"},															// Toukon 2 uses the same string mapping
	{ GBI_0_PD,  0x84c127f1, "", "Perfect Dark (v1.1)"}, 
	{ GBI_0_SE,  0xd010d659, "RSP SW Version: 2.0D, 04-01-96", "Star Wars - Shadows of the Empire (v1.0)"}, 
	{ GBI_0_LL,  0xf9ec7828, "", "Toukon Road - Brave Spirits"},											
	{ GBI_0_WR,  0xbb5a808d, "RSP SW Version: 2.0D, 04-01-96", "Wave Race 64"},
	{ GBI_0_UNK, 0x10b092bf, "", "World Driver Championship"},												// Stunt Racer 64 uses the same string
	{ GBI_0_UNK, 0x5719c8de, "", "Star Wars - Rogue Squadron"}, 

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
u32	GBIMicrocode_DetectVersion( u32 code_base, u32 code_size, u32 data_base, u32 data_size)
{
	u32 i;
	u32 index;
	u32 ucode_version = 0;
	MicrocodeData       microcode;
	microcode.ucode	  = 0;

	char str[256] = "";
	char* title = (char*)g_ROM.settings.GameName.c_str();
	if( code_size == 0 ) code_size = 0x1000;

	DAEDALUS_ASSERT( code_base != 0, "Warning : Last Ucode might be ignored!" );

	// Cheap way to cache ucodes, don't check for strings (too slow!) but check last used ucode info which is alot faster than doing sting comparison with strcmp.
	// This only needed for GBI1/SDEX1 games that use LoadUcode, else is we only check when t.ucode changes, which most of the time only happens once :)
	//
	for( index = 0; index < MAX_UCODE; index++ )
	{
		if( used[ index ].used == false )	
			break;

		if( used[ index ].code_base == code_base && used[ index ].code_size == code_size && used[ index ].data_base == data_base )
		{
			//
			//Retain last info for easier access
			//
			last = used[ index ];
			return last.ucode;
		}
	}

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

	// Check if the microcode is the same as last time around (if we didn't find a version string, we allow matches with the empty string)
	//
#ifdef DAEDALUS_UCODE_CACHE
	for( i = 0; i < NUM_MICROCODE_CACHE_ENTRIES; ++i )
	{
		if ( gMicrocodeCache[ i ].Matches( str, title, ucode_version ) )
		{
			break;
		}
	}
#endif

	u32 code_hash( murmur2_neutral_hash( &g_pu8RamBase[ code_base ], code_size, 0 ) );

	// It wasn't the same as last time around, so we'll hash it and check the array. 
	// Don't bother checking for matches when ucode was found in array
	// This only used for custom ucodes
	//
	for ( i = 0; i < ARRAYSIZE(gMicrocodeData); i++ )
	{
		if ( code_hash == gMicrocodeData[i].code_hash )
		{
			DBGConsole_Msg(0, "Ucode has been Detected in Array :[M\%s, Ucode %d]", str, gMicrocodeData[ i ].ucode);
			microcode = gMicrocodeData[ i ];
			return microcode.ucode;
		}
	}
	//
	// If it wasn't found in the array
	// See if we can identify it by string, if no match was found. Try to guess it with Fast3D ucode
	//
	const char *ucodes[] = { "F3", "L3", "S2DEX" };
	char *match = 0;

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

	// Retain used ucode info which will be cached
	// Todo : Redactor this
	//
	used[ index ].code_base = code_base;
	used[ index ].code_size = code_size;
	used[ index ].data_base = data_base;
	used[ index ].ucode = ucode_version;
	used[ index ].used = true;

	microcode.ucode = ucode_version;
	microcode.ucode_name = str;
	microcode.rom_name = title;
	microcode.code_hash = code_hash;

	DBGConsole_Msg(0,"Detected Ucode is: [M Ucode %d, 0x%08x, \"%s\", \"%s\"]\n",ucode_version, code_hash, str, title );

#ifndef DAEDALUS_PUBLIC_RELEASE
	if (gGlobalPreferences.LogMicrocodes)
	{
		FILE * fh = fopen( "ucodes.txt", "a" );
		if ( fh )
		{
			fprintf( fh,  "{ ucode=%d, 0x%08x, \"%s\", \"%s\"}, \n", ucode_version, code_hash, str, title );
			fclose(fh);
		}
	}
#endif

#ifdef DAEDALUS_UCODE_CACHE
	gMicrocodeCache[ gCurrentMicrocodeCacheEntry ].Set( str, title, microcode );
	gCurrentMicrocodeCacheEntry = ( gCurrentMicrocodeCacheEntry + 1 ) % NUM_MICROCODE_CACHE_ENTRIES;
#endif	

	return microcode.ucode;
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
