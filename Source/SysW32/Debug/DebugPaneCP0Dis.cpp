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
#include "DebugPaneCP0Dis.h"

#include "Core\Memory.h"
#include "Core\CPU.h"
#include "Core\Registers.h"

#include "OSHLE\Patch.h"

#include "Utility\PrintOpCode.h"


//*****************************************************************************
//* Static variables
//*****************************************************************************
static const WORD sc_wAttrWhite			= FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;
static const WORD sc_wAttrBoldWhite		= FOREGROUND_INTENSITY|FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;

//*****************************************************************************
// Copy the visible portion to the display
//*****************************************************************************
void	CDebugPaneCP0Dis::Display( HANDLE hOutput )
{
	u16 wY;
	DWORD * pPC;
	char opinfo[120];
	char msg[200];
	WORD wAttr;

	u32 x = GetX();
	u32 y = GetY();
	u32 width = GetWidth();
	u32 height = GetHeight();

	if ( mMemoryAddress == ~0 )
		mMemoryAddress = gCPUState.CurrentPC;

	u32 pc = mMemoryAddress;

	pc -= (4*(height/2));	// Show previous instructions

	// Clear the background area...

	wY = u16( y );
	for ( u32 dwIndex = 0; dwIndex < height; dwIndex++ )
	{
		if (pc == gCPUState.CurrentPC)
			wAttr = sc_wAttrBoldWhite;
		else
			wAttr = sc_wAttrWhite;

		if (Memory_GetInternalReadAddress(pc, (LPVOID*)&pPC))
		{
			OpCode op;
			op._u32 = pPC[0];

			SprintOpCodeInfo( opinfo, pc, op );


#ifdef DAEDALUS_BREAKPOINTS_ENABLED
			// Additional info for our hacks:
			if ( op.op == OP_DBG_BKPT )
			{
				u32 bp_index = op.bp_index;

				if (bp_index < g_BreakPoints.size())
				{
					if (g_BreakPoints[ bp_index ].mEnabled)
						sprintf(msg, "[R!]   0x%08x: %s", pc, opinfo);
					else
						sprintf(msg, "[G!]   0x%08x: %s", pc, opinfo);
				}
				else
				{
					sprintf(msg, "    0x%08x: %s", pc, opinfo);
				}
			}
			else
			{
				sprintf(msg, "    0x%08x: %s", pc, opinfo);
			}
#endif

#ifdef DAEDALUS_ENABLE_OS_HOOKS
			//if ( op.op == OP_PATCH )
			{
				LPCSTR pszSymbol = Patch_GetJumpAddressName( pc );
				if (pszSymbol[0] != '?')
				{
					// This is actually a patch target
					WriteString( hOutput, "", FALSE, x, wY, wAttr, width );
					wY++;
					dwIndex++;

					if ( dwIndex < height )
					{
						TCHAR szFuncInfo[30];
						sprintf(szFuncInfo, "[G%s]", pszSymbol);
						WriteString( hOutput, szFuncInfo, TRUE, x, wY, wAttr, width );
						wY++;
						dwIndex++;
					}
				}
			}
#endif
		}
		else
		{
			sprintf(msg, "    0x%08x: ???", pc);
		}


		if ( dwIndex < height )
		{
			WriteString( hOutput, msg, TRUE, x, wY, wAttr, width );
			wY++;
		}

		pc+=4;
	}
}


