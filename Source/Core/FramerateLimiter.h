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

#ifndef UTILITY_FRAMERATELIMITER_H_
#define UTILITY_FRAMERATELIMITER_H_

#include "Base/Types.h"

extern u32		gSpeedSyncEnabled;

bool			FramerateLimiter_Reset();
void			FramerateLimiter_Limit();
f32				FramerateLimiter_GetSync();	// Returns fraction of real n64 we're running at (1 = 100%)
u32				FramerateLimiter_GetTvFrequencyHz();

// Override the sync function, e.g. if the audio plugin wants to control sync.
using FramerateSyncFn = void (*)(void * arg);

void			FramerateLimiter_SetAuxillarySyncFunction(FramerateSyncFn fn, void * arg);

#endif // UTILITY_FRAMERATELIMITER_H_
