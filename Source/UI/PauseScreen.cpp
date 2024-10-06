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


#include <stdio.h>

#include <string>


#include "Core/CPU.h"
#include "Core/ROM.h"
#include "RomFile/RomSettings.h"
#include "Graphics/ColourValue.h"
#include "Graphics/GraphicsContext.h"
#include "Utility/MathUtil.h"
#include "DrawTextUtilities.h"
#include "UIContext.h"
#include "UIScreen.h"
#include "AboutComponent.h"
#include "GlobalSettingsComponent.h"
#include "PauseOptionsComponent.h"
#include "PauseScreen.h"

#include "Utility/Translate.h"
#include "Menu.h"

extern void battery_info();

namespace {
	enum EMenuOption
	{
		MO_GLOBAL_SETTINGS = 0,
		MO_PAUSE_OPTIONS,
		MO_ABOUT,
		NUM_MENU_OPTIONS,
	};

	const EMenuOption	MO_FIRST_OPTION [[maybe_unused]]= MO_GLOBAL_SETTINGS;
	const EMenuOption	MO_LAST_OPTION  [[maybe_unused]] = MO_ABOUT;

	const char * const	gMenuOptionNames[ NUM_MENU_OPTIONS ] =
	{
		"Global Settings",
		"Paused",
		"About",
	};
}

class IPauseScreen : public CPauseScreen, public CUIScreen
{
	public:

		IPauseScreen( CUIContext * p_context );
		~IPauseScreen();

		// CPauseScreen
		virtual void				Run();

		// CUIScreen
		virtual void				Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();
		virtual bool				IsFinished() const									{ return mIsFinished; }


	private:
		static	EMenuOption			GetNextOption( EMenuOption option )					{ return EMenuOption( (option + 1) % NUM_MENU_OPTIONS ); }
		static	EMenuOption			GetPreviousOption( EMenuOption option )				{ return EMenuOption( (option + NUM_MENU_OPTIONS -1) % NUM_MENU_OPTIONS ); }

				EMenuOption			GetPreviousOption() const							{ return GetPreviousOption( mCurrentOption ); }
				EMenuOption			GetCurrentOption() const							{ return mCurrentOption; }
				EMenuOption			GetNextOption() const								{ return GetNextOption( mCurrentOption ); }

				EMenuOption			GetPreviousValidOption() const;
				EMenuOption			GetNextValidOption() const;

				bool				IsOptionValid( EMenuOption option ) const;

				void				OnResume();
				void				OnReset();

	private:
		bool						mIsFinished;

		EMenuOption					mCurrentOption;

		CUIComponent *				mOptionComponents[ NUM_MENU_OPTIONS ];
};


CPauseScreen::~CPauseScreen() {}

std::unique_ptr<CPauseScreen>	CPauseScreen::Create( CUIContext * p_context )
{
	return std::make_unique<IPauseScreen>( p_context );
}


IPauseScreen::IPauseScreen( CUIContext * p_context )
:	CUIScreen( p_context )
,	mIsFinished( false )
,	mCurrentOption( MO_PAUSE_OPTIONS )
{
	for( auto i = 0; i < NUM_MENU_OPTIONS; ++i )
	{
		mOptionComponents[ i ] = NULL;
	}

	mOptionComponents[ MO_GLOBAL_SETTINGS ]	= CGlobalSettingsComponent::Create( mpContext );
	mOptionComponents[ MO_PAUSE_OPTIONS ]	= CPauseOptionsComponent::Create( mpContext, std::bind(&IPauseScreen::OnResume, this), std::bind(&IPauseScreen::OnReset, this ) );
	mOptionComponents[ MO_ABOUT ]			= CAboutComponent::Create( mpContext );

#ifdef DAEDALUS_ENABLE_ASSERTS
	for( u32 i = 0; i < NUM_MENU_OPTIONS; ++i )
	{
		DAEDALUS_ASSERT( mOptionComponents[ i ] != NULL, "Unhandled screen" );
	}
	#endif
}

IPauseScreen::~IPauseScreen()
{
	for( auto i = 0; i < NUM_MENU_OPTIONS; ++i )
	{
		delete mOptionComponents[ i ];
	}
}


EMenuOption		IPauseScreen::GetPreviousValidOption() const
{
	bool			looped = false;
	EMenuOption	 current_option = mCurrentOption;

	do
	{
		current_option = GetPreviousOption( current_option );
		looped = current_option == mCurrentOption;
	}
	while( !IsOptionValid( current_option ) && !looped );

	return current_option;
}


EMenuOption		IPauseScreen::GetNextValidOption() const
{
	bool			looped = false;
	EMenuOption	current_option = mCurrentOption;

	do
	{
		current_option = GetNextOption( current_option );
		looped = current_option == mCurrentOption;
	}
	while( !IsOptionValid( current_option ) && !looped );

	return current_option;
}


bool	IPauseScreen::IsOptionValid( EMenuOption option [[maybe_unused]] ) const
{
	return true;
}


void	IPauseScreen::Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons )
{
	static bool button_released = false;

	if(!(new_buttons & PSP_CTRL_SELECT) && button_released)
	{
		button_released = false;
		mIsFinished = true;
	}

	if(old_buttons != new_buttons)
	{
		if(new_buttons & PSP_CTRL_LTRIGGER)
		{
			mCurrentOption = GetPreviousValidOption();
			new_buttons &= ~PSP_CTRL_LTRIGGER;
		}
		if(new_buttons & PSP_CTRL_RTRIGGER)
		{
			mCurrentOption = GetNextValidOption();
			new_buttons &= ~PSP_CTRL_RTRIGGER;
		}
		if(new_buttons & PSP_CTRL_SELECT)
		{
			button_released = true;
			new_buttons &= ~PSP_CTRL_SELECT;
		}
	}

	mOptionComponents[ mCurrentOption ]->Update( elapsed_time, stick, old_buttons, new_buttons );
}


void	IPauseScreen::Render()
{
	mpContext->ClearBackground();

	auto y = MENU_TOP;

	c32		valid_colour( mpContext->GetDefaultTextColour() );
	c32		invalid_colour( 200, 200, 200 );

	EMenuOption		previous = GetPreviousOption();
	EMenuOption		current = GetCurrentOption();
	EMenuOption		next = GetNextOption();

	// Meh should be big enough regarding if translated..
	char					info[120];

#if DAEDALUS_PSP
	s32 bat = scePowerGetBatteryLifePercent();
	s32 batteryLifeTime = scePowerGetBatteryLifeTime();
	if(!scePowerIsBatteryCharging())
	{
			snprintf(info, sizeof(info), " [%s %d%% %s %2dh %2dm]",
			Translate_String("Battery / "), bat,
			Translate_String("Time"), batteryLifeTime / 60, batteryLifeTime % 60);

	}
	else
	{
			snprintf(info, sizeof(info), "[%s]" ,
			Translate_String("Battery is Charging"));
	}
#endif

	// Battery Info
	mpContext->SetFontStyle( CUIContext::FS_REGULAR );
	mpContext->DrawTextAlign( 0, SCREEN_WIDTH - LIST_TEXT_LEFT, AT_RIGHT, CATEGORY_TEXT_TOP, info, DrawTextUtilities::TextWhiteDisabled, DrawTextUtilities::TextBlueDisabled );
	
	auto p_option_text = gMenuOptionNames[ previous ];
	mpContext->DrawTextAlign( LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_LEFT, y + mpContext->GetFontHeight(), p_option_text, IsOptionValid( previous ) ? valid_colour : invalid_colour );

	mpContext->SetFontStyle( CUIContext::FS_HEADING );
	p_option_text = gMenuOptionNames[ current ];
	mpContext->DrawTextAlign( LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_CENTRE, y + mpContext->GetFontHeight(), p_option_text, IsOptionValid( current ) ? valid_colour : invalid_colour );
	mpContext->SetFontStyle( CUIContext::FS_REGULAR );

	p_option_text = gMenuOptionNames[ next ];
	mpContext->DrawTextAlign( LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_RIGHT, y + mpContext->GetFontHeight(), p_option_text, IsOptionValid( next ) ? valid_colour : invalid_colour );

	mOptionComponents[ mCurrentOption ]->Render();

}

void	IPauseScreen::Run()
{
	mIsFinished = false;
	CUIScreen::Run();

#ifdef DAEDALUS_PSP
	CGraphicsContext::Get()->SwitchToChosenDisplay();
#endif
	CGraphicsContext::Get()->ClearAllSurfaces();
}

void IPauseScreen::OnResume()
{
	mIsFinished = true;
}

void IPauseScreen::OnReset()
{
	CPU_Halt("Resetting");
	mIsFinished = true;
}
