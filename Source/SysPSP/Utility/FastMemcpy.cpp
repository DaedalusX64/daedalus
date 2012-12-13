/*

Copyright (C) 2009 Raphael

E-mail:   raphael@fx-world.org
homepage: http://wordpress.fx-world.org

*/

#include "stdafx.h"
#include "Utility/FastMemcpy.h"

#include <psprtc.h>
#include <psppower.h>

// Avoid using VFPU in our audio plugin, otherwise the ME will choke
//

//*****************************************************************************
//Uses VFPU if possible else normal memcpy() (orignal code by Alex? Modified by Corn)
//*****************************************************************************
//
void memcpy_vfpu( void* dst, const void* src, size_t size )
{
    //less than 16bytes or there is no 32bit alignment -> not worth optimizing
	if( ((u32)src&0x3) != ((u32)dst&0x3) && (size<16) )
    {
        memcpy( dst, src, size );
        return;
    }

    u8* src8 = (u8*)src;
    u8* dst8 = (u8*)dst;

	// Align dst to 4 bytes or just resume if already done
	while( ((u32)dst8&0x3)!=0 )
	{
		*dst8++ = *src8++;
		size--;
	}

	u32 *dst32=(u32*)dst8;
	u32 *src32=(u32*)src8;

	// Align dst to 16 bytes or just resume if already done
	while( ((u32)dst32&0xF)!=0 )
	{
		*dst32++ = *src32++;
		size -= 4;
	}

	dst8=(u8*)dst32;
	src8=(u8*)src32;
	
	if( ((u32)src8&0xF)==0 )	//Both src and dst are 16byte aligned
	{
		while (size>63)
		{
			asm(".set	push\n"					// save assembler option
				".set	noreorder\n"			// suppress reordering
				"lv.q c000, 0(%1)\n"
				"lv.q c010, 16(%1)\n"
				"lv.q c020, 32(%1)\n"
				"lv.q c030, 48(%1)\n"
				"sv.q c000, 0(%0)\n"
				"sv.q c010, 16(%0)\n"
				"sv.q c020, 32(%0)\n"
				"sv.q c030, 48(%0)\n"
				"addiu  %2, %2, -64\n"			//size -= 64;
				"addiu	%1, %1, 64\n"			//dst8 += 64;
				"addiu	%0, %0, 64\n"			//src8 += 64;
				".set	pop\n"					// restore assembler option
				:"+r"(dst8),"+r"(src8),"+r"(size)
				:
				:"memory"
				);
		}

		while (size>15)
		{
			asm(".set	push\n"					// save assembler option
				".set	noreorder\n"			// suppress reordering
				"lv.q c000, 0(%1)\n"
				"sv.q c000, 0(%0)\n"
				"addiu  %2, %2, -16\n"			//size -= 16;
				"addiu	%1, %1, 16\n"			//dst8 += 16;
				"addiu	%0, %0, 16\n"			//src8 += 16;
				".set	pop\n"					// restore assembler option
				:"+r"(dst8),"+r"(src8),"+r"(size)
				:
				:"memory"
				);
		}
	}
	else 	//At least src is 4byte and dst is 16byte aligned
    {
		while (size>63)
		{
			asm(".set	push\n"					// save assembler option
				".set	noreorder\n"			// suppress reordering
				"ulv.q c000, 0(%1)\n"
				"ulv.q c010, 16(%1)\n"
				"ulv.q c020, 32(%1)\n"
				"ulv.q c030, 48(%1)\n"
				"sv.q c000, 0(%0)\n"
				"sv.q c010, 16(%0)\n"
				"sv.q c020, 32(%0)\n"
				"sv.q c030, 48(%0)\n"
				"addiu  %2, %2, -64\n"			//size -= 64;
				"addiu	%1, %1, 64\n"			//dst8 += 64;
				"addiu	%0, %0, 64\n"			//src8 += 64;
				".set	pop\n"					// restore assembler option
				:"+r"(dst8),"+r"(src8),"+r"(size)
				:
				:"memory"
				);
		}

		while (size>15)
		{
			asm(".set	push\n"					// save assembler option
				".set	noreorder\n"			// suppress reordering
				"ulv.q c000, 0(%1)\n"
				"sv.q c000, 0(%0)\n"
				"addiu  %2, %2, -16\n"			//size -= 16;
				"addiu	%1, %1, 16\n"			//dst8 += 16;
				"addiu	%0, %0, 16\n"			//src8 += 16;
				".set	pop\n"					// restore assembler option
				:"+r"(dst8),"+r"(src8),"+r"(size)
				:
				:"memory"
				);
		}
    }

	// Most copies are completed with the VFPU, so fast out
	if (size == 0) 
		return;

	dst32=(u32*)dst8;
	src32=(u32*)src8;

	//Copy remaning 32bit...
	while( size>3 )
	{
		*dst32++ = *src32++;
		size -= 4;
	}

	dst8=(u8*)dst32;
	src8=(u8*)src32;

	//Copy remaning bytes if any...
	while( size>0 )
    {
        *dst8++ = *src8++;
        size--;
    }
}

//*****************************************************************************
//Original PSP version should work like standard memcpy()
//Big Endian
//*****************************************************************************
//
/*void memcpy_vfpu_BE( void* dst, const void* src, size_t size )
{
	u8* src8 = (u8*)src;
	u8* dst8 = (u8*)dst;
	u8* udst8;
	u8* dst64a;

	// < 4 isn't worth trying any optimisations...
	if (size<4) goto bytecopy;

	// < 64 means we don't gain anything from using vfpu...
	if (size<64)
	{
		// Align dst on 4 bytes or just resume if already done
		while (((((u32)dst8) & 0x3)!=0) && size) {
			*dst8++ = *src8++;
			size--;
		}
		//if (size<4) goto bytecopy;

		// We are dst aligned now and >= 4 bytes to copy
		u32* src32 = (u32*)src8;
		u32* dst32 = (u32*)dst8;
		switch(((u32)src8)&0x3)
		{
			case 0:	//Both src and dst are aligned to 4 bytes
				while (size&0xC)
				{
					*dst32++ = *src32++;
					size -= 4;
				}
				if (size==0) return;		// fast out
				while (size>=16)
				{
					*dst32++ = *src32++;
					*dst32++ = *src32++;
					*dst32++ = *src32++;
					*dst32++ = *src32++;
					size -= 16;
				}
				if (size==0) return;		// fast out
				src8 = (u8*)src32;
				dst8 = (u8*)dst32;
				break;
			default:
				register u32 a, b, c, d;
				while (size>=4)
				{
					a = *src8++;
					b = *src8++;
					c = *src8++;
					d = *src8++;
					*dst32++ = (d << 24) | (c << 16) | (b << 8) | a;
					size -= 4;
				}
				if (size==0) return;		// fast out
				dst8 = (u8*)dst32;
				break;
		}
		goto bytecopy;
	}

	// Align dst on 16 bytes to gain from vfpu aligned stores
	while ((((u32)dst8) & 0xF)!=0 )
	{
		*dst8++ = *src8++;
		size--;
	}

	// We use uncached dst to use VFPU writeback and free cpu cache for src only
	udst8 = (u8*)((u32)dst8 | 0x40000000);
	// We need the 64 byte aligned address to make sure the dcache is invalidated correctly
	dst64a = (u8*)((u32)dst8&~0x3F);
	// Invalidate the first line that matches up to the dst start
	if (size>=64)
	asm(".set	push\n"					// save assembler option
		".set	noreorder\n"			// suppress reordering
		"cache 0x1B, 0(%0)\n"
		"addiu	%0, %0, 64\n"
		"sync\n"
		".set	pop\n"
		:"+r"(dst64a));
	switch(((u32)src8&0xF))
	{
		// src aligned on 16 bytes too? nice!
		case 0:
			while (size>=64)
			{
				asm(".set	push\n"					// save assembler option
					".set	noreorder\n"			// suppress reordering
					"cache	0x1B,  0(%2)\n"			// Dcache writeback invalidate
					"lv.q	c000,  0(%1)\n"
					"lv.q	c010, 16(%1)\n"
					"lv.q	c020, 32(%1)\n"
					"lv.q	c030, 48(%1)\n"
					"sync\n"						// Wait for allegrex writeback
					"sv.q	c000,  0(%0), wb\n"
					"sv.q	c010, 16(%0), wb\n"
					"sv.q	c020, 32(%0), wb\n"
					"sv.q	c030, 48(%0), wb\n"
					// Lots of variable updates... but get hidden in sv.q latency anyway
					"addiu  %3, %3, -64\n"
					"addiu	%2, %2, 64\n"
					"addiu	%1, %1, 64\n"
					"addiu	%0, %0, 64\n"
					".set	pop\n"					// restore assembler option
					:"+r"(udst8),"+r"(src8),"+r"(dst64a),"+r"(size)
					:
					:"memory"
					);
			}
			if (size>16)
			{
				// Invalidate the last cache line where the max remaining 63 bytes are
				asm(".set	push\n"					// save assembler option
					".set	noreorder\n"			// suppress reordering
					"cache	0x1B, 0(%0)\n"
					"sync\n"
					".set	pop\n"					// restore assembler option
					::"r"(dst64a));
				while (size>=16)
				{
					asm(".set	push\n"					// save assembler option
						".set	noreorder\n"			// suppress reordering
						"lv.q	c000, 0(%1)\n"
						"sv.q	c000, 0(%0), wb\n"
						// Lots of variable updates... but get hidden in sv.q latency anyway
						"addiu	%2, %2, -16\n"
						"addiu	%1, %1, 16\n"
						"addiu	%0, %0, 16\n"
						".set	pop\n"					// restore assembler option
						:"+r"(udst8),"+r"(src8),"+r"(size)
						:
						:"memory"
						);
				}
			}
			asm(".set	push\n"					// save assembler option
				".set	noreorder\n"			// suppress reordering
				"vflush\n"						// Flush VFPU writeback cache
				".set	pop\n"					// restore assembler option
				);
			dst8 = (u8*)((u32)udst8 & ~0x40000000);
			break;
		// src is only qword unaligned but word aligned? We can at least use ulv.q
		case 4:
		case 8:
		case 12:
			while (size>=64)
			{
				asm(".set	push\n"					// save assembler option
					".set	noreorder\n"			// suppress reordering
					"cache	0x1B,  0(%2)\n"			// Dcache writeback invalidate
					"ulv.q	c000,  0(%1)\n"
					"ulv.q	c010, 16(%1)\n"
					"ulv.q	c020, 32(%1)\n"
					"ulv.q	c030, 48(%1)\n"
					"sync\n"						// Wait for allegrex writeback
					"sv.q	c000,  0(%0), wb\n"
					"sv.q	c010, 16(%0), wb\n"
					"sv.q	c020, 32(%0), wb\n"
					"sv.q	c030, 48(%0), wb\n"
					// Lots of variable updates... but get hidden in sv.q latency anyway
					"addiu  %3, %3, -64\n"
					"addiu	%2, %2, 64\n"
					"addiu	%1, %1, 64\n"
					"addiu	%0, %0, 64\n"
					".set	pop\n"					// restore assembler option
					:"+r"(udst8),"+r"(src8),"+r"(dst64a),"+r"(size)
					:
					:"memory"
					);
			}
			if (size>16)
			// Invalidate the last cache line where the max remaining 63 bytes are
			asm(".set	push\n"					// save assembler option
				".set	noreorder\n"			// suppress reordering
				"cache	0x1B, 0(%0)\n"
				"sync\n"
				".set	pop\n"					// restore assembler option
				::"r"(dst64a));
			while (size>=16)
			{
				asm(".set	push\n"					// save assembler option
					".set	noreorder\n"			// suppress reordering
					"ulv.q	c000, 0(%1)\n"
					"sv.q	c000, 0(%0), wb\n"
					// Lots of variable updates... but get hidden in sv.q latency anyway
					"addiu	%2, %2, -16\n"
					"addiu	%1, %1, 16\n"
					"addiu	%0, %0, 16\n"
					".set	pop\n"					// restore assembler option
					:"+r"(udst8),"+r"(src8),"+r"(size)
					:
					:"memory"
					);
			}
			asm(".set	push\n"					// save assembler option
				".set	noreorder\n"			// suppress reordering
				"vflush\n"						// Flush VFPU writeback cache
				".set	pop\n"					// restore assembler option
				);
			dst8 = (u8*)((u32)udst8 & ~0x40000000);
			break;
		// src not aligned? too bad... have to use unaligned reads
		default:
			while (size>=64)
			{
				asm(".set	push\n"					// save assembler option
					".set	noreorder\n"			// suppress reordering
					"cache 0x1B,  0(%2)\n"

					"lwr	 $8,  0(%1)\n"			//
					"lwl	 $8,  3(%1)\n"			// $8  = *(s + 0)
					"lwr	 $9,  4(%1)\n"			//
					"lwl	 $9,  7(%1)\n"			// $9  = *(s + 4)
					"lwr	$10,  8(%1)\n"			//
					"lwl	$10, 11(%1)\n"			// $10 = *(s + 8)
					"lwr	$11, 12(%1)\n"			//
					"lwl	$11, 15(%1)\n"			// $11 = *(s + 12)
					"mtv	 $8, s000\n"
					"mtv	 $9, s001\n"
					"mtv	$10, s002\n"
					"mtv	$11, s003\n"

					"lwr	 $8, 16(%1)\n"
					"lwl	 $8, 19(%1)\n"
					"lwr	 $9, 20(%1)\n"
					"lwl	 $9, 23(%1)\n"
					"lwr	$10, 24(%1)\n"
					"lwl	$10, 27(%1)\n"
					"lwr	$11, 28(%1)\n"
					"lwl	$11, 31(%1)\n"
					"mtv	 $8, s010\n"
					"mtv	 $9, s011\n"
					"mtv	$10, s012\n"
					"mtv	$11, s013\n"

					"lwr	 $8, 32(%1)\n"
					"lwl	 $8, 35(%1)\n"
					"lwr	 $9, 36(%1)\n"
					"lwl	 $9, 39(%1)\n"
					"lwr	$10, 40(%1)\n"
					"lwl	$10, 43(%1)\n"
					"lwr	$11, 44(%1)\n"
					"lwl	$11, 47(%1)\n"
					"mtv	 $8, s020\n"
					"mtv	 $9, s021\n"
					"mtv	$10, s022\n"
					"mtv	$11, s023\n"

					"lwr	 $8, 48(%1)\n"
					"lwl	 $8, 51(%1)\n"
					"lwr	 $9, 52(%1)\n"
					"lwl	 $9, 55(%1)\n"
					"lwr	$10, 56(%1)\n"
					"lwl	$10, 59(%1)\n"
					"lwr	$11, 60(%1)\n"
					"lwl	$11, 63(%1)\n"
					"mtv	 $8, s030\n"
					"mtv	 $9, s031\n"
					"mtv	$10, s032\n"
					"mtv	$11, s033\n"

					"sync\n"
					"sv.q 	c000,  0(%0), wb\n"
					"sv.q 	c010, 16(%0), wb\n"
					"sv.q 	c020, 32(%0), wb\n"
					"sv.q 	c030, 48(%0), wb\n"
					// Lots of variable updates... but get hidden in sv.q latency anyway
					"addiu	%3, %3, -64\n"
					"addiu	%2, %2, 64\n"
					"addiu	%1, %1, 64\n"
					"addiu	%0, %0, 64\n"
					".set	pop\n"					// restore assembler option
					:"+r"(udst8),"+r"(src8),"+r"(dst64a),"+r"(size)
					:
					:"$8","$9","$10","$11","memory"
					);
			}
			if (size>16)
			// Invalidate the last cache line where the max remaining 63 bytes are
			asm(".set	push\n"					// save assembler option
				".set	noreorder\n"			// suppress reordering
				"cache	0x1B, 0(%0)\n"
				"sync\n"
				".set	pop\n"					// restore assembler option
				::"r"(dst64a));
			while (size>=16)
			{
				asm(".set	push\n"					// save assembler option
					".set	noreorder\n"			// suppress reordering
					"lwr	 $8,  0(%1)\n"			//
					"lwl	 $8,  3(%1)\n"			// $8  = *(s + 0)
					"lwr	 $9,  4(%1)\n"			//
					"lwl	 $9,  7(%1)\n"			// $9  = *(s + 4)
					"lwr	$10,  8(%1)\n"			//
					"lwl	$10, 11(%1)\n"			// $10 = *(s + 8)
					"lwr	$11, 12(%1)\n"			//
					"lwl	$11, 15(%1)\n"			// $11 = *(s + 12)
					"mtv	 $8, s000\n"
					"mtv	 $9, s001\n"
					"mtv	$10, s002\n"
					"mtv	$11, s003\n"

					"sv.q	c000, 0(%0), wb\n"
					// Lots of variable updates... but get hidden in sv.q latency anyway
					"addiu	%2, %2, -16\n"
					"addiu	%1, %1, 16\n"
					"addiu	%0, %0, 16\n"
					".set	pop\n"					// restore assembler option
					:"+r"(udst8),"+r"(src8),"+r"(size)
					:
					:"$8","$9","$10","$11","memory"
					);
			}
			asm(".set	push\n"					// save assembler option
				".set	noreorder\n"			// suppress reordering
				"vflush\n"						// Flush VFPU writeback cache
				".set	pop\n"					// restore assembler option
				);
			dst8 = (u8*)((u32)udst8 & ~0x40000000);
			break;
	}

bytecopy:
	// Copy the remains byte per byte...
	while (size--)
	{
		*dst8++ = *src8++;
	}
}*/

//*****************************************************************************
//Swizzled memcopy uses VFPU if possible else normal memcpy() //Corn
//*****************************************************************************
#ifdef DAEDALUS_PSP
void memcpy_vfpu_swizzle( void* dst, const void* src, size_t size )
{
    u8* src8 = (u8*)src;
    u8* dst8 = (u8*)dst;
	u32 *dst32;
	u32 *src32;

	// < 4 isn't worth trying any optimisations...
	if( size<4 ) goto bytecopy;

	// Align dst on 4 bytes or just resume if already done
	while( ((u32)dst8&0x3)!=0 )
	{
		*(u8*)((u32)dst8++ ^ 3) = *(u8*)((u32)src8++ ^ 3);
		size--;
	}
	
	// src has to be at least 32bit aligned to try any optimisations...
	if( (((u32)src8&0x3) == 0) )
	{
		src32 = (u32*)src8;
		dst32 = (u32*)dst8;

		// Align dst to 16 bytes or just resume if already done
		while( ((u32)dst32&0xF)!=0 )
		{
			*dst32++ = *src32++;
			size -= 4;
		}

		src8 = (u8*)src32;
		dst8 = (u8*)dst32;

		// Not atleast 16 bytes at this point? -> not worth trying any VFPU optimisations...
		if (size>15)
		{
			if( ((u32)src8&0xF)==0 )	//Both src and dst are 16byte aligned
			{
				while (size>63)
				{
					asm(".set	push\n"					// save assembler option
						".set	noreorder\n"			// suppress reordering
						"lv.q c000, 0(%1)\n"
						"lv.q c010, 16(%1)\n"
						"lv.q c020, 32(%1)\n"
						"lv.q c030, 48(%1)\n"
						"sv.q c000, 0(%0)\n"
						"sv.q c010, 16(%0)\n"
						"sv.q c020, 32(%0)\n"
						"sv.q c030, 48(%0)\n"
						"addiu  %2, %2, -64\n"			//size -= 64;
						"addiu	%1, %1, 64\n"			//dst8 += 64;
						"addiu	%0, %0, 64\n"			//src8 += 64;
						".set	pop\n"					// restore assembler option
						:"+r"(dst8),"+r"(src8),"+r"(size)
						:
						:"memory"
						);
				}

				while (size>15)
				{
					asm(".set	push\n"					// save assembler option
						".set	noreorder\n"			// suppress reordering
						"lv.q c000, 0(%1)\n"
						"sv.q c000, 0(%0)\n"
						"addiu  %2, %2, -16\n"			//size -= 16;
						"addiu	%1, %1, 16\n"			//dst8 += 16;
						"addiu	%0, %0, 16\n"			//src8 += 16;
						".set	pop\n"					// restore assembler option
						:"+r"(dst8),"+r"(src8),"+r"(size)
						:
						:"memory"
						);
				}

			}
			else 	//At least src is 4byte and dst is 16byte aligned
			{
				while (size>63)
				{
					asm(".set	push\n"					// save assembler option
						".set	noreorder\n"			// suppress reordering
						"ulv.q c000, 0(%1)\n"
						"ulv.q c010, 16(%1)\n"
						"ulv.q c020, 32(%1)\n"
						"ulv.q c030, 48(%1)\n"
						"sv.q c000, 0(%0)\n"
						"sv.q c010, 16(%0)\n"
						"sv.q c020, 32(%0)\n"
						"sv.q c030, 48(%0)\n"
						"addiu  %2, %2, -64\n"			//size -= 64;
						"addiu	%1, %1, 64\n"			//dst8 += 64;
						"addiu	%0, %0, 64\n"			//src8 += 64;
						".set	pop\n"					// restore assembler option
						:"+r"(dst8),"+r"(src8),"+r"(size)
						:
						:"memory"
						);
				}

				while (size>15)
				{
					asm(".set	push\n"					// save assembler option
						".set	noreorder\n"			// suppress reordering
						"ulv.q c000, 0(%1)\n"
						"sv.q c000, 0(%0)\n"
						"addiu  %2, %2, -16\n"			//size -= 16;
						"addiu	%1, %1, 16\n"			//dst8 += 16;
						"addiu	%0, %0, 16\n"			//src8 += 16;
						".set	pop\n"					// restore assembler option
						:"+r"(dst8),"+r"(src8),"+r"(size)
						:
						:"memory"
						);
				}
			}

			// Most copies are completed with the VFPU, so fast out
			if (size == 0) 
				return;
		}

		// Copy the remaning 32bit...
		src32 = (u32*)src8;
		dst32 = (u32*)dst8;
		while(size>3)
		{
			*dst32++ = *src32++;
			size -= 4;
		}

		src8 = (u8*)src32;
		dst8 = (u8*)dst32;
	}
	else
	{
		//We are src unligned.. At least dst is aligned
		dst32 = (u32*)dst8;
		while(size>3)
		{
			register u32 tmp;
			tmp = *(u8*)((u32)src8++ ^ 3);
			tmp = (tmp << 8) | *(u8*)((u32)src8++ ^ 3);
			tmp = (tmp << 8) | *(u8*)((u32)src8++ ^ 3);

			*dst32++ = (tmp << 8) | *(u8*)((u32)src8++ ^ 3);
			size -= 4;
		}
		dst8 = (u8*)dst32;
	}

bytecopy:
	// Copy the remaning bytes if any...
	while (size--)
	{
		*(u8*)((u32)dst8++ ^ 3) = *(u8*)((u32)src8++ ^ 3);
	}
}
#endif

#ifdef PROFILE_MEMCPY
//*****************************************************************************
//
//*****************************************************************************
static inline u64 GetCurrent()
{
    u64 tick;
    sceRtcGetCurrentTick(&tick);
    return (s64)tick;
}

//*****************************************************************************
//
//*****************************************************************************
#define MEMCPY_TEST(d, s, n) {													\
	int gcc_elapsed = 0;														\
	{																			\
		u64 time = GetCurrent();												\
		for (int j=0; j<100; ++j)												\
			memcpy(d, s, n);													\
		gcc_elapsed = (int)(GetCurrent()-time);									\
	}																			\
	int vfpu_elapsed_swizzle = 0;												\
	{																			\
		u64 time = GetCurrent();												\
		for (int j=0; j<100; ++j)												\
			memcpy_vfpu_swizzle(d, s, n);										\
		vfpu_elapsed_swizzle = (int)(GetCurrent()-time);						\
	}																			\
	int vfpu_elapsed = 0;														\
	{																			\
		u64 time = GetCurrent();												\
		for (int j=0; j<100; ++j)												\
			memcpy_vfpu(d, s, n);												\
		vfpu_elapsed = (int)(GetCurrent()-time);								\
	}																			\
	scePowerTick(0);															\
	printf("%6d bytes | GCC%5d | VFPU%5d | VFPUSWIZZLE%5d\n", (int)n, gcc_elapsed, vfpu_elapsed, vfpu_elapsed_swizzle); \
	}

void memcpy_test( void * dst, const void * src, size_t size )
{
	MEMCPY_TEST(dst, src, size);
}
#endif // PROFILE_MEMCPY