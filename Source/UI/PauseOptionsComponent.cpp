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


#include "Core/CPU.h"
#include "Core/Dynamo.h"
#include "Core/ROM.h"
#include "Interface/SaveState.h"
#include "HLEGraphics/DisplayListDebugger.h"
#include "Graphics/ColourValue.h"
#include "Graphics/GraphicsContext.h"
#include "DrawTextUtilities.h"
#include "AdvancedOptionsScreen.h"
#include "SavestateSelectorComponent.h"
#include "CheatOptionsScreen.h"
#include "Dialogs.h"
#include "PauseOptionsComponent.h"
#include "Menu.h"
#include "RomPreferencesScreen.h"
#include "UIContext.h"
#include "UIScreen.h"
#include "UISetting.h"
#include "UICommand.h"
#include "UISpacer.h"


#include <functional>

extern bool gTakeScreenshot;
extern bool gTakeScreenshotSS;


class IPauseOptionsComponent : public CPauseOptionsComponent
{
	public:

		IPauseOptionsComponent( CUIContext * p_context,  std::function<void()> on_resume, std::function<void()> on_reset );
		~IPauseOptionsComponent();

		// CUIComponent
		virtual void				Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();

	private:
				void				OnResume();
				void				OnReset();
				void				EditPreferences();
				void				AdvancedOptions();
				void				CheatOptions();
				void				SaveState();
				void				LoadState();
				void				TakeScreenshot();
#ifdef DAEDALUS_DIALOGS
				void				ExitConfirmation();
#endif
				void				OnSaveStateSlotSelected( const std::filesystem::path& filename );
				void				OnLoadStateSlotSelected( const std::filesystem::path& filename );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
				void				DebugDisplayList();
#endif
#ifdef DAEDALUS_KERNEL_MODE
				void				ProfileNextFrame();
#endif

	private:
		std::function<void()> 					mOnResume;
		std::function<void()>					mOnReset;

		CUIElementBag				mElements;
};


CPauseOptionsComponent::CPauseOptionsComponent( CUIContext * p_context )
:	CUIComponent( p_context )
{}


CPauseOptionsComponent::~CPauseOptionsComponent() {}


CPauseOptionsComponent *	CPauseOptionsComponent::Create( CUIContext * p_context,  std::function<void()> on_resume, std::function<void()> on_reset )
{
	return new IPauseOptionsComponent( p_context, on_resume, on_reset );
}


IPauseOptionsComponent::IPauseOptionsComponent( CUIContext * p_context,  std::function<void()> on_resume, std::function<void()> on_reset)
:	CPauseOptionsComponent( p_context )
,	mOnResume( on_resume )
,	mOnReset( on_reset )
{
	mElements.Add(std::make_unique<CUISpacer>( 10 ) );
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&IPauseOptionsComponent::EditPreferences, this), "Edit Preferences", "Edit various preferences for this rom."));
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&IPauseOptionsComponent::AdvancedOptions, this ), "Advanced Options", "Edit advanced options for this rom." ) );
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&IPauseOptionsComponent::CheatOptions, this ), "Cheats", "Edit advanced options for this rom." ) );
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&IPauseOptionsComponent::SaveState, this ), "Save State", "Save the current state." ) );
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&IPauseOptionsComponent::LoadState, this ), "Load/Delete State", "Restore or delete a previously saved state." ) );
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&IPauseOptionsComponent::TakeScreenshot,this ), "Take Screenshot", "Take a screenshot on resume." ) );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		mElements.Add( new CUICommandImpl( std::bind(&IPauseOptionsComponent::DebugDisplayList, this ), "Debug Display List", "Debug display list on resume." ) );
#endif

#ifdef DAEDALUS_DEBUG_CONSOLE
	//mElements.Add( new CUICommandImpl(std::bind(&IPauseOptionsComponent::CPU_DumpFragmentCache, this ), "Dump Fragment Cache", "Dump the contents of the dynarec fragment cache to disk." ) );
	//	mElements.Add( new CUICommandImpl(std::bind(&IPauseOptionsComponent::CPU_ResetFragmentCache, this ), "Clear Fragment Cache", "Clear the contents of the dynarec fragment cache." ) );
	//	mElements.Add( new CUICommandImpl(std::bind(&IPauseOptionsComponent::IPauseOptionsComponent::ProfileNextFrame, this ), "Profile Frame", "Profile the next frame on resume." ) );
#endif

	mElements.Add(std::make_unique<CUISpacer>( 16 ) );
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&IPauseOptionsComponent::OnResume, this ), "Resume Emulation", "Resume emulation." ) );

#ifdef DAEDALUS_DIALOGS
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&IPauseOptionsComponent::ExitConfirmation, this ), "Return to Main Menu", "Return to the main menu." ) );
#else
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&IPauseOptionsComponent::OnReset, this ), "Return to Main Menu", "Return to the main menu." ) );
#endif
}

IPauseOptionsComponent::~IPauseOptionsComponent() {}


void	IPauseOptionsComponent::Update( float elapsed_time [[maybe_unused]], const v2 & stick [[maybe_unused]], u32 old_buttons, u32 new_buttons )
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

		auto	element = mElements.GetSelectedElement();
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


void	IPauseOptionsComponent::Render()
{

	mElements.Draw( mpContext, LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_CENTRE, BELOW_MENU_MIN );

	auto element = mElements.GetSelectedElement();
	if( element != nullptr )
	{
		const auto	p_description = element->GetDescription();

		mpContext->DrawTextArea( DESCRIPTION_AREA_LEFT,
								 DESCRIPTION_AREA_TOP,
								 DESCRIPTION_AREA_RIGHT - DESCRIPTION_AREA_LEFT,
								 DESCRIPTION_AREA_BOTTOM - DESCRIPTION_AREA_TOP,
								 p_description,
								 DrawTextUtilities::TextWhite,
								 VA_BOTTOM );
	}
}
#ifdef DAEDALUS_DIALOGS

void IPauseOptionsComponent::ExitConfirmation()
{
	if(gShowDialog->Render( mpContext,"Return to main menu?", false) )
	{
		(mOnReset)();
	}
}
#endif

void IPauseOptionsComponent::OnResume() { (mOnResume)(); }


void IPauseOptionsComponent::OnReset()
{
	(mOnReset)();
}


void	IPauseOptionsComponent::EditPreferences()
{
	auto	edit_preferences = CRomPreferencesScreen::Create( mpContext, g_ROM.mRomID );
	edit_preferences->Run();
}


void	IPauseOptionsComponent::AdvancedOptions()
{
	auto advanced_options = CAdvancedOptionsScreen::Create( mpContext, g_ROM.mRomID );
	advanced_options->Run();
}

void	IPauseOptionsComponent::CheatOptions()
{
	auto cheat_options = CCheatOptionsScreen::Create( mpContext, g_ROM.mRomID );
	cheat_options->Run();
}


void	IPauseOptionsComponent::SaveState()
{
auto onSaveStateSlotSelected = [this](const std::filesystem::path slot) { this->OnSaveStateSlotSelected(slot);
};

auto component = CSavestateSelectorComponent::Create(mpContext, CSavestateSelectorComponent::AT_SAVING, onSaveStateSlotSelected, g_ROM.settings.GameName.c_str());

	auto screen( CUIComponentScreen::Create( mpContext, component, SAVING_TITLE_TEXT ) );
	screen->Run();
	(mOnResume)();
}


void	IPauseOptionsComponent::LoadState()
{
auto onLoadStateSlotSelected = [this](const std::filesystem::path slot) {
    this->OnLoadStateSlotSelected(slot);
};

auto component = CSavestateSelectorComponent::Create(mpContext, CSavestateSelectorComponent::AT_LOADING, onLoadStateSlotSelected, g_ROM.settings.GameName.c_str());

	auto screen =  CUIComponentScreen::Create( mpContext, component, LOADING_TITLE_TEXT );
	screen->Run();
	(mOnResume)();
}


void	IPauseOptionsComponent::OnSaveStateSlotSelected( const std::filesystem::path& filename )
{
	std::filesystem::remove( filename ); // Ensure that we're re-creating the file
	CPU_RequestSaveState( filename );

	gTakeScreenshot = true;
	gTakeScreenshotSS = true;
}


void	IPauseOptionsComponent::OnLoadStateSlotSelected( const std::filesystem::path& filename )
{
	CPU_RequestLoadState( filename );
}


void IPauseOptionsComponent::TakeScreenshot()
{
	gTakeScreenshot = true;
	(mOnResume)();
}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST

void IPauseOptionsComponent::DebugDisplayList()
{
	DLDebugger_RequestDebug();
	(mOnResume)();
}
#endif
