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

#include "Base/Types.h"

#include "Core/CPU.h"
#include "Debug/DBGConsole.h"
#include "Interface/RomDB.h"
#include "System/SystemInit.h"
#include "Test/BatchTest.h"
#include "System/IO.h"
#include "Config/ConfigOptions.h"
#include "Interface/Preferences.h"
#include "Utility/Translate.h"
#include "UI/MainMenuScreen.h"

#include <SDL2/SDL.h>
#include <vector>
#include <filesystem>
#include <iostream>
#include <algorithm>

#ifdef DAEDALUS_LINUX
#include <linux/limits.h>
#endif

#include "UI/UIContext.h"
#include "Graphics/GraphicsContext.h"
#include "UI/DrawText.h"
#include "UI/PauseScreen.h"

#ifdef DAEDALUS_PROFILE_EXECUTION
static CTimer gTimer;
#endif

void HandleEndOfFrame()
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (DLDebugger_IsDebugging())
		return;
	DPF(DEBUG_FRAME, "********************************************");
#endif

// How long did the last frame take?
#ifdef DAEDALUS_PROFILE_EXECUTION
	DumpDynarecStats(elapsed_time);
#endif

	// Enter debug menu as soon as select is pressed
	static u32 oldButtons = 0;
	SceCtrlData pad;
	bool activate_pause_menu = false;
	sceCtrlPeekBufferPositive(&pad, 1);

	// If KernelButtons.prx not found. Use select for pause instead
	if (oldButtons != pad.Buttons)
	{
		// if( gCheatsEnabled && (pad.Buttons & PSP_CTRL_SELECT) )
		// {
		// 	CheatCodes_Activate( GS_BUTTON );
		// }

		if (pad.Buttons & PSP_CTRL_SELECT)
			activate_pause_menu = true;
	}

	if (activate_pause_menu)
	{

		CGraphicsContext::Get()->SwitchToLcdDisplay();
		CGraphicsContext::Get()->ClearAllSurfaces();

		CUIContext *p_context(CUIContext::Create());

		if (p_context != NULL)
		{
			CPauseScreen *pause(CPauseScreen::Create(p_context));
			pause->Run();
			delete pause;
			delete p_context;
		}

		// Commit the preferences database before starting to run
		// CPreferences::Get()->Commit();
	}

//	Reset the elapsed time to avoid glitches when we restart
#ifdef DAEDALUS_PROFILE_EXECUTION
	gTimer.Reset();
#endif
}

int main(int argc, char **argv)
{
	int result = 0;

	// ReadConfiguration();

	if (!System_Init())
	{
		fprintf(stderr, "System_Init failed\n");
		return 1;
	}

	Translate_Init();

	if (argc > 1)
	{
		bool batch_test = false;
		const char *filename = NULL;

		for (int i = 1; i < argc; ++i)
		{
			const char *arg = argv[i];
			if (*arg == '-')
			{
				++arg;
				if (strcmp(arg, "-batch") == 0)
				{
					batch_test = true;
					break;
				}
				else if (strcmp(arg, "-roms") == 0)
				{
					if (i + 1 < argc)
					{
						const char *relative_path = argv[i + 1];
						++i;

						char *dir = realpath(relative_path, nullptr);
						CRomDB::Get()->AddRomDirectory(dir);
						free(dir);
					}
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
			System_Open(filename);

			//
			// Commit the preferences and roms databases before starting to run
			//
			CRomDB::Get()->Commit();
			CPreferences::Get()->Commit();

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

	System_Finalize();
	return result;
}
