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


#ifndef UISPACER_H_
#define UISPACER_H_

#include "UIElement.h"

//*************************************************************************************
//
//*************************************************************************************
class CUISpacer : public CUIElement
{
public:
	CUISpacer( u32 height )
		:	mHeight( height )
	{
	}

	virtual ~CUISpacer() {}

	virtual bool			IsSelectable() const						{ return false; }

	virtual u32				GetHeight( CUIContext * context ) const		{ return mHeight; }
	virtual void			Draw( CUIContext * context, s32 min_x, s32 max_x, EAlignType halign, s32 y, bool selected ) const {}

	virtual const char *	GetDescription() const	{ return ""; }

private:
	u32						mHeight;
};


#endif	// UISPACER_H_
