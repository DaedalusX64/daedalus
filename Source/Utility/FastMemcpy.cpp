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
		//At least dst is aligned
		register u32 tmp;
		while (size>=4)
		{
			tmp = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
			tmp = (tmp << 8) | *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
			tmp = (tmp << 8) | *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
			*dst32++ = (tmp << 8) | *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
			size -= 4;
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
