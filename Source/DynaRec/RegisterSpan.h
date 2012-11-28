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

#ifndef __DAEDALUS_REGISTERSPAN_H__
#define __DAEDALUS_REGISTERSPAN_H__

#include "Core/N64Reg.h"

#include <vector>

//*************************************************************************************
//
//*************************************************************************************
struct SRegisterSpan
{
	SRegisterSpan( EN64Reg reg, u32 start, u32 end )
		:	Register( reg )
		,	SpanStart( start )
		,	SpanEnd( end )
	{
	}

	EN64Reg			Register;

	u32				SpanStart;
	u32				SpanEnd;
};
typedef std::vector<SRegisterSpan>		RegisterSpanList;


//*************************************************************************************
//
//*************************************************************************************
struct SAscendingSpanStartSort
{
     bool operator()(const SRegisterSpan & a, const SRegisterSpan & b)
     {
          return a.SpanStart < b.SpanStart;
     }
};

//*************************************************************************************
//
//*************************************************************************************
struct SAscendingSpanEndSort
{
     bool operator()(const SRegisterSpan & a, const SRegisterSpan & b)
     {
          return a.SpanEnd < b.SpanEnd;
     }
};

//*************************************************************************************
//
//*************************************************************************************
struct SRegisterUsageInfo
{
	RegisterSpanList		SpanList;
	u32						RegistersRead;			// Bitmask of registers which are read from.
	u32						RegistersWritten;
	u32						RegistersAsBases;

	SRegisterUsageInfo()
		:	RegistersRead( 0 )
		,	RegistersWritten( 0 )
		,	RegistersAsBases( 0 )
	{
	}

	inline bool IsRead( EN64Reg reg ) const			{ return (RegistersRead >> reg) & 1; }
	inline bool IsModified( EN64Reg reg ) const		{ return (RegistersWritten >> reg) & 1; }
	inline bool IsBase( EN64Reg reg ) const			{ return (RegistersAsBases >> reg) & 1; }
};


#endif
