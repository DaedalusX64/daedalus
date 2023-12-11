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

#include "Config/ConfigOptions.h"
#include "Core/CPU.h"
#include "Core/ROM.h"				// ROM_Unload
#include "Core/RomSettings.h"
#include "Debug/DBGConsole.h"		// DBGConsole_Enable
#include "Debug/DebugLog.h"
#include "Interface/RomDB.h"
#include "System/SystemInit.h"
#include "Test/BatchTest.h"
#include "System/IO.h"
#include "Interface/Preferences.h"
#include "Utility/Profiler.h"		// CProfiler::Create/Destroy

#include "UI/UIContext.h"
#include "Graphics/GraphicsContext.h"
#include "UI/DrawText.h"
#include "UI/PauseScreen.h"
#include "UI/MainMenuScreen.h"

void HandleEndOfFrame()
{

}

int __cdecl main(int argc, char **argv)
{
	//ReadConfiguration();

	int result = 0;
	IO::Filename rom_path;

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
			#else
				fprintf(stderr, "BatchTest mode is not present in this build.\n");
			#endif
		}
		else if (filename)
		{
			//Need absolute path when loading from Visual Studio
			//This is ok when loading from console too, since arg0 will be empty, it'll just load file name (arg1)
			fprintf(stderr, "Loading %s\n",filename);
			System_Open(filename);
			CPU_Run();
			System_Close();
		}
	}

	bool show_splash = true;
	for (;;)
	{
		DisplayRomsAndChoose(show_splash);
		show_splash = false;

		CRomDB::Get()->Commit();
		CPreferences::Get()->Commit();

		CPU_Run();
		System_Close();
	}

	//
	// Write current config out to the registry
	//
	//WriteConfiguration();

	System_Finalize();


	return result;
}
