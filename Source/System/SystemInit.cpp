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


#include "Base/Types.h"


#include "Core/Memory.h"
#include "Core/CPU.h"
#include "Core/Save.h"
#include "Core/PIF.h"
#include "RomFile/ROMBuffer.h"
#include "RomFile/RomSettings.h"

#include "Interface/RomDB.h"
#include "System/SystemInit.h"
#ifdef DAEDALUS_PSP
#include "SysPSP/Graphics/VideoMemoryManager.h"
#endif

#include "Graphics/GraphicsContext.h"

#if defined(DAEDALUS_POSIX) || defined(DAEDALUS_W32)
#include "SysPosix/Debug/WebDebug.h"
#include "HLEGraphics/TextureCacheWebDebug.h"
#include "HLEGraphics/DisplayListDebugger.h"
#endif


#include "Utility/FramerateLimiter.h"
#include "Debug/Synchroniser.h"
#include "Base/Macros.h"
#include "Utility/Profiler.h"
#include "Interface/Preferences.h"
#include "Utility/Translate.h"

#include "Input/InputManager.h"		// CInputManager::Create/Destroy

#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"

#include "HLEGraphics/GraphicsPlugin.h"
#include "HLEAudio/AudioPlugin.h"

#include <array>
#include <memory>


std::unique_ptr<CGraphicsPlugin> gGraphicsPlugin;
std::unique_ptr<CAudioPlugin> gAudioPlugin;

static bool InitAudioPlugin()
{
	std::unique_ptr<CAudioPlugin> audio_plugin = CreateAudioPlugin();
	if( audio_plugin != NULL )
	{
		gAudioPlugin = std::move(audio_plugin);
	}
	gAudioPlugin->StartEmulation();
	return true;
}

static void DisposeAudioPlugin()
{
	if ( gAudioPlugin != NULL )
	{
		gAudioPlugin->StopEmulation();
		gAudioPlugin.release();
		gAudioPlugin = NULL;
	}
}

static bool InitGraphicsPlugin()
{
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( gGraphicsPlugin == NULL, "The graphics plugin should not be initialised at this point" );
	#endif
	std::unique_ptr<CGraphicsPlugin> graphics_plugin = CreateGraphicsPlugin();
	if( graphics_plugin != NULL )
	{
		 gGraphicsPlugin = std::move(graphics_plugin);
	}
	return true;
}

static void DisposeGraphicsPlugin()
{
	if ( gGraphicsPlugin != NULL )
	{
		gGraphicsPlugin->RomClosed();
		gGraphicsPlugin.release();
		gGraphicsPlugin = NULL;
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

static const std::array<SysEntityEntry, 17> gSysInitTable =
{{
#ifdef DAEDALUS_DEBUG_CONSOLE
	{"DebugConsole",		CDebugConsole::Create,		CDebugConsole::Destroy},
#endif
#ifdef DAEDALUS_LOG
	{"Logger",				Debug_InitLogging},
#endif
#ifdef DAEDALUS_ENABLE_PROFILING
	{"Profiler",			Profiler_Init,				Profiler_Fini},
#endif
	{"ROM Database",		CRomDB::Create,				CRomDB::Destroy},
	{"ROM Settings",		CRomSettingsDB::Create,		CRomSettingsDB::Destroy},
	{"InputManager",		CInputManager::Create,		CInputManager::Destroy},
	#ifndef DAEDALUS_CTR
	{"Language",			Translate_Init,				NULL},
	#endif
#ifdef DAEDALUS_PSP
	{"VideoMemory",			CVideoMemoryManager::Create, NULL},

#endif
	{"GraphicsContext",		CGraphicsContext::Create,	CGraphicsContext::Destroy},
	{"Preference",			CPreferences::Create,		CPreferences::Destroy},
	{"Memory",				Memory_Init,				Memory_Fini},
	{"Controller",			CController::Create,		CController::Destroy},
	{"RomBuffer",			RomBuffer::Create,			RomBuffer::Destroy},

#if defined(DAEDALUS_POSIX) || defined(DAEDALUS_W32)
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	{"WebDebug",			WebDebug_Init, 				WebDebug_Fini},
	{"TextureCacheWebDebug",TextureCache_RegisterWebDebug, 	NULL},
	{"DLDebuggerWebDebug",	DLDebugger_RegisterWebDebug, 	NULL},
#endif
#endif
}};

struct RomEntityEntry
{
	const char *name;
	bool (*open)();
	void (*close)();
};
static const std::array<RomEntityEntry, 12> gRomInitTable =
{{
	{"RomBuffer",			RomBuffer::Open, 		RomBuffer::Close},
	{"Settings",			ROM_LoadFile,			ROM_UnloadFile},
	{"InputManager",		CInputManager::Init,	CInputManager::Fini},
	{"Memory",				Memory_Reset,			Memory_Cleanup},
	{"Audio",				InitAudioPlugin, 		DisposeAudioPlugin},
	{"Graphics",			InitGraphicsPlugin, 	DisposeGraphicsPlugin},
	{"FramerateLimiter",	FramerateLimiter_Reset,	NULL},
	//{"RSP", RSP_Reset, NULL},
	{"CPU",					CPU_RomOpen,				CPU_RomClose},
	{"ROM",					ROM_ReBoot,				ROM_Unload},
	{"Controller",			CController::Reset,		CController::RomClose},
	{"Save",				Save_Reset,				Save_Fini},
#ifdef DAEDALUS_ENABLE_SYNCHRONISATION
	{"CSynchroniser",		CSynchroniser::InitialiseSynchroniser, CSynchroniser::Destroy},
#endif
}
}; 
bool System_Init()
{
	for(u32 i = 0; i < gSysInitTable.size(); i++)
	{
		const SysEntityEntry & entry = gSysInitTable[i];

		if (entry.init == NULL)
			continue;

		if (entry.init())
		{
			#ifdef DAEDALUS_DEBUG_CONSOLE
			DBGConsole_Msg(0, "==>Initialized %s", entry.name);
			#endif
		}
		else
		{
				#ifdef DAEDALUS_DEBUG_CONSOLE
			DBGConsole_Msg(0, "==>Initialize %s Failed", entry.name);
			#endif
			return false;
		}
	}

	return true;
}

bool System_Open(const std::filesystem::path &filename)
{
	g_ROM.mFileName = filename;
	
	for (u32 i = 0; i < gRomInitTable.size(); i++)
	{
		const RomEntityEntry & entry = gRomInitTable[i];

		if (entry.open == NULL)
			continue;
	#ifdef DAEDALUS_DEBUG_CONSOLE
		DBGConsole_Msg(0, "==>Open %s", entry.name);
	#endif
		if (!entry.open())
		{
				#ifdef DAEDALUS_DEBUG_CONSOLE
			DBGConsole_Msg(0, "==>Open %s [RFAILED]", entry.name);
			#endif
			return false;
		}
	}

	return true;
}

void System_Close()
{
	for(s32 i = gRomInitTable.size() - 1 ; i >= 0; i--)
	{
		const RomEntityEntry & entry = gRomInitTable[i];

		if (entry.close == NULL)
			continue;
	#ifdef DAEDALUS_DEBUG_CONSOLE
		DBGConsole_Msg(0, "==>Close %s", entry.name);
		#endif
		entry.close();
	}
}

void System_Finalize()
{
	for(s32 i = gSysInitTable.size() - 1; i >= 0; i--)
	{
		const SysEntityEntry & entry = gSysInitTable[i];

		if (entry.final == NULL)
			continue;
	#ifdef DAEDALUS_DEBUG_CONSOLE
		DBGConsole_Msg(0, "==>Finalize %s", entry.name);
		#endif
		entry.final();
	}
}
