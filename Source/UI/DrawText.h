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

#ifndef SYSPSP_GRAPHICS_DRAWTEXT_H_
#define SYSPSP_GRAPHICS_DRAWTEXT_H_

#include <vector>
#include <string>
#include "Graphics/ColourValue.h"
#include "UI/UIAlignment.h"

class CDrawText
{
	public:
		static void		Initialise();

		enum EFont
		{
			F_REGULAR = 0,
			F_LARGE_BOLD,
		};
		static const u32 NUM_FONTS = F_LARGE_BOLD+1;

		static u32		Render( EFont font, s32 x, s32 y, float scale, const std::string p_str, u32 length, c32 colour );
		static u32		Render( EFont font, s32 x, s32 y, float scale, const std::string p_str, u32 length, c32 colour, c32 drop_colour );
		static s32		GetTextWidth( EFont font, const std::string p_str, u32 length );

		// Versions of above functions which implicitly calc string length
		static u32		Render( EFont font, s32 x, s32 y, float scale, const std::string p_str, c32 colour )						{ return Render( font, x, y, scale, p_str, p_str.length(), colour ); }
		static u32		Render( EFont font, s32 x, s32 y, float scale, const std::string p_str, c32 colour, c32 drop_colour )	{ return Render( font, x, y, scale, p_str, p_str.length(), colour, drop_colour ); }
		static s32		GetTextWidth( EFont font, const std::string p_str )										{ return GetTextWidth( font, p_str, p_str.length() ); }

		static s32		GetFontHeight( EFont font );

		static void		Destroy();
};

#endif // SYSPSP_GRAPHICS_DRAWTEXT_H_
