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

CUIElement::~CUIElement()
{}

CUIElementBag::CUIElementBag()
:	mSelectedIdx( 0 )
{}

CUIElementBag::~CUIElementBag()
{
	for( u32 i = 0; i < mElements.size(); ++i )
	{
		delete mElements[ i ];
	}
}


void CUIElementBag::Draw( CUIContext * context, s32 min_x, s32 max_x, EAlignType halign, s32 y ) const
{
	const s32 MINTOP = 25; //Only draw text below
	const s32 MAXBOT = 247;	//Only draw text above

	for( auto i = 0; i < mElements.size(); ++i )
	{
		const auto element = mElements[ i ];

		if (y > MINTOP && y < MAXBOT ) element->Draw( context, min_x, max_x, halign, y, i == mSelectedIdx );
		y += element->GetHeight( context );
	}
}


void	CUIElementBag::DrawCentredVertically( CUIContext * context, s32 min_x, s32 min_y, s32 max_x, s32 max_y ) const
{
	s32 total_height = 0;

	for( auto i = 0; i < mElements.size(); ++i )
	{
		const auto element = mElements[ i ];

		total_height += element->GetHeight( context );
	}

	s32	slack = (max_y - min_y) - total_height;
	s32	y = min_y + (slack / 2);

	for( auto i = 0; i < mElements.size(); ++i )
	{
		const auto element = mElements[ i ];

		element->Draw( context, min_x, max_x, AT_CENTRE, y, i == mSelectedIdx );
		y += element->GetHeight( context );
	}

}


void CUIElementBag::SelectNext() {
    u32 original_selection = mSelectedIdx;
    u32 size = mElements.size();
    for (auto i = 1; i <= size; ++i) {
        u32 new_selection = (original_selection + i) % size;
        if (mElements[new_selection]->IsSelectable()) {
            mSelectedIdx = new_selection;
            return;
        }
    }
}



void CUIElementBag::SelectPrevious() {
    s32 original_selection = mSelectedIdx;
    u32 size = mElements.size();
    for (u32 i = 1; i <= size; ++i) {
        s32 new_selection = (original_selection - i + size) % size;
        if (mElements[new_selection]->IsSelectable()) {
            mSelectedIdx = new_selection;
            return;
        }
    }
}
