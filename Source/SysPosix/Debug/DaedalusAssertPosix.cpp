/*

  Copyright (C) 2012 StrmnNrmn

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
#include "Base/Assert.h"

#ifdef DAEDALUS_ENABLE_ASSERTS

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

DaedalusAssertHook gAssertHook = NULL;

EAssertResult DaedalusAssert( const char * expression, const char * file, unsigned int line, const char * msg, ... )
{
	char buffer[ 1024 ];
	va_list va;
	va_start(va, msg);
	vsnprintf( buffer, 1024, msg, va );
	buffer[1023] = 0;
	va_end(va);

	printf( "************************************************************\n" );
	printf( "Assert Failed: %s\n", expression );
	printf( "Location: %s(%d)\n", file, line );
	printf( "\n" );
	printf( "%s\n", buffer );
	printf( "\n" );

	bool done = false;
	while (!done)
	{
		printf( "a: abort, b: break, c: continue, i: ignore\n" );
		switch (getchar())
		{
			case 'a': abort(); return AR_BREAK;		// Should be unreachable.
			case 'b': return AR_BREAK;
			case 'c': return AR_IGNORE_ONCE;
			case 'i': return AR_IGNORE;
		}
	}

	return AR_IGNORE;
}

#endif //DAEDALUS_ENABLE_ASSERTS
