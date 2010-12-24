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

#ifndef __RSP_H__
#define __RSP_H__

ALIGNED_TYPE(struct, SRSPState, CACHE_ALIGN)
{
	REG32			CPU[32];				// 0x000 .. 0x100
	REG32			CPUControl[32];			// 0x100 .. 0x200
	VECTOR			COP2[32];
	REG64			Accumulator[8];
	REG32			COP2Control[4];
	u32				CurrentPC;			// 0x400 ..			The current program counter
	u32				TargetPC;			// 0x404 ..			The PC to branch to
	u32				Delay;				// 0x408 ..			Delay state (NO_DELAY, EXEC_DELAY, DO_DELAY)
};

ALIGNED_EXTERN(SRSPState, gRSPState, CACHE_ALIGN);

extern volatile bool gRSPHLEActive;

bool RSP_IsRunning();		// Returns true if the rsp is running either LLE or HLE
bool RSP_IsRunningLLE();	// Returns true if the rsp is running with LLE
bool RSP_IsRunningHLE();	// Returns true if the rsp is running with HLE

#if 0
void RSP_DumpVector(u32 reg);
void RSP_DumpVectors(u32 reg1, u32 reg2, u32 reg3);
#endif

#endif
