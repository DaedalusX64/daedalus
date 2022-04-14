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

#include <cstdio>

#include "Debug/DBGConsole.h"

#include "HLEGraphics/BaseRenderer.h"
#include "HLEGraphics/TextureCache.h"
#include "HLEGraphics/DLParser.h"
#include "HLEGraphics/DisplayListDebugger.h"

#include "Graphics/GraphicsContext.h"
#include "Plugins/GraphicsPlugin.h"

#include "Utility/Profiler.h"
#include "Utility/FramerateLimiter.h"
#include "Utility/Preferences.h"
#include "Utility/Timing.h"

#include "Core/Memory.h"

//#define DAEDALUS_FRAMERATE_ANALYSIS

extern bool gFrameskipActive;

u32		gSoundSync = 44100;
u32		gVISyncRate = 1500;
bool	gTakeScreenshot = false;
bool	gTakeScreenshotSS = false;

EFrameskipValue		gFrameskipValue = FV_DISABLED;

float				gCurrentFramerate = 0.0f;

namespace
{
	//u32					gVblCount = 0;
	u32					gFlipCount = 0;
	//float				gCurrentVblrate = 0.0f;
	u64					gLastFramerateCalcTime = 0;
	u64					gTicksPerSecond = 0;

#ifdef DAEDALUS_FRAMERATE_ANALYSIS
	u32					gTotalFrames = 0;
	u64					gFirstFrameTime = 0;
	FILE *				gFramerateFile = nullptr;
#endif

static void	UpdateFramerate()
{
#ifdef DAEDALUS_FRAMERATE_ANALYSIS
	gTotalFrames++;
#endif
	gFlipCount++;

	u64			now {};
	NTiming::GetPreciseTime( &now );

	if(gLastFramerateCalcTime == 0)
	{
		u64		freq {};
		gLastFramerateCalcTime = now;

		NTiming::GetPreciseFrequency( &freq );
		gTicksPerSecond = freq;
	}

#ifdef DAEDALUS_FRAMERATE_ANALYSIS
	if( gFramerateFile == nullptr )
	{
		gFirstFrameTime = now;
		gFramerateFile = fopen( "ux0:data/DaedalusX64/framerate.csv", "w" );
	}
	fprintf( gFramerateFile, "%d,%f\n", gTotalFrames, f32(now - gFirstFrameTime) / f32(gTicksPerSecond) );
#endif

	// If 1 second has elapsed since last recalculation, do it now
	u64		ticks_since_recalc( now - gLastFramerateCalcTime );
	if(ticks_since_recalc > gTicksPerSecond)
	{
		//gCurrentVblrate = float( gVblCount * gTicksPerSecond ) / float( ticks_since_recalc );
		gCurrentFramerate = float( gFlipCount * gTicksPerSecond ) / float( ticks_since_recalc );

		//gVblCount = 0;
		gFlipCount = 0;
		gLastFramerateCalcTime = now;

#ifdef DAEDALUS_FRAMERATE_ANALYSIS
		if( gFramerateFile != nullptr )
		{
			fflush( gFramerateFile );
		}
#endif
	}

}
}

class CGraphicsPluginImpl : public CGraphicsPlugin
{
	public:
	CGraphicsPluginImpl();
	~CGraphicsPluginImpl();

		bool		Initialise();
		
		virtual bool		StartEmulation()		{ return true; }
		virtual void		ViStatusChanged()		{}
		virtual void		ViWidthChanged()		{}
		virtual void		ProcessDList();

		virtual void		UpdateScreen();

		virtual void		RomClosed();
private:
		u32					LastOrigin;
};

CGraphicsPluginImpl::CGraphicsPluginImpl():	LastOrigin( 0 )
{
}

CGraphicsPluginImpl::~CGraphicsPluginImpl()
{
}

bool CGraphicsPluginImpl::Initialise()
{
	if(!CreateRenderer())
	{
		return false;
	}

	if(!CTextureCache::Create())
	{
		return false;
	}

	if (!DLParser_Initialise())
	{
		return false;
	}

	return true;
}

void CGraphicsPluginImpl::ProcessDList()
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (!DLDebugger_Process())
	{
		DLParser_Process();
	}
#else
	DLParser_Process();
#endif
}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
extern u32 gNumInstructionsExecuted;
extern u32 gNumDListsCulled;
extern u32 gNumRectsClipped;
#endif

void CGraphicsPluginImpl::UpdateScreen()
{
	u32 current_origin = Memory_VI_GetRegister(VI_ORIGIN_REG);
	static bool Old_FrameskipActive = false;
	static bool Older_FrameskipActive = false;
	
	if( current_origin != LastOrigin)
	{
		if( gGlobalPreferences.DisplayFramerate )
			UpdateFramerate();

		const f32 Fsync = FramerateLimiter_GetSync();

		const f32 inv_Fsync = 1.0f / Fsync;
		gSoundSync = (u32)(44100.0f * inv_Fsync);
		gVISyncRate = (u32)(1500.0f * inv_Fsync);
		if( gVISyncRate > 4000 ) gVISyncRate = 4000;
		else if ( gVISyncRate < 1500 ) gVISyncRate = 1500;
		
		if(!gFrameskipActive)
		{
			if( gGlobalPreferences.DisplayFramerate )
			{
				printf("\x1b[1;1H");
				
				switch(gGlobalPreferences.DisplayFramerate)
				{
					case 1:
						printf( "%#.1f  ", gCurrentFramerate );
						break;
				}
			}

			CGraphicsContext::Get()->UpdateFrame( false );
		}

		static u32 current_frame = 0;
		current_frame++;
	
		Older_FrameskipActive = Old_FrameskipActive;
		Old_FrameskipActive = gFrameskipActive;

		switch(gFrameskipValue)
		{
		case FV_DISABLED:
			gFrameskipActive = false;
			break;
		case FV_AUTO1:
			if(!Old_FrameskipActive && (Fsync < 0.965f)) gFrameskipActive = true;
			else gFrameskipActive = false;
			break;
		case FV_AUTO2:
			if((!Old_FrameskipActive | !Older_FrameskipActive) && (Fsync < 0.965f)) gFrameskipActive = true;
			else gFrameskipActive = false;
			break;
		default:
			gFrameskipActive = (current_frame % (gFrameskipValue - 1)) != 0;
			break;
		}

		LastOrigin = current_origin;
	}
}

void CGraphicsPluginImpl::RomClosed()
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DBGConsole_Msg(0, "Finalising Graphics");
	#endif
	DLParser_Finalise();
	CTextureCache::Destroy();
	DestroyRenderer();
}

CGraphicsPlugin * CreateGraphicsPlugin()
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DBGConsole_Msg( 0, "Initialising Graphics Plugin [CTR]" );
#endif

	CGraphicsPluginImpl * plugin = new CGraphicsPluginImpl;
	if( !plugin->Initialise() )
	{
		delete plugin;
		plugin = nullptr;
	}

	return plugin;
}
