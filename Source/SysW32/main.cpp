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

#include "VersionNo.h"

#include "Debug/DebugLog.h"
#include "Debug/DBGConsole.h"		// DBGConsole_Enable

#include "Core/ROM.h"				// ROM_Unload
#include "Core/CPU.h"
#include "Core/RomSettings.h"

#include "System.h"

#include "Interface/MainWindow.h"
#include "Interface/RomDB.h"

#include "Utility/ConfigHandler.h"	//
#include "Utility/Preferences.h"
#include "Utility/Profiler.h"		// CProfiler::Create/Destroy
#include "Utility/ResourceString.h"
#include "Utility/IO.h"

#include "Localisation/localization.h"

#include "Resources/resource.h"

#include "Test/BatchTest.h"

#include "ConfigOptions.h"

/////////////////////////////////////////////////////////////////////
//
// Static Declarations
//
/////////////////////////////////////////////////////////////////////
int  WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hPrevInst, LPSTR lpszArgs, int nWinMode);
static void LoadStrings(void);
static int  RunMain(void);
static void DoOneTimeUpdateStuff();
static void ReadConfiguration();
static void WriteConfiguration();
static void DisplayDisclaimer();
static void DisplayConfig();

static int  DaedalusMain( int nWinMode, int argc, char** argv);


//*****************************************************************************
// Static variables
//*****************************************************************************

/////////////////////////////////////////////////////////////////////
//
// Globals (ugh)
//
/////////////////////////////////////////////////////////////////////
CComModule _Module;

char		g_szDaedalusName[256];
char		g_szCurrentRomFileName[MAX_PATH] = "";
char		gDaedalusExePath[MAX_PATH+1];

HINSTANCE	g_hInstance = NULL;

OSVERSIONINFO g_OSVersionInfo;

/////////////////////////////////////////////////////////////////////
//
// WinMain
//
/////////////////////////////////////////////////////////////////////
int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hPrevInst,
				   LPSTR command_line, int nWinMode)
{
	int		return_code;

#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF |
					//_CRTDBG_CHECK_ALWAYS_DF |
					_CRTDBG_DELAY_FREE_MEM_DF |
					_CRTDBG_LEAK_CHECK_DF );
#endif

	_Module.Init( NULL, hThisInst );

	    int    argc;
    char** argv;

    char*  arg;
    int    index;

    // count the arguments

    argc = 1;
    arg  = command_line;

    while (arg[0] != 0) {
        while (arg[0] != 0 && arg[0] == ' ') {
            arg++;
        }
        if (arg[0] != 0) {
            argc++;
            while (arg[0] != 0 && arg[0] != ' ') {
                arg++;
            }
        }
    }

    // tokenize the arguments
    argv = (char**)malloc(argc * sizeof(char*));

    arg = command_line;
    index = 1;

    while (arg[0] != 0) {
        while (arg[0] != 0 && arg[0] == ' ') {
            arg++;
        }
        if (arg[0] != 0) {
            argv[index] = arg;
            index++;
            while (arg[0] != 0 && arg[0] != ' ') {
                arg++;
            }
            if (arg[0] != 0) {
                arg[0] = 0;
                arg++;
            }
        }
    }

    // put the program name into argv[0]

    char filename[_MAX_PATH];

    GetModuleFileName(NULL, filename, _MAX_PATH);
    argv[0] = filename;

	return_code = DaedalusMain( nWinMode, argc, argv );

	free(argv);
	_Module.Term();

	return return_code;
}


//*****************************************************************************
//
//*****************************************************************************
static int  DaedalusMain( int nWinMode , int argc, char **argv)
{
	int nResult = 0;
	BOOL bResult;

	g_hInstance = _Module.GetModuleInstance();

	g_OSVersionInfo.dwOSVersionInfoSize = sizeof(g_OSVersionInfo);
	GetVersionEx(&g_OSVersionInfo);

	// must be setup before checking for DLLs to display localized error message
	Localization_SetLanguage();

	LoadStrings();
	GetModuleFileName(g_hInstance, gDaedalusExePath, MAX_PATH);
	IO::Path::RemoveFileSpec(gDaedalusExePath);

	// Check for DX8 - nice idea from Lkb!!
	// We delayload dinput8.dll (ddraw.dll is almost always likely to be found)
	// This ensures that Daedalus will start up ok.
	// We check here for dinput8.dll - if it's missing then we return
	// a more meaningful error than windows would, and quit
	{
		HINSTANCE hInstDInput;

		hInstDInput = LoadLibrary("dinput8.dll");
		if (hInstDInput == NULL)
		{
			MessageBox(NULL, CResourceString(IDS_NODX8),
							 g_szDaedalusName, MB_ICONSTOP|MB_OK);
			return 0;
		}
		FreeLibrary(hInstDInput);
	}

	//
	// Read configuration before doing anything major
	//
	ReadConfiguration();

	bResult = System_Init();
	if ( !bResult )
		return 1;

	//
	// Create the console if it's enabled. Don't care about failures
	//
	DisplayDisclaimer();
	DisplayConfig();

	// Wrap the main processing in an exception handler
	// This should at least let us save the config/inifile
	// if something goes wrong.
	try
	{
#ifdef DAEDALUS_BATCH_TEST_ENABLED
		if (argc > 1)
		{
			BatchTestMain( argc, argv);
		}
#endif
		nResult = RunMain();
	}
	catch (...)
	{
		if (g_DaedalusConfig.TrapExceptions)
			MessageBox(NULL, CResourceString(IDS_EXCEPTION), g_szDaedalusName, MB_OK);
		else
		{
			throw;
		}
	}


	//
	// Write current config out to the registry
	//
	WriteConfiguration();

	//
	// Turn off the debug console
	//
	CDebugConsole::Get()->EnableConsole( false );

	System_Finalize();

	return nResult;
}


int RunMain(void)
{
	MSG msg;
	HACCEL hMainAccel;

	// Load the accelerators
	hMainAccel = LoadAccelerators( g_hInstance, MAKEINTRESOURCE(IDR_APP_ACCELERATOR));


	// Was PeekMessage - caused huge slowdown on Win9x machines.
	// (but not on Win2k machines strangely). There is no good reason
	// for PeekMessage being there. Originally the CPU was driven off the idle
	// time in this loop. When I moved the CPU stuff to its own thread, I must
	// have fogotten to change this back to GetMessage. Oops.
	while (GetMessage(&msg, (HWND)NULL, 0,0))//, PM_REMOVE))
	{

		// Disable this code so that the listview passes accelerator commands on
		//if (msg.hwnd == CMainWindow::Get()->GetWindow())
		{
			if (!TranslateAccelerator(CMainWindow::Get()->GetWindow(), hMainAccel, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		/*else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}*/
	}

	return 0;
}
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
void LoadStrings(void)
{
	LoadString( _Module.GetResourceInstance(), IDS_DAEDALUS_STRING, g_szDaedalusName, 256);
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

static BOOL __stdcall DisclaimerDialogProc(HWND hWndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

	switch (msg)
	{
	case WM_INITDIALOG:
		SendMessage(GetDlgItem(hWndDlg, IDC_DAEDALUS_LOGO), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_DAEDALUS)));
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hWndDlg, 0);
			break;
		}
		break;
	}

	return 0;
}



void DoOneTimeUpdateStuff()
{

	DialogBox( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_DISCLAIMER), NULL, DisclaimerDialogProc);

	// Here we might do other stuff like update registry keys,
	// associate filetypes or whatever

	if (g_DaedalusConfig.nNumRomsDirs == 0 || lstrlen(g_DaedalusConfig.szRomsDirs[0]) == 0)
	{
		// g_DaedalusConfig.szRomsDir = DefaultDirectory/Daedalus
		ConfigHandler * pConfig = new ConfigHandler("Default Directory");
		if (pConfig != NULL)
		{
			pConfig->ReadString("Daedalus", g_DaedalusConfig.szRomsDirs[0], MAX_PATH, "");
			delete pConfig;
		}

		// If it's still empty, it is initialised when the ListView is filled
	}

	if (lstrlen(g_DaedalusConfig.szSaveDir) == 0)
	{
		lstrcpyn(g_DaedalusConfig.szSaveDir, g_DaedalusConfig.szRomsDirs[0], MAX_PATH);
		//Main_SelectSaveDir(CMainWindow::Get()->GetWindow());		// This is probably null?
	}

	if (lstrlen(g_DaedalusConfig.szGfxPluginFileName) == 0)
	{
		// TODO: Prompt instead?
		IO::Path::Combine( g_DaedalusConfig.szGfxPluginFileName, gDaedalusExePath, "Plugins\\DaedalusGraphics.dll" );
	}
}




// Load config details from the registry
void ReadConfiguration()
{
	ConfigHandler * pConfig = new ConfigHandler("Main");

	if (pConfig != NULL)
	{
		LONG nLeft, nTop, nWidth, nHeight;
		char last_version[30+1];

		DWORD value;

		pConfig->ReadValue("ShowDebug", &value, FALSE);					g_DaedalusConfig.ShowDebug = value ? true : false;
		pConfig->ReadString("Version", last_version, 30, "0.00");


		pConfig->ReadValue("NumRomDirs", (DWORD*)&g_DaedalusConfig.nNumRomsDirs, 0);
		for ( u32 rd = 0; rd < g_DaedalusConfig.nNumRomsDirs; rd++ )
		{
			char key[30];
			wsprintf( key, "RomsDir%d", rd+1 );
			pConfig->ReadString(key, g_DaedalusConfig.szRomsDirs[ rd ], MAX_PATH, "");
		}


		pConfig->ReadString("SaveDir", g_DaedalusConfig.szSaveDir, MAX_PATH, "");
		pConfig->ReadString("GraphicsPlugin", g_DaedalusConfig.szGfxPluginFileName, MAX_PATH, "");

		pConfig->ReadValue( "RecurseRomDirectory", &value, false );		g_DaedalusConfig.RecurseRomDirectory = value ? true : false;

		// These are used internally - mainly useful for debugging
		pConfig->ReadValue("WarnMemoryErrors", &value, false );			g_DaedalusConfig.WarnMemoryErrors = value ? true : false;
		pConfig->ReadValue("TrapExceptions", &value, true);				g_DaedalusConfig.TrapExceptions = value ? true : false;
		pConfig->ReadValue("RunAutomatically", &value, true);			g_DaedalusConfig.RunAutomatically = value ? true : false;

		pConfig->ReadValue("WindowLeft", (DWORD*)&nLeft, CW_USEDEFAULT);
		pConfig->ReadValue("WindowTop", (DWORD*)&nTop, CW_USEDEFAULT);
		pConfig->ReadValue("WindowWidth", (DWORD*)&nWidth, 640);
		pConfig->ReadValue("WindowHeight", (DWORD*)&nHeight, 480);

		g_DaedalusConfig.rcMainWindow.left = nLeft;
		g_DaedalusConfig.rcMainWindow.top = nTop;
		g_DaedalusConfig.rcMainWindow.right = nLeft + nWidth;
		g_DaedalusConfig.rcMainWindow.bottom = nTop + nHeight;

		//
		// If this is a new version, do any one-off update stuff
		//
		if (_strcmpi(last_version, DAEDALUS_VERSION) != 0)
		{
			DoOneTimeUpdateStuff();
		}

		delete pConfig;
	}



}



// Write config details back out to the registry
void WriteConfiguration()
{
	ConfigHandler * pConfig = new ConfigHandler("Main");

	if (pConfig != NULL)
	{
		pConfig->WriteValue("ShowDebug", g_DaedalusConfig.ShowDebug);
		pConfig->WriteString("Version", DAEDALUS_VERSION);


		pConfig->WriteValue("NumRomDirs", g_DaedalusConfig.nNumRomsDirs);
		for ( u32 rd = 0; rd < g_DaedalusConfig.nNumRomsDirs; rd++ )
		{
			char key[30];
			wsprintf( key, "RomsDir%d", rd+1 );
			pConfig->WriteString(key, g_DaedalusConfig.szRomsDirs[ rd ]);
		}

		pConfig->WriteString("SaveDir", g_DaedalusConfig.szSaveDir);
		pConfig->WriteString("GraphicsPlugin", g_DaedalusConfig.szGfxPluginFileName);

		pConfig->WriteValue( "RecurseRomDirectory", g_DaedalusConfig.RecurseRomDirectory );

		pConfig->WriteValue("WarnMemoryErrors", g_DaedalusConfig.WarnMemoryErrors);
		pConfig->WriteValue("TrapExceptions", g_DaedalusConfig.TrapExceptions);
		pConfig->WriteValue("RunAutomatically", g_DaedalusConfig.RunAutomatically);

		pConfig->WriteValue("WindowLeft", g_DaedalusConfig.rcMainWindow.left);
		pConfig->WriteValue("WindowTop", g_DaedalusConfig.rcMainWindow.top);
		pConfig->WriteValue("WindowWidth", g_DaedalusConfig.rcMainWindow.right - g_DaedalusConfig.rcMainWindow.left);
		pConfig->WriteValue("WindowHeight", g_DaedalusConfig.rcMainWindow.bottom - g_DaedalusConfig.rcMainWindow.top);

		delete pConfig;
	}
}



void DisplayDisclaimer()
{
	DBGConsole_Msg(0, "[cDaedalus %s Build %d] - A Nintendo64 Emulator by StrmnNrmn", DAEDALUS_VERSION, DAEDALUS_BUILD_NO);
	DBGConsole_Msg(0, "Copyright (C) 2001 StrmnNrmn");

	DBGConsole_Msg(0, "");
	DBGConsole_Msg(0, "[WDisclaimer]");
	DBGConsole_Msg(0, "[W----------]");

	DBGConsole_Msg(0, "I do not have any association with Nintendo, or");
	DBGConsole_Msg(0, "any of its affiliates.  This program was developed for");
	DBGConsole_Msg(0, "non-commercial use and is not intended to compete ");
	DBGConsole_Msg(0, "with the Nintendo 64.");
	DBGConsole_Msg(0, "The name Nintendo and various other names and");
	DBGConsole_Msg(0, "service marks are owned by either Nintendo or their ");
	DBGConsole_Msg(0, "respective owners. ");
	DBGConsole_Msg(0, "");

	DBGConsole_Msg(0, "For more information visit [B%s]", DAEDALUS_SITE);

	DBGConsole_Msg(0, "");
}

void DisplayConfig()
{
	DBGConsole_Msg(0, "[WConfig]");
	DBGConsole_Msg(0, "[W------]");
	DBGConsole_Msg(0, "WarnMemoryErrors: [W%s], TrapExceptions: [W%s], RunAutomatically: [W%s]",
		g_DaedalusConfig.WarnMemoryErrors ? "on" : "off",
		g_DaedalusConfig.TrapExceptions ? "on" : "off",
		g_DaedalusConfig.RunAutomatically ? "on" : "off");

	DBGConsole_Msg(0, "");
}

