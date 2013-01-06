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
#include "DebugPaneMemory.h"

#include "Core\Memory.h"

//*****************************************************************************
//* Static variables
//*****************************************************************************
static const WORD sc_wAttrWhite			= FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;

//*****************************************************************************
// Copy the visible portion to the display
//*****************************************************************************
void	CDebugPaneMemory::Display( HANDLE hOutput )
{
	CHAR buf[120];

	u32 x = GetX();
	u32 y = GetY();
	u32 width = GetWidth();
	u32 height = GetHeight();


	// Clear the background area...
	u32 address = mMemoryAddress;

	for ( u32 i = 0; i < height; i++ )
	{
		u32 * pMem;

		// TODO - Lookup a base for each of pMem[0], pMem[1] etc - this
		// is significant if we cross a boundary, but not *really* important

		if (Memory_GetInternalReadAddress( address, (LPVOID*)&pMem ))
		{
			wsprintf(buf, "    0x%08x: %08x %08x %08x %08x ", address, pMem[0], pMem[1], pMem[2], pMem[3]);

			CHAR * p = buf + lstrlen(buf);
			// Concat characters
			for (int i = 0; i < 16; i++)
			{
				CHAR c = ((BYTE*)pMem)[i ^ 0x3];
				// The cast to unsigned int avoids widening char to a signed int.
				// This cast avoids a failed assertion popup in MSVC 7.0
				if (isprint((unsigned int)(unsigned char)c))
					*p++ = c;
				else
					*p++ = '.';

				if ((i%4)==3)
					*p++ = ' ';
			}
			*p++ = 0;
		}
		else
		{
			wsprintf( buf, "    0x%08x: ???????? ???????? ???????? ???????? .... .... .... ....", address );
		}

		WriteString( hOutput, buf, FALSE, x, y, sc_wAttrWhite, width );
		y++;

		address+=4*4;
	}
}


