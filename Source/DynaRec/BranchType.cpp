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

#include "stdafx.h"
#include "BranchType.h"

#include "Core/R4300OpCode.h"

// 
// I think we should check for branch type in static analyzer - Salvy
//

//*************************************************************************************
//
//*************************************************************************************
/*
static const ER4300BranchType gInverseBranchTypes[] =
{
	BT_NOT_BRANCH,
	BT_BNE,		BT_BNEL,
	BT_BEQ,		BT_BEQL,
	BT_BGTZ,	BT_BGTZL,
	BT_BLEZ,	BT_BLEZL,
	BT_BGEZ,	BT_BGEZL,	BT_BGEZAL,	BT_BGEZALL,
	BT_BLTZ,	BT_BLTZL,	BT_BLTZAL,	BT_BLTZALL,
	BT_BC1T,	BT_BC1TL,
	BT_BC1F,	BT_BC1FL,
	BT_J,		// All the following are unconditional
	BT_JAL,
	BT_JR,
	BT_JALR,
	BT_ERET,
};

*/
//*************************************************************************************
//
//*************************************************************************************
ER4300BranchType GetBranchType( OpCode op_code )
{
	switch( op_code.op )
	{
	case OP_J:				return BT_J;
	case OP_JAL:			return BT_JAL;
	case OP_BEQ:			return BT_BEQ;
	case OP_BNE:			return BT_BNE;
	case OP_BLEZ:			return BT_BLEZ;
	case OP_BGTZ:			return BT_BGTZ;
	case OP_BEQL:			return BT_BEQL;
	case OP_BNEL:			return BT_BNEL;
	case OP_BLEZL:			return BT_BLEZL;
	case OP_BGTZL:			return BT_BGTZL;

	case OP_REGIMM:
		switch( op_code.regimm_op )
		{
		case RegImmOp_BLTZ:		return BT_BLTZ;
		case RegImmOp_BGEZ:		return BT_BGEZ;
		case RegImmOp_BLTZL:	return BT_BLTZL;
		case RegImmOp_BGEZL:	return BT_BGEZL;
		case RegImmOp_BLTZAL:	return BT_BLTZAL;
		case RegImmOp_BGEZAL:	return BT_BGEZAL;
		case RegImmOp_BLTZALL:	return BT_BLTZALL;
		case RegImmOp_BGEZALL:	return BT_BGEZALL;
		default:
			break;
		}
		break;

	case OP_SPECOP:
		switch( op_code.spec_op )
		{
		case SpecOp_JR:			return BT_JR;
		case SpecOp_JALR:		return BT_JALR;
		default:
			break;
		}
		break;

	case OP_COPRO0:
		if( op_code.cop0_op == Cop0Op_TLB )
		{
			switch( op_code.cop0tlb_funct )
			{
			case OP_ERET:	return BT_ERET;
			}
		}
		break;
	case OP_COPRO1:
		if( op_code.cop1_op == Cop1Op_BCInstr )
		{	
			switch( op_code.cop1_bc )
			{
			case Cop1BCOp_BC1F:		return BT_BC1F;
			case Cop1BCOp_BC1T:		return BT_BC1T;
			case Cop1BCOp_BC1FL:	return BT_BC1FL;
			case Cop1BCOp_BC1TL:	return BT_BC1TL;
			}
		}
		break;
	}

	return BT_NOT_BRANCH;
}

//*************************************************************************************
//
//*************************************************************************************
/*
OpCode	GetInverseBranch( OpCode op_code )
{
	switch( op_code.op )
	{
	case OP_J:				break;
	case OP_JAL:			break;
	case OP_BEQ:			op_code.op = OP_BNE;	break;
	case OP_BNE:			op_code.op = OP_BEQ;	break;
	case OP_BLEZ:			op_code.op = OP_BGTZ;	break;
	case OP_BGTZ:			op_code.op = OP_BLEZ;	break;
	case OP_BEQL:			op_code.op = OP_BNEL;	break;
	case OP_BNEL:			op_code.op = OP_BEQL;	break;
	case OP_BLEZL:			op_code.op = OP_BGTZL;	break;
	case OP_BGTZL:			op_code.op = OP_BLEZL;	break;

	case OP_REGIMM:
		switch( op_code.regimm_op )
		{
		case RegImmOp_BLTZ:		op_code.regimm_op = RegImmOp_BGEZ;	break;
		case RegImmOp_BGEZ:		op_code.regimm_op = RegImmOp_BLTZ;	break;
		case RegImmOp_BLTZL:	op_code.regimm_op = RegImmOp_BGEZL;	break;
		case RegImmOp_BGEZL:	op_code.regimm_op = RegImmOp_BLTZL;	break;
		case RegImmOp_BLTZAL:	op_code.regimm_op = RegImmOp_BGEZAL;break;
		case RegImmOp_BGEZAL:	op_code.regimm_op = RegImmOp_BLTZAL;break;
		case RegImmOp_BLTZALL:	op_code.regimm_op = RegImmOp_BGEZALL;break;
		case RegImmOp_BGEZALL:	op_code.regimm_op = RegImmOp_BLTZALL;break;
		default:
			NODEFAULT;
			break;
		}
		break;

	case OP_SPECOP:
		switch( op_code.spec_op )
		{
		case SpecOp_JR:			break;
		case SpecOp_JALR:		break;
		default:
			break;
		}
		break;

	case OP_COPRO0:
		if( op_code.cop0_op == Cop0Op_TLB )
		{
			switch( op_code.cop0tlb_funct )
			{
			case OP_ERET:	break;
			}
		}
		break;
	case OP_COPRO1:
		if( op_code.cop1_op == Cop1Op_BCInstr )
		{	
			switch( op_code.cop1_bc )
			{
			case Cop1BCOp_BC1F:		op_code.cop1_bc = Cop1BCOp_BC1T;	break;
			case Cop1BCOp_BC1T:		op_code.cop1_bc = Cop1BCOp_BC1F;	break;
			case Cop1BCOp_BC1FL:	op_code.cop1_bc = Cop1BCOp_BC1TL;	break;
			case Cop1BCOp_BC1TL:	op_code.cop1_bc = Cop1BCOp_BC1FL;	break;
			}
		}
		break;
	}

	return op_code;
}
*/
//*************************************************************************************
//
//*************************************************************************************
/*
namespace
{
	OpCode	UpdateBranchOffset( OpCode op_code, u32 branch_location, u32 target_location )
	{
		DAEDALUS_ASSERT( (target_location & 0x3) == 0, "Target location is not 4-byte aligned!" );

		s32		offset( target_location - branch_location );		// signed

		// XXXX check if jump is out of range!

		op_code.offset = u16( ( offset - 4 ) >> 2 );
		return op_code;
	}
	
	OpCode	UpdateJumpTarget( OpCode op_code, u32 jump_location, u32 target_location )
	{
		op_code.target = (target_location - jump_location) >> 2;
		return op_code;
	}
}
*/
//*************************************************************************************
//
//*************************************************************************************
/*
OpCode	UpdateBranchTarget( OpCode op_code, u32 op_address, u32 target_address )
{
	switch( op_code.op )
	{
	case OP_J:
	case OP_JAL:
		op_code = UpdateJumpTarget( op_code, op_address, target_address );
		break;
	case OP_BEQ:
	case OP_BNE:
	case OP_BLEZ:
	case OP_BGTZ:
	case OP_BEQL:
	case OP_BNEL:
	case OP_BLEZL:
	case OP_BGTZL:
		op_code = UpdateBranchOffset( op_code, op_address, target_address );
		break;

	case OP_REGIMM:
		switch( op_code.regimm_op )
		{
		case RegImmOp_BLTZ:
		case RegImmOp_BGEZ:
		case RegImmOp_BLTZL:
		case RegImmOp_BGEZL:
		case RegImmOp_BLTZAL:
		case RegImmOp_BGEZAL:
		case RegImmOp_BLTZALL:
		case RegImmOp_BGEZALL:
			op_code = UpdateBranchOffset( op_code, op_address, target_address );	
			break;
		default:
			NODEFAULT;
			break;
		}
		break;

	case OP_SPECOP:
		switch( op_code.spec_op )
		{
		case SpecOp_JR:
		case SpecOp_JALR:
			// No jump target - it's indirect
			break;
		default:
			break;
		}
		break;

	case OP_COPRO0:
		if( op_code.cop0_op == Cop0Op_TLB )
		{
			switch( op_code.cop0tlb_funct )
			{
			// No jump target - it's indirect
			case OP_ERET:
				break;
			}
		}
		break;
	case OP_COPRO1:
		if( op_code.cop1_op == Cop1Op_BCInstr )
		{	
			switch( op_code.cop1_bc )
			{
			case Cop1BCOp_BC1F:
			case Cop1BCOp_BC1T:
			case Cop1BCOp_BC1FL:
			case Cop1BCOp_BC1TL:
				op_code = UpdateBranchOffset( op_code, op_address, target_address );
				break;
			}
		}
		break;
	}

	return op_code;
}
*/

//*************************************************************************************
//
//*************************************************************************************
/*
ER4300BranchType	GetInverseBranch( ER4300BranchType type )
{
	ER4300BranchType	inverse( gInverseBranchTypes[ type ] );

	DAEDALUS_ASSERT( gInverseBranchTypes[ inverse ] == type, "Inconsistant inverse branch type" );

	return inverse;
}
*/
//*************************************************************************************
//
//*************************************************************************************

// From PrintOpCode
#define BranchAddress(op, address) (    (address)+4 + (s16)(((op).immediate))*4)
#define JumpTarget(op, address)    (   ((address) & 0xF0000000) | (((op).target)<<2)   )

u32 GetBranchTarget( u32 address, OpCode op_code, ER4300BranchType type )
{
	DAEDALUS_ASSERT( type != BT_NOT_BRANCH, "This is not a valid branch type" );

	// We pass the type in for efficiency - check that it's correct in debug though
	DAEDALUS_ASSERT( GetBranchType( op_code ) == type, "Specified type is inconsistant with op code" );

	if( type < BT_J )
	{
		return BranchAddress( op_code, address );
	}

	// All the following are unconditional
	if( type < BT_JR )
	{
		return JumpTarget( op_code, address );
	}
	
	// These are all indirect
	return  0;
}

//
// Bellow Functions are very simple 2 ops (jr,slti) shall we inline them?
// They assume we have already checked for BT_NOT_BRANCH which is of course already checked ;)
//
//*************************************************************************************
//
//*************************************************************************************
bool IsConditionalBranch( ER4300BranchType type )
{
	DAEDALUS_ASSERT( type != BT_NOT_BRANCH, "This is not a valid branch type" );

	if( type >= BT_J )
		return false;
	else
		return true;
}

//*************************************************************************************
//
//*************************************************************************************
bool IsBranchTypeDirect( ER4300BranchType type )
{
	DAEDALUS_ASSERT( type != BT_NOT_BRANCH, "This is not a valid branch type" );

	if( type >= BT_JR )
		return false;
	else
		return true;
}

//*************************************************************************************
//
//*************************************************************************************
bool IsBranchTypeLikely( ER4300BranchType type )
{
	DAEDALUS_ASSERT( type != BT_NOT_BRANCH, "This is not a valid branch type" );

	if( type < BT_BEQ )
		return true;
	else
		return false;

}

