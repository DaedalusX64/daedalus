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


#ifndef COLOURPULSER_H_
#define COLOURPULSER_H_

#include "Graphics/ColourValue.h"

class CColourPulser
{
	public:
		CColourPulser( c32 dim_colour, c32 bright_colour, u32 cycle_period );
		~CColourPulser();

		void				Update( u32 elapsed_ms );
		c32					GetCurrentColour() const									{ return mCurrentColour; }

	private:
		u32					mTimeCounter;
		u32					mCyclePeriod;
		c32					mDimColour;
		c32					mBrightColour;

		c32					mCurrentColour;
};

#endif	// COLOURPULSER_H_
