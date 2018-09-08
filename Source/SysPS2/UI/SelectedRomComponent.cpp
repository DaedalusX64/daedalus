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
#include "SelectedRomComponent.h"

#include "RomPreferencesScreen.h"
#include "AdvancedOptionsScreen.h"
#include "CheatOptionsScreen.h"

#include "UIContext.h"
#include "UIScreen.h"
#include "UISetting.h"
#include "UICommand.h"
#include "UISpacer.h"
#include "SysPSP/Graphics/DrawText.h"
#include "Graphics/ColourValue.h"

#include "Core/ROM.h"

#include "Input/InputManager.h"

#include <pspctrl.h>
#include <pspgu.h>

namespace
{
	const u32		TEXT_AREA_TOP = 15+16+16;
	const u32		TEXT_AREA_LEFT = 40;
	const u32		TEXT_AREA_RIGHT = 440;

	const s32		DESCRIPTION_AREA_TOP = 0;		// We render text aligned from the bottom, so this is largely irrelevant
	const s32		DESCRIPTION_AREA_BOTTOM = 272-10;
	const s32		DESCRIPTION_AREA_LEFT = 16;
	const s32		DESCRIPTION_AREA_RIGHT = 480-16;


}

//*************************************************************************************
//
//*************************************************************************************
class ISelectedRomComponent : public CSelectedRomComponent
{
	public:

		ISelectedRomComponent( CUIContext * p_context, CFunctor * on_start_emulation );
		~ISelectedRomComponent();

		// CUIComponent
		virtual void				Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();

		virtual void				SetRomID( const RomID & rom_id )			{ mRomID = rom_id; }

	private:
		void						EditPreferences();
		void						AdvancedOptions();
		void						CheatOptions();
		void						StartEmulation();

	private:
		CFunctor *					OnStartEmulation;

		CUIElementBag				mElements;

		RomID						mRomID;
};

//*************************************************************************************
//
//*************************************************************************************
CSelectedRomComponent::CSelectedRomComponent( CUIContext * p_context )
:	CUIComponent( p_context )
{
}

//*************************************************************************************
//
//*************************************************************************************
CSelectedRomComponent::~CSelectedRomComponent()
{
}

//*************************************************************************************
//
//*************************************************************************************
CSelectedRomComponent *	CSelectedRomComponent::Create( CUIContext * p_context, CFunctor * on_start_emulation )
{
	return new ISelectedRomComponent( p_context, on_start_emulation );
}

//*************************************************************************************
//
//*************************************************************************************
ISelectedRomComponent::ISelectedRomComponent( CUIContext * p_context, CFunctor * on_start_emulation )
:	CSelectedRomComponent( p_context )
,	OnStartEmulation( on_start_emulation )
{
	mElements.Add( new CUICommandImpl( new CMemberFunctor< ISelectedRomComponent >( this, &ISelectedRomComponent::EditPreferences ), "Edit Preferences", "Edit various preferences for this rom." ) );
	mElements.Add( new CUICommandImpl( new CMemberFunctor< ISelectedRomComponent >( this, &ISelectedRomComponent::AdvancedOptions ), "Advanced Options", "Edit advanced options for this rom." ) );
	mElements.Add( new CUICommandImpl( new CMemberFunctor< ISelectedRomComponent >( this, &ISelectedRomComponent::CheatOptions ), "Cheats", "Enable and select cheats for this rom." ) );

	mElements.Add( new CUISpacer( 16 ) );

	u32 i = mElements.Add( new CUICommandImpl( new CMemberFunctor< ISelectedRomComponent >( this, &ISelectedRomComponent::StartEmulation ), "Start Emulation", "Start emulating the selected rom." ) );

	mElements.SetSelected( i );
}

//*************************************************************************************
//
//*************************************************************************************
ISelectedRomComponent::~ISelectedRomComponent()
{
	delete OnStartEmulation;
}

//*************************************************************************************
//
//*************************************************************************************
void	ISelectedRomComponent::Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons )
{
	if(old_buttons != new_buttons)
	{
		if( new_buttons & PSP_CTRL_UP )
		{
			mElements.SelectPrevious();
		}
		if( new_buttons & PSP_CTRL_DOWN )
		{
			mElements.SelectNext();
		}

		CUIElement *	element( mElements.GetSelectedElement() );
		if( element != NULL )
		{
			if( new_buttons & PSP_CTRL_LEFT )
			{
				element->OnPrevious();
			}
			if( new_buttons & PSP_CTRL_RIGHT )
			{
				element->OnNext();
			}
			if( new_buttons & (PSP_CTRL_CROSS|PSP_CTRL_START) )
			{
				element->OnSelected();
			}
		}
	}
}

//*************************************************************************************
//
//*************************************************************************************
void	ISelectedRomComponent::Render()
{
	mElements.Draw( mpContext, TEXT_AREA_LEFT, TEXT_AREA_RIGHT, AT_CENTRE, TEXT_AREA_TOP );

	CUIElement *	element( mElements.GetSelectedElement() );
	if( element != NULL )
	{
		const char *		p_description( element->GetDescription() );

		mpContext->DrawTextArea( DESCRIPTION_AREA_LEFT,
								 DESCRIPTION_AREA_TOP,
								 DESCRIPTION_AREA_RIGHT - DESCRIPTION_AREA_LEFT,
								 DESCRIPTION_AREA_BOTTOM - DESCRIPTION_AREA_TOP,
								 p_description,
								 DrawTextUtilities::TextWhite,
								 VA_BOTTOM );
	}
}

//*************************************************************************************
//
//*************************************************************************************
void	ISelectedRomComponent::EditPreferences()
{
	CRomPreferencesScreen *	edit_preferences( CRomPreferencesScreen::Create( mpContext, mRomID ) );
	edit_preferences->Run();
	delete edit_preferences;
}

//*************************************************************************************
//
//*************************************************************************************
void	ISelectedRomComponent::AdvancedOptions()
{
	CAdvancedOptionsScreen *	advanced_options( CAdvancedOptionsScreen::Create( mpContext, mRomID ) );
	advanced_options->Run();
	delete advanced_options;
}

//*************************************************************************************
//
//*************************************************************************************
void	ISelectedRomComponent::CheatOptions()
{
	CCheatOptionsScreen *	cheat_options( CCheatOptionsScreen::Create( mpContext, mRomID ) );
	cheat_options->Run();
	delete cheat_options;
}
//*************************************************************************************
//
//*************************************************************************************
void	ISelectedRomComponent::StartEmulation()
{
	if(OnStartEmulation != NULL)
	{
		(*OnStartEmulation)();
	}
}
