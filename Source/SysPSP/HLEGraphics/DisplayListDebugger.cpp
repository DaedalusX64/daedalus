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
#include "HLEGraphics/DisplayListDebugger.h"

#ifdef DAEDALUS_DEBUG_DISPLAYLIST

#include "Combiner/RenderSettings.h"

#include "HLEGraphics/CachedTexture.h"
#include "HLEGraphics/DLDebug.h"
#include "HLEGraphics/DLParser.h"
#include "HLEGraphics/Microcode.h"
#include "SysPSP/HLEGraphics/RendererPSP.h"
#include "HLEGraphics/TextureCache.h"
#include "HLEGraphics/TextureInfo.h"

#include "Graphics/GraphicsContext.h"
#include "Graphics/NativeTexture.h"

#include "Core/ROM.h"
#include "Debug/Dump.h"

#include "Utility/IO.h"
#include "Utility/Preferences.h"
#include "Utility/Timer.h"
#include "Utility/Timing.h"

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
extern DebugBlendSettings gDBlend;

// FIXME: we shouldn't rely on this global state.
// We should call DLParser_Process(kUnlimitedInstructionCount) when we enter the debugger, and that will return a count. T
extern u32 gNumInstructionsExecuted;

//*****************************************************************************
//
//*****************************************************************************
static bool	gDebugDisplayList = false;
static bool	gSingleStepFrames = false;


class CDisplayListDebugger
{
	public:
		virtual ~CDisplayListDebugger();

		static CDisplayListDebugger *	Create();

		virtual void					Run() = 0;
};

bool DLDebugger_IsDebugging()
{
	return gDebugDisplayList;
}

void DLDebugger_RequestDebug()
{
	gDebugDisplayList = true;
}

bool DLDebugger_Process()
{
	// DLParser_Process may set this flag, so check again after execution
	if(gDebugDisplayList)
	{
		CDisplayListDebugger *	debugger = CDisplayListDebugger::Create();
		debugger->Run();
		delete debugger;
		gDebugDisplayList = gSingleStepFrames;
		gSingleStepFrames = false;
		return true;
	}

	return false;
}

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

		const char * const TERMINAL_RED					= "\033[1;31m";
		const char * const TERMINAL_GREEN				= "\033[1;32m";
		const char * const TERMINAL_YELLOW				= "\033[1;33m";
		const char * const TERMINAL_BLUE				= "\033[1;34m";
		const char * const TERMINAL_MAGENTA				= "\033[1;35m";
		const char * const TERMINAL_CYAN				= "\033[1;36m";
		const char * const TERMINAL_WHITE				= "\033[1;37m";

//const char * const gDDLOText[] =
//{
//	"Combiner Explorer",	// DDLO_COMBINER_EXPLORER
//	"Display List Length",	// DDLO_DLIST_LENGTH
//	"Decal Offset",			// DDLO_DECAL_OFFSET
//	"Texture Viewer",		// DDLO_TEXTURE_VIEWER
//	"Dump Textures",		// DDLO_DUMP_TEXTURES
//	"Dump Dlist",			// DDLO_DUMP_DLIST
//};

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
	const std::set< u64 > & combiner_states( gRendererPSP->GetRecordedCombinerStates() );

	printf( "   Use [] to return\n" );
	printf( "   Use O to select on/off\n" );
	printf( "   Use /\\ to highlight texture\n" );
	printf( "   Use up/down to move cursor\n\n" );
	printf( "   %sHandled Inexact Blend\n", TERMINAL_GREEN );
	printf( "   %sForced Blend\n", TERMINAL_MAGENTA );
	printf( "   %sAuto Combined or Default Inexact Blend\n", TERMINAL_CYAN );
	printf( "   %sSelected for Blend Explorer\n\n", TERMINAL_RED );
	printf( "%sCombiner States in use:\n", TERMINAL_WHITE );

	u32		idx( 0 );
	u64		selected_mux( 0 );
	for(std::set<u64>::const_iterator it = combiner_states.begin(); it != combiner_states.end(); ++it)
	{
		u64		state( *it );

		bool	selected( idx == mSelectedIdx );
		bool	disabled( gRendererPSP->IsCombinerStateDisabled( state ) );
		//bool	unhandled( gRendererPSP->IsCombinerStateUnhandled( state ) );
		bool	forced( gRendererPSP->IsCombinerStateForced( state ) );
		bool	idefault( gRendererPSP->IsCombinerStateDefault( state ) );
		const char *	text_col;

		if(selected)
		{
			//text_col = TERMINAL_YELLOW;
			selected_mux = state;
		}

		if(disabled)
		{
			text_col = TERMINAL_RED;
		}
		else if(forced)
		{
			text_col = TERMINAL_MAGENTA;
		}
		else if(idefault)
		{
			text_col = TERMINAL_CYAN;
		}
		else
		{
			text_col = TERMINAL_GREEN;
		}

		printf( "  %s%c%08x%08x\n", text_col, selected ? '*' : ' ', u32(state >> 32), u32(state) );

		idx++;
	}
	printf( "%s\n", TERMINAL_WHITE );

	if( selected_mux != 0 )
	{
		DLDebug_PrintMux( stdout, selected_mux );

		RendererPSP::SBlendStateEntry entry1( gRendererPSP->LookupBlendState( selected_mux, false ) );
		if( entry1.OverrideFunction != NULL )
		{
			printf( "1 Cycle: Overridden\n" );
		}
		else if( entry1.States != NULL )
		{
			printf( "1 Cycle:\n" );
			entry1.States->Print();
		}

		RendererPSP::SBlendStateEntry	entry2( gRendererPSP->LookupBlendState( selected_mux, true ) );
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
	const std::set< u64 > & combiner_states( gRendererPSP->GetRecordedCombinerStates() );

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

		if(pad_state.NewButtons & PSP_CTRL_TRIANGLE)
		{
			if(selected_state != 0)
			{
				gRendererPSP->ToggleDisableCombinerState( selected_state );
				gRendererPSP->ToggleNastyTexture( true );
				InvalidateDisplay();
			}
		}

		if(pad_state.NewButtons & PSP_CTRL_CIRCLE)
		{
			if(selected_state != 0)
			{
				gRendererPSP->ToggleDisableCombinerState( selected_state );
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
		virtual const char *	GetDescription() const									{ return "Blend Explorer"; }

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
	const char * const ForceColor[8] =
	{
		"( OFF )",
		"( c32::White )",
		"( c32::Black )",
		"( c32::Red )",
		"( c32::Green )",
		"( c32::Blue )",
		"( c32::Magenta )",
		"( c32::Gold )"
	};

	const char * const PSPtxtFunc[10] =
	{
		"( GU_TFX_MODULATE, GU_TCC_RGB )",
		"( GU_TFX_MODULATE, GU_TCC_RGBA )",
		"( GU_TFX_BLEND, GU_TCC_RGB )",
		"( GU_TFX_BLEND, GU_TCC_RGBA )",
		"( GU_TFX_ADD, GU_TCC_RGB )",
		"( GU_TFX_ADD, GU_TCC_RGBA )",
		"( GU_TFX_REPLACE, GU_TCC_RGB )",
		"( GU_TFX_REPLACE, GU_TCC_RGBA )",
		"( GU_TFX_DECAL, GU_TCC_RGB )",
		"( GU_TFX_DECAL, GU_TCC_RGBA )"
	};

	const char * const CAdj[5] =
	{
		"( OFF )",
		"( details.PrimColour )",
		"( details.PrimColour.ReplicateAlpha() )",
		"( details.EnvColour )",
		"( details.EnvColour.ReplicateAlpha() )"
	};

	const char * const ENVAdj[3] =
	{
		"( OFF )",
		"( details.EnvColour.GetColour() )",
		"( details.PrimColour.GetColour() )"
	};

	if( modify )
	{
		switch( mSel )
		{
			case 0: gDBlend.SetRGB = mIdx % 5; break;
			case 1: gDBlend.SetA = mIdx % 5; break;
			case 2: gDBlend.SetRGBA = mIdx % 5; break;
			case 3: gDBlend.ModRGB = mIdx % 5; break;
			case 4: gDBlend.ModA = mIdx % 5; break;
			case 5: gDBlend.ModRGBA = mIdx % 5; break;
			case 6: gDBlend.SubRGB = mIdx % 5; break;
			case 7: gDBlend.SubA = mIdx % 5; break;
			case 8: gDBlend.SubRGBA = mIdx % 5; break;
			case 9: gDBlend.AOpaque = mIdx & 1; break;
			case 10: gDBlend.sceENV = mIdx % 3; break;
			case 11: gDBlend.TXTFUNC = mIdx % 10; break;
			case 12: gDBlend.TexInstall = mIdx & 1; break;
			case 13: gDBlend.ForceRGB = mIdx % 8; break;
		}
	}

#define BLEND_SELECTION(x) (mSel==x && modify) ? TERMINAL_GREEN : TERMINAL_WHITE, mSel==x ? '*' : ' '

	printf( "Blend Explorer\n");
	printf( "   Use [] to return\n" );
	printf( "   Use X to modify\n" );
	printf( "   Use up/down to choose & left/right to adjust\n\n\n" );

	printf( " Blending Options\n" );
	printf( "   %s%cdetails.ColourAdjuster.SetRGB%s\n", BLEND_SELECTION(0), CAdj[gDBlend.SetRGB]);
	printf( "   %s%cdetails.ColourAdjuster.SetA%s\n", BLEND_SELECTION(1), CAdj[gDBlend.SetA]);
	printf( "   %s%cdetails.ColourAdjuster.SetRGBA%s\n", BLEND_SELECTION(2), CAdj[gDBlend.SetRGBA]);
	printf( "   %s%cdetails.ColourAdjuster.ModulateRGB%s\n", BLEND_SELECTION(3), CAdj[gDBlend.ModRGB]);
	printf( "   %s%cdetails.ColourAdjuster.ModulateA%s\n", BLEND_SELECTION(4), CAdj[gDBlend.ModA]);
	printf( "   %s%cdetails.ColourAdjuster.ModulateRGBA%s\n", BLEND_SELECTION(5), CAdj[gDBlend.ModRGBA]);
	printf( "   %s%cdetails.ColourAdjuster.SubtractRGB%s\n", BLEND_SELECTION(6), CAdj[gDBlend.SubRGB]);
	printf( "   %s%cdetails.ColourAdjuster.SubtractA%s\n", BLEND_SELECTION(7), CAdj[gDBlend.SubA]);
	printf( "   %s%cdetails.ColourAdjuster.SubtractRGBA%s\n", BLEND_SELECTION(8), CAdj[gDBlend.SubRGBA]);
	printf( "   %s%cdetails.ColourAdjuster.SetAOpaque() %s\n", BLEND_SELECTION(9), gDBlend.AOpaque ? "ON" : "OFF");
	printf( "%s\n", TERMINAL_WHITE );
	printf( " Environment Color (only works with GU_TFX_BLEND option)\n" );
	printf( "   %s%csceGuTexEnvColor%s\n", BLEND_SELECTION(10), ENVAdj[gDBlend.sceENV]);
	printf( "%s\n", TERMINAL_WHITE );
	printf( " Texture Blending Function\n" );
	printf( "   %s%csceGuTexFunc%s\n", BLEND_SELECTION(11), PSPtxtFunc[gDBlend.TXTFUNC]);
	printf( "%s\n", TERMINAL_WHITE );
	printf( " Other Options\n" );
	printf( "   %s%cTexture Enabled: %s\n", BLEND_SELECTION(12), gDBlend.TexInstall ? "ON" : "OFF");
	printf( "   %s%cdetails.ColourAdjuster.SetRGB%s\n", BLEND_SELECTION(13), ForceColor[gDBlend.ForceRGB]);
	printf( "%s\n", TERMINAL_WHITE );

#undef BLEND_SELECTION
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
			mSel = (mSel < 13) ? mSel + 1 : mSel;	//Number of menu rows
			modify = 0;
		}

		if(pad_state.NewButtons & PSP_CTRL_LEFT)
		{
			mIdx = (mIdx > 0) ? mIdx - 1 : mIdx;	//min select
		}

		if(pad_state.NewButtons & PSP_CTRL_RIGHT)
		{
			mIdx = (mIdx < 9) ? mIdx + 1 : mIdx;	//max select
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
	   return lhs.Info.GetLoadAddress() < rhs.Info.GetLoadAddress();
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

	{
		// The lock isn't really needed, as on the PSP we run this single threaded.
		MutexLock lock(CTextureCache::Get()->GetDebugMutex());
		CTextureCache::Get()->Snapshot( lock, mSnapshot );
	}

	sort( mSnapshot.begin(), mSnapshot.end(), &OrderTextures );
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
	u32 texture_width  = 32;
	u32 texture_height = 32;
	if( mSelectedIdx < mSnapshot.size() )
	{
		texture = mSnapshot[ mSelectedIdx ].Texture;

		const TextureInfo & info = mSnapshot[ mSelectedIdx ].Info;

		texture_width  = info.GetWidth();
		texture_height = info.GetHeight();
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
		const TextureInfo & ti = mSnapshot[i].Info;

		bool			selected = u32( i ) == mSelectedIdx;
		const char *	text_col = selected ? TERMINAL_YELLOW : TERMINAL_WHITE;

		// XXXX
		u32	left = 0;
		u32	top  = 0;

		printf( " %s%03d %c%08x (%d,%d -> %dx%d, %d) %s/%dbpp, %04x, %04x\n",
			text_col, i, selected ? '*' : ' ',
			ti.GetLoadAddress(),
			left, top, ti.GetWidth(), ti.GetHeight(), ti.GetPitch(),
			ti.GetFormatName(), ti.GetSizeInBits(),
			ti.GetTmemAddress(), ti.GetPalette() );
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
		if(fabsf(pad_state.Stick.x) < 0.001f && fabsf(pad_state.Stick.y) < 0.001f)
		{
			mTextureOffset *= powf( 0.997f, elapsed_time * 1000.0f );
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
		CDisplayListLengthDebugMenuOption(u32 total, u32 * limit);

		virtual void			Display() const;
		virtual void			Update( const SPspPadState & pad_state, float elapsed_time );
		virtual const char *	GetDescription() const									{ return "Display List Length"; }


	private:
				u32				mTotalInstructionCount;
				u32 *			mInstructionCountLimit;

				float			mFractionalAdjustment;
};

CDisplayListLengthDebugMenuOption::CDisplayListLengthDebugMenuOption(u32 total, u32 * limit)
:	mTotalInstructionCount( total )
,	mInstructionCountLimit( limit )
,	mFractionalAdjustment( 0.0f )
{
}

void CDisplayListLengthDebugMenuOption::Display() const
{
	printf( "Display list length %d / %d:\n", *mInstructionCountLimit, mTotalInstructionCount );
	printf( "   Use [] to return\n" );
	printf( "   Use up/down to adjust\n" );
}

void CDisplayListLengthDebugMenuOption::Update( const SPspPadState & pad_state, float elapsed_time )
{
	float	rate_adjustment( 1.0f );

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
		s32 new_limit = *mInstructionCountLimit + adjustment;

		*mInstructionCountLimit = u32( Clamp< s32 >( new_limit, 0, mTotalInstructionCount ) );
		mFractionalAdjustment -= float( adjustment );

		InvalidateDisplay();
	}
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

	CTimer		timer;

	typedef std::vector< CDebugMenuOption * > DebugMenuOptionVector;
	DebugMenuOptionVector	menu_options;

	u32		total_instruction_count = gNumInstructionsExecuted;
	u32		instruction_limit       = gNumInstructionsExecuted;

	menu_options.push_back( new CCombinerExplorerDebugMenuOption );
	menu_options.push_back( new CBlendDebugMenuOption );
	menu_options.push_back( new CTextureExplorerDebugMenuOption );
	menu_options.push_back( new CDisplayListLengthDebugMenuOption(total_instruction_count, &instruction_limit) );
	menu_options.push_back( new CDecalOffsetDebugMenuOption );

	u32		highlighted_option( 0 );
	CDebugMenuOption *			p_current_option( NULL );

	// Remain paused until the Select button is pressed again
	bool	need_update_display( true );
	bool	dump_next_screen( false );
	bool	dump_texture_dlist( false );

	while( (pad_state.NewButtons & PSP_CTRL_HOME) != 0 || !menu_button_pressed )
	{
		//guSwapBuffersBehaviour( PSP_DISPLAY_SETBUF_IMMEDIATE );

		if( dump_next_screen )
		{
			dump_next_screen = false;
			CGraphicsContext::Get()->DumpScreenShot();
		}

		DLDebugOutput * debug_output = NULL;

		if( dump_texture_dlist )
		{
			dump_texture_dlist = false;

			printf( TERMINAL_TOP_LEFT );
			printf( TERMINAL_CLEAR_LINE );
			printf( "Dumping Display List and Textures...\n" );

			// Dump the display list
			debug_output = DLDebug_CreateFileOutput();

			// Dump textures
			MutexLock lock(CTextureCache::Get()->GetDebugMutex());

			std::vector<CTextureCache::STextureInfoSnapshot> snapshot;
			CTextureCache::Get()->Snapshot( lock, snapshot );

			sort( snapshot.begin(), snapshot.end(), &OrderTextures );

			// Dump each in turn
			for( u32 i = 0; i < snapshot.size(); ++i )
			{
				CachedTexture::DumpTexture( snapshot[i].Info, snapshot[i].Texture );
			}
		}

		CGraphicsContext::Get()->BeginFrame();
		CGraphicsContext::Get()->ClearToBlack();
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
			DLParser_Process(instruction_limit, debug_output);

			// We can delete the sink as soon as we're done with it.
			delete debug_output;
			debug_output = NULL;
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
			printf( "Dlist took %dms (%fHz) [%d/%d]\n", s32(elapsed_ms), framerate, instruction_limit, total_instruction_count );
			printf( "\n" );

			if( p_current_option != NULL )
			{
				p_current_option->UpdateDisplay();
			}
			else
			{
				printf( "(HOME) -> Resume game\n" );
				printf( "(START) -> Screen shot\n" );
				printf( "(SELECT) -> Go to next frame\n" );
				printf( "(R-TRIG) -> Dump DList & Textures\n" );
				printf( "(L-TRIG) -> Scale Screen\n\n" );
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
			printf( "Dlist took %dms (%fHz) [%d/%d]\n", s32(elapsed_ms), framerate, instruction_limit, total_instruction_count );
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
			if(pad_state.NewButtons & PSP_CTRL_HOME)
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
			if(pad_state.NewButtons & PSP_CTRL_RTRIGGER)
			{
				dump_texture_dlist = true;
			}
			if(pad_state.NewButtons & PSP_CTRL_SELECT)
			{
				gSingleStepFrames = true;
				menu_button_pressed = true;
			}
		}

		pad_state.OldButtons = pad_state.NewButtons;
	}

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
