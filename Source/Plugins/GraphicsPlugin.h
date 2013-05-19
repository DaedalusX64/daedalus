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

#pragma once

#ifndef PLUGINS_GRAPHICSPLUGIN_H_
#define PLUGINS_GRAPHICSPLUGIN_H_

class CGraphicsPlugin
{
	public:
		virtual ~CGraphicsPlugin();

		virtual bool		StartEmulation() = 0;

		virtual void		ViStatusChanged() = 0;
		virtual void		ViWidthChanged() = 0;
		virtual void		ProcessDList() = 0;

		virtual void		UpdateScreen() = 0;

		virtual void		RomClosed() = 0;
};

//
//	This needs to be defined for all targets.
//
CGraphicsPlugin *		CreateGraphicsPlugin();
extern CGraphicsPlugin * gGraphicsPlugin;

#endif // PLUGINS_GRAPHICSPLUGIN_H_
