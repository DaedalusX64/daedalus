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


#ifndef HLEGRAPHICS_CACHEDTEXTURE_H_
#define HLEGRAPHICS_CACHEDTEXTURE_H_

#include "Utility/RefCounted.h"

#include "Graphics/NativeTexture.h"
#include "TextureInfo.h"

extern u32 gRDPFrame;

class CachedTexture
{
	protected:
		explicit CachedTexture( const TextureInfo & ti );
		~CachedTexture();

	public:
		static CachedTexture *			Create( const TextureInfo & ti );

		inline const CRefPtr<CNativeTexture> &	GetTexture() const			{ return mpTexture; }
		inline const TextureInfo &		GetTextureInfo() const				{ return mTextureInfo; }

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		static void						DumpTexture( const TextureInfo & ti, const CNativeTexture * texture );
#endif
		bool							HasExpired() const;

	private:
		friend class CTextureCache;
		void							UpdateIfNecessary();

		bool							Initialise();
		bool							IsFresh() const;
		bool							UpdateTextureHash();

	private:
		const TextureInfo				mTextureInfo;

		CRefPtr<CNativeTexture>			mpTexture;

		u32								mTextureContentsHash;
		u32								mFrameLastUpToDate;	// Frame # that this was last updated
		u32								mFrameLastUsed;		// Frame # that this was last used
};


#endif // HLEGRAPHICS_CACHEDTEXTURE_H_
