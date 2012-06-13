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

#include "stdafx.h"
#include "N64RegisterCachePSP.h"

//*****************************************************************************
//
//*****************************************************************************
CN64RegisterCachePSP::CN64RegisterCachePSP()
{
	Reset();
}

//*****************************************************************************
//
//*****************************************************************************
// ToDo: This very primitive, implement a more effective assumption to detect if XX reg is mapped as 32bit
bool CN64RegisterCachePSP::IsReg32bit( s32 lo, s32 hi )
{
	if (lo < 0 && hi == -1)
	{ 
		return true;
	} 

	if (lo >= 0 && hi == 0)
	{ 
		return true;
	} 

	return false;
}

//*****************************************************************************
//
//*****************************************************************************
void	CN64RegisterCachePSP::Reset()
{
	for( u32 lo_hi_idx = 0; lo_hi_idx < 2; ++lo_hi_idx )
	{
		for( u32 i = 0; i < NUM_N64_REGS; ++i )
		{
			mRegisterCacheInfo[ i ][ lo_hi_idx ].PspRegister = PspReg_R0;
			mRegisterCacheInfo[ i ][ lo_hi_idx ].Valid = false;
			mRegisterCacheInfo[ i ][ lo_hi_idx ].Dirty = false;
			mRegisterCacheInfo[ i ][ lo_hi_idx ].Known = false;
			mRegisterCacheInfo[ i ][ lo_hi_idx ].Is32bit	 = false;
		}
	}

	for( u32 i = 0; i < NUM_N64_FP_REGS; ++i )
	{
		mFPRegisterCacheInfo[ i ].Valid = false;
		mFPRegisterCacheInfo[ i ].Dirty = false;
		mFPRegisterCacheInfo[ i ].Sim = false;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void	CN64RegisterCachePSP::ClearCachedReg( EN64Reg n64_reg, u32 lo_hi_idx )
{
	DAEDALUS_ASSERT( IsCached( n64_reg, lo_hi_idx ), "This register is not currently cached" );
	DAEDALUS_ASSERT( !IsDirty( n64_reg, lo_hi_idx ), "This register is being cleared while still dirty" );

	mRegisterCacheInfo[ n64_reg ][ lo_hi_idx ].PspRegister = PspReg_R0;
	mRegisterCacheInfo[ n64_reg ][ lo_hi_idx ].Valid = false;
	mRegisterCacheInfo[ n64_reg ][ lo_hi_idx ].Dirty = false;
	mRegisterCacheInfo[ n64_reg ][ lo_hi_idx ].Known = false;
	mRegisterCacheInfo[ n64_reg ][ lo_hi_idx ].Is32bit	 = false;
}
