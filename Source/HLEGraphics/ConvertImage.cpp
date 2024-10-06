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


#include "Base/Types.h"

#include "DLDebug.h"
#include "Core/Memory.h"
#include "Debug/DBGConsole.h"
#include "Graphics/NativePixelFormat.h"
#include "HLEGraphics/ConvertFormats.h"
#include "HLEGraphics/ConvertImage.h"
#include "HLEGraphics/N64PixelFormat.h"
#include "HLEGraphics/RDP.h"
#include "HLEGraphics/TextureInfo.h"
#include "Utility/MathUtil.h"
#include "Ultra/ultra_gbi.h"

namespace
{

struct TextureDestInfo
{
	explicit TextureDestInfo( ETextureFormat tex_fmt )
		:	Format( tex_fmt )
		,	Width( 0 )
		,	Height( 0 )
		,	Pitch( 0 )
		,	Data( nullptr )
		,	Palette( nullptr )
	{
	}

	ETextureFormat		Format;
	u32					Width;			// Describes the width of the locked area. Use lPitch to move between successive lines
	u32					Height;			// Describes the height of the locked area
	s32					Pitch;			// Specifies the number of bytes on each row (not necessarily bitdepth*width/8)
	void *				Data;			// Pointer to the top left pixel of the image
	NativePf8888 *		Palette;
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
	using ConvertRowFunction = void (*) ( OutT * dst, const u8 * src, u32 src_offset, u32 width );


static void ConvertGeneric( const TextureDestInfo & dsti,
							const TextureInfo & ti,
							ConvertRowFunction swapped_fn,
							ConvertRowFunction unswapped_fn )
{
	OutT *		dst        = reinterpret_cast< OutT * >( dsti.Data );
	const u8 *	src  = g_pu8RamBase;
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
using ConvertPalettisedRowFunction = void (*)( NativePf8888 * dst, const u8 * src, u32 src_offset, u32 width, const NativePf8888 * palette );


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
			u8 b {src[src_offset ^ F]};

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
			u8 b {src[src_offset ^ F]};

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
			u8 b {src[src_offset ^ F]};

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
		u8 b {src[src_offset ^ F]};

		dst[ width/2 ].Bits = (b >> 4) | 0;
	}
}

template< u32 F >
static void ConvertCI4_Row_To_8888( NativePf8888 * dst, const u8 * src, u32 src_offset, u32 width, const NativePf8888 * palette )
{
	for (u32 x = 0; x+1 < width; x+=2)
	{
		u8 b = src[src_offset ^ F];

		u32 bhi = (u32)(b&0xf0)>>4;
		u32 blo = (u32)(b&0x0f);

		dst[ x + 0 ] = palette[ bhi ];	// Remember palette has already been swapped
		dst[ x + 1 ] = palette[ blo ];

		src_offset++;
	}

	// Handle any remaining odd pixels
	if(width & 1)
	{
		u8 b = src[src_offset ^ F];

		u8 bhi = (u8)((b&0xf0)>>4);

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
	for (u32 x = 0; x < width; x++)
	{
		u8 b     {src[src_offset ^ F]};
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

	NativePf8888 temp_palette[256];

	NativePf8888 *	dst_palette = dsti.Palette ? reinterpret_cast< NativePf8888 * >( dsti.Palette ) : temp_palette;
	const void * 	src_palette = g_pu8RamBase + ti.GetTlutAddress();

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

static void ConvertYUV16(const TextureDestInfo & dsti, const TextureInfo & ti)
{
	u32 * dst = static_cast<u32*>(dsti.Data);
	u32 dst_row_stride = dsti.Pitch / sizeof(u32);
	u32 dst_row_offset = 0;

	const u8 * src = g_pu8RamBase;
	u32 src_row_stride = ti.GetPitch();
	u32 src_row_offset = ti.GetLoadAddress();

	u32 width = ti.GetWidth();
	u32 height = ti.GetHeight();

	// NB! YUV/16 line needs to be doubled.
	src_row_stride *= 2;

	if (ti.IsSwapped())
	{
		//TODO: This should be easy to implement but I would like to find first a game that uses it
		DAEDALUS_ERROR("Swapped YUV16 textures are not supported yet");
	}
	else
	{
		for (u32 y = 0; y < height; y++)
		{
			u32 src_offset = src_row_offset;
			u32 dst_offset = dst_row_offset;

			// Do two pixels at a time
			for (u32 x = 0; x < width; x += 2)
			{
				s32 y0 = src[src_offset+2];
				s32 y1 = src[src_offset+0];
				s32 u0 = src[src_offset+3];
				s32 v0 = src[src_offset+1];

				dst[dst_offset+0] = YUV16(y0,u0,v0);
				dst[dst_offset+1] = YUV16(y1,u0,v0);

				src_offset += 4;
				dst_offset += 2;
			}
			src_row_offset += src_row_stride;
			dst_row_offset += dst_row_stride;	
		}
	}
}

static void ConvertCI4(const TextureDestInfo & dsti, const TextureInfo & ti)
{
	NativePf8888 temp_palette[16];

	NativePf8888 *	dst_palette = dsti.Palette ? reinterpret_cast< NativePf8888 * >( dsti.Palette ) : temp_palette;
	const void * 	src_palette = g_pu8RamBase + ti.GetTlutAddress();

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
using ConvertFunction = void (*)( const TextureDestInfo & dsti, const TextureInfo & ti);

static const ConvertFunction gConvertFunctions[ 32 ] =
{
	// 4bpp          8bpp              16bpp				32bpp
	nullptr,         nullptr,      	ConvertRGBA16,    ConvertRGBA32,// RGBA
	nullptr,         nullptr,      	ConvertYUV16,     nullptr,		// YUV
	ConvertCI4,      ConvertCI8,   	nullptr,          nullptr,		// CI
	ConvertIA4,      ConvertIA8,   	ConvertIA16,      nullptr,		// IA
	ConvertI4,       ConvertI8,		nullptr,          nullptr,		// I
	nullptr,         nullptr,       nullptr,          nullptr,		// ?
	nullptr,         nullptr,       nullptr,          nullptr,		// ?
	nullptr,         nullptr,       nullptr,          nullptr		// ?
};

bool ConvertTexture(const TextureInfo & ti,
					void * texels,
					NativePf8888 * palette,
					ETextureFormat texture_format,
					u32 pitch)
{
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
