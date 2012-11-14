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

#ifndef __TEXTURECACHE_H__
#define __TEXTURECACHE_H__

#include "Texture.h"

#include "Utility/Singleton.h"
#include "Utility/RefCounted.h"

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
#include <vector>
#endif

struct TextureInfo;


class CTextureCache : public CSingleton< CTextureCache >
{
	public:
		virtual ~CTextureCache() {};

		virtual void		PurgeOldTextures() = 0;
		virtual void		DropTextures() = 0;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		virtual void		SetDumpTextures( bool dump_textures ) = 0;
		virtual bool		GetDumpTextures( ) const = 0;

		virtual void		DisplayStats() = 0;

		struct STextureInfoSnapshot
		{
		public:
			STextureInfoSnapshot( CTexture * p_texture );

			STextureInfoSnapshot( const STextureInfoSnapshot & rhs );
			STextureInfoSnapshot & operator=( const STextureInfoSnapshot & rhs );

			~STextureInfoSnapshot();

			inline const CRefPtr<CTexture> &	GetTexture() const			{ return Texture; }
		private:
			CRefPtr<CTexture>		Texture;
		};
		virtual void		Snapshot( std::vector< STextureInfoSnapshot > & snapshot ) const = 0;
#endif

		virtual CRefPtr<CTexture>	GetTexture( const TextureInfo * pti ) = 0;
};

#endif	// __TEXTURECACHE_H__
