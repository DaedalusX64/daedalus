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

// TODO: Add a CriticalSection to stop multiple threads using the 
// Unique objects simultaneously

#include "stdafx.h"
#include "DaedCritSect.h"
#include "DaedalusGraphics.h"
#include "PVRRenderer.h"

#include "Utility\Profiler.h"

#include "DLParser.h"

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
#include "DisplayListDebugger.h"
#endif

#define DAEDALUSGRAPHICS_API __declspec(dllexport)

#define DAEDALUS_GFX_PLUGIN_VERSION		0x0001

#define OS_TV_PAL		0
#define OS_TV_NTSC		1
#define OS_TV_MPAL		2

static bool gDropAllTextures = false;
 bool gDebugDisplayList = false;

CCritSect g_CritialSection;

/******************************************************************
  Function: InitiateGFX
  Purpose:  This function is called when the DLL is started to give
            information from the emulator that the n64 graphics
			uses. This is not called from the emulation thread.
  Input:    Gfx_Info is passed to this function which is defined
            above.
  Output:   TRUE on success
            FALSE on failure to initialise
             
  ** note on interrupts **:
  To generate an interrupt set the appropriate bit in MI_INTR_REG
  and then call the function CheckInterrupts to tell the emulator
  that there is a waiting interrupt.
*******************************************************************/ 
bool PSPGraphics_Initialise()
{
	g_CritialSection.Lock();
	
	if(!PVRRenderer::Create())
	{
		g_CritialSection.Unlock();
		return false;
	}
	
	if(!CTextureCache::Create())
	{
		g_CritialSection.Unlock();
		return false;
	}

	if (!DLParser_Initialise()) 
	{
		g_CritialSection.Unlock();
		return FALSE;
	}

	g_CritialSection.Unlock();
	return true;
}

/******************************************************************

*******************************************************************/ 
void PSPGraphics_Finalise()
{
	g_CritialSection.Lock();

	DLParser_Finalise();
	CTextureCache::Destroy();
	PVRRenderer::Destroy();

	g_CritialSection.Unlock();
}


/******************************************************************

*******************************************************************/ 
void PSPGraphics_ProcessDisplayList(void)
{
	g_CritialSection.Lock();
	
	if (gDropAllTextures)
	{
		CTextureCache::Get()->DropTextures();
		gDropAllTextures = false;
	}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if(gDebugDisplayList)
	{
		CDisplayListDebugger *	p_debugger( CDisplayListDebugger::Create() );
		p_debugger->Run();
		delete p_debugger;
		gDebugDisplayList = false;
	}
	else
#endif
	{
		DLParser_Process();
	}

	g_CritialSection.Unlock();
}

//*****************************************************************************
// Execute an arbitary command (e.g. "Take ScreenShot", "Dump DList")
// pszCommand: [in] Command name
// ppResult: [in/out] Optional auxillary data
// Returns standard HRESULT (S_OK, E_FAIL, E_OUTOFMEMORY etc)
//*****************************************************************************

bool PSPGraphics_ExecuteCommand( const CHAR * pszCommand, void ** ppResult )
{
	bool ok( false );

	g_CritialSection.Lock();

	if ( _strcmpi( pszCommand, DAEDALUS_GFX_SETDAEDALUSVERSION ) == 0 )
	{
		ok = true;
	}
	else if ( _strcmpi( pszCommand, DAEDALUS_GFX_GETDAEDALUSPLUGINVERSION ) == 0 )
	{
		*ppResult = (void*)DAEDALUS_GFX_PLUGIN_VERSION;
		ok = true;
	}
	else if ( _strcmpi( pszCommand, DAEDALUS_GFX_DROPTEXTURES ) == 0 )
	{
		gDropAllTextures = true;
		ok = true;
	}
	else if ( _strcmpi( pszCommand, DAEDALUS_GFX_DUMPTEXTURES ) == 0 )
	{
	}
	else if ( _strcmpi( pszCommand, DAEDALUS_GFX_NODUMPTEXTURES ) == 0 )
	{
	}
	else if ( _strcmpi( pszCommand, DAEDALUS_GFX_USENEWCOMBINER ) == 0)
	{
		g_bDisplayNewCombiner = (BOOL)*ppResult;
	}
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	else if ( _strcmpi( pszCommand, DAEDALUS_GFX_DUMPDL ) == 0 )
	{
		DLParser_DumpNextDisplayList();
	}
#endif
	else if ( _strcmpi( pszCommand, DAEDALUS_GFX_DEBUGDL ) == 0 )
	{
		gDebugDisplayList = true;
	}
	g_CritialSection.Unlock();
	return ok;
}

