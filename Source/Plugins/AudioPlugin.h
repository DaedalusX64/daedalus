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

#ifndef PLUGINS_AUDIOPLUGIN_H_
#define PLUGINS_AUDIOPLUGIN_H_

#include "Core/RSP_HLE.h"

class CAudioPlugin
{
public:
	virtual ~CAudioPlugin() {}

	virtual bool		StartEmulation() = 0;
	virtual void		StopEmulation() = 0;

	enum ESystemType
	{
		ST_NTSC,
		ST_PAL,
		ST_MPAL,
	};

	virtual void			DacrateChanged( int SystemType ) = 0;
	virtual void			LenChanged() = 0;
	virtual u32				ReadLength() = 0;
	virtual EProcessResult	ProcessAList() = 0;
#ifdef DAEDALUS_W32
	virtual void			Update( bool wait ) = 0;
#endif
};

//
//	This needs to be defined for all targets.
//	The implementation can return NULL if audio is not supported
//
CAudioPlugin *			CreateAudioPlugin();
extern CAudioPlugin *	gAudioPlugin;

#endif // PLUGINS_AUDIOPLUGIN_H_
