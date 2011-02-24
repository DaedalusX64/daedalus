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


#ifndef UICONTEXT_H_
#define UICONTEXT_H_

#include "Graphics/ColourValue.h"
#include "UIAlignment.h"
#include "Utility/Preferences.h"

class CNativeTexture;
class v2;

class CUIContext
{
	public://
		virtual ~CUIContext();

		static CUIContext *			Create();

		virtual void				BeginRender() = 0;
		virtual void				EndRender() = 0;

		virtual c32					GetBackgroundColour() const = 0;
		virtual void				SetBackgroundColour( c32 colour ) = 0;

		virtual u32					GetScreenWidth() const = 0;
		virtual u32					GetScreenHeight() const = 0;

		virtual c32					GetDefaultTextColour() const = 0;
		virtual c32					GetSelectedTextColour() const = 0;

		virtual void				Update( float elapsed_time ) = 0;

		virtual void				RenderTexture( const CNativeTexture * texture, const v2 & tl, const v2 & wh, c32 colour ) = 0;
		virtual void				RenderTexture( const CNativeTexture * texture, s32 x, s32 y, c32 colour ) = 0;
				//void				ClearBackground()		{ ClearBackground( c32::Black ); }
		virtual void				ClearBackground( c32 colour ) = 0;
		virtual void				DrawRect( s32 x, s32 y, u32 w, u32 h, c32 colour ) = 0;
		virtual void				DrawLine( s32 x0, s32 y0, s32 x1, s32 y1, c32 colour ) = 0;

		// ToDo : Move this out of here..
		void ClearBackground()
		{ 
			c32		BACKGROUND_COLOUR = c32::Black;	// default color

			switch( gGlobalPreferences.GuiColor )
			{
			case BLACK:		BACKGROUND_COLOUR = c32::Black;		break;
			case RED:		BACKGROUND_COLOUR = c32::Red;		break;
			case GREEN:		BACKGROUND_COLOUR = c32::Green;		break;
			case MAGENTA:	BACKGROUND_COLOUR = c32::Magenta;	break;
			case BLUE:		BACKGROUND_COLOUR = c32::Blue;		break;
			case TURQUOISE:	BACKGROUND_COLOUR = c32::Turquoise;	break;
			case ORANGE:	BACKGROUND_COLOUR = c32::Orange;	break;
			case PURPLE:	BACKGROUND_COLOUR = c32::Purple;	break;
			case GREY:		BACKGROUND_COLOUR = c32::Grey;		break;
			}
			ClearBackground( BACKGROUND_COLOUR ); 
		}


		enum EFontStyle
		{
			FS_REGULAR = 0,
			FS_HEADING,
		};

		virtual void				SetFontStyle( EFontStyle font_style ) = 0;

				u32					DrawText( s32 x, s32 y, const char * text, c32 colour )																{ return DrawText( x, y, text, strlen( text ), colour ); }
				u32					DrawText( s32 x, s32 y, const char * text, c32 colour, c32 drop_colour )											{ return DrawText( x, y, text, strlen( text ), colour, drop_colour ); }
				u32					DrawTextScale( s32 x, s32 y, float scale, const char * text, c32 colour )											{ return DrawTextScale( x, y, scale,text, strlen( text ), colour ); }
				u32					DrawTextScale( s32 x, s32 y, float scale, const char * text, c32 colour, c32 drop_colour )							{ return DrawTextScale( x, y, scale,text, strlen( text ), colour, drop_colour ); }
				u32					DrawTextAlign( s32 min_x, s32 max_x, EAlignType align_type, s32 y, const char * text, c32 colour )					{ return DrawTextAlign( min_x, max_x, align_type, y, text, strlen( text ), colour ); }
				u32					DrawTextAlign( s32 min_x, s32 max_x, EAlignType align_type, s32 y, const char * text, c32 colour, c32 drop_colour ) { return DrawTextAlign( min_x, max_x, align_type, y, text, strlen( text ), colour, drop_colour ); }

		virtual u32					DrawText( s32 x, s32 y, const char * text, u32 length, c32 colour ) = 0;
		virtual u32					DrawText( s32 x, s32 y, const char * text, u32 length, c32 colour, c32 drop_colour ) = 0;
		virtual u32					DrawTextScale( s32 x, s32 y, float scale, const char * text, u32 length, c32 colour ) = 0;
		virtual u32					DrawTextScale( s32 x, s32 y, float scale, const char * text, u32 length, c32 colour, c32 drop_colour ) = 0;
		virtual u32					DrawTextAlign( s32 min_x, s32 max_x, EAlignType align_type, s32 y, const char * text, u32 length, c32 colour ) = 0;
		virtual u32					DrawTextAlign( s32 min_x, s32 max_x, EAlignType align_type, s32 y, const char * text, u32 length, c32 colour, c32 drop_colour ) = 0;

		virtual s32					DrawTextArea( s32 left, s32 top, u32 width, u32 height, const char * text, c32 colour, EVerticalAlign vertical_align ) = 0;

		virtual u32					GetFontHeight() const = 0;
		virtual u32					GetTextWidth( const char * text ) const = 0;
};

#endif	// UICONTEXT_H_
