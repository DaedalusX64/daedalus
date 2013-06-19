#include "stdafx.h"

#ifdef DAEDALUS_ACCURATE_TMEM
#include "ConvertTile.h"
#include "RDP.h"
#include "Core/ROM.h"
#include "TextureInfo.h"
#include "Graphics/NativePixelFormat.h"
#include "Utility/Alignment.h"

#include <vector>

struct TileDestInfo
{
	explicit TileDestInfo( ETextureFormat tex_fmt )
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

static const u8 OneToEight[] = {
	0x00,   // 0 -> 00 00 00 00
	0xff    // 1 -> 11 11 11 11
};

static const u8 ThreeToEight[] = {
	0x00,   // 000 -> 00 00 00 00
	0x24,   // 001 -> 00 10 01 00
	0x49,   // 010 -> 01 00 10 01
	0x6d,   // 011 -> 01 10 11 01
	0x92,   // 100 -> 10 01 00 10
	0xb6,   // 101 -> 10 11 01 10
	0xdb,   // 110 -> 11 01 10 11
	0xff    // 111 -> 11 11 11 11
};

static const u8 FourToEight[] = {
	0x00, 0x11, 0x22, 0x33,
	0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xaa, 0xbb,
	0xcc, 0xdd, 0xee, 0xff
};

static const u8 FiveToEight[] = {
	0x00, // 00000 -> 00000000
	0x08, // 00001 -> 00001000
	0x10, // 00010 -> 00010000
	0x18, // 00011 -> 00011000
	0x21, // 00100 -> 00100001
	0x29, // 00101 -> 00101001
	0x31, // 00110 -> 00110001
	0x39, // 00111 -> 00111001
	0x42, // 01000 -> 01000010
	0x4a, // 01001 -> 01001010
	0x52, // 01010 -> 01010010
	0x5a, // 01011 -> 01011010
	0x63, // 01100 -> 01100011
	0x6b, // 01101 -> 01101011
	0x73, // 01110 -> 01110011
	0x7b, // 01111 -> 01111011

	0x84, // 10000 -> 10000100
	0x8c, // 10001 -> 10001100
	0x94, // 10010 -> 10010100
	0x9c, // 10011 -> 10011100
	0xa5, // 10100 -> 10100101
	0xad, // 10101 -> 10101101
	0xb5, // 10110 -> 10110101
	0xbd, // 10111 -> 10111101
	0xc6, // 11000 -> 11000110
	0xce, // 11001 -> 11001110
	0xd6, // 11010 -> 11010110
	0xde, // 11011 -> 11011110
	0xe7, // 11100 -> 11100111
	0xef, // 11101 -> 11101111
	0xf7, // 11110 -> 11110111
	0xff  // 11111 -> 11111111
 };

ALIGNED_EXTERN(u8, gTMEM[4096], 16);

static void ConvertRGBA32(const TileDestInfo & dsti, const TextureInfo & ti)
{
	u32 width = dsti.Width;
	u32 height = dsti.Height;

	u8 * dst = static_cast<u8*>(dsti.Data);
	u32 dst_row_stride = dsti.Pitch;
	u32 dst_row_offset = 0;

	const u8 * src     = gTMEM;
	u32 src_row_stride = ti.GetLine()<<3;
	u32 src_row_offset = ti.GetTmemAddress()<<3;

	// NB! RGBA/32 line needs to be doubled.
	src_row_stride *= 2;

	u32 row_swizzle = 0;
	for (u32 y = 0; y < height; ++y)
	{
		u32 src_offset = src_row_offset;
		u32 dst_offset = dst_row_offset;
		for (u32 x = 0; x < width; ++x)
		{
			u32 o = src_offset^row_swizzle;

			dst[dst_offset+0] = src[o];
			dst[dst_offset+1] = src[o+1];
			dst[dst_offset+2] = src[o+2];
			dst[dst_offset+3] = src[o+3];

			src_offset += 4;
			dst_offset += 4;
		}
		src_row_offset += src_row_stride;
		dst_row_offset += dst_row_stride;

		row_swizzle ^= 0x8;   // Alternate lines are qword-swapped
	}
}

static void ConvertRGBA16(const TileDestInfo & dsti, const TextureInfo & ti)
{
	u32 width = dsti.Width;
	u32 height = dsti.Height;

	u8 * dst = static_cast<u8*>(dsti.Data);
	u32 dst_row_stride = dsti.Pitch;
	u32 dst_row_offset = 0;

	const u8 * src     = gTMEM;
	u32 src_row_stride = ti.GetLine()<<3;
	u32 src_row_offset = ti.GetTmemAddress()<<3;

	u32 row_swizzle = 0;
	for (u32 y = 0; y < height; ++y)
	{
		u32 src_offset = src_row_offset;
		u32 dst_offset = dst_row_offset;
		for (u32 x = 0; x < width; ++x)
		{
			u32 o         = src_offset^row_swizzle;
			u16 src_pixel = (src[o]<<8) | src[o+1];

			dst[dst_offset+0] = FiveToEight[(src_pixel>>11)&0x1f];
			dst[dst_offset+1] = FiveToEight[(src_pixel>> 6)&0x1f];
			dst[dst_offset+2] = FiveToEight[(src_pixel>> 1)&0x1f];
			dst[dst_offset+3] = ((src_pixel     )&0x01)? 255 : 0;

			src_offset += 2;
			dst_offset += 4;
		}
		src_row_offset += src_row_stride;
		dst_row_offset += dst_row_stride;

		row_swizzle ^= 0x4;   // Alternate lines are word-swapped
	}
}

template <u32 (*PalConvertFn)(u16)>
static void ConvertCI8T(const TileDestInfo & dsti, const TextureInfo & ti)
{
	u32 width = dsti.Width;
	u32 height = dsti.Height;

	u32 * dst = static_cast<u32*>(dsti.Data);
	u32 dst_row_stride = dsti.Pitch / sizeof(u32);
	u32 dst_row_offset = 0;

	const u8 * src     = gTMEM;
	u32 src_row_stride = ti.GetLine()<<3;
	u32 src_row_offset = ti.GetTmemAddress()<<3;

	// Convert the palette once, here.
	u32 pal_address = 0x100;
	u32 pal_offset = pal_address << 3;
	u32 palette[256];
	for (u32 i = 0; i < 256; ++i)
	{
		u16 src_pixel = (src[pal_offset + i*2 + 0]<<8) | src[pal_offset + i*2 + 1];
		palette[i] = PalConvertFn(src_pixel);
	}

	u32 row_swizzle = 0;
	for (u32 y = 0; y < height; ++y)
	{
		u32 src_offset = src_row_offset;
		u32 dst_offset = dst_row_offset;
		for (u32 x = 0; x < width; ++x)
		{
			u8 src_pixel = src[src_offset^row_swizzle];

			dst[dst_offset+0] = palette[src_pixel];

			src_offset += 1;
			dst_offset += 1;
		}
		src_row_offset += src_row_stride;
		dst_row_offset += dst_row_stride;

		row_swizzle ^= 0x4;   // Alternate lines are word-swapped
	}
}

template <u32 (*PalConvertFn)(u16)>
static void ConvertCI4T(const TileDestInfo & dsti, const TextureInfo & ti)
{
	u32 width = dsti.Width;
	u32 height = dsti.Height;

	u32 * dst = static_cast<u32*>(dsti.Data);
	u32 dst_row_stride = dsti.Pitch / sizeof(u32);
	u32 dst_row_offset = 0;

	const u8 * src     = gTMEM;
	u32 src_row_stride = ti.GetLine()<<3;
	u32 src_row_offset = ti.GetTmemAddress()<<3;

	// Convert the palette once, here.
	u32 pal_address = 0x100 + ((ti.GetPalette() * 16 * 2) >> 3);

	// Animal Crossing, Majora's Mask, SSV, Banjo K's N64 logo
	// Would be nice to have a proper fix
	if(g_ROM.TLUT_HACK)
		pal_address = 0x100 + (ti.GetPalette() << 4);

	u32 pal_offset = pal_address << 3;
	u32 palette[16];
	for (u32 i = 0; i < 16; ++i)
	{
		u16 src_pixel = (src[pal_offset + i*2 + 0]<<8) | src[pal_offset + i*2 + 1];
		palette[i] = PalConvertFn(src_pixel);
	}


	u32 row_swizzle = 0;
	for (u32 y = 0; y < height; ++y)
	{
		u32 src_offset = src_row_offset;
		u32 dst_offset = dst_row_offset;

		// Process 2 pixels at a time
		for (u32 x = 0; x+1 < width; x += 2)
		{
			u16 src_pixel = src[src_offset^row_swizzle];

			dst[dst_offset+0] = palette[(src_pixel&0xf0)>>4];
			dst[dst_offset+1] = palette[(src_pixel&0x0f)>>0];

			src_offset += 1;
			dst_offset += 2;
		}

		// Handle trailing pixel, if odd width
		if (width&1)
		{
			u8 src_pixel = src[src_offset^row_swizzle];

			dst[dst_offset+0] = palette[(src_pixel&0xf0)>>4];

			src_offset += 1;
			dst_offset += 1;
		}

		src_row_offset += src_row_stride;
		dst_row_offset += dst_row_stride;

		row_swizzle ^= 0x4;   // Alternate lines are word-swapped
	}
}

u32 RGBA16(u16 v)
{
	u32 r = FiveToEight[(v>>11)&0x1f];
	u32 g = FiveToEight[(v>> 6)&0x1f];
	u32 b = FiveToEight[(v>> 1)&0x1f];
	u32 a = ((v     )&0x01)? 255 : 0;

	return (a<<24) | (b<<16) | (g<<8) | r;
}

u32 IA16(u16 v)
{
	u32 i = (v>>8)&0xff;
	u32 a = (v   )&0xff;

	return (a<<24) | (i<<16) | (i<<8) | i;
}

static void ConvertCI8(const TileDestInfo & dsti, const TextureInfo & ti)
{
	switch (ti.GetTLutFormat())
	{
	case kTT_RGBA16:
		ConvertCI8T< RGBA16 >(dsti, ti);
		break;
	case kTT_IA16:
		ConvertCI8T< IA16 >(dsti, ti);
		break;
	default:
		DAEDALUS_ERROR("Unhandled tlut format %d/%d", ti.GetTLutFormat());
		break;
	}
}

static void ConvertCI4(const TileDestInfo & dsti, const TextureInfo & ti)
{
	switch (ti.GetTLutFormat())
	{
	case kTT_RGBA16:
		ConvertCI4T< RGBA16 >(dsti, ti);
		break;
	case kTT_IA16:
		ConvertCI4T< IA16 >(dsti, ti);
		break;
	default:
		DAEDALUS_ERROR("Unhandled tlut format %d/%d", ti.GetTLutFormat());
		break;
	}
}



static void ConvertIA16(const TileDestInfo & dsti, const TextureInfo & ti)
{
	u32 width = dsti.Width;
	u32 height = dsti.Height;

	u8 * dst = static_cast<u8*>(dsti.Data);
	u32 dst_row_stride = dsti.Pitch;
	u32 dst_row_offset = 0;

	const u8 * src     = gTMEM;
	u32 src_row_stride = ti.GetLine()<<3;
	u32 src_row_offset = ti.GetTmemAddress()<<3;

	u32 row_swizzle = 0;
	for (u32 y = 0; y < height; ++y)
	{
		u32 src_offset = src_row_offset;
		u32 dst_offset = dst_row_offset;
		for (u32 x = 0; x < width; ++x)
		{
			u32 o        = src_offset^row_swizzle;
			//u8 src_pixel = src[o];

			u8 i = src[o+0];
			u8 a = src[o+1];

			dst[dst_offset+0] = i;
			dst[dst_offset+1] = i;
			dst[dst_offset+2] = i;
			dst[dst_offset+3] = a;

			src_offset += 2;
			dst_offset += 4;
		}
		src_row_offset += src_row_stride;
		dst_row_offset += dst_row_stride;

		row_swizzle ^= 0x4;   // Alternate lines are word-swapped
	}
}

static void ConvertIA8(const TileDestInfo & dsti, const TextureInfo & ti)
{
	u32 width = dsti.Width;
	u32 height = dsti.Height;

	u8 * dst = static_cast<u8*>(dsti.Data);
	u32 dst_row_stride = dsti.Pitch;
	u32 dst_row_offset = 0;

	const u8 * src     = gTMEM;
	u32 src_row_stride = ti.GetLine()<<3;
	u32 src_row_offset = ti.GetTmemAddress()<<3;

	u32 row_swizzle = 0;
	for (u32 y = 0; y < height; ++y)
	{
		u32 src_offset = src_row_offset;
		u32 dst_offset = dst_row_offset;
		for (u32 x = 0; x < width; ++x)
		{
			u32 o        = src_offset^row_swizzle;
			u8 src_pixel = src[o];

			u8 i = FourToEight[(src_pixel>>4)&0xf];
			u8 a = FourToEight[(src_pixel   )&0xf];

			dst[dst_offset+0] = i;
			dst[dst_offset+1] = i;
			dst[dst_offset+2] = i;
			dst[dst_offset+3] = a;

			src_offset += 1;
			dst_offset += 4;
		}
		src_row_offset += src_row_stride;
		dst_row_offset += dst_row_stride;

		row_swizzle ^= 0x4;   // Alternate lines are word-swapped
	}
}

static void ConvertIA4(const TileDestInfo & dsti, const TextureInfo & ti)
{
	u32 width = dsti.Width;
	u32 height = dsti.Height;

	u8 * dst = static_cast<u8*>(dsti.Data);
	u32 dst_row_stride = dsti.Pitch;
	u32 dst_row_offset = 0;

	const u8 * src     = gTMEM;
	u32 src_row_stride = ti.GetLine()<<3;
	u32 src_row_offset = ti.GetTmemAddress()<<3;

	u32 row_swizzle = 0;
	for (u32 y = 0; y < height; ++y)
	{
		u32 src_offset = src_row_offset;
		u32 dst_offset = dst_row_offset;

		// Process 2 pixels at a time
		for (u32 x = 0; x+1 < width; x += 2)
		{
			u32 o         = src_offset^row_swizzle;
			u16 src_pixel = src[o];

			u8 i0 = ThreeToEight[(src_pixel&0xe0)>>5];
			u8 a0 =   OneToEight[(src_pixel&0x10)>>4];

			u8 i1 = ThreeToEight[(src_pixel&0x0e)>>1];
			u8 a1 =   OneToEight[(src_pixel&0x01)>>0];

			dst[dst_offset+0] = i0;
			dst[dst_offset+1] = i0;
			dst[dst_offset+2] = i0;
			dst[dst_offset+3] = a0;

			dst[dst_offset+4] = i1;
			dst[dst_offset+5] = i1;
			dst[dst_offset+6] = i1;
			dst[dst_offset+7] = a1;

			src_offset += 1;
			dst_offset += 8;
		}

		// Handle trailing pixel, if odd width
		if (width&1)
		{
			u32 o        = src_offset^row_swizzle;
			u8 src_pixel = src[o];

			u8 i0 = ThreeToEight[(src_pixel&0xe0)>>5];
			u8 a0 =   OneToEight[(src_pixel&0x10)>>4];

			dst[dst_offset+0] = i0;
			dst[dst_offset+1] = i0;
			dst[dst_offset+2] = i0;
			dst[dst_offset+3] = a0;

			src_offset += 1;
			dst_offset += 4;
		}

	  src_row_offset += src_row_stride;
	  dst_row_offset += dst_row_stride;

	  row_swizzle ^= 0x4;   // Alternate lines are word-swapped
	}
}

static void ConvertI8(const TileDestInfo & dsti, const TextureInfo & ti)
{
	u32 width = dsti.Width;
	u32 height = dsti.Height;

	u8 * dst = static_cast<u8*>(dsti.Data);
	u32 dst_row_stride = dsti.Pitch;
	u32 dst_row_offset = 0;

	const u8 * src     = gTMEM;
	u32 src_row_stride = ti.GetLine()<<3;
	u32 src_row_offset = ti.GetTmemAddress()<<3;

	u32 row_swizzle = 0;
	for (u32 y = 0; y < height; ++y)
	{
		u32 src_offset = src_row_offset;
		u32 dst_offset = dst_row_offset;
		for (u32 x = 0; x < width; ++x)
		{
			u32 o = src_offset^row_swizzle;

			u8 i = src[o];

			dst[dst_offset+0] = i;
			dst[dst_offset+1] = i;
			dst[dst_offset+2] = i;
			dst[dst_offset+3] = i;

			src_offset += 1;
			dst_offset += 4;
		}
		src_row_offset += src_row_stride;
		dst_row_offset += dst_row_stride;

		row_swizzle ^= 0x4;   // Alternate lines are word-swapped
	}
}

static void ConvertI4(const TileDestInfo & dsti, const TextureInfo & ti)
{
	u32 width = dsti.Width;
	u32 height = dsti.Height;

	u8 * dst = static_cast<u8*>(dsti.Data);
	u32 dst_row_stride = dsti.Pitch;
	u32 dst_row_offset = 0;

	const u8 * src     = gTMEM;
	u32 src_row_stride = ti.GetLine()<<3;
	u32 src_row_offset = ti.GetTmemAddress()<<3;

	u32 row_swizzle = 0;
	for (u32 y = 0; y < height; ++y)
	{
	  u32 src_offset = src_row_offset;
	  u32 dst_offset = dst_row_offset;

	  // Process 2 pixels at a time
	  for (u32 x = 0; x+1 < width; x += 2)
	  {
		u16 src_pixel = src[src_offset^row_swizzle];

		u8 i0 = FourToEight[(src_pixel&0xf0)>>4];
		u8 i1 = FourToEight[(src_pixel&0x0f)>>0];

		dst[dst_offset+0] = i0;
		dst[dst_offset+1] = i0;
		dst[dst_offset+2] = i0;
		dst[dst_offset+3] = i0;

		dst[dst_offset+4] = i1;
		dst[dst_offset+5] = i1;
		dst[dst_offset+6] = i1;
		dst[dst_offset+7] = i1;

		src_offset += 1;
		dst_offset += 8;
	  }

		// Handle trailing pixel, if odd width
		if (width&1)
		{
			u32 o        = src_offset^row_swizzle;
			u8 src_pixel = src[o];

			u8 i0 = FourToEight[(src_pixel&0xf0)>>4];

			dst[dst_offset+0] = i0;
			dst[dst_offset+1] = i0;
			dst[dst_offset+2] = i0;
			dst[dst_offset+3] = i0;

			src_offset += 1;
			dst_offset += 4;
		}

	  src_row_offset += src_row_stride;
	  dst_row_offset += dst_row_stride;

	  row_swizzle ^= 0x4;   // Alternate lines are word-swapped
	}
}

typedef void ( *ConvertFunction )(const TileDestInfo & dsti, const TextureInfo & ti);
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

bool ConvertTile(const TextureInfo & ti,
				 void * texels,
				 NativePf8888 * palette,
				 ETextureFormat texture_format,
				 u32 pitch)
{
	DAEDALUS_ASSERT(texture_format == TexFmt_8888, "OSX should only use RGBA 8888 textures");

	TileDestInfo dsti( texture_format );
	dsti.Data    = texels;
	dsti.Width   = ti.GetWidth();
	dsti.Height  = ti.GetHeight();
	dsti.Pitch   = pitch;
	dsti.Palette = palette;

	DAEDALUS_ASSERT(ti.GetLine() != 0, "No line");

	const ConvertFunction fn = gConvertFunctions[ (ti.GetFormat() << 2) | ti.GetSize() ];
	if( fn )
	{
		fn( dsti, ti );
		return true;
	}

	DAEDALUS_ERROR("Unhandled format %d/%d", ti.GetFormat(), ti.GetSize());
	return false;
}
#endif //DAEDALUS_ACCURATE_TMEM
