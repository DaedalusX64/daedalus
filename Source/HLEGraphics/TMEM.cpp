/*
Copyright (C) 2006,2007 StrmnNrmn
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


#include "Base/Types.h"

#include "HLEGraphics/TMEM.h"
#include "Debug/DBGConsole.h"
#include "System/Endian.h"

#ifdef DAEDALUS_ACCURATE_TMEM

// CopyLineQwords** passes in an offset in 32bit words, (i.e. does mem_address/4)
// So it's aligned by definition, we have asserts in place just in case!

// Due to how TMEM is organized, erg the last 3 bits in the address are always "0"
// It should be safe to assume copies will always be in qwords
#define FAST_TMEM_COPY

void CopyLineQwords(void * dst, const void * src, u32 qwords)
{
#ifdef FAST_TMEM_COPY
	u32* src32 = (u32*)src;
	u32* dst32 = (u32*)dst;

	DAEDALUS_ASSERT( ((uintptr_t)src32&0x3)==0, "src is not aligned!");

	while(qwords--)
	{
		dst32[0] = BSWAP32(src32[0]);
		dst32[1] = BSWAP32(src32[1]);
		dst32 += 2;
		src32 += 2;
	}
#else
	u8* src8 = (u8*)src;
	u8* dst8 = (u8*)dst;
	u32 bytes = qwords * 8;
	while(bytes--)
	{
		*dst8++ = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
	}
#endif
}

void CopyLineQwordsSwap(void * dst, const void * src, u32 qwords)
{
#ifdef FAST_TMEM_COPY
	u32* src32 = (u32*)src;
	u32* dst32 = (u32*)dst;

	DAEDALUS_ASSERT( ((uintptr_t)src32&0x3 )==0, "src is not aligned!");

	while(qwords--)
	{
		dst32[1]  = BSWAP32(src32[0]);
		dst32[0]  = BSWAP32(src32[1]);
		dst32 += 2;
		src32 += 2;
	}
#else
	u8* src8 = (u8*)src;
	u8* dst8 = (u8*)dst;
	u32 bytes = qwords * 8;
	while(bytes--)
	{
		*(u8*)((uintptr_t)dst8++ ^ 0x4)  = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
	}
#endif
}

void CopyLineQwordsSwap32(void * dst, const void * src, u32 qwords)
{
#ifdef FAST_TMEM_COPY
	u32* src32 = (u32*)src;
	u32* dst32 = (u32*)dst;

	DAEDALUS_ASSERT( ((uintptr_t)src32&0x3 )==0, "src is not aligned!");

	u32 size128 = qwords >>1;

	while(size128--)
	{
		dst32[2]  = BSWAP32(src32[0]);
		dst32[3]  = BSWAP32(src32[1]);
		dst32[0]  = BSWAP32(src32[2]);
		dst32[1]  = BSWAP32(src32[3]);
		dst32 += 4;
		src32 += 4;
	}

	// Copy any remaining quadword
	qwords&=0x1;
	while(qwords--)
	{
		*(u32*)((uintptr_t)dst32++ ^ 0x8) = BSWAP32(src32[0]);
		*(u32*)((uintptr_t)dst32++ ^ 0x8) = BSWAP32(src32[1]);
		src32+=2;
	}
#else

	u8* src8 = (u8*)src;
	u8* dst8 = (u8*)dst;
	u32 bytes = qwords * 8;
	while(bytes--)
	{
		*(u8*)((uintptr_t)dst8++ ^ 0x8)  = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
	}
#endif
}

void CopyLine(void * dst, const void * src, u32 bytes)
{
#ifdef FAST_TMEM_COPY
	u32* src32 = (u32*)src;
	u32* dst32 = (u32*)dst;

	DAEDALUS_ASSERT((bytes&0x3)==0, "CopyLine: Remaning bytes! (%d)",bytes);

	u32 size32 = bytes >> 2;
	u32 src_alignment = (uintptr_t)src32&0x3;

	if(src_alignment == 0)
	{
		while (size32--)
		{
			*dst32++ = BSWAP32(src32[0]);
			src32++;
		}
	}
	else
	{
		// Zelda and DK64 have unaligned copies. so let's optimize 'em
		src32 = (u32*)((uintptr_t)src & ~0x3);
		u32 src_tmp = *src32++;
		u32 dst_tmp = 0;

		// calculate offset 3..1..2
		u32 offset = 4-src_alignment;
		u32 lshift = src_alignment<<3;
		u32 rshift = offset<<3;

		while(size32--)
		{
			dst_tmp = src_tmp << lshift;
			src_tmp = *src32++;
			dst_tmp|= src_tmp >> rshift;
			*dst32++ = BSWAP32(dst_tmp);
		}
		src32 -= offset;
	}
#else
	u8* src8 = (u8*)src;
	u8* dst8 = (u8*)dst;
	while(bytes--)
	{
		*dst8++ = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
	}
#endif
}

void CopyLine16(u16 * dst16, const u16 * src16, u32 words)
{
#ifdef FAST_TMEM_COPY
	DAEDALUS_ASSERT( ((uintptr_t)src16&0x1 )==0, "src is not aligned!");

	u32 dwords = words >> 1;
	while(dwords--)
	{
		dst16[0] = src16[1];
		dst16[4] = src16[0];
		dst16+=8;
		src16+=2;
	}

	// Copy any remaining word
	words&= 0x1;
#endif
	while(words--)
	{
		*dst16 = *(u16*)((uintptr_t)src16++ ^ U16_TWIDDLE);
		dst16+=4;
	}
}

void CopyLineSwap(void * dst, const void * src, u32 bytes)
{
	u32* src32 = (u32*)src;
	u32* dst32 = (u32*)dst;

#ifdef FAST_TMEM_COPY
	DAEDALUS_ASSERT((bytes&0x7)==0, "CopyLineSwap: Remaning bytes! (%d)",bytes);

	if( ((uintptr_t)src32&0x3 )==0)
	{
		u32 size64 = bytes >> 3;

		while (size64--)
		{
			dst32[0] = BSWAP32(src32[1]);
			dst32[1] = BSWAP32(src32[0]);
			dst32 += 2;
			src32 += 2;
		}
	}
	else
#endif
	{
		// Optimize me: Bomberman, Zelda, and Quest 64 have unaligned copies here
		//Console_Print("[WWarning CopyLineSwap: Performing slow copy]");

		u8* src8 = (u8*)src32;
		u8* dst8 = (u8*)dst32;
		while(bytes--)
		{
			// Alternate 32 bit words are swapped
			*(u8*)((uintptr_t)dst8++ ^ 0x4) = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
		}
	}
}

void CopyLineSwap32(void * dst, const void * src, u32 bytes)
{
	u32* src32 = (u32*)(src);
	u32* dst32 = (u32*)(dst);

#ifdef FAST_TMEM_COPY
	DAEDALUS_ASSERT((bytes&0x7)==0, "CopyLineSwap32: Remaning bytes! (%d)",bytes);

	if( ((uintptr_t)src32&0x3 )==0)
	{
		u32 size128 = bytes >> 4;

		while (size128--)
		{
			dst32[0] = BSWAP32(src32[2]);
			dst32[1] = BSWAP32(src32[3]);
			dst32[2] = BSWAP32(src32[0]);
			dst32[3] = BSWAP32(src32[1]);
			dst32 += 4;
			src32 += 4;
		}

		// Copy any remaining quadword
		bytes&=0xF;
		while(bytes--)
		{
			*(u32*)((uintptr_t)dst32++ ^ 0x8) = BSWAP32(src32[0]);
			*(u32*)((uintptr_t)dst32++ ^ 0x8) = BSWAP32(src32[1]);
			src32+=2;
		}
	}
	else
#endif
	{
		// Have yet to see game with unaligned copies here
		//Console_Print("[WWarning CopyLineSwap32: Performing slow copy]");

		u8* src8 = (u8*)src32;
		u8* dst8 = (u8*)dst32;
		while(bytes--)
		{
			// Alternate 64 bit words are swapped
			*(u8*)((uintptr_t)dst8++ ^ 0x8) = *(u8*)((uintptr_t)src8++ ^ U8_TWIDDLE);
		}
	}
}
#endif