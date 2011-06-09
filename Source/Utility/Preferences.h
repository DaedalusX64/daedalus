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

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef PREFERENCES_H_
#define PREFERENCES_H_

#include "ConfigOptions.h"

enum EGuiType
{
	COVERFLOW=0,
	CLASSIC,
};

const u32 NUM_GUI_TYPES = CLASSIC+1;

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
#ifndef DAEDALUS_PUBLIC_RELEASE
	FV_99,
#endif
	NUM_FRAMESKIP_VALUES,
};

enum EGuiColor
{
	BLACK=0, // Default..
	RED,
	GREEN,
	MAGENTA,
	BLUE,
	TURQUOISE,
	ORANGE,
	PURPLE,
	GREY,
};
const u32 NUM_COLOR_TYPES = GREY+1;


enum EViewportType
{
	VT_UNSCALED_4_3 = 0,
	VT_SCALED_4_3,
	VT_FULLSCREEN,
};
const u32 NUM_VIEWPORT_TYPES = VT_FULLSCREEN+1;

enum ETVType
{
	TT_4_3 = 0,
	TT_WIDESCREEN,
};

struct SGlobalPreferences
{
	u32							DisplayFramerate;
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	bool						HighlightInexactBlendModes;
	bool						CustomBlendModes;
#endif
	bool						BatteryWarning;
	bool						LargeROMBuffer;
	bool						ForceLinearFilter;
	bool						RumblePak;

	EGuiType					GuiType;
	EGuiColor					GuiColor;

	float						StickMinDeadzone;
	float						StickMaxDeadzone;

	EViewportType				ViewportType;

	bool						TVEnable;
	bool						TVLaced;
	ETVType						TVType;

	SGlobalPreferences();

	void		Apply() const;
};

extern SGlobalPreferences	gGlobalPreferences;


struct SRomPreferences
{
	bool						PatchesEnabled;
	bool						DynarecEnabled;				// Requires DynarceSupported in RomSettings
	bool						DynarecStackOptimisation;
	bool						DynarecLoopOptimisation;
	bool						DoubleDisplayEnabled;
	bool						SimulateDoubleDisabled;
	bool						CleanSceneEnabled;
	bool						AudioRateMatch;
	bool						FogEnabled;
	bool                        MemoryAccessOptimisation;
	bool						CheatsEnabled;
//	bool						AudioAdaptFrequency;
	ETextureHashFrequency		CheckTextureHashFrequency;
	EFrameskipValue				Frameskip;
	EAudioPluginMode			AudioEnabled;
	ECheatFrequency				CheatFrequency;
	f32							ZoomX;
	u32							SpeedSyncEnabled;
	u32							ControllerIndex;
//	u32							PAD1;	//Some Bug in GCC that require to pad the struct some times...(?)

	SRomPreferences();

	void		Reset();
	void		Apply() const;
};

#include "Utility/Singleton.h"

//*****************************************************************************
//
//*****************************************************************************
class	RomID;

//*****************************************************************************
//
//*****************************************************************************
class CPreferences : public CSingleton< CPreferences >
{
	public:
		virtual					~CPreferences() {}

		virtual bool			OpenPreferencesFile( const char * filename ) = 0;
		virtual void			Commit() = 0;

		virtual bool			GetRomPreferences( const RomID & id, SRomPreferences * preferences ) const = 0;
		virtual void			SetRomPreferences( const RomID & id, const SRomPreferences & preferences ) = 0;
};

u32						ROM_GetTexureHashFrequencyAsFrames( ETextureHashFrequency thf );
ETextureHashFrequency	ROM_GetTextureHashFrequencyFromFrames( u32 frames );
const char *			ROM_GetTextureHashFrequencyDescription( ETextureHashFrequency thf );

u32						ROM_GetCheatFrequencyAsFrames( ECheatFrequency cf );
ECheatFrequency			ROM_GetCheatFrequencyFromFrames( u32 frames );
const char *			ROM_GetCheatFrequencyDescription( ECheatFrequency cf );

u32						ROM_GetFrameskipValueAsInt( EFrameskipValue value );
EFrameskipValue			ROM_GetFrameskipValueFromInt( u32 value );
const char *			ROM_GetFrameskipDescription( EFrameskipValue value );

#endif // PREFERENCES_H_
