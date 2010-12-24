/*
Copyright (C) 2006 StrmnNrmn

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


#ifndef ROMSELECTORCOMPONENT_H_
#define ROMSELECTORCOMPONENT_H_

#include "UIComponent.h"
#include "Utility/Functor.h"

class CRomSelectorComponent : public CUIComponent
{
	public:
		CRomSelectorComponent( CUIContext * p_context );
		virtual ~CRomSelectorComponent();

		static CRomSelectorComponent *	Create( CUIContext * p_context, CFunctor1< const char * > * on_rom_selected );
};

#endif	// ROMSELECTORCOMPONENT_H_
