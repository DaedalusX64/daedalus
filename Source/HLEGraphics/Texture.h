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


#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "Utility/RefCounted.h"

#include "Graphics/NativeTexture.h"
#include "TextureDescriptor.h"

extern u32 gRDPFrame;

class c32;
struct TextureInfo;

class CTexture : public CRefCounted
{
	protected:
		CTexture( const TextureInfo & ti );
		~CTexture();

	public:
		static CRefPtr<CTexture>	Create( const TextureInfo & ti );

		inline const CRefPtr<CNativeTexture> &	GetTexture() const					{ return mpTexture; }
		const CRefPtr<CNativeTexture> &			GetRecolouredTexture( c32 colour ) const;				// Returns a texture with the RGB channels set to the specified colour (alpha remains the same)

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		u32								GetVideoMemoryUsage() const;
		u32								GetSystemMemoryUsage() const;
		void							DumpTexture() const;
#endif
		inline const TextureInfo &		GetTextureInfo() const				{ return mTextureInfo; }
		inline	void					Touch() 							{ mFrameLastUsed = gRDPFrame; }
		void							UpdateIfNecessary();
		bool							HasExpired() const;

	private:
				bool					Initialise();
				bool					IsFresh() const;
		static	void					UpdateTexture( const TextureInfo & texture_info, CNativeTexture * texture, bool recolour, c32 colour );

	private:
		TextureInfo						mTextureInfo;

		CRefPtr<CNativeTexture>			mpTexture;
		mutable CRefPtr<CNativeTexture>	mpRecolouredTexture;

		u32								mTextureContentsHash;
		u32								mFrameLastUpToDate;	// Frame # that this was last updated
		u32								mFrameLastUsed;		// Frame # that this was last used
};


#endif	// __TEXTURE_H__
