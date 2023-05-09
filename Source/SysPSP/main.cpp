/*
Copyright (C) 2006,2007 StrmnNrmn

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

#include "BuildOptions.h"
#include "Base/Types.h"

#include <pspdebug.h>
#include <stdlib.h>
#include <stdio.h>

#include <pspctrl.h>
#include <psprtc.h>
#include <psppower.h>
#include <pspsdk.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspge.h>
#include <pspfpu.h>
#include <pspkernel.h>
#include <kubridge.h>
#include <pspsysmem.h>

#include "Config/ConfigOptions.h"
#include "Core/Cheats.h"
#include "Core/CPU.h"
#include "Core/CPU.h"
#include "Core/Memory.h"
#include "Core/PIF.h"
#include "Core/RomSettings.h"
#include "Core/Save.h"
#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"
#include "Graphics/GraphicsContext.h"
#include "HLEGraphics/DLParser.h"
#include "HLEGraphics/DisplayListDebugger.h"
#include "HLEGraphics/TextureCache.h"
#include "Input/InputManager.h"
#include "Interface/RomDB.h"
#include "SysPSP/Graphics/DrawText.h"
#include "SysPSP/UI/MainMenuScreen.h"
#include "SysPSP/UI/PauseScreen.h"
#include "SysPSP/UI/SplashScreen.h"
#include "SysPSP/UI/UIContext.h"


#include "System/SystemInit.h"
#include "Test/BatchTest.h"
#include "System/IO.h"
#include "Utility/ModulePSP.h"
#include "Interface/Preferences.h"
#include "Utility/Profiler.h"
#include "System/Thread.h"
#include "SysPSP/Utility/Translate.h"
#include "Utility/Timer.h"


#include <filesystem>

#ifdef DAEDALUS_SDL
#include <SDL2/SDL.h>
#else

PSP_MODULE_INFO( DaedalusX64 1.1.9a, 0, 1, 1 );
PSP_MAIN_THREAD_ATTR( PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU );
#endif


extern "C"
{

	/* Video Manager functions */
	int pspDveMgrCheckVideoOut();
	int pspDveMgrSetVideoOut(int, int, int, int, int, int, int);

#ifdef DAEDALUS_PSP_GPROF
	/* Profile with psp-gprof */
	void gprof_cleanup();
#endif
}

/* Kernel Exception Handler functions */

extern void VolatileMemInit();

/* Video Manager functions */
extern int HAVE_DVE;
extern int PSP_TV_CABLE;
extern int PSP_TV_LACED;

bool g32bitColorMode = false; // We might be able to ditch this soon. Phat exclusive.
bool PSP_IS_SLIM = false;

PSP_HEAP_SIZE_KB(-256);



static bool	Initialize()
{
	//  strcpy(gDaedalusExePath, DAEDALUS_PSP_PATH( "" ));

	scePowerSetClockFrequency(333, 333, 166);
	// InitHomeButton();
	#ifdef DAEDALUS_SDL
		if( SDL_Init( SDL_INIT_AUDIO ) < 0 )
	{
		printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
		return false;
	}
	#endif
// This Breaks gdb, better disable it in debug build
//
#ifdef DAEDALUS_DEBUG_CONSOLE
extern void initExceptionHandler();
	initExceptionHandler();
#endif

	pspFpuSetEnable(0); // Disable PFU Exceptions
	VolatileMemInit();


	// Detect PSP greater than PSP 1000
	if ( kuKernelGetModel() > 0 )
	{
		// Can't use extra memory if ME isn't available
		PSP_IS_SLIM = true;
		g32bitColorMode = true;
		sceGeEdramSetSize(4*1024*1024);
		HAVE_DVE = CModule::Load("Plugins/dvemgr.prx");
		if (HAVE_DVE >= 0)
			PSP_TV_CABLE = pspDveMgrCheckVideoOut();
		if (PSP_TV_CABLE == 1)
			PSP_TV_LACED = 1; // composite cable => interlaced
		else if( PSP_TV_CABLE == 0 )
			CModule::Unload( HAVE_DVE );	// Stop and unload dvemgr.prx since if no video cable is connected
	}


	//Set the debug output to default
	if( g32bitColorMode ) pspDebugScreenInit();
	else pspDebugScreenInitEx( NULL , GU_PSM_5650, 1); //Sets debug output to 16bit mode

	HAVE_DVE = (HAVE_DVE < 0) ? 0 : 1; // 0 == no dvemgr, 1 == dvemgr

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);


	if (!System_Init())
		return false;

	return true;
}


#ifdef DAEDALUS_PROFILE_EXECUTION
static CTimer		gTimer;
#endif

void HandleEndOfFrame()
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if(DLDebugger_IsDebugging())
		return;
			DPF( DEBUG_FRAME, "********************************************" );
#endif



// How long did the last frame take?
#ifdef DAEDALUS_PROFILE_EXECUTION
	DumpDynarecStats( elapsed_time );
#endif

	//Enter debug menu as soon as select is pressed
	static u32 oldButtons = 0;
	SceCtrlData pad;
			bool		activate_pause_menu = false;
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

	if(activate_pause_menu)
	{

		CGraphicsContext::Get()->SwitchToLcdDisplay();
		CGraphicsContext::Get()->ClearAllSurfaces();

		CDrawText::Initialise();

		CUIContext *	p_context( CUIContext::Create() );

		if(p_context != NULL)
		{
			CPauseScreen *	pause( CPauseScreen::Create( p_context ) );
			pause->Run();
			delete pause;
			delete p_context;
		}

		CDrawText::Destroy();

		// Commit the preferences database before starting to run
		// CPreferences::Get()->Commit();
	}

	//	Reset the elapsed time to avoid glitches when we restart
	#ifdef DAEDALUS_PROFILE_EXECUTION
	gTimer.Reset();
	#endif

}


int main(int argc, char* argv[])
{
	if( Initialize() )
	{
#ifdef DAEDALUS_BATCH_TEST_ENABLED
		if( argc > 1 )
		{
			BatchTestMain( argc, argv );
		}
#else
		//Makes it possible to load a ROM directly without using the GUI
		//There are no checks for wrong file name so be careful!!!
		//Ex. from PSPLink -> ./Daedalus.prx "Roms/StarFox 64.v64" //Corn
		if( argc > 1 )
		{
			printf("Loading %s\n", argv[1] );
			System_Open( argv[1] );
			CPU_Run();
			System_Close();
			System_Finalize();
			sceKernelExitGame();
			return 0;
		}
#endif
		//Translate_Init();
		bool show_splash = true;
		std::filesystem::path filename;

		for(;;)
		{
						CRomDB::Get()->RomIndex(filename);
			
			DisplayRomsAndChoose( show_splash );
			show_splash = false;
			

			// CPreferences::Get()->Commit();

			CPU_Run();
			System_Close();
		}

		System_Finalize();
	}

	sceKernelExitGame();
	return 0;
}
