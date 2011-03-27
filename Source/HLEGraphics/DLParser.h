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


#ifndef __DLPARSER_H__
#define __DLPARSER_H__

#include "Core/Memory.h"
#include "UcodeDefs.h"
//*************************************************************************************
// 
//*************************************************************************************
bool DLParser_Initialise();
void DLParser_Finalise();
void DLParser_Process();

//*************************************************************************************
// 
//*************************************************************************************
#define MAX_DL_STACK_SIZE	32
#define MAX_DL_COUNT		100000// Maybe excesive large? 1000000

// The display list PC stack. Before this was an array of 10
// items, but this way we can nest as deeply as necessary. 

struct DListStack
{
	u32 pc;
	s32 countdown;
};

extern DListStack	gDlistStack[MAX_DL_STACK_SIZE];
extern s32			gDlistStackPointer;
//*****************************************************************************
// Reads the next command from the display list, updates the PC.
//*****************************************************************************
inline void	DLParser_FetchNextCommand( MicroCodeCommand * p_command )
{
	// Current PC is the last value on the stack
	u32			pc( gDlistStack[gDlistStackPointer].pc );

	p_command->inst.cmd0 = g_pu32RamBase[(pc>>2)+0];
	p_command->inst.cmd1 = g_pu32RamBase[(pc>>2)+1];

	gDlistStack[gDlistStackPointer].pc += 8;
}

//*************************************************************************************
// 
//*************************************************************************************
// Various debugger commands:
#ifdef DAEDALUS_DEBUG_DISPLAYLIST

const u32	UNLIMITED_INSTRUCTION_COUNT( u32( ~0 ) );

void		DLParser_DumpNextDisplayList();
u32			DLParser_GetTotalInstructionCount();
u32			DLParser_GetInstructionCountLimit();
void		DLParser_SetInstructionCountLimit( u32 limit );

#endif


#endif	// __DLPARSER_H__
