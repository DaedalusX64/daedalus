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

#include "stdafx.h"

#include <pspdebug.h>
#include <stdlib.h>
#include <stdio.h>

#include <pspctrl.h>
#include <psprtc.h>
#include <psppower.h>
#include <pspsdk.h>
#include <pspdisplay.h>
#include <pspgu.h>
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
#include "HLEGraphics/TextureCache.h"
#include "Input/InputManager.h"
#include "Interface/RomDB.h"
#include "SysPSP/Graphics/DrawText.h"
#include "SysPSP/UI/MainMenuScreen.h"
#include "SysPSP/UI/PauseScreen.h"
#include "SysPSP/UI/SplashScreen.h"
#include "SysPSP/UI/UIContext.h"
#include "SysPSP/Utility/Buttons.h"
#include "SysPSP/Utility/PathsPSP.h"
#include "System/Paths.h"
#include "System/System.h"
#include "Test/BatchTest.h"
#include "Utility/IO.h"
#include "Utility/ModulePSP.h"
#include "Utility/Preferences.h"
#include "Utility/Profiler.h"
#include "Utility/Thread.h"
#include "Utility/Translate.h"
#include "Utility/Timer.h"

/* Define to enable Exit Callback */
// Do not enable this, callbacks don't get along with our exit dialog :p
// Only needed for gprof
//
#ifdef DAEDALUS_PSP_GPROF
#define DAEDALUS_CALLBACKS
#else
#undef DAEDALUS_CALLBACKS
#endif


extern "C"
{
	/* Disable FPU exceptions */
	void _DisableFPUExceptions();

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


bool g32bitColorMode = false;
bool PSP_IS_SLIM = false;

PSP_MODULE_INFO( DaedalusX64 1.1.8, 0, 1, 1 );
PSP_MAIN_THREAD_ATTR( PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU );
PSP_HEAP_SIZE_KB(-256);


static void DaedalusFWCheck()
{
// ##define PSP_FIRMWARE Borrowed from Davee
#define PSP_FIRMWARE(f) ((((f >> 8) & 0xF) << 24) | (((f >> 4) & 0xF) << 16) | ((f & 0xF) << 8) | 0x10)

	u32 ver = sceKernelDevkitVersion();

	if( (ver < PSP_FIRMWARE(0x401)) )
	{
		pspDebugScreenInit();
		pspDebugScreenSetTextColor(0xffffff);
		pspDebugScreenSetBackColor(0x000000);
		pspDebugScreenSetXY(0, 0);
		pspDebugScreenClear();
		pspDebugScreenPrintf( "\n" );
		pspDebugScreenPrintf( "--------------------------------------------------------------------\n" );
		pspDebugScreenPrintf( "\n" );
		pspDebugScreenPrintf( "	Unsupported Firmware Detected : 0x%08X\n", ver );
		pspDebugScreenPrintf( "\n" );
		pspDebugScreenPrintf( "	Daedalus requires at least Firmware 4.01 M33\n" );
		pspDebugScreenPrintf( "\n" );
		pspDebugScreenPrintf( "--------------------------------------------------------------------\n" );
		sceKernelDelayThread(1000000);
		pspDebugScreenPrintf( "\n" );
		pspDebugScreenPrintf( "\n" );
		pspDebugScreenPrintf( "\n" );
		pspDebugScreenPrintf("\nPress O to Exit or [] to Ignore");
		for (;;)
		{
			SceCtrlData pad;
			sceCtrlPeekBufferPositive(&pad, 1);
			if (pad.Buttons & PSP_CTRL_CIRCLE)
				break;
			if (pad.Buttons & PSP_CTRL_SQUARE)
				return;
		}
		sceKernelExitGame();
	}

}


extern bool InitialiseJobManager();
//*************************************************************************************
//
//*************************************************************************************
static bool	Initialize()
{
	strcpy(gDaedalusExePath, DAEDALUS_PSP_PATH( "" ));

	scePowerSetClockFrequency(333, 333, 166);
	InitHomeButton();

	// If (o) is pressed during boot the Emulator will use 32bit
	// else use default 16bit color mode
	SceCtrlData pad;
	sceCtrlPeekBufferPositive(&pad, 1);
	if( pad.Buttons & PSP_CTRL_CIRCLE ) g32bitColorMode = true;
	else g32bitColorMode = false;

// Check for firmware lower than 4.01
	DaedalusFWCheck();

	// Initiate MediaEngine
	//Note: Media Engine is not available for Vita
	bool bMeStarted = InitialiseJobManager();

// Disable for profiling
//	srand(time(0));

	//Set the debug output to default
	if( g32bitColorMode ) pspDebugScreenInit();
	else pspDebugScreenInitEx( NULL , GU_PSM_5650, 1); //Sets debug output to 16bit mode

// This Breaks gdb, better disable it in debug build
//
#ifdef DAEDALUS_DEBUG_CONSOLE
extern void initExceptionHandler();
	initExceptionHandler();
#endif

	_DisableFPUExceptions();
	VolatileMemInit();

#ifdef DAEDALUS_CALLBACKS
	//Set up callback for our thread
	SetupCallbacks();
#endif

		// Detect PSP greater than PSP 1000
	if ( kuKernelGetModel() > 0 )
	{
		// Can't use extra memory if ME isn't available
		if( bMeStarted )
			PSP_IS_SLIM = true;

	int vitaprx = sceIoOpen("flash0:/kd/registry.prx", PSP_O_RDONLY | PSP_O_WRONLY, 0777);
	if(vitaprx >= 0){
	sceIoClose(vitaprx);

	}
	else {
		HAVE_DVE = CModule::Load("dvemgr.prx");
		if (HAVE_DVE >= 0)
			PSP_TV_CABLE = pspDveMgrCheckVideoOut();
		if (PSP_TV_CABLE == 1)
			PSP_TV_LACED = 1; // composite cable => interlaced
		else if( PSP_TV_CABLE == 0 )
			CModule::Unload( HAVE_DVE );	// Stop and unload dvemgr.prx since if no video cable is connected
		}
}

	HAVE_DVE = (HAVE_DVE < 0) ? 0 : 1; // 0 == no dvemgr, 1 == dvemgr

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

	// Init the savegame directory -- We probably should create this on the fly if not as it causes issues.
	strcpy( g_DaedalusConfig.mSaveDir, DAEDALUS_PSP_PATH( "SaveGames/" ) );
	strcpy( g_DaedalusConfig.mCacheDir, DAEDALUS_PSP_PATH( "Cache/" ) );
	if (!System_Init())
		return false;

	return true;
}


#ifdef DAEDALUS_PROFILE_EXECUTION
//*************************************************************************************
//
//*************************************************************************************
static void	DumpDynarecStats( float elapsed_time )
{
	// Temp dynarec stats
	extern u64 gTotalInstructionsEmulated;
	extern u64 gTotalInstructionsExecuted;
	extern u32 gTotalRegistersCached;
	extern u32 gTotalRegistersUncached;
	extern u32 gFragmentLookupSuccess;
	extern u32 gFragmentLookupFailure;

	u32		dynarec_ratio( 0 );

	if(gTotalInstructionsExecuted + gTotalInstructionsEmulated > 0)
	{
		float fRatio = float(gTotalInstructionsExecuted * 100.0f / float(gTotalInstructionsEmulated+gTotalInstructionsExecuted));

		dynarec_ratio = u32( fRatio );

		//gTotalInstructionsExecuted = 0;
		//gTotalInstructionsEmulated = 0;
	}

	u32		cached_regs_ratio( 0 );
	if(gTotalRegistersCached + gTotalRegistersUncached > 0)
	{
		float fRatio = float(gTotalRegistersCached * 100.0f / float(gTotalRegistersCached+gTotalRegistersUncached));

		cached_regs_ratio = u32( fRatio );
	}

	const char * const TERMINAL_SAVE_CURSOR			= "\033[s";
	const char * const TERMINAL_RESTORE_CURSOR		= "\033[u";
//	const char * const TERMINAL_TOP_LEFT			= "\033[2A\033[2K";
	const char * const TERMINAL_TOP_LEFT			= "\033[H\033[2K";

	printf( TERMINAL_SAVE_CURSOR );
	printf( TERMINAL_TOP_LEFT );

	printf( "Frame: %dms, DynaRec %d%%, Regs cached %d%%, Lookup success %d/%d", u32(elapsed_time * 1000.0f), dynarec_ratio, cached_regs_ratio, gFragmentLookupSuccess, gFragmentLookupFailure );

	printf( TERMINAL_RESTORE_CURSOR );
	fflush( stdout );

	gFragmentLookupSuccess = 0;
	gFragmentLookupFailure = 0;
}
#endif

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
#include "HLEGraphics/DLParser.h"
#include "HLEGraphics/DisplayListDebugger.h"
#endif
//*************************************************************************************
//
//*************************************************************************************
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



	//
	//	Figure out how long the last frame took
	//
#ifdef DAEDALUS_PROFILE_EXECUTION
	DumpDynarecStats( elapsed_time );
#endif
	//
	//	Enter the debug menu as soon as select is newly pressed
	//
	static u32 oldButtons = 0;
	SceCtrlData pad;
			bool		activate_pause_menu {false};
	sceCtrlPeekBufferPositive(&pad, 1);

	// If kernelbuttons.prx couldn't be loaded, allow select button to be used instead
	//
	if(oldButtons != pad.Buttons)
	{
		if( gCheatsEnabled && (pad.Buttons & PSP_CTRL_SELECT) )
		{
			CheatCodes_Activate( GS_BUTTON );
		}

		if(pad.Buttons & PSP_CTRL_HOME)
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
			// Already set in ClearBackground() @ UIContext.h
			//p_context->SetBackgroundColour( c32( 94, 188, 94 ) );		// Nice green :)

			CPauseScreen *	pause( CPauseScreen::Create( p_context ) );
			pause->Run();
			delete pause;
			delete p_context;
		}

		CDrawText::Destroy();

		//
		// Commit the preferences database before starting to run
		//
		CPreferences::Get()->Commit();
	}
	//
	//	Reset the elapsed time to avoid glitches when we restart
	//
#ifdef DAEDALUS_PROFILE_EXECUTION
	gTimer.Reset();
#endif

}

static void DisplayRomsAndChoose(bool show_splash)
{
	// switch back to the LCD display
	CGraphicsContext::Get()->SwitchToLcdDisplay();

	CDrawText::Initialise();

	CUIContext *	p_context( CUIContext::Create() );

	if(p_context != NULL)
	{

		if( show_splash )
		{
			CSplashScreen *		p_splash( CSplashScreen::Create( p_context ) );
			p_splash->Run();
			delete p_splash;
		}

		CMainMenuScreen *	p_main_menu( CMainMenuScreen::Create( p_context ) );
		p_main_menu->Run();
		delete p_main_menu;
	}

	delete p_context;

	CDrawText::Destroy();
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
		for(;;)
		{
			DisplayRomsAndChoose( show_splash );
			show_splash = false;

			CRomDB::Get()->Commit();
			CPreferences::Get()->Commit();

			CPU_Run();
			System_Close();
		}

		System_Finalize();
	}

	sceKernelExitGame();
	return 0;
}
