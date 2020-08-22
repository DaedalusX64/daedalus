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

#ifndef UTILITY_PREFERENCES_H_
#define UTILITY_PREFERENCES_H_

#include "Base/Singleton.h"
#include "Interface/GlobalPreferences.h"
#include "Interface/RomPreferences.h"

class RomID;

class CPreferences : public CSingleton<CPreferences>
{
   public:
	virtual ~CPreferences();

	virtual bool OpenPreferencesFile(const char* filename) = 0;
	virtual void Commit() = 0;

	virtual bool GetRomPreferences(const RomID& id, SRomPreferences* preferences) const = 0;
	virtual void SetRomPreferences(const RomID& id, const SRomPreferences& preferences) = 0;
};

const char* Preferences_GetTextureHashFrequencyDescription(ETextureHashFrequency thf);
const char* Preferences_GetFrameskipDescription(EFrameskipValue value);

#endif  // UTILITY_PREFERENCES_H_
