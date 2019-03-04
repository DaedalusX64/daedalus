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
#include "UISetting.h"
#include "UIContext.h"

#include "SysPSP/Graphics/DrawText.h"


CUISetting::CUISetting( const char * name, const char * description )
	:	mName( name )
	,	mDescription( description )
{}

u32		CUISetting::GetHeight( CUIContext * context ) const
{
	return context->GetFontHeight() + 2;
}

void	CUISetting::Draw( CUIContext * context, s32 min_x, s32 max_x, EAlignType halign, s32 y, bool selected ) const
{
	bool		read_only( IsReadOnly() );

	c32	colour;
	if( selected )
	{
		colour = read_only ? DrawTextUtilities::TextRedDisabled : context->GetSelectedTextColour();
	}
	else
	{
		colour = read_only ? DrawTextUtilities::TextWhiteDisabled : context->GetDefaultTextColour();
	}

	// This ignores halign. Always draw name on left and setting on the right

	y += context->GetFontHeight();

	// Draw the name on the left
	context->DrawText( min_x, y, mName, colour );

	// And the current setting on the right
	const char * setting_name( GetSettingName() );
	context->DrawTextAlign( min_x, max_x, AT_RIGHT, y, setting_name, colour );
}
