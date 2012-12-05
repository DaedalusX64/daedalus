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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef AUDIOPLUGIN_H__
#define AUDIOPLUGIN_H__

#include "Core/RSP_HLE.h"

class CAudioPlugin
{
public:
	virtual ~CAudioPlugin() {}

	virtual bool		StartEmulation() = 0;
	virtual void		StopEmulation() = 0;
//ToDo: Port to PC side of things...
	virtual void		AddBufferHLE(u8 *addr, u32 len) =0;

	enum ESystemType
	{
		ST_NTSC,
		ST_PAL,
		ST_MPAL,
	};

	virtual void			DacrateChanged( ESystemType system_type ) = 0;
	virtual void			LenChanged() = 0;
	virtual u32				ReadLength() = 0;
	virtual EProcessResult	ProcessAList() = 0;
	virtual void			RomClosed() = 0;
};

//
//	This needs to be defined for all targets.
//	The implementation can return NULL if audio is not supported
//
CAudioPlugin *		CreateAudioPlugin();
extern CAudioPlugin *		g_pAiPlugin;

#endif // AUDIOPLUGIN_H__
