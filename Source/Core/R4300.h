/*
Copyright (C) 2001 StrmnNrmn

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

#ifndef __DAEDALUS_R4300_H__
#define __DAEDALUS_R4300_H__

#include "R4300OpCode.h"
#include "R4300Instruction.h"
/*
#define ToInt(x)	*((int *)&x)
#define ToQuad(x)	*((u64 *)&x)
#define ToFloat(x)	*((float *)&x)
*/
void R4300_SetSR( u32 new_value );

extern CPU_Instruction R4300Instruction[64];

CPU_Instruction	R4300_GetInstructionHandler( OpCode op_code );
bool			R4300_InstructionHandlerNeedsPC( OpCode op_code );
inline void			R4300_ExecuteInstruction( OpCode op_code )
{
	R4300Instruction[ op_code.op ]( op_code._u32 );
}

#endif
