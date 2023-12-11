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

#include <windows.h>
#include "Base/Types.h"
#include "Base/Assert.h"

#ifdef DAEDALUS_ENABLE_ASSERTS
#include <crtdbg.h>
#include <varargs.h>
#include <stdio.h>

DaedalusAssertHook gAssertHook = NULL;

#ifdef _DEBUG

EAssertResult DAEDALUS_VARARG_CALL_TYPE DaedalusAssert( const char * expression, const char * file, unsigned int line, const char * msg, ... )
{
	va_list va;
	va_start(va, msg);

	char buffer[ 1024 ];
	vsnprintf( buffer, 1024, msg, va );
	buffer[1023] = 0;
	va_end(va);

	// Lkb: I suspect this won't be defined when compiling for GNU C - can you suggest an alterative? StrmnNrmn

	// Returns 1 for retry
	// Returns 0 for ignore
	int r( _CrtDbgReport(_CRT_ASSERT, file, line, "Daedalus.exe", "%s\nMessage: %s", expression, buffer ) );

	switch( r )
	{
		case 0:		return AR_IGNORE;
		case 1:		return AR_BREAK;
	}
	return AR_IGNORE_ONCE;
}

#else

EAssertResult DAEDALUS_VARARG_CALL_TYPE DaedalusAssert( const char * expression, const char * file, unsigned int line, const char * msg, ... )
{
	va_list va;
	va_start(va, msg);

	char buffer[ 1024 ];
	vsnprintf( buffer, 1024, msg, va );
	buffer[1023] = 0;
	va_end(va);

	char text[1024];

	sprintf( text, "Debug Assertion Failed!\n"
					"\n"
					"File: %s\n"
					"Line: %d\n"
					"\n"
					"Expression: %s\n"
					"Message: %s\n"
					"\n"
					"(Press Retry to debug the application)",
					file,
					line,
					expression,
					buffer );

	int ret = ::MessageBoxA( NULL, text, "Assert", MB_ABORTRETRYIGNORE | MB_ICONERROR );

	if ( ret == IDIGNORE )
		return AR_IGNORE;
	if ( ret == IDRETRY )
		return AR_BREAK;

	exit( 1 );
}
#endif		// _DEBUG
#endif		// DAEDALUS_ENABLE_ASSERTS
