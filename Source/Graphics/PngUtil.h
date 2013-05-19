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

#ifndef GRAPHICS_PNGUTIL_H_
#define GRAPHICS_PNGUTIL_H_

#include <stdlib.h>

#include "TextureFormat.h"

class DataSink;
class CNativeTexture;

void PngSaveImage( const char* filename, const void * data, const void * palette, ETextureFormat pixelformat, s32 pitch, u32 width, u32 height, bool use_alpha );
void PngSaveImage( DataSink * sink, const void * data, const void * palette, ETextureFormat pixelformat, s32 pitch, u32 width, u32 height, bool use_alpha );
void PngSaveImage( DataSink * sink, const CNativeTexture * texture );

void FlattenTexture(const CNativeTexture * texture, void * dst, size_t len);

#endif // GRAPHICS_PNGUTIL_H_
