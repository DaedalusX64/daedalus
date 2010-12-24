/*
Copyright (C) 2001 Lkb

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

#include "Graphics/PngUtil.h"
#include "Graphics/TextureFormat.h"
#include "PixelFormatPSP.h"

#include <png.h>

using namespace PixelFormats;

template< typename T >
void WritePngRow( u8 * line, const void * src, u32 width )
{
	u32 i = 0;

	const T *	p_src( reinterpret_cast< const T * >( src ) );

	for ( u32 x = 0; x < width; x++ )
	{
		T	color( p_src[ x ] );

		line[i++] = color.GetR();
		line[i++] = color.GetG();
		line[i++] = color.GetB();
		line[i++] = color.GetA();
	}
}

void WritePngRowPal4( u8 * line, const void * src, u32 width, const Psp::Pf8888 * palette )
{
	u32 i = 0;

	const Psp::PfCI44 *	p_src( reinterpret_cast< const Psp::PfCI44 * >( src ) );

	for ( u32 x = 0; x < width; x++ )
	{
		Psp::PfCI44		colors( p_src[ x / 2 ] );
		u8				color_idx( (x&1) ? colors.GetIdxB() : colors.GetIdxA() );
		Psp::Pf8888		color( palette[ color_idx ] );

		line[i++] = color.GetR();
		line[i++] = color.GetG();
		line[i++] = color.GetB();
		line[i++] = color.GetA();
	}
}

void WritePngRowPal8( u8 * line, const void * src, u32 width, const Psp::Pf8888 * palette )
{
	u32 i = 0;

	const Psp::PfCI8 *	p_src( reinterpret_cast< const Psp::PfCI8 * >( src ) );

	for ( u32 x = 0; x < width; x++ )
	{
		u8				color_idx( p_src[ x ].Bits );
		Psp::Pf8888		color( palette[ color_idx ] );

		line[i++] = color.GetR();
		line[i++] = color.GetG();
		line[i++] = color.GetB();
		line[i++] = color.GetA();
	}
}

//*****************************************************************************
// Save texture as PNG
// From Shazz/71M - thanks guys!
//*****************************************************************************
void PngSaveImage( const char* filename, const void * data, const void * palette, ETextureFormat pixelformat, u32 pitch, u32 width, u32 height, bool use_alpha )
{
	DAEDALUS_ASSERT( !IsTextureFormatPalettised( pixelformat ) || palette, "No palette specified" );

	FILE* fp = fopen(filename, "wb");
	if (!fp)
	{
		DAEDALUS_ERROR( "Couldn't open file for output" );
		return;
	}

	png_structp png_ptr( png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL) );
	if (!png_ptr)
	{
		fclose(fp);
		return;
	}
	png_infop info_ptr( png_create_info_struct(png_ptr) );
	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		fclose(fp);
		return;
	}

	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);

	u8* line = (u8*) malloc(width * 4);

	const u8 *		p( reinterpret_cast< const u8 * >( data ) );

	const Psp::Pf8888 *		pal8888( reinterpret_cast< const Psp::Pf8888 * >( palette ) );

	for ( u32 y = 0; y < height; y++ )
	{
		switch (pixelformat)
		{
		case TexFmt_5650:			WritePngRow< Psp::Pf5650 >( line, p, width );	break;
		case TexFmt_5551:			WritePngRow< Psp::Pf5551 >( line, p, width );	break;
		case TexFmt_4444:			WritePngRow< Psp::Pf4444 >( line, p, width );	break;
		case TexFmt_8888:			WritePngRow< Psp::Pf8888 >( line, p, width );	break;
		case TexFmt_CI4_8888:		WritePngRowPal4( line, p, width, pal8888 );		break;
		case TexFmt_CI8_8888:		WritePngRowPal8( line, p, width, pal8888 );		break;
		}

		//
		//	Set alpha to full if it's not required.
		//	Should really create an PNG_COLOR_TYPE_RGB image instead
		//
		if( !use_alpha )
		{
			for( u32 x = 0; x < width; ++x )
			{
				line[ x*4 + 3 ] = 0xff;
			}
		}

		p = p + pitch;
		png_write_row(png_ptr, line);
	}
	
	free(line);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	fclose(fp);
}
