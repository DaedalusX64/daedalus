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


#ifndef SYSPSP_UI_UICOMPONENT_H_
#define SYSPSP_UI_UICOMPONENT_H_

#include "Base/Types.h"

class v2;
class CUIContext;

class CUIComponent
{
	public:
		CUIComponent( CUIContext * p_context );
		virtual ~CUIComponent();

		virtual void				Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons ) = 0;
		virtual void				Render() = 0;
		virtual bool				IsFinished() const			{ return true; }

	protected:
		CUIContext *				mpContext;
};

//
//	A simple wrapper around a component
//
#include "UIScreen.h"
#include <string>

class CUIComponentScreen : public CUIScreen
{
	private:
		CUIComponentScreen( CUIContext * p_context, CUIComponent * component, const char * title );
	public:
		virtual ~CUIComponentScreen();

		static CUIComponentScreen *	Create( CUIContext * p_context, CUIComponent * component, const char * title );

		// CUIScreen
		virtual void				Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();
		virtual bool				IsFinished() const;

	private:
		CUIComponent *				mComponent;
		std::string					mTitle;
};

#endif // SYSPSP_UI_UICOMPONENT_H_
