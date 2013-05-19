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

#pragma once

#ifndef DEBUG_DBGCONSOLE_H_
#define DEBUG_DBGCONSOLE_H_

#include "Utility/DaedalusTypes.h"
#include "Utility/Singleton.h"
#include "Utility/Macros.h"

#ifdef DAEDALUS_DEBUG_CONSOLE

class CDebugConsole : public CSingleton< CDebugConsole >
{
	public:
		virtual ~CDebugConsole();

		virtual void DAEDALUS_VARARG_CALL_TYPE	Msg( u32 type, const char * format, ... ) = 0;

		virtual void							MsgOverwriteStart() = 0;
		virtual void DAEDALUS_VARARG_CALL_TYPE	MsgOverwrite( u32 type, const char * format, ... ) = 0;
		virtual void							MsgOverwriteEnd() = 0;
};

#define DBGConsole_Msg( type, ... )			CDebugConsole::Get()->Msg( type, __VA_ARGS__ )

#else

#define DBGConsole_Msg(...)					do { DAEDALUS_USE(__VA_ARGS__); } while(0)

#endif // DAEDALUS_DEBUG_CONSOLE

#endif // DEBUG_DBGCONSOLE_H_

