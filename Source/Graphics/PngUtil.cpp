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
#include "PngUtil.h"

#include <stdlib.h>
#include <png.h>

#include "Graphics/TextureFormat.h"
#include "Graphics/NativePixelFormat.h"
#include "Graphics/NativeTexture.h"
#include "Utility/DataSink.h"

template< typename T >
static void WritePngRow( u8 * line, const void * src, u32 width )
{
	u32 i {};

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

static void WritePngRowPal4( u8 * line, const void * src, u32 width, const NativePf8888 * palette )
{
	u32 i = 0;

	const NativePfCI44 * p_src = reinterpret_cast< const NativePfCI44 * >( src );

	for ( u32 x = 0; x < width; x++ )
	{
		NativePfCI44	colors( p_src[ x / 2 ] );
		u8				color_idx( (x&1) ? colors.GetIdxB() : colors.GetIdxA() );	// FIXME(strmnnrmn): A/B should be swapped? NativeTextureOSX is broken with this ordering.
		NativePf8888	color( palette[ color_idx ] );

		line[i++] = color.GetR();
		line[i++] = color.GetG();
		line[i++] = color.GetB();
		line[i++] = color.GetA();
	}
}

static void WritePngRowPal8( u8 * line, const void * src, u32 width, const NativePf8888 * palette )
{
	u32 i = 0;

	const NativePfCI8 * p_src = reinterpret_cast< const NativePfCI8 * >( src );

	for ( u32 x = 0; x < width; x++ )
	{
		u8				color_idx( p_src[ x ].Bits );
		NativePf8888	color( palette[ color_idx ] );

		line[i++] = color.GetR();
		line[i++] = color.GetG();
		line[i++] = color.GetB();
		line[i++] = color.GetA();
	}
}

static void DAEDALUS_ZLIB_CALL_TYPE PngWrite(png_structp png_ptr, png_bytep data, png_size_t len)
{
	DataSink * sink = static_cast<DataSink*>(png_get_io_ptr(png_ptr));
	sink->Write(data, len);
}

static void DAEDALUS_ZLIB_CALL_TYPE PngFlush(png_structp png_ptr)
{
	DataSink * sink = static_cast<DataSink*>(png_get_io_ptr(png_ptr));
	sink->Flush();
}

//*****************************************************************************
// Save texture as PNG
// From Shazz/71M - thanks guys!
//*****************************************************************************
void PngSaveImage( DataSink * sink, const void * data, const void * palette, ETextureFormat pixelformat, s32 pitch, u32 width, u32 height, bool use_alpha )
{
	DAEDALUS_ASSERT( !IsTextureFormatPalettised( pixelformat ) || palette, "No palette specified" );

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return;

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		return;
	}

	png_set_write_fn(png_ptr, sink, PngWrite, PngFlush);
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);

	u8* line = (u8*) malloc(width * 4);

	const u8 *				p       = reinterpret_cast< const u8 * >( data );
	const NativePf8888 *	pal8888 = reinterpret_cast< const NativePf8888 * >( palette );

	// If the pitch is negative (i.e for a screenshot), start at the last row and work backwards.
	if (pitch < 0)
	{
		p += -pitch * (height-1);
	}

	for ( u32 y = 0; y < height; y++ )
	{
		switch (pixelformat)
		{
		case TexFmt_5650:		WritePngRow< NativePf5650 >( line, p, width );	break;
		case TexFmt_5551:		WritePngRow< NativePf5551 >( line, p, width );	break;
		case TexFmt_4444:		WritePngRow< NativePf4444 >( line, p, width );	break;
		case TexFmt_8888:		WritePngRow< NativePf8888 >( line, p, width );	break;
		case TexFmt_CI4_8888:	WritePngRowPal4( line, p, width, pal8888 );		break;
		case TexFmt_CI8_8888:	WritePngRowPal8( line, p, width, pal8888 );		break;
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

		p += pitch;
		png_write_row(png_ptr, line);
	}

	free(line);
	line = NULL;
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
}

void PngSaveImage( const char* filename, const void * data, const void * palette,
				   ETextureFormat format, s32 stride,
				   u32 width, u32 height, bool use_alpha )
{
	FileSink sink;
	if (!sink.Open(filename, "wb"))
	{
		DAEDALUS_ERROR( "Couldn't open file for output" );
		return;
	}

	PngSaveImage(&sink, data, palette, format, stride, width, height, use_alpha);
}

void PngSaveImage( DataSink * sink, const CNativeTexture * texture )
{
	DAEDALUS_ASSERT(texture->HasData(), "Should have a texture");

	PngSaveImage( sink, texture->GetData(), texture->GetPalette(),
		texture->GetFormat(), texture->GetStride(),
		texture->GetWidth(), texture->GetHeight(), true );
}

// Utility function to flatten a native texture into an array of NativePf8888 values.
// Should live elsewhere, but need to share WritePngRow.
void FlattenTexture(const CNativeTexture * texture, void * dst, size_t len)
{
	const u8 *           p       = reinterpret_cast< const u8 * >( texture->GetData() );
	const NativePf8888 * pal8888 = reinterpret_cast< const NativePf8888 * >( texture->GetPalette() );

	u32 width  = texture->GetWidth();
	u32 height = texture->GetHeight();
	u32 pitch  = texture->GetStride();

	DAEDALUS_ASSERT(len == width * sizeof(NativePf8888) * height, "Unexpected number of bytes.");

	u8 * line = static_cast<u8 *>( dst );
	for ( u32 y = 0; y < height; y++ )
	{
		switch (texture->GetFormat())
		{
		case TexFmt_5650:		WritePngRow< NativePf5650 >( line, p, width );	break;
		case TexFmt_5551:		WritePngRow< NativePf5551 >( line, p, width );	break;
		case TexFmt_4444:		WritePngRow< NativePf4444 >( line, p, width );	break;
		case TexFmt_8888:		WritePngRow< NativePf8888 >( line, p, width );	break;
		case TexFmt_CI4_8888:	WritePngRowPal4( line, p, width, pal8888 );		break;
		case TexFmt_CI8_8888:	WritePngRowPal8( line, p, width, pal8888 );		break;
		}

		p    += pitch;
		line += width * sizeof(NativePf8888);
	}
}
