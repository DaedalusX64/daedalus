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

#pragma once

#ifndef CORE_ROMIMAGE_H_
#define CORE_ROMIMAGE_H_

#include "Base/Types.h"

//const u32 RAMROM_FONTDATA_SIZE		= 1152;
//const u32 RAMROM_CLOCKRATE_MASK		= 0xfffffff0;

//const u32 RAMROM_CLOCKRATE_OFFSET	= 0x4;
//const u32 RAMROM_BOOTADDR_OFFSET	= 0x8;
//const u32 RAMROM_RELEASE_OFFSET	= 0xc;
const u32 RAMROM_BOOTSTRAP_OFFSET	= 0x40;
//const u32 RAMROM_FONTDATA_OFFSET	= 0xb70;
const u32 RAMROM_GAME_OFFSET		= 0x1000;

struct ROMHeader
{
	u8		x1, x2, x3, x4;
	u32		ClockRate;
	u32		BootAddress;
	u32		Release;
	u32		CRC1;
	u32		CRC2;
	u32		Unknown0;
	u32		Unknown1;
	s8		Name[20];
	u32		Unknown2;
	u16		Unknown3;
	u8		Unknown4;
	u8		Manufacturer;
	u16		CartID;
	s8		CountryID;
	u8		Unknown5;
};
DAEDALUS_STATIC_ASSERT( sizeof(ROMHeader) == RAMROM_BOOTSTRAP_OFFSET );

enum ECicType
{
	CIC_UNKNOWN = -1,
	CIC_6101 = 0,
	CIC_6102,
	CIC_6103,
	CIC_6104,
	CIC_6105,
	CIC_6106,

	NUM_CIC_CHIPS
};

ECicType		ROM_GenerateCICType( const u8 * rom_base );

const char *	ROM_GetCicName( ECicType cic_type );

#endif // CORE_ROMIMAGE_H_
