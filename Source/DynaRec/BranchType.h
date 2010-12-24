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

#ifndef __DAEDALUS_BRANCHTYPE_H__
#define __DAEDALUS_BRANCHTYPE_H__

struct OpCode;

enum ER4300BranchType
{
	BT_NOT_BRANCH = 0,
	BT_BEQ,		BT_BEQL,
	BT_BNE,		BT_BNEL,
	BT_BLEZ,	BT_BLEZL,
	BT_BGTZ,	BT_BGTZL,
	BT_BLTZ,	BT_BLTZL,	BT_BLTZAL,	BT_BLTZALL,
	BT_BGEZ,	BT_BGEZL,	BT_BGEZAL,	BT_BGEZALL,
	BT_BC1F,	BT_BC1FL,
	BT_BC1T,	BT_BC1TL,
	BT_J,
	BT_JAL,
	BT_JR,
	BT_JALR,
	BT_ERET,
};

ER4300BranchType		GetBranchType( OpCode op_code );
ER4300BranchType		GetInverseBranch( ER4300BranchType type );
u32						GetBranchTarget( u32 address, OpCode op_code, ER4300BranchType type );
bool					IsConditionalBranch( ER4300BranchType type );
bool					IsBranchTypeDirect( ER4300BranchType type );
bool					IsBranchTypeLikely( ER4300BranchType type );

OpCode					GetInverseBranch( OpCode op_code );
OpCode					UpdateBranchTarget( OpCode op_code, u32 op_address, u32 target_address );

#endif // __DAEDALUS_BRANCHTYPE_H__
