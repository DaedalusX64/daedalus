/*
Copyright (C) 2001 StrmnNrmn

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


#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "DaedRefCounted.h"

class CNativeTexture;

typedef struct {
	u32			Width;			// Describes the width of the locked area. Use lPitch to move between successive lines
	u32			Height;			// Describes the height of the locked area
	s32			Pitch;			// Specifies the number of bytes on each row (not necessarily bitdepth*width/8)
	void *		pSurface;		// Pointer to the top left pixel of the image
} DrawInfo;


// Virtual base class
class CTexture : public daedalus::CRefCounted
{
	protected:
		virtual ~CTexture()	{}
	public:
		// Create textures with CTexture::Create()
		//CTexture();

		static CTexture * Create( u32 width, u32 height );

		virtual CNativeTexture *	GetTexture() = 0;
		virtual CNativeTexture *	GetRecolouredTexture( u32 mask, u32 colour ) = 0;				// Returns a texture with the RGB channels set to the specified colour (alpha remains the same)

		virtual	u32			GetWidth() const = 0;
		virtual	u32			GetHeight() const = 0;
		
		virtual	u32			GetRealWidth() const = 0;
		virtual	u32			GetRealHeight() const = 0;

		// Provides access to "surface"
		virtual bool StartUpdate( DrawInfo *di ) = 0;
		virtual void EndUpdate( DrawInfo * di ) = 0;

};


#endif	// __TEXTURE_H__
