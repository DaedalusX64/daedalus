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

#ifndef __DAEDALUS_TRACE_H__
#define __DAEDALUS_TRACE_H__

#include "Core/R4300OpCode.h"
#include "StaticAnalysis.h"

struct STraceEntry
{
	u32					Address;
	struct OpCode		OpCode;
	StaticAnalysis::RegisterUsage		Usage;
	u32					BranchIdx;
	bool				BranchDelaySlot;
};

enum SpeedHackProbe
{
	SHACK_NONE,
	SHACK_POSSIBLE,
	SHACK_SKIPTOEVENT,
	SHACK_COPYREG
};

struct SBranchDetails
{
	SBranchDetails( )
		:	TargetAddress( u32(~0) )
		,	DelaySlotTraceIndex( -1 )
		,	ConditionalBranchTaken( false )
		,	Likely( false )
		,	Direct( false )
		,	Eret( false )
		,	SpeedHack( SHACK_NONE )
	{
	}

	u32					TargetAddress;

	s32					DelaySlotTraceIndex;
	bool				ConditionalBranchTaken;
	bool				Likely;
	bool				Direct;
	bool				Eret;
	SpeedHackProbe		SpeedHack;
};

#endif // __DAEDALUS_TRACE_H__
