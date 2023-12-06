/*
Copyright (C) 2006 StrmnNrmn

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
#include "UI/DrawText.h"

#include <stdarg.h>
#include <pspgu.h>
#include <pspdebug.h>

#include "Graphics/NativeTexture.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "intraFont.h"

#include "Base/Macros.h"
#include "Interface/Preferences.h"
#include "Utility/Translate.h"

intraFont *gFonts[] =
	{
		nullptr,
		nullptr,
};
DAEDALUS_STATIC_ASSERT(ARRAYSIZE(gFonts) == CDrawText::NUM_FONTS);

//*************************************************************************************
//
//*************************************************************************************
void CDrawText::Initialise()
{
	intraFontInit();

	gFonts[F_REGULAR] = intraFontLoad("flash0:/font/ltn8.pgf", INTRAFONT_CACHE_ALL | INTRAFONT_STRING_UTF8);	// Regular/sans-serif
	gFonts[F_LARGE_BOLD] = intraFontLoad("flash0:/font/ltn4.pgf", INTRAFONT_CACHE_ALL | INTRAFONT_STRING_UTF8); // Large/sans-serif/bold

#ifdef DAEDALUS_ENABLE_ASSERTS
	for (u32 i = 0; i < NUM_FONTS; ++i)
	{
		DAEDALUS_ASSERT(gFonts[i] != NULL, "Unable to load font (or forgot!)");
	}
#endif
}

//*************************************************************************************
//
//*************************************************************************************
void CDrawText::Destroy()
{
	for (u32 i = 0; i < NUM_FONTS; ++i)
	{
		intraFontUnload(gFonts[i]);
	}
	intraFontShutdown();
}

//*************************************************************************************
//
//*************************************************************************************
u32 CDrawText::Render(EFont font, s32 x, s32 y, float scale, const char *p_str, u32 length, c32 colour)
{
	return Render(font, x, y, scale, p_str, length, colour, c32(0, 0, 0, 160));
}

//*************************************************************************************
//
//*************************************************************************************
u32 CDrawText::Render(EFont font_type, s32 x, s32 y, float scale, const char *p_str, u32 length, c32 colour, c32 drop_colour)
{
#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT(font_type >= 0 && font_type < (s32)NUM_FONTS, "Invalid font");
#endif
	intraFont *font(gFonts[font_type]);
	if (font)
	{
		sceGuEnable(GU_BLEND);
		sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
		intraFontSetStyle(font, scale, colour.GetColour(), drop_colour.GetColour(), 0, INTRAFONT_ALIGN_LEFT);
		return s32(intraFontPrintEx(font, x, y, Translate_Strings(p_str, length), length)) - x;
	}

	return strlen(p_str) * 16; // Guess. Better off just returning 0?
}

//*************************************************************************************
//
//*************************************************************************************
s32 CDrawText::GetTextWidth(EFont font_type, const char *p_str, u32 length)
{
#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT(font_type >= 0 && font_type < (s32)NUM_FONTS, "Invalid font");
#endif
	intraFont *font(gFonts[font_type]);
	if (font)
	{
		intraFontSetStyle(font, 1.0f, 0xffffffff, 0xffffffff, 0, INTRAFONT_ALIGN_LEFT);
		return s32(intraFontMeasureTextEx(font, Translate_Strings(p_str, length), length));
	}

	return strlen(p_str) * 16; // Return a reasonable value. Better off just returning 0?
}

//*************************************************************************************
//
//*************************************************************************************
s32 CDrawText::GetFontHeight(EFont font_type)
{
#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT(font_type >= 0 && font_type < (s32)NUM_FONTS, "Invalid font");
#endif
	intraFont *font(gFonts[font_type]);
	if (font)
	{
		s32 pixels((s32(font->advancey) + 3) / 4);
		return pixels;
	}

	return 0; // Return a reasonable value. Better off just returning 0?
}
