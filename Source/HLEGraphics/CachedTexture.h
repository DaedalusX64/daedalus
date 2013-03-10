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


#ifndef CACHEDTEXTURE_H__
#define CACHEDTEXTURE_H__

#include "Utility/RefCounted.h"

#include "Graphics/NativeTexture.h"
#include "TextureDescriptor.h"

extern u32 gRDPFrame;

class CachedTexture : public CRefCounted
{
	protected:
		CachedTexture( const TextureInfo & ti );
		~CachedTexture();

	public:
		static CRefPtr<CachedTexture>	Create( const TextureInfo & ti );

		inline const CRefPtr<CNativeTexture> &	GetTexture() const					{ return mpTexture; }
		const CRefPtr<CNativeTexture> &			GetWhiteTexture() const;		// Returns a texture with the RGB channels set to white (alpha remains the same)

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

	private:
		TextureInfo						mTextureInfo;

		CRefPtr<CNativeTexture>			mpTexture;
		mutable CRefPtr<CNativeTexture>	mpWhiteTexture;

		u32								mTextureContentsHash;
		u32								mFrameLastUpToDate;	// Frame # that this was last updated
		u32								mFrameLastUsed;		// Frame # that this was last used
};


#endif	// CACHEDTEXTURE_H__
