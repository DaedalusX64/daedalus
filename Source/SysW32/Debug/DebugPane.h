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

#ifndef DEBUGPANE_H__
#define DEBUGPANE_H__

class CDebugPane
{
	public:
		CDebugPane() : mX( 0 ), mY( 0 ), mWidth( 0 ), mHeight( 0 ) {}
		virtual ~CDebugPane() {}

		virtual bool			Initialise() = 0;
		virtual void			Destroy() = 0;

		virtual bool			IsValid() const = 0;

		virtual const CHAR *	GetName() const = 0;

		virtual void			Display( HANDLE hOutput ) = 0;

		virtual void			ScrollUp() = 0;			// Scroll up one line
		virtual void			ScrollDown() = 0;		// Scroll down one line
		virtual void			PageUp() = 0;			// Page up (by GetHeight() lines)
		virtual void			PageDown() = 0;			// Page down (by GetHeight() lines)
		virtual void			Home() = 0;				// Return to the default position

		//
		//
		//
		void					SetPos( u32 x, u32 y, u32 width, u32 height )	{ mX = x; mY = y; mWidth = width; mHeight = height; }
		void					SetPos( u32 x, u32 y )							{ mX = x; mY = y; }
		void					SetSize( u32 width, u32 height )				{ mWidth = width; mHeight = height; }

		u32						GetX() const						{ return mX; }
		u32						GetY() const						{ return mY; }
		u32						GetWidth() const					{ return mWidth; }
		u32						GetHeight() const					{ return mHeight; }

		void					Clear( HANDLE hOutput );

	protected:
		//
		// Utility functions
		//
		bool					WriteString( HANDLE hBuffer, LPCTSTR szString, BOOL bParse, s32 x, s32 y, WORD wAttr, s32 width );
		HANDLE					CreateBuffer( u32 width, u32 height );

	private:

		WORD					GetStringHighlight( char c ) const;
		void					ParseStringHighlights( LPSTR szString, WORD * arrwAttributes, WORD wAttr ) const;

		u32						mX;
		u32						mY;
		u32						mWidth;
		u32						mHeight;

};

#endif // DEBUGPANE_H__
