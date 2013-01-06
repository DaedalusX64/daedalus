/*

  Copyright (C) 2002 StrmnNrmn

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


#include "StdAfx.h"
#include "DebugPaneCP2Dis.h"

#include "Core/RSP.h"
#include "Core/Memory.h"
#include "Core/R4300OpCode.h"

#include "OSHLE/ultra_r4300.h"

#include "Utility/PrintOpCode.h"

//*****************************************************************************
//* Static variables
//*****************************************************************************
static const WORD sc_wAttrWhite			= FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;
static const WORD sc_wAttrBoldWhite		= FOREGROUND_INTENSITY|FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;

//*****************************************************************************
// Copy the visible portion to the display
//*****************************************************************************
void	CDebugPaneCP2Dis::Display( HANDLE hOutput )
{
	char opinfo[256];
	char msg[200];
	u16 wY;

	u32 x = GetX();
	u32 y = GetY();
	u32 width = GetWidth();
	u32 height = GetHeight();

	DWORD dwRSPPC = (gRSPState.CurrentPC & 0x0FFF) | 0x04001000;

	if ( mMemoryAddress == ~0 )
		mMemoryAddress = dwRSPPC;

	u32 pc = mMemoryAddress;

	pc -= (4*(height/2));	// Show previous 4 instructions

	// Clear the background area...

	wY = u16( y );
	for (DWORD dwIndex = 0; dwIndex < height; dwIndex++)
	{
		if (pc >= 0x04002000)
			break;

		//- TODO - Translate to InternalRead
		OpCode op;
		op._u32 = Read32Bits( PHYS_TO_K1( pc ) );

		SprintRSPOpCodeInfo( opinfo, pc, op );
		wsprintf(msg, "    0x%08x: %s", pc, opinfo);

		if ((pc&0x1FFF) == (dwRSPPC&0x1FFF))
		{
			WriteString( hOutput, msg, FALSE, x, wY, sc_wAttrBoldWhite, width );
		}
		else
		{
			WriteString( hOutput, msg, FALSE, x, wY, sc_wAttrWhite, width );
		}

		wY++;
		pc += 4;
	}
}


