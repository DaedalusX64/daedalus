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
#include "DynaRec/AssemblyUtils.h"

#include "DynarecTargetPSP.h"

#include <limits.h>

#include <psputilsforkernel.h>
#include <psputils.h>

#include "Math/MathUtil.h"

extern "C" { void _DaedalusICacheInvalidate( const void * address, u32 length ); }

namespace AssemblyUtils
{


//	Patch a long jump to target the specified location.
//	Return true if the patching succeeded (i.e. within range), false otherwise

bool	PatchJumpLong( CJumpLocation jump, CCodeLabel target )
{
	// Get an uncached pointer
	PspOpCode *	p_jump_addr( reinterpret_cast< PspOpCode * >( jump.GetWritableU8P() ) );
	PspOpCode &	op_code( *p_jump_addr );

	if( op_code.op == OP_J || op_code.op == OP_JAL )
	{
		op_code.target = target.GetTargetU32() >> 2;
	}
	else
	{
#ifdef DAEDALUS_ENABLE_ASSERTS
		bool	is_standard_branch( op_code.op == OP_BNE || op_code.op == OP_BEQ ||
									op_code.op == OP_BEQL || op_code.op == OP_BNEL ||
									op_code.op == OP_BLEZ || op_code.op == OP_BGTZ );
		bool	is_regimm_branch(  (op_code.op == OP_REGIMM && (op_code.regimm_op == RegImmOp_BGEZ ||
																op_code.regimm_op == RegImmOp_BLTZ ||
																op_code.regimm_op == RegImmOp_BGEZL ||
																op_code.regimm_op == RegImmOp_BLTZL ) ) );
		bool	is_cop1_branch( (op_code.cop1_op == Cop1Op_BCInstr) && ( op_code.cop1_bc == Cop1BCOp_BC1F  ||
																		 op_code.cop1_bc == Cop1BCOp_BC1FL ||
																		 op_code.cop1_bc == Cop1BCOp_BC1T  ||
																		 op_code.cop1_bc == Cop1BCOp_BC1TL ) );

		DAEDALUS_ASSERT( is_standard_branch || is_regimm_branch || is_cop1_branch, "Unhandled branch type" );
#endif

		s32		offset( ( jump.GetOffset( target ) >> 2 ) - 1 );

		//
		//	Check if the branch is within range
		//
		if( offset < SHRT_MIN || offset > SHRT_MAX )
		{
			DAEDALUS_ERROR(" PatchJump out of range!!!");
			return false;
		}
		op_code.offset = s16(offset);	// Already divided by 4
	}

	return true;
}


//	As above but invalidates the instruction cache for the specified address.

bool	PatchJumpLongAndFlush( CJumpLocation jump, CCodeLabel target )
{
	if( PatchJumpLong( jump, target ) )
	{
		//	sceKernelDcacheWritebackRange( jump.GetTargetU8P(), 4 );
		//	sceKernelIcacheInvalidateRange( jump.GetTargetU8P(), 4 );

		const u8 * p_lower( RoundPointerDown( jump.GetTargetU8P(), 64 ) );
		const u8 * p_upper( RoundPointerUp( jump.GetTargetU8P() + 8, 64 ) );
		const u32  size( p_upper - p_lower);

		_DaedalusICacheInvalidate( p_lower, size );

		return true;
	}
	return false;
}


//	Replace a branch instruction with an unconditional jump
void		ReplaceBranchWithJump( CJumpLocation branch, CCodeLabel target )
{
	// Get an uncached pointer
	PspOpCode *	p_jump_addr( reinterpret_cast< PspOpCode * >( branch.GetWritableU8P() ) );
	PspOpCode &	op_code( *p_jump_addr );

	// Sanity check this is actually a branch?
	op_code.op = OP_J;
	op_code.target = target.GetTargetU32() >> 2;

//	sceKernelDcacheWritebackRange( branch.GetTargetU8P(), 4 );
//	sceKernelIcacheInvalidateRange( branch.GetTargetU8P(), 4 );

	const u8 * p_lower( RoundPointerDown( branch.GetTargetU8P(), 64 ) );
	const u8 * p_upper( RoundPointerUp( branch.GetTargetU8P() + 8, 64 ) );
	const u32  size( p_upper - p_lower);

	_DaedalusICacheInvalidate( p_lower, size );
}


}
