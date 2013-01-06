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

#ifndef DEBUGPANECP0REGS_H__
#define DEBUGPANECP0REGS_H__

//*****************************************************************************
//* Include files
//*****************************************************************************
#include "DebugPane.h"

//*****************************************************************************
//* Class definitions
//*****************************************************************************
class CDebugPaneCP0Regs : public CDebugPane
{
	public:
		CDebugPaneCP0Regs()	{}
		~CDebugPaneCP0Regs() {}

		bool			IsValid() const				{ return true; }

		bool			Initialise()				{ return true; }
		void			Destroy()					{ }

		const CHAR *	GetName() const				{ return "CP0 Registers"; }

		//
		// Present ourselves at the specified x/y with width/height
		//
		void			Display( HANDLE hOutput );

		//
		// Display a specific line
		//
		void			ScrollUp()					{}
		void			ScrollDown()				{}
		void			PageUp()					{}
		void			PageDown()					{}
		void			Home()						{}
};


#endif // DEBUGPANECP0REGS_H__
