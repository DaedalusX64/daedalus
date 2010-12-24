/*
Copyright (C) 2007 StrmnNrmn

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
#include "ROMImage.h"

#include "Utility/CRC.h"

#include "Debug/DBGConsole.h"

#include "Core/ROMBuffer.h"

//*****************************************************************************
// CRC / CIC values
//*****************************************************************************
#define CIC_6101_BOOT_CRC 0xb4086651		// CIC-6101
#define CIC_6102_BOOT_CRC 0xb9c47dc8		// CIC-6102 or CIC-7101
#define CIC_6103_BOOT_CRC 0xedce5ad9		// CIC-6103 or CIC-7103
#define CIC_6104_BOOT_CRC 0xb3d6a525		// CIC-6104?   CIC-7102
#define CIC_6105_BOOT_CRC 0xb53c588a		// CIC-6105 or CIC-7105
#define CIC_6106_BOOT_CRC 0x06d8ed9c		// CIC-6106 or CIC-7106

#define CIC_6101_BOOT_CIC 0x00023f3f
#define CIC_6102_BOOT_CIC 0x00063f3f
#define CIC_6103_BOOT_CIC 0x0002783f
#define CIC_6104_BOOT_CIC 0x00023f3f		// Incorrect?
#define CIC_6105_BOOT_CIC 0x0002913f
#define CIC_6106_BOOT_CIC 0x0002853f		// Same as FZero???

struct SCicInfo
{
	const char *	Name;		// Name of the CIC chip
	u32				BootCRC;	// CRC of the bootcode
	u32				CICValues;	// CIC security values
	bool (* CheckSumProc)( u32 * crc1, u32 * crc2 );	// Function to calculate/fix checksum
};

#define CIC_CHIP_ENTRY( c )		{ "CIC-"#c, CIC_##c##_BOOT_CRC, CIC_##c##_BOOT_CIC, ROM_CheckSumCic##c }

// Hmmm, feel like these really should live elsewhere, as they chew on the loaded rom's data
static bool ROM_CheckSumCic6101( u32 * crc1, u32 * crc2 );
static bool ROM_CheckSumCic6102( u32 * crc1, u32 * crc2 );
static bool ROM_CheckSumCic6103( u32 * crc1, u32 * crc2 );
static bool ROM_CheckSumCic6104( u32 * crc1, u32 * crc2 );
static bool ROM_CheckSumCic6105( u32 * crc1, u32 * crc2 );
static bool ROM_CheckSumCic6106( u32 * crc1, u32 * crc2 );

static const SCicInfo gCICInfo[] = 
{
	CIC_CHIP_ENTRY( 6101 ),
	CIC_CHIP_ENTRY( 6102 ),
	CIC_CHIP_ENTRY( 6103 ),
	CIC_CHIP_ENTRY( 6104 ),
	CIC_CHIP_ENTRY( 6105 ),
	CIC_CHIP_ENTRY( 6106 ),
};
DAEDALUS_STATIC_ASSERT( ARRAYSIZE( gCICInfo ) == NUM_CIC_CHIPS );


//*****************************************************************************
// Find out the CIC type
//*****************************************************************************
ECicType ROM_GenerateCICType( const u8 * p_rom_base )
{
	u32		crc( daedalus_crc32(0, p_rom_base + RAMROM_BOOTSTRAP_OFFSET, RAMROM_GAME_OFFSET - RAMROM_BOOTSTRAP_OFFSET) );

	for ( u32 i = 0; i < NUM_CIC_CHIPS; i++ )
	{
		if ( crc == gCICInfo[ i ].BootCRC )
		{
			return static_cast< ECicType >( i );
		}
	}

	return CIC_UNKNOWN;
}

//*****************************************************************************
//
//*****************************************************************************
const char * ROM_GetCicName( ECicType cic_type )
{
	if ( cic_type >= 0 && cic_type < NUM_CIC_CHIPS )
	{
		return gCICInfo[ cic_type ].Name;
	}

	return "?";
}

//*****************************************************************************
//
//*****************************************************************************
u32		 ROM_GetCicValue( ECicType cic_type )
{
	if ( cic_type >= 0 && cic_type < NUM_CIC_CHIPS )
	{
		return gCICInfo[ cic_type ].CICValues;
	}

	return 0;
}

//*****************************************************************************
//
//*****************************************************************************
bool	ROM_DoCicCheckSum( ECicType cic_type, u32 * crc1, u32 * crc2 )
{
	if ( cic_type >= 0 && cic_type < NUM_CIC_CHIPS )
	{
		return gCICInfo[ cic_type ].CheckSumProc( crc1, crc2 );
	}

	return false;
}

//*****************************************************************************
//
//*****************************************************************************
bool ROM_CheckSumCic6101( u32 * crc1, u32 * crc2 )
{
#ifdef DAEDALUS_DEBUG_CONSOLE
	CDebugConsole::Get()->Msg( 0, "[MUnable to check CRC for CIC-6101]" );	
#endif
	return false;
}

//*****************************************************************************
//
//*****************************************************************************
bool ROM_CheckSumCic6102( u32 * crc1, u32 * crc2 )
{
	u32 address;
	u32 a1;
	u32 t7;
	u32 v1 = 0;
	u32 t0 = 0;
	u32 v0 = 0xF8CA4DDC; //(CIC_6102_BOOT_CIC * 0x5d588b65) + 1;
	u32 a3 = 0xF8CA4DDC;
	u32 t2 = 0xF8CA4DDC;
	u32 t3 = 0xF8CA4DDC;
	u32 s0 = 0xF8CA4DDC;
	u32 a2 = 0xF8CA4DDC;
	u32 t4 = 0xF8CA4DDC;
	u32 t8, t6, a0;

#ifdef DAEDALUS_DEBUG_CONSOLE
	CDebugConsole::Get()->MsgOverwriteStart();
#endif

	for (address = 0; address < 0x00100000; address+=4)
	{
#ifdef DAEDALUS_DEBUG_CONSOLE
		if ((address % 0x2000) == 0)
		{
			CDebugConsole::Get()->MsgOverwrite(0, "Generating CRC [M%d / %d]", address, 0x00100000);
		}
#endif
		v0 = RomBuffer::ReadValueRaw< u32 >( address + RAMROM_GAME_OFFSET );
		v1 = a3 + v0;
		a1 = v1;
		if (v1 < a3) 
			t2++;
	
		v1 = v0 & 0x001f;
		t7 = 0x20 - v1;
		t8 = (v0 >> (t7&0x1f));
		t6 = (v0 << (v1&0x1f));
		a0 = t6 | t8;

		a3 = a1;
		t3 ^= v0;
		s0 += a0;
		if (a2 < v0)
			a2 ^= a3 ^ v0;
		else
			a2 ^= a0;

		t0 += 4;
		t7 = v0 ^ s0;
		t4 += t7;
	}

	a3 ^= t2 ^ t3;	// CRC1
	s0 ^= a2 ^ t4;	// CRC2

#ifdef DAEDALUS_DEBUG_CONSOLE
	CDebugConsole::Get()->MsgOverwrite(0, "Generating CRC [M%d / %d]", 0x00100000, 0x00100000);
	CDebugConsole::Get()->MsgOverwriteEnd();
#endif

	*crc1 = a3;
	*crc2 = s0;
	return true;
}

//*****************************************************************************
//
//*****************************************************************************
bool ROM_CheckSumCic6103( u32 * crc1, u32 * crc2 )
{
#ifdef DAEDALUS_DEBUG_CONSOLE
	CDebugConsole::Get()->Msg( 0, "[MUnable to check CRC for CIC-6103]" );	
#endif
	return false;
}

//*****************************************************************************
//
//*****************************************************************************
bool ROM_CheckSumCic6104( u32 * crc1, u32 * crc2 )
{
#ifdef DAEDALUS_DEBUG_CONSOLE
	CDebugConsole::Get()->Msg( 0, "[MUnable to check CRC for CIC-6104]" );	
#endif
	return false;
}


//*****************************************************************************
// Thanks Lemmy!
//*****************************************************************************
bool ROM_CheckSumCic6105( u32 * crc1, u32 * crc2 )
{
	u32 address;
	u32 address2;
	u32 t5 = 0x00000020;
	u32 a3 = 0xDF26F436;
	u32 t2 = 0xDF26F436;
	u32 t3 = 0xDF26F436;
	u32 s0 = 0xDF26F436;
	u32 a2 = 0xDF26F436;
	u32 t4 = 0xDF26F436;
	u32 v0, v1, a1, a0;

	address2 = 0;

#ifdef DAEDALUS_DEBUG_CONSOLE
	CDebugConsole::Get()->MsgOverwriteStart();
#endif

	for (address = 0; address < 0x00100000; address += 4)
	{
#ifdef DAEDALUS_DEBUG_CONSOLE
		if ((address % 0x2000) == 0)
			CDebugConsole::Get()->MsgOverwrite(0, "Generating CRC [M%d / %d]", address, 0x00100000);
#endif

		v0 = RomBuffer::ReadValueRaw< u32 >( address + RAMROM_GAME_OFFSET );
		v1 = a3 + v0;
		a1 = v1;
		
		if (v1 < a3)
			t2++;

		v1 = v0 & 0x1f;
		a0 = (v0 >> (t5-v1)) | (v0 << v1);
		a3 = a1;
		t3 = t3 ^ v0;
		s0 += a0;
		if (a2 < v0)
			a2 ^= a3 ^ v0;
		else
			a2 ^= a0;

		t4 += RomBuffer::ReadValueRaw< u32 >( address2 + 0x750 ) ^ v0;
		address2 = (address2 + 4) & 0xFF;

	}

	a3 ^= t2 ^ t3;
	s0 ^= a2 ^ t4;

#ifdef DAEDALUS_DEBUG_CONSOLE
	CDebugConsole::Get()->MsgOverwrite(0, "Generating CRC [M%d / %d]", 0x00100000, 0x00100000);
	CDebugConsole::Get()->MsgOverwriteEnd();
#endif

	*crc1 = a3;
	*crc2 = s0;
	return true;
}

//*****************************************************************************
//
//*****************************************************************************
bool ROM_CheckSumCic6106( u32 * crc1, u32 * crc2 )
{
#ifdef DAEDALUS_DEBUG_CONSOLE
	CDebugConsole::Get()->Msg( 0, "[MUnable to check CRC for CIC-6106]" );	
#endif
	return false;
}

