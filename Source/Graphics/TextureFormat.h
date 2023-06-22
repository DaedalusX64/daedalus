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

#ifndef GRAPHICS_TEXTUREFORMAT_H_
#define GRAPHICS_TEXTUREFORMAT_H_

#include <stdlib.h>

#include "Base/Types.h"
#include <unordered_map>

enum	ETextureFormat
{
	TexFmt_5650,
	TexFmt_5551,
	TexFmt_4444,
	TexFmt_8888,

	// Palletised
	TexFmt_CI4_8888,
	TexFmt_CI8_8888,
};

inline bool IsTextureFormatPalettised( ETextureFormat texture_format )
{
	return texture_format >= TexFmt_CI4_8888;
}


inline uint32_t GetBitsPerPixel(ETextureFormat texture_format)
{
    static const std::unordered_map<ETextureFormat, uint32_t> format_to_bpp{
        {ETextureFormat::TexFmt_5650, 16},
        {ETextureFormat::TexFmt_5551, 16},
        {ETextureFormat::TexFmt_4444, 16},
        {ETextureFormat::TexFmt_8888, 32},
        {ETextureFormat::TexFmt_CI4_8888, 4},
        {ETextureFormat::TexFmt_CI8_8888, 8}
    };
    
    auto it = format_to_bpp.find(texture_format);
    
    if (it == format_to_bpp.end()) {
        // Log error here, "Unhandled texture format"
        return 8;  // Default bits per pixel
    }
    
    return it->second;
}
inline u32		CalcBytesRequired( u32 pixels, ETextureFormat texture_format )
{
	return ( pixels * GetBitsPerPixel( texture_format ) + 4 ) / 8;
}

#endif // GRAPHICS_TEXTUREFORMAT_H_
