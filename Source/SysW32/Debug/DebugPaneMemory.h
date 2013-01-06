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

#ifndef DEBUGPANEMEMORY_H__
#define DEBUGPANEMEMORY_H__

//*****************************************************************************
//* Include files
//*****************************************************************************
#include "DebugPane.h"

//*****************************************************************************
//* Class definitions
//*****************************************************************************
class CDebugPaneMemory : public CDebugPane
{
	public:
		CDebugPaneMemory() : mMemoryAddress( 0 )	{}
		~CDebugPaneMemory() {}

		bool			IsValid() const				{ return true; }

		bool			Initialise()				{ return true; }
		void			Destroy()					{ }

		const CHAR *	GetName() const				{ return "Memory"; }


		//
		// Present ourselves at the specified x/y with width/height
		//
		void			Display( HANDLE hOutput );

		//
		// Display a specific line
		//
		void			ScrollUp()					{ mMemoryAddress -= 4 * 4; }
		void			ScrollDown()				{ mMemoryAddress += 4 * 4; }
		void			PageUp()					{ mMemoryAddress -= 4 * 4 * GetHeight(); }
		void			PageDown()					{ mMemoryAddress += 4 * 4 * GetHeight(); }
		void			Home()						{ mMemoryAddress = 0x80000000; }		// No "home" defined for memory! Just use the start of ram

		void			SetAddress( u32 address )		{ mMemoryAddress = address; }

	private:
		u32				mMemoryAddress;
};



#endif // DEBUGPANEMEMORY_H__
