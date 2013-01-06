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
#include "DebugPane.h"

//*****************************************************************************
//* Static variables
//*****************************************************************************
static const WORD sc_wAttrWhite			= FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;

//*****************************************************************************
// Initialise the debug pane. This creates a buffer for us to render into
//*****************************************************************************
HANDLE	CDebugPane::CreateBuffer( u32 width, u32 height )
{
	HANDLE hBuffer;
	COORD size;

	size.X = s16( width );
	size.Y = s16( height );

	// Create a new screen buffer to write to.
	hBuffer = CreateConsoleScreenBuffer(
									   GENERIC_READ |           // read-write access
									   GENERIC_WRITE,
									   0,                       // not shared
									   NULL,                    // no security attributes
									   CONSOLE_TEXTMODE_BUFFER, // must be TEXTMODE
									   NULL);                   // reserved; must be NULL
	if (hBuffer == INVALID_HANDLE_VALUE)
	{
		DAEDALUS_ERROR( "Couldn't create the output buffer" );
		return hBuffer;
	}

	// Resize to some reasonable size
	if ( !SetConsoleScreenBufferSize( hBuffer, size ) )
	{
		DAEDALUS_ERROR( "Couldn't size the output buffer" );
		// Still return the handle
		return hBuffer;
	}

	return hBuffer;
}

//*****************************************************************************
//
//*****************************************************************************
void	CDebugPane::Clear( HANDLE hOutput )
{
    COORD coord;
	DWORD cWritten;

	coord.X = s16( GetX() );            // start at first cell
	for ( coord.Y = s16( GetY() ); coord.Y < s16( GetY()+GetHeight() ); coord.Y++ )
	{
		FillConsoleOutputAttribute( hOutput, sc_wAttrWhite, GetWidth(), coord, &cWritten );
		FillConsoleOutputCharacter( hOutput, ' ', GetWidth(), coord, &cWritten );
	}
}


//*****************************************************************************
//
//*****************************************************************************
WORD	CDebugPane::GetStringHighlight( char c ) const
{
	switch(c)
	{
		case 'r': return FOREGROUND_RED;
		case 'g': return FOREGROUND_GREEN;
		case 'b': return FOREGROUND_BLUE;
		case 'c': return FOREGROUND_GREEN|FOREGROUND_BLUE;
		case 'm': return FOREGROUND_RED|FOREGROUND_BLUE;
		case 'y': return FOREGROUND_RED|FOREGROUND_GREEN;
		case 'w': return FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;
		case 'R': return FOREGROUND_RED|FOREGROUND_INTENSITY;
		case 'G': return FOREGROUND_INTENSITY|FOREGROUND_GREEN;
		case 'B': return FOREGROUND_INTENSITY|FOREGROUND_BLUE;
		case 'C': return FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_BLUE;
		case 'M': return FOREGROUND_INTENSITY|FOREGROUND_RED|FOREGROUND_BLUE;
		case 'Y': return FOREGROUND_INTENSITY|FOREGROUND_RED|FOREGROUND_GREEN;
		case 'W': return FOREGROUND_INTENSITY|FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;
		default: return FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void	CDebugPane::ParseStringHighlights( LPSTR szString, WORD * arrwAttributes, WORD wAttr ) const
{
	WORD wCurrAttribute = wAttr;
	int iIn, iOut;
	int nMax = lstrlen(szString);

	for (iIn = 0, iOut = 0; iIn < nMax; iIn++)
	{
		if (szString[iIn] == '[')
		{
			int highlight = GetStringHighlight( szString[iIn+1] );
			if(highlight != -1)
			{
				wCurrAttribute = (WORD)highlight;
			}
			else
			{
				switch (szString[iIn+1])
				{
				case '[':
				case ']':
					szString[iOut] = szString[iIn+1];
					arrwAttributes[iOut] = wCurrAttribute;

					iOut++;
					break;
				}
			}

			// Skip colour character
			iIn++;
		}
		else if (szString[iIn] == ']')
		{
			wCurrAttribute = wAttr;
		}
		else
		{
			szString[iOut] = szString[iIn];
			arrwAttributes[iOut] = wCurrAttribute;

			iOut++;
		}
	}
	szString[iOut] = '\0';

}

//*****************************************************************************
// Write a string of characters to a screen buffer.
//*****************************************************************************
bool	CDebugPane::WriteString( HANDLE hBuffer, LPCTSTR szString, BOOL bParse, s32 x, s32 y, WORD wAttr, s32 width)
{
    u32 cWritten;
    BOOL bSuccess;
    COORD coord;
	u16 wNumToWrite;
	TCHAR szBuffer[2048+1];
	u16 arrwAttributes[2048];

    coord.X = s16( x );            // start at first cell
    coord.Y = s16( y );            //   of first row

	lstrcpyn(szBuffer, szString, 2048);

	if (bParse)
	{
		ParseStringHighlights(szBuffer, arrwAttributes, wAttr);
	}

	wNumToWrite = lstrlen(szBuffer);
	if (wNumToWrite > width)
		wNumToWrite = u16( width );

    bSuccess = WriteConsoleOutputCharacter(
        hBuffer,        // screen buffer handle
        szBuffer,				// pointer to source string
        wNumToWrite,			// length of string
        coord,					// first cell to write to
        &cWritten);				// actual number written
    if (!bSuccess)
        return false;

	// Copy the visible portion to the display
	if (bParse)
	{
		/*bSuccess = */WriteConsoleOutputAttribute(
			hBuffer,
			arrwAttributes,
			wNumToWrite,
			coord,
			&cWritten);
	}
	else
	{
		// Just clear
		/*bSuccess = */FillConsoleOutputAttribute(
			hBuffer,
			wAttr,
			wNumToWrite,		// Clear to end
			coord,
			&cWritten);
	}

	// Clear the end of the buffer
	if (coord.X + wNumToWrite < width)
	{
		coord.X += wNumToWrite;
		/*bSuccess = */FillConsoleOutputAttribute(
			hBuffer,
			wAttr,
			width - wNumToWrite,		// Clear to end
			coord,
			&cWritten);

		/*bSuccess = */FillConsoleOutputCharacter(hBuffer,
			' ', width - coord.X, coord, &cWritten);
	}

	return true;
}
