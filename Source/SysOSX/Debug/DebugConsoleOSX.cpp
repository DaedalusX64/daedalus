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

#ifdef DAEDALUS_DEBUG_CONSOLE

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "Debug/DBGConsole.h"
#include "Debug/DebugConsoleImpl.h"

class IDebugConsole : public CDebugConsole
{
	public:
		IDebugConsole();
		virtual ~IDebugConsole()	{ }

		void Msg(u32 type, const char * format, ...);

		void MsgOverwriteStart() {}
		void MsgOverwrite(u32 type, const char * format, ...)
		{
			va_list marker;
			va_start( marker, format );
			vprintf( format, marker );
			va_end( marker );
			printf("\n");
		}
		void MsgOverwriteEnd() {}
};

template<> bool CSingleton< CDebugConsole >::Create()
{
	DAEDALUS_ASSERT(mpInstance == NULL, "Already initialised");

	mpInstance = new IDebugConsole();

	return mpInstance ? true : false;
}

CDebugConsole::~CDebugConsole()
{

}

IDebugConsole::IDebugConsole()
{
}


static size_t ParseFormatString(const char * format, char * out, size_t out_len)
{
	ETerminalColour tc = TC_DEFAULT;

	const char * p = format;
	char * o = out;

	size_t len = 0;
	while (*p)
	{
		if (*p == '[')
		{
			++p;

			tc = GetTerminalColour( *p );
			const char *	str 	= GetTerminalColourString(tc);
			size_t 			str_len = strlen(str);

			len += str_len;
			if (o)
			{
				strcpy(o, str);
				o += str_len;
			}
		}
		else if (*p == ']')
		{
			tc = TC_DEFAULT;
			const char * 	str 	= GetTerminalColourString(tc);
			size_t 			str_len = strlen(str);
			len += str_len;
			if (o)
			{
				strcpy(o, str);
				o += str_len;
			}
		}
		else
		{
			++len;
			if (o)
			{
				*o = *p;
				++o;
			}
		}

		++p;
	}

	if (tc != TC_DEFAULT)
	{
		tc = TC_DEFAULT;
		const char * 	str 	= GetTerminalColourString(tc);
		size_t 			str_len = strlen(str);
		len += str_len;
		if (o)
		{
			strcpy(o, str);
			o += str_len;
		}
	}

	if (o)
	{
		*o = '\0';
		++o;
		DAEDALUS_ASSERT( o == out+len+1, "Oops" );
	}

	return len;
}

void IDebugConsole::Msg(u32 type, const char * format, ...)
{
	char * temp = NULL;

	if (strchr(format, '[') != NULL)
	{
		size_t len = ParseFormatString(format, NULL, 0);

		temp = (char *)malloc(len+1);

		ParseFormatString(format, temp, len);

		format = temp;
	}


	va_list marker;
	va_start( marker, format );
	vprintf( format, marker );
	va_end( marker );
	printf("\n");

	if (temp)
		free(temp);
}
#endif
