
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


// This file contains many variables that can be used to
// change the overall operation of the emu. They are here for
// convienience, so that they can quickly be changed when
// developing. Generally they will be changed by the ini file
// settings.

#include "stdafx.h"
#include "ConfigOptions.h"

u32		gSpeedSyncEnabled			= 0;		// Enable to limit frame rate.
bool	gGraphicsEnabled			= true;		// Show graphics
bool	gDynarecEnabled				= true;		// Use dynamic recompilation
//bool	gDynarecStackOptimisation	= true;		// Enable the dynarec stack optmisation
bool	gDynarecLoopOptimisation	= false;	// Enable the dynarec loop optmisation
bool	gDynarecDoublesOptimisation	= false;	// Enable the dynarec Doubles optmisation
bool	gOSHooksEnabled				= true;		// Apply os-hooks
u32		gCheckTextureHashFrequency	= 0;		// How often to check textures for updates (every N frames, 0 to disable)
bool	gDoubleDisplayEnabled		= true;		// Workaround for games that have shaking issues
bool	gCleanSceneEnabled			= false;	// Clean our Scenes, it gets rid of many glitches
bool	gAudioRateMatch				= false;	// Matches audio rate with framerate, only works if 50-100% sync rate
bool	gVideoRateMatch				= false;	// Matches VI rate with framerate
bool	gFogEnabled					= false;	// Enable fog
bool    gMemoryAccessOptimisation   = false;    // Enable the memory access optmisation
bool	gCheatsEnabled				= false;	// Enable cheat codes
u32		gCheatFrequency				= 0;		// Cheat frequency to apply
u32		gControllerIndex			= 0;		// Which controller config to set

DaedalusConfig g_DaedalusConfig =
{
	false,			// RecurseRomDirectory	// Recursively scan rom directories?
	false,			// WarnMemoryErrors;	// Warn on all memory access errors?
											// If false, the exception handler is
											// taken without any warnings on the debug console

	true,			// RunAutomatically;	// Run roms automatically after loading

	true,			// TrapExceptions;		// If set, this causes exceptions in the audio
											// plugin, cpu thread and graphics processing to
											// be handled nicely. I turn this off for
											// development to see when things mess up
	0,				// nNumRomsDirs
	// szRomsDir
	// szSaveDir

};
