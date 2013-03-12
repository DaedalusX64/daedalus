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
#include "CachedTexture.h"

#include "TextureInfo.h"
#include "ConvertImage.h"
#include "ConfigOptions.h"
#include "Graphics/ColourValue.h"
#include "Graphics/NativePixelFormat.h"
#include "Graphics/NativeTexture.h"
#include "Graphics/PngUtil.h"
#include "Graphics/TextureTransform.h"

#include "OSHLE/ultra_gbi.h"

#include "Core/ROM.h"

#include "Debug/DBGConsole.h"
#include "Debug/Dump.h"

#include "Utility/Profiler.h"
#include "Utility/IO.h"
#include "Utility/AuxFunc.h"

#include "Math/MathUtil.h"
#include "Math/Math.h"

#include <vector>

static std::vector<u8>		gTexelBuffer;
static NativePf8888		gPaletteBuffer[ 256 ];

// NB: On the PSP we generate a lightweight hash of the texture data before
// updating the native texture. This avoids some expensive work where possible.
// On other platforms (e.g. OSX) updating textures is relatively inexpensive, so
// we just skip the hashing process entirely, and update textures every frame
// regardless of whether they've actually changed.
#ifdef DAEDALUS_PSP
static const bool kUpdateTexturesEveryFrame = false;
#else
static const bool kUpdateTexturesEveryFrame = true;
#endif


#define DEFTEX	TexFmt_8888

static const ETextureFormat TFmt[ 32 ] =
{
//	4bpp				8bpp				16bpp				32bpp
	DEFTEX,				DEFTEX,				TexFmt_5551,		TexFmt_8888,		// RGBA
	DEFTEX,				DEFTEX,				DEFTEX,				DEFTEX,				// YUV
	TexFmt_CI4_8888,	TexFmt_CI8_8888,	DEFTEX,				DEFTEX,				// CI
	TexFmt_4444,		TexFmt_4444,		TexFmt_8888,		DEFTEX,				// IA
	TexFmt_4444,		TexFmt_8888,		DEFTEX,				DEFTEX,				// I
	DEFTEX,				DEFTEX,				DEFTEX,				DEFTEX,				// ?
	DEFTEX,				DEFTEX,				DEFTEX,				DEFTEX,				// ?
	DEFTEX,				DEFTEX,				DEFTEX,				DEFTEX				// ?
};

static const ETextureFormat TFmt_hack[ 32 ] =
{
//	4bpp				8bpp				16bpp				32bpp
	DEFTEX,				DEFTEX,				TexFmt_4444,		TexFmt_8888,		// RGBA
	DEFTEX,				DEFTEX,				DEFTEX,				DEFTEX,				// YUV
	TexFmt_CI4_8888,	TexFmt_CI8_8888,	DEFTEX,				DEFTEX,				// CI
	TexFmt_4444,		TexFmt_4444,		TexFmt_8888,		DEFTEX,				// IA
	TexFmt_4444,		TexFmt_4444,		DEFTEX,				DEFTEX,				// I
	DEFTEX,				DEFTEX,				DEFTEX,				DEFTEX,				// ?
	DEFTEX,				DEFTEX,				DEFTEX,				DEFTEX,				// ?
	DEFTEX,				DEFTEX,				DEFTEX,				DEFTEX				// ?
};

static ETextureFormat SelectNativeFormat(const TextureInfo & ti)
{
	u32 idx = (ti.GetFormat() << 2) | ti.GetSize();
	return g_ROM.LOAD_T1_HACK ? TFmt_hack[idx] : TFmt[idx];
}

static bool GenerateTexels( void ** p_texels, void ** p_palette, const TextureInfo & texture_info, ETextureFormat texture_format, u32 pitch, u32 buffer_size )
{
	TextureDestInfo dst( texture_format );

	if( gTexelBuffer.size() < buffer_size ) //|| gTexelBuffer.size() > (128 * 1024))//Cut off for downsizing may need to be adjusted to prevent some thrashing
	{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		printf( "Resizing texel buffer to %d bytes. Texture is %dx%d\n", buffer_size, texture_info.GetWidth(), texture_info.GetHeight() );
#endif
		gTexelBuffer.resize( buffer_size );
	}

	void *			texels  = &gTexelBuffer[0];
	NativePf8888 *	palette = IsTextureFormatPalettised( dst.Format ) ? gPaletteBuffer : NULL;

	//memset( texels, 0, buffer_size );

	// Return a temporary buffer to use
	dst.Data    = texels;
	dst.Width   = texture_info.GetWidth();
	dst.Height  = texture_info.GetHeight();
	dst.Pitch   = pitch;
	dst.Palette = palette;

	//Do nothing if palette address is NULL or close to NULL in a palette texture //Corn
	//Loading a SaveState (OOT -> SSV) dont bring back our TMEM data which causes issues for the first rendered frame.
	//Checking if the palette pointer is less than 0x1000 (rather than just NULL) fixes it.
	if( palette && (texture_info.GetTlutAddress() < 0x1000) ) return false;

	if (ConvertTexture(dst, texture_info))
	{
		*p_texels = texels;
		*p_palette = palette;
		return true;
	}

	return false;
}

static void UpdateTexture( const TextureInfo & texture_info, CNativeTexture * texture, const c32 * recolour )
{
	DAEDALUS_PROFILE( "Texture Conversion" );

	DAEDALUS_ASSERT( texture != NULL, "No texture" );

	if ( texture != NULL && texture->HasData() )
	{
		void *	texels;
		void *	palette;

		if( GenerateTexels( &texels, &palette, texture_info, texture->GetFormat(), texture->GetStride(), texture->GetBytesRequired() ) )
		{
			//
			//	Recolour the texels
			//
			if( recolour )
			{
				Recolour( texels, palette, texture_info.GetWidth(), texture_info.GetHeight(), texture->GetStride(), texture->GetFormat(), *recolour );
			}

			//
			//	Clamp edges. We do this so that non power-of-2 textures whose whose width/height
			//	is less than the mask value clamp correctly. It still doesn't fix those
			//	textures with a width which is greater than the power-of-2 size.
			//
			ClampTexels( texels, texture_info.GetWidth(), texture_info.GetHeight(), texture->GetCorrectedWidth(), texture->GetCorrectedHeight(), texture->GetStride(), texture->GetFormat() );

			//
			//	Mirror the texels if required (in-place)
			//
			bool mirror_s = texture_info.GetMirrorS();
			bool mirror_t = texture_info.GetMirrorT();
			if( mirror_s || mirror_t )
			{
				MirrorTexels( mirror_s, mirror_t, texels, texture->GetStride(), texels, texture->GetStride(), texture->GetFormat(), texture_info.GetWidth(), texture_info.GetHeight() );
			}

			texture->SetData( texels, palette );
		}
	}
}

CRefPtr<CachedTexture> CachedTexture::Create( const TextureInfo & ti )
{
	if( ti.GetWidth() == 0 || ti.GetHeight() == 0 )
	{
		DAEDALUS_ERROR( "Trying to create 0 width/height texture" );
		return NULL;
	}

	CRefPtr<CachedTexture>	texture( new CachedTexture( ti ) );

	if (!texture->Initialise())
	{
		texture = NULL;
	}

	return texture;
}

CachedTexture::CachedTexture( const TextureInfo & ti )
:	mTextureInfo( ti )
,	mpTexture(NULL)
,	mpWhiteTexture(NULL)
,	mTextureContentsHash( 0 )
,	mFrameLastUpToDate( gRDPFrame )
,	mFrameLastUsed( gRDPFrame )
{
}

CachedTexture::~CachedTexture()
{
}

bool CachedTexture::Initialise()
{
	DAEDALUS_ASSERT_Q(mpTexture == NULL);

	u32 width  = mTextureInfo.GetWidth();
	u32 height = mTextureInfo.GetHeight();

	if (mTextureInfo.GetMirrorS()) width  *= 2;
	if (mTextureInfo.GetMirrorT()) height *= 2;

	mpTexture = CNativeTexture::Create( width, height, SelectNativeFormat(mTextureInfo) );
	if( mpTexture != NULL )
	{
		// If this we're performing Texture updated checks, randomly offset the
		// 'FrameLastUpToDate' time. This ensures when lots of textures are
		// created on the same frame we update them over a nice distribution of frames.

		if(gCheckTextureHashFrequency > 0)
		{
			mFrameLastUpToDate = gRDPFrame + (FastRand() & (gCheckTextureHashFrequency - 1));
		}
		UpdateTextureHash();
		UpdateTexture( mTextureInfo, mpTexture, NULL );
	}

	return mpTexture != NULL;
}

// Update the hash of the texture. Returns true if the texture should be updated.
bool CachedTexture::UpdateTextureHash()
{
	if (kUpdateTexturesEveryFrame)
	{
		// NB always assume we need updating.
		return true;
	}

	u32 new_hash_value = mTextureInfo.GenerateHashValue();
	bool changed       = new_hash_value != mTextureContentsHash;

	mTextureContentsHash = new_hash_value;
	return changed;
}

void CachedTexture::UpdateIfNecessary()
{
	if( !IsFresh() )
	{
		if (UpdateTextureHash())
		{
			UpdateTexture( mTextureInfo, mpTexture, NULL );
		}

		// FIXME(strmnrmn): should probably recreate mpWhiteTexture if it exists, else it may have stale data.

		mFrameLastUpToDate = gRDPFrame;
	}

	mFrameLastUsed = gRDPFrame;
}

// IsFresh - has this cached texture been updated recently?
bool CachedTexture::IsFresh() const
{
	if (gRDPFrame == mFrameLastUsed)
		return true;

	// If we're not updating textures every frame, check how long it's been
	// since we last updated it.
	if (!kUpdateTexturesEveryFrame)
	{
		return (gCheckTextureHashFrequency == 0 ||
				gRDPFrame < mFrameLastUpToDate + gCheckTextureHashFrequency);
	}

	return false;
}

bool CachedTexture::HasExpired() const
{
	if (!kUpdateTexturesEveryFrame)
	{
		if (!IsFresh())
		{
			//Hack to make WONDER PROJECT J2 work (need to reload some textures every frame!) //Corn
			if( (g_ROM.GameHacks == WONDER_PROJECTJ2) && (mTextureInfo.GetTLutFormat() == kTT_RGBA16) && (mTextureInfo.GetSize() == G_IM_SIZ_8b) ) return true;

			//Hack for Worms Armageddon
			if( (g_ROM.GameHacks == WORMS_ARMAGEDDON) && (mTextureInfo.GetSize() == G_IM_SIZ_8b) && (mTextureContentsHash != mTextureInfo.GenerateHashValue()) ) return true;

			//Hack for Zelda OOT & MM text (only needed if there is not a general hash check) //Corn
			if( g_ROM.ZELDA_HACK && (mTextureInfo.GetSize() == G_IM_SIZ_4b) && mTextureContentsHash != mTextureInfo.GenerateHashValue() ) return true;

			//Check if texture has changed
			//if( mTextureContentsHash != mTextureInfo.GenerateHashValue() ) return true;
		}
	}

	//Otherwise we wait 20+random(0-3) frames before trashing the texture if unused
	//Spread trashing them over time so not all get killed at once (lower value uses less VRAM) //Corn
	return gRDPFrame - mFrameLastUsed > (20 + (FastRand() & 0x3));
}

const CRefPtr<CNativeTexture> &	CachedTexture::GetWhiteTexture() const
{
	if(mpWhiteTexture == NULL)
	{
		DAEDALUS_ASSERT( mpTexture != NULL, "There is no existing texture" );

		CRefPtr<CNativeTexture>	new_texture( CNativeTexture::Create( mpTexture->GetWidth(), mpTexture->GetHeight(), mpTexture->GetFormat() ) );
		if( new_texture && new_texture->HasData() )
		{
			UpdateTexture( mTextureInfo, new_texture, &c32::White );
		}
		mpWhiteTexture = new_texture;
	}

	return mpWhiteTexture;
}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
void CachedTexture::DumpTexture() const
{
	if( mpTexture != NULL && mpTexture->HasData() )
	{
		char filename[MAX_PATH+1];
		char filepath[MAX_PATH+1];
		char dumpdir[MAX_PATH+1];

		IO::Path::Combine( dumpdir, g_ROM.settings.GameName.c_str(), "Textures" );

		Dump_GetDumpDirectory( filepath, dumpdir );

		sprintf( filename, "%08x-%s_%dbpp-%dx%d-%dx%d.png",
							mTextureInfo.GetLoadAddress(), mTextureInfo.GetFormatName(), mTextureInfo.GetSizeInBits(),
							0, 0,		// Left/Top
							mTextureInfo.GetWidth(), mTextureInfo.GetHeight() );

		IO::Path::Append( filepath, filename );

		void *	texels;
		void *	palette;

		// Note that we re-convert the texels because those in the native texture may well already
		// be swizzle. Maybe we should just have an unswizzle routine?
		if( GenerateTexels( &texels, &palette, mTextureInfo, mpTexture->GetFormat(), mpTexture->GetStride(), mpTexture->GetBytesRequired() ) )
		{
			// NB - this does not include the mirrored texels

			// NB we use the palette from the native texture. This is a total hack.
			// We have to do this because the palette texels come from emulated tmem, rather
			// than ram. This means that when we dump out the texture here, tmem won't necessarily
			// contain our pixels.
			#ifdef DAEDALUS_PSP
			const void * native_palette = mpTexture->GetPalette();
			#else
			const void * native_palette = NULL;
			#endif

			PngSaveImage( filepath, texels, native_palette, mpTexture->GetFormat(), mpTexture->GetStride(), mTextureInfo.GetWidth(), mTextureInfo.GetHeight(), true );
		}
	}
}
#endif // DAEDALUS_DEBUG_DISPLAYLIST
