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

#ifndef DEBUGPANECP2DIS_H__
#define DEBUGPANECP2DIS_H__

//*****************************************************************************
//* Include files
//*****************************************************************************
#include "DebugPane.h"

//*****************************************************************************
//* Class definitions
//*****************************************************************************
class CDebugPaneCP2Dis : public CDebugPane
{
	public:
		CDebugPaneCP2Dis()	: mMemoryAddress( 0 ) {}
		~CDebugPaneCP2Dis() {}

		bool			IsValid() const				{ return true; }

		bool			Initialise()				{ return true; }
		void			Destroy()					{ }

		const CHAR *	GetName() const				{ return "CP2 Dissasembly"; }

		//
		// Present ourselves at the specified x/y with width/height
		//
		void			Display( HANDLE hOutput );

		//
		// Display a specific line
		//
		void			ScrollUp()					{ mMemoryAddress -= 4; }
		void			ScrollDown()				{ mMemoryAddress += 4; }
		void			PageUp()					{ mMemoryAddress -= 4 * GetHeight(); }
		void			PageDown()					{ mMemoryAddress += 4 * GetHeight(); }
		void			Home()						{ mMemoryAddress = ~0; }

		void			SetAddress( u32 address )	{ mMemoryAddress = address; }

	private:
		u32				mMemoryAddress;
};


#endif // DEBUGPANECP2DIS_H__
