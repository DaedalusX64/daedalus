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

#ifndef SPINLOCK_H__
#define SPINLOCK_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Utility/Thread.h"

class CSpinLock
{
public:
	inline explicit CSpinLock( volatile u32 * var ) : Var( var )
	{
		// N.B. - this probably needs to use a CAS to prevent race conditions
		while( *Var != 0 )
		{
			ThreadYield();
		}
		*Var = 1;
	}

	inline ~CSpinLock()
	{
		*Var = 0;
	}

private:
	volatile u32 * Var;
};

#endif // SPINLOCK_H__
