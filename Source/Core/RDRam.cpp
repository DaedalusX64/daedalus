/**
* DaedalusX64 - RDRam.cpp
* Copyright (C) 2020 Rinnegatamante                                     *
*
* If you want to contribute to the project please contact
* me first (maybe someone is already making what you are
* planning to do).
*
*
* This program is free software; you can redistribute it and/
* or modify it under the terms of the GNU General Public Li-
* cence as published by the Free Software Foundation; either
* version 2 of the Licence, or any later version.
*
* This program is distributed in the hope that it will be use-
* ful, but WITHOUT ANY WARRANTY; without even the implied war-
* ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public Licence for more details.
*
* You should have received a copy of the GNU General Public
* Licence along with this program; if not, write to the Free
* Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
* USA.
*
**/

#include "stdafx.h"

#include <stdlib.h>
#include <string.h>

#include "Debug/DBGConsole.h"
#include "Memory.h"
#include "RDRam.h"
#include "OSHLE/ultra_sptask.h"

/* FIXME: assume presence of expansion pack */
#define MEMMASK 0x7FFFFF

u8 clamp_u8(s16 x)
{
    return (x & (0xff00)) ? ((-x) >> 15) & 0xff : x;
}

s16 clamp_s12(s16 x)
{
    if (x < -0x800) { x = -0x800; } else if (x > 0x7f0) { x = 0x7f0; }
    return x;
}

s16 clamp_s16(s32 x)
{
    if (x > 32767) { x = 32767; } else if (x < -32768) { x = -32768; }
    return x;
}

u16 clamp_RGBA_component(s16 x)
{
    if (x > 0xff0) { x = 0xff0; } else if (x < 0) { x = 0; }
    return (x & 0xf80);
}

//ToDo: fast_memcpy_swizzle?
void rdram_read_many_u8(u8 *dst, u32 address, u32 count)
{
	const u8 *src = g_pu8RamBase + (address & MEMMASK);

    while (count != 0)
    {
		u32 a = *(u8*)((uintptr_t)src++ ^ U8_TWIDDLE);

		*(dst++) = a;
		--count;
    }
}

void rdram_read_many_u16(u16 *dst, u32 address, u32 count)
{
	const u8 *src = g_pu8RamBase + (address & MEMMASK);

    while (count != 0)
    {
		u32 a = *(u8*)((uintptr_t)src++ ^ U8_TWIDDLE);
		u32 b = *(u8*)((uintptr_t)src++ ^ U8_TWIDDLE);

		*(dst++) = ((a << 8) | b);
		--count;
    }
}

void rdram_write_many_u16(const u16 *src, u32 address, u32 count)
{
	u8 *dst = g_pu8RamBase + (address & MEMMASK);
    while (count != 0)
    {
       *(u8*)((uintptr_t)dst++ ^ U8_TWIDDLE) = (u8)(*src >> 8);
       *(u8*)((uintptr_t)dst++ ^ U8_TWIDDLE)= (u8)(*(src++) & 0xff);

        --count;
    }
}

void rdram_read_many_u32(u32 *dst, u32 address, u32 count)
{
	const u8 *src = g_pu8RamBase + (address & MEMMASK);

    while (count != 0)
    {
		u32 a = *(u8*)((uintptr_t)src++ ^ U8_TWIDDLE);
		u32 b = *(u8*)((uintptr_t)src++ ^ U8_TWIDDLE);
		u32 c = *(u8*)((uintptr_t)src++ ^ U8_TWIDDLE);
		u32 d = *(u8*)((uintptr_t)src++ ^ U8_TWIDDLE);

		*(dst++) = (a << 24) | (b << 16) | (c << 8) | d;
		--count;
    }
}

u32 rdram_read_u32(u32 address)
{
	const u8 *src {g_pu8RamBase + (address& MEMMASK)};

	u32 a = *(u8*)((uintptr_t)src++ ^ U8_TWIDDLE);
	u32 b = *(u8*)((uintptr_t)src++ ^ U8_TWIDDLE);
	u32 c = *(u8*)((uintptr_t)src++ ^ U8_TWIDDLE);
	u32 d = *(u8*)((uintptr_t)src++ ^ U8_TWIDDLE);

    return (a << 24) | (b << 16) | (c << 8) | d;
}

void rdram_write_many_u32(const u32 *src, u32 address, u32 count)
{
	u8 *dst = g_pu8RamBase + (address & MEMMASK);
    while (count != 0)
    {
       *(u8*)((uintptr_t)dst++ ^ U8_TWIDDLE) = (u8)(*src >> 24);
       *(u8*)((uintptr_t)dst++ ^ U8_TWIDDLE) = (u8)(*src >> 16);
       *(u8*)((uintptr_t)dst++ ^ U8_TWIDDLE) = (u8)(*src >> 8);
       *(u8*)((uintptr_t)dst++ ^ U8_TWIDDLE) = (u8)(*(src++) & 0xff);

        --count;
    }
}
