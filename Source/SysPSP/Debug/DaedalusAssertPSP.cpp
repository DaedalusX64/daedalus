/*

  Copyright (C) 2001 StrmnNrmn

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
#include "Debug/DaedalusAssert.h"

#ifdef DAEDALUS_ENABLE_ASSERTS

#include <pspdebug.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspsysmem.h>

#define PSP_LINE_SIZE 512
#define PSP_PIXEL_FORMAT 3
#include <pspdisplay.h>

#include <stdarg.h>

DaedalusAssertHook gAssertHook = NULL;

//
//	Return -1 to ignore once, 0 to ignore permenantly, 1 to break
//
EAssertResult DaedalusAssert( const char * expression, const char * file, unsigned int line, const char * msg, ... )
{
	char buffer[ 1024 ];
	va_list va;
	va_start(va, msg);
	vsnprintf( buffer, 1024, msg, va );
	buffer[1023] = 0;
	va_end(va);

	const u32 blue = 0xffff0000;
	const u32 white = 0xffffffff;
	const u32 black = 0xff000000;

	//
	//	Enter the debug menu as soon as select is newly pressed
	//
    SceCtrlData pad;

	static u32 oldButtons = 0;

	sceCtrlPeekBufferPositive(&pad, 1);
	oldButtons = pad.Buttons;

	bool			button_pressed( false );
	EAssertResult	result( AR_IGNORE );

	void * p_base_address( sceGeEdramGetAddr() );
	void * p_vram_base = MAKE_UNCACHED_PTR(p_base_address);
	sceDisplaySetFrameBuf( p_vram_base, PSP_LINE_SIZE, PSP_PIXEL_FORMAT, PSP_DISPLAY_SETBUF_IMMEDIATE );
	pspDebugScreenSetOffset( 0 );

	pspDebugScreenSetXY(0, 0);

	pspDebugScreenSetBackColor( blue );
	pspDebugScreenSetTextColor( white );

	pspDebugScreenPrintf( "************************************************************\n" );
	pspDebugScreenPrintf( "Assert Failed: %s\n", expression );
	//pspDebugScreenPrintf( "MemFree: Total - %d, Max - %d\n", sceKernelTotalFreeMemSize(), sceKernelMaxFreeMemSize() );
	pspDebugScreenPrintf( "Location: %s(%d)\n", file, line );
	pspDebugScreenPrintf( "\n" );
	pspDebugScreenPrintf( "%s\n", buffer );
	pspDebugScreenPrintf( "\n" );
	pspDebugScreenPrintf( "Press X to ignore once, [] to ignore forever, O to break\n" );
	pspDebugScreenPrintf( "************************************************************\n" );


	printf( "************************************************************\n" );
	printf( "Assert Failed: %s\n", expression );
	//printf( "MemFree: Total - %d, Max - %d\n", sceKernelTotalFreeMemSize(), sceKernelMaxFreeMemSize() );
	printf( "Location: %s(%d)\n", file, line );
	printf( "\n" );
	printf( "%s\n", buffer );
	printf( "\n" );
	printf( "Press X to ignore once, [] to ignore forever, O to break\n" );
	printf( "************************************************************\n" );


	pspDebugScreenSetBackColor( black );
	pspDebugScreenSetTextColor( white );

	// Remain paused until the Select button is pressed again
	while(!button_pressed)
	{
		sceCtrlPeekBufferPositive(&pad, 1);
		if(oldButtons != pad.Buttons)
		{
			if(pad.Buttons & PSP_CTRL_CROSS)
			{
				button_pressed = true;
				result = AR_IGNORE_ONCE;
			}
			if(pad.Buttons & PSP_CTRL_SQUARE)
			{
				button_pressed = true;
				result = AR_IGNORE;
			}
			if(pad.Buttons & PSP_CTRL_CIRCLE)
			{
				button_pressed = true;
				result = AR_BREAK;
			}
		}

		oldButtons = pad.Buttons;
		//sceDisplayWaitVblankStart();
		//sceGuSwapBuffers();
	}


	//
	//	Wait until all buttons are release before continuing
	//
	while( pad.Buttons != 0 )
	{
		sceCtrlPeekBufferPositive(&pad, 1);
	}

	return result;
}
#endif //DAEDALUS_ENABLE_ASSERTS