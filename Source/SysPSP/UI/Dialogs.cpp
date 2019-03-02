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

CDialog	gShowDialog;


CDialog::~CDialog() {}

bool CDialog::Render( CUIContext * p_context, const char* message, bool only_dialog )
{
	SceCtrlData pad;
	//
	//	Wait until all buttons are release before continuing
	//  We do this to avoid any undesirable button input that can be triggered or passed whithin the GUI.
	//
	while( (pad.Buttons & 0xffff) != 0 )
	{
		sceCtrlPeekBufferPositive(&pad, 1);
	}

	for(;;)
	{
		p_context->BeginRender();

		// Draw our box for this dialog
		p_context->DrawRect( 100, 116, 280, 54, c32::White );
		p_context->DrawRect( 102, 118, 276, 50, c32(128, 128, 128) ); // Magic Grey

		//Render our text for our dialog
		p_context->SetFontStyle( CUIContext::FS_HEADING );
		p_context->DrawTextAlign(0,480,AT_CENTRE,135,message,DrawTextUtilities::TextRed);
		p_context->SetFontStyle( CUIContext::FS_REGULAR );

		// Only show a dialog, do not give a choice
		if(only_dialog)
			p_context->DrawTextAlign(0,480,AT_CENTRE,158,"Press any button to continue",DrawTextUtilities::TextWhite);
		else
			p_context->DrawTextAlign(0,480,AT_CENTRE,158,"(X) Confirm       (O) Cancel",DrawTextUtilities::TextWhite);

		p_context->EndRender();

		sceCtrlPeekBufferPositive(&pad, 1);
		if( only_dialog && (pad.Buttons & 0xffff)!= 0 )	 // Mask off button
			return true;
		else if( pad.Buttons & PSP_CTRL_CROSS )
			return true;
		else if( pad.Buttons & PSP_CTRL_CIRCLE )
			return false;
	}
}
