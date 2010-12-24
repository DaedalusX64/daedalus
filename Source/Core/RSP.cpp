/*
Copyright (C) 2001-2010 StrmnNrmn

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

//
// Important Note:
//	Various chunks of this code/data are derived from Zilmar's RSP plugin,
//	which has also been used as a reference for fixing bugs in original
//	code. Many thanks to Zilmar for releasing the source to this!
//

// - Salvy '2010

// On the PSP we deprecated RSP emulation
// 1 - It doesn't work correctly
// 2 - Adds a lot of weight in our elf
// 3 - Only known game that fails due the lack of RSP emulation is DK64
//

// RSP Processor stuff
#include "stdafx.h"

#include "CPU.h"
#include "R4300.h"
#include "RSP.h"

#include "Debug/DBGConsole.h"

// We need similar registers to the main CPU
ALIGNED_GLOBAL(SRSPState, gRSPState, CACHE_ALIGN);

volatile bool gRSPHLEActive = false;
//*****************************************************************************
//
//*****************************************************************************

bool RSP_IsRunning()
{
	return (Memory_SP_GetRegister( SP_STATUS_REG ) & SP_STATUS_HALT) == 0;
}

//*****************************************************************************
//
//*****************************************************************************
bool RSP_IsRunningLLE()
{
	return RSP_IsRunning() && !gRSPHLEActive;
}

//*****************************************************************************
//
//*****************************************************************************
bool RSP_IsRunningHLE()
{
	return RSP_IsRunning() && gRSPHLEActive;
}

#if 0
//*****************************************************************************
//
//*****************************************************************************
void RSP_DumpVector(u32 reg)
{
	reg &= 0x1f;

	DBGConsole_Msg(0, "Vector%d", reg);
	for(u32 i = 0; i < 8; i++)
	{
		DBGConsole_Msg(0, "%d: 0x%04x", i, gVectRSP[reg]._u16[i]);
	}
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_DumpVectors(u32 reg1, u32 reg2, u32 reg3)
{
	reg1 &= 0x1f;
	reg2 &= 0x1f;
	reg3 &= 0x1f;

	DBGConsole_Msg(0, "    Vec%2d\tVec%2d\tVec%2d", reg1, reg2, reg3);
	for(u32 i = 0; i < 8; i++)
	{
		DBGConsole_Msg(0, "%d: 0x%04x\t0x%04x\t0x%04x",
			i, gVectRSP[reg1]._u16[i], gVectRSP[reg2]._u16[i], gVectRSP[reg3]._u16[i]);
	}
}
#endif
