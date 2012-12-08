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

#include "../../stdafx.h"
#include "DrawText.h"

#include "../../Graphics/NativeTexture.h"
#include "../Graphics/intraFont/intraFont.h"

#include "../../Math/Vector2.h"
#include "../../Math/Vector3.h"

#include "SysPSP/Utility/PathsPSP.h"

#include "Utility/Preferences.h"
#include "Utility/Translate.h"

#include <stdarg.h>
#include <pspgu.h>
#include <pspdebug.h>

intraFont *	gFonts[] =
{
	NULL,
	NULL,
};
DAEDALUS_STATIC_ASSERT( ARRAYSIZE( gFonts ) == CDrawText::NUM_FONTS );

//*************************************************************************************
//
//*************************************************************************************
void	CDrawText::Initialise()
{
    intraFontInit();

	gFonts[ F_REGULAR ] = intraFontLoad( "flash0:/font/ltn8.pgf", INTRAFONT_CACHE_ALL | INTRAFONT_STRING_UTF8 );			// Regular/sans-serif
	gFonts[ F_LARGE_BOLD ] = intraFontLoad( "flash0:/font/ltn4.pgf", INTRAFONT_CACHE_ALL | INTRAFONT_STRING_UTF8 );		// Large/sans-serif/bold

	for( u32 i = 0; i < NUM_FONTS; ++i )
	{
		DAEDALUS_ASSERT( gFonts[ i ] != NULL, "Unable to load font (or forgot!)" );
	}
}

//*************************************************************************************
//
//*************************************************************************************
void	CDrawText::Destroy()
{
	for( u32 i = 0; i < NUM_FONTS; ++i )
	{
		intraFontUnload( gFonts[ i ] );
	}
	intraFontShutdown();
}

//*************************************************************************************
//
//*************************************************************************************
const char * CDrawText::Translate( const char * dest, u32 & length )
{
	return Translate_Strings( dest, length );
}

//*************************************************************************************
//
//*************************************************************************************
u32	CDrawText::Render( EFont font, s32 x, s32 y, float scale, const char * p_str, u32 length, c32 colour )
{
	return Render( font, x, y, scale, p_str, length, colour, c32( 0,0,0,160 ) );
}

//*************************************************************************************
//
//*************************************************************************************
u32	CDrawText::Render( EFont font_type, s32 x, s32 y, float scale, const char * p_str, u32 length, c32 colour, c32 drop_colour )
{
	DAEDALUS_ASSERT( font_type >= 0 && font_type < (s32)NUM_FONTS, "Invalid font" );

	intraFont * font( gFonts[ font_type ] );
	if( font )
	{
		sceGuEnable(GU_BLEND);
		sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
		intraFontSetStyle( font, scale, colour.GetColour(), drop_colour.GetColour(), INTRAFONT_ALIGN_LEFT );
		return s32( intraFontPrintEx( font,  x, y, Translate( p_str, length ), length) ) - x;
	}

	return strlen( p_str ) * 16;		// Guess. Better off just returning 0?
}

//*************************************************************************************
//
//*************************************************************************************
s32		CDrawText::GetTextWidth( EFont font_type, const char * p_str, u32 length )
{
	DAEDALUS_ASSERT( font_type >= 0 && font_type < (s32)NUM_FONTS, "Invalid font" );
	intraFont * font( gFonts[ font_type ] );
	if( font )
	{
		intraFontSetStyle( font, 1.0f, 0xffffffff, 0xffffffff, INTRAFONT_ALIGN_LEFT );
		return s32( intraFontMeasureTextEx( font, Translate( p_str, length ), length ) );
	}

	return strlen( p_str ) * 16;		// Return a reasonable value. Better off just returning 0?
}

//*************************************************************************************
//
//*************************************************************************************
s32		CDrawText::GetFontHeight( EFont font_type )
{
	DAEDALUS_ASSERT( font_type >= 0 && font_type < (s32)NUM_FONTS, "Invalid font" );

	intraFont * font( gFonts[ font_type ] );
	if( font )
	{
		s32		pixels( ( s32( font->advancey ) + 3 ) / 4 );
		return pixels;
	}

	return 16;		// Return a reasonable value. Better off just returning 0?
}

//*************************************************************************************
//
//*************************************************************************************
namespace DrawTextUtilities
{
	const c32	TextWhite			= c32( 255, 255, 255 );
	const c32	TextWhiteDisabled	= c32( 208, 208, 208 );
	const c32	TextBlue			= c32(  80,  80, 208 );
	const c32	TextBlueDisabled	= c32(  80,  80, 178 );
	const c32	TextRed				= c32( 255, 0, 0 );
	const c32	TextRedDisabled		= c32( 208, 208, 208 );

	static c32 COLOUR_SHADOW_HEAVY = c32( 0x80000000 );
	static c32 COLOUR_SHADOW_LIGHT = c32( 0x50000000 );


	const char *	FindPreviousSpace( const char * p_str_start, const char * p_str_end )
	{
		while( p_str_end > p_str_start )
		{
			if( *p_str_end == ' ' )
			{
				return p_str_end;
			}
			p_str_end--;
		}

		// Not found
		return NULL;
	}

	void	WrapText( CDrawText::EFont font, s32 width, const char * p_str, u32 length, std::vector<u32> & lengths, bool & match )
	{
		lengths.clear();

		// Manual line breaking (Used for translations)
		if(gGlobalPreferences.Language != 0)
		{
			u32 i, j;
			for (i = 0, j = 0; i < length; i++)
			{
				match = true;
				if (p_str[i] == '\n')
				{
					j++;
					lengths.push_back( match );
				}
			}
			if( match )
			{
				lengths.push_back( match );
			}

			return;
		}

		// Auto-linebreaking
		const char *	p_line_str( p_str );
		const char *	p_str_end( p_str + length );

		while( p_line_str < p_str_end )
		{
			u32		length_remaining( p_str_end - p_line_str );
			s32		chunk_width( CDrawText::GetTextWidth( font, p_line_str, length_remaining ) );

			if( chunk_width <= width )
			{
				lengths.push_back( length_remaining );
				p_line_str += length_remaining;
			}
			else
			{
				// Search backwards until we find a break
				const char *	p_chunk_end( p_str_end );
				bool			found_chunk( false );
				while( p_chunk_end > p_line_str )
				{
					const char * p_space( FindPreviousSpace( p_line_str, p_chunk_end ) );

					if( p_space != NULL )
					{
						u32		chunk_length( p_space + 1 - p_line_str );
						chunk_width = CDrawText::GetTextWidth( font, p_line_str, chunk_length );
						if( chunk_width <= width )
						{
							lengths.push_back( chunk_length );
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
						lengths.push_back( p_chunk_end - p_line_str );
						p_line_str = p_chunk_end;
						found_chunk = true;
						break;
					}
				}

				DAEDALUS_ASSERT( found_chunk, "Didn't find chunk while splitting string for rendering?" );
			}
		}
	}

}
