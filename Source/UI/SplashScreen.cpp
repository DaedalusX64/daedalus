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

#include "Graphics/ColourValue.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/NativeTexture.h"
#include "Math/Math.h"	// VFPU Math
#include "DrawTextUtilities.h"
#include "Menu.h"
#include "SplashScreen.h"
#include "UIContext.h"
#include "UIScreen.h"

#include "Interface/Preferences.h"

extern bool g32bitColorMode;

class ISplashScreen : public CSplashScreen, public CUIScreen
{
	public:

		ISplashScreen( CUIContext * p_context );
		~ISplashScreen();

		// CSplashScreen
		virtual void				Run();

		// CUIScreen
		virtual void				Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();
		virtual bool				IsFinished() const									{ return mIsFinished; }

	private:
		bool						mIsFinished;
		float						mElapsedTime;
		std::shared_ptr<CNativeTexture>		mpTexture;
};


CSplashScreen::~CSplashScreen() {}


std::unique_ptr<CSplashScreen>	CSplashScreen::Create( CUIContext * p_context )
{
	return std::make_unique<ISplashScreen>( p_context );
}


ISplashScreen::ISplashScreen( CUIContext * p_context )
:	CUIScreen( p_context )
,	mIsFinished( false )
,	mElapsedTime( 0.0f )
,	mpTexture( CNativeTexture::CreateFromPng( LOGO_FILENAME, TexFmt_8888 ) )
{}


ISplashScreen::~ISplashScreen() {}


void	ISplashScreen::Update( float elapsed_time , const v2 & stick[[maybe_unused]], u32 old_buttons, u32 new_buttons )
{
	// If any button was unpressed and is now pressed, exit
	if((~old_buttons) & new_buttons)
	{
		mIsFinished = true;
	}

	mElapsedTime += elapsed_time;
	if( mElapsedTime > MAX_TIME )
	{
		mIsFinished = true;
	}
}

void	ISplashScreen::Render()
{
	f32	alpha = 255.0f * sinf( mElapsedTime * PI / MAX_TIME );
	u8		a = 0;
	if( alpha >= 255.0f )
	{ a = 255;
	}
	else if (alpha < 0.f)
	{
		 a = 0;
	}
	else	
	{
		a = static_cast<u8>( alpha );
	}
	c32		colour( 255, 255, 255, a );

	mpContext->ClearBackground();
	mpContext->RenderTexture( mpTexture, (SCREEN_WIDTH - mpTexture->GetWidth()) / 2, (SCREEN_HEIGHT - mpTexture->GetHeight()) / 2, colour);

#if DAEDALUS_PSP
	mpContext->SetFontStyle( CUIContext::FS_HEADING );
	mpContext->DrawTextAlign(0,SCREEN_WIDTH,AT_CENTRE,SCREEN_HEIGHT-50,g32bitColorMode? "32Bit Color Selected" : "16Bit Color Selected",DrawTextUtilities::TextWhite);
#endif
}

void	ISplashScreen::Run()
{
	CUIScreen::Run();
}
