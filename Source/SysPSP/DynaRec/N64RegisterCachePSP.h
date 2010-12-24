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

		void				SetCachedReg( EN64Reg n64_reg, u32 lo_hi_idx, EPspReg psp_reg );
		void				ClearCachedReg( EN64Reg n64_reg, u32 lo_hi_idx );

		bool				IsCached( EN64Reg reg, u32 lo_hi_idx ) const;
		bool				IsValid( EN64Reg reg, u32 lo_hi_idx ) const;
		bool				IsDirty( EN64Reg reg, u32 lo_hi_idx ) const;
		bool				IsTemporary( EN64Reg reg, u32 lo_hi_idx ) const;
		EPspReg				GetCachedReg( EN64Reg reg, u32 lo_hi_idx ) const;

		void				MarkAsValid( EN64Reg reg, u32 lo_hi_idx, bool valid );
		void				MarkAsDirty( EN64Reg reg, u32 lo_hi_idx, bool dirty );

		bool				IsKnownValue( EN64Reg reg, u32 lo_hi_idx ) const;
		void				SetKnownValue( EN64Reg reg, u32 lo_hi_idx, s32 value );
		void				ClearKnownValue( EN64Reg reg, u32 lo_hi_idx );
		REG32				GetKnownValue( EN64Reg reg, u32 lo_hi_idx ) const;


		bool				IsFPValid( EN64FloatReg reg ) const;
		bool				IsFPDirty( EN64FloatReg reg ) const;
		void				MarkFPAsValid( EN64FloatReg reg, bool valid );
		void				MarkFPAsDirty( EN64FloatReg reg, bool dirty );

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
