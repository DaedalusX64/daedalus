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


#include "Base/Types.h"


#include <string>

#include "Core/CPU.h"
#include "Core/ROM.h"
#include "RomFile/RomSettings.h"
#include "Interface/SaveState.h"
#include "Graphics/ColourValue.h"
#include "Graphics/GraphicsContext.h"
#include "Utility/MathUtil.h"
#include "DrawTextUtilities.h"
#include "AboutComponent.h"
#include "GlobalSettingsComponent.h"
#include "MainMenuScreen.h"
#include "Menu.h"
#include "RomSelectorComponent.h"
#include "SelectedRomComponent.h"
#include "SavestateSelectorComponent.h"
#include "UIContext.h"
#include "UIScreen.h"
#include "SplashScreen.h"
#include "System/SystemInit.h"
#include "Interface/Preferences.h"


namespace
{

	enum EMenuOption
	{
		MO_GLOBAL_SETTINGS = 0,
		MO_ROMS,
		MO_SELECTED_ROM,
		MO_SAVESTATES,
		MO_ABOUT,
	};
	const s16 NUM_MENU_OPTIONS {MO_ABOUT+1};

	const EMenuOption	MO_FIRST_OPTION [[maybe_unused]] = MO_GLOBAL_SETTINGS;
	const EMenuOption	MO_LAST_OPTION [[maybe_unused]] = MO_ABOUT;

	const char * const	gMenuOptionNames[ NUM_MENU_OPTIONS ] =
	{
		"Global Settings",
		"Roms",
		"Selected Rom",
		"Savestates",
		"About",
	};
}

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


CMainMenuScreen::~CMainMenuScreen() {}

std::unique_ptr<CMainMenuScreen>	CMainMenuScreen::Create( CUIContext * p_context )
{
	return std::make_unique<IMainMenuScreen>( p_context );
}


IMainMenuScreen::IMainMenuScreen( CUIContext * p_context )
:	CUIScreen( p_context )
,	mIsFinished( false )
,	mCurrentOption( MO_ROMS )
,	mCurrentDisplayOption( mCurrentOption )
{
	for( auto i {0}; i < NUM_MENU_OPTIONS; ++i )
	{
		mOptionComponents[ i ] = nullptr;
	}

	mSelectedRomComponent = CSelectedRomComponent::Create( mpContext, [this]() { this->OnStartEmulation(); });

	mOptionComponents[ MO_GLOBAL_SETTINGS ]	= CGlobalSettingsComponent::Create( mpContext );
	mOptionComponents[ MO_ROMS ] 			= CRomSelectorComponent::Create( mpContext, [this](const char *rom ) { this->OnRomSelected(rom); } );
	mOptionComponents[ MO_SELECTED_ROM ]	= mSelectedRomComponent;
	mOptionComponents[ MO_SAVESTATES ]		= CSavestateSelectorComponent::Create( mpContext, CSavestateSelectorComponent::AT_LOADING,[this](const char* savestate) { this->OnSavestateSelected(savestate); }, std::filesystem::path());
	mOptionComponents[ MO_ABOUT ]			= CAboutComponent::Create( mpContext );

}

IMainMenuScreen::~IMainMenuScreen()
{
	for( u32 i = 0; i < NUM_MENU_OPTIONS; ++i )
	{
		delete mOptionComponents[ i ];
	}
}

EMenuOption	IMainMenuScreen::AsMenuOption( s32 option )
{
	s32 m = option % static_cast<s32>(NUM_MENU_OPTIONS);
	if( m < 0 )
		m += NUM_MENU_OPTIONS;

#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( m >= 0 && m < (s32)NUM_MENU_OPTIONS, "Whoops" );
#endif
	return EMenuOption( m );
}


s32	IMainMenuScreen::GetPreviousValidOption() const
{
	bool		looped =  false;
	s32			current_option =  mCurrentOption;
	EMenuOption	initial_option = AsMenuOption( current_option );

	do
	{
		current_option--;
		looped = AsMenuOption( current_option ) == initial_option;
	}
	while( !IsOptionValid( AsMenuOption( current_option ) ) && !looped );

	return current_option;
}


//

s32	IMainMenuScreen::GetNextValidOption() const
{
	bool			looped =  false;
	s32			current_option = mCurrentOption;
	EMenuOption	initial_option = AsMenuOption( current_option );

	do
	{
		current_option++;
		looped = AsMenuOption( current_option ) == initial_option;
	}
	while( !IsOptionValid( AsMenuOption( current_option ) ) && !looped );

	return current_option;
}


//

bool	IMainMenuScreen::IsOptionValid( EMenuOption option ) const
{
	// Rom Settings is only valid if a rom has already been selected
	if( option == MO_SELECTED_ROM )
	{
		return !mRomFilename.empty();
	}

	return true;
}



//

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


//

void	IMainMenuScreen::Render()
{
	mpContext->ClearBackground();


	c32		valid_colour( mpContext->GetDefaultTextColour() );
	c32		invalid_colour( 200, 200, 200 );

	f32		min_scale =  0.60f;
	f32		max_scale =  1.0f;

	s32		SCREEN_LEFT = 20;
	s32		SCREEN_RIGHT = SCREEN_WIDTH - 20;

	mpContext->SetFontStyle( CUIContext::FS_HEADING );

	s32		y = MENU_TOP + mpContext->GetFontHeight();

	for( s32 i = -2; i <= 2; ++i )
	{
		EMenuOption		option =  AsMenuOption( mCurrentOption + i );
		c32				text_col( IsOptionValid( option ) ? valid_colour : invalid_colour );
		const char *	option_text =  gMenuOptionNames[ option ];
		u32				text_width =  mpContext->GetTextWidth( option_text );

		f32				diff =  static_cast<f32>( mCurrentOption + i ) - mCurrentDisplayOption;
		f32				dist = fabsf( diff );

		s32				centre = ( SCREEN_WIDTH - text_width ) / 2;
		s32				extreme = diff < 0 ? SCREEN_LEFT : static_cast<s32>( SCREEN_RIGHT - (text_width * min_scale) );

		// Interpolate between central and extreme position and centre
		f32				scale =  max_scale + (min_scale - max_scale) * dist;
		s32				x = s32( centre + (extreme - centre) * dist );

		mpContext->DrawTextScale( x, y, scale, option_text, text_col );
	}

	mpContext->SetFontStyle( CUIContext::FS_REGULAR );

	mOptionComponents[ GetCurrentOption() ]->Render();
}


void	IMainMenuScreen::Run()
{
	mIsFinished = false;
	CUIScreen::Run();

	// switch back to the emulator display
	CGraphicsContext::Get()->SwitchToChosenDisplay();

	// Clear everything to black - looks a bit tidier
	CGraphicsContext::Get()->ClearAllSurfaces();
}

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


// This feature is not really stable

void	IMainMenuScreen::OnSavestateSelected( const char * savestate_filename )
{
	// If the CPU is running we need to queue a request to load the state
	// (and possibly switch roms). Otherwise we just load the rom directly
	if( CPU_IsRunning() )
	{
		if( CPU_RequestLoadState( savestate_filename ) )
		{
			mIsFinished = true;
		}
		else
		{
			// Should report some kind of error
		}
	}
	else
	{
		const std::filesystem::path rom_filename = SaveState_GetRom(savestate_filename);

		System_Open(rom_filename);

		if( !SaveState_LoadFromFile( savestate_filename ) )
		{
			// Should report some kind of error
		}

		mIsFinished = true;
	}
}


void	IMainMenuScreen::OnStartEmulation()
{
	System_Open(mRomFilename.c_str());
	mIsFinished = true;
}

void DisplayRomsAndChoose(bool show_splash)
{
	// switch back to the LCD display
	CGraphicsContext::Get()->SwitchToLcdDisplay();

	auto	p_context =  CUIContext::Create();

	if(p_context != NULL)
	{
		p_context->ClearBackground(c32::Red);

		if( show_splash )
		{
			auto p_splash = CSplashScreen::Create( p_context );
			p_splash->Run();
		}

		auto p_main_menu = CMainMenuScreen::Create( p_context );
		p_main_menu->Run();
	}

	delete p_context;
}

