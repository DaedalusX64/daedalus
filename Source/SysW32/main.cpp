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

#include "Debug/DebugLog.h"
#include "Debug/DBGConsole.h"		// DBGConsole_Enable

#include "Core/ROM.h"				// ROM_Unload
#include "Core/CPU.h"
#include "Core/RomSettings.h"

#include "System.h"

#include "Interface/RomDB.h"

#include "Utility/ConfigHandler.h"	//
#include "Utility/Preferences.h"
#include "Utility/Profiler.h"		// CProfiler::Create/Destroy
#include "Utility/IO.h"

#include "Test/BatchTest.h"

#include "ConfigOptions.h"

char		gDaedalusExePath[MAX_PATH+1];

int __cdecl main(int argc, char **argv)
{
	int result = 0;
	if (argc > 0)
	{
		//ToDO: Implementation of realpath for W32
		IO::Path::RemoveFileSpec(argv[0]);
		strcpy(gDaedalusExePath, argv[0]);
		
	}
	else
	{
		fprintf(stderr, "Couldn't determine executable path\n");
		return 1;
	}
	
	//ReadConfiguration();

	if (!System_Init())
		return 1;

	if (argc > 1)
	{
		bool 			batch_test = false;
		const char *	filename   = NULL;

		for (int i = 1; i < argc; ++i)
		{
			const char * arg = argv[i];
			if (*arg == '-')
			{
				++arg;
				if( strcmp( arg, "-batch" ) == 0 )
				{
					batch_test = true;
					break;
				}
			}
			else
			{
				filename = arg;
			}
		}

		if (batch_test)
		{
			#ifdef DAEDALUS_BATCH_TEST_ENABLED
				BatchTestMain(argc, argv);
			#endif
		}
		else if (filename)
		{
			//Need absolute path when loading from Visual Studio
			strcat(argv[0], argv[1]);
			fprintf(stderr, "Loading %s\n",argv[0]);
			System_Open(argv[0]);
			CPU_Run();
			System_Close();
		}
	}
	else
	{
//		result = RunMain();
	}

	//
	// Write current config out to the registry
	//
	//WriteConfiguration();


	//
	// Turn off the debug console
	//
#ifdef DAEDALUS_DEBUG_CONSOLE
	CDebugConsole::Get()->EnableConsole( false );
#endif

	System_Finalize();

	return result;
}