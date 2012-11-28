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
#include "CheatOptionsScreen.h"

#include "UIContext.h"
#include "UIScreen.h"
#include "UISetting.h"
#include "UISpacer.h"
#include "UICommand.h"

#include "Core/Cheats.h"
#include "Core/ROM.h"
#include "Core/RomSettings.h"

#include "Utility/Preferences.h"

#include "Input/InputManager.h"
#include "../../Utility/Stream.h"
#include "../../Utility/IO.h"
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

	const u32				TEXT_AREA_TOP = 272 / 2;


}

//*************************************************************************************
//
//*************************************************************************************
class ICheatOptionsScreen : public CCheatOptionsScreen, public CUIScreen
{
	public:

		ICheatOptionsScreen( CUIContext * p_context, const RomID & rom_id );
		~ICheatOptionsScreen();

		// CCheatOptionsScreen
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
class CCheatType : public CUISetting
{
public:
	CCheatType( u32 i,const char * name, bool * cheat_enabled, const char * description )
		:	CUISetting( name, description )
		,	mIndex( i )
		,	mCheatEnabled( cheat_enabled )
	{
	}
	// Make read only the cheat list if enable cheat code option is disable
	virtual bool			IsReadOnly() const
	{
		// Check for any active cheat codes
		if(!(*mCheatEnabled) && codegrouplist[mIndex].enable)
		{
			// Disable 'em
			codegrouplist[mIndex].active = false;
			// Do one pass to restore their value too
			CheatCodes_Activate(IN_GAME);
		}

		return !(*mCheatEnabled);
	}

	virtual bool			IsSelectable()	const	{ return !IsReadOnly(); }

	virtual	void			OnNext()				{ if( !IsReadOnly() ) codegrouplist[mIndex].active = !codegrouplist[mIndex].active; }
	virtual	void			OnPrevious()			{ if( !IsReadOnly() ) codegrouplist[mIndex].active = !codegrouplist[mIndex].active; }

	virtual	void			OnSelected()
	{

		if(!codegrouplist[mIndex].active)
		{
			//printf("Enable %d\n",index);
			codegrouplist[mIndex].active = true;
		}
		else
		{
			//printf("Disable %d\n",index);
			codegrouplist[mIndex].active = false;

		}

	}
	virtual const char *	GetSettingName() const
	{
		return codegrouplist[mIndex].active ? "Enabled" : "Disabled";
	}

private:
	u32						mIndex;
	bool *					mCheatEnabled;
};

//*************************************************************************************
//
//*************************************************************************************
class CCheatNotFound : public CUISetting
	{
	public:
		CCheatNotFound(  const char * name )
			:	CUISetting( name, "" )
		{
		}
		// Always show as read only when no cheats are found
		//
		virtual bool			IsReadOnly()	const	{ return true; }

		virtual bool			IsSelectable()	const	{ return false; }
		virtual	void			OnSelected()			{ }

		//virtual	void			OnSelected(){}

		virtual const char *	GetSettingName() const	{ return "N/A";	}
	};

//*************************************************************************************
//
//*************************************************************************************
	class CCheatFrequency : public CUISetting
	{
	public:
		CCheatFrequency( ECheatFrequency * setting, const char * name, const char * description )
			:	CUISetting( name, description )
			,	mSetting( setting )
		{
		}

		virtual	void			OnNext()				{ *mSetting = ECheatFrequency( ( *mSetting + 1 ) % NUM_CF ); }
		virtual	void			OnPrevious()			{ *mSetting = ECheatFrequency( ( *mSetting + NUM_CF - 1 ) % NUM_CF ); }

		virtual const char *	GetSettingName() const	{ return ROM_GetCheatFrequencyDescription( *mSetting ); }

	private:
		ECheatFrequency *	mSetting;
	};

//*************************************************************************************
//
//*************************************************************************************
CCheatOptionsScreen::~CCheatOptionsScreen()
{
}

//*************************************************************************************
//
//*************************************************************************************
CCheatOptionsScreen *	CCheatOptionsScreen::Create( CUIContext * p_context, const RomID & rom_id )
{
	return new ICheatOptionsScreen( p_context, rom_id );
}

//*************************************************************************************
//
//*************************************************************************************
ICheatOptionsScreen::ICheatOptionsScreen( CUIContext * p_context, const RomID & rom_id )
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

	// Read hack code for this rom
	// We always parse the cheat file when the cheat menu is accessed, to always have cheats ready to be used by the user without hassle
	// Also we do this to make sure we clear any non-associated cheats, we only parse once per ROM access too :)
	//
	CheatCodes_Read( (char*)mRomName.c_str(), (char*)"Daedalus.cht", mRomID.CountryID );

	mElements.Add( new CBoolSetting( &mRomPreferences.CheatsEnabled, "Enable Cheat Codes", "Whether to use cheat codes for this ROM", "Yes", "No" ) );
	mElements.Add( new CCheatFrequency( &mRomPreferences.CheatFrequency, "Apply Cheat Codes Frequency", "The higher this value, the less cheats will hog the emulator at the expense that certain cheats won't work properly." ) );


	for(u32 i = 0; i < MAX_CHEATCODE_PER_LOAD; i++)
	{
		// Only display the cheat list when the cheatfile been loaded correctly and there were cheats found
		// ToDo: add a check/msg if cheatcodes were truncated, aka MAX_CHEATCODE_PER_GROUP is passed
		//
		if(codegroupcount > 0)
		{
			// Check for only available entries, if any entry isn't available, compesate it with a note to the user
			//
			if(codegroupcount > i)
			{
				// Generate list of available cheatcodes
				//
				mElements.Add( new CCheatType( i, codegrouplist[i].name, &mRomPreferences.CheatsEnabled, codegrouplist[i].note ) );

			}
			else
			{
				//mElements.Add( new CCheatNotFound("No cheat codes found for this entry", "Make sure codes are formatted correctly for this entry. Daedalus supports a max of eight cheats per game." ) );
				mElements.Add( new CCheatNotFound("No cheat codes found for this entry" ) );
			}
		}
		else
		{
			// Display Msg to user if he opens the cheat list without loading the cheatfile or no cheats found
			//
			//codegrouplist[i].active = false; // Overkill IMO
			//mElements.Add( new CCheatNotFound("No cheat codes found for this entry", "Make sure codes are formatted correctly for this entry. Daedalus supports a max of eight cheats per game." ) );
			mElements.Add( new CCheatNotFound("No cheat codes found for this entry" ) );

		}
	}


	mElements.Add( new CUICommandImpl( new CMemberFunctor< ICheatOptionsScreen >( this, &ICheatOptionsScreen::OnConfirm ), "Save & Return", "Confirm changes to settings and return." ) );
	mElements.Add( new CUICommandImpl( new CMemberFunctor< ICheatOptionsScreen >( this, &ICheatOptionsScreen::OnCancel ), "Cancel", "Cancel changes to settings and return." ) );

}

//*************************************************************************************
//
//*************************************************************************************
ICheatOptionsScreen::~ICheatOptionsScreen()
{
}

//*************************************************************************************
//
//*************************************************************************************
void	ICheatOptionsScreen::Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons )
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
void	ICheatOptionsScreen::Render()
{
	mpContext->ClearBackground();

	//u32		font_height( mpContext->GetFontHeight() );
	//u32		line_height( font_height + 2 );
	s32		y;

	const char * const title_text = "Cheat Options";
	mpContext->SetFontStyle( CUIContext::FS_HEADING );
	u32		heading_height( mpContext->GetFontHeight() );
	y = TITLE_AREA_TOP + heading_height;
	mpContext->DrawTextAlign( TEXT_AREA_LEFT, TEXT_AREA_RIGHT, AT_CENTRE, y, title_text, mpContext->GetDefaultTextColour() ); y += heading_height;
	mpContext->SetFontStyle( CUIContext::FS_REGULAR );

	y += 2;

	//mpContext->DrawTextAlign( TEXT_AREA_LEFT, TEXT_AREA_RIGHT, AT_CENTRE, y, mRomName.c_str(), mpContext->GetDefaultTextColour() ); y += line_height;

	y += 4;

	// Very basic scroller for cheats, note ROM tittle is disabled since it overlaps when scrolling - FIX ME
	//
	if( mElements.GetSelectedIndex() > 1 )
		mElements.Draw( mpContext, TEXT_AREA_LEFT, TEXT_AREA_RIGHT, AT_CENTRE, TEXT_AREA_TOP - mElements.GetSelectedIndex()*11 );
	else
		mElements.Draw( mpContext, TEXT_AREA_LEFT, TEXT_AREA_RIGHT, AT_CENTRE, TEXT_AREA_TOP - mElements.GetSelectedIndex());

	//mElements.Draw( mpContext, TEXT_AREA_LEFT, TEXT_AREA_RIGHT, AT_CENTRE, y );

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
void	ICheatOptionsScreen::Run()
{
	CUIScreen::Run();
}


//*************************************************************************************
//
//*************************************************************************************
void	ICheatOptionsScreen::OnConfirm()
{
	CPreferences::Get()->SetRomPreferences( mRomID, mRomPreferences );

	CPreferences::Get()->Commit();

	mRomPreferences.Apply();

	mIsFinished = true;
}

//*************************************************************************************
//
//*************************************************************************************
void	ICheatOptionsScreen::OnCancel()
{
	mIsFinished = true;
}
