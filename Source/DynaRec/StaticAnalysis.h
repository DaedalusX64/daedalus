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
#pragma once

#include "Core/Memory.h"
struct OpCode;

namespace StaticAnalysis
{
	enum MemAcess
	{
		Segment_Unknown = 0,
		Segment_8000 = 1,
		Segment_A000 = 2
	};

	struct RegSrcUse
	{
		explicit RegSrcUse( u32 r ) : Reg( r ) {}

		u32		Reg;
	};

	struct RegDstUse
	{
		explicit RegDstUse( u32 r ) : Reg( r ) {}

		u32		Reg;
	};

	struct RegBaseUse
	{
		explicit RegBaseUse( u32 r ) : Reg( r ) {}

		u32		Reg;
	};
	struct RegisterUsage
	{
		u32			RegReads;
		u32			RegWrites;
		u32			RegBase;
		MemAcess    Memory;

		RegisterUsage()
			:	RegReads( 0 )
			,	RegWrites( 0 )
			,	RegBase( 0 )
			,   Memory( Segment_Unknown )
		{
		}

		// Ignore floating point for now
		void		Record( RegDstUse d, RegSrcUse s, RegSrcUse t )
		{
			RegWrites = (1<<d.Reg);
			RegReads = (1<<s.Reg) | (1<<t.Reg);
		}
		void		Record( RegDstUse d, RegSrcUse s )
		{
			RegWrites = (1<<d.Reg);
			RegReads = (1<<s.Reg);
		}
		void		Record( RegSrcUse s, RegSrcUse t )
		{
			RegReads = (1<<s.Reg) | (1<<t.Reg);
		}
		void		Record( RegSrcUse s )
		{
			RegReads = (1<<s.Reg);
		}
		void		Record( RegDstUse d )
		{
			RegWrites = (1<<d.Reg);
		}
		void		Record( RegBaseUse b, RegDstUse d )
		{
			RegWrites = (1<<d.Reg);
			RegBase = (1<<b.Reg);
		}
		void		Record( RegBaseUse b, RegSrcUse s )
		{
			RegReads = (1<<s.Reg);
			RegBase = (1<<b.Reg);
		}

		void		Record( RegBaseUse b )
		{
			RegBase = (1<<b.Reg);
		}

		void		Access(u32 address)
		{
			if (address >= 0x80000000 && address < 0x80000000 + gRamSize)
			{
				  Memory = Segment_8000;
			}
			else if (address >= 0xA0000000 && address < 0xA0000000 + gRamSize)
			{
				  Memory = Segment_A000;
			}

			DAEDALUS_ASSERT(!(RegBase == 1 << 29 && Memory == Segment_Unknown), "Why Stack Point to static segment: 0x%08x", address);
		}
			
	};

	void		Analyse( OpCode op_code, RegisterUsage & reg_usage );
}
