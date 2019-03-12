/*
Copyright (C) 2005-2007 StrmnNrmn

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
#include "Graphics/NativePixelFormat.h"
#include "Graphics/ColourValue.h"
#include "VideoMemoryManager.h"
#include "Utility/FastMemcpy.h"

#include "Math/MathUtil.h"

#include <png.h>

#include <pspgu.h>
#include <pspdebug.h>
#include <pspkernel.h>

//*****************************************************************************
//
//*****************************************************************************
namespace
{
static const u32 kPalette4BytesRequired = 16 * sizeof( NativePf8888 );
static const u32 kPalette8BytesRequired = 256 * sizeof( NativePf8888 );

//*****************************************************************************
//
//*****************************************************************************
enum	EPspTextureFormat
{
	PspTexFmt_5650	= GU_PSM_5650,
	PspTexFmt_5551	= GU_PSM_5551,
	PspTexFmt_4444	= GU_PSM_4444,
	PspTexFmt_8888	= GU_PSM_8888,
	PspTexFmt_T4	= GU_PSM_T4,
	PspTexFmt_T8	= GU_PSM_T8,
	PspTexFmt_T16	= GU_PSM_T16,
	PspTexFmt_T32	= GU_PSM_T32,
	PspTexFmt_DXT1	= GU_PSM_DXT1,
	PspTexFmt_DXT3	= GU_PSM_DXT3,
	PspTexFmt_DXT5	= GU_PSM_DXT5,
};

//*****************************************************************************
//
//*****************************************************************************
EPspTextureFormat	GetPspTextureFormat( ETextureFormat texture_format )
{
	switch( texture_format )
	{
	case TexFmt_5650:		return PspTexFmt_5650;
	case TexFmt_5551:		return PspTexFmt_5551;
	case TexFmt_4444:		return PspTexFmt_4444;
	case TexFmt_8888:		return PspTexFmt_8888;

	case TexFmt_CI4_8888:	return PspTexFmt_T4;
	case TexFmt_CI8_8888:	return PspTexFmt_T8;
	}

	DAEDALUS_ERROR( "Unhandled texture format" );
	return PspTexFmt_8888;
}

//*****************************************************************************
//	This is slower, but necessary because it deals with heights of < 8
//*****************************************************************************
/*void swizzle_slow(u8* out, const u8* in, u32 width, u32 height)
{
	u32 rowblocks = (width / 16);

	for (u32 j = 0; j < height; ++j)
	{
		for (u32 i = 0; i < width; ++i)
		{
			u32 blockx = i / 16;
			u32 blocky = j / 8;

			u32 x = (i - blockx * 16);
			u32 y = (j - blocky * 8);
			u32 block_index = blockx + ((blocky) * rowblocks);
			u32 block_address = block_index * 16 * 8;

			out[block_address + x + y * 16] = in[i + j * width];
		}
	}
}
*/
//*****************************************************************************
//
//*****************************************************************************
inline void swizzle_fast(u8* out, const u8* in, u32 width, u32 height)
{
	DAEDALUS_ASSERT( (width & 15 ) == 0, "Width is not a multiple of 16 - is %d", width );
	DAEDALUS_ASSERT( (height & 7 ) == 0, "Height is not a multiple of 8 - is %d", height );

#if 1	//1->Raphaels version, 0->Original
	u32 rowblocks = (width / 16);
	u32 rowblocks_add = (rowblocks - 1) * 128;
	u32 block_address = 0;
	u32 *src = (u32*)in;

	for (u32 j = 0; j < height; j++,block_address+=16)
	{
		u32 *block = (u32*)&out[block_address];

		for (u32 i = 0; i < rowblocks; i++)
		{
			*block++ = *src++;
			*block++ = *src++;
			*block++ = *src++;
			*block++ = *src++;
			block += 28;
		}

		if ((j & 0x7) == 0x7) block_address += rowblocks_add;
	}
#else
	u32 width_blocks = (width / 16);
	u32 height_blocks = (height / 8);

	u32 src_pitch = (width - 16) / 4;
	u32 src_row = width * 8;

	const u8* ysrc = in;
	u32* dst = (u32*)out;

	for (u32 blocky = 0; blocky < height_blocks; ++blocky)
	{
		const u8* xsrc = ysrc;
		for (u32 blockx = 0; blockx < width_blocks; ++blockx)
		{
			const u32* src = (u32*)xsrc;
			for (u32 j = 0; j < 8; ++j)
			{
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				src += src_pitch;
			}
			xsrc += 16;
		}
		ysrc += src_row;
	}
#endif
}

//*****************************************************************************
//
//*****************************************************************************
inline bool swizzle(u8* out, const u8* in, u32 width, u32 height)
{
	if(width < 16 || height < 8)
	{
		//swizzle_slow( out, in, width, height );

		memcpy_vfpu( out, in, width * height );
		return false;
	}
	else
	{
		swizzle_fast( out, in, width, height );
		return true;
	}
}

//*****************************************************************************
//	Swizzle the texture, explicitly setting the texture colour to the specified value (alpha is left alone!)
//*****************************************************************************
/*void swizzle_fast_setcolour(u8* out, const u8* in, u32 width, u32 height, u32 mask, u32 colour)
{
	u32 blockx, blocky;
	u32 j;

	u32 width_blocks = (width / 16);
	u32 height_blocks = (height / 8);

	u32 src_pitch = (width-16)/4;
	u32 src_row = width * 8;

	const u8* ysrc = in;
	u32* dst = (u32*)out;

	const u32	MASK = mask;
	const u32	INV_MASK  = ~mask;

	// For speed, clear out the bits we're not setting
	u32 just_colour( colour & MASK );

	for (blocky = 0; blocky < height_blocks; ++blocky)
	{
		const u8* xsrc = ysrc;
		for (blockx = 0; blockx < width_blocks; ++blockx)
		{
			const u32* src = (u32*)xsrc;
			for (j = 0; j < 8; ++j)
			{
				*(dst++) = (*(src++) & INV_MASK) | just_colour;
				*(dst++) = (*(src++) & INV_MASK) | just_colour;
				*(dst++) = (*(src++) & INV_MASK) | just_colour;
				*(dst++) = (*(src++) & INV_MASK) | just_colour;
				src += src_pitch;
			}
			xsrc += 16;
		}
		ysrc += src_row;
	}
}
*/
//*****************************************************************************
//
//*****************************************************************************
u32	GetTextureBlockWidth( u32 dimension, ETextureFormat texture_format )
{
	DAEDALUS_ASSERT( GetNextPowerOf2( dimension ) == dimension, "This is not a power of 2" );

	// Ensure that the pitch is at least 16 bytes
	while( CalcBytesRequired( dimension, texture_format ) < 16 )
	{
		dimension *= 2;
	}

	return dimension;
}

//*****************************************************************************
//
//*****************************************************************************
u32	CorrectDimension( u32 dimension )
{
	static const u32 MIN_TEXTURE_DIMENSION = 1;
	return Max( GetNextPowerOf2( dimension ), MIN_TEXTURE_DIMENSION );
}

}

//*****************************************************************************
//
//*****************************************************************************
CRefPtr<CNativeTexture>	CNativeTexture::Create( u32 width, u32 height, ETextureFormat texture_format )
{
	return new CNativeTexture( width, height, texture_format );
}

//*****************************************************************************
//
//*****************************************************************************
CNativeTexture::CNativeTexture( u32 w, u32 h, ETextureFormat texture_format )
:	mTextureFormat( texture_format )
,	mWidth( w )
,	mHeight( h )
,	mCorrectedWidth( CorrectDimension( w ) )
,	mCorrectedHeight( CorrectDimension( h ) )
,	mTextureBlockWidth( GetTextureBlockWidth( mCorrectedWidth, texture_format ) )
,	mpPalette( NULL )
,	mIsPaletteVidMem( false )
,	mIsSwizzled( true )
#ifdef DAEDALUS_ENABLE_ASSERTS
,	mPaletteSet( false )
#endif
{
	mScale.x = 1.0f / mCorrectedWidth;
	mScale.y = 1.0f / mCorrectedHeight;

	u32		bytes_required( GetBytesRequired() );

	if( !CVideoMemoryManager::Get()->Alloc( bytes_required, &mpData, &mIsDataVidMem ) )
	{
		DAEDALUS_ERROR( "Out of memory for texels ( %d bytes)", bytes_required );
	}
	switch( texture_format )
	{
	case TexFmt_CI4_8888:
		if( !CVideoMemoryManager::Get()->Alloc( kPalette4BytesRequired, &mpPalette, &mIsPaletteVidMem ) )
		{
			DAEDALUS_ERROR( "Out of memory for 4-bit palette, %d bytes", kPalette4BytesRequired );
		}
		break;

	case TexFmt_CI8_8888:
		if( !CVideoMemoryManager::Get()->Alloc( kPalette8BytesRequired, &mpPalette, &mIsPaletteVidMem ) )
		{
			DAEDALUS_ERROR( "Out of memory for 8-bit palette, %d bytes", kPalette8BytesRequired );
		}
		break;

	default:
		DAEDALUS_ASSERT( !IsTextureFormatPalettised( texture_format ), "Unhandled palette texture" );
		break;
	}
}

//*****************************************************************************
//
//*****************************************************************************
CNativeTexture::~CNativeTexture()
{
	CVideoMemoryManager::Get()->Free( mpData );
	CVideoMemoryManager::Get()->Free( mpPalette );
}

//*****************************************************************************
//
//*****************************************************************************
bool	CNativeTexture::HasData() const
{
	return mpData != NULL && (!IsTextureFormatPalettised( mTextureFormat ) || mpPalette != NULL );
}

//*****************************************************************************
//
//*****************************************************************************
void	CNativeTexture::InstallTexture() const
{
	sceGuEnable(GU_TEXTURE_2D);

	// This needed otherwise we'll crash if running out of memory
	if( !HasData() )
	{
		// Todo : Fix placeholder
		// Use the placeholder texture
		sceGuTexMode( GU_PSM_4444, 0, 0, 1 );		// maxmips/a2/swizzle = 0
		//sceGuTexImage( 0, gPlaceholderTextureWidth, gPlaceholderTextureHeight, gPlaceholderTextureWidth, gWhiteTexture );
	}
	else
	{
		EPspTextureFormat	psp_texture_format( GetPspTextureFormat( mTextureFormat ) );
		sceGuTexMode( psp_texture_format, 0, 0, mIsSwizzled ? 1 : 0 );		// maxmips/a2/swizzle = 0
		sceGuTexImage( 0, mCorrectedWidth, mCorrectedHeight, mTextureBlockWidth, mpData );

#ifdef DAEDALUS_ENABLE_ASSERTS
		DAEDALUS_ASSERT( !IsTextureFormatPalettised( mTextureFormat ) || mPaletteSet, "The palette has not been set" );

		DAEDALUS_ASSERT( ((u32)mpData & 0xf) == 0, "Palette not 16-byte aligned" )
		DAEDALUS_ASSERT( ((u32)mpPalette & 0xf) == 0, "Palette not 16-byte aligned" )
#endif
		switch( mTextureFormat )
		{
		case TexFmt_CI4_8888:
			sceGuClutMode( GU_PSM_8888, 0, 0xf, 0 );		// shift, mask, startentry
			sceGuClutLoad( 16/8, mpPalette );
			break;

		case TexFmt_CI8_8888:
			sceGuClutMode( GU_PSM_8888, 0, 0xff, 0 );		// shift, mask, startentry
			sceGuClutLoad( 256/8, mpPalette );
			break;

		default:
			DAEDALUS_ASSERT( !IsTextureFormatPalettised( mTextureFormat ), "Unhandled palette texture" );
			break;
		}
	}
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

		FILE * fh( fopen( p_filename,"rb" ) );
		if(fh == NULL)
		{
			return NULL;
		}

		if (fread( signature, sizeof(u8), SIGNATURE_SIZE, fh ) != SIGNATURE_SIZE)
		{
			fclose(fh);
			return NULL;
		}

		if ( !png_check_sig( signature, SIGNATURE_SIZE ) )
		{
			return NULL;
		}

		png_struct * p_png_struct( png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL ) );
		if ( p_png_struct == NULL)
		{
			return NULL;
		}

		png_info * p_png_info( png_create_info_struct( p_png_struct ) );
		if ( p_png_info == NULL )
		{
			png_destroy_read_struct( &p_png_struct, NULL, NULL );
			return NULL;
		}

		if ( setjmp( png_jmpbuf(p_png_struct) ) != 0 )
		{
			png_destroy_read_struct( &p_png_struct, NULL, NULL );
			return NULL;
		}

		png_init_io( p_png_struct, fh );
		png_set_sig_bytes( p_png_struct, SIGNATURE_SIZE );
		png_read_png( p_png_struct, p_png_info, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR, NULL );

		png_uint_32 width = png_get_image_width(p_png_struct, p_png_info);//p_png_info->width;
		png_uint_32 height = png_get_image_height(p_png_struct, p_png_info);//p_png_info->height;


		CRefPtr<CNativeTexture>	texture = CNativeTexture::Create( width, height, texture_format );

		DAEDALUS_ASSERT( texture->GetWidth() >= width, "Width is unexpectedly small" );
		DAEDALUS_ASSERT( texture->GetHeight() >= height, "Height is unexpectedly small" );
		DAEDALUS_ASSERT( texture_format == texture->GetFormat(), "Texture format doesn't match" );

		u8 *	p_dest( new u8[ texture->GetBytesRequired() ] );
		if( !p_dest )
		{
			texture = NULL;
		}
		else
		{
			u32		stride( texture->GetStride() );

			switch( texture_format )
			{
				case TexFmt_5650:
					ReadPngData< NativePf5650 >( width, height, stride, png_get_rows(p_png_struct, p_png_info), png_get_color_type(p_png_struct, p_png_info), reinterpret_cast< NativePf5650 * >( p_dest ) );
					break;
				case TexFmt_5551:
					ReadPngData< NativePf5551 >( width, height, stride, png_get_rows(p_png_struct, p_png_info), png_get_color_type(p_png_struct, p_png_info), reinterpret_cast< NativePf5551 * >( p_dest ) );
					break;
				case TexFmt_4444:
					ReadPngData< NativePf4444 >( width, height, stride, png_get_rows(p_png_struct, p_png_info), png_get_color_type(p_png_struct, p_png_info), reinterpret_cast< NativePf4444 * >( p_dest ) );
					break;
				case TexFmt_8888:
					ReadPngData< NativePf8888 >( width, height, stride, png_get_rows(p_png_struct, p_png_info), png_get_color_type(p_png_struct, p_png_info), reinterpret_cast< NativePf8888 * >( p_dest ) );
					break;

			case TexFmt_CI4_8888:
			case TexFmt_CI8_8888:
				DAEDALUS_ERROR( "Can't use palettised format for png." );
				break;

			default:
				DAEDALUS_ERROR( "Unhandled texture format" );
				break;
			}

			texture->SetData( p_dest, NULL );
		}

		//
		// Cleanup
		//
		delete [] p_dest;
		png_destroy_read_struct( &p_png_struct, &p_png_info, NULL );
		fclose(fh);

		return texture;
	}
}

//*****************************************************************************
//
//*****************************************************************************
CRefPtr<CNativeTexture>	CNativeTexture::CreateFromPng( const char * p_filename, ETextureFormat texture_format )
{
	return LoadPng( p_filename, texture_format );
}

//*****************************************************************************
//
//*****************************************************************************
void	CNativeTexture::SetData( void * data, void * palette )
{
	u32		bytes_per_row( GetStride() );

	if( HasData() )
	{
		mIsSwizzled = swizzle( reinterpret_cast< u8 * >( mpData ), reinterpret_cast< u8 * >( data ), bytes_per_row, mCorrectedHeight );

		// We need to flush out the data from the cache if this isn't in vid mem
		if( !mIsDataVidMem )
		{
			sceKernelDcacheWritebackAll();
			sceGuTexFlush();
		}

		if( mpPalette != NULL )
		{
			DAEDALUS_ASSERT( palette != NULL, "No palette provided" );

			#ifdef DAEDALUS_ENABLE_ASSERTS
				mPaletteSet = true;
			#endif

			switch( mTextureFormat )
			{
			case TexFmt_CI4_8888:
				memcpy_vfpu( mpPalette, palette, kPalette4BytesRequired );
				break;
			case TexFmt_CI8_8888:
				memcpy_vfpu( mpPalette, palette, kPalette8BytesRequired );
				break;

			default:
				DAEDALUS_ERROR( "Unhandled palette format" );
				break;
			}

			// We need to flush out the data from the cache if this isn't in vid mem
			if( !mIsPaletteVidMem )
			{
				sceKernelDcacheWritebackAll();
				sceGuTexFlush();
			}
		}
		else
		{
			DAEDALUS_ASSERT( palette == NULL, "Palette provided when not needed" );
		}
	}
}

//*****************************************************************************
//
//*****************************************************************************
u32	CNativeTexture::GetStride() const
{
	return CalcBytesRequired( mTextureBlockWidth, mTextureFormat );

}
//*****************************************************************************
//
//*****************************************************************************

u32		CNativeTexture::GetBytesRequired() const
{
	return GetStride() * mCorrectedHeight;
}
