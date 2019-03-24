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

#ifndef HLEGRAPHICS_N64PIXELFORMAT_H_
#define HLEGRAPHICS_N64PIXELFORMAT_H_

template< typename PixelFormatA, typename PixelFormatB >
inline PixelFormatA ConvertPixelFormat( PixelFormatB colour )
{
	return PixelFormatA( colour.GetR(), colour.GetG(), colour.GetB(), colour.GetA() );
}

struct N64Pf5551
{
	u16	Bits;

	static u16 Make( u8 r, u8 g, u8 b, u8 a )
	{
		return ((r >> (8-BitsR)) << ShiftR) |
			   ((g >> (8-BitsG)) << ShiftG) |
			   ((b >> (8-BitsB)) << ShiftB) |
			   ((a >> (8-BitsA)) << ShiftA);		// Or could do 'a ? MaskA : 0'
	}

	N64Pf5551()
	{
	}

	// Would like to remove this
	explicit N64Pf5551( u16 bits )
		:	Bits( bits )
	{
	}

	N64Pf5551( u8 r, u8 g, u8 b, u8 a )
		:	Bits( Make( r,g,b,a ) )
	{
	}

	u8	GetR() const { u8 r( u8( ( Bits & MaskR ) >> ShiftR ) ); return (r << (8 - BitsR)) | (r >> (BitsR - (8 - BitsR))); }
	u8	GetG() const { u8 g( u8( ( Bits & MaskG ) >> ShiftG ) ); return (g << (8 - BitsG)) | (g >> (BitsG - (8 - BitsG))); }
	u8	GetB() const { u8 b( u8( ( Bits & MaskB ) >> ShiftB ) ); return (b << (8 - BitsB)) | (b >> (BitsB - (8 - BitsB))); }
	u8	GetA() const { return (Bits & MaskA) ? 255 : 0; }

	static const u32	MaskR {0xf800};
	static const u32	MaskG {0x07c0};
	static const u32	MaskB {0x003e};
	static const u32	MaskA {0x0001};

	static const u32	ShiftR {11};
	static const u32	ShiftG {6};
	static const u32	ShiftB {1};
	static const u32	ShiftA {};

	static const u32	BitsR {5};
	static const u32	BitsG {5};
	static const u32	BitsB {5};
	static const u32	BitsA {1};
};
DAEDALUS_STATIC_ASSERT( sizeof( N64Pf5551 ) == 2 );

struct N64Pf8888
{
	union
	{
		struct
		{
			u8				A;
			u8				B;
			u8				G;
			u8				R;
		};
		u32					Bits;
	};

	static u32 Make( u8 r, u8 g, u8 b, u8 a )
	{
		return (r << ShiftR) |
			   (g << ShiftG) |
			   (b << ShiftB) |
			   (a << ShiftA);
	}

	template< typename T >
	static N64Pf8888 Make( T c )
	{
		return N64Pf8888( c.GetR(), c.GetG(), c.GetB(), c.GetA() );
	}

	N64Pf8888()
	{
	}

	explicit N64Pf8888( u32 bits )
		:	Bits( bits )
	{
	}

	N64Pf8888( u8 r, u8 g, u8 b, u8 a )
		:	Bits( Make( r,g,b,a ) )
	{
	}

	u8	GetR() const { return R; }
	u8	GetG() const { return G; }
	u8	GetB() const { return B; }
	u8	GetA() const { return A; }

	static const u32	MaskR {0xff000000};
	static const u32	MaskG {0x00ff0000};
	static const u32	MaskB {0x0000ff00};
	static const u32	MaskA {0x000000ff};

	static const u32	ShiftR {24};
	static const u32	ShiftG {16};
	static const u32	ShiftB {8};
	static const u32	ShiftA {0};

	static const u32	BitsR {8};
	static const u32	BitsG {8};
	static const u32	BitsB {8};
	static const u32	BitsA {8};
};
DAEDALUS_STATIC_ASSERT( sizeof( N64Pf8888 ) == 4 );

struct N64PfIA8
{
	u8	Bits;

	N64PfIA8()
	{
	}

	u8	GetR() const { return GetI(); }
	u8	GetG() const { return GetI(); }
	u8	GetB() const { return GetI(); }

	u8	GetI() const { u8 i( ( Bits & MaskI ) >> ShiftI ); return (i << (8 - BitsI)) | (i >> (BitsI - (8 - BitsI))); }
	u8	GetA() const { u8 a( ( Bits & MaskA ) >> ShiftA ); return (a << (8 - BitsA)) | (a >> (BitsA - (8 - BitsA))); }

	static const u32	MaskI {0xf0};
	static const u32	MaskA {0x0f};

	static const u32	ShiftI {4};
	static const u32	ShiftA {};

	static const u32	BitsI {4};
	static const u32	BitsA {4};
};
DAEDALUS_STATIC_ASSERT( sizeof( N64PfIA8 ) == 1 );

struct N64PfIA16
{
	union
	{
		struct
		{
			u8		A;
			u8		I;
		};
		u16			Bits;
	};

	N64PfIA16()
	{
	}

	u8	GetR() const { return I; }
	u8	GetG() const { return I; }
	u8	GetB() const { return I; }
	u8	GetA() const { return A; }

	static const u32	MaskI {0xff00};
	static const u32	MaskA {0x00ff};

	static const u32	ShiftI {8};
	static const u32	ShiftA {};

	static const u32	BitsI {8};
	static const u32	BitsA {8};
};
DAEDALUS_STATIC_ASSERT( sizeof( N64PfIA16 ) == 2 );

struct N64PfI8
{
	union
	{
		struct
		{
			u8		I;
		};
		u8			Bits;
	};

	N64PfI8()
	{
	}

	u8	GetR() const { return I; }
	u8	GetG() const { return I; }
	u8	GetB() const { return I; }
	u8	GetA() const { return I; }		// Always I and not 255?

	static const u32	MaskI {0xff};

	static const u32	ShiftI {};

	static const u32	BitsI {};
};
DAEDALUS_STATIC_ASSERT( sizeof( N64PfI8 ) == 1 );

#endif // HLEGRAPHICS_N64PIXELFORMAT_H_
