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

// Limit cache ucode entries to 6
// In theory we should never reach this max
#define MAX_UCODE_CACHE_ENTRIES 6

extern void DLParser_SetCustom( u32 ucode, u32 offset );


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
// Used to keep track of used ucode entries
//
struct UcodeInfo
{
	u32	ucode;
	u32	index;

	bool set;
};

static UcodeInfo gUcodeInfo[ MAX_UCODE_CACHE_ENTRIES ];

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
				char c( ram[ (data_base + i)  ^ U8_TWIDDLE ] );
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

static u32 GBIMicrocode_MicrocodeHash(u32 code_base, u32 code_size)
{
	// Needed for Conker's Bad Fur Day
	if( code_size == 0 ) code_size = 0x1000;

	const u8 * ram( g_pu8RamBase );

	u32 hash = 0;
	for (u32 i = 0; i < code_size; ++i)
	{
		hash = (hash << 4) + hash + ram[ (code_base+i) ^ U8_TWIDDLE ];   // Best hash ever!
	}
	return hash;
}

void GBIMicrocode_Reset()
{
	memset(&gUcodeInfo, 0, sizeof(gUcodeInfo));
}

//*****************************************************************************
//
//*****************************************************************************
static void GBIMicrocode_Custom( u32 code_hash, u32 &ucode_version, u32 &ucode_offset )
{

	//
	//	The only games that need defining are custom ucodes
	//	If you believe a title should be here post the line for it from ucodes.txt @ http://www.daedalusx64.com
	//	Note - Games are in alphabetical order by game title
	//

	switch( code_hash )
	{
	case 0x60256efc:		// "RSP Gfx ucode F3DEXBG.NoN fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo.", "Conker's Bad Fur Day"
		ucode_version = GBI_CONKER;
		ucode_offset  = GBI_2;
		break;

	case 0x6d8bec3e:		//"", "Dark Rift"
	case 0x26da8a4c:		//"", "Last Legion UX"
	case 0xdd560323:		//"", "Toukon Road - Brave Spirits"
		ucode_version = GBI_LL;
		ucode_offset  = GBI_1;
		break;

	case 0x0c10181a:		//"", "Diddy Kong Racing (v1.0)"
	case 0x713311dc:		//"", "Diddy Kong Racing (v1.1)"
	case 0x169dcc9d:		//"", "Jet Force Gemini"
		ucode_version = GBI_DKR;
		ucode_offset  = GBI_0;
		break;

	case 0x23f92542:		//"RSP SW Version: 2.0G, 09-30-96", "GoldenEye 007"
		ucode_version = GBI_GE;
		ucode_offset  = GBI_0;
		break;

	case 0xcac47dc4:		//"", "Perfect Dark (v1.1)"
		ucode_version = GBI_PD;
		ucode_offset  = GBI_0;
		break;

	case 0x6cbb521d:		//"RSP SW Version: 2.0D, 04-01-96", "Star Wars - Shadows of the Empire (v1.0)
		ucode_version = GBI_SE;
		ucode_offset  = GBI_0;
		break;

	case 0x64cc729d:		//"RSP SW Version: 2.0D, 04-01-96", "Wave Race 64"
		ucode_version = GBI_WR;
		ucode_offset  = GBI_0;
		break;

	default:
		break;
	}

}

u32	GBIMicrocode_DetectVersion( u32 code_base, u32 code_size, u32 data_base, u32 data_size )
{
	u32 i;
	// I think only checking code_base should be enough..
	u32 idx( code_base + data_base );
	//

	// Cheap way to cache ucodes, don't check for strings (too slow!) but check last used ucode entries which is alot faster than string comparison.
	// This only needed for GBI1/2/SDEX ucodes that use LoadUcode, else we only check when code_base changes, which usually never happens
	//
	for( i = 0; i < MAX_UCODE_CACHE_ENTRIES; i++ )
	{
		const UcodeInfo &used( gUcodeInfo[ i ] );

		// If this returns false, it means this entry is free to use
		if( used.set == false )
			break;

		if( used.index == idx )
			return used.ucode;
	}

	//
	//	Try to find the version string in the microcode data. This is faster than calculating a crc of the code
	//
	char str[256] = "";
	GBIMicrocode_DetectVersionString( data_base, data_size, str, 256 );

	// It wasn't the same as the last time around, we'll hash it and check if is a custom ucode.
	//
	u32 code_hash( GBIMicrocode_MicrocodeHash( code_base, code_size ) );
	u32 ucode_version = GBI_0; // default ucode (Fast3D)
	u32 ucode_offset  = ~0;

	GBIMicrocode_Custom( code_hash, ucode_version, ucode_offset );
	if( ucode_offset == u32(~0) )
	{
		//
		// If it wasn't a custom ucode
		// See if we can identify it by string, if no match was found
		//
		const char  *ucodes[] = { "F3", "L3", "S2DEX" };
		char 		*match = 0;

		for(u32 j = 0; j<3;j++)
		{
			if( (match = strstr(str, ucodes[j])) )
				break;
		}

		if( match )
		{
			if( strstr(match, "fifo") || strstr(match, "xbus") )
			{
				if( !strncmp(match, "S2DEX", 5) )
					ucode_version = GBI_2_S2DEX;
				else
					ucode_version = GBI_2;
			}
			else
			{
				if( !strncmp(match, "S2DEX", 5) )
					ucode_version = GBI_1_S2DEX;
				else
					ucode_version = GBI_1;
			}
		}
	}
	else
	{
		// If this a custom ucode, let's build an array based from ucode_offset
		DLParser_SetCustom( ucode_version, ucode_offset );
	}

	//
	// Retain used ucode info which will be cached
	//
	gUcodeInfo[ i ].index = idx;
	gUcodeInfo[ i ].ucode = ucode_version;
	gUcodeInfo[ i ].set = true;

	DBGConsole_Msg(0,"Detected %s Ucode is: [M Ucode %d, 0x%08x, \"%s\", \"%s\"]",ucode_offset == u32(~0) ? "Custom" :"", ucode_version, code_hash, str, g_ROM.settings.GameName.c_str() );
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

