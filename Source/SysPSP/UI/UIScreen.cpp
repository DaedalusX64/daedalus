/*
Copyright (C) 2006 StrmnNrmn

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
#include "UIScreen.h"
#include "UIContext.h"

#include "Math/Vector2.h"

#include "Utility/Timer.h"
#include "Graphics/GraphicsContext.h"

#include "SysPSP/Utility/Buttons.h"

#include <pspctrl.h>

//const u32 PSP_BUTTONS_MASK( 0xffff );		// Mask off e.g. PSP_CTRL_HOME, PSP_CTRL_HOLD etc
//*************************************************************************************
//
//*************************************************************************************
CUIScreen::CUIScreen( CUIContext * p_context )
:	mpContext( p_context )
{
}

//*************************************************************************************
//
//*************************************************************************************
CUIScreen::~CUIScreen()
{
}

//*************************************************************************************
//
//*************************************************************************************
void	CUIScreen::Run()
{
	DAEDALUS_ASSERT( mpContext != NULL, "No context" );

	u32		old_buttons = gButtons.type;

	static const s32	STICK_DEADZONE = 20;

	CTimer		timer;

	// Simple rom chooser
	while( !IsFinished() )
	{
		// Only stick reading is needed.
		SceCtrlData		pad2;
		sceCtrlPeekBufferPositive(&pad2, 1);

		float		elapsed_time( timer.GetElapsedSeconds() );

		s32		stick_x( pad2.Lx - 128 );
		s32		stick_y( pad2.Ly - 128 );


		if(stick_x >= -STICK_DEADZONE && stick_x <= STICK_DEADZONE)
		{
			stick_x = 0;
		}
		if(stick_y >= -STICK_DEADZONE && stick_y <= STICK_DEADZONE)
		{
			stick_y = 0;
		}

		v2	stick;
		stick.x = f32(stick_x) / 128.0f;
		stick.y = f32(stick_y) / 128.0f;

		mpContext->Update( elapsed_time );

		Update( elapsed_time, stick, old_buttons, gButtons.type );

		// Otherwise gButtons.type will fail
		old_buttons = gButtons.type;

		mpContext->BeginRender();

		Render();

		mpContext->EndRender();
	}

	//
	//	Wait until all buttons are release before continuing
	//
	
	/*while( (pad.Buttons & PSP_BUTTONS_MASK) != 0 )
	{
		if( gKernelButtons.mode )
			KernelPeekBufferPositive(&pad, 1); 
		else
			sceCtrlPeekBufferPositive(&pad, 1); 
	}*/

}
