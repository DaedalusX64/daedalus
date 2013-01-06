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
#include "DebugPaneOutput.h"

//*****************************************************************************
//* Static variables
//*****************************************************************************
static const WORD sc_wAttrWhite			= FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;


//*****************************************************************************
//
//*****************************************************************************
CDebugPaneOutput::CDebugPaneOutput() :
	mScrollOffset( 0 ), mMsgOverwriteLine(-1), mhConsoleBuffer( INVALID_HANDLE_VALUE ),
	mCurrentLine( 0 )
{
	ZeroMemory( mBuffer, sizeof( mBuffer ) );
}

//*****************************************************************************
//
//*****************************************************************************
bool	CDebugPaneOutput::IsValid() const
{
	//return mhConsoleBuffer != INVALID_HANDLE_VALUE;
	return true;
}


//*****************************************************************************
//
//*****************************************************************************
bool	CDebugPaneOutput::Initialise()
{
	//mhConsoleBuffer = CreateBuffer( 80, 120 );
	return IsValid();
}

//*****************************************************************************
// Destroy the debug pane, freeing any resources
//*****************************************************************************
void	CDebugPaneOutput::Destroy()
{
	// Maybe preserve this - we might want to use the contents later
	/*if (mhConsoleBuffer != INVALID_HANDLE_VALUE)
	{
		CloseHandle( mhConsoleBuffer );
		mhConsoleBuffer = INVALID_HANDLE_VALUE;
	}*/
}

//*****************************************************************************
//
//*****************************************************************************
void CDebugPaneOutput::InsertLine( const TCHAR * pszBuffer, const WORD * arrwAttributes )
{
/*
	BOOL bSuccess;
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
	SMALL_RECT srctScrollRect, srctClipRect;
	CHAR_INFO chiFill;
	COORD coordDest;
	DWORD cWritten;

	bSuccess = GetConsoleScreenBufferInfo( GetBuffer(), &csbiInfo );
	if (!bSuccess)
		return;

	// Scan through szBuffer and set up attributes
	//ParseStringHighlights(pszBuffer, arrwAttributes, sc_wAttrWhite);

	u32 length = lstrlen(pszBuffer);

	// Scroll the contents of the buffer up by one line
	// Define the scrolling rectangle (we lose the top line)
	srctScrollRect.Top = 1;
	srctScrollRect.Bottom = csbiInfo.dwSize.Y - 1;
	srctScrollRect.Left = 0;
	srctScrollRect.Right = csbiInfo.dwSize.X - 1;

	// The destination for the scroll rectangle is one row up.
	coordDest.X = 0;
	coordDest.Y = 0;

	// The clipping rectangle is the same as the scrolling rectangle.
	// The destination row is left unchanged.
	srctClipRect = srctScrollRect;

	// Fill the bottom row
	chiFill.Attributes = sc_wAttrWhite;
	chiFill.Char.AsciiChar = ' ';

	// Scroll up one line.
	ScrollConsoleScreenBuffer(
		GetBuffer(),         // screen buffer handle
		&srctScrollRect, // scrolling rectangle
		&srctClipRect,   // clipping rectangle
		coordDest,       // top left destination cell
		&chiFill);       // fill character and color

	//
	// Move the overwrite line up one when we scroll
	//
	if ( mMsgOverwriteLine >= 0 )
		mMsgOverwriteLine--;

	// Write the output to the buffer
	COORD coordCursor;

	// Write the new string to the bottom of the buffer
	coordCursor.X = 0;
	coordCursor.Y = csbiInfo.dwSize.Y - 1;

	WriteConsoleOutputCharacter(
		GetBuffer(),            // screen buffer handle
		pszBuffer,				// pointer to source string
		length,					// length of string
		coordCursor,			// first cell to write to
		&cWritten);				// actual number written

	WriteConsoleOutputAttribute(
		GetBuffer(),
		arrwAttributes,
		length,
		coordCursor,
		&cWritten);*/

	u32 len = lstrlen( pszBuffer );
	if ( len > 80 )
		len = 80;

	ZeroMemory( &mBuffer[ mCurrentLine ], sizeof( mBuffer[ mCurrentLine ] ) );

	if ( len >  0 )
	{
		memcpy( mBuffer[ mCurrentLine ].Text,		pszBuffer,		len * sizeof( TCHAR ) );
		memcpy( mBuffer[ mCurrentLine ].Attributes,	arrwAttributes, len * sizeof( u16 ) );
	}

	mCurrentLine = ( mCurrentLine + 1 ) % MAX_LINES;
}


//*****************************************************************************
//
//*****************************************************************************
void	CDebugPaneOutput::OverwriteLineStart()
{
	BOOL bSuccess;
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;

	bSuccess = GetConsoleScreenBufferInfo( GetBuffer(), &csbiInfo );
	if (!bSuccess)
		return;

	//
	// Write a blank line to scroll up the previous message
	//
	WORD attributes[ 1 ];

	InsertLine( "", attributes );

	// Write the new string to the bottom of the buffer
	mMsgOverwriteLine = mCurrentLine;	//csbiInfo.dwSize.Y - 1;
}

//*****************************************************************************
//
//*****************************************************************************
void	CDebugPaneOutput::OverwriteLine( const TCHAR * pszBuffer, const WORD * arrwAttributes )
{
	/*DWORD cWritten;


	u32 length = lstrlen(pszBuffer);

	// Write the output to the buffer
	if ( mMsgOverwriteLine >= 0 )
	{
		COORD coord;

		// Write the new string to the bottom of the buffer
		coord.X = 0;
		coord.Y = mMsgOverwriteLine;

		WriteConsoleOutputCharacter(
			GetBuffer(),            // screen buffer handle
			pszBuffer,				// pointer to source string
			length,					// length of string
			coord,					// first cell to write to
			&cWritten);				// actual number written

		WriteConsoleOutputAttribute(
			GetBuffer(),
			arrwAttributes,
			length,
			coord,
			&cWritten);

			// Clear the end of the buffer
			if (length < 80)
			{
				coord.X += length;
				FillConsoleOutputCharacter(GetBuffer(),
					' ', 80 - length, coord, &cWritten);
			}
	}*/

	if ( mMsgOverwriteLine >= 0 && mMsgOverwriteLine < MAX_LINES )
	{
		u32 len = lstrlen( pszBuffer );
		if ( len > 80 )
			len = 80;

		ZeroMemory( &mBuffer[ mMsgOverwriteLine ], sizeof( mBuffer[ mMsgOverwriteLine ] ) );

		if ( len >  0 )
		{
			memcpy( mBuffer[ mMsgOverwriteLine ].Text,       pszBuffer,      len * sizeof( TCHAR ) );
			memcpy( mBuffer[ mMsgOverwriteLine ].Attributes, arrwAttributes, len * sizeof( u16 ) );
		}
	}
}


//*****************************************************************************
//
//*****************************************************************************
void	CDebugPaneOutput::OverwriteLineEnd()
{
}


//#define NUM_OUTPUT_LINES 60
//*****************************************************************************
// Copy the visible portion to the display
//*****************************************************************************
void	CDebugPaneOutput::Display( HANDLE hOutput )
{
/*	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
	SMALL_RECT srctReadRect;
	SMALL_RECT srctWriteRect;
	CHAR_INFO chiBuffer[ NUM_OUTPUT_LINES * 80 ];
	COORD coordBufSize;
	COORD coordBufCoord;
	BOOL bSuccess;

	u32 x = GetX();
	u32 y = GetY();
	u32 width = GetWidth();
	u32 height = GetHeight();

	bSuccess = GetConsoleScreenBufferInfo(GetBuffer(), &csbiInfo);
	if (!bSuccess)
		return;

	// Offset top/left by scroll amount
	srctReadRect.Left = (SHORT)(csbiInfo.dwSize.X - ( width - 0 ));
	srctReadRect.Top  = (SHORT)(csbiInfo.dwSize.Y - ( height - mScrollOffset ));


	if (srctReadRect.Left < 0)
		srctReadRect.Left = 0;

	if (srctReadRect.Top < 0)
		srctReadRect.Top = 0;

	srctReadRect.Right = srctReadRect.Left + width;			// -1?
	srctReadRect.Bottom = srctReadRect.Top + height;		// -1?

	// The temporary buffer size is NUM_OUTPUT_LINES rows x 80 columns.
	coordBufSize.X = width;
	coordBufSize.Y = height;

	// The top left destination cell of the temporary buffer is
	// row 0, col 0.
	coordBufCoord.X = 0;
	coordBufCoord.Y = 0;

	// Copy the block from the screen buffer to the temp. buffer.

	bSuccess = ReadConsoleOutput(
	   GetBuffer(),    // screen buffer to read from
	   chiBuffer,      // buffer to copy into
	   coordBufSize,   // col-row size of chiBuffer
	   coordBufCoord,  // top left dest. cell in chiBuffer
	   &srctReadRect); // screen buffer source rectangle
	if (!bSuccess)
		return;

	// Set the destination rectangle.
	srctWriteRect.Left = x;
	srctWriteRect.Top = y;
	srctWriteRect.Right = (x+width)-1;
	srctWriteRect.Bottom = (y+height)-1;

	// Copy from the temporary buffer to the new screen buffer.

	bSuccess = WriteConsoleOutput(
		hOutput,		// screen buffer to write to
		chiBuffer,        // buffer to copy from
		coordBufSize,     // col-row size of chiBuffer
		coordBufCoord,    // top left src cell in chiBuffer
		&srctWriteRect);  // dest. screen buffer rectangle
	if (!bSuccess)
		return;
*/
	DWORD cWritten;

	u32 num_to_write = 80;
	u32 width = GetWidth();
	u32 height = GetHeight();

	if ( num_to_write > width )
	{
		num_to_write = width;
	}

	for ( u32 i = 0; i < height; i++ )
	{
		u32 offset = ( mScrollOffset + MAX_LINES - i ) % MAX_LINES;			// Add MAX_LINES so that modular arithmetic works out with negative values
		u32 line = ( mCurrentLine + offset ) % MAX_LINES;

		const TCHAR * text = mBuffer[ line ].Text;
		const u16 * attributes = mBuffer[ line ].Attributes;

		COORD pos;

		pos.X = s16( GetX() );
		pos.Y = s16( GetY() + (height - 1) - i );

		WriteConsoleOutputCharacter(hOutput, text, num_to_write, pos, &cWritten);
		WriteConsoleOutputAttribute(hOutput, attributes, num_to_write, pos, &cWritten);
	}
}


