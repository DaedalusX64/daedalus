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

#ifndef DYNAREC_STATICANALYSIS_H_
#define DYNAREC_STATICANALYSIS_H_

#include "BranchType.h"
#include "Core/Memory.h"

struct OpCode;

namespace StaticAnalysis
{
	/*enum MemAcess
	{
		Segment_Unknown = 0,
		Segment_8000 = 1,
		Segment_A000 = 2
	};*/

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
		ER4300BranchType BranchType;
		bool		Access8000;

		RegisterUsage()
			:	RegReads( 0 )
			,	RegWrites( 0 )
			,	RegBase( 0 )
			,   BranchType( BT_NOT_BRANCH )
			,   Access8000( false )
		{
		}

		// Ignore floating point for now
		inline void		Record( RegDstUse d, RegSrcUse s, RegSrcUse t )
		{
			RegWrites = (1<<d.Reg);
			RegReads = (1<<s.Reg) | (1<<t.Reg);
		}
		void		Record( RegDstUse d, RegSrcUse s )
		{
			RegWrites = (1<<d.Reg);
			RegReads = (1<<s.Reg);
		}
		inline void		Record( RegSrcUse s, RegSrcUse t )
		{
			RegReads = (1<<s.Reg) | (1<<t.Reg);
		}
		inline void		Record( RegSrcUse s )
		{
			RegReads = (1<<s.Reg);
		}
		inline void		Record( RegDstUse d )
		{
			RegWrites = (1<<d.Reg);
		}
		inline void		Record( RegBaseUse b, RegDstUse d )
		{
			RegWrites = (1<<d.Reg);
			RegBase = (1<<b.Reg);
		}
		inline void		Record( RegBaseUse b, RegSrcUse s )
		{
			RegReads = (1<<s.Reg);
			RegBase = (1<<b.Reg);
		}

		inline void		Record( RegBaseUse b )
		{
			RegBase = (1<<b.Reg);
		}

		inline void	BranchOP(ER4300BranchType type)
		{
			BranchType = type;
		}

		inline void		Access(u32 address)
		{
			Access8000 = ((address >> 23) == 0x100);
			#ifdef DAEDALUS_ENABLE_ASSERTS
			DAEDALUS_ASSERT((address >= 0x80000000 && address < 0x80000000 + gRamSize) == Access8000, "Access8000 is inconsistent");
			#endif
		}

	};

	void		Analyse( OpCode op_code, RegisterUsage & reg_usage );
}

#endif // DYNAREC_STATICANALYSIS_H_
