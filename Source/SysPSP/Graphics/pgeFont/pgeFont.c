/*
 * pgeFont.c
 * This file is part of the "Phoenix Game Engine".
 *
 * Copyright (C) 2007 Phoenix Game Engine
 * Copyright (C) 2007 David Perry <tias_dp@hotmail.com>
 *
 * This work is licensed under the Creative Commons Attribution-Share Alike 3.0 License.
 * See LICENSE for more details.
 *
 */

#include <pspkernel.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <malloc.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "pgeFont.h"

static const int PGE_FONT_TEXTURE_MIN_SIZE = 64;
static const int PGE_FONT_TEXTURE_MAX_SIZE = 512;

static const char *PGE_FONT_CHARSET =
	" .,!?:;"
    	"0123456789"
    	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    	"abcdefghijklmnopqrstuvwxyz"
    	"@#\"�$%^&*()[]{}<>/\\|~`+-=_~"
;

static unsigned int __attribute__((aligned(16))) clut[16];

typedef struct GlyphInfo
{
	struct GlyphInfo *next, *prev;
	unsigned int c;
	Glyph glyph;
} GlyphInfo;

GlyphInfo glyphList =
{
	.next = &glyphList,
	.prev = &glyphList,
};

static int pgeFontSwizzle(pgeFont *font)
{
	int byteWidth = font->texSize>>1;
	int textureSize = font->texSize*font->texHeight>>1;
	int height = textureSize / byteWidth;

	int rowBlocks = (byteWidth>>4);
	int rowBlocksAdd = (rowBlocks - 1)<<7;
	unsigned int blockAddress = 0;
	unsigned int *src = (unsigned int*) font->texture;
	static unsigned char *tData;

	tData = (unsigned char*) malloc(textureSize);

	if(!tData)
		return 0;

	int j;

	for(j = 0; j < height; j++, blockAddress += 16)
	{
		unsigned int *block = ((unsigned int*)&tData[blockAddress]);

		int i;

		for(i=0; i < rowBlocks; i++)
		{
			*block++ = *src++;
			*block++ = *src++;
			*block++ = *src++;
			*block++ = *src++;
			block += 28;
		}

		if((j & 0x7) == 0x7)
			blockAddress += rowBlocksAdd;
	}

	free(font->texture);

	font->texture = tData;

	return 1;
}

pgeFont* pgeFontLoad(const char *filename, unsigned int fontSize, enum pgeFontSizeType fontSizeType, unsigned int textureSize)
{
	FT_Library library;
	FT_Face face;

	if(FT_Init_FreeType(&library))
		return NULL;

	if(FT_New_Face(library, filename, 0, &face))
		return NULL;

	FT_GlyphSlot slot;
	GlyphInfo *gp;
	GlyphInfo gi[256];
	int n, count, charCount;
	int xx, yy;

	charCount = strlen(PGE_FONT_CHARSET);
	count = charCount;

	if((textureSize < PGE_FONT_TEXTURE_MIN_SIZE) || (textureSize > PGE_FONT_TEXTURE_MAX_SIZE))
		return NULL;

	pgeFont* font = (pgeFont*) malloc(sizeof(pgeFont));

	if(!font)
		return NULL;

	font->texSize = textureSize;

	if(fontSizeType == PGE_FONT_SIZE_PIXELS)
	{
		if(FT_Set_Pixel_Sizes(face, fontSize, 0))
		{
			pgeFontUnload(font);
			return NULL;
		}
	}
	else
	{
		if(FT_Set_Char_Size(face, fontSize<<6, 0, 100, 0))
		{
			pgeFontUnload(font);
			return NULL;
		}
	}

	slot = face->glyph;

	for(n = 0; n < count; n++)
	{
		if(FT_Load_Char(face, PGE_FONT_CHARSET[n], FT_LOAD_RENDER))
		{
			pgeFontUnload(font);
			return NULL;
		}

		gi[n].c = PGE_FONT_CHARSET[n];
		gi[n].glyph.x = 0;
		gi[n].glyph.y = 0;
        	gi[n].glyph.width = slot->bitmap.width;
        	gi[n].glyph.height = slot->bitmap.rows;
        	gi[n].glyph.top = slot->bitmap_top;
        	gi[n].glyph.left = slot->bitmap_left;
        	gi[n].glyph.advance = slot->advance.x>>6;
        	gi[n].glyph.unused = 0;

        	gp = glyphList.next;

		while((gp != &glyphList) && (gp->glyph.height > gi[n].glyph.height))
		{
			gp = gp->next;
		}

        	gi[n].next = gp;
        	gi[n].prev = gp->prev;
        	gi[n].next->prev = gi;
        	gi[n].prev->next = gi;
	}

    	int x = 0;
    	int y = 0;
    	int ynext = 0;
    	int used = 0;

    	count = 0;
    	memset(font->map, 255, 256);

    	font->texture = (unsigned char*) malloc(textureSize * textureSize>>1);

    	if(!font->texture)
    	{
		pgeFontUnload(font);
		return NULL;
	}

    	memset(font->texture, 0, textureSize * textureSize>>1);

    	for(n = 0; n < charCount; n++)
	{
		if(FT_Load_Char(face, gi[n].c, FT_LOAD_RENDER))
		{
			pgeFontUnload(font);
			return NULL;
		}

		if((x + gi[n].glyph.width) > textureSize)
		{
			y += ynext;
			x = 0;
		}

		if(gi[n].glyph.height > ynext)
			ynext = gi[n].glyph.height;

		if((y + ynext) > textureSize)
		{
			pgeFontUnload(font);
			return NULL;
		}

		font->map[gi[n].c] = count++;
		gi[n].glyph.x = x;
		gi[n].glyph.y = y;

        	for(yy = 0; yy < gi[n].glyph.height; yy++)
		{
			xx = 0;
			if (x & 1)
			{
				font->texture[(x + (y + yy) * textureSize)>>1] |= (slot->bitmap.buffer[yy * slot->bitmap.width] & 0xF0);
				xx++;
			}
			for(; xx < gi[n].glyph.width; xx += 2)
			{
				if (xx + 1 < gi[n].glyph.width)
					font->texture[((x + xx) + (y + yy) * textureSize)>>1] = (slot->bitmap.buffer[yy * slot->bitmap.width + xx] >> 4) | (slot->bitmap.buffer[yy * slot->bitmap.width + xx + 1] & 0xF0);
				else
					font->texture[((x + xx) + (y + yy) * textureSize)>>1] = (slot->bitmap.buffer[yy * slot->bitmap.width + xx] >> 4);
			}
		}

		x += gi[n].glyph.width;

		used += (gi[n].glyph.width * gi[n].glyph.height);
	}

	font->texHeight = (y + ynext + 7)&~7;

	if (font->texHeight > font->texSize)
		font->texHeight = font->texSize;

    	for(n = 0; n < 256; n++)
	{
		if(font->map[n] == 255)
			font->map[n] = font->map[' '];
	}

    	for(n = 0; n < charCount; n++)
	{
		memcpy(&font->glyph[n], &gi[n].glyph, sizeof(gi[n].glyph));
	}

	FT_Done_Face(face);
	FT_Done_FreeType(library);

	sceKernelDcacheWritebackAll();

	if(!pgeFontSwizzle(font))
	{
		pgeFontUnload(font);
		return NULL;
	}

	sceKernelDcacheWritebackAll();

	return font;
}

void pgeFontUnload(pgeFont *font)
{
	if(font->texture)
		free(font->texture);

	if(font)
		free(font);
}

int pgeFontInit(void)
{
	int n;

	for(n = 0; n < 16; n++)
		clut[n] = ((n * 17) << 24) | 0xffffff;

	return 1;
}

void pgeFontShutdown(void)
{
	//Nothing yet
}

void pgeFontActivate(pgeFont *font)
{
	if(!font)
		return;

	sceGuClutMode(GU_PSM_8888, 0, 255, 0);
	sceGuClutLoad(2, clut);

	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMode(GU_PSM_T4, 0, 0, 1);
	sceGuTexImage(0, font->texSize, font->texSize, font->texSize, font->texture);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	sceGuTexEnvColor(0x0);
	sceGuTexOffset(0.0f, 0.0f);
	sceGuTexWrap(GU_REPEAT, GU_REPEAT);
	sceGuTexFilter(GU_NEAREST, GU_NEAREST);
}

int pgeFontPrintf(pgeFont *font, float x, float y, unsigned int color, const char *text, ...)
{
	if(!font)
		return 0;

	char buffer[256];
	va_list ap;

	va_start(ap, text);
	vsnprintf(buffer, 256, text, ap);
	va_end(ap);
	buffer[255] = 0;

	return pgeFontPrint(font, x, y, color, buffer);
}

int pgeFontPrint(pgeFont *font, float x, float y, unsigned int color, const char *text)
{
	if(!font)
		return 0;

	int i, length;

	typedef struct
	{
		unsigned short u, v;
		//unsigned int c;
		short x, y, z;
	} fontVertex;

	fontVertex *v, *v0, *v1;

	if((length = strlen(text)) == 0)
		return 0;

	v = sceGuGetMemory((sizeof(fontVertex)<<1) * length);

	sceGuColor( color );
	for(i = 0; i < length; i++)
	{
		Glyph *glyph = font->glyph + font->map[text[i] & 0xff];

		v0 = &v[(i<<1) + 0];
		v1 = &v[(i<<1) + 1];

		v0->u = glyph->x;
		v0->v = glyph->y;
		//v0->c = color;
		v0->x = x + glyph->left;
		v0->y = y - glyph->top;
		v0->z = 0;

		v1->u = glyph->x + glyph->width;
		v1->v = glyph->y + glyph->height;
		//v1->c = color;
		v1->x = v0->x + glyph->width;
		v1->y = v0->y + glyph->height;
		v1->z = 0;

		x += glyph->advance;
	}

	sceGuDisable(GU_DEPTH_TEST);
	sceGuDepthMask(GU_TRUE);
	sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT|/*GU_COLOR_8888|*/GU_VERTEX_16BIT|GU_TRANSFORM_2D, length<<1, 0, v);
	sceGuDepthMask(GU_FALSE);
	sceGuEnable(GU_DEPTH_TEST);

	return x;
}

int pgeFontMeasureText(pgeFont *font, const char *text)
{
	if(!font)
		return 0;

	int x = 0;

	while(*text)
	{
		Glyph *glyph = font->glyph + font->map[*text & 0xff];
		x += glyph->advance;
	}

	return x;
}
