/*
Copyright (C) 2001 StrmnNrmn

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
#include "ROM.h"
#include "CPU.h"
#include "RSP.h"
#include "PIF.h"		// CController
#include "R4300.h"
#include "ROMBuffer.h"
#include "ROMImage.h"
#include "RomSettings.h"

#include "Interface/RomDB.h"

#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"
#include "OSHLE/patch.h"			// Patch_ApplyPatches
#include "OSHLE/ultra_R4300.h"
#include "OSHLE/ultra_os.h"		// System type

#include "Utility/CRC.h"
#include "Utility/ROMFile.h"
#include "Utility/Preferences.h"
#include "Utility/FramerateLimiter.h"
#include "Utility/Stream.h"
#include "Utility/IO.h"
#include "Utility/Synchroniser.h"

#include "Plugins/AudioPlugin.h"
#include "Plugins/GraphicsPlugin.h"

#include "ConfigOptions.h"

#ifndef DAEDALUS_SILENT
//*****************************************************************************
//
//*****************************************************************************
const char *gGameHackNames[13] = 
{
	"No Game Specific Hack",
	"TLB Hack", 
	"Viewport Hack", 
	"Conker's Flushtris Hack", 
	"Pilot Wings' Flushtris Hack",
	"Zelda OOT Hacks", 
	"Zelda MM Hacks",
	"Flat Shade Disabled Hack",
	"Rumble Pak Disabled Hack",
	"1/2 ZBuffer Hack",
	"Depth Hack",
	"Width Hack"
};
#endif

//*****************************************************************************
// This isn't really the most appropriate place. Need to check with
// the graphics plugin really
//*****************************************************************************
u32 g_dwNumFrames = 0;

//*****************************************************************************
//
//*****************************************************************************
RomInfo g_ROM;


#ifndef DAEDALUS_SILENT
//*****************************************************************************
//
//*****************************************************************************
void DumpROMInfo( const ROMHeader & header )
{
	// The "Header" is actually something to do with the PI_DOM_*_OFS values...
	DBGConsole_Msg(0, "Header:			0x%02x%02x%02x%02x", header.x1, header.x2, header.x3, header.x4);
	DBGConsole_Msg(0, "Clockrate:       0x%08x", header.ClockRate);
	DBGConsole_Msg(0, "BootAddr:		0x%08x", SwapEndian(header.BootAddress));
	DBGConsole_Msg(0, "Release:         0x%08x", header.Release);
	DBGConsole_Msg(0, "CRC1:            0x%08x", header.CRC1);
	DBGConsole_Msg(0, "CRC2:            0x%08x", header.CRC2);
	DBGConsole_Msg(0, "Unknown0:        0x%08x", header.Unknown0);
	DBGConsole_Msg(0, "Unknown1:        0x%08x", header.Unknown1);
	DBGConsole_Msg(0, "ImageName:       '%s'",   header.Name);
	DBGConsole_Msg(0, "Unknown2:        0x%08x", header.Unknown2);
	DBGConsole_Msg(0, "Unknown3:        0x%04x", header.Unknown3);
	DBGConsole_Msg(0, "Unknown4:        0x%02x", header.Unknown4);
	DBGConsole_Msg(0, "Manufacturer:    0x%02x", header.Manufacturer);
	DBGConsole_Msg(0, "CartID:          0x%04x", header.CartID);
	DBGConsole_Msg(0, "CountryID:       0x%02x - '%c'", header.CountryID, (char)header.CountryID);
	DBGConsole_Msg(0, "Unknown5:        0x%02x", header.Unknown5);
}
#endif
//*****************************************************************************
//
//*****************************************************************************
/*
static void ROM_RunPIFBoot( ECicType cic_chip )
{
	CPU_SetPC(0xBFC00000);

	// Set up CIC value in 0xbfc007e4
	*(u32*)((u8*)g_pMemoryBuffers[MEM_PIF_RAM] + 0x7C0 + 0x24) = ROM_GetCicValue( cic_chip );
}
*/
//*****************************************************************************
//
//*****************************************************************************
static void ROM_SimulatePIFBoot( ECicType cic_chip, u32 Country )
{
	// gCPUState.CPUControl[C0_SR]		= 0x34000000;	//*SR_FR |*/ SR_ERL | SR_CU2|SR_CU1|SR_CU0;
	R4300_SetSR(0x34000000);
	gCPUState.CPUControl[C0_CONFIG]._u64	= 0x0006E463;	// 0x00066463;

	// Copy low 1000 bytes to MEM_SP_MEM
	RomBuffer::GetRomBytesRaw( (u8*)g_pMemoryBuffers[MEM_SP_MEM] + RAMROM_BOOTSTRAP_OFFSET,
		   RAMROM_BOOTSTRAP_OFFSET,
		   RAMROM_GAME_OFFSET - RAMROM_BOOTSTRAP_OFFSET );

	// Need to copy crap to SP_IMEM for CIC-6105 boot.
	u8 * pIMemBase = (u8*)g_pMemoryBuffers[ MEM_SP_MEM ] + 0x1000;

	gCPUState.CPUControl[C0_COUNT]._u64 = 0x5000;
	gCPUState.CPUControl[C0_CAUSE]._u64 = 0x0000005C;
	//ENTRYHI_REGISTER	  = 0xFFFFE0FF;
	gCPUState.CPUControl[C0_CONTEXT]._u64 = 0x007FFFF0;
	gCPUState.CPUControl[C0_EPC]._u64 = 0xFFFFFFFF;
	gCPUState.CPUControl[C0_BADVADDR]._u64 = 0xFFFFFFFF;
	gCPUState.CPUControl[C0_ERROR_EPC]._u64= 0xFFFFFFFF;
	gCPUState.CPUControl[C0_CONFIG]._u64= 0x0006E463;
	
	//REVISION_REGISTER   = 0x00000511;
	//STATUS_REGISTER     = 0x34000000;


	gGPR[0]._u64=0x0000000000000000LL;
	gGPR[6]._u64=0xFFFFFFFFA4001F0CLL;
	gGPR[7]._u64=0xFFFFFFFFA4001F08LL;
	gGPR[8]._u64=0x00000000000000C0LL;
	gGPR[9]._u64=0x0000000000000000LL;
	gGPR[10]._u64=0x0000000000000040LL;
	gGPR[11]._u64=0xFFFFFFFFA4000040LL;
	gGPR[16]._u64=0x0000000000000000LL;
	gGPR[17]._u64=0x0000000000000000LL;
	gGPR[18]._u64=0x0000000000000000LL;
	gGPR[19]._u64=0x0000000000000000LL;
	gGPR[21]._u64=0x0000000000000000LL; 
	gGPR[26]._u64=0x0000000000000000LL;
	gGPR[27]._u64=0x0000000000000000LL;
	gGPR[28]._u64=0x0000000000000000LL;
	gGPR[29]._u64=0xFFFFFFFFA4001FF0LL;
	gGPR[30]._u64=0x0000000000000000LL;

	switch (Country) {
		case 0x44: //Germany
		case 0x46: //french
		case 0x49: //Italian
		case 0x50: //Europe
		case 0x53: //Spanish
		case 0x55: //Australia
		case 0x58: // ????
		case 0x59: // X (PAL)
			switch (cic_chip) {
				case CIC_6102:
					gGPR[5]._u64=0xFFFFFFFFC0F1D859LL;
					gGPR[14]._u64=0x000000002DE108EALL;
					gGPR[24]._u64=0x0000000000000000LL;
					break;
				case CIC_6103:
					gGPR[5]._u64=0xFFFFFFFFD4646273LL;
					gGPR[14]._u64=0x000000001AF99984LL;
					gGPR[24]._u64=0x0000000000000000LL;
					break;
				case CIC_6105:
					*(u32 *)&pIMemBase[0x04] = 0xBDA807FC;
					gGPR[5]._u64=0xFFFFFFFFDECAAAD1LL;
					gGPR[14]._u64=0x000000000CF85C13LL;
					gGPR[24]._u64=0x0000000000000002LL;
					break;
				case CIC_6106:
					gGPR[5]._u64=0xFFFFFFFFB04DC903LL;
					gGPR[14]._u64=0x000000001AF99984LL;
					gGPR[24]._u64=0x0000000000000002LL;
					break;
				default:
					break;
			}

			gGPR[20]._u64=0x0000000000000000LL;
			gGPR[23]._u64=0x0000000000000006LL;
			gGPR[31]._u64=0xFFFFFFFFA4001554LL;
			break;
		case 0x37: // 7 (Beta)
		case 0x41: // ????
		case 0x45: //USA
		case 0x4A: //Japan
		default:
			switch (cic_chip) {
				case CIC_6102:
					gGPR[5]._u64=0xFFFFFFFFC95973D5LL;
					gGPR[14]._u64=0x000000002449A366LL;
					break;
				case CIC_6103:
					gGPR[5]._u64=0xFFFFFFFF95315A28LL;
					gGPR[14]._u64=0x000000005BACA1DFLL;
					break;
				case CIC_6105:
					*(u32  *)&pIMemBase[0x04] = 0x8DA807FC;
					gGPR[5]._u64=0x000000005493FB9ALL;
					gGPR[14]._u64=0xFFFFFFFFC2C20384LL;
				case CIC_6106:
					gGPR[5]._u64=0xFFFFFFFFE067221FLL;
					gGPR[14]._u64=0x000000005CD2B70FLL;
					break;
				default:
					break;
			}
			gGPR[20]._u64=0x0000000000000001LL;
			gGPR[23]._u64=0x0000000000000000LL;
			gGPR[24]._u64=0x0000000000000003LL;
			gGPR[31]._u64=0xFFFFFFFFA4001550LL;
	}

	switch (cic_chip) {
		case CIC_6101: 
			gGPR[22]._u64=0x000000000000003FLL; 
			break;
		case CIC_6102: 
			gGPR[1]._u64=0x0000000000000001LL;
			gGPR[2]._u64=0x000000000EBDA536LL;
			gGPR[3]._u64=0x000000000EBDA536LL;
			gGPR[4]._u64=0x000000000000A536LL;
			gGPR[12]._u64=0xFFFFFFFFED10D0B3LL;
			gGPR[13]._u64=0x000000001402A4CCLL;
			gGPR[15]._u64=0x000000003103E121LL;
			gGPR[22]._u64=0x000000000000003FLL; 
			gGPR[25]._u64=0xFFFFFFFF9DEBB54FLL;
			break;
		case CIC_6103: 
			gGPR[1]._u64=0x0000000000000001LL;
			gGPR[2]._u64=0x0000000049A5EE96LL;
			gGPR[3]._u64=0x0000000049A5EE96LL;
			gGPR[4]._u64=0x000000000000EE96LL;
			gGPR[12]._u64=0xFFFFFFFFCE9DFBF7LL;
			gGPR[13]._u64=0xFFFFFFFFCE9DFBF7LL;
			gGPR[15]._u64=0x0000000018B63D28LL;
			gGPR[22]._u64=0x0000000000000078LL; 
			gGPR[25]._u64=0xFFFFFFFF825B21C9LL;
			break;
		case CIC_6105: 
			*(u32  *)&pIMemBase[0x00] = 0x3C0DBFC0;
			*(u32  *)&pIMemBase[0x08] = 0x25AD07C0;
			*(u32  *)&pIMemBase[0x0C] = 0x31080080;
			*(u32  *)&pIMemBase[0x10] = 0x5500FFFC;
			*(u32  *)&pIMemBase[0x14] = 0x3C0DBFC0;
			*(u32  *)&pIMemBase[0x18] = 0x8DA80024;
			*(u32  *)&pIMemBase[0x1C] = 0x3C0BB000;
			gGPR[1]._u64=0x0000000000000000LL;
			gGPR[2]._u64=0xFFFFFFFFF58B0FBFLL;
			gGPR[3]._u64=0xFFFFFFFFF58B0FBFLL;
			gGPR[4]._u64=0x0000000000000FBFLL;
			gGPR[12]._u64=0xFFFFFFFF9651F81ELL;
			gGPR[13]._u64=0x000000002D42AAC5LL;
			gGPR[15]._u64=0x0000000056584D60LL;
			gGPR[22]._u64=0x0000000000000091LL; 
			gGPR[25]._u64=0xFFFFFFFFCDCE565FLL;
			break;
		case CIC_6106: 
			gGPR[1]._u64=0x0000000000000000LL;
			gGPR[2]._u64=0xFFFFFFFFA95930A4LL;
			gGPR[3]._u64=0xFFFFFFFFA95930A4LL;
			gGPR[4]._u64=0x00000000000030A4LL;
			gGPR[12]._u64=0xFFFFFFFFBCB59510LL;
			gGPR[13]._u64=0xFFFFFFFFBCB59510LL;
			gGPR[15]._u64=0x000000007A3C07F4LL;
			gGPR[22]._u64=0x0000000000000085LL; 
			gGPR[25]._u64=0x00000000465E3F72LL;
			break;
		default:
			break;
	}

	// Also need to set up PI_BSD_DOM1 regs etc!
	CPU_SetPC(0xA4000040);
}

//*****************************************************************************
// Get the name of the correct pit for the region
//*****************************************************************************
/*
static const char * ROM_GetPIFName( u32 tv_type )
{
	switch ( tv_type )
	{
		case OS_TV_PAL:		return "pal.pif";
		case OS_TV_NTSC:	return "ntsc.pif";
		case OS_TV_MPAL:	return "mpal.pif";
	}
	DAEDALUS_ERROR( "Unknown TV type" );
	return NULL;
}
*/

//*****************************************************************************
// Load pif bootcode if it is available
//*****************************************************************************
/*
static bool ROM_LoadPIF( u32 tv_type )
{
	const char * pif_name;
	char filename[ MAX_PATH ];
	FILE * fp;
	u32 op;

	pif_name = ROM_GetPIFName( tv_type );
	if ( !pif_name )
	{
		return false;
	}

	IO::Path::Combine( filename, gDaedalusExePath, "Boot\\" );
	IO::Path::Append( filename, pif_name );

	fp = fopen( filename, "rb" );
	if ( fp )
	{
		u32 * pPifRom = (u32*)g_pMemoryBuffers[MEM_PIF_RAM];
		u32 i = 0;
		while (fread(&op, 4, 1, fp) == 1 && i < MemoryRegionSizes[MEM_PIF_RAM])
		{
			*pPifRom = SwapEndian( op );
			pPifRom++;
			i+=4;
		}

		fclose(fp);

		return true;
	}
	else
	{
		DBGConsole_Msg( 0, "Couldn't find '[W%s]', simulating boot", filename );
		return false;
	}
}
*/

//*****************************************************************************
//
//*****************************************************************************
void ROM_ReBoot()
{
	//
	// Find out the CIC type and initialise various systems based on the CIC type
	//
	u8			rom_base[ RAMROM_GAME_OFFSET ];
	RomBuffer::GetRomBytesRaw( rom_base, 0, RAMROM_GAME_OFFSET );

	g_ROM.cic_chip = ROM_GenerateCICType( rom_base );

#ifndef DAEDALUS_SILENT
	if (g_ROM.cic_chip == CIC_UNKNOWN)
	{
		//DAEDALUS_ERROR( "Unknown CIC CRC: 0x%08x\nAssuming CIC-6102", crc );
		//DBGConsole_Msg(0, "[MUnknown CIC CRC: 0x%08x]", crc );
		DBGConsole_Msg(0, "[MUnknown CIC]" );
	}
	else
	{
		DBGConsole_Msg(0, "[MRom uses %s]", ROM_GetCicName( g_ROM.cic_chip ) );

/* goodn64 already tell us if the rom is good or bad
		u32 crc1;
		u32 crc2;
		if( ROM_DoCicCheckSum( g_ROM.cic_chip, &crc1, &crc2 ) )
		{
			if (crc1 != RomBuffer::ReadValueRaw< u32 >( 0x10 ) ||
				crc2 != RomBuffer::ReadValueRaw< u32 >( 0x14 ))
			{
				DBGConsole_Msg(0, "[MWarning, CRC values don't match, fixing]");

				RomBuffer::WriteValueRaw< u32 >( 0x10, crc1 );
				RomBuffer::WriteValueRaw< u32 >( 0x14, crc2 );
			}
		}
		else
		{
			// Unable to checksum - just continue with what we have
		}
*/
	}
#endif
	// XXXX Update this rom's boot info

	g_dwNumFrames = 0;
#ifdef DAEDALUS_ENABLE_OS_HOOKS
	Patch_Reset();
#endif

	// This will always return false since we always simulate boot code instead of loading from file
	//
	/*if ( ROM_LoadPIF( g_ROM.TvType ) )
	{
		ROM_RunPIFBoot( g_ROM.cic_chip );
	}
	else
	{
		ROM_SimulatePIFBoot( g_ROM.cic_chip, g_ROM.rh.CountryID );
	}*/
	ROM_SimulatePIFBoot( g_ROM.cic_chip, g_ROM.rh.CountryID );

	//CPU_AddBreakPoint( 0xA4000040 );
	//CPU_AddBreakPoint( 0xbfc00000 );
#ifdef DAEDALUS_DEBUG_CONSOLE
	CDebugConsole::Get()->UpdateDisplay();
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void ROM_Unload()
{

}

//*****************************************************************************
//
//*****************************************************************************
void SpecificGameHacks( const ROMHeader & id )
{
	switch( id.CartID )
	{
	case 0x4547: g_ROM.GameHacks = GOLDEN_EYE;			break;
	case 0x5742: g_ROM.GameHacks = SUPER_BOWLING;		break;
	case 0x5546: g_ROM.GameHacks = CONKER;				break;
	case 0x5750: g_ROM.GameHacks = PILOT_WINGS;			break;
	case 0x4c5a: g_ROM.GameHacks = ZELDA_OOT;			break;
	case 0x535a: g_ROM.GameHacks = ZELDA_MM;			break;
	case 0x3954: g_ROM.GameHacks = TIGERS_HONEY_HUNT;	break;
	case 0x5443: g_ROM.GameHacks = CHAMELEON_TWIST;		break;
	case 0x4f52: g_ROM.GameHacks = ROAD_RASH;			break;
	case 0x524c: g_ROM.GameHacks = LOAD_RUNNER;			break;
	case 0x494b: g_ROM.GameHacks = KILLER_INSTINCT;		break;
	case 0x324e: 
	case 0x4339:
		g_ROM.GameHacks = NASCAR;
		break;
	default:
		g_ROM.GameHacks = NO_GAME_HACK;
		break;
	}
}

//*****************************************************************************
// Copy across text, null terminate, and strip spaces
//*****************************************************************************
void ROM_GetRomNameFromHeader( std::string & rom_name, const ROMHeader & header )
{
	char	buffer[20+1];
	memcpy( buffer, header.Name, 20 );
	buffer[20] = '\0';

	rom_name = buffer;

	const char * whitespace_chars( " \t\r\n" );
	rom_name.erase( 0, rom_name.find_first_not_of(whitespace_chars) );
	rom_name.erase( rom_name.find_last_not_of(whitespace_chars)+1 );
}

//*****************************************************************************
//
//*****************************************************************************
void ROM_LoadFile()
{
	RomID		rom_id;
	u32			rom_size;
	ECicType	boot_type;

	if(ROM_GetRomDetailsByFilename(g_ROM.szFileName, &rom_id, &rom_size, &boot_type ))
	{
		RomSettings			settings;
		SRomPreferences		preferences;

		if ( !CRomSettingsDB::Get()->GetSettings( rom_id, &settings ) )
		{
			settings.Reset();
		}
		if( !CPreferences::Get()->GetRomPreferences( rom_id, &preferences ) )
		{
			preferences.Reset();
		}

		ROM_LoadFile( rom_id, settings, preferences );
	}

	return;
}


void ROM_UnloadFile()
{
	// Copy across various bits
	g_ROM.mRomID = RomID();
	g_ROM.settings = RomSettings();
}
//*****************************************************************************
//
//*****************************************************************************
bool ROM_LoadFile(const RomID & rom_id, const RomSettings & settings, const SRomPreferences & preferences )
{
#ifndef DAEDALUS_SILENT
	const char *filename(g_ROM.szFileName);
	DBGConsole_Msg(0, "Reading rom image: [C%s]", filename);
#endif
	// Get information about the rom header
	RomBuffer::GetRomBytesRaw( &g_ROM.rh, 0, sizeof(ROMHeader) );
	
	//
	//	Swap into native format
	//
	ROMFile::ByteSwap_3210( &g_ROM.rh, sizeof(ROMHeader) );

	DAEDALUS_ASSERT( RomID( g_ROM.rh ) == rom_id, "Why is the rom id incorrect?" );

	// Copy across various bits
	g_ROM.mRomID = rom_id;
	g_ROM.settings = settings;
	g_ROM.TvType = ROM_GetTvTypeFromID( g_ROM.rh.CountryID );

#ifndef DAEDALUS_SILENT
	DumpROMInfo( g_ROM.rh );
#endif
	//
	//
	//
	preferences.Apply();

	// Game specific hacks..
	SpecificGameHacks( g_ROM.rh );

	DBGConsole_Msg(0, "[G%s]", g_ROM.settings.GameName.c_str());
	DBGConsole_Msg(0, "This game has been certified as [G%s] (%s)", g_ROM.settings.Comment.c_str(), g_ROM.settings.Info.c_str());
	DBGConsole_Msg(0, "SaveType: [G%s]", ROM_GetSaveTypeName( g_ROM.settings.SaveType ) );
	DBGConsole_Msg(0, "ApplyPatches: [G%s]", gOSHooksEnabled ? "on" : "off");
	DBGConsole_Msg(0, "Check Texture Hash Freq: [G%d]", gCheckTextureHashFrequency);
	DBGConsole_Msg(0, "SpeedSync: [G%s]", gSpeedSyncEnabled ? "on" : "off");
	DBGConsole_Msg(0, "DynaRec: [G%s]", gDynarecEnabled ? "on" : "off");
	DBGConsole_Msg(0, "Aplying: [G%s]", gGameHackNames[g_ROM.GameHacks]);

	//Patch_ApplyPatches();

	return true;
}

//*****************************************************************************
//
//*****************************************************************************
bool ROM_GetRomName( const char * filename, std::string & game_name )
{
	ROMFile * p_rom_file( ROMFile::Create( filename ) );
	if(p_rom_file == NULL)
	{
		return false;
	}

	CNullOutputStream messages;

	if( !p_rom_file->Open( messages ) )
	{
		delete p_rom_file;
		return false;
	}

	// Only read in the header
	const u32	bytes_to_read( sizeof(ROMHeader) );
	u8 *		p_bytes;
	u32			buffer_size;
	if(!p_rom_file->LoadDataChunk( bytes_to_read, &p_bytes, &buffer_size, messages ))
	{
		// Lots of files don't have any info - don't worry about it
		delete p_rom_file;
		return false;
	}

	//
	//	Swap into native format
	//
	ROMFile::ByteSwap_3210( p_bytes, buffer_size );

	//
	// Get the address of the rom header
	// Setup the rom id and size
	//
	const ROMHeader * prh( reinterpret_cast<const ROMHeader *>( p_bytes ) );
	ROM_GetRomNameFromHeader( game_name, *prh );

	delete [] p_bytes;
	delete p_rom_file;
	return true;
}

//*****************************************************************************
//
//*****************************************************************************
bool ROM_GetRomDetailsByFilename( const char * filename, RomID * id, u32 * rom_size, ECicType * boot_type )
{
	return CRomDB::Get()->QueryByFilename( filename, id, rom_size, boot_type );
}

//*****************************************************************************
//
//*****************************************************************************
bool ROM_GetRomDetailsByID( const RomID & id, u32 * rom_size, ECicType * boot_type )
{
	return CRomDB::Get()->QueryByID( id, rom_size, boot_type );
}

//*****************************************************************************
// Association between a country id value, tv type and name
//*****************************************************************************
struct CountryIDInfo
{
	s8				CountryID;
	const char *	CountryName;
	u32				TvType;
};

static const CountryIDInfo g_CountryCodeInfo[] =
{
	{  0,  "0",			OS_TV_NTSC },
	{ '7', "Beta",		OS_TV_NTSC },
	{ 'A', "NTSC",		OS_TV_NTSC },
	{ 'D', "Germany",	OS_TV_PAL },
	{ 'E', "USA",		OS_TV_NTSC },
	{ 'F', "France",	OS_TV_PAL },
	{ 'I', "Italy",		OS_TV_PAL },
	{ 'J', "Japan",		OS_TV_NTSC },
	{ 'P', "Europe",	OS_TV_PAL },
	{ 'S', "Spain",		OS_TV_PAL },
	{ 'U', "Australia", OS_TV_PAL },
	{ 'X', "PAL",		OS_TV_PAL },
	{ 'Y', "PAL",		OS_TV_PAL }
};



//*****************************************************************************
// Get a string representing the country name from an ID value
//*****************************************************************************
const char * ROM_GetCountryNameFromID( u8 country_id )
{
	for ( u32 i = 0; i < ARRAYSIZE( g_CountryCodeInfo ); i++)
	{
		if ( g_CountryCodeInfo[i].CountryID == country_id )
		{
			return g_CountryCodeInfo[i].CountryName;
		}
	}
	return "?";
}


//*****************************************************************************
//
//*****************************************************************************
u32 ROM_GetTvTypeFromID( u8 country_id )
{
	for ( u32 i = 0; i < ARRAYSIZE( g_CountryCodeInfo ); i++ )
	{
		if ( g_CountryCodeInfo[i].CountryID == country_id )
		{
			return g_CountryCodeInfo[i].TvType;
		}
	}
	return OS_TV_NTSC;
}

