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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef DEBUGPANEOUTPUT_H__
#define DEBUGPANEOUTPUT_H__

//*****************************************************************************
//* Include files
//*****************************************************************************
#include "DebugPane.h"

//*****************************************************************************
//* Class definitions
//*****************************************************************************
class CDebugPaneOutput : public CDebugPane
{
	public:
		CDebugPaneOutput();
		~CDebugPaneOutput() {}

		bool			IsValid() const;

		bool			Initialise();
		void			Destroy();

		const CHAR *	GetName() const				{ return "Output"; }

		//
		// Insert a line at the bottom of the buffer
		//
		void			InsertLine( const TCHAR * pszBuffer, const WORD * arrwAttributes );

		//
		// Overwrite the specified line
		//
		void			OverwriteLineStart();
		void			OverwriteLine( const TCHAR * pszBuffer, const WORD * arrwAttributes );
		void			OverwriteLineEnd();

		//
		// Present ourselves at the specified x/y with width/height
		//
		void			Display( HANDLE hOutput );


		//
		// Display a specific line
		//
		void			ScrollUp()					{ SetOffset( mScrollOffset-1 ); }
		void			ScrollDown()				{ SetOffset( mScrollOffset+1 ); }
		void			PageUp()					{ SetOffset( mScrollOffset - GetHeight() ); }
		void			PageDown()					{ SetOffset( mScrollOffset + GetHeight() ); }
		void			Home()						{ SetOffset( 0 ); }

		void			SetOffset( s32 offset )		{ mScrollOffset = offset; while ( mScrollOffset < 0 ) mScrollOffset += MAX_LINES; }

	private:
		HANDLE			GetBuffer() const			{ return mhConsoleBuffer; }

	private:

		struct SLineInfo
		{
			TCHAR		Text[ 81 ];
			u16			Attributes[ 81 ];
		};

		enum { MAX_LINES = 1000 };

		s32				mScrollOffset;
		s32				mMsgOverwriteLine;
		HANDLE			mhConsoleBuffer;

		SLineInfo		mBuffer[ MAX_LINES ];
		u32				mCurrentLine;
};


#endif // DEBUGPANEOUTPUT_H__
