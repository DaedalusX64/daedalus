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
#include "DebugPaneCP0Regs.h"

#include "Core\CPU.h"
#include "Core\Registers.h"
#include "OSHLE\ultra_r4300.h"

//*****************************************************************************
//* Static variables
//*****************************************************************************
static const WORD sc_wAttrWhite			= FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;
static const WORD sc_wAttrBoldRed		= FOREGROUND_INTENSITY|FOREGROUND_RED;

static const DWORD sc_Cop0RegsToShow[] =
{
	C0_INX,
	//C0_RAND,
	C0_ENTRYLO0,
	C0_ENTRYLO1,
	C0_CONTEXT,
	C0_PAGEMASK,
	C0_WIRED,
	C0_BADVADDR,
	C0_COUNT,
	C0_ENTRYHI,
	C0_SR,
	C0_CAUSE,
	C0_EPC,
	//C0_PRID,
	C0_COMPARE,
	C0_CONFIG,
	C0_LLADDR,
	C0_WATCHLO,
	C0_WATCHHI,
	C0_ECC,			// PERR
	C0_CACHE_ERR,
	C0_TAGLO,
	C0_TAGHI,
	C0_ERROR_EPC,
	~0
};

//*****************************************************************************
// Copy the visible portion to the display
//*****************************************************************************
void	CDebugPaneCP0Regs::Display( HANDLE hOutput )
{
#ifndef DAEDALUS_SILENT
	char msg[200];

	u32 base;
	u32 r;
	s16 wX;
	s16 wY;
	static REG64 gGPRCopy[2][32];
	static u32 last_pc = ~0;

	u32 x = GetX();
	u32 y = GetY();
	u32 width = GetWidth();
	u32 height = GetHeight();

	static const DWORD dwRegDisplayWidth = 11;

	//
	// If we've executed an instruction, update the register copies
	//
	if ( last_pc != gCPUState.CurrentPC )
	{
		memcpy( gGPRCopy[1], gGPRCopy[0], sizeof( gGPR ) );
		memcpy( gGPRCopy[0], gGPR, sizeof( gGPR ) );
		last_pc = gCPUState.CurrentPC;
	}

	wX = s16( x );
	wY = s16( y );

	u32 bottom = y + GetHeight();

	if ( u32( wY ) < bottom )
	{
		sprintf(msg, "PC:%08x", gCPUState.CurrentPC);
		WriteString( hOutput, msg, FALSE, wX, wY, sc_wAttrWhite, 19 ); wY ++;
	}

	if ( u32( wY ) < bottom )
	{
		sprintf(msg, "MH:%016I64x", gCPUState.MultHi._u64);
		WriteString( hOutput, msg, FALSE, wX, wY, sc_wAttrWhite, 19 ); wY ++;
	}

	if ( u32( wY ) < bottom )
	{
		sprintf(msg, "ML:%016I64x", gCPUState.MultLo._u64);
		WriteString( hOutput, msg, FALSE, wX, wY, sc_wAttrWhite, 19 ); wY ++;
	}

	// Do main registers
	for (base = 0; base < 32; base += 8)
	{
		wY = s16( y+3 );
		for ( r = base; r < base+8 && u32( wY ) < bottom; r++)
		{
			// Not wsprintf as we want long int output
			wsprintf(msg, "%s:%08x", RegNames[r], gGPRCopy[0][r]._u32_0);

			// Only draw if it's changed
			if (gGPRCopy[0][r]._u64 != gGPRCopy[1][r]._u64)
			{
				WriteString( hOutput, msg, FALSE, wX, wY, sc_wAttrBoldRed, dwRegDisplayWidth );
			}
			else
			{
				WriteString( hOutput, msg, FALSE, wX, wY, sc_wAttrWhite, dwRegDisplayWidth );
			}
			wY++;
		}
		wX += dwRegDisplayWidth + 1;
	}

	// Do CoPro1 regs
	wY = s16( y );
	DWORD i;
	for (i = 0; ; i++)
	{
		r = sc_Cop0RegsToShow[i];
		if (r == ~0)
			break;

		if ( u32( wY ) < bottom )
		{
			wsprintf( msg, "%4.4s:%08x", ShortCop0RegNames[r], gCPUState.CPUControl[r]._u32 );
			WriteString( hOutput, msg, FALSE, wX, wY, sc_wAttrWhite, 13 );
		}
		wY ++;
		if (i == 10)
		{
			wX += 15;
			wY = s16( y );
		}
	}
#endif
}


