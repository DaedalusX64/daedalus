#ifndef DRAWTEXT_UTILIEIES_H_
#define DRAWTEXT_UTILIEIES_H_

#include <string.h>

#include <vector>

#include "Graphics/ColourValue.h"
#include "UI/UIAlignment.h"
#include "UI/DrawText.h"

namespace DrawTextUtilities
{
	extern const c32	TextWhite;
	extern const c32	TextWhiteDisabled;
	extern const c32	TextBlue;
	extern const c32	TextBlueDisabled;
	extern const c32	TextRed;
	extern const c32	TextRedDisabled;

	void WrapText(CDrawText::EFont font, s32 width, const char *p_str, u32 length, std::vector<u32> &lengths, bool &match);
}

#endif