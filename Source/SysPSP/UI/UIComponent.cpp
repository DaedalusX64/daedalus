/*
Copyright (C) 2007 StrmnNrmn

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
#include "UIComponent.h"
#include "UIContext.h"

#include "SysPSP/Graphics/DrawText.h"

//*************************************************************************************
//
//*************************************************************************************
CUIComponent::CUIComponent( CUIContext * p_context )
:	mpContext( p_context )
{
}

//*************************************************************************************
//
//*************************************************************************************
CUIComponent::~CUIComponent()
{
}


//*************************************************************************************
//
//*************************************************************************************
CUIComponentScreen::CUIComponentScreen( CUIContext * p_context, CUIComponent * component, const char * title )
:	CUIScreen( p_context )
,	mComponent( component )
,	mTitle( title )
{
}

//*************************************************************************************
//
//*************************************************************************************
CUIComponentScreen::~CUIComponentScreen()
{
	delete mComponent;
}

//*************************************************************************************
//
//*************************************************************************************
CUIComponentScreen *	CUIComponentScreen::Create( CUIContext * p_context, CUIComponent * component, const char * title )
{
	return new CUIComponentScreen( p_context, component, title );
}

//*************************************************************************************
//
//*************************************************************************************
void	CUIComponentScreen::Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons )
{
	mComponent->Update( elapsed_time, stick, old_buttons, new_buttons );
}

//*************************************************************************************
//
//*************************************************************************************
void	CUIComponentScreen::Render()
{
	mpContext->ClearBackground();

	const u32				TITLE_AREA_TOP = 10;

	s32 y( TITLE_AREA_TOP );
	mpContext->DrawTextAlign( 0, mpContext->GetScreenWidth(), AT_CENTRE, y, mTitle.c_str(), mpContext->GetDefaultTextColour() );

	mComponent->Render();
}

//*************************************************************************************
//
//*************************************************************************************
bool	CUIComponentScreen::IsFinished() const
{
	return mComponent->IsFinished();
}
