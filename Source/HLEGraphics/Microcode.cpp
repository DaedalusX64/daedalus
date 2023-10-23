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

#include "Base/Types.h"
#include "HLEGraphics/Microcode.h"

#include "Core/ROM.h"
#include "Core/Memory.h"
#include "Math/Math.h"

#include "Debug/DBGConsole.h"
#include "Ultra/ultra_gbi.h"

#include <random>

// Limit cache ucode entries to 6
// In theory we should never reach this max
#define MAX_UCODE_CACHE_ENTRIES 6

//*****************************************************************************
//
//*****************************************************************************
static void GBIMicrocode_SetCustomArray( u32 ucode_version, u32 ucode_offset );

static MicroCodeInstruction gCustomInstruction[256];
static const char * gCustomInstructionName[256];
#define SetCommand( cmd, func, name )	\
	gCustomInstruction[ cmd ] = func;	\
	gCustomInstructionName[ cmd ] = name;


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
struct UcodeUsage
{
	bool ucode_set;

	u32 code_base;
	u32 data_base;
	
	UcodeInfo info;
};


static UcodeUsage gUcodeUsage[ MAX_UCODE_CACHE_ENTRIES ];

static bool	GBIMicrocode_DetectVersionString( u32 data_base, u32 data_size, char * str, u32 str_len )
{
	const s8 *ram = g_ps8RamBase;

	for ( u32 i = 0; i+2 < data_size; i++ )
	{
		if ( ram[ (data_base + i+0) ^ U8_TWIDDLE ] == 'R' &&
			 ram[ (data_base + i+1) ^ U8_TWIDDLE ] == 'S' &&
			 ram[ (data_base + i+2) ^ U8_TWIDDLE ] == 'P' )
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
	const u8 * ram = g_pu8RamBase;
	u32 hash = 0;

	for (u32 i = 0; i < code_size; ++i)
	{
		hash = (hash << 4) + hash + ram[ (code_base+i) ^ U8_TWIDDLE ];   // Best hash ever!
	}
	return hash;
}

void GBIMicrocode_Reset()
{
	// Unset any previously cached ucode
	for (u32 i = 0; i < MAX_UCODE_CACHE_ENTRIES; i++)
 		gUcodeUsage[i].ucode_set = false;
}

//*****************************************************************************
//
//*****************************************************************************
struct MicrocodeData
{
	u32	ucode;
	u32 offset;
	u32	hash;

	const char *ucode_name;
};

static const std::array<MicrocodeData, 12> gMicrocodeData {
{
	//
	//	The only games that need defining here are custom ucodes or ucodes that lack a version string in the microcode data
	//	Note - Games are in alphabetical order by game title
	//
	{ GBI_CONKER,	GBI_2,	0x60256efc,	"RSP Gfx ucode F3DEXBG.NoN fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo."},	// Conker's Bad Fur Day
	{ GBI_LL,		GBI_1,	0x6d8bec3e,	"RSP Gfx ucode: Unknown"},			//"Dark Rift"
	{ GBI_DKR,		GBI_0,	0x0c10181a,	"RSP Gfx ucode: Unknown"},			//"Diddy Kong Racing (v1.0)"
	{ GBI_DKR,		GBI_0,	0x713311dc,	"RSP Gfx ucode: Unknown"},			//"Diddy Kong Racing (v1.1)"
	{ GBI_GE,		GBI_0,	0x23f92542,	"RSP SW Version: 2.0G, 09-30-96"},	//"GoldenEye 007"
	{ GBI_DKR,		GBI_0,	0x169dcc9d,	"RSP Gfx ucode: Unknown"},			//"Jet Force Gemini"
	{ GBI_LL,		GBI_1,	0x26da8a4c,	"RSP Gfx ucode: Unknown"},			//"Last Legion UX"}
	{ GBI_PD,		GBI_0,	0xcac47dc4,	"RSP Gfx ucode: Unknown"},			//"Perfect Dark (v1.1)"
	{ GBI_RS,		GBI_1,	0xc62a1631, "RSP Gfx ucode: Unknown"},			// Star Wars - Rogue Squadron
	{ GBI_BETA,		GBI_0,	0x6cbb521d,	"RSP SW Version: 2.0D, 04-01-96"},	//"Star Wars - Shadows of the Empire (v1.0)"
	{ GBI_LL,		GBI_1,	0xdd560323,	"RSP Gfx ucode: Unknown"},			//"Toukon Road - Brave Spirits"
	{ GBI_BETA,		GBI_0,	0x64cc729d,	"RSP SW Version: 2.0D, 04-01-96"},	//"Wave Race 64 (v1.1)"
}};

UcodeInfo GBIMicrocode_SetCache(u32 index, u32 code_base, u32 data_base, 
	const MicroCodeInstruction * ucode_function, const char ** name )
{
		std::default_random_engine FastRand;
	//
	// If the max of ucode entries is reached, spread it randomly
	// Otherwise we'll keep overriding the last entry
	//
	if (index >= MAX_UCODE_CACHE_ENTRIES)
	{
		DBGConsole_Msg(0, "Reached max of ucode entries, spreading entry..");
		index = FastRand() % MAX_UCODE_CACHE_ENTRIES;
	}

	UcodeUsage& used(gUcodeUsage[index]);
	used.ucode_set = true;
	used.code_base = code_base;
	used.data_base = data_base;
	
	used.info.func = ucode_function;
	used.info.name = name;
	return used.info;
}

UcodeInfo GBIMicrocode_DetectVersion( u32 code_base, u32 code_size, u32 data_base, u32 data_size )
{
	// Cheap way to cache ucodes, don't compare strings (too slow!) instead check the last used ucode entries which is alot faster.
	u32 index;
	for( index = 0; index < MAX_UCODE_CACHE_ENTRIES; index++ )
	{
		const UcodeUsage &used( gUcodeUsage[ index ] );

		// If this returns false, it means this entry its free to use
		if( used.ucode_set == false )
			break;

		if( used.data_base == data_base && used.code_base == code_base)
		{
			return used.info; // Found a match!
		}
	}

	//
	// If it wasn't the same ucode as the last time around, we'll hash it to check if is a custom ucode.
	//
	u32 code_hash = GBIMicrocode_MicrocodeHash( code_base, code_size );

	for ( u32 i = 0; i < gMicrocodeData.size(); i++ )
	{
		if ( code_hash == gMicrocodeData[i].hash )
		{
			u32 ucode_version = gMicrocodeData[i].ucode;
			u32 ucode_offset = gMicrocodeData[i].offset;

			GBIMicrocode_SetCustomArray( ucode_version, ucode_offset ); 
			DBGConsole_Msg(0, "Detected Custom Ucode is: [M Ucode %d, 0x%08x, \"%s\", \"%s\"]", ucode_version, code_hash, 
				gMicrocodeData[i].ucode_name, g_ROM.settings.GameName.c_str());
			return GBIMicrocode_SetCache( index, code_base, data_base, gCustomInstruction, gCustomInstructionName );
		}
	}
	
	//
	// If it wasn't a custom ucode. Try to detect by checking the version string in the microcode data.
	// This is faster than calculating a crc of the code
	//

	// Select Fast3D ucode in case there's no match or if the version string its missing
	u32 ucode_version = GBI_0;

	char str[256] = "";
	if( !GBIMicrocode_DetectVersionString( data_base, data_size, str, 256 ) ) 
	{
		DBGConsole_Msg(0, "Unable to detect Ucode: [Y Version string its missing, defaulting to Fast3D ucode, expect errors]");
	}
	else
	{
		const char  *ucodes[] { "F3", "L3", "S2DEX" };
		char 		*match = 0;

		for(u32 j = 0; j < 3; j++)
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
	DBGConsole_Msg(0, "Detected Ucode is: [M Ucode %d, 0x%08x, \"%s\", \"%s\"]", ucode_version, code_hash, 
		str, g_ROM.settings.GameName.c_str());
	return GBIMicrocode_SetCache(index, code_base, data_base, gNormalInstruction[ ucode_version ], gNormalInstructionName[ ucode_version ]);
}

//****************************************************'*********************************
// This is called after a custom ucode has been detected. This function gets cached and its only called once per custom ucode set
// Main resaon for this function is to save memory since custom ucodes share a common table
// USAGE:
//		ucode:			custom ucode: (ucode>= 5), defined in GBIVersion enum
//		offset:			offset to a normal ucode which this custom ucode is based of ex GBI0
//*************************************************************************************
static void GBIMicrocode_SetCustomArray( u32 ucode_version, u32 ucode_offset )
{
	//DBGConsole_Msg(0, "Building a custom array now.. version:%d >>> offset:%d",ucode_version, ucode_offset);
	
	for (u32 i = 0; i < 256; i++)
	{
		gCustomInstruction[i] = gNormalInstruction[ucode_offset][i];
		gCustomInstructionName[i] = gNormalInstructionName[ucode_offset][i];
	}

	// Start patching to create our custom ucode table ;)
	switch( ucode_version )
	{
		case GBI_GE:
			SetCommand( G_GBI1_RDPHALF_1, DLParser_RDPHalf1_GE,			"G_RDPHalf1_GE" );
			SetCommand( G_GE_TriX, DLParser_Trix_GE,					"G_TriX_GE" );
			break;
		case GBI_BETA:
			SetCommand( G_GBI1_VTX,	DLParser_GBI0_Vtx_Beta, 			"G_Vtx_Beta" );
			SetCommand( G_GBI1_TRI1, DLParser_GBI0_Tri1_Beta, 			"G_Tri1_Beta" );
			SetCommand( G_GBI1_TRI2, DLParser_GBI0_Tri2_Beta, 			"G_Tri2_Beta" );
			SetCommand( G_GBI1_LINE3D, DLParser_GBI0_Line3D_Beta,		"G_Line3_Beta" );
			break;
		case GBI_LL:
			SetCommand( 0x80, DLParser_Last_Legion_0x80,				"G_Last_Legion_0x80" );
			SetCommand( 0x00, DLParser_Last_Legion_0x00,				"G_Last_Legion_0x00" );
			SetCommand( G_RDP_TEXRECT, DLParser_TexRect_Last_Legion,	"G_TexRect_Last_Legion" );
			break;
		case GBI_PD:
			SetCommand( G_GBI1_VTX, DLParser_Vtx_PD,					"G_Vtx_PD" );
			SetCommand( G_PD_VTXBASE, DLParser_Set_Vtx_CI_PD,			"G_Set_Vtx_CI_PD" );
			SetCommand( G_GBI1_RDPHALF_1, DLParser_RDPHalf1_GE,			"G_RDPHalf1_GE" );
			SetCommand( G_GE_TriX, DLParser_Trix_GE,					"G_TriX_GE" );
			break;
		case GBI_DKR:
			SetCommand( G_GBI1_MTX, DLParser_Mtx_DKR,		 			"G_Mtx_DKR" );
			SetCommand( G_GBI1_VTX, DLParser_GBI0_Vtx_DKR, 				"G_Vtx_DKR" );
			SetCommand( G_DKR_DMATRI, DLParser_DMA_Tri_DKR,  			"G_DMA_Tri_DKR" );
			SetCommand( G_DKR_DLINMEM, DLParser_DLInMem,		 		"G_DLInMem" );
			SetCommand( G_GBI1_MOVEWORD, DLParser_MoveWord_DKR, 		"G_MoveWord_DKR" );
			SetCommand( G_GBI1_TRI1, DLParser_Set_Addr_DKR, 			"G_Set_Addr_DKR" );
			SetCommand( G_GBI1_TEXTURE, DLParser_GBI1_Texture_DKR,		"G_Texture_DKR" );
			break;
		case GBI_CONKER:
			SetCommand( G_GBI2_VTX,	DLParser_Vtx_Conker, 				"G_Vtx_Conker" );
			SetCommand( G_GBI2_TRI1, DLParser_Tri1_Conker, 				"G_Tri1_Conker" );
			SetCommand( G_GBI2_TRI2, DLParser_Tri2_Conker,				"G_Tri2_Conker" );
			SetCommand( G_GBI2_MOVEWORD, DLParser_MoveWord_Conker,		"G_MoveWord_Conker");
			SetCommand( G_GBI2_MOVEMEM, DLParser_MoveMem_Conker,		"G_MoveMem_Conker" );
			for (u32 i = 0; i < 16; i++)
			{
				SetCommand( G_CONKER_QUAD + i, DLParser_Tri4_Conker, "G_Tri4_Conker" );
			}
			break;
		case GBI_ACCLAIM:
			//SetCommand( G_GBI2_MOVEMEM, DLParser_MoveMem_Acclaim, "G_MoveMem_Acclaim" );
			break;
		default:
			DAEDALUS_ERROR("Unknown custom ucode set:%d [Y Did you forget to define it?]");
			break;
	}
}
