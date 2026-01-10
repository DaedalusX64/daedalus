/*
Copyright (C) 2005 StrmnNrmn

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


#ifndef NATIVETEXTURE_H_
#define NATIVETEXTURE_H_
#include <kos.h>
#include "DaedRefCounted.h"

class CNativeTexture : public daedalus::CRefCounted
{
		~CNativeTexture();
	public:

		static	void					CorrectDimensions( u32 width, u32 height, u32 & adjusted_width, u32 & adjusted_height );

		static	CNativeTexture *		Create( u32 width, u32 height );

		CNativeTexture( u32 w, u32 h );
		void	InstallTexture();
		void	SetData( void * p_data, u32 w, u32 h );
		void	SetDataForceColour( void * p_data, u32 w, u32 h, u32 mask, u32 colour );			// Set the data, but replace RGB with the specified colour (Alpha remains)

		u32		GetWidth() const
		{
			return mWidth;
		}
		u32		GetHeight() const
		{
			return mHeight;
		}
		
		pvr_ptr_t GetData() const
		{
            return mpData;
        }

	private:
		u32			mWidth;
		u32			mHeight;
		pvr_ptr_t   mpData;
};

#endif	// NATIVETEXTURE_H_
