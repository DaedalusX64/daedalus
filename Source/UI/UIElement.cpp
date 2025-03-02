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
#include "UIElement.h"

#include "UIContext.h"
#include "Menu.h"

CUIElement::~CUIElement()
{}

CUIElementBag::CUIElementBag()
:	mSelectedIdx( 0 )
{}

CUIElementBag::~CUIElementBag()
{
}


void CUIElementBag::Draw( CUIContext * context, s32 min_x, s32 max_x, EAlignType halign, s32 y ) const
{
	constexpr s32 MAXBOT = SCREEN_HEIGHT - 25;	//Only draw text above

	for( std::size_t i = 0; i < mElements.size(); ++i )
	{
		const auto& element = mElements[ i ];

		if (y > LIST_TEXT_TOP && y < MAXBOT ) element->Draw( context, min_x, max_x, halign, y, i == mSelectedIdx );
		y += element->GetHeight( context );
	}
}


void	CUIElementBag::DrawCentredVertically( CUIContext * context, s32 min_x, s32 min_y, s32 max_x, s32 max_y ) const
{
	s32 total_height = 0;

	for( std::size_t i = 0; i < mElements.size(); ++i )
	{
		const auto&	element = mElements[ i ];

		total_height += element->GetHeight( context );
	}

	s32		slack = (max_y - min_y) - total_height;
	s32		y = min_y + (slack / 2);

	for( std::size_t i = 0; i < mElements.size(); ++i )
	{
		const auto& element = mElements[ i ];

		element->Draw( context, min_x, max_x, AT_CENTRE, y, i == mSelectedIdx );
		y += element->GetHeight( context );
	}

}


void	CUIElementBag::SelectNext()
{
	s32 new_selection = mSelectedIdx + 1;
	u32 count = 0;
	while( static_cast<u32>( new_selection ) != mSelectedIdx && count <= mElements.size())
	{
		if( new_selection == static_cast<s32>( mElements.size() ) )
			new_selection = 0;
		if( mElements[ new_selection ]->IsSelectable() )
		{
			mSelectedIdx = static_cast<u32>( new_selection );
			return;
		}
		new_selection++;
		count++;
	}
}


void CUIElementBag::SelectPrevious()
{
	s32 new_selection = mSelectedIdx - 1;
	u32 count = 0;
	while( static_cast<u32>( new_selection ) != mSelectedIdx && count <= mElements.size())
	{
		if( new_selection == -1 )
			new_selection = static_cast<s32>( mElements.size() ) - 1;
		if( mElements[ new_selection ]->IsSelectable() )
		{
			mSelectedIdx = static_cast<u32>( new_selection );
			return;
		}
		new_selection--;
		count++;
	}
}
