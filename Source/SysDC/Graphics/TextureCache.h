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
#include "DaedCritSect.h"
#include "DaedSingleton.h"

struct TextureInfo
{
	u32	TmemAddress;
	u32	TmemPalAddress;

	u32 Address;				// Corresponds to dwAddress in Tile
	s16 Left;
	s16 Top;
	u16 Width;
	u16 Height;
	u32 Pitch;

private:
	unsigned	Format : 3;		// e.g. RGBA, IA
	unsigned	Size : 2;		// e.g. 16bpp
	unsigned	TLutFmt : 2;
	bool		Swapped : 1;	// Are odd lines word swapped?

public:
	TextureInfo()
	{
		memset( this, 0, sizeof( TextureInfo ) );
	}

	u32					GetHashCode() const;

	const char *		GetFormatName() const;
	u32					GetSizeInBits() const;

	u32					GetWidthInBytes() const;
	u32					GetLeftInBytes() const;

	u32					GetFormat() const				{ return Format; }
	u32					GetSize() const					{ return Size; }
	u32					GetTLutFormat() const;
	bool				IsSwapped() const				{ return Swapped; }

	void				SetFormat( u32 format )			{ Format = format; }
	void				SetSize( u32 size )				{ Size = size; }
	void				SetTLutFormat( u32 format );
	void				SetSwapped( bool swapped )		{ Swapped = swapped; }


	// or bool operator==(,,) ?
	BOOL IsEqual(const TextureInfo & ti) const
	{
		return ( memcmp(this, &ti, sizeof(TextureInfo)) == 0 );
	}

};

class CTextureCache : public CCritSect, public CSingleton< CTextureCache >
{
	public:
		virtual ~CTextureCache() {};

		virtual void PurgeOldTextures() = 0;
		virtual void DropTextures() = 0;

		virtual void SetDumpTextures( bool dump_textures ) = 0;
		virtual bool GetDumpTextures( ) const = 0;

		virtual CTexture * GetTexture( const TextureInfo * pti ) = 0;
};

#endif	// __TEXTURECACHE_H__
