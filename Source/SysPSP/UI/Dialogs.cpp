/*
Copyright (C) 2011 Salvy6735

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
#include "Dialogs.h"

#include "UIContext.h"

#include "Graphics/ColourValue.h"
#include "SysPSP/Graphics/DrawText.h"


#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspgu.h>
//*************************************************************************************
//
//*************************************************************************************
#define PSP_CTRL_ALL		\
	(PSP_CTRL_SELECT |		\
	 PSP_CTRL_START |		\
	 PSP_CTRL_UP |			\
	 PSP_CTRL_RIGHT |		\
	 PSP_CTRL_DOWN |		\
	 PSP_CTRL_LEFT |		\
	 PSP_CTRL_LTRIGGER |	\
	 PSP_CTRL_RTRIGGER |	\
	 PSP_CTRL_TRIANGLE |	\
	 PSP_CTRL_CIRCLE |		\
	 PSP_CTRL_CROSS |		\
	 PSP_CTRL_SQUARE |		\
	 PSP_CTRL_HOME)
//*************************************************************************************
//
//*************************************************************************************
Dialog::~Dialog() 
{
}
//*************************************************************************************
//
//*************************************************************************************
inline bool ButtonPressed()
{

	SceCtrlData pad;
	sceCtrlReadBufferPositive(&pad, 1);

	return pad.Buttons & PSP_CTRL_ALL;
}
//*************************************************************************************
//
//*************************************************************************************
bool Dialog::ShowDialog(CUIContext * p_context, const char* message, bool only_dialog)
{
	SceCtrlData pad;
	sceCtrlReadBufferPositive(&pad, 1);

	// Only show a dialog, do not give a choice
	if(only_dialog)
	{
		// Draw our dialog box
		p_context->DrawRect( 100, 116, 280, 54, c32::White );
		p_context->DrawRect( 102, 118, 276, 50, c32(128, 128, 128) ); // Magic Grey

		//Render our text for our dialog
		p_context->SetFontStyle( CUIContext::FS_HEADING );
		p_context->DrawTextAlign(0,480,AT_CENTRE,135,message,DrawTextUtilities::TextRed);
		p_context->SetFontStyle( CUIContext::FS_REGULAR );
		p_context->DrawTextAlign(0,480,AT_CENTRE,158,"(X) Confirm       (O) Cancel",DrawTextUtilities::TextWhite);

		// This doesn't work as expected, it frezes the screen way too early before :(
		// Need to figure out a way to delay this to give some time to the dialog to displayed :/
		//
		// loop 4eva until any button is pressed, this is used to freeze the screen too.
		while(!ButtonPressed()){}
	
		return true;

	}

	// Draw our dialog box
	p_context->DrawRect( 100, 116, 280, 54, c32::White );
	p_context->DrawRect( 102, 118, 276, 50, c32(128, 128, 128) );

	//Render our text for our dialog
	p_context->SetFontStyle( CUIContext::FS_HEADING );
	p_context->DrawTextAlign(0,480,AT_CENTRE,135,message,DrawTextUtilities::TextRed);
	p_context->SetFontStyle( CUIContext::FS_REGULAR );
	p_context->DrawTextAlign(0,480,AT_CENTRE,158,"(X) Confirm       (O) Cancel",DrawTextUtilities::TextWhite);
	
	// This doesn't work as expected, it frezes the screen way too early before :(
	// Need to figure out a way to delay this to give some time to the dialog to displayed :/
	//
	// loop 4eva until specified buttons are pressed, this is used to freeze the screen too.
	//do
	//{

	//}
	//while(!ButtonTriggered(pad.Buttons));
//	while(!ButtonTriggered(pad.Buttons)){}

	if(pad.Buttons & PSP_CTRL_CROSS)
		return true;

	return false;
}

