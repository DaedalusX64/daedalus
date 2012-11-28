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

#ifndef BUTTONS_H
#define BUTTONS_H
//
// This a wrapper to easy up the use of kernel buttons prx
//
//*************************************************************************************
//
//*************************************************************************************
/*struct PSPButtons
{
	//u32		type;			// input type of our buttons, X,O,[] etc, kernel inputs supported too.
	//u32		style;			// input style for either kernel or non-kernel button
	bool	kmode;			// returns true if kernelbuttons.prx loaded correctly
};
*/

// Extern
//
//extern PSPButtons gButtons;
//*************************************************************************************
//
//*************************************************************************************

// Function to init our buttons funcs etc

bool InitHomeButton();

// Function to read our buttons

//void DaedalusReadButtons(u32 buttons);

#endif	//BUTTONS_H
