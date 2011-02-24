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

#ifndef COLOURVALUE_H_
#define COLOURVALUE_H_

class v4;

class c32 /*: public PixelFormats::Psp::Pf8888*/
{
	public:
		c32() {}
		c32( u8 r, u8 g, u8 b ) : mColour( Make( r, g, b, 255 ) ) {}
		c32( u8 r, u8 g, u8 b, u8 a ) : mColour( Make( r, g, b, a ) ) {}
		explicit c32( u32 colour ) : mColour( colour ) {}
		explicit c32( const v4 & colour );

				u32		GetColour() const			{ return mColour; }
				v4		GetColourV4() const;

				u8		GetR() const				{ return u8(mColour      ); }
				u8		GetG() const				{ return u8(mColour >>  8); }
				u8		GetB() const				{ return u8(mColour >> 16); }
				u8		GetA() const				{ return u8(mColour >> 24); }

				c32		Add( c32 colour ) const;
				c32		AddRGB( c32 colour ) const;
				c32		AddA( c32 colour ) const;

				c32		Sub( c32 colour ) const;
				c32		SubRGB( c32 colour ) const;
				c32		SubA( c32 colour ) const;

				c32		Modulate( c32 colour ) const;
				c32		ModulateRGB( c32 colour ) const;
				c32		ModulateA( c32 colour ) const;

				c32		Interpolate( c32 colour, float factor ) const;
				c32		Interpolate( c32 colour, c32 factor ) const;

				c32		ReplicateAlpha() const;

				void	SetBits( c32 colour, u32 mask )		{ mColour = (mColour &(~mask)) | (colour.mColour & mask); }

		static	u32		Make( u8 r, u8 g, u8 b, u8 a )
		{
			return (u32(a) << 24) | (u32(b)<<16) | (u32(g)<<8) | u32(r);
		}

	public:
		static const u32 MASK_A = 0xff000000;
		static const u32 MASK_RGB = 0x00ffffff;
		static const u32 MASK_RGBA = 0xffffffff;

	public:
		static const c32	Black;
		static const c32	White;
		static const c32	Red;
		static const c32	Green;
		static const c32	Blue;
		static const c32	Gold;
		static const c32	Magenta;
		static const c32	Turquoise;
		static const c32	Orange;
		static const c32	Purple;
		static const c32	Grey;
	private:
		u32		mColour;
};
DAEDALUS_STATIC_ASSERT( sizeof( c32 ) == 4 );


#endif // COLOURVALUE_H_
