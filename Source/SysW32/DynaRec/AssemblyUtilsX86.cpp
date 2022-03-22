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

namespace AssemblyUtils
{

//*****************************************************************************
//	Patch a long jump to target the specified location.
//	Return true if the patching succeeded (i.e. within range), false otherwise
//*****************************************************************************
bool	PatchJumpLong( CJumpLocation jump, CCodeLabel target )
{
	const u32	JUMP_DIRECT_LONG_LENGTH = 5;
	const u32	JUMP_LONG_LENGTH = 6;

	u8 *	p_jump_addr( reinterpret_cast< u8 * >( jump.GetWritableU8P() ) );
	u32		instruction_length;
	u32 *	p_jump_instr_offset;

	if( *p_jump_addr == 0xe8 || *p_jump_addr == 0xe9 )
	{
		// call/jmp
		instruction_length = JUMP_DIRECT_LONG_LENGTH;
		p_jump_instr_offset = reinterpret_cast< u32 * >( p_jump_addr + 1 );
	}
	else if( *p_jump_addr == 0x0f )
	{
		// jne etc
		instruction_length = JUMP_LONG_LENGTH;
		p_jump_instr_offset = reinterpret_cast< u32 * >( p_jump_addr + 2 );
	}
	else
	{
		DAEDALUS_ERROR( "Unhandled jump type" );
		return false;
	}

	u32		offset( jump.GetOffset( target ) - instruction_length );

	*p_jump_instr_offset = offset;

	// All jumps are 32 bit offsets, and so always succeed.
	return true;
}

//*****************************************************************************
//	As above no (need to flush on intel)
//*****************************************************************************
bool	PatchJumpLongAndFlush( CJumpLocation jump, CCodeLabel target )
{
	return PatchJumpLong( jump, target );
}

}
