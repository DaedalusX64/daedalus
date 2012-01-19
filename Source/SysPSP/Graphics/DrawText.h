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

#ifndef DRAWTEXT_H_
#define DRAWTEXT_H_

#include <vector>
#include "Graphics/ColourValue.h"
#include "SysPSP/UI/UIAlignment.h"

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

		static u32		Render( EFont font, s32 x, s32 y, float scale, const char * p_str, u32 length, c32 colour );
		static u32		Render( EFont font, s32 x, s32 y, float scale, const char * p_str, u32 length, c32 colour, c32 drop_colour );
		static s32		GetTextWidth( EFont font, const char * p_str, u32 length );
		static f32		IntrPrintf( f32 x, f32 y, f32 scale, c32 colour, const char * p_text, ... );
		static const char *	Translate( const char * dest, u32 * length );

		// Versions of above functions which implicitly calc string length
		static u32		Render( EFont font, s32 x, s32 y, float scale, const char * p_str, c32 colour )						{ return Render( font, x, y, scale, p_str, strlen( p_str ), colour ); }
		static u32		Render( EFont font, s32 x, s32 y, float scale, const char * p_str, c32 colour, c32 drop_colour )	{ return Render( font, x, y, scale, p_str, strlen( p_str ), colour, drop_colour ); }
		static s32		GetTextWidth( EFont font, const char * p_str )										{ return GetTextWidth( font, p_str, strlen( p_str ) ); }

		static s32		GetFontHeight( EFont font );

		static void		Destroy();
};

namespace DrawTextUtilities
{
	extern const c32	TextWhite;
	extern const c32	TextWhiteDisabled;
	extern const c32	TextBlue;
	extern const c32	TextBlueDisabled;
	extern const c32	TextRed;
	extern const c32	TextRedDisabled;

	void			WrapText( CDrawText::EFont font, s32 width, const char * p_str, u32 length, std::vector<u32> & lengths, bool & match );
	//inline void		WrapText( CDrawText::EFont font, s32 width, const char * p_str, std::vector<u32> & lengths )			{ WrapText( font, width, p_str, strlen( p_str ), lengths ); }
}

#endif	// DRAWTEXT_H_
