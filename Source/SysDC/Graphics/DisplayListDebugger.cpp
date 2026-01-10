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

#include "PSPRenderer.h"
#include "DLParser.h"
#include "GraphicsContext.h"

#include "DaedTiming.h"
#include "DaedMathUtil.h"

#include <set>
#include <vector>

#include <pspctrl.h>


extern float	GetElapsedTime();
extern void		ResetElapsedTime();
extern float	DECAL_Z_OFFSET;
extern void		PrintMux( FILE * fh, u64 mux );

namespace
{
	//	const char * const TERMINAL_TOP_LEFT			= "\033[2A\033[2K";
		const char * const TERMINAL_TOP_LEFT			= "\033[H";
		const char * const TERMINAL_CLEAR_SCREEN		= "\033[2J";
		const char * const TERMINAL_CLEAR_LINE			= "\033[2K";

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

class CDebugMenuOption 
{
	public:
				CDebugMenuOption();
		virtual ~CDebugMenuOption() {}

		virtual void			Enter()											{}
		virtual void			Exit()											{}
		virtual void			Update( const SPspPadState & pad_state, float elapsed_time ) = 0;
			
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
	if( mRefreshDisplay )
	{
		Display();
		mRefreshDisplay = false;
	}
}


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

class CDecalOffsetDebugMenuOption : public CDebugMenuOption
{
	public:	
		virtual void			Display() const;
		virtual void			Update( const SPspPadState & pad_state, float elapsed_time );
		virtual const char *	GetDescription() const									{ return "Decal Offset"; }
};

void CDecalOffsetDebugMenuOption::Display() const
{
	printf( "ZDecal offset is %f\n", DECAL_Z_OFFSET );
	printf( "   Use [] to return\n" );
	printf( "   Use stick up/down to adjust\n" );
}

void CDecalOffsetDebugMenuOption::Update( const SPspPadState & pad_state, float elapsed_time )
{
	const float DECAL_Z_CHANGE_PER_SECOND = 1;

	if( pad_state.Stick.y != 0 )
	{
		DECAL_Z_OFFSET += pad_state.Stick.y * DECAL_Z_CHANGE_PER_SECOND * elapsed_time;
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

	ResetElapsedTime();

	typedef std::vector< CDebugMenuOption * > DebugMenuOptionVector;
	DebugMenuOptionVector	menu_options;

	menu_options.push_back( new CCombinerExplorerDebugMenuOption );
	menu_options.push_back( new CDisplayListLengthDebugMenuOption );
	menu_options.push_back( new CDecalOffsetDebugMenuOption );

	u32		highlighted_option( 0 );
	CDebugMenuOption *			p_current_option( NULL );

	// Remain paused until the Select button is pressed again
	while(!menu_button_pressed)
	{
		//guSwapBuffersBehaviour( PSP_DISPLAY_SETBUF_IMMEDIATE );

		CGraphicsContext::Get()->BeginFrame();
		CGraphicsContext::Get()->Clear( true, true );
		CGraphicsContext::Get()->EndFrame();

		u64			time_before;
		NTiming::GetPreciseTime( &time_before );

		//
		//	Re-render the current frame
		//
		DLParser_Process();

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

		float actual_elapsed_time( GetElapsedTime() );

		printf( TERMINAL_TOP_LEFT );
		
		if(p_current_option != NULL)
		{
			if(p_current_option->NeedsUpdateDisplay())
			{
				printf( TERMINAL_CLEAR_SCREEN );
			}
		}

		printf( TERMINAL_CLEAR_LINE );
		printf( "Dlist took %dms (%fhz) \n", s32(elapsed_ms), framerate );
		//printf( "Vertices - In: %d, Out: %d\n", gTesselatorVerticesIn, gTesselatorVerticesOut );
		//CTextureCache::Get()->DisplayStats();
		//printf( "Software clipping %s\n", gSoftwareClipping ? "enabled" : "disabled" );


		if( p_current_option != NULL )
		{
			p_current_option->Update( pad_state, actual_elapsed_time );

			if(p_current_option->NeedsUpdateDisplay())
			{
				p_current_option->UpdateDisplay();
			}

			if(pad_state.OldButtons != pad_state.NewButtons)
			{
				if(pad_state.NewButtons & PSP_CTRL_SQUARE)
				{
					p_current_option = NULL;
					printf( TERMINAL_CLEAR_SCREEN );
				}
			}
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

			if(pad_state.OldButtons != pad_state.NewButtons)
			{
				if(pad_state.NewButtons & PSP_CTRL_UP)
				{
					if( highlighted_option > 0 )
						highlighted_option--;
				}
				if(pad_state.NewButtons & PSP_CTRL_DOWN)
				{
					if( highlighted_option < menu_options.size() - 1 )
						highlighted_option++;
				}
				if(pad_state.NewButtons & PSP_CTRL_CROSS)
				{
					p_current_option = menu_options[ highlighted_option ];
				}
			}
		}


		if(pad_state.OldButtons != pad_state.NewButtons)
		{
			if(pad_state.NewButtons & PSP_CTRL_SELECT)
			{
				menu_button_pressed = true;
			}
		}

		pad_state.OldButtons = pad_state.NewButtons;
	}

	PSPRenderer::Get()->SetRecordCombinerStates( false );
	//??DLParser_SetInstructionCountLimit( UNLIMITED_INSTRUCTION_COUNT );

	//
	//	Clean up
	//
	for( DebugMenuOptionVector::const_iterator it = menu_options.begin(); it != menu_options.end(); ++it )
	{
		CDebugMenuOption *	p_option( *it );

		delete p_option;
	}
}
