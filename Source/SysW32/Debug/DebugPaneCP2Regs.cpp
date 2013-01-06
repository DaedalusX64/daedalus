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
#include "DebugPaneCP2Regs.h"

#include "Core\RSP.h"
#include "Core\Memory.h"
#include "Core\Registers.h"
#include "OSHLE\ultra_r4300.h"

//*****************************************************************************
//* Static variables
//*****************************************************************************
static const WORD sc_wAttrWhite			= FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;
static const WORD sc_wAttrBoldRed		= FOREGROUND_INTENSITY|FOREGROUND_RED;

//*****************************************************************************
// Copy the visible portion to the display
//*****************************************************************************
void	CDebugPaneCP2Regs::Display( HANDLE hOutput )
{
	char msg[200];

	u32 reg_idx;
	u32 base;
	u16 wX = 0;
	u16 wY = 0;
	static bool init = false;
	static REG32 rsp_gpr_copy[32];

	static const DWORD dwRegDisplayWidth = 11;

	u32 x = GetX();
	u32 y = GetY();
	u32 width = GetWidth();
	u32 height = GetHeight();

	if ( !init )
	{
		// Make sure that we render all registers
		for (reg_idx = 0; reg_idx < 32; reg_idx++)
		{
			rsp_gpr_copy[reg_idx] = gRSPState.CPU[reg_idx];
		}
		init = true;
	}

	wY = u16( y );
	wsprintf(msg, "PC: %08x", gRSPState.CurrentPC);
	WriteString( hOutput, msg, FALSE, wX, wY, sc_wAttrWhite, dwRegDisplayWidth+8 ); wY++;

	// Do main registers
	for (base = 0; base < 32; base += 8)
	{
		wY = u16( y+3 );
		for (reg_idx = base; reg_idx < base+8; reg_idx++)
		{
			wsprintf(msg, "%s:%08x", RegNames[reg_idx], gRSPState.CPU[reg_idx]);

			// Only draw if changed
			if (rsp_gpr_copy[reg_idx]._u32 != gRSPState.CPU[reg_idx]._u32)
			{
				WriteString( hOutput, msg, FALSE, wX, wY, sc_wAttrBoldRed, 20 );
			}
			else
			{
				WriteString( hOutput, msg, FALSE, wX, wY, sc_wAttrWhite, 20 );
			}
			wY++;
		}
		wX += dwRegDisplayWidth + 1;
	}

	for (reg_idx = 0; reg_idx < 32; reg_idx++)
	{
		rsp_gpr_copy[reg_idx] = gRSPState.CPU[reg_idx];
	}

}


