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
//*****************************************************************************
// Find out the CIC type
//*****************************************************************************
ECicType ROM_GenerateCICType( const u8 * p_rom_base )
{
	u32	cic = 0;

	for(u32 i = 0; i < 0xFC0; i++)
	{
		cic = cic + (u8) p_rom_base[0x40 + i];
	}

	switch( cic )
	{
	case 0x33a27:	return CIC_6101;	// TWINE
	case 0x3421e:	return CIC_6101;	// Starfox
	case 0x34044:	return CIC_6102;	// Mario
	case 0x357d0:	return CIC_6103;	// Banjo
	case 0x47a81:	return CIC_6105;	// Zelda
	case 0x371cc:	return CIC_6106;	// F-Zero
	case 0x343c9:	return CIC_6106;	// ???
	default:
		DAEDALUS_ERROR("Unknown CIC Code");
		return CIC_UNKNOWN;
	}
}

//*****************************************************************************
//
//*****************************************************************************
const char * ROM_GetCicName( ECicType cic_type )
{
	switch(cic_type)
	{
	case CIC_6101:	return "CIC-6101";
	case CIC_6102:	return "CIC-6102";
	case CIC_6103:	return "CIC-6103";
	case CIC_6105:	return "CIC-6105";
	case CIC_6106:	return "CIC-6106";
	default:		return "?";
	}
}


