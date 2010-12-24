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


#ifndef UICOMMAND_H_
#define UICOMMAND_H_

#include "UIElement.h"

#include "Utility/Functor.h"

#include <string>

//*************************************************************************************
//
//*************************************************************************************
class CUICommand : public CUIElement
{
public:
	CUICommand( const char * name, const char * description )
		:	mName( name )
		,	mDescription( description )
	{

	}
	virtual ~CUICommand() {}

	virtual	void			OnSelected() = 0;

	virtual u32				GetHeight( CUIContext * context ) const;
	virtual void			Draw( CUIContext * context, s32 min_x, s32 max_x, EAlignType halign, s32 y, bool selected ) const;

	virtual const char *	GetName() const			{ return mName.c_str(); }
	virtual const char *	GetDescription() const	{ return mDescription.c_str(); }

private:
	std::string				mName;
	std::string				mDescription;
};

class CUICommandImpl : public CUICommand
{
public:
	CUICommandImpl( CFunctor * on_selected, const char * name, const char * description )
		:	CUICommand( name, description )
		,	mOnSelected( on_selected )
	{
	}
	virtual ~CUICommandImpl()
	{
		delete mOnSelected;
	}

	virtual	void			OnSelected()			{ (*mOnSelected)(); }

private:
	CFunctor *	mOnSelected;
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

#endif	// UICOMMAND_H_
