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
		}
	}

	for( u32 i = 0; i < NUM_N64_FP_REGS; ++i )
	{
		mFPRegisterCacheInfo[ i ].Valid = false;
		mFPRegisterCacheInfo[ i ].Dirty = false;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void	CN64RegisterCachePSP::SetCachedReg( EN64Reg n64_reg, u32 lo_hi_idx, EPspReg psp_reg )
{
	mRegisterCacheInfo[ n64_reg ][ lo_hi_idx ].PspRegister = psp_reg;
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
}

//*****************************************************************************
//
//*****************************************************************************
bool	CN64RegisterCachePSP::IsCached( EN64Reg reg, u32 lo_hi_idx ) const
{
	return mRegisterCacheInfo[ reg ][ lo_hi_idx ].PspRegister != PspReg_R0;
}

//*****************************************************************************
//
//*****************************************************************************
EPspReg		CN64RegisterCachePSP::GetCachedReg( EN64Reg reg, u32 lo_hi_idx ) const
{
	DAEDALUS_ASSERT( IsCached( reg, lo_hi_idx ), "Trying to retreive an uncached register" );

	return mRegisterCacheInfo[ reg ][ lo_hi_idx ].PspRegister;
}

//*****************************************************************************
//
//*****************************************************************************
bool	CN64RegisterCachePSP::IsTemporary( EN64Reg reg, u32 lo_hi_idx ) const
{
	if( IsCached( reg, lo_hi_idx ) )
	{
		return PspReg_IsTemporary( mRegisterCacheInfo[ reg ][ lo_hi_idx ].PspRegister );
	}

	return false;
}

//*****************************************************************************
//
//*****************************************************************************
bool	CN64RegisterCachePSP::IsValid( EN64Reg reg, u32 lo_hi_idx ) const
{
	DAEDALUS_ASSERT( !mRegisterCacheInfo[ reg ][ lo_hi_idx ].Valid || IsCached( reg, lo_hi_idx ), "Checking register is valid but uncached?" );

	return mRegisterCacheInfo[ reg ][ lo_hi_idx ].Valid;
}

//*****************************************************************************
//
//*****************************************************************************
bool	CN64RegisterCachePSP::IsDirty( EN64Reg reg, u32 lo_hi_idx ) const
{
#ifdef DAEDALUS_ENABLE_ASSERTS
	bool	is_dirty( mRegisterCacheInfo[ reg ][ lo_hi_idx ].Dirty );

	if( is_dirty )
	{
		DAEDALUS_ASSERT( IsKnownValue( reg, lo_hi_idx ) || IsCached( reg, lo_hi_idx ), "Checking dirty flag on unknown/uncached register?" );
	}

	return is_dirty;
#else
	return mRegisterCacheInfo[ reg ][ lo_hi_idx ].Dirty;
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void	CN64RegisterCachePSP::MarkAsValid( EN64Reg reg, u32 lo_hi_idx, bool valid )
{
	DAEDALUS_ASSERT( IsCached( reg, lo_hi_idx ), "Changing valid flag on uncached register?" );

	mRegisterCacheInfo[ reg ][ lo_hi_idx ].Valid = valid;
}

//*****************************************************************************
//
//*****************************************************************************
void	CN64RegisterCachePSP::MarkAsDirty( EN64Reg reg, u32 lo_hi_idx, bool dirty )
{
#ifdef DAEDALUS_ENABLE_ASSERTS
	if( dirty )
	{ 	 
		 DAEDALUS_ASSERT( IsKnownValue( reg, lo_hi_idx ) || IsCached( reg, lo_hi_idx ), "Setting dirty flag on unknown/uncached register?" ); 	 
	}
#endif
	mRegisterCacheInfo[ reg ][ lo_hi_idx ].Dirty = dirty;
}

//*****************************************************************************
//
//*****************************************************************************
bool	CN64RegisterCachePSP::IsKnownValue( EN64Reg reg, u32 lo_hi_idx ) const
{
	return mRegisterCacheInfo[ reg ][ lo_hi_idx ].Known;
}

//*****************************************************************************
//
//*****************************************************************************
void	CN64RegisterCachePSP::SetKnownValue( EN64Reg reg, u32 lo_hi_idx, s32 value )
{
	mRegisterCacheInfo[ reg ][ lo_hi_idx ].Known = true;
	mRegisterCacheInfo[ reg ][ lo_hi_idx ].KnownValue._u32 = value;
}

//*****************************************************************************
//
//*****************************************************************************
void	CN64RegisterCachePSP::ClearKnownValue( EN64Reg reg, u32 lo_hi_idx )
{
	mRegisterCacheInfo[ reg ][ lo_hi_idx ].Known = false;
}

//*****************************************************************************
//
//*****************************************************************************
REG32		CN64RegisterCachePSP::GetKnownValue( EN64Reg reg, u32 lo_hi_idx ) const
{
	return mRegisterCacheInfo[ reg ][ lo_hi_idx ].KnownValue;
}

//*****************************************************************************
//
//*****************************************************************************
bool	CN64RegisterCachePSP::IsFPValid( EN64FloatReg reg ) const
{
	return mFPRegisterCacheInfo[ reg ].Valid;
}

//*****************************************************************************
//
//*****************************************************************************
bool	CN64RegisterCachePSP::IsFPDirty( EN64FloatReg reg ) const
{
	return mFPRegisterCacheInfo[ reg ].Dirty;
}

//*****************************************************************************
//
//*****************************************************************************
void	CN64RegisterCachePSP::MarkFPAsValid( EN64FloatReg reg, bool valid )
{
	mFPRegisterCacheInfo[ reg ].Valid = valid;
}

//*****************************************************************************
//
//*****************************************************************************
void	CN64RegisterCachePSP::MarkFPAsDirty( EN64FloatReg reg, bool dirty )
{
	mFPRegisterCacheInfo[ reg ].Dirty = dirty;
}
