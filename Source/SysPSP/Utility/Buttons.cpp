/*
Copyright (C) 2010 Salvy6735

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


#include "stdafx.h"
#include "Buttons.h"

#include "Utility/ModulePSP.h"

#include <pspctrl.h>

//
/****** Wrapper for Home Button functions ******/
//
extern "C" 
{ 
	/* Impose Home button */
	void SetImposeHomeButton();
}


//*****************************************************************************
//	Init our buttons, various stats etc
//*****************************************************************************
bool InitHomeButton()
{
	// Start our stack for either kernel or usermode buttons
	//
	int impose = CModule::Load("imposectrl.prx");

	//
	// if imposectrl.prx loaded correctly, let's do some magic to take (forcely) control of HOME button 
	//
	if(impose >= 0)
	{
		//
		// Unset home button and imposed to allow use it as normal button
		//
		SetImposeHomeButton(); 

		//
		// Stop and unload imposectrl.prx since we only needed it once to impose HOME button
		//
		CModule::Unload( impose );

		return true;
	}

	//
	// Errg for some reasons we couldn't load imposectrl.prx, OFW?
	// Anyways it'll sort of works in >=6XX
	// BTW We didn't load anything.. so no point to unload anything at this point
	//
	return false;	// In < 5XX Daedalus will be crippled.. warn user what up..
}

//*****************************************************************************
// // Function to avoid reading buttons in tight loops
//*****************************************************************************
/*
void DaedalusReadButtons(u32 buttons)	
{ 
	gButtons.type = buttons; 
}
*/



