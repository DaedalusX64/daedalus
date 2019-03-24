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

#ifndef DYNAREC_ASSEMBLYUTILS_H_
#define DYNAREC_ASSEMBLYUTILS_H_

#include <stdlib.h>

#include "Utility/DaedalusTypes.h"

class CCodeLabel
{
public:
	CCodeLabel () : mpLocation( NULL ) {}
	explicit CCodeLabel( const void * p_location )
		:	mpLocation( p_location )
	{
	}

	bool			IsSet() const				{ return mpLocation != NULL; }
	const void *	GetTarget() const			{ return mpLocation; }
	const u8 *		GetTargetU8P() const		{ return reinterpret_cast< const u8 * >( mpLocation ); }
	u32				GetTargetU32() const		{ return reinterpret_cast< u32 >( mpLocation ); }



private:
	const void *	mpLocation;		// This is the location of an arbitrary block of code
};

class CJumpLocation
{
public:
	CJumpLocation () : mpLocation( NULL ) {}

	explicit CJumpLocation( void * p_location )
		:	mpLocation( p_location )
	{
	}

	bool			IsSet() const				{ return mpLocation != NULL; }
	s32				GetOffset( const CCodeLabel & label ) const	{ return label.GetTargetU8P() - GetTargetU8P();	}

	const u8 *		GetTargetU8P() const		{ return reinterpret_cast< const u8 * >( mpLocation ); }
	u8 *			GetWritableU8P() const
	{
		//Todo: PSP
		return reinterpret_cast< u8 * >( MAKE_UNCACHED_PTR(mpLocation) );
		//Todo: Check this
		//return reinterpret_cast< u8 * >( mpLocation );
	}

private:
	void *			mpLocation;		// This is the location of a jump instruction
};


namespace AssemblyUtils
{
	bool		PatchJumpLong( CJumpLocation jump, CCodeLabel target );
	bool		PatchJumpLongAndFlush( CJumpLocation jump, CCodeLabel target );
	void		ReplaceBranchWithJump( CJumpLocation branch, CCodeLabel target );
}

#endif // DYNAREC_ASSEMBLYUTILS_H_
