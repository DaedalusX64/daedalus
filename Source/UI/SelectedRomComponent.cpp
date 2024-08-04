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


#include "Base/Types.h"

#include "Core/ROM.h"
#include "Graphics/ColourValue.h"
#include "Input/InputManager.h"
#include "DrawTextUtilities.h"
#include "AdvancedOptionsScreen.h"
#include "CheatOptionsScreen.h"
#include "Menu.h"
#include "RomPreferencesScreen.h"
#include "SelectedRomComponent.h"
#include "UIContext.h"
#include "UIScreen.h"
#include "UISetting.h"
#include "UICommand.h"
#include "UISpacer.h"


class ISelectedRomComponent : public CSelectedRomComponent
{
	public:

		ISelectedRomComponent( CUIContext * p_context, std::function<void()> on_start_emulation );
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
		std::function<void()> OnStartEmulation;

		CUIElementBag				mElements;

		RomID						mRomID;
};

CSelectedRomComponent::CSelectedRomComponent( CUIContext * p_context )
:	CUIComponent( p_context )
{}


CSelectedRomComponent::~CSelectedRomComponent() {}


CSelectedRomComponent *	CSelectedRomComponent::Create( CUIContext * p_context, std::function<void()> on_start_emulation )
{
	return new ISelectedRomComponent( p_context, on_start_emulation );
}


ISelectedRomComponent::ISelectedRomComponent( CUIContext * p_context, std::function<void()> on_start_emulation )
:	CSelectedRomComponent( p_context )
,	OnStartEmulation( on_start_emulation )
{
	mElements.Add( std::make_unique<CUICommandImpl>( std::bind(&ISelectedRomComponent::EditPreferences, this ), "Edit Preferences", "Edit various preferences for this rom." ) );
	mElements.Add( std::make_unique<CUICommandImpl>( std::bind(&ISelectedRomComponent::AdvancedOptions, this ), "Advanced Options", "Edit advanced options for this rom." ) );
	mElements.Add( std::make_unique<CUICommandImpl>( std::bind(&ISelectedRomComponent::CheatOptions, this ), "Cheats", "Enable and select cheats for this rom." ) );

	mElements.Add(std::make_unique<CUISpacer>( 16 ) );

	u32 i = mElements.Add(std::make_unique<CUICommandImpl>( std::bind(&ISelectedRomComponent::StartEmulation, this ), "Start Emulation", "Start emulating the selected rom." ) );

	mElements.SetSelected( i );
}


ISelectedRomComponent::~ISelectedRomComponent() {}


void	ISelectedRomComponent::Update( float elapsed_time[[maybe_unused]], const v2 & stick[[maybe_unused]], u32 old_buttons, u32 new_buttons )
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

		auto element = mElements.GetSelectedElement();
		if( element != nullptr )
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


void	ISelectedRomComponent::Render()
{
	mElements.Draw( mpContext, LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_CENTRE, BELOW_MENU_MIN );

	auto element = mElements.GetSelectedElement();
	if( element != NULL )
	{
		const auto p_description = element->GetDescription();

		mpContext->DrawTextArea( DESCRIPTION_AREA_LEFT,
								 DESCRIPTION_AREA_TOP,
								 DESCRIPTION_AREA_RIGHT - DESCRIPTION_AREA_LEFT,
								 DESCRIPTION_AREA_BOTTOM - DESCRIPTION_AREA_TOP,
								 p_description,
								 DrawTextUtilities::TextWhite,
								 VA_BOTTOM );
	}
}

void	ISelectedRomComponent::EditPreferences()
{
	auto edit_preferences = CRomPreferencesScreen::Create( mpContext, mRomID );
	edit_preferences->Run();
}


void	ISelectedRomComponent::AdvancedOptions()
{
	auto advanced_options = CAdvancedOptionsScreen::Create( mpContext, mRomID );
	advanced_options->Run();
}

void	ISelectedRomComponent::CheatOptions()
{
	auto cheat_options = CCheatOptionsScreen::Create( mpContext, mRomID );
	cheat_options->Run();
}


void	ISelectedRomComponent::StartEmulation()
{
	if(OnStartEmulation != NULL)
	{
		OnStartEmulation();
	}
}
