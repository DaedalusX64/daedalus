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

#include "MainMenuScreen.h"
#include "System.h"

#include "UIContext.h"
#include "UIScreen.h"
#include "AboutComponent.h"
#include "GlobalSettingsComponent.h"
#include "RomSelectorComponent.h"
#include "SelectedRomComponent.h"
#include "SavestateSelectorComponent.h"

#include "SysPSP/Graphics/DrawText.h"
#include "Graphics/ColourValue.h"
#include "Graphics/GraphicsContext.h"

#include "Core/ROM.h"
#include "Core/CPU.h"
#include "Core/SaveState.h"
#include "Core/RomSettings.h"

#include "Utility/Preferences.h"

#include "Math/MathUtil.h"

#include <pspctrl.h>
#include <pspgu.h>

#include <string>

namespace
{
	const u32				TEXT_AREA_TOP = 8;
	const u32				SCREEN_WIDTH = 480;

	enum EMenuOption
	{
		MO_GLOBAL_SETTINGS = 0,
		MO_ROMS,
		MO_SELECTED_ROM,
		MO_SAVESTATES,
		MO_ABOUT,
	};
	const u32 NUM_MENU_OPTIONS = MO_ABOUT+1;

	const EMenuOption	MO_FIRST_OPTION = MO_GLOBAL_SETTINGS;
	const EMenuOption	MO_LAST_OPTION = MO_ABOUT;

	const char * const	gMenuOptionNames[ NUM_MENU_OPTIONS ] =
	{
		"Global Settings",
		"Roms",
		"Selected Rom",
		"Savestates",
		"About",
	};
}

//*************************************************************************************
//
//*************************************************************************************
class IMainMenuScreen : public CMainMenuScreen, public CUIScreen
{
	public:

		IMainMenuScreen( CUIContext * p_context );
		~IMainMenuScreen();

		// CMainMenuScreen
		virtual void				Run();

		// CUIScreen
		virtual void				Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();
		virtual bool				IsFinished() const									{ return mIsFinished; }


	private:
		static	EMenuOption			AsMenuOption( s32 option );

				EMenuOption			GetPreviousOption() const							{ return AsMenuOption( mCurrentOption - 1 ); }
				EMenuOption			GetCurrentOption() const							{ return AsMenuOption( mCurrentOption ); }
				EMenuOption			GetNextOption() const								{ return AsMenuOption( mCurrentOption + 1 ); }

				s32					GetPreviousValidOption() const;
				s32					GetNextValidOption() const;

				bool				IsOptionValid( EMenuOption option ) const;

				void				OnRomSelected( const char * rom_filename );
				void				OnSavestateSelected( const char * savestate_filename );
				void				OnStartEmulation();

	private:
		bool						mIsFinished;

		s32							mCurrentOption;
		f32							mCurrentDisplayOption;

		CUIComponent *				mOptionComponents[ NUM_MENU_OPTIONS ];
		CSelectedRomComponent *		mSelectedRomComponent;

		std::string					mRomFilename;
		RomID						mRomID;
};

//*************************************************************************************
//
//*************************************************************************************
CMainMenuScreen::~CMainMenuScreen()
{
}

//*************************************************************************************
//
//*************************************************************************************
CMainMenuScreen *	CMainMenuScreen::Create( CUIContext * p_context )
{
	return new IMainMenuScreen( p_context );
}

//*************************************************************************************
//
//*************************************************************************************
IMainMenuScreen::IMainMenuScreen( CUIContext * p_context )
:	CUIScreen( p_context )
,	mIsFinished( false )
,	mCurrentOption( MO_ROMS )
,	mCurrentDisplayOption( mCurrentOption )
{
	for( u32 i = 0; i < NUM_MENU_OPTIONS; ++i )
	{
		mOptionComponents[ i ] = NULL;
	}

	mSelectedRomComponent = CSelectedRomComponent::Create( mpContext, new CMemberFunctor< IMainMenuScreen >( this, &IMainMenuScreen::OnStartEmulation ) );

	mOptionComponents[ MO_GLOBAL_SETTINGS ]	= CGlobalSettingsComponent::Create( mpContext );
	mOptionComponents[ MO_ROMS ]			= CRomSelectorComponent::Create( mpContext, new CMemberFunctor1< IMainMenuScreen, const char * >( this, &IMainMenuScreen::OnRomSelected ) );
	mOptionComponents[ MO_SELECTED_ROM ]	= mSelectedRomComponent;
	mOptionComponents[ MO_SAVESTATES ]		= CSavestateSelectorComponent::Create( mpContext, CSavestateSelectorComponent::AT_LOADING, new CMemberFunctor1< IMainMenuScreen, const char * >( this, &IMainMenuScreen::OnSavestateSelected ), 0 );
	mOptionComponents[ MO_ABOUT ]			= CAboutComponent::Create( mpContext );

	for( u32 i = 0; i < NUM_MENU_OPTIONS; ++i )
	{
		DAEDALUS_ASSERT( mOptionComponents[ i ] != NULL, "Unhandled screen" );
	}
}

//*************************************************************************************
//
//*************************************************************************************
IMainMenuScreen::~IMainMenuScreen()
{
	for( u32 i = 0; i < NUM_MENU_OPTIONS; ++i )
	{
		delete mOptionComponents[ i ];
	}
}

//*************************************************************************************
//
//*************************************************************************************
EMenuOption	IMainMenuScreen::AsMenuOption( s32 option )
{
	s32 m( option % s32(NUM_MENU_OPTIONS) );
	if( m < 0 )
		m += NUM_MENU_OPTIONS;

	DAEDALUS_ASSERT( m >= 0 && m < (s32)NUM_MENU_OPTIONS, "Whoops" );

	return EMenuOption( m );
}

//*************************************************************************************
//
//*************************************************************************************
s32	IMainMenuScreen::GetPreviousValidOption() const
{
	bool		looped( false );
	s32			current_option( mCurrentOption );
	EMenuOption	initial_option( AsMenuOption( current_option ) );

	do
	{
		current_option--;
		looped = AsMenuOption( current_option ) == initial_option;
	}
	while( !IsOptionValid( AsMenuOption( current_option ) ) && !looped );

	return current_option;
}

//*************************************************************************************
//
//*************************************************************************************
s32	IMainMenuScreen::GetNextValidOption() const
{
	bool			looped( false );
	s32			current_option( mCurrentOption );
	EMenuOption	initial_option( AsMenuOption( current_option ) );

	do
	{
		current_option++;
		looped = AsMenuOption( current_option ) == initial_option;
	}
	while( !IsOptionValid( AsMenuOption( current_option ) ) && !looped );

	return current_option;
}

//*************************************************************************************
//
//*************************************************************************************
bool	IMainMenuScreen::IsOptionValid( EMenuOption option ) const
{
	// Rom Settings is only valid if a rom has already been selected
	if( option == MO_SELECTED_ROM )
	{
		return !mRomFilename.empty();
	}

	return true;
}


//*************************************************************************************
//
//*************************************************************************************
void	IMainMenuScreen::Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons )
{
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
	}

	// Approach towards target
	mCurrentDisplayOption = mCurrentDisplayOption + (mCurrentOption - mCurrentDisplayOption) * 0.1f;

	mOptionComponents[ GetCurrentOption() ]->Update( elapsed_time, stick, old_buttons, new_buttons );
}

//*************************************************************************************
//
//*************************************************************************************
void	IMainMenuScreen::Render()
{
	mpContext->ClearBackground();


	c32		valid_colour( mpContext->GetDefaultTextColour() );
	c32		invalid_colour( 200, 200, 200 );

	f32		min_scale( 0.60f );
	f32		max_scale( 1.0f );

	s32		SCREEN_LEFT = 20;
	s32		SCREEN_RIGHT = SCREEN_WIDTH - 20;

	mpContext->SetFontStyle( CUIContext::FS_HEADING );

	s32		y( TEXT_AREA_TOP + mpContext->GetFontHeight() );

	for( s32 i = -2; i <= 2; ++i )
	{
		EMenuOption		option( AsMenuOption( mCurrentOption + i ) );
		c32				text_col( IsOptionValid( option ) ? valid_colour : invalid_colour );
		const char *	option_text( gMenuOptionNames[ option ] );
		u32				text_width( mpContext->GetTextWidth( option_text ) );

		f32				diff( f32( mCurrentOption + i ) - mCurrentDisplayOption );
		f32				dist( pspFpuAbs( diff ) );

		s32				centre( ( SCREEN_WIDTH - text_width ) / 2 );
		s32				extreme( diff < 0 ? SCREEN_LEFT : s32( SCREEN_RIGHT - (text_width * min_scale) ) );

		// Interpolate between central and extreme position and centre
		f32				scale( max_scale + (min_scale - max_scale) * dist );
		s32				x( s32( centre + (extreme - centre) * dist ) );

		mpContext->DrawTextScale( x, y, scale, option_text, text_col );
	}

	mpContext->SetFontStyle( CUIContext::FS_REGULAR );

	mOptionComponents[ GetCurrentOption() ]->Render();
}

//*************************************************************************************
//
//*************************************************************************************
void	IMainMenuScreen::Run()
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
void	IMainMenuScreen::OnRomSelected( const char * rom_filename )
{
	u32			rom_size;
	ECicType	boot_type;

	if(ROM_GetRomDetailsByFilename( rom_filename, &mRomID, &rom_size, &boot_type ))
	{
		mRomFilename = rom_filename;
		mSelectedRomComponent->SetRomID( mRomID );
		mCurrentOption = MO_SELECTED_ROM;
		mCurrentDisplayOption = float( mCurrentOption );		// Snap to this
	}
	else
	{
		// XXXX Problem retrieving rom info- should report this!
		mRomFilename = "";
	}
}

//*************************************************************************************
// This feature is not really stable
//*************************************************************************************
void	IMainMenuScreen::OnSavestateSelected( const char * savestate_filename )
{
	if (!CPU_IsRunning())
	{
		const char *romName = SaveState_GetRom(savestate_filename);

		if (romName == NULL)
		{
			// report error?
			return;
		}

		System_Open(romName);
		mIsFinished = true;
	}

	if( CPU_LoadState( savestate_filename ) )
	{
		mIsFinished = true;
	}
	else
	{
		// Should report some kind of error
	}
}

//*************************************************************************************
//
//*************************************************************************************
void	IMainMenuScreen::OnStartEmulation()
{
	System_Open(mRomFilename.c_str());
	mIsFinished = true;
}
