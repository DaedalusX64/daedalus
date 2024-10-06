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


#ifndef UI_UISETTING_H_
#define UI_UISETTING_H_

#include "UIElement.h"


class CUISetting : public CUIElement
{
public:
	CUISetting( const std::string name, const std::string description );
	virtual ~CUISetting() {}

	virtual const char *	GetSettingName() const = 0;

	virtual	void			OnSelected()			{ OnNext(); }

	virtual const std::string	GetName() const			{ return mName; }
	virtual const std::string	GetDescription() const	{ return mDescription; }

	virtual bool			IsReadOnly() const		{ return false; }
	//virtual bool			IsSelectable() const	{ return !IsReadOnly(); }		// We actually want to be able to read the descriptions of various settings, so don't do this.

	virtual u32				GetHeight( CUIContext * context ) const;
	virtual void			Draw( CUIContext * context, s32 min_x, s32 max_x, EAlignType halign, s32 y, bool selected ) const;

private:
	const std::string	mName;
	const std::string		mDescription;
};


class CBoolSetting : public CUISetting
{
public:

	CBoolSetting( bool * p_bool, const std::string name, const std::string description, const char * true_text, const char * false_text, bool read_only = false )
		:	CUISetting( name, description )
		,	mBool( p_bool )
		,	mTrueText( true_text )
		,	mFalseText( false_text )
		,	mReadOnly( read_only )
	{
	}

	virtual	void			OnNext()				{ *mBool = !*mBool; }
	virtual	void			OnPrevious() 			{ *mBool = !*mBool; }

	virtual bool			IsReadOnly() const		{ return mReadOnly; }

	virtual const char *	GetSettingName() const	{ return *mBool ? mTrueText : mFalseText; }

private:
	bool *					mBool;
	const char *			mTrueText;
	const char *			mFalseText;
	bool					mReadOnly;
};

#endif // UI_UISETTING_H_
