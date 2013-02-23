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
#include "System.h"

#include "Debug/DBGConsole.h"
#include "Core/CPU.h"
#include "Utility/IO.h"
#include "Test/BatchTest.h"

char		gDaedalusExePath[MAX_PATH+1];

int main(int argc, char **argv)
{
	int result = 0;

	if (argc > 0)
	{
		char exe_path[PATH_MAX+1];
		realpath(argv[0], exe_path);

		strcpy(gDaedalusExePath, exe_path);
		IO::Path::RemoveFileSpec(gDaedalusExePath);
	}
	else
	{
		fprintf(stderr, "Couldn't determine executable path\n");
		return 1;
	}

	//ReadConfiguration();

	if (!System_Init())
		return 1;

	//
	// Create the console if it's enabled. Don't care about failures
	//
	//DisplayDisclaimer();
	//DisplayConfig();

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
			System_Open( argv[1] );
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
	CDebugConsole::Get()->EnableConsole( false );

	System_Finalize();

	return result;
}

//FIXME: All this stuff needs tidying

void Dynarec_ClearedCPUStuffToDo()
{
}

void Dynarec_SetCPUStuffToDo()
{
}


extern "C" {
void _EnterDynaRec()
{
	DAEDALUS_ASSERT(false, "Unimplemented");
}
}

