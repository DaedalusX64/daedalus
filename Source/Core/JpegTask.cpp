/**
* Mupen64 hle rsp - jpeg.c
* Copyright (C) 2012 Bobby Smiles                                       *
* Copyright (C) 2009 Richard Goedeken                                   *
* Copyright (C) 2002 Hacktarux
*
* Mupen64 homepage: http://mupen64.emulation64.com
* email address: hacktarux@yahoo.fr
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

#include "Debug/DBGConsole.h"
#include "Memory.h"
#include "OSHLE/ultra_sptask.h"

#define SUBBLOCK_SIZE 64

typedef void (*tile_line_emitter_t)(const s16 *y, const s16 *u, u32 address);

/* rdram operations */
// FIXME: these functions deserve their own module
static void rdram_read_many_u16(u16 *dst, u32 address, u32 count);
static void rdram_write_many_u16(const u16 *src, u32 address, u32 count);
static u32 rdram_read_u32(u32 address);
static void rdram_write_many_u32(const u32 *src, u32 address, u32 count);

/* helper functions */
static u8 clamp_u8(s16 x);
//static s16 clamp_s12(s16 x);
static s16 clamp_s16(s32 x);
static u16 clamp_RGBA_component(s16 x);

/* pixel conversion & foratting */
static u32 GetUYVY(s16 y1, s16 y2, s16 u, s16 v);
static u16 GetRGBA(s16 y, s16 u, s16 v);

/* tile line emitters */
static void EmitYUVTileLine(const s16 *y, const s16 *u, u32 address);
//static void EmitYUVTileLine_SwapY1Y2(const s16 *y, const s16 *u, u32 address);
static void EmitRGBATileLine(const s16 *y, const s16 *u, u32 address);

/* macroblocks operations */
static void DecodeMacroblock1(s16 *macroblock, s32 *y_dc, s32 *u_dc, s32 *v_dc, const s16 *qtable);
static void DecodeMacroblock2(s16 *macroblock, u32 subblock_count, const s16 qtables[3][SUBBLOCK_SIZE]);
//static void DecodeMacroblock3(s16 *macroblock, u32 subblock_count, const s16 qtables[3][SUBBLOCK_SIZE]);
static void EmitTilesMode0(const tile_line_emitter_t emit_line, const s16 *macroblock, u32 address);
static void EmitTilesMode2(const tile_line_emitter_t emit_line, const s16 *macroblock, u32 address);

/* subblocks operations */
static void TransposeSubBlock(s16 *dst, const s16 *src);
static void ZigZagSubBlock(s16 *dst, const s16 *src);
static void ReorderSubBlock(s16 *dst, const s16 *src, const u32 *table);
static void MultSubBlocks(s16 *dst, const s16 *src1, const s16 *src2, u32 shift);
static void ScaleSubBlock(s16 *dst, const s16 *src, s16 scale);
static void RShiftSubBlock(s16 *dst, const s16 *src, u32 shift);
static void InverseDCT1D(const float * const x, float *dst, u32 stride);
static void InverseDCTSubBlock(s16 *dst, const s16 *src);
//static void RescaleYSubBlock(s16 *dst, const s16 *src);
//static void RescaleUVSubBlock(s16 *dst, const s16 *src);

/* transposed dequantization table */
const s16 DEFAULT_QTABLE[SUBBLOCK_SIZE] =
{
    16, 12, 14, 14,  18,  24,  49,  72,
    11, 12, 13, 17,  22,  35,  64,  92,
    10, 14, 16, 22,  37,  55,  78,  95,
    16, 19, 24, 29,  56,  64,  87,  98,
    24, 26, 40, 51,  68,  81, 103, 112,
    40, 58, 57, 87, 109, 104, 121, 100,
    51, 60, 69, 80, 103, 113, 120, 103,
    61, 55, 56, 62,  77,  92, 101,  99
};

/* zig-zag indices */
const u32 ZIGZAG_TABLE[SUBBLOCK_SIZE] =
{
     0,  1,  5,  6, 14, 15, 27, 28,
     2,  4,  7, 13, 16, 26, 29, 42,
     3,  8, 12, 17, 25, 30, 41, 43,
     9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
};

/* transposition indices */
const u32 TRANSPOSE_TABLE[SUBBLOCK_SIZE] =
{
    0,  8, 16, 24, 32, 40, 48, 56,
    1,  9, 17, 25, 33, 41, 49, 57,
    2, 10, 18, 26, 34, 42, 50, 58,
    3, 11, 19, 27, 35, 43, 51, 59,
    4, 12, 20, 28, 36, 44, 52, 60,
    5, 13, 21, 29, 37, 45, 53, 61,
    6, 14, 22, 30, 38, 46, 54, 62,
    7, 15, 23, 31, 39, 47, 55, 63
};



/***************************************************************************
 * JPEG decoding ucode found in Ocarina of Time, Pokemon Stadium 1 and
 * Pokemon Stadium 2.
 **************************************************************************/
void jpeg_decode_PS(OSTask *task)
{
	s16 *macroblock {};
    s16 qtables[3][SUBBLOCK_SIZE];
    u32 mb {};

    #ifdef DAEDALUS_DEBUG_CONSOLE
    if (task->t.flags & 0x1)
    {
        DBGConsole_Msg(0, "jpeg_decode_PS: task yielding not implemented");
        return;
    }
    #endif
    u32       address          {rdram_read_u32((u32)task->t.data_ptr)};
    const u32 macroblock_count {rdram_read_u32((u32)task->t.data_ptr + 4)};
    const u32 mode             {rdram_read_u32((u32)task->t.data_ptr + 8)};
    const u32 qtableY_ptr      {rdram_read_u32((u32)task->t.data_ptr + 12)};
    const u32 qtableU_ptr      {rdram_read_u32((u32)task->t.data_ptr + 16)};
    const u32 qtableV_ptr      {rdram_read_u32((u32)task->t.data_ptr + 20)};

    #ifdef DAEDALUS_DEBUG_CONSOLE
    if (mode != 0 && mode != 2)
    {
        DBGConsole_Msg(0, "jpeg_decode_PS: invalid mode %d", mode);
        return;
    }
    #endif

    rdram_read_many_u16((u16*)qtables[0], qtableY_ptr, SUBBLOCK_SIZE);
    rdram_read_many_u16((u16*)qtables[1], qtableU_ptr, SUBBLOCK_SIZE);
    rdram_read_many_u16((u16*)qtables[2], qtableV_ptr, SUBBLOCK_SIZE);

	void (*EmitTilesMode)(const tile_line_emitter_t, const s16 *, u32);

	if (mode == 0)
	{
		EmitTilesMode =  EmitTilesMode0;
	}
	else
	{
		EmitTilesMode =  EmitTilesMode2;
	}

	const u32 subblock_count {mode + 4};
	const u32 macroblock_size {2*subblock_count*SUBBLOCK_SIZE};

	macroblock = (s16 *)malloc(sizeof(*macroblock) * macroblock_size);
  #ifdef DAEDALUS_DEBUG_CONSOLE
	if (!macroblock)
	{
		DBGConsole_Msg(0, "jpeg_decode_PS: could not allocate macroblock");
		return;
	}
  #endif

    for (mb = 0; mb < macroblock_count; ++mb)
    {
        rdram_read_many_u16((u16*)macroblock, address, macroblock_size >> 1);
        DecodeMacroblock2(macroblock, subblock_count, (const s16 (*)[SUBBLOCK_SIZE])qtables);
		EmitTilesMode(EmitRGBATileLine, macroblock, address);

        address += macroblock_size;
    }
	free(macroblock);
}

/***************************************************************************
 * JPEG decoding ucode found in Ogre Battle and Bottom of the 9th.
 **************************************************************************/
void jpeg_decode_OB(OSTask *task)
{
    s16 qtable[SUBBLOCK_SIZE] {};
    u32 mb {};

    s32 y_dc {}, u_dc {}, v_dc {};

	u32  address  {(u32)task->t.data_ptr};
	const u32 macroblock_count {task->t.data_size};
	const u32  qscale   {task->t.yield_data_size};

    if (task->t.yield_data_size != 0 )
    {
        if (task->t.yield_data_size > 0)
        {
            ScaleSubBlock(qtable, DEFAULT_QTABLE, qscale);
        }
        else
        {
            RShiftSubBlock(qtable, DEFAULT_QTABLE, -qscale);
        }
    }

    for (mb = 0; mb < macroblock_count; ++mb)
    {
        s16 macroblock[6*SUBBLOCK_SIZE];

        rdram_read_many_u16((u16*)macroblock, address, 6*SUBBLOCK_SIZE);
        DecodeMacroblock1(macroblock, &y_dc, &u_dc, &v_dc, (qscale != 0) ? qtable : NULL);
        EmitTilesMode2(EmitYUVTileLine, macroblock, address);

        address += (2*6*SUBBLOCK_SIZE);
    }
}

static u8 clamp_u8(s16 x)
{
    return (x & (0xff00)) ? ((-x) >> 15) & 0xff : x;
}

//static s16 clamp_s12(s16 x)
//{
//    if (x < -0x800) { x = -0x800; } else if (x > 0x7f0) { x = 0x7f0; }
//    return x;
//}

static s16 clamp_s16(s32 x)
{
    if (x > 32767) { x = 32767; } else if (x < -32768) { x = -32768; }
    return x;
}

static u16 clamp_RGBA_component(s16 x)
{
    if (x > 0xff0) { x = 0xff0; } else if (x < 0) { x = 0; }
    return (x & 0xf80);
}

static u32 GetUYVY(s16 y1, s16 y2, s16 u, s16 v)
{
    return (u32)clamp_u8(u)  << 24
        |  (u32)clamp_u8(y1) << 16
        |  (u32)clamp_u8(v)  << 8
        |  (u32)clamp_u8(y2);
}

static u16 GetRGBA(s16 y, s16 u, s16 v)
{
    const float fY = (float)y + 2048.0f;
    const float fU = (float)u;
    const float fV = (float)v;

    const u16 r = clamp_RGBA_component((s16)(fY             + 1.4025*fV));
    const u16 g = clamp_RGBA_component((s16)(fY - 0.3443*fU - 0.7144*fV));
    const u16 b = clamp_RGBA_component((s16)(fY + 1.7729*fU            ));

    return (r << 4) | (g >> 1) | (b >> 6) | 1;
}

static void EmitYUVTileLine(const s16 *y, const s16 *u, u32 address)
{
    u32 uyvy[8] {};

    const s16 * const v  = u + SUBBLOCK_SIZE;
    const s16 * const y2 = y + SUBBLOCK_SIZE;

    uyvy[0] = GetUYVY(y[0],  y[1],  u[0], v[0]);
    uyvy[1] = GetUYVY(y[2],  y[3],  u[1], v[1]);
    uyvy[2] = GetUYVY(y[4],  y[5],  u[2], v[2]);
    uyvy[3] = GetUYVY(y[6],  y[7],  u[3], v[3]);
    uyvy[4] = GetUYVY(y2[0], y2[1], u[4], v[4]);
    uyvy[5] = GetUYVY(y2[2], y2[3], u[5], v[5]);
    uyvy[6] = GetUYVY(y2[4], y2[5], u[6], v[6]);
    uyvy[7] = GetUYVY(y2[6], y2[7], u[7], v[7]);

    rdram_write_many_u32(uyvy, address, 8);
}
/*
static void EmitYUVTileLine_SwapY1Y2(const s16 *y, const s16 *u, u32 address)
{
    u32 uyvy[8];

    const s16 * const v  = u + SUBBLOCK_SIZE;
    const s16 * const y2 = y + SUBBLOCK_SIZE;

    uyvy[0] = GetUYVY(y[1],  y[0],  u[0], v[0]);
    uyvy[1] = GetUYVY(y[3],  y[2],  u[1], v[1]);
    uyvy[2] = GetUYVY(y[5],  y[4],  u[2], v[2]);
    uyvy[3] = GetUYVY(y[7],  y[6],  u[3], v[3]);
    uyvy[4] = GetUYVY(y2[1], y2[0], u[4], v[4]);
    uyvy[5] = GetUYVY(y2[3], y2[2], u[5], v[5]);
    uyvy[6] = GetUYVY(y2[5], y2[4], u[6], v[6]);
    uyvy[7] = GetUYVY(y2[7], y2[6], u[7], v[7]);

    rdram_write_many_u32(uyvy, address, 8);
}
*/
static void EmitRGBATileLine(const s16 *y, const s16 *u, u32 address)
{
    u16 rgba[16] {};

    const s16 * const v  = u + SUBBLOCK_SIZE;
    const s16 * const y2 = y + SUBBLOCK_SIZE;

    rgba[0]  = GetRGBA(y[0],  u[0], v[0]);
    rgba[1]  = GetRGBA(y[1],  u[0], v[0]);
    rgba[2]  = GetRGBA(y[2],  u[1], v[1]);
    rgba[3]  = GetRGBA(y[3],  u[1], v[1]);
    rgba[4]  = GetRGBA(y[4],  u[2], v[2]);
    rgba[5]  = GetRGBA(y[5],  u[2], v[2]);
    rgba[6]  = GetRGBA(y[6],  u[3], v[3]);
    rgba[7]  = GetRGBA(y[7],  u[3], v[3]);
    rgba[8]  = GetRGBA(y2[0], u[4], v[4]);
    rgba[9]  = GetRGBA(y2[1], u[4], v[4]);
    rgba[10] = GetRGBA(y2[2], u[5], v[5]);
    rgba[11] = GetRGBA(y2[3], u[5], v[5]);
    rgba[12] = GetRGBA(y2[4], u[6], v[6]);
    rgba[13] = GetRGBA(y2[5], u[6], v[6]);
    rgba[14] = GetRGBA(y2[6], u[7], v[7]);
    rgba[15] = GetRGBA(y2[7], u[7], v[7]);

    rdram_write_many_u16(rgba, address, 16);
}

static void EmitTilesMode0(const tile_line_emitter_t emit_line, const s16 *macroblock, u32 address)
{
    u32 i;

    u32 y_offset = 0;
    u32 u_offset = 2*SUBBLOCK_SIZE;

    for (i = 0; i < 8; ++i)
    {
        emit_line(&macroblock[y_offset], &macroblock[u_offset], address);

        y_offset += 8;
        u_offset += 8;
        address += 32;
    }
}

static void EmitTilesMode2(const tile_line_emitter_t emit_line, const s16 *macroblock, u32 address)
{

    u32 y_offset {0};
    u32 u_offset {4*SUBBLOCK_SIZE};

    for (u32 i = 0; i < 8; ++i)
    {
        emit_line(&macroblock[y_offset],     &macroblock[u_offset], address);
        emit_line(&macroblock[y_offset + 8], &macroblock[u_offset], address + 32);

        y_offset += (i == 3) ? SUBBLOCK_SIZE+16 : 16;
        u_offset += 8;
        address += 64;
    }
}

static void DecodeMacroblock1(s16 *macroblock, s32 *y_dc, s32 *u_dc, s32 *v_dc, const s16 *qtable)
{

    for (u32 sb = 0; sb < 6; ++sb)
    {
        s16 tmp_sb[SUBBLOCK_SIZE] {};

        /* update DC */
        s32 dc {(s32)macroblock[0]};
        switch(sb)
        {
        case 0: case 1: case 2: case 3:
                *y_dc += dc; macroblock[0] = *y_dc & 0xffff; break;
        case 4: *u_dc += dc; macroblock[0] = *u_dc & 0xffff; break;
        case 5: *v_dc += dc; macroblock[0] = *v_dc & 0xffff; break;
        }

        ZigZagSubBlock(tmp_sb, macroblock);
        if (qtable != NULL) { MultSubBlocks(tmp_sb, tmp_sb, qtable, 0); }
        TransposeSubBlock(macroblock, tmp_sb);
        InverseDCTSubBlock(macroblock, macroblock);

        macroblock += SUBBLOCK_SIZE;
    }
}

static void DecodeMacroblock2(s16 *macroblock, u32 subblock_count, const s16 qtables[3][SUBBLOCK_SIZE])
{
    u32 q {};

    for (u32 sb = 0; sb < subblock_count; ++sb)
    {
        s16 tmp_sb[SUBBLOCK_SIZE] {};
        const int isChromaSubBlock = (subblock_count - sb <= 2);

        if (isChromaSubBlock) { ++q; }

        MultSubBlocks(macroblock, macroblock, qtables[q], 4);
        ZigZagSubBlock(tmp_sb, macroblock);
        InverseDCTSubBlock(macroblock, tmp_sb);

        macroblock += SUBBLOCK_SIZE;
    }

}
/*
static void DecodeMacroblock3(s16 *macroblock, u32 subblock_count, const s16 qtables[3][SUBBLOCK_SIZE])
{
    u32 sb;
    u32 q = 0;

    for (sb = 0; sb < subblock_count; ++sb)
    {
        s16 tmp_sb[SUBBLOCK_SIZE];
        const int isChromaSubBlock = (subblock_count - sb <= 2);

        if (isChromaSubBlock) { ++q; }

        MultSubBlocks(macroblock, macroblock, qtables[q], 4);
        ZigZagSubBlock(tmp_sb, macroblock);
        InverseDCTSubBlock(macroblock, tmp_sb);

        if (isChromaSubBlock)
        {
            RescaleUVSubBlock(macroblock, macroblock);
        }
        else
        {
            RescaleYSubBlock(macroblock, macroblock);
        }

        macroblock += SUBBLOCK_SIZE;
    }
}
*/

static void TransposeSubBlock(s16 *dst, const s16 *src)
{
    ReorderSubBlock(dst, src, TRANSPOSE_TABLE);
}

static void ZigZagSubBlock(s16 *dst, const s16 *src)
{
    ReorderSubBlock(dst, src, ZIGZAG_TABLE);
}

static void ReorderSubBlock(s16 *dst, const s16 *src, const u32 *table)
{
    /* source and destination sublocks cannot overlap */
    //assert(abs(dst - src) > SUBBLOCK_SIZE);

    for (u32 i {}; i < SUBBLOCK_SIZE; ++i)
    {
        dst[i] = src[table[i]];
    }
}

static void MultSubBlocks(s16 *dst, const s16 *src1, const s16 *src2, u32 shift)
{

    for (u32 i {}; i < SUBBLOCK_SIZE; ++i)
    {
        s32 v {src1[i] * src2[i]};
        dst[i] = clamp_s16(v) << shift;
    }
}

static void ScaleSubBlock(s16 *dst, const s16 *src, s16 scale)
{
    for (u32 i {}; i < SUBBLOCK_SIZE; ++i)
    {
        s32 v {src[i] * scale};
        dst[i] = clamp_s16(v);
    }
}

static void RShiftSubBlock(s16 *dst, const s16 *src, u32 shift)
{

    for (u32 i {}; i < SUBBLOCK_SIZE; ++i)
    {
        dst[i] = src[i] >> shift;
    }
}

/***************************************************************************
 * Fast 2D IDCT using separable formulation and normalization
 * Computations use single precision floats
 * Implementation based on Wikipedia :
 * http://fr.wikipedia.org/wiki/Transform%C3%A9e_en_cosinus_discr%C3%A8te
 **************************************************************************/

/* Normalized such as C4 = 1 */
#define C3   1.175875602f
#define C6   0.541196100f
#define K1   0.765366865f   //  C2-C6
#define K2  -1.847759065f   // -C2-C6
#define K3  -0.390180644f   //  C5-C3
#define K4  -1.961570561f   // -C5-C3
#define K5   1.501321110f   //  C1+C3-C5-C7
#define K6   2.053119869f   //  C1+C3-C5+C7
#define K7   3.072711027f   //  C1+C3+C5-C7
#define K8   0.298631336f   // -C1+C3+C5-C7
#define K9  -0.899976223f   //  C7-C3
#define K10 -2.562915448f   // -C1-C3
static void InverseDCT1D(const float * const x, float *dst, u32 stride)
{
    float e[4] {};
    float f[4] {};
    float x26 {}, x1357 {}, x15 {}, x37 {}, x17 {}, x35 {};

    x15   =  K3 * (x[1] + x[5]);
    x37   =  K4 * (x[3] + x[7]);
    x17   =  K9 * (x[1] + x[7]);
    x35   = K10 * (x[3] + x[5]);
    x1357 =  C3 * (x[1] + x[3] + x[5] + x[7]);
    x26   =  C6 * (x[2] + x[6]);

    f[0] = x[0] + x[4];
    f[1] = x[0] - x[4];
    f[2] = x26 + K1*x[2];
    f[3] = x26 + K2*x[6];

    e[0] = x1357 + x15 + K5*x[1] + x17;
    e[1] = x1357 + x37 + K7*x[3] + x35;
    e[2] = x1357 + x15 + K6*x[5] + x35;
    e[3] = x1357 + x37 + K8*x[7] + x17;

    *dst = f[0] + f[2] + e[0]; dst += stride;
    *dst = f[1] + f[3] + e[1]; dst += stride;
    *dst = f[1] - f[3] + e[2]; dst += stride;
    *dst = f[0] - f[2] + e[3]; dst += stride;
    *dst = f[0] - f[2] - e[3]; dst += stride;
    *dst = f[1] - f[3] - e[2]; dst += stride;
    *dst = f[1] + f[3] - e[1]; dst += stride;
    *dst = f[0] + f[2] - e[0]; dst += stride;
}
#undef C3
#undef C6
#undef K1
#undef K2
#undef K3
#undef K4
#undef K5
#undef K6
#undef K7
#undef K8
#undef K9
#undef K10

static void InverseDCTSubBlock(s16 *dst, const s16 *src)
{
    float x[8] {};
    float block[SUBBLOCK_SIZE] {};
    u32 i {}, j {};

    /* idct 1d on rows (+transposition) */
    for (i = 0; i < 8; ++i)
    {
        for (j = 0; j < 8; ++j)
        {
            x[j] = (float)src[i*8+j];
        }

        InverseDCT1D(x, &block[i], 8);
    }

    /* idct 1d on columns (thanks to previous transposition) */
    for (i = 0; i < 8; ++i)
    {
        InverseDCT1D(&block[i*8], x, 1);

        /* C4 = 1 normalization implies a division by 8 */
        for (j = 0; j < 8; ++j)
        {
            dst[i+j*8] = (s16)x[j] >> 3;
        }
    }
}
/*
static void RescaleYSubBlock(s16 *dst, const s16 *src)
{
    u32 i;

    for (i = 0; i < SUBBLOCK_SIZE; ++i)
    {
#if 0
        dst[i] = (((u32)(clamp_s12(src[i]) + 0x800) * 0xdb0) >> 16) + 0x10;
#else
        // FIXME: ! DIRTY HACK ! (compensate for too dark pictures)
        dst[i] = (((u32)(clamp_s12(src[i]) + 0x800) * 0xdb0) >> 16) + 0x50;
#endif
    }
}

static void RescaleUVSubBlock(s16 *dst, const s16 *src)
{
    u32 i;

    for (i = 0; i < SUBBLOCK_SIZE; ++i)
    {
        dst[i] = (((int)clamp_s12(src[i]) * 0xe00) >> 16) + 0x80;
    }
}
*/


/* FIXME: assume presence of expansion pack */
#define MEMMASK 0x7fffff

//ToDo: fast_memcpy_swizzle?
static void rdram_read_many_u16(u16 *dst, u32 address, u32 count)
{
	const u8 *src {g_pu8RamBase + (address& MEMMASK)};

    while (count != 0)
    {
		u32 a {*(u8*)((uintptr_t)src++ ^ U8_TWIDDLE)};
		u32 b {*(u8*)((uintptr_t)src++ ^ U8_TWIDDLE)};

		*(dst++) = ((a << 8) | b);
		--count;
    }
}

static void rdram_write_many_u16(const u16 *src, u32 address, u32 count)
{
	u8 *dst {g_pu8RamBase + (address& MEMMASK)};
    while (count != 0)
    {
       *(u8*)((uintptr_t)dst++ ^ U8_TWIDDLE) = (u8)(*src >> 8);
       *(u8*)((uintptr_t)dst++ ^ U8_TWIDDLE)= (u8)(*(src++) & 0xff);

        --count;
    }
}

static u32 rdram_read_u32(u32 address)
{
	const u8 *src {g_pu8RamBase + (address& MEMMASK)};

	u32 a {*(u8*)((uintptr_t)src++ ^ U8_TWIDDLE)};
	u32 b {*(u8*)((uintptr_t)src++ ^ U8_TWIDDLE)};
	u32 c {*(u8*)((uintptr_t)src++ ^ U8_TWIDDLE)};
	u32 d {*(u8*)((uintptr_t)src++ ^ U8_TWIDDLE)};

    return (a << 24) | (b << 16) | (c << 8) | d;
}

static void rdram_write_many_u32(const u32 *src, u32 address, u32 count)
{
	u8 *dst {g_pu8RamBase + (address& MEMMASK)};
    while (count != 0)
    {
       *(u8*)((uintptr_t)dst++ ^ U8_TWIDDLE) = (u8)(*src >> 24);
       *(u8*)((uintptr_t)dst++ ^ U8_TWIDDLE) = (u8)(*src >> 16);
       *(u8*)((uintptr_t)dst++ ^ U8_TWIDDLE) = (u8)(*src >> 8);
       *(u8*)((uintptr_t)dst++ ^ U8_TWIDDLE) = (u8)(*(src++) & 0xff);

        --count;
    }
}
