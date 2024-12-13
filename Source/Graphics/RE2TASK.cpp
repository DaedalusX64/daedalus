/**
* Daedalus X64 - RE2Task.cpp
* Copyright (C) 2020 Rinnegatamante
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

//
//	N.B. This source code is derived from Mupen64Plus's source
//	and modified by Rinnegatamante to work with Daedalus X64.
//

//#include <stdafx.h>

#include <stdlib.h>
#include <string.h>

#include "Debug/DBGConsole.h"
#include "Core/Memory.h"
#include "Core/RDRam.h"
#include "Ultra/ultra_sptask.h"

#define SATURATE8(x) ((unsigned int) x <= 255 ? x : (x < 0 ? 0: 255))

extern "C" {

void resize_bilinear_task(OSTask *task)
{
    int data_ptr = (u32)task->t.ucode;

    int src_addr = *(u32*)(g_pu8RamBase + data_ptr);
    int dst_addr = *(u32*)(g_pu8RamBase + data_ptr + 4);
    int dst_width = *(u32*)(g_pu8RamBase + data_ptr + 8);
    int dst_height = *(u32*)(g_pu8RamBase + data_ptr + 12);
    int x_ratio = *(u32*)(g_pu8RamBase + data_ptr + 16);
    int y_ratio = *(u32*)(g_pu8RamBase + data_ptr + 20);
    int src_offset = *(u32*)(g_pu8RamBase + data_ptr + 36);

    int a, b, c ,d, index, y_index, xr, yr, blue, green, red, addr, i, j;
    long long x, y, x_diff, y_diff, one_min_x_diff, one_min_y_diff;
    unsigned short pixel;

    src_addr += (src_offset >> 16) * (320 * 3);
    x = y = 0;

    for(i = 0; i < dst_height; i++)
    {
        yr = (int)(y >> 16);
        y_diff = y - (yr << 16);
        one_min_y_diff = 65536 - y_diff;
        y_index = yr * 320;
        x = 0;

        for(j = 0; j < dst_width; j++)
        {
            xr = (int)(x >> 16);
            x_diff = x - (xr << 16);
            one_min_x_diff = 65536 - x_diff;
            index = y_index + xr;
            addr = src_addr + (index * 3);

            rdram_read_many_u8((uint8_t*)&a, addr, 3);
            rdram_read_many_u8((uint8_t*)&b, (addr + 3), 3);
            rdram_read_many_u8((uint8_t*)&c, (addr + (320 * 3)), 3);
            rdram_read_many_u8((uint8_t*)&d, (addr + (320 * 3) + 3), 3);

            blue = (int)(((a&0xff)*one_min_x_diff*one_min_y_diff + (b&0xff)*x_diff*one_min_y_diff +
                          (c&0xff)*y_diff*one_min_x_diff         + (d&0xff)*x_diff*y_diff) >> 32);

            green = (int)((((a>>8)&0xff)*one_min_x_diff*one_min_y_diff + ((b>>8)&0xff)*x_diff*one_min_y_diff +
                           ((c>>8)&0xff)*y_diff*one_min_x_diff         + ((d>>8)&0xff)*x_diff*y_diff) >> 32);

            red = (int)((((a>>16)&0xff)*one_min_x_diff*one_min_y_diff + ((b>>16)&0xff)*x_diff*one_min_y_diff +
                         ((c>>16)&0xff)*y_diff*one_min_x_diff         + ((d>>16)&0xff)*x_diff*y_diff) >> 32);

            blue = (blue >> 3) & 0x001f;
            green = (green >> 3) & 0x001f;
            red = (red >> 3) & 0x001f;
            pixel = (red << 11) | (green << 6) | (blue << 1) | 1;

            rdram_write_many_u16(&pixel, dst_addr, 1);
            dst_addr += 2;

            x += x_ratio;
        }
        y += y_ratio;
    }
}

static uint32_t YCbCr_to_RGBA(uint8_t Y, uint8_t Cb, uint8_t Cr)
{
    int r, g, b;

    r = (int)(((double)Y * 0.582199097) + (0.701004028 * (double)(Cr - 128)));
    g = (int)(((double)Y * 0.582199097) - (0.357070923 * (double)(Cr - 128)) - (0.172073364 * (double)(Cb - 128)));
    b = (int)(((double)Y * 0.582199097) + (0.886001587 * (double)(Cb - 128)));
    
    r = SATURATE8(r);
    g = SATURATE8(g);
    b = SATURATE8(b);
    
    return (r << 24) | (g << 16) | (b << 8) | 0;
}

void decode_video_frame_task(OSTask *task)
{
    int data_ptr = (u32)task->t.ucode;

    int pLuminance = *(u32*)(g_pu8RamBase + data_ptr);
    int pCb = *(u32*)(g_pu8RamBase + data_ptr + 4);
    int pCr = *(u32*)(g_pu8RamBase + data_ptr + 8);
    int pDestination = *(u32*)(g_pu8RamBase + data_ptr + 12);
    int nMovieWidth = *(u32*)(g_pu8RamBase + data_ptr + 16);
    int nMovieHeight = *(u32*)(g_pu8RamBase + data_ptr + 20);
    int nScreenDMAIncrement = *(u32*)(g_pu8RamBase + data_ptr + 36);

    int i, j;
    uint8_t Y, Cb, Cr;
    uint32_t pixel;
    int pY_1st_row, pY_2nd_row, pDest_1st_row, pDest_2nd_row;

    for (i = 0; i < nMovieHeight; i += 2)
    {
        pY_1st_row = pLuminance;
        pY_2nd_row = pLuminance + nMovieWidth;
        pDest_1st_row = pDestination;
        pDest_2nd_row = pDestination + (nScreenDMAIncrement >> 1);

        for (j = 0; j < nMovieWidth; j += 2)
        {
            rdram_read_many_u8((uint8_t*)&Cb, pCb++, 1);
            rdram_read_many_u8((uint8_t*)&Cr, pCr++, 1);

            /*1st row*/
            rdram_read_many_u8((uint8_t*)&Y, pY_1st_row++, 1);
            pixel = YCbCr_to_RGBA(Y, Cb, Cr);
            rdram_write_many_u32(&pixel, pDest_1st_row, 1);
            pDest_1st_row += 4;

            rdram_read_many_u8((uint8_t*)&Y, pY_1st_row++, 1);
            pixel = YCbCr_to_RGBA(Y, Cb, Cr);
            rdram_write_many_u32(&pixel, pDest_1st_row, 1);
            pDest_1st_row += 4;

            /*2nd row*/
            rdram_read_many_u8((uint8_t*)&Y, pY_2nd_row++, 1);
            pixel = YCbCr_to_RGBA(Y, Cb, Cr);
            rdram_write_many_u32(&pixel, pDest_2nd_row, 1);
            pDest_2nd_row += 4;

            rdram_read_many_u8((uint8_t*)&Y, pY_2nd_row++, 1);
            pixel = YCbCr_to_RGBA(Y, Cb, Cr);
            rdram_write_many_u32(&pixel, pDest_2nd_row, 1);
            pDest_2nd_row += 4;
        }

        pLuminance += (nMovieWidth << 1);
        pDestination += nScreenDMAIncrement;
    }
}

void fill_video_double_buffer_task(OSTask *task)
{
    int data_ptr = (u32)task->t.ucode;

    int pSrc = *(u32*)(g_pu8RamBase + data_ptr);
    int pDest = *(u32*)(g_pu8RamBase + data_ptr + 0x4);
    int width = *(u32*)(g_pu8RamBase + data_ptr + 0x8) >> 1;
    int height = *(u32*)(g_pu8RamBase + data_ptr + 0x10) << 1;
    int stride = *(u32*)(g_pu8RamBase + data_ptr + 0x1c) >> 1;

    int i, j;
    int r, g, b;
    uint32_t pixel, pixel1, pixel2;

    for(i = 0; i < height; i++)
    {
      for(j = 0; j < width; j=j+4)
      {
        pixel1 = *(u32*)(g_pu8RamBase + (pSrc+j));
        pixel2 = *(u32*)(g_pu8RamBase + (pDest+j));
      
        r = (((pixel1 >> 24) & 0xff) + ((pixel2 >> 24) & 0xff)) >> 1;
        g = (((pixel1 >> 16) & 0xff) + ((pixel2 >> 16) & 0xff)) >> 1;
        b = (((pixel1 >> 8) & 0xff) + ((pixel2 >> 8) & 0xff)) >> 1;
      
        pixel = (r << 24) | (g << 16) | (b << 8) | 0;
      
        rdram_write_many_u32(&pixel, pDest+j, 1);
      }
      pSrc += stride;
      pDest += stride;
    }
}

};