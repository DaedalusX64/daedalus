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


}
extern bool current_0;
extern bool current_1;
extern bool current_2;

extern u32 current_cheat;
u32 current_cheat1=0;
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
	class CCheatType0 : public CUISetting
	{
	public:
		CCheatType0(  const char * name, bool * cheat_enabled, const char * description )
			:	CUISetting( name, description )
			//,	mSetting( codegrouplist[0].active )
			,	mCheatEnabled( cheat_enabled )
		{
		}
		// Disable this if cheatcodes aren't enabled
		//
		virtual bool			IsReadOnly() const		{ return !(*mCheatEnabled); }

		virtual	void			OnNext()				{ if( !IsReadOnly() ) codegrouplist[0].active = !codegrouplist[0].active; }
		virtual	void			OnPrevious()			{ if( !IsReadOnly() ) codegrouplist[0].active = !codegrouplist[0].active; }

		virtual	void			OnSelected()
		{

			if(codegrouplist[0].number==0)
			{
				if(!codegrouplist[0].active)
				{
					//printf("Enable\n");
					codegrouplist[0].active = true;
				}
				else
				{
					//printf("Disable\n");
					codegrouplist[0].active = false;

				}
			}
	
		}
		virtual const char *	GetSettingName() const
		{
			return codegrouplist[0].active ? "Enabled" : "Disabled";
		}

	private:
		bool *					mCheatEnabled;
	};
//*************************************************************************************
//
//*************************************************************************************
	class CCheatType1 : public CUISetting
	{
	public:
		CCheatType1(  const char * name, const char * description )
			:	CUISetting( name, description )
		{
		}
		virtual	void			OnSelected()
		{

			if(!codegrouplist[1].active)
			{
				printf("Enable\n");
				codegrouplist[1].active = true;
			}
			else
			{
				printf("Disable\n");
				codegrouplist[1].active = false;

			}
	
		}
		virtual const char *	GetSettingName() const
		{
			return codegrouplist[1].active ? "Enabled" : "Disabled";
		}

	};
//*************************************************************************************
//
//*************************************************************************************
	class CCheatType2 : public CUISetting
	{
	public:
		CCheatType2(  const char * name, const char * description )
			:	CUISetting( name, description )
		{
		}
		virtual	void			OnSelected()
		{
			if(codegrouplist[2].number==2)
			{
				if(!codegrouplist[2].active)
				{
					codegrouplist[2].active = true;
				}
				else
				{
					codegrouplist[2].active = false;
				}
			}

		}
		virtual const char *	GetSettingName() const
		{
			return codegrouplist[2].active ? "Enabled" : "Disabled";
		}
	};

//*************************************************************************************
//
//*************************************************************************************
class CCheatNotFound : public CUISetting
	{
	public:
		CCheatNotFound(  const char * name, bool * cheat_enabled, const char * description )
			:	CUISetting( name, description )
			,	mCheatEnabled( cheat_enabled )
		{
		}
		// Disable this if cheatcodes aren't enabled
		//
		virtual bool			IsReadOnly() const		{ return true; }

		//virtual	void			OnSelected(){}

		virtual const char *	GetSettingName() const
		{
			return "N/A";
		}

	private:
		bool *					mSetting;
		bool *					mCheatEnabled;
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

	mElements.Add( new CBoolSetting( &mRomPreferences.CheatsEnabled, "Cheat Codes", "Enable cheat Codes", "Enabled", "Disabled" ) );

	// Make sure to only display the cheat list when the cheatfile been loaded correctly and there were cheats found
	//
	if(codegroupcount > 0)
	{
		// Hacky and barely working way to generate list of cheats
		// We must slim down this :(
		// I only display 1 entry since generating the list barely working, but we support a max of 6 entries per group :)
		//
		mElements.Add( new CCheatType0(  codegrouplist[0].name, &mRomPreferences.CheatsEnabled, codegrouplist[0].note ) );
		//mElements.Add( new CCheatType1(  codegrouplist[1].name, codegrouplist[1].note ) );
		//mElements.Add( new CCheatType2(  codegrouplist[2].name, codegrouplist[2].note ) );
	}
	else
	{	
		// Display Msg to user if he opens the cheat list without loading the cheatfile or no cheats found
		//
		for(u32 i = 0; i < 6; i++)
		{
			//codegrouplist[i].active = false;	// Most likley overkill, but meh just to be sure
			mElements.Add( new CCheatNotFound("No cheatcodes found for this ROM", &mRomPreferences.CheatsEnabled, "Make sure codes are formatted correctly, and ROM was started with Cheat Codes option enabled. Note : Cheats won't be displayed until you start the ROM first." ) );

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

	u32		font_height( mpContext->GetFontHeight() );
	u32		line_height( font_height + 2 );
	s32		y;

	const char * const title_text = "Cheat Options";
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
