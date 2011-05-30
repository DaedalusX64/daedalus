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
#include "PauseOptionsComponent.h"

#include "RomPreferencesScreen.h"
#include "AdvancedOptionsScreen.h"
#include "SavestateSelectorComponent.h"
#include "CheatOptionsScreen.h"
#include "Dialogs.h"

#include "UIContext.h"
#include "UIScreen.h"
#include "UISetting.h"
#include "UICommand.h"
#include "UISpacer.h"

#include "Graphics/ColourValue.h"
#include "Graphics/GraphicsContext.h"
#include "SysPSP/Graphics/DrawText.h"

#include "Core/CPU.h"
#include "Core/Dynamo.h"
#include "Core/ROM.h"
#include "Core/SaveState.h"

#include "Utility/Functor.h"
#include "Utility/IO.h"

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

	const char * const		SAVING_TITLE_TEXT  = "Select a Slot to Save To";
	const char * const		LOADING_TITLE_TEXT = "Select a Slot to Load From";
}

extern bool gTakeScreenshot;
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
extern bool gDebugDisplayList;
#endif

//*************************************************************************************
//
//*************************************************************************************
class IPauseOptionsComponent : public CPauseOptionsComponent
{
	public:

		IPauseOptionsComponent( CUIContext * p_context, CFunctor * on_resume, CFunctor * on_reset );
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
				void				OnSaveStateSlotSelected( const char * filename );
				void				OnLoadStateSlotSelected( const char * filename );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
				void				DebugDisplayList();
#endif
#ifdef DAEDALUS_KERNEL_MODE
				void				ProfileNextFrame();
#endif

	private:
		CFunctor *					mOnResume;
		CFunctor *					mOnReset;

		CUIElementBag				mElements;
};

//*************************************************************************************
//
//*************************************************************************************
CPauseOptionsComponent::CPauseOptionsComponent( CUIContext * p_context )
:	CUIComponent( p_context )
{
}

//*************************************************************************************
//
//*************************************************************************************
CPauseOptionsComponent::~CPauseOptionsComponent()
{
}

//*************************************************************************************
//
//*************************************************************************************
CPauseOptionsComponent *	CPauseOptionsComponent::Create( CUIContext * p_context, CFunctor * on_resume, CFunctor * on_reset )
{
	return new IPauseOptionsComponent( p_context, on_resume, on_reset );
}

//*************************************************************************************
//
//*************************************************************************************
IPauseOptionsComponent::IPauseOptionsComponent( CUIContext * p_context, CFunctor * on_resume, CFunctor * on_reset )
:	CPauseOptionsComponent( p_context )
,	mOnResume( on_resume )
,	mOnReset( on_reset )
{
	mElements.Add( new CUICommandImpl( new CMemberFunctor< IPauseOptionsComponent >( this, &IPauseOptionsComponent::EditPreferences ), "Edit Preferences", "Edit various preferences for this rom." ) );
	mElements.Add( new CUICommandImpl( new CMemberFunctor< IPauseOptionsComponent >( this, &IPauseOptionsComponent::AdvancedOptions ), "Advanced Options", "Edit advanced options for this rom." ) );
	mElements.Add( new CUICommandImpl( new CMemberFunctor< IPauseOptionsComponent >( this, &IPauseOptionsComponent::CheatOptions ), "Cheats", "Edit advanced options for this rom." ) );
	mElements.Add( new CUICommandImpl( new CMemberFunctor< IPauseOptionsComponent >( this, &IPauseOptionsComponent::SaveState ), "Save State", "Save the current state." ) );
	mElements.Add( new CUICommandImpl( new CMemberFunctor< IPauseOptionsComponent >( this, &IPauseOptionsComponent::LoadState ), "Load/Delete State", "Restore or delete a previously saved state." ) );

	mElements.Add( new CUICommandImpl( new CMemberFunctor< IPauseOptionsComponent >( this, &IPauseOptionsComponent::TakeScreenshot ), "Take Screenshot", "Take a screenshot on resume." ) );



#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		mElements.Add( new CUICommandImpl( new CMemberFunctor< IPauseOptionsComponent >( this, &IPauseOptionsComponent::DebugDisplayList ), "Debug Display List", "Debug display list on resume." ) );
#endif
#ifdef DAEDALUS_KERNEL_MODE
		mElements.Add( new CUICommandImpl( new CMemberFunctor< IPauseOptionsComponent >( this, &IPauseOptionsComponent::ProfileNextFrame ), "Profile Frame", "Profile the next frame on resume." ) );
#endif


#ifndef DAEDALUS_PUBLIC_RELEASE
		// Other options (can be performed immediately...)
	#ifdef DAEDALUS_ENABLE_DYNAREC
		mElements.Add( new CUICommandImpl( new CStaticFunctor( CPU_ResetFragmentCache ), "Clear Fragment Cache", "Clear the contents of the dynarec fragment cache." ) );
	#endif			
#endif

#ifdef DAEDALUS_ENABLE_DYNAREC
	#ifdef DAEDALUS_DEBUG_DYNAREC
		mElements.Add( new CUICommandImpl( new CStaticFunctor( CPU_DumpFragmentCache ), "Dump Fragment Cache", "Dump the contents of the dynarec fragment cache to disk." ) );
	#endif
#endif

	mElements.Add( new CUISpacer( 16 ) );

	mElements.Add( new CUICommandImpl( new CMemberFunctor< IPauseOptionsComponent >( this, &IPauseOptionsComponent::OnResume ), "Resume Emulation", "Resume emulation." ) );

#ifdef DAEDALUS_DIALOGS
	mElements.Add( new CUICommandImpl( new CMemberFunctor< IPauseOptionsComponent >( this, &IPauseOptionsComponent::ExitConfirmation ), "Return to Main Menu", "Return to the main menu." ) );
#else
	mElements.Add( new CUICommandImpl( new CMemberFunctor< IPauseOptionsComponent >( this, &IPauseOptionsComponent::OnReset ), "Return to Main Menu", "Return to the main menu." ) );
#endif
}

//*************************************************************************************
//
//*************************************************************************************
IPauseOptionsComponent::~IPauseOptionsComponent()
{
}

//*************************************************************************************
//
//*************************************************************************************
void	IPauseOptionsComponent::Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons )
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
void	IPauseOptionsComponent::Render()
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
#ifdef DAEDALUS_DIALOGS
//*************************************************************************************
//
//*************************************************************************************
void IPauseOptionsComponent::ExitConfirmation()
{
	if(gShowDialog.Render( mpContext,"Return to main menu?", false) )
	{
		(*mOnReset)();
	}
}
#endif
//*************************************************************************************
//
//*************************************************************************************
void IPauseOptionsComponent::OnResume()
{
	(*mOnResume)();
}

//*************************************************************************************
//
//*************************************************************************************
void IPauseOptionsComponent::OnReset()
{
	(*mOnReset)();
}

//*************************************************************************************
//
//*************************************************************************************
void	IPauseOptionsComponent::EditPreferences()
{
	CRomPreferencesScreen *	edit_preferences( CRomPreferencesScreen::Create( mpContext, g_ROM.mRomID ) );
	edit_preferences->Run();
	delete edit_preferences;
}

//*************************************************************************************
//
//*************************************************************************************
void	IPauseOptionsComponent::AdvancedOptions()
{
	CAdvancedOptionsScreen *	advanced_options( CAdvancedOptionsScreen::Create( mpContext, g_ROM.mRomID ) );
	advanced_options->Run();
	delete advanced_options;
}

//*************************************************************************************
//
//*************************************************************************************
void	IPauseOptionsComponent::CheatOptions()
{
	CCheatOptionsScreen *	cheat_options( CCheatOptionsScreen::Create( mpContext, g_ROM.mRomID ) );
	cheat_options->Run();
	delete cheat_options;
}
//*************************************************************************************
//
//*************************************************************************************
void	IPauseOptionsComponent::SaveState()
{
	CSavestateSelectorComponent *	component( CSavestateSelectorComponent::Create( mpContext, CSavestateSelectorComponent::AT_SAVING, new CMemberFunctor1< IPauseOptionsComponent, const char * >( this, &IPauseOptionsComponent::OnSaveStateSlotSelected ), g_ROM.settings.GameName.c_str() ) );

	CUIComponentScreen *			screen( CUIComponentScreen::Create( mpContext, component, SAVING_TITLE_TEXT ) );
	screen->Run();
	delete screen;
	(*mOnResume)();
}

//*************************************************************************************
//
//*************************************************************************************
void	IPauseOptionsComponent::LoadState()
{
	CSavestateSelectorComponent *	component( CSavestateSelectorComponent::Create( mpContext, CSavestateSelectorComponent::AT_LOADING, new CMemberFunctor1< IPauseOptionsComponent, const char * >( this, &IPauseOptionsComponent::OnLoadStateSlotSelected ), g_ROM.settings.GameName.c_str() ) );
	CUIComponentScreen *			screen( CUIComponentScreen::Create( mpContext, component, LOADING_TITLE_TEXT ) );
	screen->Run();
	delete screen;
	(*mOnResume)();
}

//*************************************************************************************
//
//*************************************************************************************
void	IPauseOptionsComponent::OnSaveStateSlotSelected( const char * filename )
{
	IO::File::Delete( filename ); // Ensure that we're re-creating the file
	CPU_SaveState( filename );
}


//*************************************************************************************
//
//*************************************************************************************
void	IPauseOptionsComponent::OnLoadStateSlotSelected( const char * filename )
{
	CPU_LoadState( filename );
}

//*************************************************************************************
//
//*************************************************************************************
void IPauseOptionsComponent::TakeScreenshot()
{
	gTakeScreenshot = true;
	(*mOnResume)();
}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
//*************************************************************************************
//
//*************************************************************************************
void IPauseOptionsComponent::DebugDisplayList()
{
	gDebugDisplayList = true;
	(*mOnResume)();
}
#endif
