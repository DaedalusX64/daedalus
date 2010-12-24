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

#ifndef __ULTRA_ABI_H__
#define __ULTRA_ABI_H__


// Audio commands:
#define	A_SPNOOP				0
#define	A_ADPCM					1
#define	A_CLEARBUFF				2
#define	A_ENVMIXER				3
#define	A_LOADBUFF				4
#define	A_RESAMPLE				5
#define A_SAVEBUFF				6
#define A_SEGMENT				7
#define A_SETBUFF				8
#define A_SETVOL				9
#define A_DMEMMOVE              10
#define A_LOADADPCM             11
#define A_MIXER					12
#define A_INTERLEAVE            13
#define A_POLEF                 14
#define A_SETLOOP               15

//
// Audio flags
//

#define A_INIT			0x01
#define A_CONTINUE		0x00
#define A_LOOP          0x02
#define A_OUT           0x02
#define A_LEFT			0x02
#define	A_RIGHT			0x00
#define A_VOL			0x04
#define A_RATE			0x00
#define A_AUX			0x08
#define A_NOAUX			0x00
#define A_MAIN			0x00
#define A_MIX			0x10


#endif // __ULTRA_ABI_H__
