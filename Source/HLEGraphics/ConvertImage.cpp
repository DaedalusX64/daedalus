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

#include "stdafx.h"
#include "ConvertImage.h"
#include "TextureInfo.h"

#include "DLDebug.h"
#include "Core/Memory.h"

#include "RDP.h"
#include "N64PixelFormat.h"

#include "Graphics/NativePixelFormat.h"

#include "Math/MathUtil.h"

#include "OSHLE/ultra_gbi.h"

namespace
{

struct TextureDestInfo
{
	explicit TextureDestInfo( ETextureFormat tex_fmt )
		:	Format( tex_fmt )
		,	Width( 0 )
		,	Height( 0 )
		,	Pitch( 0 )
		,	Data( NULL )
		,	Palette( NULL )
	{
	}

	ETextureFormat		Format;
	u32					Width;			// Describes the width of the locked area. Use lPitch to move between successive lines
	u32					Height;			// Describes the height of the locked area
	s32					Pitch;			// Specifies the number of bytes on each row (not necessarily bitdepth*width/8)
	void *				Data;			// Pointer to the top left pixel of the image
	NativePf8888 *		Palette;
};

static const u8 OneToEight[2] =
{
	0x00,		// 0 -> 00 00 00 00
	0xff		// 1 -> 11 11 11 11
};

static const u8 ThreeToEight[8] =
{
	0x00,		// 000 -> 00 00 00 00
	0x24,		// 001 -> 00 10 01 00
	0x49,		// 010 -> 01 00 10 01
	0x6d,       // 011 -> 01 10 11 01
	0x92,       // 100 -> 10 01 00 10
	0xb6,		// 101 -> 10 11 01 10
	0xdb,		// 110 -> 11 01 10 11
	0xff		// 111 -> 11 11 11 11
};


static const u8 FourToEight[16] =
{
	0x00, 0x11, 0x22, 0x33,
	0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xaa, 0xbb,
	0xcc, 0xdd, 0xee, 0xff
};


template< u32 Size >
struct SByteswapInfo;

template<> struct SByteswapInfo< 1 >
{
	enum { Fiddle = 3 };
};
template<> struct SByteswapInfo< 2 >
{
	enum { Fiddle = 2 };
};
template<> struct SByteswapInfo< 4 >
{
	enum { Fiddle = 0 };
};

template< u32 Size >
struct SSwizzleInfo;

template<> struct SSwizzleInfo< 1 >
{
	enum { Swizzle = 4 };
};
template<> struct SSwizzleInfo< 2 >
{
	enum { Swizzle = 2 };
};
template<> struct SSwizzleInfo< 4 >
{
	enum { Swizzle = 2 };
};

template < typename OutT >
struct SConvertGeneric
{
typedef void (*ConvertRowFunction)( OutT * dst, const u8 * src, u32 src_offset, u32 width );


static void ConvertGeneric( const TextureDestInfo & dsti,
							const TextureInfo & ti,
							ConvertRowFunction swapped_fn,
							ConvertRowFunction unswapped_fn )
{
	OutT *		dst        = reinterpret_cast< OutT * >( dsti.Data );
	const u8 *	src        = g_pu8RamBase;
	u32			src_offset = ti.GetLoadAddress();
	u32			src_pitch  = ti.GetPitch();

	if ( ti.IsSwapped())
	{
		for (u32 y = 0; y < ti.GetHeight(); y++)
		{
			if ((y&1) == 0)
			{
				unswapped_fn( dst, src, src_offset, ti.GetWidth() );
			}
			else
			{
				swapped_fn( dst, src, src_offset, ti.GetWidth() );
			}

			src_offset += src_pitch;
			dst = reinterpret_cast< OutT * >( (u8*)dst + dsti.Pitch );
		}
	}
	else
	{
		for (u32 y = 0; y < ti.GetHeight(); y++)
		{
			unswapped_fn( dst, src, src_offset, ti.GetWidth() );

			src_offset += src_pitch;
			dst = reinterpret_cast< OutT * >( (u8*)dst + dsti.Pitch );
		}
	}
}

};

typedef void (*ConvertPalettisedRowFunction)( NativePf8888 * dst, const u8 * src, u32 src_offset, u32 width, const NativePf8888 * palette );

static void ConvertPalettisedTo8888( const TextureDestInfo & dsti, const TextureInfo & ti,
									 const NativePf8888 * palette,
									 ConvertPalettisedRowFunction swapped_fn,
									 ConvertPalettisedRowFunction unswapped_fn )
{
	NativePf8888 *	dst        = reinterpret_cast< NativePf8888 * >( dsti.Data );
	const u8 *		src        = g_pu8RamBase;
	u32				src_offset = ti.GetLoadAddress();
	u32				src_pitch  = ti.GetPitch();

	if (ti.IsSwapped())
	{
		for (u32 y = 0; y < ti.GetHeight(); y++)
		{
			if ((y&1) == 0)
			{
				unswapped_fn( dst, src, src_offset, ti.GetWidth(), palette );
			}
			else
			{
				swapped_fn( dst, src, src_offset, ti.GetWidth(), palette );
			}

			src_offset += src_pitch;
			dst = reinterpret_cast< NativePf8888 * >( (u8*)dst + dsti.Pitch );
		}
	}
	else
	{
		for (u32 y = 0; y < ti.GetHeight(); y++)
		{
			unswapped_fn( dst, src, src_offset, ti.GetWidth(), palette );

			src_offset += src_pitch;
			dst = reinterpret_cast< NativePf8888 * >( (u8*)dst + dsti.Pitch );
		}
	}
}

template<typename OutT>
static void ConvertPalettisedToCI( const TextureDestInfo & dsti, const TextureInfo & ti,
								   void (*swapped_fn)( OutT * dst, const u8 * src, u32 src_offset, u32 width ),
								   void (*unswapped_fn)( OutT * dst, const u8 * src, u32 src_offset, u32 width ) )
{
	OutT *		dst        = reinterpret_cast< OutT * >( dsti.Data );
	const u8 *	src        = g_pu8RamBase;
	u32			src_offset = ti.GetLoadAddress();
	u32			src_pitch  = ti.GetPitch();

	if (ti.IsSwapped())
	{
		for (u32 y = 0; y < ti.GetHeight(); y++)
		{
			if ((y&1) == 0)
			{
				unswapped_fn( dst, src, src_offset, ti.GetWidth() );
			}
			else
			{
				swapped_fn( dst, src, src_offset, ti.GetWidth() );
			}

			src_offset += src_pitch;
			dst = reinterpret_cast< OutT * >( (u8*)dst + dsti.Pitch );
		}
	}
	else
	{
		for (u32 y = 0; y < ti.GetHeight(); y++)
		{
			unswapped_fn( dst, src, src_offset, ti.GetWidth() );

			src_offset += src_pitch;
			dst = reinterpret_cast< OutT * >( (u8*)dst + dsti.Pitch );
		}
	}
}

template < typename InT >
struct SConvert
{
	enum { Fiddle = SByteswapInfo< sizeof( InT ) >::Fiddle };
	enum { Swizzle = SSwizzleInfo< sizeof( InT ) >::Swizzle };

	//
	//	This routine converts from any format which is > 1 byte to any Psp format.
	//
	template < typename OutT, u32 InFiddle, u32 OutFiddle >
	static inline void ConvertRow( OutT * dst, const u8 * src, u32 src_offset, u32 width )
	{
		DAEDALUS_DL_ASSERT( IsAligned( src_offset, sizeof( InT ) ), "Offset should be correctly aligned" );

		//
		//	Need to be careful of this - ensure that it's doing the right thing in all cases and not overflowing rows.
		//	This is to ensure that we correctly convert all the texels in a row, even when we're fiddling.
		//	If we have a fiddle of 2 for instance, and the row is not a multiple of the fiddle amount
		//	then we don't convert enough pixels (we actually poke some values in past the end of the row)
		//	and get some random noise at the end instead.
		//
		//	There may well be an easier (and less gross)  way of doing this if we move the OutFiddle calculation
		//	into the source pixel lookup, and just have dst[x] = ...
		//
		width = AlignPow2( width, 1<<OutFiddle );

		for (u32 x = 0; x < width; x++)
		{
			InT	colour( *reinterpret_cast< const InT * >( &src[src_offset ^ InFiddle] ) );

			dst[x ^ OutFiddle] = ConvertPixelFormat< OutT, InT >( colour );

			src_offset += sizeof( InT );
		}
	}

	template < typename OutT >
	static inline void ConvertTextureT( const TextureDestInfo & dsti, const TextureInfo & ti )
	{
		SConvertGeneric< OutT >::ConvertGeneric( dsti, ti,
												 ConvertRow< OutT, Fiddle, Swizzle >,
												 ConvertRow< OutT, Fiddle, 0 > );
	}

	static void ConvertTexture( const TextureDestInfo & dsti, const TextureInfo & ti )
	{
		switch( dsti.Format )
		{
		case TexFmt_5650:	ConvertTextureT< NativePf5650 >( dsti, ti ); return;
		case TexFmt_5551:	ConvertTextureT< NativePf5551 >( dsti, ti ); return;
		case TexFmt_4444:	ConvertTextureT< NativePf4444 >( dsti, ti ); return;
		case TexFmt_8888:	ConvertTextureT< NativePf8888 >( dsti, ti ); return;

		case TexFmt_CI4_8888: break;
		case TexFmt_CI8_8888: break;

		}

		DAEDALUS_DL_ERROR( "Unhandled format" );
	}
};

struct SConvertIA4
{
	enum { Fiddle = 0x3 };

	template < typename OutT, u32 F >
	static inline void ConvertRow( OutT * dst, const u8 * src, u32 src_offset, u32 width )
	{
		// Do two pixels at a time
		for (u32 x = 0; x < width; x+=2)
		{
			u8 b = src[src_offset ^ F];

			// Even
			dst[x + 0] = OutT( ThreeToEight[(b & 0xE0) >> 5],
							   ThreeToEight[(b & 0xE0) >> 5],
							   ThreeToEight[(b & 0xE0) >> 5],
							     OneToEight[(b & 0x10) >> 4]);
			// Odd
			dst[x + 1] = OutT( ThreeToEight[(b & 0x0E) >> 1],
							   ThreeToEight[(b & 0x0E) >> 1],
							   ThreeToEight[(b & 0x0E) >> 1],
							     OneToEight[(b & 0x01)     ] );
			src_offset++;
		}

		if(width & 1)
		{
			u8 b = src[src_offset ^ F];

			// Even
			dst[width-1] = OutT( ThreeToEight[(b & 0xE0) >> 5],
								 ThreeToEight[(b & 0xE0) >> 5],
								 ThreeToEight[(b & 0xE0) >> 5],
								   OneToEight[(b & 0x10) >> 4]);
		}
	}

	template < typename OutT >
	static inline void ConvertTextureT( const TextureDestInfo & dsti, const TextureInfo & ti )
	{
		SConvertGeneric< OutT >::ConvertGeneric( dsti, ti, ConvertRow< OutT, 0x4 | Fiddle >, ConvertRow< OutT, Fiddle > );
	}

	static void ConvertTexture( const TextureDestInfo & dsti, const TextureInfo & ti )
	{
		switch( dsti.Format )
		{
		case TexFmt_5650:	ConvertTextureT< NativePf5650 >( dsti, ti ); return;
		case TexFmt_5551:	ConvertTextureT< NativePf5551 >( dsti, ti ); return;
		case TexFmt_4444:	ConvertTextureT< NativePf4444 >( dsti, ti ); return;
		case TexFmt_8888:	ConvertTextureT< NativePf8888 >( dsti, ti ); return;

		case TexFmt_CI4_8888: break;
		case TexFmt_CI8_8888: break;

		}

		DAEDALUS_DL_ERROR( "Unhandled format" );
	}
};

struct SConvertI4
{
	enum { Fiddle = 0x3 };

	template< typename OutT, u32 F >
	static inline void ConvertRow( OutT * dst, const u8 * src, u32 src_offset, u32 width )
	{
		// Do two pixels at a time
		for ( u32 x = 0; x+1 < width; x+=2 )
		{
			u8 b = src[src_offset ^ F];

			// Even
			dst[x + 0] = OutT( FourToEight[(b & 0xF0)>>4],
							   FourToEight[(b & 0xF0)>>4],
							   FourToEight[(b & 0xF0)>>4],
							   FourToEight[(b & 0xF0)>>4] );
			// Odd
			dst[x + 1] = OutT( FourToEight[(b & 0x0F)],
							   FourToEight[(b & 0x0F)],
							   FourToEight[(b & 0x0F)],
							   FourToEight[(b & 0x0F)] );

			src_offset++;
		}

		if(width & 1)
		{
			u8 b = src[src_offset ^ F];

			// Even
			dst[width-1] = OutT( FourToEight[(b & 0xF0)>>4],
								 FourToEight[(b & 0xF0)>>4],
								 FourToEight[(b & 0xF0)>>4],
								 FourToEight[(b & 0xF0)>>4] );

		}
	}

	template < typename OutT >
	static inline void ConvertTextureT( const TextureDestInfo & dsti, const TextureInfo & ti )
	{
		SConvertGeneric< OutT >::ConvertGeneric( dsti, ti, ConvertRow< OutT, 0x4 | Fiddle >, ConvertRow< OutT, Fiddle > );
	}

	static void ConvertTexture( const TextureDestInfo & dsti, const TextureInfo & ti )
	{
		switch( dsti.Format )
		{
		case TexFmt_5650:	ConvertTextureT< NativePf5650 >( dsti, ti ); return;
		case TexFmt_5551:	ConvertTextureT< NativePf5551 >( dsti, ti ); return;
		case TexFmt_4444:	ConvertTextureT< NativePf4444 >( dsti, ti ); return;
		case TexFmt_8888:	ConvertTextureT< NativePf8888 >( dsti, ti ); return;

		case TexFmt_CI4_8888: break;
		case TexFmt_CI8_8888: break;

		}

		DAEDALUS_DL_ERROR( "Unhandled format" );
	}
};

static void ConvertPalette(ETLutFmt tlut_format, NativePf8888 * dst, const void * src, u32 count)
{
	if( tlut_format == kTT_IA16 )
	{
		const N64PfIA16 * palette = static_cast< const N64PfIA16 * >( src );

		for( u32 i = 0; i < count; ++i )
		{
			dst[ i ] = NativePf8888::Make( palette[ i ^ U16H_TWIDDLE ] );
		}
	}
	else //if( tlut_format == kTT_RGBA16 )
	{
		// NB: assume RGBA for all other tlut_formats.
		const N64Pf5551 * palette = static_cast< const N64Pf5551 * >( src );

		for( u32 i = 0; i < count; ++i )
		{
			dst[ i ] = NativePf8888::Make( palette[ i ^ U16H_TWIDDLE ] );
		}
	}
}

template< u32 F >
static void ConvertCI4_Row( NativePfCI44 * dst, const u8 * src, u32 src_offset, u32 width )
{
	for (u32 x = 0; x+1 < width; x+=2)
	{
		u8 b = src[src_offset ^ F];

		dst[ x/2 ].Bits = (b >> 4) | (b << 4);

		src_offset++;
	}

	// Handle any remaining odd pixels
	if( width & 1 )
	{
		u8 b = src[src_offset ^ F];

		dst[ width/2 ].Bits = (b >> 4) | 0;
	}
}

template< u32 F >
static void ConvertCI4_Row_To_8888( NativePf8888 * dst, const u8 * src, u32 src_offset, u32 width, const NativePf8888 * palette )
{
	DAEDALUS_ASSERT(palette, "No palette");

	for (u32 x = 0; x+1 < width; x+=2)
	{
		u8 b = src[src_offset ^ F];

		u8 bhi = (b&0xf0)>>4;
		u8 blo = (b&0x0f);

		dst[ x + 0 ] = palette[ bhi ];	// Remember palette has already been swapped
		dst[ x + 1 ] = palette[ blo ];

		src_offset++;
	}

	// Handle any remaining odd pixels
	if(width & 1)
	{
		u8 b = src[src_offset ^ F];

		u8 bhi = (b&0xf0)>>4;

		dst[width-1] = palette[ bhi ];	// Remember palette has already been swapped
	}
}

template< u32 F >
static void ConvertCI8_Row( NativePfCI8 * dst, const u8 * src, u32 src_offset, u32 width )
{
	for (u32 x = 0; x < width; x++)
	{
		dst[ x ].Bits = src[src_offset ^ F];
		src_offset++;
	}
}

template< u32 F >
static  void ConvertCI8_Row_To_8888( NativePf8888 * dst, const u8 * src, u32 src_offset, u32 width, const NativePf8888 * palette )
{
	DAEDALUS_ASSERT(palette, "No palette");

	for (u32 x = 0; x < width; x++)
	{
		u8 b     = src[src_offset ^ F];
		dst[ x ] = palette[ b ];	// Remember palette has already been swapped
		src_offset++;
	}
}

static void ConvertRGBA16(const TextureDestInfo & dsti, const TextureInfo & ti)
{
	SConvert< N64Pf5551 >::ConvertTexture( dsti, ti );
}

static void ConvertRGBA32(const TextureDestInfo & dsti, const TextureInfo & ti)
{
	// Did have Fiddle of 8 here, pretty sure this was wrong (should have been 4)
	SConvert< N64Pf8888 >::ConvertTexture( dsti, ti );
}

static void ConvertIA4(const TextureDestInfo & dsti, const TextureInfo & ti)
{
	SConvertIA4::ConvertTexture( dsti, ti );
}

static void ConvertIA8(const TextureDestInfo & dsti, const TextureInfo & ti)
{
	SConvert< N64PfIA8 >::ConvertTexture( dsti, ti );
}

static void ConvertIA16(const TextureDestInfo & dsti, const TextureInfo & ti)
{
	SConvert< N64PfIA16 >::ConvertTexture( dsti, ti );
}

static void ConvertI4(const TextureDestInfo & dsti, const TextureInfo & ti)
{
	SConvertI4::ConvertTexture( dsti, ti );
}

static void ConvertI8(const TextureDestInfo & dsti, const TextureInfo & ti)
{
	SConvert< N64PfI8 >::ConvertTexture( dsti, ti );
}

static void ConvertCI8(const TextureDestInfo & dsti, const TextureInfo & ti)
{
	DAEDALUS_ASSERT(ti.GetTlutAddress(), "No TLUT address");

	NativePf8888 temp_palette[256];

	NativePf8888 *	dst_palette = dsti.Palette ? reinterpret_cast< NativePf8888 * >( dsti.Palette ) : temp_palette;
	const void * 	src_palette = reinterpret_cast< const void * >( ti.GetTlutAddress() );

	ConvertPalette(ti.GetTLutFormat(), dst_palette, src_palette, 256);

	switch( dsti.Format )
	{
	case TexFmt_8888:
		ConvertPalettisedTo8888( dsti, ti, dst_palette,
								 ConvertCI8_Row_To_8888< 0x4 | 0x3 >,
								 ConvertCI8_Row_To_8888< 0x3 > );
		break;

	case TexFmt_CI8_8888:
		ConvertPalettisedToCI( dsti, ti,
							   ConvertCI8_Row< 0x4 | 0x3 >,
							   ConvertCI8_Row< 0x3 > );
		break;

	default:
		DAEDALUS_ERROR( "Unhandled format for CI8 textures" );
		break;
	}
}

static void ConvertCI4(const TextureDestInfo & dsti, const TextureInfo & ti)
{
	DAEDALUS_ASSERT(ti.GetTlutAddress(), "No TLUT address");

	NativePf8888 temp_palette[16];

	NativePf8888 *	dst_palette = dsti.Palette ? reinterpret_cast< NativePf8888 * >( dsti.Palette ) : temp_palette;
	const void * 	src_palette = reinterpret_cast< const void * >( ti.GetTlutAddress() );

	ConvertPalette(ti.GetTLutFormat(), dst_palette, src_palette, 16);

	switch( dsti.Format )
	{
	case TexFmt_8888:
		ConvertPalettisedTo8888( dsti, ti, dst_palette,
								 ConvertCI4_Row_To_8888< 0x4 | 0x3 >,
								 ConvertCI4_Row_To_8888< 0x3 > );
		break;

	case TexFmt_CI4_8888:
		ConvertPalettisedToCI( dsti, ti,
							   ConvertCI4_Row< 0x4 | 0x3 >,
							   ConvertCI4_Row< 0x3 > );
		break;

	default:
		DAEDALUS_ERROR( "Unhandled format for CI4 textures" );
		break;
	}
}

} // anonymous namespace

typedef void ( *ConvertFunction )( const TextureDestInfo & dsti, const TextureInfo & ti);
static const ConvertFunction gConvertFunctions[ 32 ] =
{
	// 4bpp				8bpp			16bpp				32bpp
	NULL,			NULL,			ConvertRGBA16,		ConvertRGBA32,			// RGBA
	NULL,			NULL,			NULL,				NULL,					// YUV
	ConvertCI4,		ConvertCI8,		NULL,				NULL,					// CI
	ConvertIA4,		ConvertIA8,		ConvertIA16,		NULL,					// IA
	ConvertI4,		ConvertI8,		NULL,				NULL,					// I
	NULL,			NULL,			NULL,				NULL,					// ?
	NULL,			NULL,			NULL,				NULL,					// ?
	NULL,			NULL,			NULL,				NULL					// ?
};

bool ConvertTexture(const TextureInfo & ti,
					void * texels,
					NativePf8888 * palette,
					ETextureFormat texture_format,
					u32 pitch)
{
	//Do nothing if palette address is NULL or close to NULL in a palette texture //Corn
	//Loading a SaveState (OOT -> SSV) dont bring back our TMEM data which causes issues for the first rendered frame.
	//Checking if the palette pointer is less than 0x1000 (rather than just NULL) fixes it.
	// Seems to happen on the first frame of Goldeneye too?
	if( (ti.GetFormat() == G_IM_FMT_CI) && (ti.GetTlutAddress() < 0x1000) ) return false;

	//memset( texels, 0, buffer_size );

	TextureDestInfo dsti( texture_format );
	dsti.Data    = texels;
	dsti.Width   = ti.GetWidth();
	dsti.Height  = ti.GetHeight();
	dsti.Pitch   = pitch;
	dsti.Palette = palette;

	const ConvertFunction fn = gConvertFunctions[ (ti.GetFormat() << 2) | ti.GetSize() ];
	if( fn )
	{
		fn( dsti, ti );
		return true;
	}

	return false;
}

