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


#ifndef UI_SELECTEDROMCOMPONENT_H_
#define UI_SELECTEDROMCOMPONENT_H_

#include "UIComponent.h"
#include <functional>

class RomID;

class CSelectedRomComponent : public CUIComponent
{
	public:
		CSelectedRomComponent( CUIContext * p_context );
		virtual ~CSelectedRomComponent();

		static CSelectedRomComponent *	Create( CUIContext * p_context, std::function<void()> on_start_emulation );

		virtual void					SetRomID( const RomID & rom_id ) = 0;
};

#endif // UI_SELECTEDROMCOMPONENT_H_
