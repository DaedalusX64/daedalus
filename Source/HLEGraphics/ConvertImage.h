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

#ifndef __CONVERTIMAGE_H__
#define __CONVERTIMAGE_H__


#include "Graphics/TextureFormat.h"

struct TextureInfo;

namespace PixelFormats
{
	namespace Psp
	{
		struct Pf8888;
	}
}


struct TextureDestInfo
{
	TextureDestInfo( ETextureFormat tex_fmt )
		:	Format( tex_fmt )
		,	Width( 0 )
		,	Height( 0 )
		,	Pitch( 0 )
		,	pSurface( NULL )
		,	Palette( NULL )
	{
	}

	ETextureFormat				Format;
	u32							Width;			// Describes the width of the locked area. Use lPitch to move between successive lines
	u32							Height;			// Describes the height of the locked area
	s32							Pitch;			// Specifies the number of bytes on each row (not necessarily bitdepth*width/8)
	void *						pSurface;		// Pointer to the top left pixel of the image

	PixelFormats::Psp::Pf8888 *	Palette;
};


typedef void ( *ConvertFunction )( const TextureDestInfo & dst, const TextureInfo & ti);
extern const ConvertFunction gConvertFunctions[ 32 ];

#endif
