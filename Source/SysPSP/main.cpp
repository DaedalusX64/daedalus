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

#include "System.h"
#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"
#include "Interface/RomDB.h"
#include "Core/RomSettings.h"
#include "Core/Memory.h"
#include "Core/CPU.h"
#include "Core/PIF.h"
#include "Core/CPU.h"
#include "Core/Save.h"
#include "Input/InputManager.h"
#include "Utility/Profiler.h"
#include "Utility/IO.h"
#include "Utility/Preferences.h"
#include "Utility/Timer.h"
#include "Utility/Thread.h"

#include "Graphics/GraphicsContext.h"

#include "SysPSP/UI/UIContext.h"
#include "SysPSP/UI/MainMenuScreen.h"
#include "SysPSP/UI/PauseScreen.h"
#include "SysPSP/UI/SplashScreen.h"
#include "SysPSP/Graphics/DrawText.h"
#include "SysPSP/Utility/PathsPSP.h"
#include "SysPSP/Utility/Buttons.h"

#include "HLEGraphics/TextureCache.h"

#include "Test/BatchTest.h"

#include "ConfigOptions.h"

/* Undef to kill Exit Callback */
//#define DAEDALUS_CALLBACKS

char gDaedalusExePath[MAX_PATH+1] = DAEDALUS_PSP_PATH( "" );

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
extern void initExceptionHandler();

/* Video Manager functions */
extern int HAVE_DVE;
extern int PSP_TV_CABLE;
extern int PSP_TV_LACED;


bool PSP_IS_SLIM = false;
//*************************************************************************************
//Set up our initial eviroment settings for the PSP
//*************************************************************************************
PSP_MODULE_INFO( DaedalusX64 Alpha, 0, 1, 1 );
PSP_MAIN_THREAD_ATTR( PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU );
//PSP_HEAP_SIZE_KB(20000);// Set Heapsize to 18.5mb
PSP_HEAP_SIZE_KB(-256);

//*************************************************************************************
//Used to check for compatible FW, we don't allow anything lower than 4.01
//*************************************************************************************
static void DaedalusFWCheck()
{
	u32 ver = sceKernelDevkitVersion();

	//if(ver < 0x05050010)
	if(ver < 0x04000110)
	{
		pspDebugScreenInit();
		pspDebugScreenSetTextColor(0xffffff);
		pspDebugScreenSetBackColor(0x000000);
		pspDebugScreenSetXY(0, 0);
		pspDebugScreenClear();
		pspDebugScreenPrintf( "\n" );
		pspDebugScreenPrintf("XXXXXXX       XXXXXXX        66666666         444444444\n" );  
		pspDebugScreenPrintf("X:::::X       X:::::X       6::::::6         4::::::::4\n" );  
		pspDebugScreenPrintf("X:::::X       X:::::X      6::::::6         4:::::::::4\n" );  
		pspDebugScreenPrintf("X::::::X     X::::::X     6::::::6         4::::44::::4\n" );  
		pspDebugScreenPrintf("XXX:::::X   X:::::XXX    6::::::6         4::::4 4::::4\n" );  
		pspDebugScreenPrintf("   X:::::X X:::::X      6::::::6         4::::4  4::::4\n" );  
		pspDebugScreenPrintf("    X:::::X:::::X      6::::::6         4::::4   4::::4\n" );  
		pspDebugScreenPrintf("     X:::::::::X      6::::::::66666   4::::444444::::444\n" );
		pspDebugScreenPrintf("     X:::::::::X     6::::::::::::::66 4::::::::::::::::4\n" );
		pspDebugScreenPrintf( "   X:::::X:::::X    6::::::66666:::::64444444444:::::444\n" );
		pspDebugScreenPrintf("   X:::::X X:::::X   6:::::6     6:::::6         4::::4\n" );  
		pspDebugScreenPrintf("XXX:::::X   X:::::XXX6:::::6     6:::::6         4::::4\n" );  
		pspDebugScreenPrintf("X::::::X     X::::::X6::::::66666::::::6         4::::4\n" );  
		pspDebugScreenPrintf("X:::::X       X:::::X 66:::::::::::::66        44::::::44\n" );
		pspDebugScreenPrintf("X:::::X       X:::::X   66:::::::::66          4::::::::4\n" );
		pspDebugScreenPrintf("XXXXXXX       XXXXXXX     666666666            4444444444\n" );
		pspDebugScreenPrintf( "\n" );
		pspDebugScreenPrintf( "\n" );
		pspDebugScreenPrintf( "--------------------------------------------------------------------\n" );
		pspDebugScreenPrintf( "\n" );
		pspDebugScreenPrintf( "	Unsupported Firmware Detected : 0x%08X\n", ver );
		pspDebugScreenPrintf( "\n" );
		pspDebugScreenPrintf( "	Daedalus requires atleast 4.01 M33 Custom Firmware\n" );
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
#ifdef DAEDALUS_CALLBACKS
//*************************************************************************************
//Set up the Exit Callback (Used to allow the Home Button to work)
//*************************************************************************************
static int ExitCallback( int arg1, int arg2, void * common )
{
#ifdef DAEDALUS_PSP_GPROF
	gprof_cleanup();
#endif
	sceKernelExitGame();
	return 0;
}

//*************************************************************************************
//Initialise the Exit Callback (Also used to allow the Home Button to work)
//*************************************************************************************
static int CallbackThread( SceSize args, void * argp )
{
	int cbid;

	cbid = sceKernelCreateCallback( "Exit Callback", ExitCallback, NULL );
	sceKernelRegisterExitCallback( cbid );

	sceKernelSleepThreadCB();

	return 0;
}

//*************************************************************************************
// Sets up the callback thread and returns its thread id
//*************************************************************************************
static int SetupCallbacks()
{
	int thid = 0;

	thid = sceKernelCreateThread( "CallbackThread", CallbackThread, 0x11, 0xFA0, PSP_THREAD_ATTR_USER, 0 );
	if(thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}
	return thid;
}
#endif

//*************************************************************************************
//Panic button thread quits to the menu when pressed
//*************************************************************************************
static int PanicThread( SceSize args, void * argp )
{
	const u32 MASK = PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_START;

	u32 count = 0;

	//Loop 4 ever
	while(1)
	{
		SceCtrlData pad;
		sceCtrlPeekBufferPositive(&pad, 1); 

		// Start reading our buttons
		DaedalusReadButtons( pad.Buttons );

		if( (pad.Buttons & MASK) == MASK )
		{
			if(++count > 30 )		//If button presse for more that 2sec we return to main menu
			{
				count = 0;
				CGraphicsContext::Get()->ClearAllSurfaces();
				CPU_Halt("Panic");
			}
		}
		else count = 0;

		//Idle here, only check button 13.5 times/sec not to hog CPU time from EMU
		ThreadSleepMs(77);	
	}

	return 0;
}

//*************************************************************************************
//
//*************************************************************************************
static int SetupPanic()
{
	int thidf = sceKernelCreateThread( "PanicThread", PanicThread, 0x11, 0xFA0, PSP_THREAD_ATTR_USER, 0 );

	if(thidf >= 0)
	{
		sceKernelStartThread(thidf, 0, 0);
	}

	return 0;
}

extern void InitialiseJobManager();
//*************************************************************************************
//
//*************************************************************************************
static bool	Initialize()
{
	// Check for unsupported FW
	DaedalusFWCheck();

	printf( "Cpu was: %dMHz, Bus: %dMHz\n", scePowerGetCpuClockFrequency(), scePowerGetBusClockFrequency() );
	if (scePowerSetClockFrequency(333, 333, 166) != 0)
	{
		printf( "Could not set CPU to 333MHz\n" );
	}
	printf( "Cpu now: %dMHz, Bus: %dMHz\n", scePowerGetCpuClockFrequency(), scePowerGetBusClockFrequency() );

	InitialiseJobManager();

// Disable for profiling
//	srand(time(0));

	//Set the debug output to default
#ifndef DAEDALUS_SCRN_16BIT
	pspDebugScreenInit();
#else
	pspDebugScreenInitEx( NULL , GU_PSM_5650, 1); //Sets debug output to 16bit mode
#endif

	initExceptionHandler();

	_DisableFPUExceptions();

	// Set up our Kernel Buttons : Home, Vol- +, etc
	InitButtons();

	// Unset home button and imposed to allow use it as normal button
	SetImposeHomeButton(0x10000, 0); 

	// Force non-kernelbuttons when profiling
#ifdef DAEDALUS_PSP_GPROF
	gButtons.mode = false;
#endif

	//Init Panic button thread
	SetupPanic();

	// We have to Callbacks if kernelbuttons.prx failed
	if( gButtons.mode == false )
	{
#ifdef DAEDALUS_CALLBACKS
		//Set up callback for our thread
		SetupCallbacks();
#endif
	}

	//Set up the DveMgr (TV Display) and Detect PSP Slim 
	if ( kuKernelGetModel() == PSP_MODEL_SLIM_AND_LITE )
	{
		PSP_IS_SLIM = true;
		HAVE_DVE = pspSdkLoadStartModule("dvemgr.prx", PSP_MEMORY_PARTITION_KERNEL);
		if (HAVE_DVE >= 0)
			PSP_TV_CABLE = pspDveMgrCheckVideoOut();
		if (PSP_TV_CABLE == 1)
			PSP_TV_LACED = 1; // composite cable => interlaced
	}

	HAVE_DVE = (HAVE_DVE < 0) ? 0 : 1; // 0 == no dvemgr, 1 == dvemgr

    //setup Pad
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

	// Init the savegame directory
	strcpy( g_DaedalusConfig.szSaveDir, DAEDALUS_PSP_PATH( "SaveGames/" ) );

	if (!System_Init())
		return false;

	return true;
}

//*************************************************************************************
//
//*************************************************************************************
static void Finalise()
{
	System_Finalize();
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
extern bool gDebugDisplayList;
#include "HLEGraphics/DLParser.h"
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
	if(gDebugDisplayList)
		return;
#endif

	DPF( DEBUG_FRAME, "********************************************" );

	bool		activate_pause_menu( false );
	//
	//	Figure out how long the last frame took
	//
#ifdef DAEDALUS_PROFILE_EXECUTION	
	DumpDynarecStats( elapsed_time );
#endif
	//
	//	Enter the debug menu as soon as select is newly pressed
	//
	// If kernelbuttons.prx couldn't be loaded, allow select button to be used instead
	//
	static u32 oldButtons = 0;

	if(oldButtons != gButtons.type)
	{
		if(gButtons.type & gButtons.style)
		{
			activate_pause_menu = true;
		}
	}
	oldButtons = gButtons.type;

	if(activate_pause_menu)
	{
		// See how much texture memory we're using
		//CTextureCache::Get()->DropTextures();
//#ifdef DAEDALUS_DEBUG_MEMORY
		//CVideoMemoryManager::Get()->DisplayDebugInfo();
//#endif
		Save::Flush(true);
		// switch back to the LCD display
		CGraphicsContext::Get()->SwitchToLcdDisplay();

		// Call this initially, to tidy up any state set by the emulator
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

//*************************************************************************************
// Here's where we load up the GUI
//*************************************************************************************
static void DisplayRomsAndChoose(bool show_splash)
{
	// switch back to the LCD display
	CGraphicsContext::Get()->SwitchToLcdDisplay();

	CDrawText::Initialise();

	CUIContext *	p_context( CUIContext::Create() );

	if(p_context != NULL)
	{
		// Already set in ClearBackground() @ UIContext.h 
		//const c32		BACKGROUND_COLOUR = c32( 107, 188, 255 );		// blue
		//const c32		BACKGROUND_COLOUR = c32( 92, 162, 219 );		// blue
		//const c32		BACKGROUND_COLOUR = c32( 1, 1, 127 );			// dark blue
		//const c32		BACKGROUND_COLOUR = c32( 1, 127, 1 );			// dark green

		//p_context->SetBackgroundColour( BACKGROUND_COLOUR );

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

//*************************************************************************************
// This is our main loop
//*************************************************************************************
extern "C"
{
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
			printf("Loading %s\n", (char*)argv[1] );
			System_Open( (char*)argv[1] );
			CPU_Run();
			System_Close();
			Finalise();
			sceKernelExitGame();
			return 0;
		}
#endif

		bool show_splash = true;
		for(;;)
		{
			DisplayRomsAndChoose( show_splash );
			show_splash = false;

			//
			// Commit the preferences and roms databases before starting to run
			//
			CRomDB::Get()->Commit();
			CPreferences::Get()->Commit();

			CPU_Run();
			System_Close();
		}

		Finalise();
	}

	sceKernelExitGame();
	return 0;
}
}
