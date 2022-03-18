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

#ifndef GRAPHICS_NATIVEPIXELFORMAT_H_
#define GRAPHICS_NATIVEPIXELFORMAT_H_

#include "Base/Macros.h"

struct NativePf5650
{
	u16	Bits;

	static u16 Make( u8 r, u8 g, u8 b, u8 a )
	{
		// Alpha is discarded
		DAEDALUS_USE( a );

		return ((r >> (8-BitsR)) << ShiftR) |
			   ((g >> (8-BitsG)) << ShiftG) |
			   ((b >> (8-BitsB)) << ShiftB);
	}


	NativePf5650()
	{
	}

	NativePf5650( u8 r, u8 g, u8 b, u8 a )
		:	Bits( Make( r,g,b,a ) )
	{
	}

	u8	GetR() const { u8 r( u8( ( Bits & MaskR ) >> ShiftR ) ); return (r << (8 - BitsR)) | (r >> (BitsR - (8 - BitsR))); }
	u8	GetG() const { u8 g( u8( ( Bits & MaskG ) >> ShiftG ) ); return (g << (8 - BitsG)) | (g >> (BitsG - (8 - BitsG))); }
	u8	GetB() const { u8 b( u8( ( Bits & MaskB ) >> ShiftB ) ); return (b << (8 - BitsB)) | (b >> (BitsB - (8 - BitsB))); }
	u8	GetA() const { return 255; }

	static const u32	MaskR = 0x001f;
	static const u32	MaskG = 0x07e0;
	static const u32	MaskB = 0xf800;
	//static const u32	MaskA = 0x0000;

	static const u32	ShiftR = 0;
	static const u32	ShiftG = 5;
	static const u32	ShiftB = 11;
	//static const u32	ShiftA = 16;

	static const u32	BitsR = 5;
	static const u32	BitsG = 6;
	static const u32	BitsB = 5;
	//static const u32	BitsA = 0;
};
DAEDALUS_STATIC_ASSERT( sizeof( NativePf5650 ) == 2 );

struct NativePf5551
{
	u16	Bits;

	static u16 Make( u8 r, u8 g, u8 b, u8 a )
	{
		return ((r >> (8-BitsR)) << ShiftR) |
			   ((g >> (8-BitsG)) << ShiftG) |
			   ((b >> (8-BitsB)) << ShiftB) |
			   ((a >> (8-BitsA)) << ShiftA);		// Or could do 'a ? MaskA : 0'
	}


	NativePf5551()
	{
	}

	NativePf5551( u8 r, u8 g, u8 b, u8 a )
		:	Bits( Make( r,g,b,a ) )
	{
	}

	u8	GetR() const { u8 r( u8( ( Bits & MaskR ) >> ShiftR ) ); return (r << (8 - BitsR)) | (r >> (BitsR - (8 - BitsR))); }
	u8	GetG() const { u8 g( u8( ( Bits & MaskG ) >> ShiftG ) ); return (g << (8 - BitsG)) | (g >> (BitsG - (8 - BitsG))); }
	u8	GetB() const { u8 b( u8( ( Bits & MaskB ) >> ShiftB ) ); return (b << (8 - BitsB)) | (b >> (BitsB - (8 - BitsB))); }
	u8	GetA() const { return (Bits & MaskA) ? 255 : 0; }

	static const u32	MaskR = 0x001f;
	static const u32	MaskG = 0x03e0;
	static const u32	MaskB = 0x7c00;
	static const u32	MaskA = 0x8000;

	static const u32	ShiftR = 0;
	static const u32	ShiftG = 5;
	static const u32	ShiftB = 10;
	static const u32	ShiftA = 15;

	static const u32	BitsR = 5;
	static const u32	BitsG = 5;
	static const u32	BitsB = 5;
	static const u32	BitsA = 1;
};
DAEDALUS_STATIC_ASSERT( sizeof( NativePf5551 ) == 2 );

struct NativePf4444
{
	u16	Bits;

	static u16 Make( u8 r, u8 g, u8 b, u8 a )
	{
		return ((r >> (8-BitsR)) << ShiftR) |
			   ((g >> (8-BitsG)) << ShiftG) |
			   ((b >> (8-BitsB)) << ShiftB) |
			   ((a >> (8-BitsA)) << ShiftA);
	}

	NativePf4444()
	{
	}

	NativePf4444( u8 r, u8 g, u8 b, u8 a )
		:	Bits( Make( r,g,b,a ) )
	{
	}

	u8	GetR() const { u8 r( u8( ( Bits & MaskR ) >> ShiftR ) ); return (r << (8 - BitsR)) | (r >> (BitsR - (8 - BitsR))); }
	u8	GetG() const { u8 g( u8( ( Bits & MaskG ) >> ShiftG ) ); return (g << (8 - BitsG)) | (g >> (BitsG - (8 - BitsG))); }
	u8	GetB() const { u8 b( u8( ( Bits & MaskB ) >> ShiftB ) ); return (b << (8 - BitsB)) | (b >> (BitsB - (8 - BitsB))); }
	u8	GetA() const { u8 a( u8( ( Bits & MaskA ) >> ShiftA ) ); return (a << (8 - BitsA)) | (a >> (BitsA - (8 - BitsA))); }

	static const u32	MaskR = 0x000f;
	static const u32	MaskG = 0x00f0;
	static const u32	MaskB = 0x0f00;
	static const u32	MaskA = 0xf000;

	static const u32	ShiftR = 0;
	static const u32	ShiftG = 4;
	static const u32	ShiftB = 8;
	static const u32	ShiftA = 12;

	static const u32	BitsR = 4;
	static const u32	BitsG = 4;
	static const u32	BitsB = 4;
	static const u32	BitsA = 4;
};
DAEDALUS_STATIC_ASSERT( sizeof( NativePf4444 ) == 2 );

struct NativePf8888
{
	union
	{
		struct
		{
			u8		R;
			u8		G;
			u8		B;
			u8		A;
		};
		u32			Bits;
	};

	static u32 Make( u8 r, u8 g, u8 b, u8 a )
	{
		return (r << ShiftR) |
			   (g << ShiftG) |
			   (b << ShiftB) |
			   (a << ShiftA);
	}

	template< typename T >
	static NativePf8888 Make( T c )
	{
		return NativePf8888( c.GetR(), c.GetG(), c.GetB(), c.GetA() );
	}

	NativePf8888()
	{
	}

	// Would like to remove this
	explicit NativePf8888( u32 bits )
		:	Bits( bits )
	{
	}

	NativePf8888( u8 r, u8 g, u8 b, u8 a )
		:	Bits( Make( r,g,b,a ) )
	{
	}

	u8	GetR() const { return R; }
	u8	GetG() const { return G; }
	u8	GetB() const { return B; }
	u8	GetA() const { return A; }

	static const u32	MaskR = 0x000000ff;
	static const u32	MaskG = 0x0000ff00;
	static const u32	MaskB = 0x00ff0000;
	static const u32	MaskA = 0xff000000;

	static const u32	ShiftR = 0;
	static const u32	ShiftG = 8;
	static const u32	ShiftB = 16;
	static const u32	ShiftA = 24;

	static const u32	BitsR = 8;
	static const u32	BitsG = 8;
	static const u32	BitsB = 8;
	static const u32	BitsA = 8;
};
DAEDALUS_STATIC_ASSERT( sizeof( NativePf8888 ) == 4 );

struct NativePfCI44		// This represents 2 pixels
{
	u8	Bits;

	NativePfCI44()
	{
	}

	explicit NativePfCI44( u8 bits )
		:	Bits( bits )
	{
	}

	inline u8 GetIdxA() const		{ return (Bits >> 4) & 0xf; }
	inline u8 GetIdxB() const		{ return (Bits     ) & 0xf; }

	inline void SetIdxA(u8 value) 	{ Bits &= MaskPixelA; Bits |= (value << 4 )& MaskPixelB; }
	inline void SetIdxB(u8 value) 	{ Bits &= MaskPixelB; Bits |= value & MaskPixelA; }

	static const u8	MaskPixelA = 0x0f;
	static const u8	MaskPixelB = 0xf0;

};
DAEDALUS_STATIC_ASSERT( sizeof( NativePfCI44 ) == 1 );

struct NativePfCI8
{
	u8	Bits;

	NativePfCI8()
	{
	}

	explicit NativePfCI8( u8 bits )
		:	Bits( bits )
	{
	}
};
DAEDALUS_STATIC_ASSERT( sizeof( NativePfCI8 ) == 1 );

#endif // GRAPHICS_NATIVEPIXELFORMAT_H_
