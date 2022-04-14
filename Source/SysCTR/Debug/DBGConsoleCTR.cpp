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

#include "stdafx.h"
#include "Debug/DBGConsole.h"

#ifdef DAEDALUS_DEBUG_CONSOLE

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "Debug/DebugConsoleImpl.h"
#include "Test/BatchTest.h"

static const char * const kTerminalSaveCursor			= "\033[s";
static const char * const kTerminalRestoreCursor		= "\033[u";
static const char * const kTerminalEraseLine			= "\033[2K";


class IDebugConsole : public CDebugConsole
{
public:
	virtual void		Msg(u32 type, const char * format, ...);

	virtual void		MsgOverwriteStart();
	virtual void		MsgOverwrite(u32 type, const char * format, ...);
	virtual void		MsgOverwriteEnd();

private:
};

template<> bool	CSingleton< CDebugConsole >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = new IDebugConsole();

	return true;
}

CDebugConsole::~CDebugConsole()
{
}

void IDebugConsole::Msg(u32 type, const char * format, ...)
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	char * temp = NULL;
	va_list marker;
	va_start( marker, format );
	vprintf( format, marker );
	va_end( marker );
	printf("\n");

	if (temp)
		free(temp);
	#endif
}

void IDebugConsole::MsgOverwriteStart()
{
}

void IDebugConsole::MsgOverwrite(u32 type, const char * format, ...)
{
}

void IDebugConsole::MsgOverwriteEnd()
{
}

#endif // DAEDALUS_DEBUG_CONSOLE
