
#include "Base/Types.h"

#ifdef DAEDALUS_ACCURATE_TMEM
#include "Core/ROM.h"
#include "HLEGraphics/ConvertFormats.h"
#include "HLEGraphics/ConvertTile.h"
#include "HLEGraphics/RDP.h"
#include "HLEGraphics/TextureInfo.h"
#include "Graphics/NativePixelFormat.h"
#include "System/Endian.h"

#include <vector>

struct TileDestInfo
{
	explicit TileDestInfo( ETextureFormat tex_fmt )
		:	Format( tex_fmt )
		,	Width( 0 )
		,	Height( 0 )
		,	Pitch( 0 )
		,	Data( nullptr )
		//,	Palette( nullptr )
	{
	}

	ETextureFormat		Format;
	u32					Width;			// Describes the width of the locked area. Use lPitch to move between successive lines
	u32					Height;			// Describes the height of the locked area
	s32					Pitch;			// Specifies the number of bytes on each row (not necessarily bitdepth*width/8)
	void *				Data;			// Pointer to the top left pixel of the image
	//NativePf8888 *		Palette;
};

extern u8 gTMEM[4096];


static void ConvertRGBA32(const TileDestInfo & dsti, const TextureInfo & ti)
{
	u32 width = dsti.Width;
	u32 height = dsti.Height;

	u32 * dst = static_cast<u32*>(dsti.Data);
	u32 dst_row_stride = dsti.Pitch / sizeof(u32);
	u32 dst_row_offset = 0;

	const u8 * src = gTMEM;
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
			u8 r = src[(o+3)&0xfff];
			u8 b = src[(o+1)&0xfff];
			u8 g = src[(o+2)&0xfff];
			u8 a = src[(o+0)&0xfff];

			dst[dst_offset+0] = RGBA32(r, g, b, a);

			src_offset += 4;
			dst_offset += 1;
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

	u32 * dst = static_cast<u32*>(dsti.Data);
	u32 dst_row_stride = dsti.Pitch / sizeof(u32);
	u32 dst_row_offset = 0;

	const u8 * src = gTMEM;
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
			u8 src_pixel_hi = src[(o+0)&0xfff];
			u8 src_pixel_lo = src[(o+1)&0xfff];
			u16 src_pixel = (src_pixel_hi << 8) | src_pixel_lo;

			dst[dst_offset+0] = RGBA16(src_pixel);

			src_offset += 2;
			dst_offset += 1;
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
	const u16 * src16 = (u16*)src;

	u32 src_row_stride = ti.GetLine()<<3;
	u32 src_row_offset = ti.GetTmemAddress()<<3;

	// Convert the palette once, here.
	u32 palette[256];
	for (u32 i = 0; i < 256; ++i)
	{
		u16 src_pixel = src16[0x400+(i<<2)];
		palette[i] = PalConvertFn(src_pixel);
	}

	u32 row_swizzle = 0;
	for (u32 y = 0; y < height; ++y)
	{
		u32 src_offset = src_row_offset;
		u32 dst_offset = dst_row_offset;
		for (u32 x = 0; x < width; ++x)
		{
			u32 o = src_offset^row_swizzle;
			u8 src_pixel = src[o&0xfff];

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

	const u8 * src =  gTMEM;
	const u16 * src16 = (u16*)src;

	u32 src_row_stride = ti.GetLine()<<3;
	u32 src_row_offset = ti.GetTmemAddress()<<3;

	// Convert the palette once, here.
	u32 pal_address = 0x400 + (ti.GetPalette()<<6);
	u32 palette[16];
	for (u32 i = 0; i < 16; ++i)
	{
		u16 src_pixel = src16[pal_address+(i<<2)];
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
			u32 o = src_offset^row_swizzle;
			u8 src_pixel = src[o&0xfff];

			dst[dst_offset+0] = palette[(src_pixel&0xf0)>>4];
			dst[dst_offset+1] = palette[(src_pixel&0x0f)>>0];

			src_offset += 1;
			dst_offset += 2;
		}

		// Handle trailing pixel, if odd width
		if (width&1)
		{
			u32 o = src_offset^row_swizzle;
			u8 src_pixel = src[o&0xfff];

			dst[dst_offset+0] = palette[(src_pixel&0xf0)>>4];

			src_offset += 1;
			dst_offset += 1;
		}

		src_row_offset += src_row_stride;
		dst_row_offset += dst_row_stride;

		row_swizzle ^= 0x4;   // Alternate lines are word-swapped
	}
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

	u32 * dst = static_cast<u32*>(dsti.Data);
	u32 dst_row_stride = dsti.Pitch / sizeof(u32);
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
			u8 src_pixel_hi = src[(o+0)&0xfff];
			u8 src_pixel_lo = src[(o+1)&0xfff];
			u16 src_pixel = (src_pixel_hi << 8) | src_pixel_lo;

			dst[dst_offset+0] = IA16(src_pixel);

			src_offset += 2;
			dst_offset += 1;
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
			u32 o = src_offset^row_swizzle;
			u8 src_pixel = src[o&0xfff];

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

	u32 * dst = static_cast<u32*>(dsti.Data);
	u32 dst_row_stride = dsti.Pitch / sizeof(u32);
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
			u32 o = src_offset^row_swizzle;
			u8 src_pixel = src[o&0xfff];

			dst[dst_offset+0] = IA4(src_pixel>>4);
			dst[dst_offset+1] = IA4((src_pixel&0xf));

			src_offset += 1;
			dst_offset += 2;
		}

		// Handle trailing pixel, if odd width
		if (width&1)
		{
			u32 o = src_offset^row_swizzle;
			u8 src_pixel = src[o&0xfff];

			dst[dst_offset+0] = IA4(src_pixel>>4);

			src_offset += 1;
			dst_offset += 1;
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
			u8 i = src[o&0xfff];

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

	u32 * dst = static_cast<u32*>(dsti.Data);
	u32 dst_row_stride = dsti.Pitch / sizeof(u32);
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
			u32 o = src_offset^row_swizzle;
			u8 src_pixel = src[o&0xfff];

			dst[dst_offset+0] = I4(src_pixel>>4);
			dst[dst_offset+1] = I4((src_pixel&0xf));

			src_offset += 1;
			dst_offset += 2;
		}

		// Handle trailing pixel, if odd width
		if (width&1)
		{
			u32 o = src_offset^row_swizzle;
			u8 src_pixel = src[o&0xfff];

			dst[dst_offset+0] = I4(src_pixel>>4);

			src_offset += 1;
			dst_offset += 1;
		}

		src_row_offset += src_row_stride;
		dst_row_offset += dst_row_stride;

		row_swizzle ^= 0x4;   // Alternate lines are word-swapped
	}
}

static void ConvertYUV16(const TileDestInfo & dsti, const TextureInfo & ti)
{
	u32 width = dsti.Width;
	u32 height = dsti.Height;

	u32 * dst = static_cast<u32*>(dsti.Data);
	u32 dst_row_stride = dsti.Pitch / sizeof(u32);
	u32 dst_row_offset = 0;

	const u8 * src     = gTMEM;
	u32 src_row_stride = ti.GetLine()<<3;
	u32 src_row_offset = ti.GetTmemAddress()<<3;

	// NB! YUV/16 line needs to be doubled.
	src_row_stride *= 2;
	u32 row_swizzle = 0;

	for (u32 y = 0; y < height; ++y)
	{
		u32 src_offset = src_row_offset;
		u32 dst_offset = dst_row_offset;
		
		// Process 2 pixels at a time
		for (u32 x = 0; x < width; x += 2)
		{
			u32 o = src_offset^row_swizzle;
			s32 y0 = src[(o+1)&0xfff];
			s32 y1 = src[(o+3)&0xfff];
			s32 u0 = src[(o+0)&0xfff];
			s32 v0 = src[(o+2)&0xfff];

			dst[dst_offset+0] = YUV16(y0,u0,v0);
			dst[dst_offset+1] = YUV16(y1,u0,v0);

			src_offset += 4;
			dst_offset += 2;
		}
		src_row_offset += src_row_stride;
		dst_row_offset += dst_row_stride;

		row_swizzle ^= 0x4;   // Alternate lines are word-swapped
	}
}
using ConvertFunction = void (*)(const TileDestInfo & dsti, const TextureInfo & ti);

static const ConvertFunction gConvertFunctions[ 32 ] =
{
	// 4bpp				8bpp			16bpp				32bpp
	nullptr,			nullptr,		ConvertRGBA16,		ConvertRGBA32,			// RGBA
	nullptr,			nullptr,		ConvertYUV16,		nullptr,				// YUV
	ConvertCI4,			ConvertCI8,		nullptr,			nullptr,				// CI
	ConvertIA4,			ConvertIA8,		ConvertIA16,		nullptr,				// IA
	ConvertI4,			ConvertI8,		nullptr,			nullptr,				// I
	nullptr,			nullptr,		nullptr,			nullptr,				// ?
	nullptr,			nullptr,		nullptr,			nullptr,				// ?
	nullptr,			nullptr,		nullptr,			nullptr					// ?
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
	//dsti.Palette = palette;

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
