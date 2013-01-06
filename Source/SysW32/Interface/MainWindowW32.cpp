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

//*****************************************************************************
// Include files
//*****************************************************************************
#include "stdafx.h"

#include "Interface/MainWindow.h"
#include "ConfigDialog.h"
#include "RomSettingsDialog.h"
#include "FileNameHandler.h"

#include "System.h"

#include "Core/Interrupt.h"
#include "Core/CPU.h"
#include "Core/RomSettings.h"
#include "Core/SaveState.h"

#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"

#include "Input/InputManager.h"

#include "OSHLE/ultra_rcp.h"
#include "OSHLE/ultra_r4300.h"
#include "OSHLE/ultra_os.h"			// Controller buttons defs

#include "Utility/Timing.h"
#include "Utility/ResourceString.h"		// Useful everywhere
#include "Utility/FramerateLimiter.h"
#include "Utility/IO.h"

#include "Resources/resource.h"

#include "Plugins/AudioPlugin.h"
#include "SysW32/Plugins/GraphicsPluginW32.h"

#include "ConfigOptions.h"

#include <shellapi.h>
#include <shlobj.h>

//*****************************************************************************
//
//*****************************************************************************
enum EColumnType
{
	COLUMN_NAME = 0,
	COLUMN_COUNTRY,
	COLUMN_SIZE,
	COLUMN_COMMENT,
	COLUMN_INFO,
	COLUMN_SAVE,
	COLUMN_BOOT,

	NUM_COLUMNS
};


struct SColumnInfo
{
	u32		ResourceID;
	u32		CommandID;
	u32		ColumnWidth;
	u32		Order;
	s32		ColumnIndex;
	bool	Enabled;
};

static SColumnInfo	gColumnInfo[] =
{
	{ IDS_COLUMN_NAME,		IDM_COLUMN_NAME,	180, 0, -1, true },	// COLUMN_NAME,
	{ IDS_COLUMN_COUNTRY,	IDM_COLUMN_COUNTRY, 100, 1, -1, true },	// COLUMN_COUNTRY,
	{ IDS_COLUMN_SIZE,		IDM_COLUMN_SIZE,	100, 2, -1, true },	// COLUMN_SIZE,
	{ IDS_COLUMN_COMMENT,	IDM_COLUMN_COMMENT, 100, 3, -1, true },	// COLUMN_COMMENT,
	{ IDS_COLUMN_INFO,		IDM_COLUMN_INFO,	100, 4, -1, true },	// COLUMN_INFO,
	{ IDS_COLUMN_SAVE,		IDM_COLUMN_SAVE,	100, 5, -1, true },	// COLUMN_SAVE,
	{ IDS_COLUMN_BOOT,		IDM_COLUMN_BOOT,	100, 6, -1, true },	// COLUMN_BOOT,
};

DAEDALUS_STATIC_ASSERT( ARRAYSIZE( gColumnInfo ) == NUM_COLUMNS );

//*****************************************************************************
//
//*****************************************************************************
class CListViewItemInfo
{
	private:
		CListViewItemInfo( const CHAR * szFileName, RomID rom_id, u32 rom_size, ECicType cic_type )
		:	mRomID( rom_id )
		,	mRomSize( rom_size )
		,	mCicType( cic_type )
		{
			ZeroMemory( &mSettings, sizeof( mSettings ) );
			lstrcpyn( mFileName, szFileName, MAX_PATH );
			bInitialised = false;
			bValid = true;
		}

	public:
		static CListViewItemInfo * Create( const CHAR * szFileName )
		{
			RomID			rom_id;
			u32				rom_size;
			ECicType		cic_type;

			if ( ROM_GetRomDetailsByFilename( szFileName, &rom_id, &rom_size, &cic_type ) )
			{
				return new CListViewItemInfo( szFileName, rom_id, rom_size, cic_type );
			}
			else
			{
				return NULL;
			}
		}

		~CListViewItemInfo()
		{
		}

		bool Initialise()
		{
			if ( !CRomSettingsDB::Get()->GetSettings( mRomID, &mSettings ) )
			{
				// Create new entry, add
				mSettings.Reset();
				mSettings.Comment = "No Comment";

				//
				// We want to get the "internal" name for this rom from the header
				// Failing that, use the filename
				//
				std::string game_name;
				if ( !ROM_GetRomName( mFileName, game_name ) )
				{
					game_name = IO::Path::FindFileName( mFileName );
				}
				game_name = game_name.substr(0, 63);
				mSettings.GameName = game_name.c_str();

				CRomSettingsDB::Get()->SetSettings( mRomID, mSettings );
			}

			bInitialised = true;
			return true;
		}

		bool	IsValid()					// Should be const, but we need to init
		{
			//
			// If we already know this is invalid, don't try to scan again
			//
			if ( !bValid )
			{
				return false;
			}

			//
			// If we've not managed to initialise this yes, try now
			//
			if ( !bInitialised )
			{
				return Initialise();
			}

			return true;
		}

		void	Invalidate()				// Force us to reload info about this rom
		{
			bValid = true;
			bInitialised = false;
		}

		const RomID &	GetRomID() const			{ return mRomID; }
		const CHAR *	GetFileName() const			{ return mFileName; }
		const CHAR *	GetGameName() const			{ return mSettings.GameName.c_str(); }
		const CHAR *	GetComment() const			{ return mSettings.Comment.c_str(); }
		const CHAR *	GetInfo() const				{ return mSettings.Info.c_str(); }
		ESaveType		GetSaveType() const			{ return mSettings.SaveType; }
		ECicType		GetCicType() const			{ return mCicType; }

		u32				GetRomSize() const			{ return mRomSize; }
		u8				GetCountryID() const		{ return mRomID.CountryID; }

	private:
		CHAR			mFileName[ MAX_PATH + 1 ];
		const RomID		mRomID;
		const u32		mRomSize;
		const ECicType	mCicType;
		RomSettings		mSettings;

		bool			bValid;
		bool			bInitialised;		// If false, mSettings/mRomID have not been set up
};

//*****************************************************************************
// Static variables
//*****************************************************************************
static const char c_szRomFileStr[] = "ROM Files\0*.v64;*.z64;*.n64;*.rom;*.jap;*.pal;*.usa;*.zip\0All Files (*.*)\0*.*\0";
static const char c_szSaveStateFileStr[] = "Savestate Files\0*.n64state;*.n6s;*.pj;*.pj0;*.pj1;*.pj2;*.pj3;*.pj4;*.pj5;*.pj6;*.pj7;*.pj8;*.pj9\0All Files (*.*)\0";
static const TCHAR	sc_szMainWindowClassname[256] = "DaedalusWindowClass";


//*****************************************************************************
//
//*****************************************************************************
class IMainWindow : public CMainWindow
{
	public:
		IMainWindow();
		~IMainWindow() {}

		HRESULT CreateWnd(INT nWinMode);

		void SelectSaveDir(HWND hWndParent);

		void DAEDALUS_VARARG_CALL_TYPE SetStatus(INT nPiece, const char * szFormat, ...);

		LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		HRESULT Initialise(void);
	private:


		BOOL	CreateStatus(HWND hWndParent);

		BOOL	CreateList(HWND hWndParent);
		BOOL	InitList();
		BOOL	FillList();
		void	ToggleColumn( EColumnType column );

		static int CALLBACK ListView_CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lpData);


		BOOL AddDirectory(const char * szDirectory);
		BOOL AddItem(const char * szFileName);

		void SaveWindowPos(HWND hWnd);

		void PauseDrawing();
		void RestartDrawing();

		void AskForRomDirectory(HWND hWndParent);

		static LRESULT CALLBACK WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		BOOL OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct);
		void OnDestroy(HWND hWnd);

		void OnInitMenu(HWND hwnd, HMENU hMenu);
		void OnTimer(HWND hWnd, UINT id);
		void OnMove(HWND hWnd, int x, int y);
		void OnSize(HWND hWnd, UINT state, int cx, int cy);

		void OnCommand(HWND hWnd, int id, HWND hwndCtl, UINT codeNotify);
		void OnKeyUp(HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
		void OnContextMenu(HWND hWnd, HWND hWndContext, UINT xPos, UINT yPos);

		void OnStartEmu(HWND hWnd);
		void OnEndEmu(HWND hWnd);

		LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
		LRESULT OnListView_DeleteItem(int idCtrl, LPNMHDR pnmh);
		LRESULT OnListView_GetDispInfo(int idCtrl, LPNMHDR pnmh);
		LRESULT OnListView_ColumnClick(int idCtrl, LPNMHDR pnmh);
		LRESULT OnListView_DblClick(int idCtrl, LPNMHDR pnmh);
		LRESULT OnListView_RClick(int idCtrl, LPNMHDR pnmh);
		LRESULT OnListView_Return(int idCtrl, LPNMHDR pnmh);

	private:
		//
		// Member variables
		//
		HWND m_hWndList;
		BOOL m_bActive;
};


//*****************************************************************************
// Singleton creation
//*****************************************************************************
template<> bool	CSingleton< CMainWindow >::Create()
{
	HRESULT hr;

	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	IMainWindow * pInstance = new IMainWindow();

	hr = pInstance->Initialise();
	if (FAILED(hr))
	{
		delete pInstance;
		return false;
	}

	mpInstance = pInstance;
	pInstance->CreateWnd(SW_SHOW);
	return true;
}

//*****************************************************************************
//
//*****************************************************************************
IMainWindow::IMainWindow() :
	m_hWndList( NULL ),
	m_bActive( TRUE )
{
	m_hWnd = NULL;
	m_hWndStatus = NULL;
}



//*****************************************************************************
//
//*****************************************************************************
HRESULT IMainWindow::Initialise()
{
    WNDCLASS wc;

    // Register the frame window class.
    wc.style         = 0;
    wc.hInstance     = g_hInstance;
    wc.cbClsExtra    = 0;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpfnWndProc   = (WNDPROC)WndProcStatic;
    wc.cbWndExtra    = 0;
    wc.hIcon         = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_DAEDALUS_ICON));
    wc.hbrBackground = (HBRUSH)NULL;// (COLOR_APPWORKSPACE + 1); //(HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName  = MAKEINTRESOURCE(IDR_APP_MENU);
    wc.lpszClassName = sc_szMainWindowClassname;

    if (!RegisterClass (&wc) )
        return E_FAIL;

    return S_OK;
}



//*****************************************************************************
//
//*****************************************************************************
HRESULT IMainWindow::CreateWnd(INT nWinMode)
{
	m_hWnd = CreateWindow(sc_szMainWindowClassname,
								g_szDaedalusName,
								WS_OVERLAPPEDWINDOW,
								g_DaedalusConfig.rcMainWindow.left,
								g_DaedalusConfig.rcMainWindow.top,
								g_DaedalusConfig.rcMainWindow.right - g_DaedalusConfig.rcMainWindow.left,
								g_DaedalusConfig.rcMainWindow.bottom - g_DaedalusConfig.rcMainWindow.top,
								HWND_DESKTOP,
								LoadMenu( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_APP_MENU) ),
								g_hInstance,
								NULL);

	// Get the window pos, if CW_USEDEFAULT has been used for left/top
	if (IsWindow(m_hWnd))
	{
		ShowWindow(m_hWnd, nWinMode);
		UpdateWindow(m_hWnd);

		SaveWindowPos(m_hWnd);
		FillList();

		return S_OK;
	}

	return E_FAIL;
}


//*****************************************************************************
//
//*****************************************************************************
void IMainWindow::SaveWindowPos(HWND hWnd)
{
	/*bResult = */GetWindowRect(hWnd, &g_DaedalusConfig.rcMainWindow);
}

void DAEDALUS_VARARG_CALL_TYPE IMainWindow::SetStatus(INT nPiece, const char * szFormat, ...)
{
    va_list va;
	TCHAR szBuffer[2048+1];

	// Format the output
	va_start(va, szFormat);
	// Don't use wvsprintf as it doesn't handle floats!
	vsprintf(szBuffer, szFormat, va);
	va_end(va);

	// I changed this to PostMessage as the SendMessage would
	// stall if the user tried to stop the CPU thread while
	// the game was searching for OS functions to patch.
	::PostMessage(m_hWndStatus, SB_SETTEXT, nPiece | 0, (LPARAM)szBuffer);
}


//*****************************************************************************
//
//*****************************************************************************
void IMainWindow::PauseDrawing()
{
	//LPDIRECTDRAW8 pDD;

    m_bActive = FALSE;

//	CGraphicsContext::Get()->Lock();
/*D3D8FIX		pDD = CGraphicsContext::Get()->GetDD();
		if( pDD )
			pDD->FlipToGDISurface();*/
//	CGraphicsContext::Get()->Unlock();


    DrawMenuBar( m_hWnd );
    RedrawWindow( m_hWnd, NULL, NULL, RDW_FRAME);
    ShowCursor(TRUE);
}

//*****************************************************************************
//
//*****************************************************************************
void IMainWindow::RestartDrawing()
{
    m_bActive = TRUE;
    ShowCursor(FALSE);
}


//*****************************************************************************
//
//*****************************************************************************
static BOOL __stdcall AboutDlgProc(HWND hWndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
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

//*****************************************************************************
//
//*****************************************************************************
LRESULT CALLBACK IMainWindow::WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return Get()->WndProc(hWnd, message, wParam, lParam);
}

//*****************************************************************************
//
//*****************************************************************************
LRESULT IMainWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_ENTERSIZEMOVE: CInputManager::Get()->Unaquire(); break;
		case WM_ENTERMENULOOP: CInputManager::Get()->Unaquire(); PauseDrawing(); break;
	    case WM_EXITMENULOOP: RestartDrawing(); break;

		HANDLE_MSG( hWnd, WM_CREATE, OnCreate );
		HANDLE_MSG( hWnd, WM_DESTROY, OnDestroy );
		HANDLE_MSG( hWnd, WM_TIMER, OnTimer );
		HANDLE_MSG( hWnd, WM_INITMENU, OnInitMenu );
		HANDLE_MSG( hWnd, WM_MOVE, OnMove );
		HANDLE_MSG( hWnd, WM_SIZE, OnSize );
		HANDLE_MSG( hWnd, WM_COMMAND, OnCommand );
		HANDLE_MSG( hWnd, WM_KEYUP, OnKeyUp );
		HANDLE_MSG( hWnd, WM_CONTEXTMENU, OnContextMenu );

		case WM_CLOSE:		break;


// We can't really do this, because we're calling from a different thread. A rethink is required....
//		case WM_PAINT:		if ( gGraphicsPlugin != NULL ) gGraphicsPlugin->DrawScreen(); break;	//return 0;

		case WM_NOTIFY: return OnNotify((int)wParam, (LPNMHDR)lParam);
		case MWM_STARTEMU: return (OnStartEmu(hWnd), 0);
		case MWM_ENDEMU: return (OnEndEmu(hWnd), 0);

	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

//*****************************************************************************
//
//*****************************************************************************
BOOL IMainWindow::OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
	if (CreateList(hWnd))
	{
		// Allow the columns to be rearranged
		ListView_SetExtendedListViewStyle(m_hWndList, LVS_EX_HEADERDRAGDROP );

		if(InitList())
		{

		}
	}

	CreateStatus(hWnd);

	SetTimer(hWnd, 0, 2000, NULL);

	// Created ok
	return TRUE;
}

//*****************************************************************************
//
//*****************************************************************************
void IMainWindow::OnDestroy(HWND hWnd)
{
	SaveWindowPos(hWnd);

	CPU_StopThread();
	m_hWndList = NULL;
	m_hWndStatus = NULL;

	PostQuitMessage(0);
}


//*****************************************************************************
//
//*****************************************************************************
LRESULT IMainWindow::OnNotify(int idCtrl, LPNMHDR pnmh)
{
	LRESULT lResult = 0;
	BOOL bHandled;

	bHandled = FALSE;

	if (pnmh->idFrom == IDC_ROMLISTVIEW)
	{
		bHandled = TRUE;
		switch (pnmh->code)
		{
		case LVN_DELETEITEM:	lResult = OnListView_DeleteItem(idCtrl, pnmh); break;
		case LVN_GETDISPINFO:	lResult = OnListView_GetDispInfo(idCtrl, pnmh); break;
		case LVN_COLUMNCLICK:	lResult = OnListView_ColumnClick(idCtrl, pnmh); break;
		case NM_DBLCLK:			lResult = OnListView_DblClick(idCtrl, pnmh); break;
		case NM_RCLICK:			lResult = OnListView_RClick(idCtrl, pnmh); break;
		case NM_RETURN:			lResult = OnListView_Return(idCtrl, pnmh); break;
		default:
			bHandled = FALSE;
			break;
		}
	}

	return lResult;
}

//*****************************************************************************
//
//*****************************************************************************
void IMainWindow::OnInitMenu(HWND hWnd, HMENU/* hMenu*/)
{
	// Only need this for Display menu
	HMENU hMenu;

	hMenu = GetMenu(hWnd);
}

//*****************************************************************************
//
//*****************************************************************************
void IMainWindow::OnTimer(HWND hWnd, UINT id)
{
	static u64 qwLastCount = 0;
	static u64 last_ticks = 0;
	s64		qwNum( gCPUState.CPUControl[C0_COUNT]._u64 - qwLastCount );
	static DWORD dwLastNumFrames = 0;
#ifdef DAEDALUS_PROFILE_EXECUTION
	static DWORD dwLastTLBRHit = 0;
	static DWORD dwLastTLBWHit = 0;
	static DWORD dwLastNumEx = 0;
	static DWORD dwLastNumInt = 0;
	extern u32 gFragmentLookupSuccess;
	extern u32 gFragmentLookupFailure;
#endif

	static DWORD dwNumSRCompiled = 0;
	static DWORD dwNumSROptimised = 0;
	static DWORD dwNumSRFailed = 0;


	if (CPU_IsRunning())
	{
		u64		now;
		u64		elapsed_ticks;
		u64		frequency;
		float fElapsedTime;
		float fFPS;
		float fVIs;
		f32 Fsync = FramerateLimiter_GetSync();


		if (NTiming::GetPreciseTime(&now) && NTiming::GetPreciseFrequency(&frequency))
		{
			if (last_ticks == 0)
				last_ticks = now;
			if (dwLastNumFrames == 0)
				dwLastNumFrames = g_dwNumFrames;

			elapsed_ticks = now - last_ticks;
			last_ticks = now;

			fElapsedTime = float( elapsed_ticks ) / float( frequency );
			fFPS = ((float)(g_dwNumFrames-dwLastNumFrames)/fElapsedTime);
			fVIs = ( Fsync * f32( FramerateLimiter_GetTvFrequencyHz() ) );

			if ( CDebugConsole::Get()->IsVisible() )
			{
				float fMIPS = ((float)qwNum / 1000000.0f)/fElapsedTime;
				CDebugConsole::Get()->Stats( CDebugConsole::STAT_MIPS, "MIPS: %#.2f  FPS: %#.2f", fMIPS, fFPS);
				CDebugConsole::Get()->Stats( CDebugConsole::STAT_VIS, "VIs/s: %#.2f",fVIs);
				CDebugConsole::Get()->Stats( CDebugConsole::STAT_SYNC, "Sync: %#.2f%%",	100 * FramerateLimiter_GetSync());
#ifdef DAEDALUS_PROFILE_EXECUTION
				CDebugConsole::Get()->Stats( CDebugConsole::STAT_TEX, "Lookup:S%d/F%d", gFragmentLookupSuccess, gFragmentLookupFailure);
				gFragmentLookupSuccess = 0; gFragmentLookupFailure=0;
#endif

			#ifdef DAEDALUS_PROFILE_EXECUTION
				float fTLBRS = ((float)(gTLBReadHit - dwLastTLBRHit)/fElapsedTime);
				float fTLBWS = ((float)(gTLBWriteHit - dwLastTLBWHit)/fElapsedTime);
				float fExS = ((float)(gNumExceptions - dwLastNumEx)/fElapsedTime);
				float fIntS = ((float)(gNumInterrupts - dwLastNumInt)/fElapsedTime);

				CDebugConsole::Get()->Stats( CDebugConsole::STAT_TLB, "TLB: R %#.2f W: %#.2f", fTLBRS, fTLBWS);
				CDebugConsole::Get()->Stats( CDebugConsole::STAT_EXCEP, "Ex/S %#.2f Int/S: %#.2f", fExS, fIntS );

				if(gTotalInstructionsExecuted + gTotalInstructionsEmulated > 0)
				{
					float fRatio = float(gTotalInstructionsExecuted * 100.0f / float(gTotalInstructionsEmulated+gTotalInstructionsExecuted));

					CDebugConsole::Get()->Stats( CDebugConsole::STAT_DYNAREC, "ExecutionRatio: %.2f", fRatio);

					gTotalInstructionsExecuted = 0;
					gTotalInstructionsEmulated = 0;
				}
			#endif

			}

			CMainWindow::Get()->SetStatus(1, "VIs/s: %#.2f FPS: %#.2f",fVIs, fFPS);

		}

	}
	qwLastCount = gCPUState.CPUControl[C0_COUNT]._u64;
	dwLastNumFrames = g_dwNumFrames;
#ifdef DAEDALUS_PROFILE_EXECUTION
	dwLastTLBRHit = gTLBReadHit;
	dwLastTLBWHit = gTLBWriteHit;
	dwLastNumEx = gNumExceptions;
	dwLastNumInt = gNumInterrupts;
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void IMainWindow::OnMove(HWND hWnd, int x, int y)
{
	if ( gGraphicsPlugin != NULL )
	{
		gGraphicsPlugin->MoveScreen(x, y);
	}
}

//*****************************************************************************
//
//*****************************************************************************
void IMainWindow::OnSize(HWND hWnd, UINT state, int cx, int cy)
{

	WORD wTop = 0;
	LONG nStatusHeight;

	if (IsWindow(m_hWndStatus))
	{
		RECT rc;

		::SendMessage(m_hWndStatus, WM_SIZE, (WPARAM)state, (LPARAM)MAKELPARAM(cx,cy));

		GetWindowRect(m_hWndStatus, &rc);
		nStatusHeight = rc.bottom - rc.top;
	}
	else
	{
		nStatusHeight = 0;
	}

	cy -= nStatusHeight;

	//resize the ListView to fit our window
	if (IsWindow(m_hWndList))
	{
		MoveWindow(m_hWndList, 0, wTop, cx, cy, TRUE);
	}

//	CGraphicsContext::Get()->OnSize( state );
}


//*****************************************************************************
//
//*****************************************************************************
void IMainWindow::OnCommand(HWND hWnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch ( id )
	{
		// File menu commands
		case IDM_OPEN:
			{
				TCHAR szFileName[MAX_PATH+1];

				FileNameHandler * pfn( new FileNameHandler("Daedalus", c_szRomFileStr, 1, "v64") );
				if (pfn != NULL)
				{
					BOOL bSelected;

					bSelected = pfn->GetOpenName(hWnd, szFileName);
					if (bSelected)
					{
						char reason[100];
						System_Open(szFileName);
						/*bSuccess = */CPU_StartThread(reason, 100);

						// Add this directory to the rom directory list?
						//pfn->GetCurrentDirectory(g_DaedalusConfig.szRomsDir);
					}
					delete pfn;
				}
			}
			break;

		case IDM_LOAD:
			{
				TCHAR szFileName[MAX_PATH+1];

				FileNameHandler * pfn( new FileNameHandler("Savestates", c_szSaveStateFileStr, 1, "n64state", g_DaedalusConfig.szSaveDir) );
				if (pfn != NULL)
				{
					BOOL bSelected;

					bSelected = pfn->GetOpenName(hWnd, szFileName);
					if (bSelected)
					{
						if (!CPU_IsRunning())
						{
							const char *romName = SaveState_GetRom(szFileName);

							if (romName == NULL)
							{
								// report error?
								delete pfn;
								break;
							}

							System_Open(romName);
						}
						CPU_LoadState( szFileName );
						CHAR szReason[300+1];

						if (!CPU_StartThread(szReason, 300))
							MessageBox(szReason);
						pfn->GetCurrentDirectory(g_DaedalusConfig.szSaveDir);
					}
					delete pfn;
				}
			}
			break;

		case IDM_SAVE:
			{
				TCHAR szFileName[MAX_PATH+1];

				FileNameHandler * pfn( new FileNameHandler("Savestates", c_szSaveStateFileStr, 1, "n64state", g_DaedalusConfig.szSaveDir) );
				if (pfn != NULL)
				{
					BOOL bSelected;

					bSelected = pfn->GetSaveName(hWnd, szFileName);
					if (bSelected)
					{
						CPU_SaveState( szFileName );
						pfn->GetCurrentDirectory(g_DaedalusConfig.szSaveDir);
					}
					delete pfn;
				}
			}
			break;

		case IDM_EXIT:
			::SendMessage( hWnd, WM_CLOSE, 0, 0 );
			break;

		case IDM_CPUSTART:
			{
				CHAR szReason[300+1];

				if (!CPU_StartThread(szReason, 300))
					MessageBox(szReason);
			}
			break;

		case IDM_CPUSTOP:
			CPU_StopThread();
			break;

		case IDM_REFRESH:
			// Refresh
			FillList();
			break;
		case IDM_SELECTDIR:
			{
				//AskForRomDirectory(hWnd);

				// Refresh
				FillList();
			}
			break;

		case IDM_INPUTCONFIG:
			CInputManager::Get()->Configure(hWnd);
			break;

		case IDM_CONFIGURATION:
			{
				CConfigDialog	dialog;

				dialog.DoModal();
			}
			break;

		case IDM_GRAPHICS_CONFIG:
			if ( gGraphicsPlugin == NULL )
			{
				gGraphicsPlugin = CreateGraphicsPlugin();
			}
			gGraphicsPlugin->DllConfig( hWnd );

			break;

		case IDM_ABOUT:
			DialogBox( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_ABOUT), hWnd, AboutDlgProc);
			break;
		case IDM_SCREENSHOT:
			if ( gGraphicsPlugin != NULL )
			{
				gGraphicsPlugin->ExecuteCommand( DAEDALUS_GFX_CAPTURESCREEN, NULL );
			}
			break;

		case IDM_TOGGLEFULLSCREEN:
			{
				if ( gGraphicsPlugin != NULL )
				{
					gGraphicsPlugin->ChangeWindow();
				}
			}

			break;

		case IDM_DAEDALUS_HOME:
			// Show the daedalus home page
			ShellExecute( hWnd, "open", DAEDALUS_SITE, NULL, "", SW_SHOWNORMAL );
			break;



		case IDM_COLUMN_NAME:		ToggleColumn( COLUMN_NAME );		break;
		case IDM_COLUMN_COUNTRY:	ToggleColumn( COLUMN_COUNTRY );		break;
		case IDM_COLUMN_SIZE:		ToggleColumn( COLUMN_SIZE );		break;
		case IDM_COLUMN_COMMENT:	ToggleColumn( COLUMN_COMMENT );		break;
		case IDM_COLUMN_INFO:		ToggleColumn( COLUMN_INFO );		break;
		case IDM_COLUMN_SAVE:		ToggleColumn( COLUMN_SAVE );		break;
		case IDM_COLUMN_BOOT:		ToggleColumn( COLUMN_BOOT );		break;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void IMainWindow::OnKeyUp(HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	switch ( vk )
	{
		// None of this will work, because we're using exclusive dinput:
		case VK_F1:
			if ( gGraphicsPlugin != NULL )
			{
				gGraphicsPlugin->ExecuteCommand( DAEDALUS_GFX_DUMPDL, NULL );
			}
			break;

#ifdef DAEDALUS_LOG
		case VK_F2:
			{
				bool	enable( !Debug_GetLoggingEnabled() );

				DBGConsole_Msg(0, "Toggling logging %s", enable ? "on" : "off");

				Debug_SetLoggingEnabled( enable );
			}
			break;
#endif

		// NB Leave VK_SNAPSHOT (printscreen) so that Windows can do its own screenshots
		//case VK_SNAPSHOT:
			//if ( gGraphicsPlugin != NULL )
			//{
			//	gGraphicsPlugin->ExecuteCommand( DAEDALUS_GFX_CAPTURESCREEN, NULL );
			//}
			//break;
	}
}

//*****************************************************************************
// WM_CONTEXTMENU
//*****************************************************************************
void	IMainWindow::OnContextMenu(HWND hWnd, HWND hWndContext, UINT xPos, UINT yPos)
{
	HMENU			menu( CreatePopupMenu() );
	MENUITEMINFO	mii;

	ZeroMemory( &mii, sizeof( mii ) );
	mii.cbSize = sizeof( mii );

	u32 i;
	for ( i = 0; i < NUM_COLUMNS; i++ )
	{
		mii.fMask = MIIM_ID | MIIM_STATE | MIIM_TYPE;
		mii.fType = MFT_STRING;
		mii.fState = gColumnInfo[ i ].Enabled ? MFS_CHECKED : MFS_UNCHECKED;
		mii.wID = gColumnInfo[ i ].CommandID;
		mii.dwTypeData = CResourceString( gColumnInfo[ i ].ResourceID );

		if ( i == COLUMN_NAME )
		{
			mii.fState |= MFS_DISABLED;
		}

		::InsertMenuItem( menu, i, TRUE, &mii );
	}

	mii.fMask = MIIM_TYPE;
	mii.fType = MFT_SEPARATOR;
	mii.dwTypeData = NULL;
	::InsertMenuItem( menu, i, TRUE, &mii );

	mii.fMask = MIIM_ID | MIIM_STATE | MIIM_TYPE;
	mii.fType = MFT_STRING;
	mii.fState = MFS_DEFAULT;
	mii.wID = IDM_COLUMN_MORE;
	mii.dwTypeData = TEXT( "More..." );
	::InsertMenuItem( menu, i+1, TRUE, &mii );


	::TrackPopupMenu( menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, xPos, yPos, 0, hWnd, NULL );
}

//*****************************************************************************
//
//*****************************************************************************
void IMainWindow::OnStartEmu(HWND hWnd)
{

	TCHAR szName[256+1];
	// Save the current window size
	SaveWindowPos(hWnd);

	// Set the title text to include the rom name
	wsprintf(szName, "Daedalus - %s", g_ROM.settings.GameName.c_str());
	SetWindowText(hWnd, szName);

	EnableWindow(m_hWndList, FALSE);
	ShowWindow(m_hWndList, SW_HIDE);

	// Remove keyboard focus from the list (if is stil has it)
	SetFocus( hWnd );

	RECT rc;
	GetWindowRect(hWnd, &rc);
	if ( gGraphicsPlugin != NULL )
	{
		gGraphicsPlugin->MoveScreen(rc.left,rc.top);
	}
}


//*****************************************************************************
//
//*****************************************************************************
void IMainWindow::OnEndEmu(HWND hWnd)
{
	// Release the keyboard!
	CInputManager::Get()->Unaquire();

	ShowWindow(m_hWndList, SW_SHOW);
	EnableWindow(m_hWndList, TRUE);

	// Restore the title text
	SetWindowText(hWnd, "Daedalus");

	// TODO: Restore window size/pos?
	//SizeForDisplay();
}


//*****************************************************************************
//
//*****************************************************************************
BOOL IMainWindow::CreateStatus(HWND hWndParent)
{
	DWORD dwStyle;
	INT sizes[2] = { 340, -1 };

	dwStyle  = WS_CHILD|WS_VISIBLE;
	dwStyle |= SBARS_SIZEGRIP;

	m_hWndStatus = CreateStatusWindow(dwStyle, TEXT("Daedalus"), hWndParent, IDC_STATUSBAR);
	if (!IsWindow(m_hWndStatus))
		return FALSE;

	::SendMessage(m_hWndStatus, SB_SETPARTS, 2, (LPARAM)sizes);

	return TRUE;
}

//*****************************************************************************
//
//*****************************************************************************
BOOL IMainWindow::CreateList(HWND hWndParent)
{
	DWORD dwStyle;
	DWORD dwExStyle;

	dwStyle =   WS_TABSTOP |
				WS_VISIBLE |
				WS_CHILD |
				WS_BORDER |
				LVS_SINGLESEL |
				LVS_REPORT |
				LVS_SHAREIMAGELISTS;

	dwExStyle = WS_EX_CLIENTEDGE;

	m_hWndList = ::CreateWindowEx( dwExStyle,
								WC_LISTVIEW,
								NULL,
								dwStyle,
								0,
								0,
								0,
								0,
								hWndParent,
								(HMENU)IDC_ROMLISTVIEW, // Child window id
								g_hInstance,
								NULL);

	return IsWindow(m_hWndList);

}

//*****************************************************************************
//
//*****************************************************************************
BOOL IMainWindow::InitList()
{
	LV_COLUMN   lvColumn;

	//initialize the columns
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM/* | LVCF_ORDER*/;
	lvColumn.fmt = LVCFMT_LEFT;

	for ( u32 i = 0; i < NUM_COLUMNS; i++ )
	{
		if ( gColumnInfo[ i ].Enabled )
		{
			lvColumn.cx = gColumnInfo[ i ].ColumnWidth;
			lvColumn.pszText = CResourceString( gColumnInfo[ i ].ResourceID );
			//lvColumn.iOrder = gColumnInfo[ i ].Order;
			lvColumn.iSubItem = i;

			gColumnInfo[ i ].ColumnIndex = ListView_InsertColumn(m_hWndList, i, &lvColumn);
		}
	}

	return TRUE;
}


//*****************************************************************************
//
//*****************************************************************************
void	IMainWindow::ToggleColumn( EColumnType column )
{
	// Can't toggle the main colun
	if ( column == COLUMN_NAME )
		return;

	gColumnInfo[ column ].Enabled = !gColumnInfo[ column ].Enabled;

	// Remove all the columns
	while ( ListView_DeleteColumn( m_hWndList, 0 ) )
	{
	}

	InitList();

}

//*****************************************************************************
//
//*****************************************************************************
int CALLBACK IMainWindow::ListView_CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lpData)
{
	// lpData is column to sort on:

	CListViewItemInfo * plvii1 = (CListViewItemInfo *)lParam1;
	CListViewItemInfo * plvii2 = (CListViewItemInfo *)lParam2;

	if ( !plvii1->IsValid() || !plvii2->IsValid() )
		return 0;

	switch ( EColumnType( lpData ) )
	{
		case COLUMN_NAME:			return _strcmpi(plvii1->GetGameName(),			 plvii2->GetGameName() );
		case COLUMN_COMMENT:		return _strcmpi(plvii1->GetComment(),			 plvii2->GetComment() );
		case COLUMN_INFO:			return _strcmpi(plvii1->GetInfo(),				 plvii2->GetInfo() );
		case COLUMN_COUNTRY:		return	   (int)plvii1->GetCountryID() -	(int)plvii2->GetCountryID();
		case COLUMN_SIZE:			return	   (int)plvii1->GetRomSize()   -	(int)plvii2->GetRomSize();
		case COLUMN_SAVE:			return	   (int)plvii1->GetSaveType()  -	(int)plvii2->GetSaveType();
		case COLUMN_BOOT:			return	   (int)plvii1->GetCicType()   -	(int)plvii2->GetCicType();
	}

	return 0;
}


//*****************************************************************************
//
//*****************************************************************************
void IMainWindow::SelectSaveDir(HWND hWndParent)
{
	BROWSEINFO bi;
	LPITEMIDLIST lpidl;
	TCHAR szDisplayName[MAX_PATH+1];

	bi.hwndOwner = hWndParent;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = szDisplayName;
	bi.lpszTitle = CResourceString(IDS_SELECTSAVEFOLDER);
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = NULL;
	bi.lParam = NULL;
	bi.iImage = 0;

	lpidl = SHBrowseForFolder(&bi);
	if (lpidl)
	{
		/*bResult = */SHGetPathFromIDList(lpidl, g_DaedalusConfig.szSaveDir);
	}
	else
	{

	}
}

//*****************************************************************************
// Prompt for a default rom directory if none are currently defined
//*****************************************************************************
void IMainWindow::AskForRomDirectory(HWND hWndParent)
{
	BROWSEINFO bi;
	LPITEMIDLIST lpidl;
	TCHAR szDisplayName[MAX_PATH+1];

	bi.hwndOwner = hWndParent;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = szDisplayName;
	bi.lpszTitle = CResourceString(IDS_SELECTFOLDER);
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = NULL;
	bi.lParam = NULL;
	bi.iImage = 0;

	lpidl = SHBrowseForFolder(&bi);
	if (lpidl)
	{
		if ( SHGetPathFromIDList(lpidl, g_DaedalusConfig.szRomsDirs[ 0 ] ) )
		{
			g_DaedalusConfig.nNumRomsDirs = 1;
		}
	}
	else
	{

	}
}


//*****************************************************************************
//
//*****************************************************************************
BOOL IMainWindow::FillList()
{
	if (g_DaedalusConfig.nNumRomsDirs == 0 || !IO::Directory::IsDirectory(g_DaedalusConfig.szRomsDirs[ 0 ]))
	{
		// Don't pass in m_hWnd - as it may not have been set up yet!
		AskForRomDirectory(m_hWndList);
		if (!IO::Directory::IsDirectory(g_DaedalusConfig.szRomsDirs[ 0 ]))
		{
			return FALSE;
		}
	}

	//empty the list
	ListView_DeleteAllItems(m_hWndList);

	for ( u32 rd = 0; rd < g_DaedalusConfig.nNumRomsDirs; rd++ )
	{
		/* bResult = */AddDirectory(g_DaedalusConfig.szRomsDirs[ rd ]);
	}

	//sort the items by name
	ListView_SortItems(m_hWndList, ListView_CompareItems, (LPARAM)COLUMN_NAME);

	for ( u32 i = 0; i < NUM_COLUMNS; i++ )
	{
		ListView_SetColumnWidth(m_hWndList, i, LVSCW_AUTOSIZE_USEHEADER);
	}

	return TRUE;
}

//*****************************************************************************
// Return FALSE on critical error (stop recursing)
//*****************************************************************************
BOOL IMainWindow::AddDirectory(const char * szDirectory)
{
	HANDLE hFind;
	WIN32_FIND_DATA wfd;
	TCHAR szFullPath[MAX_PATH+1];
	TCHAR szSearchSpec[MAX_PATH+1];
	const char * p_ext;

	if (!IO::Directory::IsDirectory(szDirectory))
		return FALSE;

	// Recurse, adding all roms that we find...
	IO::Path::Combine(szSearchSpec, szDirectory, TEXT("*.*"));

	hFind = FindFirstFile(szSearchSpec, &wfd);
	if (hFind == INVALID_HANDLE_VALUE)
		return FALSE;

	do
	{
		// Ignore current/parent directories
		if (lstrcmp(wfd.cFileName, TEXT(".")) == 0 ||
			lstrcmp(wfd.cFileName, TEXT("..")) == 0)
			continue;

		IO::Path::Combine(szFullPath, szDirectory, wfd.cFileName);

		if (IO::Directory::IsDirectory(szFullPath))
		{
			// Recurse - add the contents of this directory
			if ( g_DaedalusConfig.RecurseRomDirectory && !AddDirectory(szFullPath) )
			{
				// Break/Exit??
			}
		}
		else
		{
			p_ext = IO::Path::FindExtension(wfd.cFileName);

			if (_strcmpi(p_ext, TEXT(".v64")) == 0 ||
				_strcmpi(p_ext, TEXT(".z64")) == 0 ||
				_strcmpi(p_ext, TEXT(".n64")) == 0 ||
				_strcmpi(p_ext, TEXT(".rom")) == 0 ||
				_strcmpi(p_ext, TEXT(".jap")) == 0 ||
				_strcmpi(p_ext, TEXT(".pal")) == 0 ||
				_strcmpi(p_ext, TEXT(".usa")) == 0 ||
				_strcmpi(p_ext, TEXT(".zip")) == 0)
			{
				// Add to the list
				AddItem(szFullPath);
			}
		}

	} while (FindNextFile(hFind, &wfd));

	FindClose(hFind);

	return TRUE;
}


//*****************************************************************************
//
//*****************************************************************************
BOOL IMainWindow::AddItem(const char * szFileName)
{
	LV_ITEM  lvItem;
	CListViewItemInfo * plvii;

	plvii = CListViewItemInfo::Create( szFileName );
	if (plvii == NULL)
		return FALSE;

	ZeroMemory(&lvItem, sizeof(lvItem));

	lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvItem.iItem = ListView_GetItemCount(m_hWndList);	//add the item to the end of the list
	lvItem.lParam = (LPARAM)plvii;
	lvItem.pszText = LPSTR_TEXTCALLBACK;
	lvItem.iImage = I_IMAGECALLBACK;

	ListView_InsertItem(m_hWndList, &lvItem);

	return TRUE;
}

//*****************************************************************************
//
//*****************************************************************************
LRESULT IMainWindow::OnListView_DeleteItem(int idCtrl, LPNMHDR pnmh)
{
	NM_LISTVIEW *lpnmlv = (NM_LISTVIEW*)pnmh;
	CListViewItemInfo * plvii = (CListViewItemInfo *)lpnmlv->lParam;

	//delete the pidl because we made a copy of it
	delete plvii;
	return 0;
}


//*****************************************************************************
//
//*****************************************************************************
LRESULT IMainWindow::OnListView_GetDispInfo(int idCtrl, LPNMHDR pnmh)
{
	LVCOLUMN	column;
	LV_DISPINFO * lpdi = (LV_DISPINFO *)pnmh;
	CListViewItemInfo * plvii = (CListViewItemInfo *)lpdi->item.lParam;

	if ( plvii == NULL )
		return 0;

	// This will initialise the rom info if it has not been done already
	if ( !plvii->IsValid() )
		return 0;


	// Get the column associated with this sub-item
	column.mask = LVCF_SUBITEM;
	if ( !ListView_GetColumn( m_hWndList, lpdi->item.iSubItem, &column ) )
		return 0;


	// Which column is being requested?
	switch ( column.iSubItem )
	{
		case COLUMN_NAME:			if(lpdi->item.mask & LVIF_TEXT)
									{
										lstrcpyn(lpdi->item.pszText, plvii->GetGameName(), lpdi->item.cchTextMax);
									}

									//is the image being requested?
									if(lpdi->item.mask & LVIF_IMAGE)
									{
										lpdi->item.iImage = -1;
									}
									break;

		// Country
		case COLUMN_COUNTRY:		if (lpdi->item.mask & LVIF_TEXT)
									{
										lstrcpyn(lpdi->item.pszText, ROM_GetCountryNameFromID( plvii->GetCountryID() ), lpdi->item.cchTextMax);
									}
									break;

		// Size
		case COLUMN_SIZE:			if (lpdi->item.mask & LVIF_TEXT)
									{
										StrFormatByteSize(plvii->GetRomSize(), lpdi->item.pszText, lpdi->item.cchTextMax);
									}
									break;

		// Comments
		case COLUMN_COMMENT:		if (lpdi->item.mask & LVIF_TEXT)
									{
										lstrcpyn(lpdi->item.pszText, plvii->GetComment(), lpdi->item.cchTextMax);
									}
									break;

		// Info
		case COLUMN_INFO:			if (lpdi->item.mask & LVIF_TEXT)
									{
										lstrcpyn(lpdi->item.pszText, plvii->GetInfo(), lpdi->item.cchTextMax);
									}
									break;

		// Save Type
		case COLUMN_SAVE:			if (lpdi->item.mask & LVIF_TEXT)
									{
										const char * text( "" );
										if( plvii->GetSaveType() != SAVE_TYPE_UNKNOWN )
										{
											text = ROM_GetSaveTypeName( plvii->GetSaveType() );
										}

										lstrcpyn(lpdi->item.pszText, text, lpdi->item.cchTextMax);
									}
									break;

		// Boot Type
		case COLUMN_BOOT:			if (lpdi->item.mask & LVIF_TEXT)
									{
										lstrcpyn(lpdi->item.pszText, ROM_GetCicName( plvii->GetCicType() ), lpdi->item.cchTextMax);
									}
									break;


		default:
			// Huh?
			break;
	}
	return 0;
}

//*****************************************************************************
//
//*****************************************************************************
LRESULT IMainWindow::OnListView_ColumnClick(int idCtrl, LPNMHDR pnmh)
{
	LPNMLISTVIEW	lpnmlv = LPNMLISTVIEW(pnmh);

	// Process LVN_COLUMNCLICK to sort items by column.
	ListView_SortItems(lpnmlv->hdr.hwndFrom, ListView_CompareItems, (LPARAM)(lpnmlv->iSubItem));

	return 0;

}

//*****************************************************************************
//
//*****************************************************************************
LRESULT IMainWindow::OnListView_DblClick(int idCtrl, LPNMHDR pnmh)
{
	LPNMLISTVIEW	lpnmlv = LPNMLISTVIEW(pnmh);

	if (lpnmlv->iItem != -1)
	{
		LV_ITEM lvItem;

		ZeroMemory(&lvItem, sizeof(LV_ITEM));
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = lpnmlv->iItem;

		if (ListView_GetItem(m_hWndList, &lvItem))
		{
			CListViewItemInfo * plvii;
			plvii = (CListViewItemInfo *)lvItem.lParam;

			if (plvii != NULL)
			{
				char reason[100];
				System_Open(plvii->GetFileName());
				/*bSuccess = */CPU_StartThread(reason, 100);
			}
		}
	}

	return 0;

}


//*****************************************************************************
//
//*****************************************************************************
LRESULT IMainWindow::OnListView_RClick(int idCtrl, LPNMHDR pnmh)
{
	LPNMLISTVIEW   lpnmlv = (LPNMLISTVIEW)pnmh;

	if (lpnmlv->iItem != -1)
	{
		LV_ITEM lvItem;

		ZeroMemory(&lvItem, sizeof(LV_ITEM));
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = lpnmlv->iItem;

		if (ListView_GetItem(m_hWndList, &lvItem))
		{
			CListViewItemInfo * plvii;
			plvii = (CListViewItemInfo *)lvItem.lParam;

			if ( plvii && plvii->IsValid() )
			{
				CRomSettings	dlg( plvii->GetRomID() );
				dlg.DoModal( m_hWnd );

				plvii->Invalidate();
				// Refresh
				ListView_Update(m_hWndList, lpnmlv->iItem);
			}

		}
	}

	return 0;

}

//*****************************************************************************
//
//*****************************************************************************
LRESULT IMainWindow::OnListView_Return(int idCtrl, LPNMHDR pnmh)
{
	LPNMLISTVIEW	lpnmlv= (LPNMLISTVIEW)pnmh;

	// Get the currently highlighted item
	LONG			iItem = ListView_GetNextItem(m_hWndList, -1, LVNI_SELECTED);

	if (iItem != -1)
	{
		LV_ITEM lvItem;

		ZeroMemory(&lvItem, sizeof(LV_ITEM));
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = iItem;

		if (ListView_GetItem(m_hWndList, &lvItem))
		{
			CListViewItemInfo * plvii;
			plvii = (CListViewItemInfo *)lvItem.lParam;

			if (plvii != NULL)
			{
				char reason[100];
				System_Open(plvii->GetFileName());
				/*bSuccess = */CPU_StartThread(reason, 100);
			}

		}
	}

	return 0;
}

