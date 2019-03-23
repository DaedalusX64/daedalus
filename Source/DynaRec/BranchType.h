/*
Copyright (C) 2006 StrmnNrmn

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

#ifndef DYNAREC_BRANCHTYPE_H_
#define DYNAREC_BRANCHTYPE_H_

#include "Utility/DaedalusTypes.h"
#include <stdlib.h>

struct OpCode;

enum ER4300BranchType
{
	BT_NOT_BRANCH = 0,

	BT_BEQL,
	BT_BNEL,
	BT_BLEZL,
	BT_BGTZL,
	BT_BLTZL,
	BT_BGEZL,
	BT_BLTZALL,
	BT_BGEZALL,
	BT_BC1FL,
	BT_BC1TL,

	BT_BEQ,
	BT_BNE,
	BT_BLEZ,
	BT_BGTZ,
	BT_BLTZ,
	BT_BLTZAL,
	BT_BGEZ,
	BT_BGEZAL,
	BT_BC1F,
	BT_BC1T,

	BT_J,
	BT_JAL,
	BT_JR,
	BT_JALR,
	BT_ERET,

//	BT_MAX,
};

inline bool IsBranchTypeLikely( ER4300BranchType type )
{
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( type != BT_NOT_BRANCH, "This is not a valid branch type" );
#endif
	if( type < BT_BEQ )
		return true;
	else
		return false;

}

inline bool IsConditionalBranch( ER4300BranchType type )
{
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( type != BT_NOT_BRANCH, "This is not a valid branch type" );
#endif

	if( type >= BT_J )
		return false;
	else
		return true;
}

inline bool IsBranchTypeDirect( ER4300BranchType type )
{
	DAEDALUS_ASSERT( type != BT_NOT_BRANCH, "This is not a valid branch type" );

	if( type >= BT_JR )
		return false;
	else
		return true;
}

u32	GetBranchTarget( u32 address, OpCode op_code, ER4300BranchType type );

#endif // DYNAREC_BRANCHTYPE_H_
