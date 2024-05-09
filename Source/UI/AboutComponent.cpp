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
#include "AboutComponent.h"

#include "UIContext.h"
#include "UIScreen.h"

#include "Graphics/ColourValue.h"
#include "Graphics/NativeTexture.h"
#include "Base/MathUtil.h"
#include "Math/Vector2.h"
#include "DrawTextUtilities.h"

#include "Base/Macros.h"
#include "Utility/Translate.h"
#include "PSPMenu.h"

#include <string>

class IAboutComponent : public CAboutComponent
{
	public:

		IAboutComponent( CUIContext * p_context );
		~IAboutComponent();

		// CUIComponent
		virtual void				Update( f32 elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();

	private:
		std::shared_ptr<CNativeTexture>		mpTexture;
};


CAboutComponent::CAboutComponent( CUIContext * p_context )
:	CUIComponent( p_context )
 {}


CAboutComponent::~CAboutComponent() {}


CAboutComponent *	CAboutComponent::Create( CUIContext * p_context )
{
	return new IAboutComponent( p_context );
}

IAboutComponent::IAboutComponent( CUIContext * p_context )
:	CAboutComponent( p_context )
,	mpTexture( CNativeTexture::CreateFromPng( LOGO_FILENAME, TexFmt_8888 ) )
{}


IAboutComponent::~IAboutComponent() {}

void	IAboutComponent::Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons ) {}


void	IAboutComponent::Render()
{

		s16 text_top = 38;

	if(mpTexture != NULL)
	{
		s16		w = mpTexture->GetWidth();
		s16		h = mpTexture->GetHeight();

		f32		desired_height = 60.0f;
		f32		scale = desired_height / f32( h );

		v2		wh( f32( w ) * scale, f32( h ) * scale );
		v2		tl( f32( (SCREEN_WIDTH - wh.x)/2 ), f32( text_top ) );

		mpContext->RenderTexture( mpTexture, tl, wh, c32::White );

		text_top += u32( wh.y + 10.0f );
	}

	const s16	line_height = mpContext->GetFontHeight() + 2;

	s16 y = text_top;

	std::string	version = DAEDALUS_VERSION_TEXT + DAEDALUS_CONFIG_VERSION;


	std::string	date = DATE_TEXT + __DATE__;
	mpContext->DrawTextAlign( LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_CENTRE, y, version.data(), DrawTextUtilities::TextWhite ); y += line_height;
	mpContext->DrawTextAlign( LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_CENTRE, y, date.data(), DrawTextUtilities::TextWhite ); y += line_height;

	// Spacer
	y += line_height;

	for( auto i = 0; i < ARRAYSIZE( INFO_TEXT ); ++i )
	{
		const char * str = INFO_TEXT[ i ];

		mpContext->DrawTextAlign( LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_CENTRE, y, str, DrawTextUtilities::TextWhite );
		y += line_height;
	}

	mpContext->DrawTextAlign( LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_CENTRE, y, URL_TEXT_1, DrawTextUtilities::TextRed, c32( 255,255,255,160 ) );	y += line_height;
	mpContext->DrawTextAlign( LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_CENTRE, y, URL_TEXT_2, DrawTextUtilities::TextRed, c32( 255,255,255,255 ) );	y += line_height;
}
