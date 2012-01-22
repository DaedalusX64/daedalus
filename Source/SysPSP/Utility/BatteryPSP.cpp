/*
Copyright (C) 2010 Salvy6735 / psppwner300 / Corn

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

#include "SysPSP/Graphics/DrawText.h"
#include "Utility/Translate.h"

#include <pspdebug.h>
#include <psprtc.h>
#include <psppower.h>
//*****************************************************************************
//
//*****************************************************************************
void battery_info()
{	
    pspTime s;
    sceRtcGetCurrentClockLocalTime(&s);
	s32 bat = scePowerGetBatteryLifePercent();

	// Meh should be big enough regarding if translated..
	char					time[30], batt[30], remaining[30];

	sprintf(time,"%s:  %d:%02d%c%02d",Translate_String("Time"), s.hour, s.minutes, (s.seconds&1?':':' '), s.seconds );

	CDrawText::Render( CDrawText::F_REGULAR,22, 43, 0.9f, time, DrawTextUtilities::TextWhite );

	if(!scePowerIsBatteryCharging())
	{
		s32 batteryLifeTime = scePowerGetBatteryLifeTime();

		sprintf(batt,"%s:  %d%% | %0.2fV | %dC",Translate_String("Battery"), bat, (f32) scePowerGetBatteryVolt() / 1000.0f, scePowerGetBatteryTemp() );
		sprintf(remaining,"%s: %2dh %2dm",Translate_String("Remaining"), batteryLifeTime / 60, batteryLifeTime - 60 * (batteryLifeTime / 60));


		CDrawText::Render( CDrawText::F_REGULAR, 140, 43, 0.9f, batt, DrawTextUtilities::TextWhite );
		CDrawText::Render( CDrawText::F_REGULAR, 335, 43, 0.9f, remaining, DrawTextUtilities::TextWhite );
	}
	else
	{
		CDrawText::Render( CDrawText::F_REGULAR, 210, 43, 0.9f, "Charging...", DrawTextUtilities::TextWhite );
		CDrawText::Render( CDrawText::F_REGULAR, 335, 43, 0.9f, "Remaining: --h--m", DrawTextUtilities::TextWhite );
	}
}

//*****************************************************************************
//
//*****************************************************************************
void battery_warning()
{
	if( scePowerIsBatteryCharging() ) return;
	int bat = scePowerGetBatteryLifePercent();
	if (bat > 9) return;	//No warning unless battery is under 10%

	static u32 counter = 0;

	if ((++counter & 63) < 50)	// Make it flash
	{
		const u32 red	=	0x000000ff;	//  Red..
		const u32 white =	0xffffffff;	//  White..

		pspDebugScreenSetXY(50, 0);		// Allign to the left, becareful not touch the edges
		pspDebugScreenSetBackColor( red );
		pspDebugScreenSetTextColor( white );
		pspDebugScreenPrintf( " Battery Low: %d%% ", bat);
	}
}

//*****************************************************************************
//
//*****************************************************************************