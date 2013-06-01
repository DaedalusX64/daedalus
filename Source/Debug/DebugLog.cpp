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

#include "stdafx.h"

#include "DebugLog.h"
#include "Dump.h"
#include "DBGConsole.h"

#include "Utility/IO.h"

#ifdef DAEDALUS_LOG

//*****************************************************************************
//
//*****************************************************************************
static bool			g_bLog = false;
static FILE *		g_hOutputLog	= NULL;

//*****************************************************************************
//
//*****************************************************************************
bool Debug_InitLogging()
{
	IO::Filename log_filename;

	Dump_GetDumpDirectory(log_filename, "");

	IO::Path::Append(log_filename, "daedalus.txt");

#ifdef DAEDALUS_DEBUG_CONSOLE
	if ( CDebugConsole::IsAvailable() )
	{
		CDebugConsole::Get()->Msg( 0, "Creating Dump file '%s'", log_filename );
	}
#endif
	g_hOutputLog = fopen( log_filename, "w" );

	return g_hOutputLog != NULL;
}

//*****************************************************************************
//
//*****************************************************************************
void Debug_FinishLogging()
{
	if( g_hOutputLog )
	{
		fclose( g_hOutputLog );
		g_hOutputLog = NULL;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void Debug_Print( const char * format, ... )
{
	if(g_bLog && format != NULL )
	{
		char buffer[1024+1];
		char * p = buffer;
		va_list va;
		// Parse the buffer:
		// Format the output
		va_start(va, format);
		// Don't use wvsprintf as it doesn't handle floats!
		vsprintf(p, format, va);
		va_end(va);

		fprintf( g_hOutputLog, "%s\n", p );
	}
}

//*****************************************************************************
//
//*****************************************************************************
bool		Debug_GetLoggingEnabled()
{
	return g_bLog && (g_hOutputLog != NULL);
}

//*****************************************************************************
//
//*****************************************************************************
void		Debug_SetLoggingEnabled( bool enabled )
{
	g_bLog = enabled;
}


#endif // DAEDALUS_LOG
