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

#ifndef __CONVERTIMAGE_H__
#define __CONVERTIMAGE_H__

#define RGBA_GETALPHA(rgb)      ((rgb) >> 24)
#define RGBA_GETRED(rgb)        (((rgb) >> 16) & 0xff)
#define RGBA_GETGREEN(rgb)      (((rgb) >> 8) & 0xff)
#define RGBA_GETBLUE(rgb)       ((rgb) & 0xff)
#define RGBA_MAKE(r, g, b, a)   ((u32) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))

static const u8 OneToEight[2] =
{
	0x00,		// 0 -> 00 00 00 00
	0xff		// 1 -> 11 11 11 11
};

static const u8 OneToFour[2] =
{
	0x00,		// 0 -> 00 00
	0x0f		// 1 -> 11 11
};

static const u8 TwoToEight[4] =
{
	0x00,		// 00 -> 00 00 00 00
	0x55,		// 01 -> 01 01 01 01
	0xaa,		// 10 -> 10 10 10 10
	0xff		// 11 -> 11 11 11 11
};

static const u8 TwoToFour[4] =
{
	0x0,		// 00 -> 00 00
	0x5,		// 01 -> 01 01
	0xa,		// 10 -> 10 10
	0xf			// 11 -> 11 11
};

static const u8 ThreeToEight[8] =
{
	0x00,		// 000 -> 00 00 00 00
	0x24,		// 001 -> 00 10 01 00
	0x49,		// 010 -> 01 00 10 01
	0x6d,       // 011 -> 01 10 11 01
	0x92,       // 100 -> 10 01 00 10
	0xb6,		// 101 -> 10 11 01 10
	0xdb,		// 110 -> 11 01 10 11
	0xff		// 111 -> 11 11 11 11
};

static const u8 FourToEight[16] =
{
	0x00, 0x11, 0x22, 0x33,
	0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xaa, 0xbb,
	0xcc, 0xdd, 0xee, 0xff
};

static const u8 FiveToEight[32] =
{
	0x00, // 00000 -> 00000000
	0x08, // 00001 -> 00001000
	0x10, // 00010 -> 00010000
	0x18, // 00011 -> 00011000
	0x21, // 00100 -> 00100001
	0x29, // 00101 -> 00101001
	0x31, // 00110 -> 00110001
	0x39, // 00111 -> 00111001
	0x42, // 01000 -> 01000010
	0x4a, // 01001 -> 01001010
	0x52, // 01010 -> 01010010
	0x5a, // 01011 -> 01011010
	0x63, // 01100 -> 01100011
	0x6b, // 01101 -> 01101011
	0x73, // 01110 -> 01110011
	0x7b, // 01111 -> 01111011
	
	0x84, // 10000 -> 10000100
	0x8c, // 10001 -> 10001100
	0x94, // 10010 -> 10010100
	0x9c, // 10011 -> 10011100
	0xa5, // 10100 -> 10100101
	0xad, // 10101 -> 10101101
	0xb5, // 10110 -> 10110101
	0xbd, // 10111 -> 10111101
	0xc6, // 11000 -> 11000110
	0xce, // 11001 -> 11001110
	0xd6, // 11010 -> 11010110
	0xde, // 11011 -> 11011110
	0xe7, // 11100 -> 11100111
	0xef, // 11101 -> 11101111
	0xf7, // 11110 -> 11110111
	0xff  // 11111 -> 11111111
};


#define RGBA5551_RedMask   (0xF800)
#define RGBA5551_GreenMask (0x07C0)
#define RGBA5551_BlueMask  (0x003E)
#define RGBA5551_AlphaMask (0x0001)


#define RGBA5551_RedShift	11
#define RGBA5551_GreenShift	6
#define RGBA5551_BlueShift	1
#define RGBA5551_AlphaShift	0

#define RGBA565_RedMask		(0xF800)
#define RGBA565_GreenMask	(0x07E0)
#define RGBA565_BlueMask	(0x001F)

#define RGBA565_RedShift	11
#define RGBA565_GreenShift	5
#define RGBA565_BlueShift	0

#define R4G4B4A4_MAKE(r,g,b,a) 	(((a) << 12) | ((r)<< 8) | ((g)<<4) | (b))


inline WORD Convert555ToR4G4B4A4(WORD w555)
{
	u8 dwRed   = ((w555&RGBA5551_RedMask)  >> RGBA5551_RedShift)>>1;
	u8 dwGreen = ((w555&RGBA5551_GreenMask)>> RGBA5551_GreenShift)>>1;
	u8 dwBlue  = ((w555&RGBA5551_BlueMask) >> RGBA5551_BlueShift)>>1;
	u8 dwAlpha =             (w555&RGBA5551_AlphaMask) ? 0xF : 0x0;

	return R4G4B4A4_MAKE(dwRed, dwGreen, dwBlue, dwAlpha);

}

inline WORD ConvertIA16ToR4G4B4A4(WORD wIA)
{
	u32 dwIntensity = (wIA >> 12) & 0x0F;
	u32 dwAlpha     = (wIA >> 4) & 0x0F;

	return R4G4B4A4_MAKE(dwIntensity, dwIntensity, dwIntensity, dwAlpha);
}

inline u32 Convert555ToRGBA(WORD w555)
{
	u32 dwRed   = FiveToEight[(w555&RGBA5551_RedMask)  >> RGBA5551_RedShift];
	u32 dwGreen = FiveToEight[(w555&RGBA5551_GreenMask)>> RGBA5551_GreenShift];
	u32 dwBlue  = FiveToEight[(w555&RGBA5551_BlueMask) >> RGBA5551_BlueShift];
	u32 dwAlpha =             (w555&RGBA5551_AlphaMask) ? 0xFF : 0x00;
	return RGBA_MAKE(dwRed, dwGreen, dwBlue, dwAlpha);

}

// 16 a4r4g4b4
void ConvertRGBA16_16(CTexture *pSurf, const TextureInfo & ti);
void ConvertRGBA32_16(CTexture *pSurf, const TextureInfo & ti);


void ConvertIA4_16(CTexture *pSurf, const TextureInfo & ti);
void ConvertIA8_16(CTexture *pSurf, const TextureInfo & ti);
void ConvertIA16_16(CTexture *pSurf, const TextureInfo & ti);

void ConvertI4_16(CTexture *pSurf, const TextureInfo & ti);
void ConvertI8_16(CTexture *pSurf, const TextureInfo & ti);

void ConvertCI4_RGBA16_16(CTexture *pSurf, const TextureInfo & ti);
void ConvertCI4_IA16_16(CTexture *pSurf, const TextureInfo & ti);
void ConvertCI8_RGBA16_16(CTexture *pSurf, const TextureInfo & ti);
void ConvertCI8_IA16_16(CTexture *pSurf, const TextureInfo & ti);

#endif
