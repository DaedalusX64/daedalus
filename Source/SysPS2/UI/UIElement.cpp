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
#include "UIElement.h"

#include "UIContext.h"

//*************************************************************************************
//
//*************************************************************************************
CUIElement::~CUIElement()
{
}

//*************************************************************************************
//
//*************************************************************************************
CUIElementBag::CUIElementBag()
:	mSelectedIdx( 0 )
{

}

//*************************************************************************************
//
//*************************************************************************************
CUIElementBag::~CUIElementBag()
{
	for( u32 i = 0; i < mElements.size(); ++i )
	{
		delete mElements[ i ];
	}
}

//*************************************************************************************
//
//*************************************************************************************
void CUIElementBag::Draw( CUIContext * context, s32 min_x, s32 max_x, EAlignType halign, s32 y ) const
{
	const s32 MINTOP = 25; //Only draw text below
	const s32 MAXBOT = 247;	//Only draw text above

	for( u32 i = 0; i < mElements.size(); ++i )
	{
		const CUIElement *	element( mElements[ i ] );

		if (y > MINTOP && y < MAXBOT ) element->Draw( context, min_x, max_x, halign, y, i == mSelectedIdx );
		y += element->GetHeight( context );
	}
}

//*************************************************************************************
//
//*************************************************************************************
void	CUIElementBag::DrawCentredVertically( CUIContext * context, s32 min_x, s32 min_y, s32 max_x, s32 max_y ) const
{
	s32 total_height( 0 );

	for( u32 i = 0; i < mElements.size(); ++i )
	{
		const CUIElement *	element( mElements[ i ] );

		total_height += element->GetHeight( context );
	}

	s32		slack( (max_y - min_y) - total_height );
	s32		y( min_y + (slack / 2) );

	for( u32 i = 0; i < mElements.size(); ++i )
	{
		const CUIElement *	element( mElements[ i ] );

		element->Draw( context, min_x, max_x, AT_CENTRE, y, i == mSelectedIdx );
		y += element->GetHeight( context );
	}

}

//*************************************************************************************
//
//*************************************************************************************
void	CUIElementBag::SelectNext()
{
	s32 new_selection = mSelectedIdx + 1;
	u32 count = 0;
	while( u32( new_selection ) != mSelectedIdx && count <= mElements.size())
	{
		if( new_selection == s32( mElements.size() ) )
			new_selection = 0;
		if( mElements[ new_selection ]->IsSelectable() )
		{
			mSelectedIdx = u32( new_selection );
			return;
		}
		new_selection++;
		count++;
	}
}

//*************************************************************************************
//
//*************************************************************************************
void CUIElementBag::SelectPrevious()
{
	s32 new_selection = mSelectedIdx - 1;
	u32 count = 0;
	while( u32( new_selection ) != mSelectedIdx && count <= mElements.size())
	{
		if( new_selection == -1 )
			new_selection = s32( mElements.size() ) - 1;
		if( mElements[ new_selection ]->IsSelectable() )
		{
			mSelectedIdx = u32( new_selection );
			return;
		}
		new_selection--;
		count++;
	}
}
