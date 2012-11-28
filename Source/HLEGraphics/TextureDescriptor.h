/*
Copyright (C) 2001,2007 StrmnNrmn

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

#ifndef TEXTUREDESCRIPTOR_H_
#define TEXTUREDESCRIPTOR_H_

#include "Graphics/TextureFormat.h"

struct TextureInfo
{
private:
	u32			LoadAddress;		// Address to texture surface
	u32			TlutAddress;		// Address to palette
	u16			Width;				// X dimensions
	u16			Height;				// Y dimensions
	u16			Pitch;				// Number of bytes in a texture row

	u32			TmemAddress : 9;	// TMEM address (0x000 - 0x1FF)
	u32			TLutIndex : 4;		// Palette index (0-15)
	u32			Format : 3;			// e.g. RGBA, YUV, CI, IA, I...
	u32			Size : 2;			// e.g. 4bpp, 8bpp, 16bpp, 32bpp
	u32			TLutFmt : 2;		// e.g. ?, ?, RGBA16, IA16
//	u32			Tile : 3;			// e.g. Tile number (0-7)
	bool		Swapped : 1;		// Are odd lines word swapped?
	bool		MirrorS : 1;
	bool		MirrorT : 1;

public:
	// Pretty gross. Needed so that any padding bytes are consistently zeroed.
	TextureInfo()											{ memset( this, 0, sizeof( TextureInfo ) ); }
	TextureInfo( const TextureInfo & rhs )					{ memcpy( this, &rhs, sizeof( TextureInfo ) ); }
	TextureInfo & operator=( const TextureInfo & rhs )		{ memcpy( this, &rhs, sizeof( TextureInfo ) ); return *this;}

	//the hash output should match the number of bits used in the texture cache array
	inline u32				GetHashCode() const				{ u8 *ptr( (u8*)this ); u8 *end_ptr( ptr + sizeof( TextureInfo ) ); u32 hash(0); while( ptr < end_ptr ) hash = ((hash << 1) | (hash >> 0x9)) ^ *ptr++; return hash; }
	//inline u32				GetHashCode() const				{ return murmur2_neutral_hash( reinterpret_cast< const u8 * >( this ), sizeof( TextureInfo ), 0 ); }

	// Compute a hash of the contents of the texture data. Not to be confused with GetHashCode() that hashes the Textureinfo!
	u32						GenerateHashValue() const;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	const char *			GetFormatName() const;
	u32						GetSizeInBits() const;
#endif

	u32						GetWidthInBytes() const;

	inline u32				GetLoadAddress() const			{ return LoadAddress; }
	inline u32				GetTlutAddress() const			{ return TlutAddress; }
	inline u32				GetTmemAddress() const			{ return TmemAddress; }
	inline u32				GetFormat() const				{ return Format; }
	inline u32				GetSize() const					{ return Size; }
	inline u32				GetWidth() const				{ return Width; }
	inline u32				GetHeight() const				{ return Height; }
	inline u32				GetPitch() const				{ return Pitch; }
	inline u32				GetTLutFormat() const			{ return TLutFmt; }
	inline u32				GetTLutIndex() const			{ return TLutIndex; }
	inline bool				IsSwapped() const				{ return Swapped; }
	inline bool				GetMirrorS() const				{ return MirrorS; }
	inline bool				GetMirrorT() const				{ return MirrorT; }

	inline void				SetLoadAddress( u32 address )	{ LoadAddress = address; }
	inline void				SetTlutAddress( u32 address )	{ TlutAddress = address; }
	inline void				SetTmemAddress( u32 address )	{ TmemAddress = address; }
	inline void				SetFormat( u32 format )			{ Format = format; }
	inline void				SetSize( u32 size )				{ Size = size; }
	inline void				SetWidth( u32 width )			{ Width = width; }
	inline void				SetHeight( u32 height )			{ Height = height; }
	inline void				SetPitch( u32 pitch )			{ Pitch = pitch; }
	inline void				SetTLutFormat( u32 format )		{ TLutFmt = format; }
	inline void				SetTLutIndex( u32 index )		{ TLutIndex = index; }
	inline void				SetSwapped( bool swapped )		{ Swapped = swapped; }
	inline void				SetMirrorS( bool mirror_s )		{ MirrorS = mirror_s; }
	inline void				SetMirrorT( bool mirror_t )		{ MirrorT = mirror_t; }

	ETextureFormat			SelectNativeFormat() const;

	inline int				Compare( const TextureInfo & rhs ) const			{ return memcmp( this, &rhs, sizeof( TextureInfo ) ); }
	inline bool				operator==( const TextureInfo & rhs ) const			{ return Compare( rhs ) == 0; }
	inline bool				operator!=( const TextureInfo & rhs ) const			{ return Compare( rhs ) != 0; }
	inline bool				operator<( const TextureInfo & rhs ) const			{ return Compare( rhs ) < 0; }

};

#endif // TEXTUREDESCRIPTOR_H_
