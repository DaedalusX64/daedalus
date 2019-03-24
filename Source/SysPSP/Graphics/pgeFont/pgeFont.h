/*
 * pgeFont.h: Header for bitmap fonts
 * This file is part of the "Phoenix Game Engine".
 *
 * Copyright (C) 2007 Phoenix Game Engine
 * Copyright (C) 2007 David Perry <tias_dp@hotmail.com>
 *
 * This work is licensed under the Creative Commons Attribution-Share Alike 3.0 License.
 * See LICENSE for more details.
 *
 */

#ifndef __PGEFONT_H__
#define __PGEFONT_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup pgeFont Font Library
 *  @{
 */

/**
 * A Glyph struct
 *
 * @note This is used internally by ::pgeFont and has no other relevance.
 */
typedef struct Glyph
{
	unsigned short x;
	unsigned short y;
	unsigned char width;
	unsigned char height;
	char left;
	char top;
	char advance;
	unsigned char unused;
} Glyph;

/**
 * A Font struct
 */
typedef struct
{
	unsigned int texSize; /**<  Texture size (power2) */
	unsigned int texHeight; /**<  Texture height (power2) */
	unsigned char *texture; /**<  The bitmap data */
	unsigned char map[256]; /**<  Character map */
	Glyph glyph[256]; /**<  Character glyphs */
} pgeFont;

enum pgeFontSizeType
{
	PGE_FONT_SIZE_PIXELS = 0,
	PGE_FONT_SIZE_POINTS
};

/**
 * Initialise the Font library
 *
 * @returns 1 on success.
 */
int pgeFontInit(void);

/**
 * Shutdown the Font library
 */
void pgeFontShutdown(void);

/**
 * Load a TrueType font.
 *
 * @param filename - Path to the font
 *
 * @param size - Size to set the font to (in points)
 *
 * @param textureSize - Size of the bitmap texture to create (must be power2)
 *
 * @returns A ::pgeFont struct
 */
pgeFont* pgeFontLoad(const char *filename, unsigned int fontSize, enum pgeFontSizeType fontSizeType, unsigned int textureSize);

/**
 * Free the specified font.
 *
 * @param font - A valid ::pgeFont
 */
void pgeFontUnload(pgeFont *font);

/**
 * Activate the specified font.
 *
 * @param font - A valid ::pgeFont
 */
void pgeFontActivate(pgeFont *font);

/**
 * Draw text along the baseline starting at x, y.
 *
 * @param font - A valid ::pgeFont
 *
 * @param x - X position on screen
 *
 * @param y - Y position on screen
 *
 * @param color - Text color
 *
 * @param text - Text to draw
 *
 * @returns The total width of the text drawn.
 */
int pgeFontPrint(pgeFont *font, float x, float y, unsigned int color, const char *text);

/**
 * Draw text along the baseline starting at x, y (with formatting).
 *
 * @param font - A valid ::pgeFont
 *
 * @param x - X position on screen
 *
 * @param y - Y position on screen
 *
 * @param color - Text color
 *
 * @param text - Text to draw
 *
 * @returns The total width of the text drawn.
 */
int pgeFontPrintf(pgeFont *font, float x, float y, unsigned int color, const char *text, ...);

/**
 * Measure a length of text if it were to be drawn
 *
 * @param font - A valid ::pgeFont
 *
 * @param text - Text to measure
 *
 * @returns The total width of the text.
 */
int pgeFontMeasureText(pgeFont *font, const char *text);

/** @} */

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __PGEFONT_H__
