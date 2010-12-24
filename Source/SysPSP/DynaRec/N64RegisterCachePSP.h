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

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef N64REGISTERCACHEPSP_H_
#define N64REGISTERCACHEPSP_H_

#include "DynarecTargetPSP.h"

//*************************************************************************************
//
//*************************************************************************************
class CN64RegisterCachePSP
{
public:
		CN64RegisterCachePSP();

		void				Reset();

//		void				SetCachedReg( EN64Reg n64_reg, u32 lo_hi_idx, EPspReg psp_reg );
inline void	SetCachedReg( EN64Reg n64_reg, u32 lo_hi_idx, EPspReg psp_reg ) {	mRegisterCacheInfo[ n64_reg ][ lo_hi_idx ].PspRegister = psp_reg; }

		void				ClearCachedReg( EN64Reg n64_reg, u32 lo_hi_idx );


//		bool				IsCached( EN64Reg reg, u32 lo_hi_idx ) const;
inline bool	IsCached( EN64Reg reg, u32 lo_hi_idx ) const {	return mRegisterCacheInfo[ reg ][ lo_hi_idx ].PspRegister != PspReg_R0; }

//		bool				IsValid( EN64Reg reg, u32 lo_hi_idx ) const;
inline bool	IsValid( EN64Reg reg, u32 lo_hi_idx ) const
{
	DAEDALUS_ASSERT( !mRegisterCacheInfo[ reg ][ lo_hi_idx ].Valid || IsCached( reg, lo_hi_idx ), "Checking register is valid but uncached?" );

	return mRegisterCacheInfo[ reg ][ lo_hi_idx ].Valid;
}
		bool				IsDirty( EN64Reg reg, u32 lo_hi_idx ) const;

//		bool				IsTemporary( EN64Reg reg, u32 lo_hi_idx ) const;
inline bool	IsTemporary( EN64Reg reg, u32 lo_hi_idx ) const
{
	if( IsCached( reg, lo_hi_idx ) )
	{
		return PspReg_IsTemporary( mRegisterCacheInfo[ reg ][ lo_hi_idx ].PspRegister );
	}

	return false;
}

//		EPspReg				GetCachedReg( EN64Reg reg, u32 lo_hi_idx ) const;
inline EPspReg	GetCachedReg( EN64Reg reg, u32 lo_hi_idx ) const
{
	DAEDALUS_ASSERT( IsCached( reg, lo_hi_idx ), "Trying to retreive an uncached register" );

	return mRegisterCacheInfo[ reg ][ lo_hi_idx ].PspRegister;
}

//		void				MarkAsValid( EN64Reg reg, u32 lo_hi_idx, bool valid );
inline void	MarkAsValid( EN64Reg reg, u32 lo_hi_idx, bool valid )
{
	DAEDALUS_ASSERT( IsCached( reg, lo_hi_idx ), "Changing valid flag on uncached register?" );

	mRegisterCacheInfo[ reg ][ lo_hi_idx ].Valid = valid;
}

//		void				MarkAsDirty( EN64Reg reg, u32 lo_hi_idx, bool dirty );
inline void	MarkAsDirty( EN64Reg reg, u32 lo_hi_idx, bool dirty )
{
#ifdef DAEDALUS_ENABLE_ASSERTS
	if( dirty )
	{ 	 
		 DAEDALUS_ASSERT( IsKnownValue( reg, lo_hi_idx ) || IsCached( reg, lo_hi_idx ), "Setting dirty flag on unknown/uncached register?" ); 	 
	}
#endif
	mRegisterCacheInfo[ reg ][ lo_hi_idx ].Dirty = dirty;
}

//		bool				IsKnownValue( EN64Reg reg, u32 lo_hi_idx ) const;
inline bool	IsKnownValue( EN64Reg reg, u32 lo_hi_idx ) const {	return mRegisterCacheInfo[ reg ][ lo_hi_idx ].Known; }

//		void				SetKnownValue( EN64Reg reg, u32 lo_hi_idx, s32 value );
inline void	SetKnownValue( EN64Reg reg, u32 lo_hi_idx, s32 value )
{
	mRegisterCacheInfo[ reg ][ lo_hi_idx ].Known = true;
	mRegisterCacheInfo[ reg ][ lo_hi_idx ].KnownValue._u32 = value;
}

//		void				ClearKnownValue( EN64Reg reg, u32 lo_hi_idx );
inline void	ClearKnownValue( EN64Reg reg, u32 lo_hi_idx ) {	mRegisterCacheInfo[ reg ][ lo_hi_idx ].Known = false; }

//		REG32				GetKnownValue( EN64Reg reg, u32 lo_hi_idx ) const;
inline REG32	GetKnownValue( EN64Reg reg, u32 lo_hi_idx ) const {	return mRegisterCacheInfo[ reg ][ lo_hi_idx ].KnownValue; }

//		bool				IsFPValid( EN64FloatReg reg ) const;
inline bool	IsFPValid( EN64FloatReg reg ) const {	return mFPRegisterCacheInfo[ reg ].Valid; }

//		bool				IsFPDirty( EN64FloatReg reg ) const;
inline bool	IsFPDirty( EN64FloatReg reg ) const {	return mFPRegisterCacheInfo[ reg ].Dirty; }

//		void				MarkFPAsValid( EN64FloatReg reg, bool valid );
inline void	MarkFPAsValid( EN64FloatReg reg, bool valid ) {	mFPRegisterCacheInfo[ reg ].Valid = valid; }

//	void				MarkFPAsDirty( EN64FloatReg reg, bool dirty );
inline void	MarkFPAsDirty( EN64FloatReg reg, bool dirty ) {	mFPRegisterCacheInfo[ reg ].Dirty = dirty; }

private:

		struct RegisterCacheInfoPSP
		{
			EPspReg			PspRegister;		// If cached, this is the psp register we're using
			bool			Valid;				// Is the contents of the register valid?
			bool			Dirty;				// Are the contents of the register modified?

			bool			Known;				// Are the contents known
			//bool			SignExtended;		// Is this (high) register just sign extension of low reg?
			REG32			KnownValue;			// The contents (if known)
		};

		// PSP fp registers are stored in a 1:1 mapping with the n64 counterparts
		struct FPRegisterCacheInfoPSP
		{
			bool			Valid;
			bool			Dirty;
		};

		RegisterCacheInfoPSP	mRegisterCacheInfo[ NUM_N64_REGS ][ 2 ];
		FPRegisterCacheInfoPSP	mFPRegisterCacheInfo[ NUM_N64_FP_REGS ];
};

#endif
