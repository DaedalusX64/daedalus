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
#include "Debug/DBGConsole.h"

#ifdef DAEDALUS_DEBUG_CONSOLE

#include "Graphics/ColourValue.h"

#include "Test/BatchTest.h"

#include <pspdebug.h>
#include <stdarg.h>

#define DAEDALUS_USETERMINAL

class IDebugConsole : public CDebugConsole
{
	public:
		IDebugConsole();
		virtual ~IDebugConsole();

		void			EnableConsole( bool bEnable );
		bool			IsVisible() const;

		void			UpdateDisplay();

		void DAEDALUS_VARARG_CALL_TYPE	Msg(u32 type, const char * szFormat, ...);

		void							MsgOverwriteStart();
		void DAEDALUS_VARARG_CALL_TYPE	MsgOverwrite(u32 type, const char * szFormat, ...);
		void							MsgOverwriteEnd();
		void DAEDALUS_VARARG_CALL_TYPE	Stats( StatType stat, const char * szFormat, ...)			{}

private:
			enum	ETerminalColour
			{
				TC_DEFAULT = 0,
				TC_r, TC_g, TC_y, TC_b, TC_m, TC_c, TC_w,
				TC_R, TC_G, TC_Y, TC_B, TC_M, TC_C, TC_W,
				TC_INVALID,
				NUM_TERMINAL_COLOURS,
			};

	static ETerminalColour		GetTerminalColour( char c );

			void				ParseAndDisplayString( const char * p_string, ETerminalColour default_colour );

			void				DisplayString( const char * p_string );
			void				SetCurrentTerminalColour( ETerminalColour tc );
private:
			bool				mDebugScreenEnabled;
			u32					mCurrentY;

	static	const char * const	mTerminalColours[NUM_TERMINAL_COLOURS];
	static	const c32			mScreenColours[NUM_TERMINAL_COLOURS];
	static	char				mFormattingBuffer[ 2048 ];

};

//*****************************************************************************
//
//*****************************************************************************

const char * const IDebugConsole::mTerminalColours[NUM_TERMINAL_COLOURS] =
{
	"",		// TC_DEFAULT
	"\033[0;31m", "\033[0;32m", "\033[0;33m", "\033[0;34m", "\033[0;35m", "\033[0;36m", "\033[0;37m",
	"\033[1;31m", "\033[1;32m", "\033[1;33m", "\033[1;34m", "\033[1;35m", "\033[1;36m", "\033[1;37m",
	"",		// TC_INVALID
};

namespace
{
	const u8 NORMAL = 200;
	const u8 BRIGHT = 255;
	const char * const TERMINAL_SAVE_CURSOR			= "\033[s";
	const char * const TERMINAL_RESTORE_CURSOR		= "\033[u";
	const char * const TERMINAL_ERASE_LINE			= "\033[2K";
}

const c32 IDebugConsole::mScreenColours[NUM_TERMINAL_COLOURS] =
{
	c32( 0, 0, 0 ),		// TC_DEFAULT
	c32( NORMAL, 0, 0 ),
	c32( 0, NORMAL, 0 ),
	c32( NORMAL, NORMAL, 0 ),
	c32( 0, 0, NORMAL ),
	c32( NORMAL, 0, NORMAL ),
	c32( 0, NORMAL, NORMAL ),
	c32( NORMAL, NORMAL, NORMAL ),
	c32( BRIGHT, 0, 0 ),
	c32( 0, BRIGHT, 0 ),
	c32( BRIGHT, BRIGHT, 0 ),
	c32( 0, 0, BRIGHT ),
	c32( BRIGHT, 0, BRIGHT ),
	c32( 0, BRIGHT, BRIGHT ),
	c32( BRIGHT, BRIGHT, BRIGHT ),
	c32( 0, 0, 0 ),		// TC_INVALID
};


//*****************************************************************************
//
//*****************************************************************************
char	IDebugConsole::mFormattingBuffer[ 2048 ];

//*****************************************************************************
//
//*****************************************************************************
template<> bool	CSingleton< CDebugConsole >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = new IDebugConsole();

	return true;
}

//*****************************************************************************
//
//*****************************************************************************
CDebugConsole::~CDebugConsole()
{

}

//*****************************************************************************
//
//*****************************************************************************
void	IDebugConsole::DisplayString( const char * p_string )
{
	if( mDebugScreenEnabled )
	{
		pspDebugScreenPrintf( "%s", p_string );
	}
	printf( "%s", p_string );

#ifdef DAEDALUS_BATCH_TEST_ENABLED
	CBatchTestEventHandler * handler( BatchTest_GetHandler() );
	if( handler )
		handler->OnDebugMessage( p_string );
#endif

}

//*****************************************************************************
//
//*****************************************************************************
void	IDebugConsole::SetCurrentTerminalColour( ETerminalColour tc )
{
	if( mDebugScreenEnabled )
	{
		pspDebugScreenSetTextColor( mScreenColours[ tc ].GetColour() );
	}
#ifdef DAEDALUS_USETERMINAL
	printf( mTerminalColours[ tc ] );
#endif
}

//*****************************************************************************
//
//*****************************************************************************
IDebugConsole::ETerminalColour IDebugConsole::GetTerminalColour(char c)
{
	switch(c)
	{
		case 'r': return TC_r;
		case 'g': return TC_g;
		case 'y': return TC_y;
		case 'b': return TC_b;
		case 'm': return TC_m;
		case 'c': return TC_c;
		case 'w': return TC_w;
		case 'R': return TC_R;
		case 'G': return TC_G;
		case 'Y': return TC_Y;
		case 'B': return TC_B;
		case 'M': return TC_M;
		case 'C': return TC_C;
		case 'W': return TC_W;
		default: return TC_INVALID;
	}
}


//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::ParseAndDisplayString( const char * p_string, ETerminalColour default_colour )
{
	SetCurrentTerminalColour( default_colour );

	char		current_string[ 1024 ];
	u32			out_idx( 0 );
	u32 string_len( strlen(p_string) );
	for ( u32 in_idx = 0; in_idx < string_len; ++in_idx )
	{
		if (p_string[in_idx] == '[')
		{
			ETerminalColour tc = GetTerminalColour(p_string[in_idx+1]);
			if(tc != TC_INVALID)
			{
				// Flush the current string and update the colour
				current_string[out_idx] = 0;
				DisplayString( current_string );
				out_idx = 0;
				SetCurrentTerminalColour( tc );
			}
			else
			{
				switch (p_string[in_idx+1])
				{
				case '[':
				case ']':
					current_string[out_idx] = p_string[in_idx+1];
					out_idx++;
					break;
				}
			}

			// Skip colour character
			in_idx++;
		}
		else if (p_string[in_idx] == ']')
		{
			// Flush the current string and update the colour
			current_string[out_idx] = 0;
			DisplayString( current_string );
			out_idx = 0;
			SetCurrentTerminalColour( default_colour );
		}
		else
		{
			current_string[out_idx] = p_string[in_idx];
			out_idx++;
		}
	}

	// Flush the current string and restore the colour
	current_string[out_idx] = 0;
	DisplayString( current_string );

	SetCurrentTerminalColour( TC_DEFAULT );
}

//*****************************************************************************
//
//*****************************************************************************
IDebugConsole::IDebugConsole()
:	mDebugScreenEnabled( true )
,	mCurrentY( 0 )
{
	pspDebugScreenSetXY(0, 0);
}

//*****************************************************************************
//
//*****************************************************************************
IDebugConsole::~IDebugConsole()
{
}

//*****************************************************************************
//
//*****************************************************************************
void	IDebugConsole::EnableConsole( bool bEnable )
{
	if( !mDebugScreenEnabled && bEnable )
	{
		pspDebugScreenSetXY(0, 0);
	}
	mDebugScreenEnabled = bEnable;
}

//*****************************************************************************
//
//*****************************************************************************
bool	IDebugConsole::IsVisible() const
{
	return mDebugScreenEnabled;
}

//*****************************************************************************
//
//*****************************************************************************
void	IDebugConsole::UpdateDisplay()
{
}

//*****************************************************************************
//
//*****************************************************************************
void DAEDALUS_VARARG_CALL_TYPE	IDebugConsole::Msg(u32 type, const char * szFormat, ...)
{
	va_list			va;

	// Format the output
	va_start( va, szFormat );
	// Don't use wvsprintf as it doesn't handle floats!
	vsprintf( mFormattingBuffer, szFormat, va );
	va_end( va );

	ParseAndDisplayString( mFormattingBuffer, TC_w );
	DisplayString( "\n" );
}

//*****************************************************************************
//
//*****************************************************************************
void	IDebugConsole::MsgOverwriteStart()
{
	mCurrentY = pspDebugScreenGetY();
	printf( TERMINAL_SAVE_CURSOR );
}

//*****************************************************************************
//
//*****************************************************************************
void DAEDALUS_VARARG_CALL_TYPE	IDebugConsole::MsgOverwrite(u32 type, const char * szFormat, ...)
{
	va_list			va;

	// Format the output
	va_start( va, szFormat );
	// Don't use wvsprintf as it doesn't handle floats!
	vsprintf( mFormattingBuffer, szFormat, va );
	va_end( va );

	pspDebugScreenSetXY(0, mCurrentY);
#ifdef DAEDALUS_USETERMINAL
	printf( "%s%s", TERMINAL_ERASE_LINE, TERMINAL_RESTORE_CURSOR );
#endif

	ParseAndDisplayString( mFormattingBuffer, TC_w );
	//DisplayString( "\n" );
	if( mDebugScreenEnabled )
	{
		pspDebugScreenPrintf( "\n" );
	}
}

//*****************************************************************************
//
//*****************************************************************************
void	IDebugConsole::MsgOverwriteEnd()
{
	printf( "\n" );		// Final newline for the terminal
}
#endif //DAEDALUS_DEBUG_CONSOLE
