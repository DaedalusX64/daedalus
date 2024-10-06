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
#include "AdjustDeadzoneScreen.h"

#include <algorithm>


#include "UIContext.h"
#include "UIScreen.h"

#include "Input/InputManager.h"
#include "Math/Math.h"	// VFPU Math
#include "Utility/MathUtil.h"
#include "Math/Vector2.h"
#include "DrawTextUtilities.h"
#include "Interface/Preferences.h"
#include "Utility/Translate.h"
#include "Menu.h"
#include <algorithm>


class IAdjustDeadzoneScreen : public CAdjustDeadzoneScreen, public CUIScreen
{
	public:

		IAdjustDeadzoneScreen( CUIContext * p_context );
		~IAdjustDeadzoneScreen();

		virtual void				Run();
		virtual void				Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();
		virtual bool				IsFinished() const									{ return mIsFinished; }

	private:
		void						DrawCircle( s32 x, s32 y, s32 r, c32 colour );
		void						DrawCrosshair( s32 x, s32 y, c32 colour );
		void						DrawStick( s32 x, s32 y, s32 r, const v2 & stick, f32 min_deadzone, f32 max_deadzone );

	private:
		bool						mIsFinished;
		v2							mPspStick;
		v2							mN64Stick;
		bool						mAdjustingMinDeadzone;
		f32							mStickMinDeadzone;
		f32							mStickMaxDeadzone;
};


CAdjustDeadzoneScreen::~CAdjustDeadzoneScreen() {}


CAdjustDeadzoneScreen *	CAdjustDeadzoneScreen::Create( CUIContext * p_context )
{
	return new IAdjustDeadzoneScreen( p_context );
}


IAdjustDeadzoneScreen::IAdjustDeadzoneScreen( CUIContext * p_context )
:	CUIScreen( p_context )
,	mIsFinished( false )
,	mPspStick( 0.0f, 0.0f )
,	mN64Stick( 0.0f, 0.0f )
,	mAdjustingMinDeadzone( true )
,	mStickMinDeadzone( gGlobalPreferences.StickMinDeadzone )
,	mStickMaxDeadzone( gGlobalPreferences.StickMaxDeadzone )
{
}

IAdjustDeadzoneScreen::~IAdjustDeadzoneScreen() {}


void	IAdjustDeadzoneScreen::Update( float elapsed_time[[maybe_unused]], const v2 & stick [[maybe_unused]], u32 old_buttons, u32 new_buttons )
{
	if(new_buttons & PSP_CTRL_DOWN)
	{
		if( mAdjustingMinDeadzone )
		{
			mStickMinDeadzone = std::clamp( mStickMinDeadzone - DEADZONE_INCREMENT, 0.0f, mStickMaxDeadzone );
		}
		else
		{
			mStickMaxDeadzone = std::clamp( mStickMaxDeadzone - DEADZONE_INCREMENT, mStickMinDeadzone, 1.0f );
		}
	}
	if(new_buttons & PSP_CTRL_UP)
	{
		if( mAdjustingMinDeadzone )
		{
			mStickMinDeadzone = std::clamp( mStickMinDeadzone + DEADZONE_INCREMENT, 0.0f, mStickMaxDeadzone );
		}
		else
		{
			mStickMaxDeadzone = std::clamp( mStickMaxDeadzone + DEADZONE_INCREMENT, mStickMinDeadzone, 1.0f );
		}
	}

	if(old_buttons != new_buttons)
	{
		if(new_buttons & (PSP_CTRL_LEFT|PSP_CTRL_RIGHT))
		{
			mAdjustingMinDeadzone = !mAdjustingMinDeadzone;
		}

		if(new_buttons & (PSP_CTRL_TRIANGLE))
		{
			mStickMinDeadzone = DEFAULT_MIN_DEADZONE;
			mStickMaxDeadzone = DEFAULT_MAX_DEADZONE;
		}

		if(new_buttons & (PSP_CTRL_CIRCLE|PSP_CTRL_SELECT))
		{
			// Discard settings
			mIsFinished = true;
		}

		if(new_buttons & (PSP_CTRL_CROSS|PSP_CTRL_START))
		{
			// Commit settings
			gGlobalPreferences.StickMinDeadzone = mStickMinDeadzone;
			gGlobalPreferences.StickMaxDeadzone = mStickMaxDeadzone;
			mIsFinished = true;
		}
	}

	// Ignore the 'stick' input. This has already been normalised etc by the UI
	//mLastStick = stick;
	SceCtrlData		pad;
	sceCtrlPeekBufferPositive(&pad, 1);

	s32		stick_x = pad.Lx - 128;
	s32		stick_y = pad.Ly - 128;

	mPspStick.x = static_cast<f32>(stick_x) / 128.0f;
	mPspStick.y = static_cast<f32>(stick_y) / 128.0f;

	mN64Stick = ApplyDeadzone( mPspStick, mStickMinDeadzone, mStickMaxDeadzone );
}


void	IAdjustDeadzoneScreen::DrawCircle( s32 x, s32 y, s32 r, c32 colour )
{
	const u32 NUM_POINTS = 32;

	f32		radius =  r;
	s32		x0 = static_cast<s32>( sinf( 0 ) * radius ) + x;
	s32		y0 = static_cast<s32>( cosf( 0 ) * radius ) + y;

	for( u32 i = 0; i < NUM_POINTS; ++i )
	{
		f32		angle( 2 * PI * static_cast<f32>( i+1 ) / static_cast<f32>( NUM_POINTS ) );
		s32		x1 = static_cast<s32>( sinf( angle ) * radius ) + x;
		s32		y1 = static_cast<s32>( cosf( angle ) * radius ) + y;

		mpContext->DrawLine( x0, y0, x1, y1, colour );
		x0 = x1;
		y0 = y1;
	}
}


void	IAdjustDeadzoneScreen::DrawCrosshair( s32 x, s32 y, c32 colour )
{
	mpContext->DrawLine( x - 4, y, x - 1, y, colour );
	mpContext->DrawLine( x + 4, y, x + 1, y, colour );
	mpContext->DrawLine( x, y - 4, x, y - 1, colour );
	mpContext->DrawLine( x, y + 4, x, y + 1, colour );
}


void	IAdjustDeadzoneScreen::DrawStick( s32 x, s32 y, s32 r, const v2 & stick, f32 min_deadzone, f32 max_deadzone )
{
	c32		white( 255, 255, 255 );
	c32		red( 255, 0, 0 );

	mpContext->DrawLine( x - r, y - r, x - r, y + r, white );
	mpContext->DrawLine( x - r, y + r, x + r, y + r, white );
	mpContext->DrawLine( x + r, y + r, x + r, y - r, white );
	mpContext->DrawLine( x + r, y - r, x - r, y - r, white );

	if( max_deadzone >= 0.0f )
	{
		DrawCircle( x, y, s32(r * max_deadzone), mAdjustingMinDeadzone ? white : red );
	}

	if( min_deadzone >= 0.0f )
	{
		DrawCircle( x, y, s32(r * min_deadzone), mAdjustingMinDeadzone ? red : white );
	}

	s32		stick_x = x + s32( stick.x * r );
	s32		stick_y = y + s32( stick.y * r );

	DrawCrosshair( stick_x, stick_y, white );
}


void	IAdjustDeadzoneScreen::Render()
{
	mpContext->ClearBackground();

	mpContext->DrawTextAlign( 0, SCREEN_WIDTH, AT_CENTRE, TITLE_Y, TITLE_TEXT, DrawTextUtilities::TextWhite );

	mpContext->DrawText( PSP_TITLE_X, PSP_TITLE_Y, "PSP", DrawTextUtilities::TextWhite );
	mpContext->DrawText( N64_TITLE_X, N64_TITLE_Y, "N64", DrawTextUtilities::TextWhite );

	DrawStick( PSP_CIRCLE_X, PSP_CIRCLE_Y, DISPLAY_RADIUS, mPspStick, mStickMinDeadzone, mStickMaxDeadzone );
	DrawStick( N64_CIRCLE_X, N64_CIRCLE_Y, DISPLAY_RADIUS, mN64Stick, -1.0f, -1.0f );

	char str[ 128 ];
	snprintf( str, sizeof(str), "%s: %d, %s: %d", Translate_String("Min"), s32( 100.f * mStickMinDeadzone ), Translate_String("Max"), s32( 100.0f * mStickMaxDeadzone ) );
	mpContext->DrawText( PSP_CIRCLE_X - DISPLAY_RADIUS, PSP_CIRCLE_Y + DISPLAY_RADIUS + 10, str, DrawTextUtilities::TextWhite );

	mpContext->DrawTextArea( LIST_TEXT_LEFT,
							 BELOW_MENU_MIN,
							 LIST_TEXT_WIDTH - LIST_TEXT_LEFT,
							 (ROM_INFO_TEXT_X - 30) - BELOW_MENU_MIN,
							 INSTRUCTIONS_TEXT,
							 DrawTextUtilities::TextWhite,
							 VA_BOTTOM );
}


void	IAdjustDeadzoneScreen::Run()
{
	CUIScreen::Run();
}
