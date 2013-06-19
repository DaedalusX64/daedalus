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

#include "Interface/RomDB.h"
#ifdef DAEDALUS_PSP
#include "Graphics/VideoMemoryManager.h"
#endif

#include "Graphics/GraphicsContext.h"

#if defined(DAEDALUS_OSX) || defined(DAEDALUS_W32)
#include "SysOSX/Debug/WebDebug.h"
#include "HLEGraphics/TextureCacheWebDebug.h"
#include "HLEGraphics/DisplayListDebugger.h"
#endif

#ifdef DAEDALUS_GL
#include "SysGL/Interface/UI.h"
#endif

#include "Utility/FramerateLimiter.h"
#include "Utility/Synchroniser.h"
#include "Utility/Macros.h"
#include "Utility/Profiler.h"
#include "Utility/Preferences.h"
#ifdef DAEDALUS_PSP
#include "Utility/Translate.h"
#endif
#include "Input/InputManager.h"		// CInputManager::Create/Destroy

#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"

#include "Plugins/GraphicsPlugin.h"
#include "Plugins/AudioPlugin.h"

CGraphicsPlugin * gGraphicsPlugin   = NULL;
CAudioPlugin	* gAudioPlugin		= NULL;

static bool InitAudioPlugin()
{
	DAEDALUS_ASSERT( gAudioPlugin == NULL, "Why is there already an audio plugin?" );
	CAudioPlugin * audio_plugin = CreateAudioPlugin();
	if( audio_plugin != NULL )
	{
		if( !audio_plugin->StartEmulation() )
		{
			delete audio_plugin;
			audio_plugin = NULL;
		}
		gAudioPlugin = audio_plugin;
	}
	return true;
}

static bool InitGraphicsPlugin()
{
	DAEDALUS_ASSERT( gGraphicsPlugin == NULL, "The graphics plugin should not be initialised at this point" );
	CGraphicsPlugin * graphics_plugin = CreateGraphicsPlugin();
	if( graphics_plugin != NULL )
	{
		if( !graphics_plugin->StartEmulation() )
		{
			delete graphics_plugin;
			graphics_plugin = NULL;
		}
		gGraphicsPlugin = graphics_plugin;
	}
	return true;
}

static void DisposeGraphicsPlugin()
{
	if ( gGraphicsPlugin != NULL )
	{
		gGraphicsPlugin->RomClosed();
		delete gGraphicsPlugin;
		gGraphicsPlugin = NULL;
	}
}

static void DisposeAudioPlugin()
{
	// Make a copy of the plugin, and set the global pointer to NULL;
	// This stops other threads from trying to access the plugin
	// while we're in the process of shutting it down.
	CAudioPlugin * audio_plugin = gAudioPlugin;
	gAudioPlugin = NULL;
	if (audio_plugin != NULL)
	{
		audio_plugin->StopEmulation();
		delete audio_plugin;
	}
}

struct SysEntityEntry
{
	const char *name;
	bool (*init)();
	void (*final)();
};

#ifdef DAEDALUS_ENABLE_PROFILING
static void ProfilerVblCallback(void * arg)
{
	CProfiler::Get()->Update();
	CProfiler::Get()->Display();
}

static bool Profiler_Init()
{
	if (!CProfiler::Create())
		return false;

	CPU_RegisterVblCallback(&ProfilerVblCallback, NULL);

	return true;
}

static void Profiler_Fini()
{
	CPU_UnregisterVblCallback(&ProfilerVblCallback, NULL);
	CProfiler::Destroy();
}
#endif

static const SysEntityEntry gSysInitTable[] =
{
#ifdef DAEDALUS_DEBUG_CONSOLE
	{"DebugConsole",		CDebugConsole::Create,		CDebugConsole::Destroy},
#endif
#ifdef DAEDALUS_LOG
	{"Logger",				Debug_InitLogging,			Debug_FinishLogging},
#endif
#ifdef DAEDALUS_ENABLE_PROFILING
	{"Profiler",			Profiler_Init,				Profiler_Fini},
#endif
	{"ROM Database",		CRomDB::Create,				CRomDB::Destroy},
	{"ROM Settings",		CRomSettingsDB::Create,		CRomSettingsDB::Destroy},
	{"InputManager",		CInputManager::Create,		CInputManager::Destroy},
#ifdef DAEDALUS_PSP
	{"VideoMemory",			CVideoMemoryManager::Create, NULL},
#endif
	{"GraphicsContext",		CGraphicsContext::Create,	CGraphicsContext::Destroy},
#ifdef DAEDALUS_PSP
	{"Language",			Translate_Init,				NULL},
#endif
	{"Preference",			CPreferences::Create,		CPreferences::Destroy},
	{"Memory",				Memory_Init,				Memory_Fini},

	{"Controller",			CController::Create,		CController::Destroy},
	{"RomBuffer",			RomBuffer::Create,			RomBuffer::Destroy},

#if defined(DAEDALUS_OSX) || defined(DAEDALUS_W32)
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	{"WebDebug",			WebDebug_Init, 				WebDebug_Fini},
	{"TextureCacheWebDebug",TextureCache_RegisterWebDebug, 	NULL},
	{"DLDebuggerWebDebug",	DLDebugger_RegisterWebDebug, 	NULL},
#endif
#endif

#ifdef DAEDALUS_GL
	{"UI",					UI_Init,				 	UI_Finalise},
#endif
};

struct RomEntityEntry
{
	const char *name;
	bool (*open)();
	void (*close)();
};

static const RomEntityEntry gRomInitTable[] =
{
	{"RomBuffer",			RomBuffer::Open, 		RomBuffer::Close},
	{"Settings",			ROM_LoadFile,			ROM_UnloadFile},
	{"InputManager",		CInputManager::Init,	CInputManager::Fini},
	{"Memory",				Memory_Reset,			Memory_Cleanup},
	{"Audio",				InitAudioPlugin,		DisposeAudioPlugin},
	{"Graphics",			InitGraphicsPlugin,		DisposeGraphicsPlugin},
	{"FramerateLimiter",	FramerateLimiter_Reset,	NULL},
	//{"RSP", RSP_Reset, NULL},
	{"CPU",					CPU_RomOpen,			CPU_RomClose},
	{"ROM",					ROM_ReBoot,				ROM_Unload},
	{"Controller",			CController::Reset,		CController::RomClose},
	{"Save",				Save_Reset,				Save_Fini},
#ifdef DAEDALUS_ENABLE_SYNCHRONISATION
	{"CSynchroniser",		CSynchroniser::InitialiseSynchroniser, CSynchroniser::Destroy},
#endif
};

bool System_Init()
{
	for(u32 i = 0; i < ARRAYSIZE(gSysInitTable); i++)
	{
		const SysEntityEntry & entry = gSysInitTable[i];

		if (entry.init == NULL)
			continue;

		if (entry.init())
		{
			DBGConsole_Msg(0, "==>Initialized %s", entry.name);
		}
		else
		{
			DBGConsole_Msg(0, "==>Initialize %s Failed", entry.name);
			return false;
		}
	}

	return true;
}

bool System_Open(const char * filename)
{
	strcpy(g_ROM.mFileName, filename);
	for(u32 i = 0; i < ARRAYSIZE(gRomInitTable); i++)
	{
		const RomEntityEntry & entry = gRomInitTable[i];

		if (entry.open == NULL)
			continue;

		DBGConsole_Msg(0, "==>Open %s", entry.name);
		if (!entry.open())
		{
			DBGConsole_Msg(0, "==>Open %s [RFAILED]", entry.name);
			return false;
		}
	}

	return true;
}

void System_Close()
{
	for(s32 i = ARRAYSIZE(gRomInitTable) - 1 ; i >= 0; i--)
	{
		const RomEntityEntry & entry = gRomInitTable[i];

		if (entry.close == NULL)
			continue;

		DBGConsole_Msg(0, "==>Close %s", entry.name);
		entry.close();
	}
}

void System_Finalize()
{
	for(s32 i = ARRAYSIZE(gSysInitTable) - 1; i >= 0; i--)
	{
		const SysEntityEntry & entry = gSysInitTable[i];

		if (entry.final == NULL)
			continue;

		DBGConsole_Msg(0, "==>Finalize %s", entry.name);
		entry.final();
	}
}

