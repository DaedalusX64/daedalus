/*

Copyright (C) 2009 Raphael

E-mail:   raphael@fx-world.org
homepage: http://wordpress.fx-world.org

*/

#include "stdafx.h"
#include "FastMemcpy.h"

//*****************************************************************************
//Copy native N64 memory with CPU only //Corn
//Little Endian
//*****************************************************************************
void memcpy_swizzle( void* dst, const void* src, size_t size )
{
	u8* src8 = (u8*)src;
	u8* dst8 = (u8*)dst;
	u32* src32;
	u32* dst32;

	// < 4 isn't worth trying any optimisations...
	if (size<4) goto bytecopy;

	// Align dst on 4 bytes or just resume if already done
	while (((((uintptr_t)dst8) & 0x3)!=0) )
	{
		*(u8*)((uintptr_t)dst8++ ^ U8_TWIDDLE) = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
		size--;
	}

	// We are dst aligned now and >= 4 bytes to copy
	src32 = (u32*)src8;
	dst32 = (u32*)dst8;
	if( (((uintptr_t)src8)&0x3) == 0 ) 
	{
		//Both src and dst are aligned to 4 bytes at this point

		while (size&0xC)
		{
			*dst32++ = *src32++;
			size -= 4;
		}

		while (size>=16)
		{
			*dst32++ = *src32++;
			*dst32++ = *src32++;
			*dst32++ = *src32++;
			*dst32++ = *src32++;
			size -= 16;
		}

		src8 = (u8*)src32;
		dst8 = (u8*)dst32;
	}
	else
	{
		//At least dst is aligned at this point
		dst32 = (u32*)dst8;

		//src is word aligned
		if( (((uintptr_t)src8&0x1) == 0) )
		{
			u16 *src16 = (u16 *)src8;
			while (size>=4)
			{
				u32 a = *(u16*)((uintptr_t)src16++ ^ U16_TWIDDLE);
				u32 b = *(u16*)((uintptr_t)src16++ ^ U16_TWIDDLE);

				*dst32++ = ((a << 16) | b);
				size -= 4;
			}
			src8 = (u8*)src16;
		}
		else
		{
			//We are src unligned..
			while (size>=4)
			{
				u32 a = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
				u32 b = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
				u32 c = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
				u32 d = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);

				*dst32++ = (a << 24) | (b << 16) | (c << 8) | d;
				size -= 4;
			}
		}
		dst8 = (u8*)dst32;
	}

bytecopy:
	// Copy the remains byte per byte...
	while (size--)
	{
		*(u8*)((uintptr_t)dst8++ ^ U8_TWIDDLE) = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
	}
}
