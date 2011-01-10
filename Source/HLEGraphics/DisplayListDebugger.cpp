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
#include "DisplayListDebugger.h"

#ifdef DAEDALUS_DEBUG_DISPLAYLIST

#include "PSPRenderer.h"
#include "DLParser.h"
#include "TextureCache.h"
#include "TextureDescriptor.h"
#include "Microcode.h"

#include "Graphics/GraphicsContext.h"
#include "Graphics/NativeTexture.h"

#include "Combiner/RenderSettings.h"

#include "Core/ROM.h"
#include "Debug/Dump.h"

#include "Utility/Timer.h"
#include "Utility/Preferences.h"
#include "Utility/Timing.h"
#include "Utility/IO.h"

#include <set>
#include <vector>
#include <algorithm>

#include "Math/Math.h"	// VFPU Math
#include "Math/MathUtil.h"

#include <pspctrl.h>
#include <pspgu.h>

using std::sort;

//*****************************************************************************
//
//*****************************************************************************
extern float	TEST_VARX, TEST_VARY;
extern void		PrintMux( FILE * fh, u64 mux );
//*****************************************************************************
//
//*****************************************************************************
extern u32  gTexInstall;
extern u32	gSetRGB;
extern u32	gSetA;
extern u32	gSetRGBA;
extern u32	gModA;
extern u32	gAOpaque;

extern u32	gsceENV;

extern u32	gTXTFUNC;

extern u32	gNumCyc;

extern u32	gForceRGB;

extern const char *gForceColor[8];
extern const char *gPSPtxtFunc[10];
extern const char *gCAdj[4];
//*****************************************************************************
//
//*****************************************************************************
namespace
{
	//	const char * const TERMINAL_TOP_LEFT			= "\033[2A\033[2K";
		const char * const TERMINAL_TOP_LEFT			= "\033[H";
		const char * const TERMINAL_CLEAR_SCREEN		= "\033[2J";
		const char * const TERMINAL_CLEAR_LINE			= "\033[2K";
		const char * const TERMINAL_SAVE_POS			= "\033[2s";
		const char * const TERMINAL_RESTORE_POS			= "\033[2u";

		const char * const TERMINAL_YELLOW				= "\033[1;33m";
		const char * const TERMINAL_GREEN				= "\033[1;32m";
		const char * const TERMINAL_MAGENTA				= "\033[1;35m";
		const char * const TERMINAL_WHITE				= "\033[1;37m";

const char * const gDDLOText[] = 
{
	"Combiner Explorer",	// DDLO_COMBINER_EXPLORER
	"Display List Length",	// DDLO_DLIST_LENGTH
	"Decal Offset",			// DDLO_DECAL_OFFSET
	"Texture Viewer",		// DDLO_TEXTURE_VIEWER
	"Dump Textures",		// DDLO_DUMP_TEXTURES
	"Dump Dlist",			// DDLO_DUMP_DLIST
};

struct SPspPadState
{
	v2		Stick;
	u32		OldButtons;
	u32		NewButtons;
};

//*****************************************************************************
//
//*****************************************************************************
class CDebugMenuOption 
{
	public:
				CDebugMenuOption();
		virtual ~CDebugMenuOption() {}

		virtual void			Enter()											{}
		virtual void			Exit()											{}
		virtual void			Update( const SPspPadState & pad_state, float elapsed_time ) = 0;
		
		virtual bool			OverrideDisplay() const							{ return false; }

				bool			NeedsUpdateDisplay() const						{ return mRefreshDisplay; }
				void			UpdateDisplay();

		virtual const char *	GetDescription() const = 0;

	protected:
				void			InvalidateDisplay()								{ mRefreshDisplay = true; }
		virtual void			Display() const = 0;

	private:
				bool			mRefreshDisplay;
};

CDebugMenuOption::CDebugMenuOption()
:	mRefreshDisplay( true )
{
}

void	CDebugMenuOption::UpdateDisplay()
{
	Display();
	mRefreshDisplay = false;
}

//*****************************************************************************
//
//*****************************************************************************
class CCombinerExplorerDebugMenuOption : public CDebugMenuOption
{
	public:	
		CCombinerExplorerDebugMenuOption();

		virtual void			Display() const;
		virtual void			Update( const SPspPadState & pad_state, float elapsed_time );
		virtual const char *	GetDescription() const									{ return "Combiner Explorer"; }

	private:
				u32				mSelectedIdx;
};

CCombinerExplorerDebugMenuOption::CCombinerExplorerDebugMenuOption()
:	mSelectedIdx( 0 )
{
}

void CCombinerExplorerDebugMenuOption::Display() const
{
	const std::set< u64 > & 	combiner_states( PSPRenderer::Get()->GetRecordedCombinerStates() );

	printf( "Combiner States in use:\n" );
	printf( "   Use [] to return\n" );
	printf( "   Use O to toggle on/off:\n" );

	u32		idx( 0 );
	u64		selected_mux( 0 );
	for(std::set<u64>::const_iterator it = combiner_states.begin(); it != combiner_states.end(); ++it)
	{
		u64		state( *it );

		bool	selected( idx == mSelectedIdx );
		bool	disabled( PSPRenderer::Get()->IsCombinerStateDisabled( state ) );
		bool	unhandled( PSPRenderer::Get()->IsCombinerStateUnhandled( state ) );
		const char *	text_col;

		if(selected)
		{
			text_col = TERMINAL_YELLOW;
			selected_mux = state;
		}
		else if(disabled)
		{
			text_col = TERMINAL_GREEN;
		}
		else if(unhandled)
		{
			text_col = TERMINAL_MAGENTA;
		}
		else
		{
			text_col = TERMINAL_WHITE;
		}

		printf( " %s%c%08x%08x\n", text_col, selected ? '*' : ' ', u32(state >> 32), u32(state) );

		idx++;
	}
	printf( "%s\n", TERMINAL_WHITE );

	if( selected_mux != 0 )
	{
		PrintMux( stdout, selected_mux );

		PSPRenderer::SBlendStateEntry entry1( PSPRenderer::Get()->LookupBlendState( selected_mux, false ) );
		if( entry1.OverrideFunction != NULL )
		{
			printf( "1 Cycle: Overridden\n" );
		}
		else if( entry1.States != NULL )
		{
			printf( "1 Cycle:\n" );
			entry1.States->Print();
		}

		PSPRenderer::SBlendStateEntry	entry2( PSPRenderer::Get()->LookupBlendState( selected_mux, true ) );
		if( entry2.OverrideFunction != NULL )
		{
			printf( "2 Cycles: Overridden\n" );
		}
		else if( entry2.States != NULL )
		{
			printf( "2 Cycles:\n" );
			entry2.States->Print();
		}

	}
}

void CCombinerExplorerDebugMenuOption::Update( const SPspPadState & pad_state, float elapsed_time )
{
	const std::set< u64 > & 	combiner_states( PSPRenderer::Get()->GetRecordedCombinerStates() );

	u32		idx( 0 );
	u64		selected_state( 0 );
	for(std::set<u64>::const_iterator it = combiner_states.begin(); it != combiner_states.end(); ++it)
	{
		if(idx == mSelectedIdx)
		{
			selected_state = *it;
		}
		idx++;
	}

	u32		state_count( combiner_states.size() );

	if(pad_state.OldButtons != pad_state.NewButtons)
	{
		if(pad_state.NewButtons & PSP_CTRL_UP)
		{
			mSelectedIdx = (mSelectedIdx > 0) ? mSelectedIdx - 1 : mSelectedIdx;
			InvalidateDisplay();
		}
		if(pad_state.NewButtons & PSP_CTRL_DOWN)
		{
			mSelectedIdx = (mSelectedIdx < state_count-1) ? mSelectedIdx + 1 : mSelectedIdx;
			InvalidateDisplay();
		}

		if(pad_state.NewButtons & PSP_CTRL_CIRCLE)
		{
			if(selected_state != 0)
			{
				PSPRenderer::Get()->ToggleDisableCombinerState( selected_state );
				InvalidateDisplay();
			}
		}
	}
}

//*****************************************************************************
//
//*****************************************************************************
class CBlendDebugMenuOption : public CDebugMenuOption
{
	public:	
		CBlendDebugMenuOption();
		virtual void			Display() const;
		virtual void			Update( const SPspPadState & pad_state, float elapsed_time );
		virtual const char *	GetDescription() const									{ return "Blender Explorer"; }

	private:
		u32				mIdx;
		u32				mSel;
		bool			modify;

};

CBlendDebugMenuOption::CBlendDebugMenuOption()
:	mIdx( 0 )
,	mSel( 0 )
,	modify( false )
{
}

void CBlendDebugMenuOption::Display() const
{
	if( mSel == 0 && modify ) gTexInstall = mIdx & 1;
	if( mSel == 1 && modify ) gSetRGB = mIdx & 3;
	if( mSel == 2 && modify ) gSetA = mIdx & 3;
	if( mSel == 3 && modify ) gSetRGBA = mIdx & 3;
	if( mSel == 4 && modify ) gModA = mIdx & 3;
	if( mSel == 5 && modify ) gAOpaque = mIdx & 1;
	if( mSel == 6 && modify ) gsceENV = mIdx % 3;
	if( mSel == 7 && modify ) gTXTFUNC = mIdx % 10;
	if( mSel == 8 && modify ) gNumCyc = (mIdx % 3) + 1;
	if( mSel == 9 && modify ) gForceRGB = mIdx % 8;
	

	printf( "Blender Explorer\n");
	printf( "   Use [] to return\n" );
	printf( "   Use X to modify\n" );
	printf( "   Use up/down to choose & left/right to adjust\n\n\n" );

	printf( " Blending Options (Color Adjuster)\n" );
	
	printf( "   %s%cTextureEnabled: %s\n",(mSel==0 && modify) ? TERMINAL_GREEN : TERMINAL_WHITE, mSel==0 ? '*' : ' ', gTexInstall ? "ON" : "OFF");
	printf( "   %s%cSetRGB: %s\n",		  (mSel==1 && modify) ? TERMINAL_GREEN : TERMINAL_WHITE, mSel==1 ? '*' : ' ', gCAdj[gSetRGB]);
	printf( "   %s%cSetA: %s\n",		  (mSel==2 && modify) ? TERMINAL_GREEN : TERMINAL_WHITE, mSel==2 ? '*' : ' ', gCAdj[gSetA]);
	printf( "   %s%cSetRGBA: %s\n",		  (mSel==3 && modify) ? TERMINAL_GREEN : TERMINAL_WHITE, mSel==3 ? '*' : ' ', gCAdj[gSetRGBA]);
	printf( "   %s%cModifyA: %s\n",		  (mSel==4 && modify) ? TERMINAL_GREEN : TERMINAL_WHITE, mSel==4 ? '*' : ' ', gCAdj[gModA]);
	printf( "   %s%cSetAOpaque: %s\n",	  (mSel==5 && modify) ? TERMINAL_GREEN : TERMINAL_WHITE, mSel==5 ? '*' : ' ', gAOpaque ? "ON" : "OFF");
	printf( "%s\n", TERMINAL_WHITE );
	printf( " Environment Color in SDK\n" );
	printf( "   %s%cTexEnvColor: %s\n",   (mSel==6 && modify) ? TERMINAL_GREEN : TERMINAL_WHITE, mSel==6 ? '*' : ' ', gsceENV ? ((gsceENV==1) ? "EnvColor" : "PrimColor") : "OFF");
	printf( "%s\n", TERMINAL_WHITE );
	printf( " PSP Texture Function\n" );
	printf( "   %s%c%s\n",				(mSel==7 && modify) ? TERMINAL_GREEN : TERMINAL_WHITE, mSel==7 ? '*' : ' ', gPSPtxtFunc[gTXTFUNC]);
	printf( "%s\n", TERMINAL_WHITE );
	printf( " Cycle\n" );
	printf( "   %s%c%s\n",				(mSel==8 && modify) ? TERMINAL_GREEN : TERMINAL_WHITE, mSel==8 ? '*' : ' ', gNumCyc==3 ? "ALL" :((gNumCyc==1) ? "1" : "2"));
	printf( "%s\n", TERMINAL_WHITE );
	printf( " Force RGB\n" );
	printf( "   %s%c%s\n",				(mSel==9 && modify) ? TERMINAL_GREEN : TERMINAL_WHITE, mSel==9 ? '*' : ' ', gForceColor[gForceRGB]);
	printf( "%s\n", TERMINAL_WHITE );
}

void CBlendDebugMenuOption::Update( const SPspPadState & pad_state, float elapsed_time )
{

	if(pad_state.OldButtons != pad_state.NewButtons)
	{
		if(pad_state.NewButtons & PSP_CTRL_UP)
		{
			mSel = (mSel > 0) ? mSel - 1 : mSel;
			modify = 0;
		}

		if(pad_state.NewButtons & PSP_CTRL_DOWN)
		{
			mSel = (mSel < 9) ? mSel + 1 : mSel;	//Number of menu rows
			modify = 0;
		}

		if(pad_state.NewButtons & PSP_CTRL_LEFT)
		{
			mIdx = (mIdx > 0) ? mIdx - 1 : mIdx;
		}

		if(pad_state.NewButtons & PSP_CTRL_RIGHT)
		{
			mIdx = (mIdx < 9) ? mIdx + 1 : mIdx;
		}

		if(pad_state.NewButtons & PSP_CTRL_CROSS)
		{
			modify ^= true;
		}
	
		InvalidateDisplay();

	}
}

//*****************************************************************************
//
//*****************************************************************************
class CTextureExplorerDebugMenuOption : public CDebugMenuOption
{
	public:	
		CTextureExplorerDebugMenuOption();

		virtual void			Display() const;
		virtual void			Update( const SPspPadState & pad_state, float elapsed_time );
		virtual const char *	GetDescription() const									{ return "Texture Explorer"; }

		virtual bool			OverrideDisplay() const;

	private:
				u32				mSelectedIdx;
				bool			mDisplayTexture;

				u32				mScaleFactor;
				v2				mTextureOffset;

		CRefPtr<CNativeTexture> mCheckerTexture;

		std::vector<CTextureCache::STextureInfoSnapshot>	mSnapshot;
};

namespace
{
	bool OrderTextures( const CTextureCache::STextureInfoSnapshot & lhs, const CTextureCache::STextureInfoSnapshot & rhs )
	{
	   return lhs.GetTexture()->GetTextureInfo().GetLoadAddress() < rhs.GetTexture()->GetTextureInfo().GetLoadAddress();
	}

#define GL_TRUE                           1
#define GL_FALSE                          0


const u32 gCheckTextureWidth( 16 );
const u32 gCheckTextureHeight( 16 );
u16	gCheckTexture[gCheckTextureWidth * gCheckTextureHeight ] = 
{
	0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
	0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
	0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
	0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
	0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
	0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
	0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
	0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb, 0xfbbb,
};


}

CTextureExplorerDebugMenuOption::CTextureExplorerDebugMenuOption()
:	mSelectedIdx( 0 )
,	mDisplayTexture( false )
,	mScaleFactor( 2 )
,	mTextureOffset( 0,0 )
{
	mCheckerTexture = CNativeTexture::Create( gCheckTextureWidth, gCheckTextureHeight, TexFmt_4444 );

	if( mCheckerTexture != NULL )
	{
		DAEDALUS_ASSERT( mCheckerTexture->GetBytesRequired() == sizeof( gCheckTexture ), "Incorrect size for checker texture" );
		mCheckerTexture->SetData( gCheckTexture, NULL );
	}

	CTextureCache::Get()->Snapshot( mSnapshot );

	sort( mSnapshot.begin(), mSnapshot.end(), OrderTextures );

	// Dump each in turn
	for( u32 i = 0; i < mSnapshot.size(); ++i )
	{
		const CRefPtr<CTexture> &		n64_texture( mSnapshot[ i ].GetTexture() );
		if( n64_texture != NULL )
		{
			n64_texture->DumpTexture();
		}
	}
}

struct TextureVtx
{
    v2	t0;
    v3	pos;
};

DAEDALUS_STATIC_ASSERT( sizeof(TextureVtx) == 20 );

#define TEXTURE_VERTEX_FLAGS (GU_TEXTURE_32BITF|GU_VERTEX_32BITF )


bool CTextureExplorerDebugMenuOption::OverrideDisplay() const
{
	if( !mDisplayTexture )
		return false;

	CRefPtr<CNativeTexture> texture;
	u32		texture_width( 32 );
	u32		texture_height( 32 );
	if( mSelectedIdx < mSnapshot.size() )
	{
		const CRefPtr<CTexture> &	n64_texture( mSnapshot[ mSelectedIdx ].GetTexture() );
		if( n64_texture != NULL )
		{
			const TextureInfo &		info( n64_texture->GetTextureInfo() );

			texture_width = info.GetWidth();
			texture_height = info.GetHeight();

			texture = n64_texture->GetTexture();
		}
	}

	CGraphicsContext::Get()->BeginFrame();

	sceGuDisable(GU_DEPTH_TEST);
	sceGuDepthMask( GL_TRUE );	// GL_TRUE to disable z-writes
	sceGuShadeModel( GU_FLAT );

	sceGuTexFilter(GU_NEAREST,GU_NEAREST);
	sceGuTexScale(1.0f,1.0f);
	sceGuTexOffset(0.0f,0.0f);
	sceGuDisable(GU_ALPHA_TEST);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);

	sceGuSetMatrix( GU_PROJECTION, reinterpret_cast< const ScePspFMatrix4 * >( &gMatrixIdentity ) );

	const f32		screen_width( 480.0f );
	const f32		screen_height( 272.0f );

	if ( mCheckerTexture != NULL )
	{
		u32				num_verts( 2 );
		TextureVtx*	p_verts = (TextureVtx*)sceGuGetMemory(num_verts*sizeof(TextureVtx));

		mCheckerTexture->InstallTexture();

		sceGuDisable(GU_BLEND);
		sceGuTexWrap(GU_REPEAT,GU_REPEAT);

		p_verts[0].pos = v3( 0.0f, 0.0f, 0.0f );
		p_verts[0].t0 = v2( 0.0f, 0.0f );

		p_verts[1].pos = v3( screen_width, screen_height, 0.0f );
		p_verts[1].t0 = v2( screen_width, screen_height );

		sceGuDrawArray(GU_SPRITES,TEXTURE_VERTEX_FLAGS|GU_TRANSFORM_2D,num_verts,NULL,p_verts);
	}

	if( texture != NULL )
	{
		u32		num_verts( 2 );
		TextureVtx*	p_verts = (TextureVtx*)sceGuGetMemory(num_verts*sizeof(TextureVtx));

		texture->InstallTexture();

		f32		display_width( f32( texture_width * mScaleFactor ) );
		f32		display_height( f32( texture_height * mScaleFactor ) );

		f32		left_offset( (screen_width - display_width) / 2.0f - mTextureOffset.x );
		f32		top_offset( (screen_height - display_height) / 2.0f - mTextureOffset.y );

		// Clamp to pixel boundary
		left_offset = f32( s32( left_offset ) );
		top_offset = f32( s32( top_offset ) );

		sceGuBlendFunc( GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
		sceGuEnable(GU_BLEND);

		sceGuTexWrap(GU_CLAMP,GU_CLAMP);

		p_verts[0].pos = v3( left_offset, top_offset, 0.0f );
		p_verts[0].t0 = v2( 0.0f, 0.0f );

		p_verts[1].pos = v3( left_offset + display_width, top_offset + display_height, 0.0f );
		p_verts[1].t0 = v2( (float)texture_width, (float)texture_height );

		sceGuDrawArray(GU_SPRITES,TEXTURE_VERTEX_FLAGS|GU_TRANSFORM_2D,num_verts,NULL,p_verts);
	}
	CGraphicsContext::Get()->EndFrame();

	return true;
}

void CTextureExplorerDebugMenuOption::Display() const
{
	printf( "Textures in use:\n" );
	printf( "   Use [] to return\n" );
	printf( "   Use /\\ to force texture reload\n" );
	printf( "   Use X to toggle display on/off:\n" );

	printf( "\nThere are %d textures\n\n", mSnapshot.size() );

	s32		min_to_show( mSelectedIdx - 16 );
	s32		max_to_show( mSelectedIdx + 16 );

	if( min_to_show < 0 )
	{
		s32	num_spare( 0 - min_to_show );
		max_to_show = Clamp< s32 >( max_to_show + num_spare, 0, mSnapshot.size() - 1 );
		min_to_show = 0;
	}

	if( max_to_show >= s32( mSnapshot.size() ) )
	{
		s32 num_spare( max_to_show - (mSnapshot.size() - 1) );
		min_to_show = Clamp< s32 >( min_to_show - num_spare, 0, mSnapshot.size() - 1 );
		max_to_show = mSnapshot.size() - 1;
	}

	printf( "   #  LoadAddr (x,y -> w x h, p) fmt/size tmem pal\n" );
	for( s32 i = min_to_show; i <= max_to_show; ++i )
	{
		DAEDALUS_ASSERT( i >= 0 && i < s32( mSnapshot.size() ), "Invalid snapshot index" );
		const CTextureCache::STextureInfoSnapshot &		info( mSnapshot[ i ] );
		const TextureInfo &								ti( info.GetTexture()->GetTextureInfo() );

		bool	selected( u32( i ) == mSelectedIdx );
		const char *	text_col;

		if(selected)
		{
			text_col = TERMINAL_YELLOW;
		}
		else
		{
			text_col = TERMINAL_WHITE;
		}

		// XXXX
		u32	left( 0 );
		u32	top( 0 );

		printf( " %s%03d %c%08x (%d,%d -> %dx%d, %d) %s/%dbpp, %04x, %04x\n",
			text_col, i, selected ? '*' : ' ',
			ti.GetLoadAddress(),
			left, top, ti.GetWidth(), ti.GetHeight(), ti.GetPitch(),
			ti.GetFormatName(), ti.GetSizeInBits(),
			ti.GetTmemAddress(), ti.GetTLutIndex() );
	}
	printf( "%s\n", TERMINAL_WHITE );

}

void CTextureExplorerDebugMenuOption::Update( const SPspPadState & pad_state, float elapsed_time )
{
	u32		texture_count( mSnapshot.size() );

	if(pad_state.OldButtons != pad_state.NewButtons)
	{
		if(pad_state.NewButtons & PSP_CTRL_UP)
		{
			mSelectedIdx = (mSelectedIdx > 0) ? mSelectedIdx - 1 : mSelectedIdx;
			InvalidateDisplay();
		}
		if(pad_state.NewButtons & PSP_CTRL_DOWN)
		{
			mSelectedIdx = (mSelectedIdx < texture_count-1) ? mSelectedIdx + 1 : mSelectedIdx;
			InvalidateDisplay();
		}

		if(pad_state.NewButtons & PSP_CTRL_TRIANGLE)
		{
			CTextureCache::Get()->DropTextures();
		}
		if(pad_state.NewButtons & PSP_CTRL_CROSS)
		{
			mDisplayTexture = !mDisplayTexture;
		}
		if(pad_state.NewButtons & PSP_CTRL_LTRIGGER)
		{
			mScaleFactor = mScaleFactor / 2;
			if(mScaleFactor < 1)
			{
				mScaleFactor = 1;
			}
		}
		if(pad_state.NewButtons & PSP_CTRL_RTRIGGER)
		{
			mScaleFactor = mScaleFactor * 2;
			if(mScaleFactor > 16)
			{
				mScaleFactor = 16;
			}
		}

	}

	if(mDisplayTexture)
	{
		const float STICK_ADJUST_PIXELS_PER_SECOND = 256.0f;
		if(pspFpuAbs(pad_state.Stick.x) < 0.001f && pspFpuAbs(pad_state.Stick.y) < 0.001f)
		{
			mTextureOffset *= vfpu_powf( 0.997f, elapsed_time * 1000.0f );
		}
		else
		{
			mTextureOffset += pad_state.Stick * STICK_ADJUST_PIXELS_PER_SECOND * elapsed_time;
		}
	}

}

//*****************************************************************************
//
//*****************************************************************************
class CDisplayListLengthDebugMenuOption : public CDebugMenuOption
{
	public:	
		CDisplayListLengthDebugMenuOption();

		virtual void			Display() const;
		virtual void			Update( const SPspPadState & pad_state, float elapsed_time );
		virtual const char *	GetDescription() const									{ return "Display List Length"; }

		
	private:
				u32				mTotalInstructionCount;
				u32				mInstructionCountLimit;

				float			mFractionalAdjustment;
};

CDisplayListLengthDebugMenuOption::CDisplayListLengthDebugMenuOption()
:	mTotalInstructionCount( 0 )
,	mInstructionCountLimit( UNLIMITED_INSTRUCTION_COUNT )
,	mFractionalAdjustment( 0.0f )
{
}

void CDisplayListLengthDebugMenuOption::Display() const
{
	printf( "Display list length %d / %d:\n", mInstructionCountLimit == UNLIMITED_INSTRUCTION_COUNT ? mTotalInstructionCount : mInstructionCountLimit, mTotalInstructionCount );
	printf( "   Use [] to return\n" );
	printf( "   Use up/down to adjust\n" );
}

void CDisplayListLengthDebugMenuOption::Update( const SPspPadState & pad_state, float elapsed_time )
{
	if( mTotalInstructionCount == 0 )
	{
		mTotalInstructionCount = DLParser_GetTotalInstructionCount();
		mInstructionCountLimit = mTotalInstructionCount;
	}

	float		rate_adjustment( 1.0f );

	if(pad_state.NewButtons & PSP_CTRL_RTRIGGER)
	{
		rate_adjustment = 5.0f;
	}

	float	new_adjustment( 0.0f );

	if(pad_state.OldButtons != pad_state.NewButtons)
	{
		if(pad_state.NewButtons & PSP_CTRL_UP)
		{
			new_adjustment = -1;
		}

		if(pad_state.NewButtons & PSP_CTRL_DOWN)
		{
			new_adjustment = +1;
		}
	}

	const float STICK_ADJUST_PER_SECOND = 100.0f;
	new_adjustment += pad_state.Stick.y * STICK_ADJUST_PER_SECOND * rate_adjustment * elapsed_time;

	mFractionalAdjustment += new_adjustment;

	s32 adjustment = s32( mFractionalAdjustment );
	if( adjustment != 0 )
	{
		s32		new_limit( mInstructionCountLimit + adjustment );

		mInstructionCountLimit = u32( Clamp< s32 >( new_limit, 0, mTotalInstructionCount ) );
		mFractionalAdjustment -= float( adjustment );

		InvalidateDisplay();
	}

	DLParser_SetInstructionCountLimit( mInstructionCountLimit );
}

//*****************************************************************************
//
//*****************************************************************************
class CDecalOffsetDebugMenuOption : public CDebugMenuOption
{
	public:	
		virtual void			Display() const;
		virtual void			Update( const SPspPadState & pad_state, float elapsed_time );
		virtual const char *	GetDescription() const									{ return "Test variables"; }
};

void CDecalOffsetDebugMenuOption::Display() const
{
	printf( "Test variable X:%0.2f Y:%0.2f\n", TEST_VARX, TEST_VARY );
	printf( "   Use [] to return\n" );
	printf( "   Use stick up/down & left/right to adjust\n" );
}

void CDecalOffsetDebugMenuOption::Update( const SPspPadState & pad_state, float elapsed_time )
{
	const float CHANGE_PER_SECOND = 1000;

	if( pad_state.Stick.x != 0 )
	{
		//TEST_VAR += pad_state.Stick.y * CHANGE_PER_SECOND * elapsed_time;
		if(pad_state.Stick.x > 0) TEST_VARX += CHANGE_PER_SECOND * elapsed_time;
		else TEST_VARX -= CHANGE_PER_SECOND * elapsed_time;
		InvalidateDisplay();
	}
	if( pad_state.Stick.y != 0 )
	{
		if(pad_state.Stick.y < 0) TEST_VARY += CHANGE_PER_SECOND * elapsed_time;
		else TEST_VARY -= CHANGE_PER_SECOND * elapsed_time;
		InvalidateDisplay();
	}
}


}

//*************************************************************************************
//
//*************************************************************************************
class IDisplayListDebugger : public CDisplayListDebugger
{
	public:
		virtual ~IDisplayListDebugger() {}

		virtual void					Run();
};


//*************************************************************************************
//
//*************************************************************************************
CDisplayListDebugger *	CDisplayListDebugger::Create()
{
	return new IDisplayListDebugger;
}

//*************************************************************************************
//
//*************************************************************************************
CDisplayListDebugger::~CDisplayListDebugger()
{
}

//*************************************************************************************
//
//*************************************************************************************
void IDisplayListDebugger::Run()
{
	//
	//	Enter the debug menu as soon as select is newly pressed
	//
    SceCtrlData		pad;
	SPspPadState	pad_state;

	pad_state.OldButtons = 0;

	sceCtrlPeekBufferPositive(&pad, 1);

	pad_state.OldButtons = pad.Buttons;

	bool menu_button_pressed( false );

	u64		freq;
	NTiming::GetPreciseFrequency( &freq );
	float freq_inv = 1.0f / f32( freq );

	PSPRenderer::Get()->SetRecordCombinerStates( true );

	// Dumpl the display list on the first time through the loop
	DLParser_DumpNextDisplayList();

	CTimer		timer;

	typedef std::vector< CDebugMenuOption * > DebugMenuOptionVector;
	DebugMenuOptionVector	menu_options;

	menu_options.push_back( new CCombinerExplorerDebugMenuOption );
	menu_options.push_back( new CBlendDebugMenuOption );
	menu_options.push_back( new CTextureExplorerDebugMenuOption );
	menu_options.push_back( new CDisplayListLengthDebugMenuOption );
	menu_options.push_back( new CDecalOffsetDebugMenuOption );

	u32		highlighted_option( 0 );
	CDebugMenuOption *			p_current_option( NULL );

	// Remain paused until the Select button is pressed again
	bool	need_update_display( true );
	bool	dump_next_screen( false );
	u32		total_instruction_count( DLParser_GetTotalInstructionCount() );
	while(!menu_button_pressed)
	{
		//guSwapBuffersBehaviour( PSP_DISPLAY_SETBUF_IMMEDIATE );

		if( dump_next_screen )
		{
			CGraphicsContext::Get()->DumpScreenShot();
			dump_next_screen = false;
		}

		CGraphicsContext::Get()->BeginFrame();
		CGraphicsContext::Get()->Clear( true, true );
		CGraphicsContext::Get()->EndFrame();

		u64			time_before;
		NTiming::GetPreciseTime( &time_before );

		//
		//	Re-render the current frame
		//
		bool	render_dlist( true );
		if( p_current_option != NULL )
		{
			if( p_current_option->OverrideDisplay() )
			{
				render_dlist = false;
			}
		}

		if( render_dlist )
		{
			DLParser_Process();
		}

		u64			time_after;
		NTiming::GetPreciseTime( &time_after );

		//
		//	Figure out how long the last frame took
		//
		u64			elapsed_ticks( time_after - time_before );
		float		elapsed_ms( f32(elapsed_ticks) * 1000.0f * freq_inv );
		float		framerate( 0.0f );
		if(elapsed_ms > 0)
		{
			framerate = 1000.0f / elapsed_ms;
		}

		CGraphicsContext::Get()->UpdateFrame( false );

		//sceDisplayWaitVblankStart();

		sceCtrlPeekBufferPositive(&pad, 1);

		pad_state.NewButtons = pad.Buttons;

		const s32	STICK_DEADZONE = 20;

		s32		stick_x( pad.Lx - 128 );
		s32		stick_y( pad.Ly - 128 );

		if(stick_x >= -STICK_DEADZONE && stick_x <= STICK_DEADZONE)
		{
			stick_x = 0;
		}
		if(stick_y >= -STICK_DEADZONE && stick_y <= STICK_DEADZONE)
		{
			stick_y = 0;
		}

		pad_state.Stick.x = float(stick_x) / 128.0f;
		pad_state.Stick.y = float(stick_y) / 128.0f;

		float actual_elapsed_time( timer.GetElapsedSeconds() );
		
		//
		//	Update input
		//
		if( p_current_option != NULL )
		{
			p_current_option->Update( pad_state, actual_elapsed_time );
			if(p_current_option->NeedsUpdateDisplay())
			{
				need_update_display = true;
			}
		}

		//
		//	Refresh display
		//
		if(need_update_display)
		{
			printf( TERMINAL_CLEAR_SCREEN );
			printf( TERMINAL_TOP_LEFT );
			printf( "Dlist took %dms (%fhz) [%d/%d]\n", s32(elapsed_ms), framerate, DLParser_GetInstructionCountLimit(), total_instruction_count );
			printf( "\n" );
			for( u32 i = 0; i < GBIMicrocode_GetMicrocodeHistoryStringCount(); ++i )
			{
				printf( "%s\n", GBIMicrocode_GetMicrocodeHistoryString( i ) );
			}
			printf( "\n" );

			if( p_current_option != NULL )
			{
				p_current_option->UpdateDisplay();
			}
			else
			{
				u32 idx = 0;
				for( DebugMenuOptionVector::const_iterator it = menu_options.begin(); it != menu_options.end(); ++it, idx++ )
				{
					bool				selected( idx == highlighted_option );
					CDebugMenuOption *	p_option( *it );

					printf( "%c%s\n", selected ? '*' : ' ', p_option->GetDescription() );
				}
			}

			need_update_display = false;

			printf( TERMINAL_SAVE_POS );
		}
		else
		{
			// Just update timing info
			printf( TERMINAL_TOP_LEFT );
			printf( TERMINAL_CLEAR_LINE );
			printf( "Dlist took %dms (%fhz) [%d/%d]\n", s32(elapsed_ms), framerate, DLParser_GetInstructionCountLimit(), total_instruction_count );
			printf( TERMINAL_RESTORE_POS );
		}
		fflush( stdout );

		//
		//	Input
		//
		if( p_current_option != NULL )
		{
			if(pad_state.OldButtons != pad_state.NewButtons)
			{
				if(pad_state.NewButtons & PSP_CTRL_SQUARE)
				{
					p_current_option = NULL;
					need_update_display = true;
				}
			}
		}
		else
		{
			if(pad_state.OldButtons != pad_state.NewButtons)
			{
				if(pad_state.NewButtons & PSP_CTRL_UP)
				{
					if( highlighted_option > 0 )
					{
						highlighted_option--;
						need_update_display = true;
					}
				}
				if(pad_state.NewButtons & PSP_CTRL_DOWN)
				{
					if( highlighted_option < menu_options.size() - 1 )
					{
						highlighted_option++;
						need_update_display = true;
					}
				}
				if(pad_state.NewButtons & PSP_CTRL_CROSS)
				{
					p_current_option = menu_options[ highlighted_option ];
					need_update_display = true;
				}

			}
		}

		if(pad_state.OldButtons != pad_state.NewButtons)
		{
			if(pad_state.NewButtons & PSP_CTRL_SELECT)
			{
				menu_button_pressed = true;
			}
			if(pad_state.NewButtons & PSP_CTRL_START)
			{
				dump_next_screen = true;
			}
			if(pad_state.NewButtons & PSP_CTRL_LTRIGGER)
			{
				gGlobalPreferences.ViewportType = EViewportType( (gGlobalPreferences.ViewportType+1) % NUM_VIEWPORT_TYPES );	
				CGraphicsContext::Get()->ClearAllSurfaces();
			}
		}

		pad_state.OldButtons = pad_state.NewButtons;
	}

	PSPRenderer::Get()->SetRecordCombinerStates( false );
	DLParser_SetInstructionCountLimit( UNLIMITED_INSTRUCTION_COUNT );

	//
	//	Clean up
	//
	for( DebugMenuOptionVector::const_iterator it = menu_options.begin(); it != menu_options.end(); ++it )
	{
		CDebugMenuOption *	p_option( *it );

		delete p_option;
	}
}

#endif
