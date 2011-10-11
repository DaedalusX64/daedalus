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

#ifndef MICROCODE_H__
#define MICROCODE_H__

//*****************************************************************************
// Struct
//*****************************************************************************
struct UcodeInfo
{
	u32	ucode;
	u32	code_base;
	u32	data_base;

	bool used;
};
//*****************************************************************************
// Enum
//*****************************************************************************
enum GBIVersion
{
	GBI_0 = 0,
	GBI_1,
	GBI_2,
	GBI_1_S2DEX,
	GBI_WR,
	GBI_DKR,
	GBI_LL,
	GBI_SE,
	GBI_GE,
	GBI_CONKER,
	GBI_PD
};
//*****************************************************************************
// Function
//*****************************************************************************
u32	GBIMicrocode_DetectVersion( u32 code_base, u32 code_size, u32 data_base, u32 data_size );
//*****************************************************************************
// Extern
//*****************************************************************************
extern UcodeInfo current;

#endif // MICROCODE_H__
