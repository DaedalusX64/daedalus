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


#include "Debug/DBGConsole.h"

#ifdef DAEDALUS_DEBUG_CONSOLE

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include "Debug/DebugConsoleImpl.h"
#include "Utility/BatchTest.h"

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
	DAEDALUS_ASSERT_Q(mpInstance == nullptr);

	mpInstance = std::make_shared<IDebugConsole>();

	return true;
}

CDebugConsole::~CDebugConsole()
{
}
void IDebugConsole::Msg(u32 type, const char* format, ...)
{
    // Buffer to hold formatted string
    char buffer[1024];
    
    // Initialize variable argument list
    va_list args;
    va_start(args, format);
    
    // Format the string
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    // Clean up variable argument list
    va_end(args);
    
    // Output the formatted string to std::cout
    std::cout << buffer << std::endl;
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
