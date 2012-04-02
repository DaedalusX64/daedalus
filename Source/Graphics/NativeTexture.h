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


#ifndef NATIVETEXTURE_H_
#define NATIVETEXTURE_H_

#include "Utility/RefCounted.h"

#include "TextureFormat.h"

#include "Math/Vector2.h"

class c32;

class CNativeTexture : public CRefCounted
{
	friend class CRefPtr<CNativeTexture>::_NoAddRefRelease<CNativeTexture>;

		CNativeTexture( u32 w, u32 h, ETextureFormat texture_format );
		~CNativeTexture();

	public:
		static	CRefPtr<CNativeTexture>		Create( u32 width, u32 height, ETextureFormat texture_format );
		static	CRefPtr<CNativeTexture>		CreateFromPng( const char * p_filename, ETextureFormat texture_format );

		void							InstallTexture() const;

		void							SetData( void * data, void * palette );

		inline u32						GetWidth() const				{ return mWidth; }
		inline u32						GetHeight() const				{ return mHeight; }
		inline u32						GetCorrectedWidth() const		{ return mCorrectedWidth; }
		inline u32						GetCorrectedHeight() const		{ return mCorrectedHeight; }
		inline const v2 &				GetScale() const				{ return mScale; }
		inline const f32 &				GetScaleX() const				{ return mScale.x; }
		inline const f32 &				GetScaleY() const				{ return mScale.y; }
		u32								GetStride() const;
		inline ETextureFormat			GetFormat() const				{ return mTextureFormat; }
		inline const void *				GetPalette() const				{ return mpPalette; }
		inline const void *				GetData() const					{ return mpData; }

		inline bool						IsPalettised() const			{ return IsTextureFormatPalettised( mTextureFormat ); }

		u32								GetBytesRequired() const;
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		u32								GetVideoMemoryUsage() const;
		u32								GetSystemMemoryUsage() const;
#endif
		bool							HasData() const;				// If we run out of texture memory, this will return true

	private:
		ETextureFormat		mTextureFormat;
		u32					mWidth;
		u32					mHeight;
		u32					mCorrectedWidth;
		u32					mCorrectedHeight;
		u32					mTextureBlockWidth;		// Multiple of 16 bytes
		v2					mScale;
		void *				mpData;
		void *				mpPalette;
		bool				mIsDataVidMem;
		bool				mIsPaletteVidMem;
		bool				mIsSwizzled;
#ifdef DAEDALUS_ENABLE_ASSERTS
		bool				mPaletteSet;
#endif
};

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
const u32 gPlaceholderTextureWidth( 16 );
const u32 gPlaceholderTextureHeight( 16 );	

ALIGNED_EXTERN(u32,gWhiteTexture[gPlaceholderTextureWidth * gPlaceholderTextureHeight ], DATA_ALIGN);
ALIGNED_EXTERN(u32,gPlaceholderTexture[gPlaceholderTextureWidth * gPlaceholderTextureHeight ], DATA_ALIGN);
ALIGNED_EXTERN(u32,gSelectedTexture[gPlaceholderTextureWidth * gPlaceholderTextureHeight ], DATA_ALIGN);
#endif

#endif	// NATIVETEXTURE_H_
