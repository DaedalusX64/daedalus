#pragma once

#include "RomFile/RomSettings.h"

namespace UI
{
	void LoadRomPreferences(RomID mRomID);
	bool DrawOptionsPage(RomID mRomID);
	void DrawInGameMenu();
}