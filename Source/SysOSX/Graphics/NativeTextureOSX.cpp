/*
Copyright (C) 2013 StrmnNrmn

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
#include "Graphics/NativeTexture.h"
#include "Graphics/ColourValue.h"
#include "SysPSP/Graphics/PixelFormatPSP.h" // FIXME

#include "Math/MathUtil.h"

#include <png.h>

using namespace PixelFormats::Psp;


static u32 GetTextureBlockWidth( u32 dimension, ETextureFormat texture_format )
{
	DAEDALUS_ASSERT( GetNextPowerOf2( dimension ) == dimension, "This is not a power of 2" );

	// Ensure that the pitch is at least 16 bytes
	while( CalcBytesRequired( dimension, texture_format ) < 16 )
	{
		dimension *= 2;
	}

	return dimension;
}

static inline u32 CorrectDimension( u32 dimension )
{
	static const u32 MIN_TEXTURE_DIMENSION = 1;
	return Max( GetNextPowerOf2( dimension ), MIN_TEXTURE_DIMENSION );
}

CRefPtr<CNativeTexture>	CNativeTexture::Create( u32 width, u32 height, ETextureFormat texture_format )
{
	return new CNativeTexture( width, height, texture_format );
}

CNativeTexture::CNativeTexture( u32 w, u32 h, ETextureFormat texture_format )
:	mTextureFormat( texture_format )
,	mWidth( w )
,	mHeight( h )
,	mCorrectedWidth( CorrectDimension( w ) )
,	mCorrectedHeight( CorrectDimension( h ) )
,	mTextureBlockWidth( GetTextureBlockWidth( mCorrectedWidth, texture_format ) )
,	mTextureId( 0 )
{
	mScale.x = 1.0f / mCorrectedWidth;
	mScale.y = 1.0f / mCorrectedHeight;

	glGenTextures( 1, &mTextureId );
}

CNativeTexture::~CNativeTexture()
{
	glDeleteTextures( 1, &mTextureId );
}

bool CNativeTexture::HasData() const
{
	return glIsTexture( mTextureId );
}

void CNativeTexture::InstallTexture() const
{
	glBindTexture( GL_TEXTURE_2D, mTextureId );
}


namespace
{
	template< typename T >
	void ReadPngData( u32 width, u32 height, u32 stride, u8 ** p_row_table, int color_type, T * p_dest )
	{
		u8 r=0, g=0, b=0, a=0;

		for ( u32 y = 0; y < height; ++y )
		{
			const u8 * pRow = p_row_table[ y ];

			T * p_dest_row( p_dest );

			for ( u32 x = 0; x < width; ++x )
			{
				switch ( color_type )
				{
				case PNG_COLOR_TYPE_GRAY:
					r = g = b = *pRow++;
					if ( r == 0 && g == 0 && b == 0 )	a = 0x00;
					else								a = 0xff;
					break;
				case PNG_COLOR_TYPE_GRAY_ALPHA:
					r = g = b = *pRow++;
					if ( r == 0 && g == 0 && b == 0 )	a = 0x00;
					else								a = 0xff;
					pRow++;
					break;
				case PNG_COLOR_TYPE_RGB:
					b = *pRow++;
					g = *pRow++;
					r = *pRow++;
					if ( r == 0 && g == 0 && b == 0 )	a = 0x00;
					else								a = 0xff;
					break;
				case PNG_COLOR_TYPE_RGB_ALPHA:
					b = *pRow++;
					g = *pRow++;
					r = *pRow++;
					a = *pRow++;
					break;
				}

				p_dest_row[ x ] = T( r, g, b, a );
			}

			p_dest = reinterpret_cast< T * >( reinterpret_cast< u8 * >( p_dest ) + stride );
		}
	}

	//*****************************************************************************
	//	Thanks 71M/Shazz
	//	p_texture is either an existing texture (in case it must be of the
	//	correct dimensions and format) else a new texture is created and returned.
	//*****************************************************************************
	CRefPtr<CNativeTexture>	LoadPng( const char * p_filename, ETextureFormat texture_format )
	{
		const size_t	SIGNATURE_SIZE = 8;
		u8	signature[ SIGNATURE_SIZE ];

		FILE * fh = fopen( p_filename,"rb" );
		if (fh == NULL)
		{
			return NULL;
		}

		if (fread( signature, sizeof(u8), SIGNATURE_SIZE, fh ) != SIGNATURE_SIZE)
		{
			fclose(fh);
			return NULL;
		}

		if (!png_check_sig( signature, SIGNATURE_SIZE ))
		{
			return NULL;
		}

		png_struct * p_png_struct = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
		if (p_png_struct == NULL)
		{
			return NULL;
		}

		png_info * p_png_info = png_create_info_struct( p_png_struct );
		if (p_png_info == NULL)
		{
			png_destroy_read_struct( &p_png_struct, NULL, NULL );
			return NULL;
		}

		if (setjmp( png_jmpbuf(p_png_struct) ) != 0)
		{
			png_destroy_read_struct( &p_png_struct, NULL, NULL );
			return NULL;
		}

		png_init_io( p_png_struct, fh );
		png_set_sig_bytes( p_png_struct, SIGNATURE_SIZE );
		png_read_png( p_png_struct, p_png_info, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR, NULL );

		png_uint_32 width  = png_get_image_width( p_png_struct, p_png_info );
		png_uint_32 height = png_get_image_height( p_png_struct, p_png_info );

		CRefPtr<CNativeTexture>	texture = CNativeTexture::Create( width, height, texture_format );

		DAEDALUS_ASSERT( texture->GetWidth() >= width, "Width is unexpectedly small" );
		DAEDALUS_ASSERT( texture->GetHeight() >= height, "Height is unexpectedly small" );
		DAEDALUS_ASSERT( texture_format == texture->GetFormat(), "Texture format doesn't match" );

		u8 * buffer = new u8[ texture->GetBytesRequired() ];
		if( !buffer )
		{
			texture = NULL;
		}
		else
		{
			u32 stride = texture->GetStride();

			u8 ** row_pointers = png_get_rows( p_png_struct, p_png_info );
			int color_type = png_get_color_type( p_png_struct, p_png_info );

			switch( texture_format )
			{
			case TexFmt_5650:
				ReadPngData< Pf5650 >( width, height, stride, row_pointers, color_type, reinterpret_cast< Pf5650 * >( buffer ) );
				break;
			case TexFmt_5551:
				ReadPngData< Pf5551 >( width, height, stride, row_pointers, color_type, reinterpret_cast< Pf5551 * >( buffer ) );
				break;
			case TexFmt_4444:
				ReadPngData< Pf4444 >( width, height, stride, row_pointers, color_type, reinterpret_cast< Pf4444 * >( buffer ) );
				break;
			case TexFmt_8888:
				ReadPngData< Pf8888 >( width, height, stride, row_pointers, color_type, reinterpret_cast< Pf8888 * >( buffer ) );
				break;

			case TexFmt_CI4_8888:
			case TexFmt_CI8_8888:
				DAEDALUS_ERROR( "Can't use palettised format for png." );
				break;

			default:
				DAEDALUS_ERROR( "Unhandled texture format" );
				break;
			}

			texture->SetData( buffer, NULL );
		}

		//
		// Cleanup
		//
		delete [] buffer;
		png_destroy_read_struct( &p_png_struct, &p_png_info, NULL );
		fclose(fh);

		return texture;
	}
}

CRefPtr<CNativeTexture>	CNativeTexture::CreateFromPng( const char * p_filename, ETextureFormat texture_format )
{
	return LoadPng( p_filename, texture_format );
}

void CNativeTexture::SetData( void * data, void * palette )
{
	if( HasData() )
	{
		switch( mTextureFormat )
		{
		case TexFmt_CI4_8888:
			DAEDALUS_ASSERT( false, "CI4 not handled" );
			DAEDALUS_ASSERT( palette, "No palette provided" );
			break;

		case TexFmt_CI8_8888:
			DAEDALUS_ASSERT( false, "CI8 not handled" );
			DAEDALUS_ASSERT( palette, "No palette provided" );
			break;

		default:
			DAEDALUS_ASSERT( !IsTextureFormatPalettised( mTextureFormat ), "Unhandled palette texture" );
			DAEDALUS_ASSERT( palette == NULL, "Palette provided when not needed" );
			break;
		}

		glBindTexture( GL_TEXTURE_2D, mTextureId );
		glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA,
					mCorrectedWidth, mCorrectedHeight,
					0, GL_RGBA, GL_UNSIGNED_BYTE, data );
	}
}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
u32	CNativeTexture::GetVideoMemoryUsage() const
{
	return GetBytesRequired();
}

u32	CNativeTexture::GetSystemMemoryUsage() const
{
	return 0;
}
#endif

u32	CNativeTexture::GetStride() const
{
	return CalcBytesRequired( mTextureBlockWidth, mTextureFormat );
}

u32		CNativeTexture::GetBytesRequired() const
{
	return GetStride() * mCorrectedHeight;
}
