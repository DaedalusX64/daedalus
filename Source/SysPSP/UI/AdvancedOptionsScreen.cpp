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
#include "AdvancedOptionsScreen.h"

#include "UIContext.h"
#include "UIScreen.h"
#include "UISetting.h"
#include "UISpacer.h"
#include "UICommand.h"

#include "Core/ROM.h"
#include "Core/RomSettings.h"

#include "Utility/Preferences.h"

#include "Input/InputManager.h"

#include "Graphics/ColourValue.h"
#include "SysPSP/Graphics/DrawText.h"

#include "ConfigOptions.h"

#include <pspctrl.h>

namespace
{
	const u32		TITLE_AREA_TOP = 10;

	const u32		TEXT_AREA_LEFT = 40;
	const u32		TEXT_AREA_RIGHT = 440;

	const s32		DESCRIPTION_AREA_TOP = 0;		// We render text aligned from the bottom, so this is largely irrelevant
	const s32		DESCRIPTION_AREA_BOTTOM = 272-6;
	const s32		DESCRIPTION_AREA_LEFT = 16;
	const s32		DESCRIPTION_AREA_RIGHT = 480-16;


}

//*************************************************************************************
//
//*************************************************************************************
class CScrnSetting : public CUISetting
	{
	public:
		CScrnSetting( u32 * setting, const char * name, const char * description )
			:	CUISetting( name, description )
			,	mSetting( setting )
		{
		}

		virtual	void			OnNext()				{ (*mSetting < 1) ? *mSetting += 1 : *mSetting = 1; }
		virtual	void			OnPrevious()			{ (*mSetting > 0) ? *mSetting -= 1 : *mSetting = 0; }

		virtual const char *	GetSettingName() const
		{
			switch ( *mSetting )
			{
				case 0:		return "Dlist start";
				case 1:		return "Dlist end";
			}
			return "?";
		}

	private:
		u32		*mSetting;
	};

//*************************************************************************************
//
//*************************************************************************************
class IAdvancedOptionsScreen : public CAdvancedOptionsScreen, public CUIScreen
{
	public:

		IAdvancedOptionsScreen( CUIContext * p_context, const RomID & rom_id );
		~IAdvancedOptionsScreen();

		// CAdvancedOptionsScreen
		virtual void				Run();

		// CUIScreen
		virtual void				Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();
		virtual bool				IsFinished() const									{ return mIsFinished; }

	private:
				void				OnConfirm();
				void				OnCancel();

	private:
		RomID						mRomID;
		std::string					mRomName;
		SRomPreferences				mRomPreferences;

		bool						mIsFinished;

		CUIElementBag				mElements;
};

//*************************************************************************************
//
//*************************************************************************************
CAdvancedOptionsScreen::~CAdvancedOptionsScreen()
{
}

//*************************************************************************************
//
//*************************************************************************************
CAdvancedOptionsScreen *	CAdvancedOptionsScreen::Create( CUIContext * p_context, const RomID & rom_id )
{
	return new IAdvancedOptionsScreen( p_context, rom_id );
}

//*************************************************************************************
//
//*************************************************************************************
IAdvancedOptionsScreen::IAdvancedOptionsScreen( CUIContext * p_context, const RomID & rom_id )
:	CUIScreen( p_context )
,	mRomID( rom_id )
,	mRomName( "?" )
,	mIsFinished( false )
{
	CPreferences::Get()->GetRomPreferences( mRomID, &mRomPreferences );

	RomSettings			settings;
	if ( CRomSettingsDB::Get()->GetSettings( rom_id, &settings ) )
	{
 		mRomName = settings.GameName;
	}

	mElements.Add( new CBoolSetting( &mRomPreferences.MemoryAccessOptimisation, "Memory Access Optimisation", "Enable for speed-up (WARNING, can cause instability and/or crash on certain ROMs).", "Enabled", "Disabled" ) );
	mElements.Add( new CBoolSetting( &mRomPreferences.CleanSceneEnabled, "Clean Scene", "Force clear of frame buffer before drawing any primitives", "Enabled", "Disabled" ) );
	mElements.Add( new CBoolSetting( &mRomPreferences.DynarecLoopOptimisation, "Dynamic Loop Optimisation", "Enable for speed-up (WARNING, can cause instability and/or crash on certain ROMs).", "Enabled", "Disabled" ) );
	mElements.Add( new CBoolSetting( &mRomPreferences.DoubleDisplayEnabled, "Double Display Lists", "Double Display Lists enabled for a speed-up (Works on most ROMs)", "Enabled", "Disabled" ) );
	mElements.Add( new CBoolSetting( &mRomPreferences.AudioRateMatch, "Audio Rate Match", "Whether to match audio rate to the frame rate (works between 50-100% sync)", "Yes", "No" ) );
	mElements.Add( new CBoolSetting( &mRomPreferences.FogEnabled, "Fog Emulation", "Whether to enable or disable fog emulation (Experimental, only works on a few titles)", "Enabled", "Disabled" ) );

	//	mElements.Add( new CUISpacer( 16 ) );
#ifndef DAEDALUS_PUBLIC_RELEASE	// Below option is irrelevant to end user or game especific, we force this option on roms.ini
	mElements.Add( new CBoolSetting( &mRomPreferences.SimulateDoubleDisabled, "Disable Simulate Double", "Whether Simulate Double is Disabled", "Yes", "No" ) );
#endif

	mElements.Add( new CUICommandImpl( new CMemberFunctor< IAdvancedOptionsScreen >( this, &IAdvancedOptionsScreen::OnConfirm ), "Save & Return", "Confirm changes to settings and return." ) );
	mElements.Add( new CUICommandImpl( new CMemberFunctor< IAdvancedOptionsScreen >( this, &IAdvancedOptionsScreen::OnCancel ), "Cancel", "Cancel changes to settings and return." ) );

}

//*************************************************************************************
//
//*************************************************************************************
IAdvancedOptionsScreen::~IAdvancedOptionsScreen()
{
}

//*************************************************************************************
//
//*************************************************************************************
void	IAdvancedOptionsScreen::Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons )
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
			if( new_buttons & (PSP_CTRL_CROSS/*|PSP_CTRL_START*/) )
			{
				element->OnSelected();
			}
		}
	}
}

//*************************************************************************************
//
//*************************************************************************************
void	IAdvancedOptionsScreen::Render()
{
	mpContext->ClearBackground();

	u32		font_height( mpContext->GetFontHeight() );
	u32		line_height( font_height + 2 );
	s32		y;

	const char * const title_text = "Advanced Options";
	mpContext->SetFontStyle( CUIContext::FS_HEADING );
	u32		heading_height( mpContext->GetFontHeight() );
	y = TITLE_AREA_TOP + heading_height;
	mpContext->DrawTextAlign( TEXT_AREA_LEFT, TEXT_AREA_RIGHT, AT_CENTRE, y, title_text, mpContext->GetDefaultTextColour() ); y += heading_height;
	mpContext->SetFontStyle( CUIContext::FS_REGULAR );

	y += 2;

	mpContext->DrawTextAlign( TEXT_AREA_LEFT, TEXT_AREA_RIGHT, AT_CENTRE, y, mRomName.c_str(), mpContext->GetDefaultTextColour() ); y += line_height;

	y += 4;

	mElements.Draw( mpContext, TEXT_AREA_LEFT, TEXT_AREA_RIGHT, AT_CENTRE, y );

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
void	IAdvancedOptionsScreen::Run()
{
	CUIScreen::Run();
}


//*************************************************************************************
//
//*************************************************************************************
void	IAdvancedOptionsScreen::OnConfirm()
{
	CPreferences::Get()->SetRomPreferences( mRomID, mRomPreferences );

	CPreferences::Get()->Commit();

	mRomPreferences.Apply();

	mIsFinished = true;
}

//*************************************************************************************
//
//*************************************************************************************
void	IAdvancedOptionsScreen::OnCancel()
{
	mIsFinished = true;
}
