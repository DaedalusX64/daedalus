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
#include "Timer.h"

#include "Utility/Timing.h"

//*************************************************************************************
//
//*************************************************************************************
CTimer::CTimer()
{
	u64		freq;
	NTiming::GetPreciseFrequency( &freq );
	mFreqInv = 1.0f / f32( freq );

	NTiming::GetPreciseTime( &mResetTime );
	mLastTime = mResetTime;
}

//*************************************************************************************
//
//*************************************************************************************
float	CTimer::GetElapsedSecondsSinceReset()
{
	u64			now;
	NTiming::GetPreciseTime( &now );

	u64			elapsed_ticks( now - mResetTime );
	return f32(elapsed_ticks) * mFreqInv;
}


//*************************************************************************************
//
//*************************************************************************************
float	CTimer::GetElapsedSeconds()
{
	u64			now;
	NTiming::GetPreciseTime( &now );

	u64			elapsed_ticks( now - mLastTime );
	mLastTime = now;

	return f32(elapsed_ticks) * mFreqInv;
}

//*************************************************************************************
//
//*************************************************************************************
void CTimer::Reset()
{
	u64			now;
	NTiming::GetPreciseTime( &now );

	mResetTime = now;
	mLastTime = now;
}
