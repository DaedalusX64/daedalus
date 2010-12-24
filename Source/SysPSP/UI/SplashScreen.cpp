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
#include "SplashScreen.h"

#include "UIContext.h"
#include "UIScreen.h"
#include "Graphics/ColourValue.h"
#include "Graphics/NativeTexture.h"

#include "Math/Math.h"	// VFPU Math

#include "Utility/Preferences.h"
#include "SysPSP/Utility/PathsPSP.h"

#include <pspctrl.h>
#include <pspgu.h>

namespace
{
	const char * const		LOGO_FILENAME = DAEDALUS_PSP_PATH( "Resources/logo.png" );

	const float				MAX_TIME = 0.8f;
}

//*************************************************************************************
//
//*************************************************************************************
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
		CRefPtr<CNativeTexture>		mpTexture;
};

//*************************************************************************************
//
//*************************************************************************************
CSplashScreen::~CSplashScreen()
{
}

//*************************************************************************************
//
//*************************************************************************************
CSplashScreen *	CSplashScreen::Create( CUIContext * p_context )
{
	return new ISplashScreen( p_context );
}

//*************************************************************************************
//
//*************************************************************************************
ISplashScreen::ISplashScreen( CUIContext * p_context )
:	CUIScreen( p_context )
,	mIsFinished( false )
,	mElapsedTime( 0.0f )
,	mpTexture( CNativeTexture::CreateFromPng( LOGO_FILENAME, TexFmt_8888 ) )
{

}

//*************************************************************************************
//
//*************************************************************************************
ISplashScreen::~ISplashScreen()
{
}

//*************************************************************************************
//
//*************************************************************************************
void	ISplashScreen::Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons )
{
	// If any button was unpressed and is now pressed, exit
#ifndef DAEDALUS_PUBLIC_RELEASE
	if(((~old_buttons) & new_buttons) || gGlobalPreferences.SkipSplash )
#else
	if((~old_buttons) & new_buttons)
#endif
	{
		mIsFinished = true;
	}

	mElapsedTime += elapsed_time;
	if( mElapsedTime > MAX_TIME )
	{
		mIsFinished = true;
	}
}

//*************************************************************************************
//
//*************************************************************************************
void	ISplashScreen::Render()
{
	f32	alpha( 255.0f * vfpu_sinf( mElapsedTime * PI / MAX_TIME ) );
	u8		a;
	if( alpha >= 255.0f ) a = 255;
	else if (alpha < 0.f) a = 0;
	else	a = u8( alpha );

	c32		colour( 255, 255, 255, a );

	mpContext->ClearBackground();
	mpContext->RenderTexture( mpTexture, (480 - 328)/2, (272-90)/2, colour );
}

//*************************************************************************************
//
//*************************************************************************************
void	ISplashScreen::Run()
{
	CUIScreen::Run();
}
