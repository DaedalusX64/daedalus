/*
Copyright (C) 2006 StrmnNrmn

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
#include "ColourPulser.h"

#include "Graphics/ColourValue.h"

#include "Math/Math.h"	// VFPU Math

//*************************************************************************************
//
//*************************************************************************************
CColourPulser::CColourPulser( c32 dim_colour, c32 bright_colour, u32 cycle_period )
:	mTimeCounter( 0 )
,	mCyclePeriod( cycle_period )
,	mDimColour( dim_colour )
,	mBrightColour( bright_colour )
,	mCurrentColour( mDimColour )
{
}

//*************************************************************************************
//
//*************************************************************************************
CColourPulser::~CColourPulser()
{
}

//*************************************************************************************
//
//*************************************************************************************
void	CColourPulser::Update( u32 elapsed_ms )
{
	mTimeCounter = (mTimeCounter + elapsed_ms) % mCyclePeriod;

	f32	cycle_fraction( f32(mTimeCounter) / f32(mCyclePeriod) );

	f32	sin_val( vfpu_cosf( cycle_fraction * 2.0f * PI ) );				// In range -1..+1
	f32	factor( ( sin_val + 1.0f ) / 2.0f );							// In range 0..1

	mCurrentColour = mDimColour.Interpolate( mBrightColour, factor );
}
