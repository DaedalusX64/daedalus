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
#include "Utility/BatchTest.h"
#include "Graphics/GraphicsContext.h"
#include "Interface/ConfigOptions.h"
#include "Interface/Preferences.h"
#include "Utility/Translate.h"
#include "UI/MainMenuScreen.h"

#include <SDL2/SDL.h>
#include <vector>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <string_view>

#ifdef DAEDALUS_LINUX
#include <linux/limits.h>
#endif

#include "UI/UIContext.h"

#include "UI/DrawText.h"
#include "UI/PauseScreen.h"

#ifdef DAEDALUS_PROFILE_EXECUTION
static CTimer gTimer;
#endif
bool isRunning = false;
bool gFullScreenMode = false;

void HandleEndOfFrame()
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
#include "HLEGraphics/DisplayListDebugger.h"
#include "Debug/DebugLog.h"
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
	if(oldButtons != pad.Buttons)
	{
		// if( gCheatsEnabled && (pad.Buttons & PSP_CTRL_SELECT) )
		// {
		// 	CheatCodes_Activate( GS_BUTTON );
		// }

		if(pad.Buttons & PSP_CTRL_SELECT)
				activate_pause_menu = true;
	}


	if (activate_pause_menu && isRunning == true)
	{

		CGraphicsContext::Get()->SwitchToLcdDisplay();
		CGraphicsContext::Get()->ClearAllSurfaces();

		auto p_context = CUIContext::Create();

		if (p_context != NULL)
		{
			auto pause = CPauseScreen::Create(p_context);
			pause->Run();
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
	bool batch_test = false;
	const char* filename = nullptr;

	// Stage 1: Parse arguments
	for (int i = 1; i < argc; ++i)
	{
		std::string_view arg(argv[i]);

		if (arg == "--batch")
			batch_test = true;
		else if (arg == "--fullscreen")
			gFullScreenMode = true;
		else if (arg == "--roms")
		{
			if (i + 1 < argc)
			{
				const char* relative_path = argv[++i];
				try
				{
					std::filesystem::path dir = std::filesystem::absolute(relative_path);
					CRomDB::Get()->AddRomDirectory(dir.string().c_str());
				}
				catch (const std::filesystem::filesystem_error& e)
				{
					std::cerr << "Error resolving path: " << e.what() << std::endl;
				}
			}
			else
			{
				std::cerr << "--roms requires a directory argument\n";
			}
		}
		else if (arg.starts_with('-'))
		{
			std::cerr << "Unknown option: " << arg << '\n';
		}
		else if (!filename)
		{
			filename = argv[i];
		}
	}

	// Stage 2: Init AFTER flags are parsed
	if (!System_Init())
	{
		fprintf(stderr, "System_Init failed\n");
		return 1;
	}

	// Now fullscreen mode has been set before System_Init()

	if (batch_test)
	{
#ifdef DAEDALUS_BATCH_TEST_ENABLED
		BatchTestMain(argc, argv);
#else
		fprintf(stderr, "BatchTest mode is not present in this build.\n");
#endif
		return 0;
	}

	if (filename)
	{
		if (filename[0] == '-')
		{
			fprintf(stderr, "Invalid ROM filename: %s\n", filename);
			return 1;
		}

		if (!System_Open(filename))
		{
			fprintf(stderr, "System_Open failed for %s\n", filename);
			return 1;
		}

		CRomDB::Get()->Commit();
		CPreferences::Get()->Commit();

		CPU_Run();
		System_Close();
		return 0;
	}

	// Menu loop
	Translate_Init();
	bool show_splash = true;
	for (;;)
	{
		DisplayRomsAndChoose(show_splash);
		show_splash = false;
		CPreferences::Get()->Commit();
		isRunning = true;
		CPU_Run();
		System_Close();
	}

	System_Finalize();
	return 0;
}