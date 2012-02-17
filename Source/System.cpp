/*
Copyright (C) 2009 Howard Su (howard0su@gmail.com)

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

#include "Core/Memory.h"
#include "Core/CPU.h"
#include "Core/Save.h"
#include "Core/PIF.h"
#include "Core/ROMBuffer.h"
#include "Core/RomSettings.h"
#include "Utility/Preferences.h"
#include "Utility/Translate.h"

#include "Interface/RomDB.h"

#include "Graphics/VideoMemoryManager.h"
#include "Graphics/GraphicsContext.h"

#include "Utility/FramerateLimiter.h"
#include "Utility/Synchroniser.h"
#include "Utility/Profiler.h"

#include "Input/InputManager.h"		// CInputManager::Create/Destroy

#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"

#include "Plugins/GraphicsPlugin.h"
#include "Plugins/AudioPlugin.h"

typedef struct 
{
	const char *name;
	bool (*init)();
	void (*final)();
}SysEntityEntry;

typedef struct 
{
	const char *name;
	void (*open)();
	void (*close)();
}RomEntityEntry;


CGraphicsPlugin * gGraphicsPlugin   = NULL;
CAudioPlugin	* g_pAiPlugin		= NULL;
//*****************************************************************************
//
//*****************************************************************************
static void InitAudioPlugin()
{
	DAEDALUS_ASSERT( g_pAiPlugin == NULL, "Why is there already an audio plugin?" );

	CAudioPlugin * audio_plugin( CreateAudioPlugin() );
	if( audio_plugin != NULL )
	{
		if( !audio_plugin->StartEmulation() )
		{
			delete audio_plugin;
			audio_plugin = NULL;
		}
		g_pAiPlugin = audio_plugin;
	}
}

//*****************************************************************************
//
//*****************************************************************************
static void InitGraphicsPlugin()
{
	DAEDALUS_ASSERT( gGraphicsPlugin == NULL, "The graphics plugin should not be initialised at this point" );
	CGraphicsPlugin *	graphics_plugin( CreateGraphicsPlugin() );
	if( graphics_plugin != NULL )
	{
		if( !graphics_plugin->StartEmulation() )
		{
			delete graphics_plugin;
			graphics_plugin = NULL;
		}
		gGraphicsPlugin = graphics_plugin;
	}
}

//*****************************************************************************
//
//*****************************************************************************
static void DisposeGraphicsPlugin()
{
	if ( gGraphicsPlugin != NULL )
	{
		gGraphicsPlugin->RomClosed();
		delete gGraphicsPlugin;
		gGraphicsPlugin = NULL;
	}
}

//*****************************************************************************
//
//*****************************************************************************
static void DisposeAudioPlugin()
{
	CAudioPlugin *audio_plugin(g_pAiPlugin);
	// Make a copy of the plugin, and set the global pointer to NULL;
	// This stops other threads from trying to access the plugin
	// while we're in the process of shutting it down.
	g_pAiPlugin = NULL;
	if (audio_plugin != NULL)
	{
		audio_plugin->StopEmulation();
		delete audio_plugin;
	}
}

//*****************************************************************************
//
//*****************************************************************************
SysEntityEntry SysInitTable[] = 
{
#ifdef DAEDALUS_DEBUG_CONSOLE
	{"DebugConsole", CDebugConsole::Create, CDebugConsole::Destroy},
#endif
#ifdef DAEDALUS_LOG
	{"Logger", Debug_InitLogging,Debug_FinishLogging},
#endif
#ifdef DAEDALUS_ENABLE_PROFILING
	{"Profiler", CProfiler::Create, CProfiler::Destroy},
#endif
	{"ROM Database", CRomDB::Create, CRomDB::Destroy},
	{"ROM Settings", CRomSettingsDB::Create, CRomSettingsDB::Destroy},
	{"InputManager", CInputManager::Create, CInputManager::Destroy},
	{"Language", 	 Translate_Init, NULL},
	{"Preference", CPreferences::Create, CPreferences::Destroy},
	{"Memory", Memory_Init, Memory_Fini},
	{"Controller", CController::Create, CController::Destroy},
	{"VideoMemory", CVideoMemoryManager::Create, NULL},
	{"RomBuffer", RomBuffer::Create, RomBuffer::Destroy},
	{"GraphicsContext", CGraphicsContext::Create,   CGraphicsContext::Destroy},
};

//*****************************************************************************
//
//*****************************************************************************
RomEntityEntry RomInitTable[] = 
{
	{"RomBuffer", RomBuffer::Open, RomBuffer::Close},
	{"Settings", ROM_LoadFile, ROM_UnloadFile},
	{"InputManager", CInputManager::Init, CInputManager::Fini},
	{"Memory", Memory_Reset, Memory_Cleanup},
	{"Audio", InitAudioPlugin, DisposeAudioPlugin },
	{"Graphics", InitGraphicsPlugin, DisposeGraphicsPlugin},
	{"FramerateLimiter", FramerateLimiter_Reset, NULL},
	//{"RSP", RSP_Reset, NULL},
	{"CPU", CPU_Reset, CPU_Finalise},
	{"ROM", ROM_ReBoot, ROM_Unload},
	{"Controller", CController::Reset, CController::RomClose},
	{"Save", Save::Reset, Save::Fini},
#ifdef DAEDALUS_ENABLE_SYNCHRONISATION
	{"CSynchroniser", CSynchroniser::InitialiseSynchroniser, CSynchroniser::Destroy},
#endif

};

//*****************************************************************************
//
//*****************************************************************************
static const int nSysInitEntries = sizeof(SysInitTable) / sizeof(SysEntityEntry);
static const int nRomInitEntries = sizeof(RomInitTable) / sizeof(RomEntityEntry);


//*****************************************************************************
//
//*****************************************************************************
bool System_Init()
{
	for(int i = 0; i < nSysInitEntries; i++)
	{
		if (SysInitTable[i].init == NULL)
			continue;
		
		if (SysInitTable[i].init())
		{
			DBGConsole_Msg(0, "==>Initialized %s", SysInitTable[i].name);
		}
		else
		{
			DBGConsole_Msg(0, "==>Initialize %s Failed", SysInitTable[i].name);
			return false;
		}
	}

	return true;
}

//*****************************************************************************
//
//*****************************************************************************
void System_Open(const char *romname)
{
	strcpy(g_ROM.szFileName, romname);
	for(int i = 0; i < nRomInitEntries; i++)
	{
		if (RomInitTable[i].open == NULL)
			continue;
		
		DBGConsole_Msg(0, "==>Open %s", RomInitTable[i].name);
		RomInitTable[i].open();
	}
}

//*****************************************************************************
//
//*****************************************************************************
void System_Close()
{
	for(int i = nRomInitEntries - 1 ; i >= 0; i--)
	{
		if (RomInitTable[i].close == NULL)
			continue;
		
		DBGConsole_Msg(0, "==>Close %s", RomInitTable[i].name);
		RomInitTable[i].close();
	}
}

//*****************************************************************************
//
//*****************************************************************************
void System_Finalize()
{
	for(int i = nSysInitEntries - 1; i >= 0; i--)
	{
		if (SysInitTable[i].final == NULL)
			continue;
		
		DBGConsole_Msg(0, "==>Finalize %s", SysInitTable[i].name);
		SysInitTable[i].final();
	}
}

