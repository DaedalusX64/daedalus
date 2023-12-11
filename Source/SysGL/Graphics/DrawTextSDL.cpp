#include "SysGL/GL.h"
#include "UI/DrawText.h"

static float scaleX = 1.0f, scaleY = 1.0f;

TTF_Font *gFonts[] =
{
		nullptr,
		nullptr,
};
DAEDALUS_STATIC_ASSERT(ARRAYSIZE(gFonts) == CDrawText::NUM_FONTS);

void CDrawText::Initialise()
{
    gFonts[CDrawText::F_REGULAR] = TTF_OpenFont("Resources/OpenSans.ttf", 12);
    gFonts[CDrawText::F_LARGE_BOLD] = TTF_OpenFont("Resources/OpenSans.ttf", 22);
    TTF_SetFontStyle(gFonts[CDrawText::F_LARGE_BOLD], TTF_STYLE_BOLD);

    if (gFonts[0] == 0)
    {
		printf( "SDL could not open TTF Font! SDL Error: %s\n", SDL_GetError() );
	}

}

void CDrawText::Destroy()
{
    TTF_CloseFont(gFonts[CDrawText::F_REGULAR]);
    TTF_CloseFont(gFonts[CDrawText::F_LARGE_BOLD]);
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
	DAEDALUS_ASSERT(font_type >= 0 && font_type < (s32)NUM_FONTS, "Invalid font");

	TTF_Font *font(gFonts[font_type]);
	if (font && p_str[0] != '\0')
	{
        SDL_Color c {colour.GetR(), colour.GetG(), colour.GetB()};
        SDL_Color dc {drop_colour.GetR(), drop_colour.GetG(), drop_colour.GetB()};
        SDL_Surface *surface = TTF_RenderUTF8_Shaded(font, p_str, c, dc);
        SDL_Texture* Message = SDL_CreateTextureFromSurface(gSdlRenderer, surface);

        SDL_Rect Message_rect; //create a rect
        Message_rect.x = x * scaleX;  //controls the rect's x coordinate 
        Message_rect.y = y * scaleY; // controls the rect's y coordinte
        Message_rect.w = surface->w * scaleX * scale; // controls the width of the rect
        Message_rect.h = surface->h * scaleY * scale; // controls the height of the rect

        SDL_RenderCopy(gSdlRenderer, Message, NULL, &Message_rect);

        SDL_FreeSurface(surface);

        return Message_rect.w;
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
	TTF_Font *font(gFonts[font_type]);
	if (font)
	{
        int w, h;
        if (p_str[length] == '\0')
            TTF_SizeText(font, p_str, &w, &h);
        else
        {
            char buf[128];
            memcpy(buf, p_str, length);
            buf[length] = '\0';
            TTF_SizeText(font, buf, &w, &h);
        }
        return w;
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
	TTF_Font *font(gFonts[font_type]);
	if (font)
	{
        return TTF_FontAscent(font);
    }

	return 0; // Return a reasonable value. Better off just returning 0?
}

void DrawText_SetScale(float X, float Y)
{
    //TODO: we can scale the font size?
    scaleX = X;
    scaleY = Y;
}