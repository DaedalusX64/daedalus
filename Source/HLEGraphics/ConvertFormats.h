/*
Copyright (C) 2020 StrmnNrmn

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

#ifndef HLEGRAPHICS_CONVERTFORMATS_H_
#define HLEGRAPHICS_CONVERTFORMATS_H_

// convert rgba values (0-255 per channel) to a dword in A8R8G8B8 order..
#define CONVERT_RGBA(r,g,b,a)  (a<<24) | (b<<16) | (g<<8) | r

#include <array>

static std::array<const u8, 2> OneToEight = {
// static const u8 OneToEight[] = {
	0x00,   // 0 -> 00 00 00 00
	0xff    // 1 -> 11 11 11 11
};

// static const u8 ThreeToEight[8] =
static std::array<const u8, 8> ThreeToEight = {
	0x00,	// 000 -> 00 00 00 00
	0x24,	// 001 -> 00 10 01 00
	0x49,	// 010 -> 01 00 10 01
	0x6d,	// 011 -> 01 10 11 01
	0x92,	// 100 -> 10 01 00 10
	0xb6,	// 101 -> 10 11 01 10
	0xdb,	// 110 -> 11 01 10 11
	0xff	// 111 -> 11 11 11 11
};

// static const u8 FourToEight[16] =
static std::array< const u8, 16> FourToEight = {
	0x00, 0x11, 0x22, 0x33,
	0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xaa, 0xbb,
	0xcc, 0xdd, 0xee, 0xff
};

// static const u8 FiveToEight[] = {
static std::array<const u8, 32> FiveToEight = {
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

 static inline u32 RGBA32(u8 a, u8 b, u8 g, u8 r)
 {
	 return CONVERT_RGBA(r, g, b, a);
 }

static inline u32 RGBA16(u16 v)
{
	u32 r = FiveToEight[(v>>11)&0x1f];
	u32 g = FiveToEight[(v>> 6)&0x1f];
	u32 b = FiveToEight[(v>> 1)&0x1f];
	u32 a = ((v     )&0x01)? 255 : 0;
	return CONVERT_RGBA(r, g, b, a);
}

static inline u32 IA16(u16 v)
{
	u32 i = (v>>8)&0xff;
	u32 a = (v   )&0xff;
	return CONVERT_RGBA(i, i, i, a);
}

static inline u32 I4(u8 v)
{
	u32 i = FourToEight[v & 0x0f];
	return CONVERT_RGBA(i, i, i, i);
}

static inline u32 IA4(u8 v)
{
	u32 i = ThreeToEight[(v & 0x0f) >> 1];
	u32 a = OneToEight[(v & 0x01)];
	return CONVERT_RGBA(i, i, i, a);
}

static inline u32 YUV16(s32 Y, s32 U, s32 V)
{
    s32 r = s32(Y + (1.370705f * (V-128)));
    s32 g = s32(Y - (0.698001f * (V-128)) - (0.337633f * (U-128)));
    s32 b = s32(Y + (1.732446f * (U-128)));

    r = r < 0 ? 0 : (r > 255 ? 255 : r);
    g = g < 0 ? 0 : (g > 255 ? 255 : g);
    b = b < 0 ? 0 : (b > 255 ? 255 : b);
    return CONVERT_RGBA(r, g, b, 255);
}

static inline u16 YUVtoRGBA(u8 y, u8 u, u8 v)
{
	f32 r = y + (1.370705f * (v-128));
	f32 g = y - (0.698001f * (v-128)) - (0.337633f * (u-128));
	f32 b = y + (1.732446f * (u-128));
	r *= 0.125f;
	g *= 0.125f;
	b *= 0.125f;

	//clipping the result
	r = r < 0 ? 0 : (r > 32 ? 32 : r);
    g = g < 0 ? 0 : (g > 32 ? 32 : g);
    b = b < 0 ? 0 : (b > 32 ? 32 : b);

	return (u16)(((u16)(r) << 11) |((u16)(g) << 6) |((u16)(b) << 1) | 1);
}

#endif // HLEGRAPHICS_CONVERTFORMATS_H_
