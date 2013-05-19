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
	void				ParseAndDisplayString( const char * p_string, ETerminalColour default_colour );

	void				DisplayString( const char * p_string );
	void				SetCurrentTerminalColour( ETerminalColour tc );


	char				mFormattingBuffer[ 2048 ];
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

void IDebugConsole::DisplayString( const char * p_string )
{
	printf( "%s", p_string );

#ifdef DAEDALUS_BATCH_TEST_ENABLED
	CBatchTestEventHandler * handler( BatchTest_GetHandler() );
	if( handler )
		handler->OnDebugMessage( p_string );
#endif

}

void IDebugConsole::SetCurrentTerminalColour( ETerminalColour tc )
{
	printf( GetTerminalColourString(tc) );
}

void IDebugConsole::ParseAndDisplayString( const char * p_string, ETerminalColour default_colour )
{
	SetCurrentTerminalColour( default_colour );

	char		current_string[ 1024 ];
	u32			out_idx( 0 );
	u32 string_len( strlen(p_string) );
	for ( u32 in_idx = 0; in_idx < string_len; ++in_idx )
	{
		if (p_string[in_idx] == '[')
		{
			ETerminalColour tc = GetTerminalColour(p_string[in_idx+1]);
			if(tc != TC_INVALID)
			{
				// Flush the current string and update the colour
				current_string[out_idx] = 0;
				DisplayString( current_string );
				out_idx = 0;
				SetCurrentTerminalColour( tc );
			}
			else
			{
				switch (p_string[in_idx+1])
				{
				case '[':
				case ']':
					current_string[out_idx] = p_string[in_idx+1];
					out_idx++;
					break;
				}
			}

			// Skip colour character
			in_idx++;
		}
		else if (p_string[in_idx] == ']')
		{
			// Flush the current string and update the colour
			current_string[out_idx] = 0;
			DisplayString( current_string );
			out_idx = 0;
			SetCurrentTerminalColour( default_colour );
		}
		else
		{
			current_string[out_idx] = p_string[in_idx];
			out_idx++;
		}
	}

	// Flush the current string and restore the colour
	current_string[out_idx] = 0;
	DisplayString( current_string );

	SetCurrentTerminalColour( TC_DEFAULT );
}

void IDebugConsole::Msg(u32 type, const char * format, ...)
{
	va_list			va;

	// Format the output
	va_start( va, format );
	// Don't use wvsprintf as it doesn't handle floats!
	vsprintf( mFormattingBuffer, format, va );
	va_end( va );

	ParseAndDisplayString( mFormattingBuffer, TC_w );
	DisplayString( "\n" );
}

void IDebugConsole::MsgOverwriteStart()
{
	printf( kTerminalSaveCursor );
}

void IDebugConsole::MsgOverwrite(u32 type, const char * format, ...)
{
	va_list			va;

	// Format the output
	va_start( va, format );
	// Don't use wvsprintf as it doesn't handle floats!
	vsprintf( mFormattingBuffer, format, va );
	va_end( va );

	printf( "%s%s", kTerminalEraseLine, kTerminalRestoreCursor );

	ParseAndDisplayString( mFormattingBuffer, TC_w );
}

void IDebugConsole::MsgOverwriteEnd()
{
	printf( "\n" );		// Final newline for the terminal
}

#endif // DAEDALUS_DEBUG_CONSOLE
