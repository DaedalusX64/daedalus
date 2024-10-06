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


#ifndef UI_UISPACER_H_
#define UI_UISPACER_H_

#include "UIElement.h"


class CUISpacer : public CUIElement
{
public:
	CUISpacer( u32 height )
		:	mHeight( height )
	{}

	virtual ~CUISpacer() {}

	virtual bool			IsSelectable() const						{ return false; }

	virtual u32				GetHeight( CUIContext * context [[maybe_unused]] ) const		{ return mHeight; }
	virtual void			Draw( CUIContext * context [[maybe_unused]], s32 min_x [[maybe_unused]], [[maybe_unused]] s32 max_x [[maybe_unused]], EAlignType halign [[maybe_unused]], s32 y [[maybe_unused]], bool selected [[maybe_unused]] ) const {}

	virtual const std::string	GetDescription() const	{ return ""; }

private:
	u32						mHeight;
};


#endif // UI_UISPACER_H_
