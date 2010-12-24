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
	char szLogFileName[MAX_PATH+1];

	Dump_GetDumpDirectory(szLogFileName, "");

	IO::Path::Append(szLogFileName, "daedalus.txt");

	if ( CDebugConsole::IsAvailable() )
	{
		CDebugConsole::Get()->Msg( 0, "Creating Dump file '%s'", szLogFileName );
	}
	g_hOutputLog = fopen( szLogFileName, "w" );

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
void Debug_Print( const char * szFormat, ... )
{
	if(g_bLog && szFormat != NULL )
	{
		char szBuffer[1024+1];
		char * pszBuffer = szBuffer;
		va_list va;
		// Parse the buffer:
		// Format the output
		va_start(va, szFormat);
		// Don't use wvsprintf as it doesn't handle floats!
		vsprintf(pszBuffer, szFormat, va);
		va_end(va);

		fprintf( g_hOutputLog, "%s\n", pszBuffer );
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
