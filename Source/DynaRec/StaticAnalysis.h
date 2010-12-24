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

		RegisterUsage()
			:	RegReads( 0 )
			,	RegWrites( 0 )
			,	RegBase( 0 )
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
			
	};

	void		Analyse( OpCode op_code, RegisterUsage & reg_usage );
}
