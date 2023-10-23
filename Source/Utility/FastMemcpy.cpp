/*
Copyright (C) 2012 Corn

aligned case & byte copy (except ASM) by
Copyright (C) 2009 Raphael
E-mail:   raphael@fx-world.org
homepage: http://wordpress.fx-world.org
*/


#include "Base/Types.h"


#include "Base/Types.h"
#include "System/Endian.h"
#include "Utility/FastMemcpy.h"
#include "System/Timing.h"

#include <cstring>
#include <stdio.h>

//*****************************************************************************
//Copy native N64 memory with CPU only //Corn
//Little Endian
//*****************************************************************************
void memcpy_byteswap( void* dst, const void* src, size_t size )
{
	u8* src8 = (u8*)src;
	u8* dst8 = (u8*)dst;

	// < 4 isn't worth trying any optimisations...
	if(size>=4)
	{
		// Align dst on 4 bytes or just resume if already done
		while (((((uintptr_t)dst8) & 0x3)!=0) )
		{
			*(u8*)((uintptr_t)dst8++ ^ U8_TWIDDLE) = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
			size--;
		}
		
		// We are dst aligned now but need at least 4 bytes to copy
		if(size>=4)
		{
			u32 src_alignment = (uintptr_t)src8&0x3;
			if (src_alignment == 0)		// We are now both dst and src aligned and >= 4 bytes to copy 
			{
#if defined(DAEDALUS_POSIX) || defined(DAEDALUS_W32)	
				u32 size_aligned = (size & ~0x3);
				
				// memcpy is almost 50% faster for windows and linux
				memcpy(dst8, src8, size_aligned);
				src8 += size_aligned;
				dst8 += size_aligned;
#else
				//This is faster than the PSP's GCC memcpy
				//TODO: Profile for other plaforms to see if memcpy is faster
				u32* src32 = (u32*)src8;
				u32* dst32 = (u32*)dst8;

				u32 size32 = size >> 2;
				while (size32 & 0x3)
				{
					*dst32++ = *src32++;
					size32--;
				}

				u32 size128 = size32 >> 2;
				while (size128--)
				{
					dst32[0] = src32[0];
					dst32[1] = src32[1];
					dst32[2] = src32[2];
					dst32[3] = src32[3];
					src32 += 4;
					dst32 += 4;
				}
				src8 = (u8*)src32;
				dst8 = (u8*)dst32;
#endif
			}
			else	// We are now dst aligned and src unligned and >= 4 bytes to copy
			{
				u32* src32 = (u32*)((uintptr_t)src8 & ~0x3);
				u32* dst32 = (u32*)dst8;
				u32 srcTmp = *src32++;
				u32 dstTmp = 0;
				u32 size32 = size >> 2;
	
				switch( src_alignment )
				{
				case 1:
					while(size32--)
					{
						dstTmp = srcTmp << 8;
						srcTmp = *src32++;
						*dst32++ = dstTmp | (srcTmp >> 24);
					}
					break;
				case 2:
					while(size32--)
					{
						dstTmp = srcTmp << 16;
						srcTmp = *src32++;
						*dst32++ = dstTmp | (srcTmp >> 16);
					}
					break;
				case 3:
					while(size32--)
					{
						dstTmp = srcTmp << 24;
						srcTmp = *src32++;
						*dst32++ = dstTmp | (srcTmp >> 8);
						
					}
					break;
				}
				src8 = (u8*)src32 - (4-src_alignment);
				dst8 = (u8*)dst32;
			}
		}
	}

	// Copy any remaining byte by byte...
	size &= 0x03;
	while(size--)
	{
		*(u8*)((uintptr_t)dst8++ ^ U8_TWIDDLE) = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
	}
}

#ifdef PROFILE_MEMCPY
void byteswap_copy( void* dst, const void* src, size_t size )
{
	u8* src8 = (u8*)src;
	u8* dst8 = (u8*)dst;

	while(size--)
	{
		*(u8*)((uintptr_t)dst8++ ^ U8_TWIDDLE) = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
	}
}

static inline u64 GetCurrent()
{
    u64 tick;
	NTiming::GetPreciseTime(&tick);
    return tick;
}

#define MEMCPY_TEST(d, s, n) {													\
	u32 _fast_memcpy_swizzle = 0;												\
	{																			\
		u64 time;																\
		NTiming::GetPreciseTime(&time);											\
		for (u32 j=0; j<10000; ++j)												\
			memcpy_byteswap(d, s, n);											\
		_fast_memcpy_swizzle = (u32)(GetCurrent()-time);						\
	}																			\
	u32 _copy_byteswap = 0;														\
	{																			\
		u64 time = GetCurrent();												\
		for (u32 j=0; j<10000; ++j)												\
			byteswap_copy(d, s, n);												\
		_copy_byteswap = (u32)(GetCurrent()-time);								\
	}																			\
	printf("%ld bytes | BYTESWAP COPY %d | MEMCPY SWIZZLE %d\n", n, _copy_byteswap, _fast_memcpy_swizzle); \
	}

void memcpy_test( void * dst, const void * src, size_t size )
{
	MEMCPY_TEST(dst, src, size);
}
#endif // PROFILE_MEMCPY