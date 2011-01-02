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

/*
enum UCodeVersion
{
	FAST3D,
	F3DEX,
	F3DLX,
	F3DLP,
	S2DEX
	//L3DEX
};
*/
enum GBIVersion
{
	GBI_0 = 0,
	GBI_1,
	GBI_2,
	GBI_0_WR,
	GBI_0_DKR,
	GBI_0_JFG,
	GBI_0_LL,
	GBI_0_SE,
	GBI_0_GE,
	GBI_0_CK,
	GBI_0_PD,
	S2DEX_GBI_1,
	GBI_0_UNK	// This always has to be the last one
};
/*
struct LastUcodeInfo
{
	bool bUcodeKnown;
	u32	 ucStart;
};
*/
struct UcodeInfo
{
	bool used;
	u32	ucode;
	
	u32	code_base;
	u32	code_size;
	u32	data_base;
	//u32	data_size;
};

u32	GBIMicrocode_DetectVersion( u32 code_base, u32 code_size, u32 data_base, u32 data_size );
/*
struct MicroCodeCommand
{
	union
	{
		u64		_u64;

		struct
		{
			u32		cmd1;
			u32		cmd0;
		};

		struct
		{
			int		: 32;
			int		: 24;
			unsigned int		cmd : 8;
		};
	};
};
*/

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
u32				GBIMicrocode_GetMicrocodeHistoryStringCount();
const char *	GBIMicrocode_GetMicrocodeHistoryString( u32 i );
void			GBIMicrocode_ResetMicrocodeHistory();
#endif


#endif // MICROCODE_H__
