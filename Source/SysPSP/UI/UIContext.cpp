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

#include "stdafx.h"
#include "UIContext.h"

#include <psptypes.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>

#include "ColourPulser.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Matrix4x4.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/NativeTexture.h"
#include "SysPSP/Graphics/DrawText.h"

#define GL_TRUE                           1
#define GL_FALSE                          0

namespace
{
const u32				BACKGROUND_WIDTH = 480;
const u32				BACKGROUND_HEIGHT = 272;

struct BackgroundVtx
{
    v2	t0;
    v3	pos;

	static const u32 Flags = GU_TEXTURE_32BITF|GU_VERTEX_32BITF;
};

DAEDALUS_STATIC_ASSERT( sizeof(BackgroundVtx) == 20 );

struct BackgroundColourVtx
{
	c32		colour;
    v3		pos;

	static const u32 Flags = GU_COLOR_8888|GU_VERTEX_32BITF;
};

DAEDALUS_STATIC_ASSERT( sizeof(BackgroundColourVtx) == 16 );

struct BackgroundTextureVtx
{
    v2	t0;
	c32		colour;
    v3		pos;

	static const u32 Flags = GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF;
};

DAEDALUS_STATIC_ASSERT( sizeof(BackgroundTextureVtx) == 24 );


const u32		MS_PER_COLOUR_CYCLE = 1200;

const c32		COLOUR_SELECTION_DIM	= c32( 0xe0, 0xe0, 0x80 );
const c32		COLOUR_SELECTION_BRIGHT	= c32( 0xff, 0xff, 0x80 );
}

//*************************************************************************************
//
//*************************************************************************************
class IUIContext : public CUIContext
{
	public:

		IUIContext();
		~IUIContext();

		virtual void				BeginRender();
		virtual void				EndRender();

		virtual c32					GetBackgroundColour() const			{ return mBackgroundColour; }
		virtual void				SetBackgroundColour( c32 colour )	{ mBackgroundColour = colour; }

		virtual u32					GetScreenWidth() const				{ return BACKGROUND_WIDTH; }
		virtual u32					GetScreenHeight() const				{ return BACKGROUND_HEIGHT; }

		virtual c32					GetDefaultTextColour() const		{ return DrawTextUtilities::TextWhite; }
		virtual c32					GetSelectedTextColour() const		{ return mColourPulser.GetCurrentColour(); }

		virtual void				Update( float elapsed_time );

		virtual void				RenderTexture( const CNativeTexture * texture, const v2 & tl, const v2 & wh, c32 colour );
		virtual void				RenderTexture( const CNativeTexture * texture, s32 x, s32 y, c32 colour );
		virtual void				ClearBackground( c32 colour );
		virtual void				DrawRect( s32 x, s32 y, u32 w, u32 h, c32 colour );
		virtual void				DrawLine( s32 x0, s32 y0, s32 x1, s32 y1, c32 colour );

		virtual void				SetFontStyle( EFontStyle font_style );

		virtual u32					DrawText( s32 x, s32 y, const char * text, u32 lenght, c32 colour );
		virtual u32					DrawText( s32 x, s32 y, const char * text, u32 lenght, c32 colour, c32 drop_colour );
		virtual u32					DrawTextScale( s32 x, s32 y, float scale, const char * text, u32 lenght, c32 colour );
		virtual u32					DrawTextScale( s32 x, s32 y, float scale, const char * text, u32 lenght, c32 colour, c32 drop_colour );
		virtual u32					DrawTextAlign( s32 min_x, s32 max_x, EAlignType align_type, s32 y, const char * text, u32 length, c32 colour );
		virtual u32					DrawTextAlign( s32 min_x, s32 max_x, EAlignType align_type, s32 y, const char * text, u32 length, c32 colour, c32 drop_colour );
		virtual s32					DrawTextArea( s32 left, s32 top, u32 width, u32 height, const char * text, c32 colour, EVerticalAlign vertical_align );

		virtual u32					GetFontHeight() const;
		virtual u32					GetTextWidth( const char * text ) const;

	private:
				s32					AlignText( s32 min_x, s32 max_x, const char * p_str, u32 length, EAlignType align_type );

	private:
		CDrawText::EFont			mCurrentFont;
		CColourPulser				mColourPulser;
		c32							mBackgroundColour;
};

//*************************************************************************************
//
//*************************************************************************************
CUIContext::~CUIContext()
{
}

//*************************************************************************************
//
//*************************************************************************************
CUIContext *	CUIContext::Create()
{
	return new IUIContext;
}

//*************************************************************************************
//
//*************************************************************************************
IUIContext::IUIContext()
:	mCurrentFont( CDrawText::F_REGULAR )
,	mColourPulser( COLOUR_SELECTION_DIM, COLOUR_SELECTION_BRIGHT, MS_PER_COLOUR_CYCLE )
,	mBackgroundColour( 0,0,0 )
{
}

//*************************************************************************************
//
//*************************************************************************************
IUIContext::~IUIContext()
{
	// Clear everything to black
	for( u32 i = 0; i < 2; ++i )
	{
		CGraphicsContext::Get()->BeginFrame();
		CGraphicsContext::Get()->Clear( true, true );
		CGraphicsContext::Get()->EndFrame();
		CGraphicsContext::Get()->UpdateFrame( false );
	}
}

//*************************************************************************************
//
//*************************************************************************************
void	IUIContext::Update( float elapsed_time )
{
	s32		elapsed_ms = s32( 1000.0f * elapsed_time );
	mColourPulser.Update( elapsed_ms );
}

//*************************************************************************************
//
//*************************************************************************************
void	IUIContext::RenderTexture( const CNativeTexture * texture, s32 x, s32 y, c32 colour )
{
	if(texture == NULL)
		return;

	v2		tl = v2( f32( x ), f32( y ) );
	v2		wh = v2( f32( texture->GetWidth() ), f32( texture->GetHeight() ) );

	RenderTexture( texture, tl, wh, colour );
}

//*************************************************************************************
//
//*************************************************************************************
void	IUIContext::RenderTexture( const CNativeTexture * texture, const v2 & tl, const v2 & wh, c32 colour )
{
	if(texture == NULL)
		return;

	u32				num_verts( 2 );
	BackgroundTextureVtx*	p_verts = (BackgroundTextureVtx*)sceGuGetMemory(num_verts*sizeof(BackgroundTextureVtx));

	sceGuDisable(GU_DEPTH_TEST);
	sceGuDepthMask( GL_TRUE );	// GL_TRUE to disable z-writes
	sceGuShadeModel( GU_FLAT );

	sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	sceGuTexScale(1.0f,1.0f);
	sceGuTexOffset(0.0f,0.0f);

	u32		width( texture->GetWidth() );
	u32		height( texture->GetHeight() );

	texture->InstallTexture();

	sceGuDisable(GU_ALPHA_TEST);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuEnable(GU_BLEND);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);

	sceGuSetMatrix( GU_PROJECTION, reinterpret_cast< const ScePspFMatrix4 * >( &gMatrixIdentity ) );
	//sceGumOrtho(0, 480 +1, 272 +1, 0, 0.0f, -1.0f);

	v2		tex_uv0( (float)0, (float)0 );
	v2		tex_uv1( (float)width, (float)height );

	p_verts[0].pos = v3( tl.x, tl.y, 0.0f );
	p_verts[0].t0 = v2( 0.0f, 0.0f );
	p_verts[0].colour = colour;

	p_verts[1].pos = v3( tl.x + wh.x, tl.y + wh.y, 0.0f );
	p_verts[1].t0 = v2( (float)width, (float)height );
	p_verts[1].colour = colour;

	sceGuDrawArray(GU_SPRITES,BackgroundTextureVtx::Flags|GU_TRANSFORM_2D,num_verts,NULL,p_verts);
}


//*************************************************************************************
//
//*************************************************************************************
void	IUIContext::ClearBackground( c32 colour )
{
	CGraphicsContext::Get()->Clear( colour.GetColour(), 0 );
}

//*************************************************************************************
//
//*************************************************************************************
void	IUIContext::DrawRect( s32 x, s32 y, u32 w, u32 h, c32 colour )
{
	u32						num_verts( 2 );
	BackgroundColourVtx*	p_verts = (BackgroundColourVtx*)sceGuGetMemory(num_verts*sizeof(BackgroundColourVtx));

	sceGuDisable(GU_DEPTH_TEST);
	sceGuDepthMask( GL_TRUE );	// GL_TRUE to disable z-writes
	sceGuShadeModel( GU_FLAT );

	sceGuDisable(GU_TEXTURE_2D);

	sceGuDisable(GU_ALPHA_TEST);
	sceGuDisable(GU_BLEND);

	sceGuSetMatrix( GU_PROJECTION, reinterpret_cast< const ScePspFMatrix4 * >( &gMatrixIdentity ) );
	//sceGumOrtho(0, 480 +1, 272 +1, 0, 0.0f, -1.0f);

	p_verts[0].pos = v3( float( x ), float( y ), 0.0f );
	p_verts[0].colour = colour;

	p_verts[1].pos = v3( float( x + w ), float( y + h ), 0.0f );
	p_verts[1].colour = colour;

	sceGuDrawArray(GU_SPRITES,BackgroundColourVtx::Flags|GU_TRANSFORM_2D,num_verts,NULL,p_verts);

}

//*************************************************************************************
//
//*************************************************************************************
void	IUIContext::DrawLine( s32 x0, s32 y0, s32 x1, s32 y1, c32 colour )
{
	u32						num_verts( 2 );
	BackgroundColourVtx*	p_verts = (BackgroundColourVtx*)sceGuGetMemory(num_verts*sizeof(BackgroundColourVtx));

	sceGuDisable(GU_DEPTH_TEST);
	sceGuDepthMask( GL_TRUE );	// GL_TRUE to disable z-writes
	sceGuShadeModel( GU_FLAT );

	sceGuDisable(GU_TEXTURE_2D);

	sceGuDisable(GU_ALPHA_TEST);
	sceGuDisable(GU_BLEND);

	sceGuSetMatrix( GU_PROJECTION, reinterpret_cast< const ScePspFMatrix4 * >( &gMatrixIdentity ) );
	//sceGumOrtho(0, 480 +1, 272 +1, 0, 0.0f, -1.0f);

	p_verts[0].pos = v3( float( x0 ), float( y0 ), 0.0f );
	p_verts[0].colour = colour;

	p_verts[1].pos = v3( float( x1 ), float( y1 ), 0.0f );
	p_verts[1].colour = colour;

	sceGuDrawArray(GU_LINES,BackgroundColourVtx::Flags|GU_TRANSFORM_2D,num_verts,NULL,p_verts);

}

//*************************************************************************************
//
//*************************************************************************************
void	IUIContext::SetFontStyle( EFontStyle font_style )
{
	switch( font_style )
	{
	case FS_REGULAR: mCurrentFont = CDrawText::F_REGULAR;		return;
	case FS_HEADING: mCurrentFont = CDrawText::F_LARGE_BOLD;	return;
	}

	DAEDALUS_ERROR( "Unhandled font style" );
}

//*************************************************************************************
//
//*************************************************************************************
u32		IUIContext::GetFontHeight() const
{
	return CDrawText::GetFontHeight( mCurrentFont );
}

//*************************************************************************************
//
//*************************************************************************************
u32		IUIContext::GetTextWidth( const char * text ) const
{
	return CDrawText::GetTextWidth( mCurrentFont, text );
}

//*************************************************************************************
//
//*************************************************************************************
s32		IUIContext::AlignText( s32 min_x, s32 max_x, const char * p_str, u32 length, EAlignType align_type )
{
	s32		x;

	switch( align_type )
	{
	case AT_LEFT:
		x = min_x;
		break;

	case AT_CENTRE:
		x = min_x + ((max_x - min_x) - CDrawText::GetTextWidth( mCurrentFont, p_str, length )) / 2;
		break;

	case AT_RIGHT:
		x = max_x - CDrawText::GetTextWidth( mCurrentFont, p_str, length );
		break;

	default:
		DAEDALUS_ERROR( "Unhandled alignment type" );
		x = min_x;
		break;
	}

	return x;
}

//*************************************************************************************
//
//*************************************************************************************
u32	IUIContext::DrawText( s32 x, s32 y, const char * text, u32 length, c32 colour )
{
	return CDrawText::Render( mCurrentFont, x, y, 1.0f, text, length, colour );
}

//*************************************************************************************
//
//*************************************************************************************
u32	IUIContext::DrawText( s32 x, s32 y, const char * text, u32 length, c32 colour, c32 drop_colour )
{
	return CDrawText::Render( mCurrentFont, x, y, 1.0f, text, length, colour, drop_colour );
}

//*************************************************************************************
//
//*************************************************************************************
u32	IUIContext::DrawTextScale( s32 x, s32 y, float scale, const char * text, u32 length, c32 colour )
{
	return CDrawText::Render( mCurrentFont, x, y, scale, text, length, colour );
}

//*************************************************************************************
//
//*************************************************************************************
u32	IUIContext::DrawTextScale( s32 x, s32 y, float scale, const char * text, u32 length, c32 colour, c32 drop_colour )
{
	return CDrawText::Render( mCurrentFont, x, y, scale, text, length, colour, drop_colour );
}

//*************************************************************************************
//
//*************************************************************************************
u32	IUIContext::DrawTextAlign( s32 min_x, s32 max_x, EAlignType align_type, s32 y, const char * text, u32 length, c32 colour )
{
	s32 x( AlignText( min_x, max_x, text, length, align_type ) );

	return CDrawText::Render( mCurrentFont, x, y, 1.0f, text, length, colour );
}

//*************************************************************************************
//
//*************************************************************************************
u32	IUIContext::DrawTextAlign( s32 min_x, s32 max_x, EAlignType align_type, s32 y, const char * text, u32 length, c32 colour, c32 drop_colour )
{
	s32 x( AlignText( min_x, max_x, text, length, align_type ) );

	return CDrawText::Render( mCurrentFont, x, y, 1.0f, text, length, colour, drop_colour );
}


namespace
{
	s32		VerticalAlign( EVerticalAlign vertical_align, s32 top, u32 height, u32 text_height )
	{
		switch( vertical_align )
		{
			case VA_TOP:	return top;
			case VA_BOTTOM: return top + height - text_height;
		}

		DAEDALUS_ERROR( "Unhandled vertical align" );
		return top;
	}
}
//*************************************************************************************
//
//*************************************************************************************
s32		IUIContext::DrawTextArea( s32 left, s32 top, u32 width, u32 height, const char * text, c32 colour, EVerticalAlign vertical_align )
{
	const u32			font_height( CDrawText::GetFontHeight( mCurrentFont ) );
	std::vector<u32>	lengths;
	DrawTextUtilities::WrapText( mCurrentFont, width, text, strlen( text ), lengths );

	s32 x( left );
	s32 y( VerticalAlign( vertical_align, top, height, lengths.size() * font_height ) );

	for( u32 i = 0; i < lengths.size(); ++i )
	{
		y += font_height;
		DrawTextScale( x, y, 0.8f, text, lengths[ i ], colour );
		y += 2;
		text += lengths[ i ];
	}

	return y - top;
}

//*************************************************************************************
//
//*************************************************************************************
void	IUIContext::BeginRender()
{
	CGraphicsContext::Get()->BeginFrame();
}

//*************************************************************************************
//
//*************************************************************************************
void	IUIContext::EndRender()
{
	CGraphicsContext::Get()->EndFrame();
	CGraphicsContext::Get()->UpdateFrame( true );
}
