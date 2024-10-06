/*
Copyright (C) 2011 StrmnNrmn
Copyright (C) 1999-2004 Joel Middendorf, <schibo@emulation64.com>

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

#ifndef CORE_CHEATS_H_
#define CORE_CHEATS_H_

#include "Base/Types.h"

// Limit the number of cheatcodes on the PSP, for performance reasons
#define MAX_CHEATCODE_PER_LOAD		16

// Max number of entries per cheat
#define MAX_CHEATCODE_PER_ENTRY		100

//Cannot exceed 254 groups, must be represented by using 1 byte
#define MAX_CHEATCODE_GROUP_PER_ROM 254

enum CHEAT_MODE
{
	IN_GAME,
	GS_BUTTON
};

struct CODENODE_STRUCT
{
	u32	addr;
	u16	val;
	u16	orig;
};
using CHEATCODENODE = struct CODENODE_STRUCT;


struct CODEGROUP_STRUCT
{
	//u32				country;
	u32				codecount;
	bool			enable;
	bool			active;
	char			name[80];
	char			note[256];
	CHEATCODENODE	codelist[MAX_CHEATCODE_PER_ENTRY];
};
using CODEGROUP = struct CODEGROUP_STRUCT;

extern u32			codegroupcount;
extern CODEGROUP	*codegrouplist;

void				CheatCodes_Activate( CHEAT_MODE mode );
bool				CheatCodes_Read(const char *rom_name, const char *file, u8 countryID);
void				CheatCodes_Disable( u32 index );


#endif // CORE_CHEATS_H_
