/*
Copyright (C) 2012 StrmnNrmn

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
#include "DebugConsoleImpl.h"

ETerminalColour GetTerminalColour(char c)
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

const char * const gTerminalColours[NUM_TERMINAL_COLOURS] =
{
	"\033[0m",		// TC_DEFAULT
	"\033[0;31m", "\033[0;32m", "\033[0;33m", "\033[0;34m", "\033[0;35m", "\033[0;36m", "\033[0;37m",
	"\033[1;31m", "\033[1;32m", "\033[1;33m", "\033[1;34m", "\033[1;35m", "\033[1;36m", "\033[1;37m",
	"",		// TC_INVALID
};

const char * GetTerminalColourString(ETerminalColour tc)
{
	return gTerminalColours[tc];
}
