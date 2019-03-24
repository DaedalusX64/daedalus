/*

Copyright (C) 2009 Raphael

E-mail:   raphael@fx-world.org
homepage: http://wordpress.fx-world.org

*/

#include "stdafx.h"
#include "Utility/FastMemcpy.h"
#include "Utility/DaedalusTypes.h"

#include <string.h>


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
//Swizzled memcopy uses VFPU if possible else normal memcpy() //Corn
//*****************************************************************************
void memcpy_vfpu_byteswap( void* dst, const void* src, size_t size )
{
    u8* src8 = (u8*)src;
    u8* dst8 = (u8*)dst;

	u32 *dst32;
	u32 *src32;

	// < 4 isn't worth trying any optimisations...
	if( size>=4 )
	{
		// Align dst on 4 bytes or just resume if already done
		while( ((u32)dst8&0x3)!=0 )
		{
			*(u8*)((u32)dst8++ ^ 3) = *(u8*)((u32)src8++ ^ 3);
			size--;
		}

		// if < 4 its not possible to optimize...
		if( size>=4 )
		{
			// src has to be at least 32bit aligned to try any optimisations...
			if( (((u32)src8&0x3) == 0) )
			{
				// Not atleast 16 bytes at this point? -> not worth trying any VFPU optimisations...
				if (size>15)
				{
					src32 = (u32*)src8;
					dst32 = (u32*)dst8;

					// Align dst to 16 bytes to gain from vfpu aligned stores or just resume if already done
					while( ((u32)dst32&0xF)!=0 )
					{
						*dst32++ = *src32++;
						size -= 4;
					}

					src8 = (u8*)src32;
					dst8 = (u8*)dst32;

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
				//We are now dst aligned and src unligned and >= 4 bytes to copy
				dst32 = (u32*)dst8;
				src32 = (u32*)((u32)src8 & ~0x3);

				u32 srcTmp {*src32++};
				u32 dstTmp {0};
				u32 size32 {size >> 2}; // Size in dwords (to avoid a sltiu in loop)

				size &= 0x3; // Update remaining bytes if any..

				switch( (u32)src8&0x3 )
				{
					/*case 0:	//src is aligned too
						src32 = (u32*)src8;
						while(size32--)
						{
							*dst32++ = *src32++;
						}
						break;*/

					case 1:
						{
							while(size32--)
							{
								dstTmp = srcTmp << 8;
								srcTmp = *src32++;
								dstTmp |= srcTmp >> 24;
								*dst32++ = dstTmp;
							}
							src8 = (u8*)src32 - 3;
						}
						break;

					case 2:
						{
							while(size32--)
							{
								dstTmp = srcTmp << 16;
								srcTmp = *src32++;
								dstTmp |= srcTmp >> 16;
								*dst32++ = dstTmp;
							}
							src8 = (u8*)src32 - 2;
						}
						break;

					case 3:
						{
							while(size32--)
							{
								dstTmp = srcTmp << 24;
								srcTmp = *src32++;
								dstTmp |= srcTmp >> 8;
								*dst32++ = dstTmp;
							}
							src8 = (u8*)src32 - 1;
						}
						break;
				}
				dst8 = (u8*)dst32;
			}
		}
	}

	// Copy the remaning bytes if any...
	while (size--)
	{
		*(u8*)((u32)dst8++ ^ 3) = *(u8*)((u32)src8++ ^ 3);
	}
}
