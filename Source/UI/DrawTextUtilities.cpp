#include "Base/Types.h"
#include "Dialogs.h"
#include "UIContext.h"

#include "Graphics/ColourValue.h"
#include "DrawTextUtilities.h"
#include "DrawText.h"

//*************************************************************************************
//
//*************************************************************************************
namespace DrawTextUtilities
{
	const c32 TextWhite = c32(255, 255, 255);
	const c32 TextWhiteDisabled = c32(208, 208, 208);
	const c32 TextBlue = c32(80, 80, 208);
	const c32 TextBlueDisabled = c32(80, 80, 178);
	const c32 TextRed = c32(255, 0, 0);
	const c32 TextRedDisabled = c32(208, 208, 208);

	static c32 COLOUR_SHADOW_HEAVY = c32(0x80000000);
	static c32 COLOUR_SHADOW_LIGHT = c32(0x50000000);

	static const char *FindPreviousSpace(const char *p_str_start, const char *p_str_end)
	{
		while (p_str_end > p_str_start)
		{
			if (*p_str_end == ' ')
			{
				return p_str_end;
			}
			p_str_end--;
		}

		// Not found
		return nullptr;
	}

	void WrapText(CDrawText::EFont font, s32 width, const char *p_str, u32 length, std::vector<u32> &lengths, bool &match)
	{
		lengths.clear();

		// Manual line breaking (Used for translations)
		if (gGlobalPreferences.Language != 0)
		{
			u32 i, j;
			for (i = 0, j = 0; i < length; i++)
			{
				match = true;
				if (p_str[i] == '\n')
				{
					j++;
					lengths.push_back(match);
				}
			}
			if (match)
			{
				lengths.push_back(match);
			}

			return;
		}

		// Auto-linebreaking
		const char *p_line_str(p_str);
		const char *p_str_end(p_str + length);

		while (p_line_str < p_str_end)
		{
			u32 length_remaining(p_str_end - p_line_str);
			s32 chunk_width(CDrawText::GetTextWidth(font, p_line_str, length_remaining));

			if (chunk_width <= width)
			{
				lengths.push_back(length_remaining);
				p_line_str += length_remaining;
			}
			else
			{
				// Search backwards until we find a break
				const char *p_chunk_end(p_str_end);
				bool found_chunk(false);
				while (p_chunk_end > p_line_str)
				{
					const char *p_space(FindPreviousSpace(p_line_str, p_chunk_end));

					if (p_space != nullptr)
					{
						u32 chunk_length(p_space + 1 - p_line_str);
						chunk_width = CDrawText::GetTextWidth(font, p_line_str, chunk_length);
						if (chunk_width <= width)
						{
							lengths.push_back(chunk_length);
							p_line_str += chunk_length;
							found_chunk = true;
							break;
						}
						else
						{
							// Need to try again with the previous space
							p_chunk_end = p_space - 1;
						}
					}
					else
					{
						// No more spaces - just render the whole chunk
						lengths.push_back(p_chunk_end - p_line_str);
						p_line_str = p_chunk_end;
						found_chunk = true;
						break;
					}
				}
#ifdef DAEDALUS_ENABLE_ASSERTS
				DAEDALUS_ASSERT(found_chunk, "Didn't find chunk while splitting string for rendering?");
#endif
			}
		}
	}

} // namespace DrawTextUtilities
