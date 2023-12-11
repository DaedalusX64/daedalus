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

#ifndef UTILITY_GLOBAL_PREFERENCES_H_
#define UTILITY_GLOBAL_PREFERENCES_H_

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
	VT_FULLSCREEN_HD,
};
const u32 NUM_VIEWPORT_TYPES = VT_FULLSCREEN_HD+1;

enum ETVType
{
	TT_4_3 = 0,
	TT_WIDESCREEN,
};

struct SGlobalPreferences
{
	bool							DisplayFramerate;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	bool						HighlightInexactBlendModes;
	bool						CustomBlendModes;
#endif
	bool						BatteryWarning;
	bool						LargeROMBuffer;
	bool						ForceLinearFilter;
	bool						RumblePak;

	EGuiColor					GuiColor;

	float						StickMinDeadzone;
	float						StickMaxDeadzone;

	u32							Language;

	EViewportType				ViewportType;

	bool						TVEnable;
	bool						TVLaced;
	ETVType						TVType;

	SGlobalPreferences();

	void		Apply() const;
};

extern SGlobalPreferences	gGlobalPreferences;

#endif // INTERFACE_GLOBAL_PREFERENCES