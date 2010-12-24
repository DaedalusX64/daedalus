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


#ifndef UIELEMENT_H_
#define UIELEMENT_H_

class CUIContext;
class c32;

#include <vector>
#include "UIAlignment.h"

//*************************************************************************************
//
//*************************************************************************************
class CUIElement
{
public:
	virtual ~CUIElement();

	virtual bool			IsSelectable() const	{ return true; }

	virtual	void			OnSelected()			{}		// Selected with Start/X
	virtual	void			OnNext()				{}
	virtual	void			OnPrevious()			{}

	virtual u32				GetHeight( CUIContext * context ) const = 0;
	virtual void			Draw( CUIContext * context, s32 min_x, s32 max_x, EAlignType halign, s32 y, bool selected ) const = 0;

	virtual const char *	GetDescription() const = 0;


private:
};

class CUIElementBag
{
public:
	CUIElementBag();
	~CUIElementBag();

	u32				Add( CUIElement * element )		{ u32 idx = mElements.size(); mElements.push_back( element ); return idx; }
	void			Clear()	{	mElements.clear(); mSelectedIdx = 0; }

	void			SetSelected( u32 idx )			{ DAEDALUS_ASSERT( idx < mElements.size(), "Invalid idx" ); mSelectedIdx = idx; }

	void			SelectNext();
	void			SelectPrevious();

	u32				GetNumElements() const			{ return mElements.size(); }
	CUIElement *	GetElement( u32 i ) const		{ DAEDALUS_ASSERT( i < mElements.size(), "Invalid idx" ); return mElements[ i ]; }
	u32		GetSelectedIndex()	{ return mSelectedIdx; }
	CUIElement *	GetSelectedElement() const		{ if( mSelectedIdx < mElements.size() ) return mElements[ mSelectedIdx ]; return NULL; }

	void			Draw( CUIContext * context, s32 min_x, s32 max_x, EAlignType halign, s32 y ) const;
	void			DrawCentredVertically( CUIContext * context, s32 min_x, s32 min_y, s32 max_x, s32 max_y ) const;

private:
	std::vector< CUIElement * >			mElements;
	u32									mSelectedIdx;
};


#endif	// UIELEMENT_H_
