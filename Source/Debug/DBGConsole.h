/*
Copyright (C) 2001 StrmnNrmn

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


#ifndef DBGCONSOLE_H_
#define DBGCONSOLE_H_

#include "Utility/Singleton.h"

#ifdef DAEDALUS_DEBUG_CONSOLE

class CDebugConsole : public CSingleton< CDebugConsole >
{
	public:
		virtual ~CDebugConsole();

		virtual void EnableConsole( bool bEnable ) = 0;
		virtual bool IsVisible() const = 0;

		virtual void UpdateDisplay() = 0;

		enum StatType
		{
			STAT_MIPS = 0,
			STAT_VIS,
			STAT_GEOM,
			STAT_SYNC,
			STAT_TEX,
			STAT_TLB,
			STAT_EXCEP,
			STAT_DYNAREC,
			STAT_TEXIGNORE,
			STAT_DLCULL,
			STAT_PI,
			STAT_SP,

			NUM_STAT_ITEMS,
		};

		virtual void DAEDALUS_VARARG_CALL_TYPE	Msg( u32 type, const char * format, ... ) = 0;

		virtual void							MsgOverwriteStart() = 0;
		virtual void DAEDALUS_VARARG_CALL_TYPE	MsgOverwrite( u32 type, const char * format, ... ) = 0;
		virtual void							MsgOverwriteEnd() = 0;

		virtual void DAEDALUS_VARARG_CALL_TYPE	Stats( StatType stat, const char * format, ... ) = 0;
};

#define DBGConsole_Msg( type, ... )			CDebugConsole::Get()->Msg( type, __VA_ARGS__ )

#else

#define DBGConsole_Msg(...)					do {} while(0)

#endif // DAEDALUS_DEBUG_CONSOLE

#endif // DBGCONSOLE_H_

