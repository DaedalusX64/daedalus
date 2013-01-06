/*
Copyright (C) 2007 StrmnNrmn

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
#include "GraphicsPluginW32.h"

#include "Interface/MainWindow.h"
#include "Core/Interrupt.h"
#include "Core/Memory.h"
#include "Core/ROM.h"

#include "ConfigOptions.h"

//*****************************************************************************
//
//*****************************************************************************
CGraphicsPluginDll *	CGraphicsPluginDll::Create( const char * dll_name )
{
	CGraphicsPluginDll *	plugin( new CGraphicsPluginDll );
	if ( plugin->Open( dll_name ) )
	{
		if ( plugin->Init() )
		{
			return plugin;
		}

		plugin->Close();
	}

	delete plugin;
	return NULL;
}

//*****************************************************************************
// Construction
//*****************************************************************************
CGraphicsPluginDll::CGraphicsPluginDll() :
	m_pViWidthChanged(NULL),
	m_pViStatusChanged(NULL),
	m_pMoveScreen(NULL),
	m_pDrawScreen(NULL),
	m_pChangeWindow(NULL),
	m_pUpdateScreen(NULL),
	m_pCloseDll(NULL),
	m_pDllAbout(NULL),
	m_pDllConfig(NULL),
	m_pDllTest(NULL),
	m_pInitiateGFX(NULL),
	m_pProcessDList(NULL),
	m_pRomClosed(NULL),
	m_pRomOpen(NULL),

	m_pExecuteCommand(NULL),

	m_hModule(NULL),
	m_bLoadedOk(FALSE)

{
	lstrcpyn(mModuleName, "", MAX_PATH);
}


//*****************************************************************************
// Destruction
//*****************************************************************************
CGraphicsPluginDll::~CGraphicsPluginDll()
{
	Close();
}

//*****************************************************************************
//
//*****************************************************************************
bool	CGraphicsPluginDll::StartEmulation()
{
	RomOpen();
	ExecuteCommand( DAEDALUS_GFX_SET_DEBUG, CDebugConsole::Get() );
	return true;
}

//*****************************************************************************
//
//*****************************************************************************
static void __cdecl CheckInterrupts(void)
{
	//DBGConsole_Msg(0, "GFX Plugin called Check Interrupts");
	R4300_Interrupt_UpdateCause3();
}


//*****************************************************************************
//
//*****************************************************************************
BOOL CGraphicsPluginDll::GetPluginInfo( const char * szFilename, PLUGIN_INFO & pi )
{
	HMODULE hModule = LoadLibrary(szFilename);
	if (hModule == NULL)
		return FALSE;

	void   (PLUGIN_SPEC_CALL * pGetDllInfo)(PLUGIN_INFO *);

	pGetDllInfo = (void (PLUGIN_SPEC_CALL *)(PLUGIN_INFO *))GetProcAddress(hModule, "GetDllInfo");
	if (pGetDllInfo == NULL)
	{
		FreeLibrary(hModule);
		return FALSE;
	}

	pGetDllInfo(&pi);
	if (pi.Type != PLUGIN_TYPE_GFX)
	{
		FreeLibrary(hModule);
		return FALSE;
	}

	FreeLibrary(hModule);

	return TRUE;
}

//*****************************************************************************
//
//*****************************************************************************
BOOL CGraphicsPluginDll::Open( const char * dll_name )
{
	lstrcpyn(mModuleName, dll_name, MAX_PATH);

	if (!GetPluginInfo( mModuleName, m_pi ) )
		return FALSE;

	m_hModule = LoadLibrary(mModuleName);
	if (m_hModule == NULL)
		return FALSE;

    m_pViWidthChanged = (void   (PLUGIN_SPEC_CALL * )(void))GetProcAddress(m_hModule, "ViWidthChanged");
    m_pViStatusChanged= (void   (PLUGIN_SPEC_CALL * )(void))GetProcAddress(m_hModule, "ViStatusChanged");
    m_pMoveScreen     = (void   (PLUGIN_SPEC_CALL * )(int,int))GetProcAddress(m_hModule, "MoveScreen");
    m_pDrawScreen     = (void   (PLUGIN_SPEC_CALL * )(void))GetProcAddress(m_hModule, "DrawScreen");
    m_pChangeWindow   = (void   (PLUGIN_SPEC_CALL * )(void))GetProcAddress(m_hModule, "ChangeWindow");
    m_pUpdateScreen   = (void   (PLUGIN_SPEC_CALL * )(void))GetProcAddress(m_hModule, "UpdateScreen");
    m_pCloseDll       = (void   (PLUGIN_SPEC_CALL * )(void))GetProcAddress(m_hModule, "CloseDLL");
    m_pDllAbout       = (void   (PLUGIN_SPEC_CALL * )(HWND))GetProcAddress(m_hModule, "DllAbout");
    m_pDllConfig      = (void   (PLUGIN_SPEC_CALL * )(HWND))GetProcAddress(m_hModule, "DllConfig");
    m_pDllTest        = (void   (PLUGIN_SPEC_CALL * )(HWND))GetProcAddress(m_hModule, "DllTest");
    m_pInitiateGFX	= (BOOL   (PLUGIN_SPEC_CALL * )(GFX_INFO))GetProcAddress(m_hModule, "InitiateGFX");
    m_pProcessDList   = (void   (PLUGIN_SPEC_CALL * )(void))GetProcAddress(m_hModule, "ProcessDList");
    m_pRomClosed      = (void   (PLUGIN_SPEC_CALL * )(void))GetProcAddress(m_hModule, "RomClosed");
    m_pRomOpen		= (void   (PLUGIN_SPEC_CALL * )(void))GetProcAddress(m_hModule, "RomOpen");

	m_pExecuteCommand = (HRESULT (PLUGIN_SPEC_CALL * )( const CHAR * pszCommand, void * pResult))GetProcAddress( m_hModule, "ExecuteCommand" );

	m_bLoadedOk = TRUE;

	return TRUE;
}

//*****************************************************************************
//
//*****************************************************************************
void CGraphicsPluginDll::Close()
{
	CloseDll();

	m_bLoadedOk = FALSE;

	if (m_hModule != NULL)
	{
		//CloseDll();
		FreeLibrary(m_hModule);
	}

    m_pViWidthChanged = NULL;
    m_pViStatusChanged = NULL;
    m_pMoveScreen = NULL;
    m_pDrawScreen = NULL;
    m_pChangeWindow = NULL;
    m_pUpdateScreen = NULL;
    m_pCloseDll = NULL;
    m_pDllAbout = NULL;
    m_pDllConfig = NULL;
    m_pDllTest = NULL;
    m_pInitiateGFX = NULL;
    m_pProcessDList = NULL;
    m_pRomClosed = NULL;
    m_pRomOpen = NULL;
	m_hModule = NULL;
}

//*****************************************************************************
//
//*****************************************************************************
BOOL CGraphicsPluginDll::Init()
{
	GFX_INFO gfx;

	gfx.hWnd = CMainWindow::Get()->GetWindow();
	gfx.hStatusBar = CMainWindow::Get()->GetStatusWindow();
	gfx.MemoryBswaped = TRUE;
	gfx.HEADER = (BYTE*)&g_ROM.rh;
	gfx.RDRAM = (BYTE*)g_pMemoryBuffers[MEM_RD_RAM];
	gfx.DMEM = (BYTE*)g_pMemoryBuffers[MEM_SP_MEM] + SP_DMA_DMEM;
	gfx.IMEM = (BYTE*)g_pMemoryBuffers[MEM_SP_MEM] + SP_DMA_IMEM;

	gfx.xMI_INTR_REG = MI_REG_ADDRESS(MI_INTR_REG);

	gfx.xDPC_START_REG = DPC_REG_ADDRESS(DPC_START_REG);
	gfx.xDPC_END_REG = DPC_REG_ADDRESS(DPC_END_REG);
	gfx.xDPC_CURRENT_REG = DPC_REG_ADDRESS(DPC_CURRENT_REG);
	gfx.xDPC_STATUS_REG = DPC_REG_ADDRESS(DPC_STATUS_REG);
	gfx.xDPC_CLOCK_REG = DPC_REG_ADDRESS(DPC_CLOCK_REG);
	gfx.xDPC_BUFBUSY_REG = DPC_REG_ADDRESS(DPC_BUFBUSY_REG);
	gfx.xDPC_PIPEBUSY_REG = DPC_REG_ADDRESS(DPC_PIPEBUSY_REG);
	gfx.xDPC_TMEM_REG = DPC_REG_ADDRESS(DPC_TMEM_REG);

	gfx.xVI_STATUS_REG = VI_REG_ADDRESS(VI_STATUS_REG);
	gfx.xVI_ORIGIN_REG = VI_REG_ADDRESS(VI_ORIGIN_REG);
	gfx.xVI_WIDTH_REG = VI_REG_ADDRESS(VI_WIDTH_REG);
	gfx.xVI_INTR_REG = VI_REG_ADDRESS(VI_INTR_REG);
	gfx.xVI_V_CURRENT_LINE_REG = VI_REG_ADDRESS(VI_V_CURRENT_LINE_REG);
	gfx.xVI_TIMING_REG = VI_REG_ADDRESS(VI_TIMING_REG);
	gfx.xVI_V_SYNC_REG = VI_REG_ADDRESS(VI_V_SYNC_REG);
	gfx.xVI_H_SYNC_REG = VI_REG_ADDRESS(VI_H_SYNC_REG);
	gfx.xVI_LEAP_REG = VI_REG_ADDRESS(VI_LEAP_REG);
	gfx.xVI_H_START_REG = VI_REG_ADDRESS(VI_H_START_REG);
	gfx.xVI_V_START_REG = VI_REG_ADDRESS(VI_V_START_REG);
	gfx.xVI_V_BURST_REG = VI_REG_ADDRESS(VI_V_BURST_REG);
	gfx.xVI_X_SCALE_REG = VI_REG_ADDRESS(VI_X_SCALE_REG);
	gfx.xVI_Y_SCALE_REG = VI_REG_ADDRESS(VI_Y_SCALE_REG);


	gfx.CheckInterrupts = CheckInterrupts;

	BOOL bGFX = InitiateGFX(gfx);
	if (!bGFX)
	{
		DBGConsole_Msg(0, "Unable to initialise gfx plugin");
		return FALSE;
	}
	else
	{
		DBGConsole_Msg(0, "Graphics plugin initialised ok");
		return TRUE;
	}

}

//*****************************************************************************
//
//*****************************************************************************
CGraphicsPlugin *		CreateGraphicsPlugin()
{
	CGraphicsPluginDll *	plugin( NULL );
	const char *			plugin_name( g_DaedalusConfig.szGfxPluginFileName );

	if (strlen( plugin_name ) > 0)
	{
		DBGConsole_Msg( 0, "Initialising Graphics Plugin [C%s]", plugin_name );

		plugin = CGraphicsPluginDll::Create( plugin_name );
		if ( plugin == NULL )
		{
			DBGConsole_Msg( 0, "Error loading graphics plugin" );
		}
	}

	return plugin;
}
