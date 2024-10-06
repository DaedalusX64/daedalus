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


#ifndef UI_UICOMMAND_H_
#define UI_UICOMMAND_H_

#include "UIElement.h"

#include <string>
#include <functional>

class CUICommand : public CUIElement
{
public:
	CUICommand( const std::string name, const std::string description )
		:	mName( name )
		,	mDescription( description )
	{

	}
	virtual ~CUICommand() {}

	virtual	void			OnSelected() = 0;

	virtual u32				GetHeight( CUIContext * context ) const;
	virtual void			Draw( CUIContext * context, s32 min_x, s32 max_x, EAlignType halign, s32 y, bool selected ) const;

	virtual const std::string GetName() const			{ return mName; }
	virtual const std::string GetDescription() const	{ return mDescription; }

private:
	std::string				mName;
	std::string				mDescription;
};


class CUICommandImpl : public CUICommand {
public:
    CUICommandImpl(std::function<void()> on_selected, const std::string name, const std::string description)
        : CUICommand(name, description), mOnSelected(on_selected) {}

    virtual void OnSelected() override {
        mOnSelected();
    }

private:
    std::function<void()> mOnSelected; // Store the function directly as std::function
};


// For e.g. unselectable items
class CUICommandDummy : public CUICommand
{
public:
	CUICommandDummy( const char * name, const char * description )
		:	CUICommand( name, description )
	{
	}
	virtual ~CUICommandDummy()
	{
	}

	virtual bool			IsSelectable() const	{ return false; }
	virtual	void			OnSelected()			{ }
};

#endif // UI_UICOMMAND_H_
