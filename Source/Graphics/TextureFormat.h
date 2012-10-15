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


#ifndef TEXTUREFORMAT_H_
#define TEXTUREFORMAT_H_

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


inline u32		GetBitsPerPixel( ETextureFormat texture_format )
{
	switch( texture_format )
	{
	case TexFmt_5650:		return 16;
	case TexFmt_5551:		return 16;
	case TexFmt_4444:		return 16;
	case TexFmt_8888:		return 32;

	case TexFmt_CI4_8888:	return 4;
	case TexFmt_CI8_8888:	return 8;
	}

	DAEDALUS_ERROR( "Unhandled texture format" );
	return 8;
}

inline u32		CalcBytesRequired( u32 pixels, ETextureFormat texture_format )
{
	return ( pixels * GetBitsPerPixel( texture_format ) + 4 ) / 8;
}

#endif // TEXTUREFORMAT_H_
