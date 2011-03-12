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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef DAEDALUS_ROM_H_
#define DAEDALUS_ROM_H_

#include "ROMImage.h"


inline u32 SwapEndian( u32 x )
{
	return ((x >> 24)&0x000000FF) |
		   ((x >> 8 )&0x0000FF00) |
		   ((x << 8 )&0x00FF0000) |
		   ((x << 24)&0xFF000000);
}

class RomID
{
	public: 
		RomID()
		{
			CRC[0] = 0;
			CRC[1] = 0;
			CountryID = 0;
		}

		RomID( u32 crc1, u32 crc2, u8 country_id )
		{
			CRC[0] = crc1;
			CRC[1] = crc2;
			CountryID = country_id;
		}

		explicit RomID( const ROMHeader & header )
		{
			CRC[0] = header.CRC1;
			CRC[1] = header.CRC2;
			CountryID = header.CountryID;
		}

		bool Empty() const
		{
			return CRC[0] == 0 && CRC[1] == 0 && CountryID == 0;
		}

		bool operator==( const RomID & id ) const		{ return Compare( id ) == 0; }
		bool operator!=( const RomID & id ) const		{ return Compare( id ) != 0; }
		bool operator<( const RomID & rhs ) const		{ return Compare( rhs ) < 0; }

		s32 Compare( const RomID & rhs ) const
		{
			s32		diff;

			diff = CRC[0] - rhs.CRC[0];
			if( diff != 0 )
				return diff;

			diff = CRC[1] - rhs.CRC[1];
			if( diff != 0 )
				return diff;

			diff = CountryID - rhs.CountryID;
			if( diff != 0 )
				return diff;

			return 0;
		}
		
		u32		CRC[2];
		u8		CountryID;
};

#include "RomSettings.h"

struct SRomPreferences;

// Increase this everytime you add a new hack, don't forget to add it in gGameHackNames too !!!
//
#define MAX_HACK_NAMES 22
//
//*****************************************************************************
//	Hacks for games etc.
//*****************************************************************************
enum EGameHacks
{
	NO_GAME_HACK = 0,
	GOLDEN_EYE,
	SUPER_BOWLING,
	PILOT_WINGS,
	ZELDA_OOT,
	ZELDA_MM,
	TIGERS_HONEY_HUNT,
	CHAMELEON_TWIST,
	KIRBY64,
	TARZAN,
	DONALD,
	PMARIO,
	AEROGAUGE,
	GEX_GECKO,
	RIDGE_RACER,
	WONDER_PROJECTJ2,
	ANIMAL_CROSSING,
	CHAMELEON_TWIST_2,
	CLAY_FIGHTER_63,
	BODY_HARVEST,
	AIDYN_CRONICLES
};

//*****************************************************************************
//
//*****************************************************************************
struct RomInfo
{
	char        szFileName[MAX_PATH + 1];
	RomID		mRomID;					// The RomID (unique to this rom)

	ROMHeader	rh;						// Copy of the ROM header, correctly byteswapped
	RomSettings settings;				// Settings for this rom
	u32			TvType;					// OS_TV_NTSC etc
	ECicType	cic_chip;				// CIC boot chip type
	EGameHacks	GameHacks;				// Hacks for specific games
};

//*****************************************************************************
// Functions
//*****************************************************************************
void ROM_ReBoot();
void ROM_Unload();
void ROM_LoadFile();
void ROM_UnloadFile();
bool ROM_LoadFile(const RomID & rom_id, const RomSettings & settings, const SRomPreferences & preferences );

bool ROM_GetRomDetailsByFilename( const char * filename, RomID * id, u32 * rom_size, ECicType * boot_type );
bool ROM_GetRomDetailsByID( const RomID & id, u32 * rom_size, ECicType * boot_type );
bool ROM_GetRomName( const char * filename, std::string & game_name );

const char *	ROM_GetCountryNameFromID( u8 country_id );
u32				ROM_GetTvTypeFromID( u8 country_id );
const char *	ROM_GetCicTypeName( ECicType cic_type );

//*****************************************************************************
// Externs (urgh)
//*****************************************************************************
extern RomInfo g_ROM;
extern u32 g_dwNumFrames;
extern bool gTLUTalt_mode;

#endif // DAEDALUS_ROM_H_
