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
#include "PauseScreen.h"

#include "UIContext.h"
#include "UIScreen.h"
#include "AboutComponent.h"
#include "GlobalSettingsComponent.h"
#include "PauseOptionsComponent.h"

#include "SysPSP/Graphics/DrawText.h"
#include "Graphics/ColourValue.h"
#include "Graphics/GraphicsContext.h"

#include "Core/ROM.h"
#include "Core/CPU.h"
#include "Core/RomSettings.h"

#include "Math/MathUtil.h"
#include "Utility/Functor.h"
#include "Utility/Translate.h"

#include "SysPSP/Utility/Buttons.h"

#include <psprtc.h>
#include <psppower.h>
#include <pspctrl.h>
#include <pspgu.h>

#include <string>

extern void battery_info();

namespace
{
	const u32				TEXT_AREA_TOP = 10;
	const u32				TEXT_AREA_LEFT = 20;
	const u32				TEXT_AREA_RIGHT = 480-20;

	enum EMenuOption
	{
		MO_GLOBAL_SETTINGS = 0,
		MO_PAUSE_OPTIONS,
		MO_ABOUT,
		NUM_MENU_OPTIONS,
	};

	const EMenuOption	MO_FIRST_OPTION = MO_GLOBAL_SETTINGS;
	const EMenuOption	MO_LAST_OPTION = MO_ABOUT;

	const char * const	gMenuOptionNames[ NUM_MENU_OPTIONS ] =
	{
		"Global Settings",
		"Paused",
		"About",
	};
}

//*************************************************************************************
//
//*************************************************************************************
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

//*************************************************************************************
//
//*************************************************************************************
CPauseScreen::~CPauseScreen()
{
}

//*************************************************************************************
//
//*************************************************************************************
CPauseScreen *	CPauseScreen::Create( CUIContext * p_context )
{
	return new IPauseScreen( p_context );
}

//*************************************************************************************
//
//*************************************************************************************
IPauseScreen::IPauseScreen( CUIContext * p_context )
:	CUIScreen( p_context )
,	mIsFinished( false )
,	mCurrentOption( MO_PAUSE_OPTIONS )
{
	for( u32 i = 0; i < NUM_MENU_OPTIONS; ++i )
	{
		mOptionComponents[ i ] = NULL;
	}

	mOptionComponents[ MO_GLOBAL_SETTINGS ]	= CGlobalSettingsComponent::Create( mpContext );
	mOptionComponents[ MO_PAUSE_OPTIONS ]	= CPauseOptionsComponent::Create( mpContext, new CMemberFunctor< IPauseScreen >( this, &IPauseScreen::OnResume ), new CMemberFunctor< IPauseScreen >( this, &IPauseScreen::OnReset ) );
	mOptionComponents[ MO_ABOUT ]			= CAboutComponent::Create( mpContext );

	for( u32 i = 0; i < NUM_MENU_OPTIONS; ++i )
	{
		DAEDALUS_ASSERT( mOptionComponents[ i ] != NULL, "Unhandled screen" );
	}
}

//*************************************************************************************
//
//*************************************************************************************
IPauseScreen::~IPauseScreen()
{
	for( u32 i = 0; i < NUM_MENU_OPTIONS; ++i )
	{
		delete mOptionComponents[ i ];
	}
}


//*************************************************************************************
//
//*************************************************************************************
EMenuOption		IPauseScreen::GetPreviousValidOption() const
{
	bool			looped( false );
	EMenuOption		current_option( mCurrentOption );

	do
	{
		current_option = GetPreviousOption( current_option );
		looped = current_option == mCurrentOption;
	}
	while( !IsOptionValid( current_option ) && !looped );

	return current_option;
}

//*************************************************************************************
//
//*************************************************************************************
EMenuOption		IPauseScreen::GetNextValidOption() const
{
	bool			looped( false );
	EMenuOption		current_option( mCurrentOption );

	do
	{
		current_option = GetNextOption( current_option );
		looped = current_option == mCurrentOption;
	}
	while( !IsOptionValid( current_option ) && !looped );

	return current_option;
}

//*************************************************************************************
//
//*************************************************************************************
bool	IPauseScreen::IsOptionValid( EMenuOption option ) const
{
	return true;
}

//*************************************************************************************
//
//*************************************************************************************
void	IPauseScreen::Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons )
{
	static bool button_released(false);

	if(!(new_buttons & PSP_CTRL_HOME) && button_released)
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
		if(new_buttons & PSP_CTRL_HOME)
		{
			button_released = true;
			new_buttons &= ~PSP_CTRL_HOME;
		}
	}

	mOptionComponents[ mCurrentOption ]->Update( elapsed_time, stick, old_buttons, new_buttons );
}

//*************************************************************************************
//
//*************************************************************************************
void	IPauseScreen::Render()
{
	mpContext->ClearBackground();

	s32			y( TEXT_AREA_TOP );

	const char * p_option_text;

	c32		valid_colour( mpContext->GetDefaultTextColour() );
	c32		invalid_colour( 200, 200, 200 );

	EMenuOption		previous( GetPreviousOption() );
	EMenuOption		current( GetCurrentOption() );
	EMenuOption		next( GetNextOption() );

	/* Time & Battery info*/
	pspTime s;
	sceRtcGetCurrentClockLocalTime(&s);
	s32 bat = scePowerGetBatteryLifePercent();
	s32 batteryLifeTime = scePowerGetBatteryLifeTime();

	// Meh should be big enough regarding if translated..
	char					info[120];

	if(!scePowerIsBatteryCharging())
	{
		sprintf(info,"[%s %d:%02d%c%02d]  [%s %d%% %0.2fV %dC]  [%s %2dh %2dm]",
			Translate_String("Time"), s.hour, s.minutes, (' '/*s.seconds&1?':':' '*/), s.seconds,
			Translate_String("Battery"), bat, (f32) scePowerGetBatteryVolt() / 1000.0f, scePowerGetBatteryTemp(),
			Translate_String("Remaining"), batteryLifeTime / 60, batteryLifeTime - 60 * (batteryLifeTime / 60));
	}
	else
	{
		sprintf(info, "[%s %d:%02d%c%02d]  [%s]  [%s]",
			Translate_String("Time"), s.hour, s.minutes, (' '/*s.seconds&1?':':' '*/), s.seconds,
			Translate_String("Charging..."),
			Translate_String("Remaining: --h--m") );
	}

	mpContext->DrawTextAlign( 0, 480, AT_CENTRE, 43, info, DrawTextUtilities::TextWhiteDisabled, DrawTextUtilities::TextBlueDisabled );

	p_option_text = gMenuOptionNames[ previous ];
	mpContext->DrawTextAlign( TEXT_AREA_LEFT, TEXT_AREA_RIGHT, AT_LEFT, y + mpContext->GetFontHeight(), p_option_text, IsOptionValid( previous ) ? valid_colour : invalid_colour );

	mpContext->SetFontStyle( CUIContext::FS_HEADING );
	p_option_text = gMenuOptionNames[ current ];
	mpContext->DrawTextAlign( TEXT_AREA_LEFT, TEXT_AREA_RIGHT, AT_CENTRE, y + mpContext->GetFontHeight(), p_option_text, IsOptionValid( current ) ? valid_colour : invalid_colour );
	mpContext->SetFontStyle( CUIContext::FS_REGULAR );

	p_option_text = gMenuOptionNames[ next ];
	mpContext->DrawTextAlign( TEXT_AREA_LEFT, TEXT_AREA_RIGHT, AT_RIGHT, y + mpContext->GetFontHeight(), p_option_text, IsOptionValid( next ) ? valid_colour : invalid_colour );

	mOptionComponents[ mCurrentOption ]->Render();
}

//*************************************************************************************
//
//*************************************************************************************
void	IPauseScreen::Run()
{
	mIsFinished = false;
	CUIScreen::Run();

	// switch back to the emulator display
	CGraphicsContext::Get()->SwitchToChosenDisplay();

	// Clear everything to black - looks a bit tidier
	CGraphicsContext::Get()->ClearAllSurfaces();
}

//*************************************************************************************
//
//*************************************************************************************
void IPauseScreen::OnResume()
{
	mIsFinished = true;
}

//*************************************************************************************
//
//*************************************************************************************
void IPauseScreen::OnReset()
{
	CPU_Halt("Resetting");
	mIsFinished = true;
}
