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

#pragma once

#ifndef UTILITY_ROM_PREFERENCES_H_
#define UTILITY_ROM_PREFERENCES_H_

#include "Interface/ConfigOptions.h"


enum ETextureHashFrequency
{
	THF_DISABLED = 0,
	THF_EVERY_FRAME,
	THF_EVERY_2,
	THF_EVERY_4,
	THF_EVERY_8,
	THF_EVERY_16,
	THF_EVERY_32,

	NUM_THF,
};

enum ECheatFrequency
{
	CF_EVERY_FRAME = 0,
	CF_EVERY_4,
	CF_EVERY_8,
	CF_EVERY_16,
	CF_EVERY_32,
	CF_EVERY_64,

	NUM_CF,
};

enum EFrameskipValue
{
	FV_DISABLED = 0,
	FV_AUTO1,
	FV_AUTO2,
	FV_1,
	FV_2,
	FV_3,
	FV_4,
	FV_5,
	FV_6,
	FV_7,
	FV_8,
	FV_9,
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	FV_99,
#endif
	NUM_FRAMESKIP_VALUES,
};




struct SRomPreferences
{
	bool						PatchesEnabled;
	bool						DynarecEnabled;				// Requires DynarceSupported in RomSettings
	bool						DynarecLoopOptimisation;
	bool						DynarecDoublesOptimisation;
	bool						DoubleDisplayEnabled;
	bool						CleanSceneEnabled;
	bool						ClearDepthFrameBuffer;
	bool						AudioRateMatch;
	bool						VideoRateMatch;
	bool						FogEnabled;
	bool                        MemoryAccessOptimisation;
	bool						CheatsEnabled;
//	bool						AudioAdaptFrequency;
	ETextureHashFrequency		CheckTextureHashFrequency;
	EFrameskipValue				Frameskip;
	EAudioPluginMode			AudioEnabled;
	f32							ZoomX;
	u32							SpeedSyncEnabled;
	u32							ControllerIndex;
//	u32							PAD1;	//Some Bug in GCC that require to pad the struct some times...(?)

	SRomPreferences();

	void		Reset();
	void		Apply() const;
};

#endif //INTERFACE_ROM_PREFERENCES