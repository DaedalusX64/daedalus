/*
Copyright (C) 2005 StrmnNrmn

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
#include "CodeGeneratorPSP.h"
#include "N64RegisterCachePSP.h"
#include "DynaRec/AssemblyUtils.h"
#include "DynaRec/Trace.h"

#include "Math/MathUtil.h"

#include "Utility/PrintOpCode.h"

#include "Core/R4300.h"
#include "Core/CPU.h"
#include "Core/Memory.h"
#include "Core/Registers.h"

#include "OSHLE/ultra_R4300.h"

#include "Debug/DBGConsole.h"

#include "ConfigOptions.h"

#include <algorithm>
#include <limits.h>

using namespace AssemblyUtils;

#define NOT_IMPLEMENTED( x )	DAEDALUS_ERROR( x )

extern "C" { const void * g_ReadAddressLookupTableForDynarec = g_ReadAddressLookupTable; }

extern "C" { void _ReturnFromDynaRec(); }
extern "C" { void _DirectExitCheckNoDelay( u32 instructions_executed, u32 exit_pc ); }
extern "C" { void _DirectExitCheckDelay( u32 instructions_executed, u32 exit_pc, u32 target_pc ); }
extern "C" { void _IndirectExitCheck( u32 instructions_executed, CIndirectExitMap * p_map, u32 target_pc ); }

extern "C"
{
	u32 _ReadBitsDirect_u8( u32 address, u32 current_pc );
	u32 _ReadBitsDirect_s8( u32 address, u32 current_pc );
	u32 _ReadBitsDirect_u16( u32 address, u32 current_pc );
	u32 _ReadBitsDirect_s16( u32 address, u32 current_pc );
	u32 _ReadBitsDirect_u32( u32 address, u32 current_pc );

	u32 _ReadBitsDirectBD_u8( u32 address, u32 current_pc );
	u32 _ReadBitsDirectBD_s8( u32 address, u32 current_pc );
	u32 _ReadBitsDirectBD_u16( u32 address, u32 current_pc );
	u32 _ReadBitsDirectBD_s16( u32 address, u32 current_pc );
	u32 _ReadBitsDirectBD_u32( u32 address, u32 current_pc );

	// Dynarec calls this for simplicity
	void Write32BitsForDynaRec( u32 address, u32 value )
	{
		Write32Bits_NoSwizzle( address, value );
	}
	void Write16BitsForDynaRec( u32 address, u16 value )
	{
		Write16Bits_NoSwizzle( address, value );
	}
	void Write8BitsForDynaRec( u32 address, u8 value )
	{
		Write8Bits_NoSwizzle( address, value );
	}

	void _WriteBitsDirect_u32( u32 address, u32 value, u32 current_pc );
	void _WriteBitsDirect_u16( u32 address, u32 value, u32 current_pc );		// Value in low 16 bits
	void _WriteBitsDirect_u8( u32 address, u32 value, u32 current_pc );			// Value in low 8 bits

	void _WriteBitsDirectBD_u32( u32 address, u32 value, u32 current_pc );
	void _WriteBitsDirectBD_u16( u32 address, u32 value, u32 current_pc );		// Value in low 16 bits
	void _WriteBitsDirectBD_u8( u32 address, u32 value, u32 current_pc );			// Value in low 8 bits
}

#define ReadBitsDirect_u8 _ReadBitsDirect_u8
#define ReadBitsDirect_s8 _ReadBitsDirect_s8
#define ReadBitsDirect_u16 _ReadBitsDirect_u16
#define ReadBitsDirect_s16 _ReadBitsDirect_s16
#define ReadBitsDirect_u32 _ReadBitsDirect_u32

#define ReadBitsDirectBD_u8 _ReadBitsDirectBD_u8
#define ReadBitsDirectBD_s8 _ReadBitsDirectBD_s8
#define ReadBitsDirectBD_u16 _ReadBitsDirectBD_u16
#define ReadBitsDirectBD_s16 _ReadBitsDirectBD_s16
#define ReadBitsDirectBD_u32 _ReadBitsDirectBD_u32


#define WriteBitsDirect_u32 _WriteBitsDirect_u32
#define WriteBitsDirect_u16 _WriteBitsDirect_u16
#define WriteBitsDirect_u8 _WriteBitsDirect_u8

#define WriteBitsDirectBD_u32 _WriteBitsDirectBD_u32
#define WriteBitsDirectBD_u16 _WriteBitsDirectBD_u16
#define WriteBitsDirectBD_u8 _WriteBitsDirectBD_u8



extern "C" { void _ReturnFromDynaRecIfStuffToDo( u32 register_mask ); }
extern "C" { void _DaedalusICacheInvalidate( const void * address, u32 length ); }

bool			gHaveSavedPatchedOps = false;
PspOpCode		gOriginalOps[2];
PspOpCode		gReplacementOps[2];

void Dynarec_ClearedCPUStuffToDo()
{
	// Replace first two ops of _ReturnFromDynaRecIfStuffToDo with 'jr ra, nop'
	u8 *			p_void_function( reinterpret_cast< u8 * >( _ReturnFromDynaRecIfStuffToDo ) );
	PspOpCode *		p_function_address = reinterpret_cast< PspOpCode * >( MAKE_UNCACHED_PTR( p_void_function ) );

	if(!gHaveSavedPatchedOps)
	{
		gOriginalOps[0] = p_function_address[0];
		gOriginalOps[1] = p_function_address[1];

		PspOpCode	op_code;
		op_code._u32 = 0;
		op_code.op = OP_SPECOP;
		op_code.spec_op = SpecOp_JR;
		op_code.rs = PspReg_RA;
		gReplacementOps[0] = op_code;		// JR RA
		gReplacementOps[1]._u32 = 0;		// NOP

		gHaveSavedPatchedOps = true;
	}

	p_function_address[0] = gReplacementOps[0];
	p_function_address[1] = gReplacementOps[1];

	const u8 * p_lower( RoundPointerDown( p_void_function, 64 ) );
	const u8 * p_upper( RoundPointerUp( p_void_function + 8, 64 ) );
	const u32  size( p_upper - p_lower);
	//sceKernelDcacheWritebackRange( p_lower, size );
	//sceKernelIcacheInvalidateRange( p_lower, size );

	_DaedalusICacheInvalidate( p_lower, size );
}

void Dynarec_SetCPUStuffToDo()
{
	// Restore first two ops of _ReturnFromDynaRecIfStuffToDo

	u8 *			p_void_function( reinterpret_cast< u8 * >( _ReturnFromDynaRecIfStuffToDo ) );
	PspOpCode *		p_function_address = reinterpret_cast< PspOpCode * >( MAKE_UNCACHED_PTR( p_void_function ) );

	p_function_address[0] = gOriginalOps[0];
	p_function_address[1] = gOriginalOps[1];

	const u8 * p_lower( RoundPointerDown( p_void_function, 64 ) );
	const u8 * p_upper( RoundPointerUp( p_void_function + 8, 64 ) );
	const u32  size( p_upper - p_lower);
	//sceKernelDcacheWritebackRange( p_lower, size );
	//sceKernelIcacheInvalidateRange( p_lower, size );

	_DaedalusICacheInvalidate( p_lower, size );
}

extern "C"
{
void HandleException_extern()
{
	switch (gCPUState.Delay)
	{
		case DO_DELAY:
			gCPUState.CurrentPC += 4;
			gCPUState.Delay = EXEC_DELAY;
			break;
		case EXEC_DELAY:
			gCPUState.CurrentPC = gCPUState.TargetPC;
			gCPUState.Delay = NO_DELAY;
			break;
		case NO_DELAY:
			gCPUState.CurrentPC += 4;
			break;
		default:	// MSVC extension - the default will never be reached
			NODEFAULT;
	}
}

}

namespace
{
const EPspReg	gMemUpperBoundReg = PspReg_S6;
const EPspReg	gMemoryBaseReg = PspReg_S7;

const EPspReg	gRegistersToUseForCaching[] =
{
	PspReg_S0,
	PspReg_S1,
	PspReg_S2,
	PspReg_S3,
	PspReg_S4,
	PspReg_S5,
//	PspReg_S6,		// Memory upper bound
//	PspReg_S7,		// Used for g_pu8RamBase - 0x80000000
//	PspReg_S8,		// Used for base register (&gCPUState)
//	PspReg_T0,		// Used as calculation temp
//	PspReg_T1,		// Used as calculation temp
	PspReg_T2,
	PspReg_T3,
	PspReg_T4,
	PspReg_T5,
	PspReg_T6,
	PspReg_T7,
	PspReg_T8,
	PspReg_T9,
	PspReg_A2,
	PspReg_A3,
};
}

u32		gTotalRegistersCached = 0;
u32		gTotalRegistersUncached = 0;


//*****************************************************************************
//
//*****************************************************************************
CCodeGeneratorPSP::CCodeGeneratorPSP( CAssemblyBuffer * p_buffer_a, CAssemblyBuffer * p_buffer_b )
:	CCodeGenerator( )
,	CAssemblyWriterPSP( p_buffer_a, p_buffer_b )
,	mpBasePointer( NULL )
,	mBaseRegister( PspReg_S8 )		// TODO
,	mEntryAddress( 0 )
,	mLoopTop( NULL )
,	mUseFixedRegisterAllocation( false )
{
}


//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::Initialise( u32 entry_address, u32 exit_address, u32 * hit_counter, const void * p_base, const SRegisterUsageInfo & register_usage )
{
	mEntryAddress = entry_address;

	mpBasePointer = reinterpret_cast< const u8 * >( p_base );
	SetRegisterSpanList( register_usage, entry_address == exit_address );

	if( hit_counter != NULL )
	{
		GetVar( PspReg_T0, hit_counter );
		ADDIU( PspReg_T0, PspReg_T0, 1 );
		SetVar( hit_counter, PspReg_T0 );
	}

}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::Finalise()
{
	GenerateAddressCheckFixups();

	CAssemblyWriterPSP::Finalise();
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::SetRegisterSpanList( const SRegisterUsageInfo & register_usage, bool loops_to_self )
{
	mRegisterSpanList = register_usage.SpanList;

	// Sort in order of increasing start point
	std::sort( mRegisterSpanList.begin(), mRegisterSpanList.end(), SAscendingSpanStartSort() );

	const u32 NUM_CACHE_REGS( sizeof(gRegistersToUseForCaching) / sizeof(gRegistersToUseForCaching[0]) );

	// Push all the available registers in reverse order (i.e. use temporaries later)
	DAEDALUS_ASSERT( mAvailableRegisters.empty(), "Why isn't the available register list empty?" );
	for( s32 i = NUM_CACHE_REGS-1; i >= 0 ; --i )
	{
		mAvailableRegisters.push( gRegistersToUseForCaching[ i ] );
	}
	// Optimization for self looping code
	if( gDynarecLoopOptimisation & loops_to_self )
	{
		mUseFixedRegisterAllocation = true;
		u32		cache_reg_idx( 0 );
		u32		HiLo = 0;
		//while ( HiLo<2 )		// If there are still unused registers, assign to high part of reg
		//{
			RegisterSpanList::const_iterator span_it = mRegisterSpanList.begin();
			while( span_it < mRegisterSpanList.end() )
			{
				const SRegisterSpan &	span( *span_it );
				if( cache_reg_idx < NUM_CACHE_REGS )
				{
					EPspReg		cachable_reg( gRegistersToUseForCaching[cache_reg_idx] );
					mRegisterCache.SetCachedReg( span.Register, HiLo, cachable_reg );
					cache_reg_idx++;
				}
				++span_it;
			}
			//++HiLo;
		//}
		//
		//	Pull all the cached registers into memory
		//
		// Skip r0
		s32 i = NUM_N64_REGS;
		while( i > 0 )
		{
			EN64Reg	n64_reg = EN64Reg( i );
			if( mRegisterCache.IsCached( n64_reg, 0 ) )
			{
				PrepareCachedRegister( n64_reg, 0 );

				//
				//	If the register is modified anywhere in the fragment, we need
				//	to mark it as dirty so it's flushed correctly on exit.
				//
				if( register_usage.IsModified( n64_reg ) )
				{
					mRegisterCache.MarkAsDirty( n64_reg, 0, true );
				}
			}
			--i; 
		}
		mLoopTop = GetAssemblyBuffer()->GetLabel();
	} //End of Loop optimization code
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::ExpireOldIntervals( u32 instruction_idx )
{
	// mActiveIntervals is held in order of increasing end point
	for(RegisterSpanList::iterator span_it = mActiveIntervals.begin(); span_it < mActiveIntervals.end(); ++span_it )
	{
		const SRegisterSpan &	span( *span_it );

		if( span.SpanEnd >= instruction_idx )
		{
			break;
		}

		// This interval is no longer active - flush the register and return it to the list of available regs
		EPspReg		psp_reg( mRegisterCache.GetCachedReg( span.Register, 0 ) );

		FlushRegister( mRegisterCache, span.Register, 0, true );

		mRegisterCache.ClearCachedReg( span.Register, 0 );

		mAvailableRegisters.push( psp_reg );

		span_it = mActiveIntervals.erase( span_it );
	}
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::SpillAtInterval( const SRegisterSpan & live_span )
{
	DAEDALUS_ASSERT( !mActiveIntervals.empty(), "There are no active intervals" );

	const SRegisterSpan &	last_span( mActiveIntervals.back() );		// Spill the last active interval (it has the greatest end point)

	if( last_span.SpanEnd > live_span.SpanEnd )
	{
		// Uncache the old span
		EPspReg		psp_reg( mRegisterCache.GetCachedReg( last_span.Register, 0 ) );
		FlushRegister( mRegisterCache, last_span.Register, 0, true );
		mRegisterCache.ClearCachedReg( last_span.Register, 0 );

		// Cache the new span
		mRegisterCache.SetCachedReg( live_span.Register, 0, psp_reg );

		mActiveIntervals.pop_back();				// Remove the last span
		mActiveIntervals.push_back( live_span );	// Insert in order of increasing end point

		std::sort( mActiveIntervals.begin(), mActiveIntervals.end(), SAscendingSpanEndSort() );		// XXXX - will be quicker to insert in the correct place rather than sorting each time
	}
	else
	{
		// There is no space for this register - we just don't update the register cache info, so we save/restore it from memory as needed
	}
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::UpdateRegisterCaching( u32 instruction_idx )
{
	if( !mUseFixedRegisterAllocation )
	{
		ExpireOldIntervals( instruction_idx );

		for(RegisterSpanList::const_iterator span_it = mRegisterSpanList.begin(); span_it < mRegisterSpanList.end(); ++span_it )
		{
			const SRegisterSpan &	span( *span_it );

			// As we keep the intervals sorted in order of SpanStart, we can exit as soon as we encounter a SpanStart in the future
			if( instruction_idx < span.SpanStart )
			{
				break;
			}

			// Only process live intervals
			if( instruction_idx >= span.SpanStart && instruction_idx <= span.SpanEnd )
			{
				if( !mRegisterCache.IsCached( span.Register, 0 ) )
				{
					if( mAvailableRegisters.empty() )
					{
						SpillAtInterval( span );
					}
					else
					{
						// Use this register for caching
						mRegisterCache.SetCachedReg( span.Register, 0, mAvailableRegisters.top() );

						// Pop this register from the available list
						mAvailableRegisters.pop();
						mActiveIntervals.push_back( span );		// Insert in order of increasing end point

						std::sort( mActiveIntervals.begin(), mActiveIntervals.end(), SAscendingSpanEndSort() );		// XXXX - will be quicker to insert in the correct place rather than sorting each time
					}
				}
			}
		}
	}
}

//*****************************************************************************
//
//*****************************************************************************
RegisterSnapshotHandle	CCodeGeneratorPSP::GetRegisterSnapshot()
{
	RegisterSnapshotHandle	handle( mRegisterSnapshots.size() );

	mRegisterSnapshots.push_back( mRegisterCache );

	return handle;
}

//*****************************************************************************
//
//*****************************************************************************
CCodeLabel	CCodeGeneratorPSP::GetEntryPoint() const
{
	return GetAssemblyBuffer()->GetStartAddress();
}

//*****************************************************************************
//
//*****************************************************************************
CCodeLabel	CCodeGeneratorPSP::GetCurrentLocation() const
{
	return GetAssemblyBuffer()->GetLabel();
}

//*****************************************************************************
//
//*****************************************************************************
u32	CCodeGeneratorPSP::GetCompiledCodeSize() const
{
	return GetAssemblyBuffer()->GetSize();
}

//*****************************************************************************
//
//*****************************************************************************
EPspReg	CCodeGeneratorPSP::GetRegisterNoLoad( EN64Reg n64_reg, u32 lo_hi_idx, EPspReg scratch_reg )
{
	if( mRegisterCache.IsCached( n64_reg, lo_hi_idx ) )
	{
		return mRegisterCache.GetCachedReg( n64_reg, lo_hi_idx );
	}
	else
	{
		return scratch_reg;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::GetRegisterValue( EPspReg psp_reg, EN64Reg n64_reg, u32 lo_hi_idx )
{
	if( mRegisterCache.IsKnownValue( n64_reg, lo_hi_idx ) )
	{
		//printf( "Loading %s[%d] <- %08x\n", RegNames[ n64_reg ], lo_hi_idx, mRegisterCache.GetKnownValue( n64_reg, lo_hi_idx ) );
		LoadConstant( psp_reg, mRegisterCache.GetKnownValue( n64_reg, lo_hi_idx )._s32 );
	}
	else
	{
		GetVar( psp_reg, lo_hi_idx ? &gGPR[ n64_reg ]._u32_1 : &gGPR[ n64_reg ]._u32_0 );
	}
}

//*****************************************************************************
//
//*****************************************************************************
EPspReg	CCodeGeneratorPSP::GetRegisterAndLoad( EN64Reg n64_reg, u32 lo_hi_idx, EPspReg scratch_reg )
{
	EPspReg		reg;
	bool		need_load( false );

	if( mRegisterCache.IsCached( n64_reg, lo_hi_idx ) )
	{
		gTotalRegistersCached++;
		reg = mRegisterCache.GetCachedReg( n64_reg, lo_hi_idx );

		// We're loading it below, so set the valid flag
		if( !mRegisterCache.IsValid( n64_reg, lo_hi_idx ) )
		{
			need_load = true;
			mRegisterCache.MarkAsValid( n64_reg, lo_hi_idx, true );
		}
	}
	else if( n64_reg == N64Reg_R0 )
	{
		reg = PspReg_R0;
	}
	else
	{
		gTotalRegistersUncached++;
		reg = scratch_reg;
		need_load = true;
	}

	if( need_load )
	{
		GetRegisterValue( reg, n64_reg, lo_hi_idx );
	}

	return reg;
}

//*****************************************************************************
//	Similar to GetRegisterAndLoad, but ALWAYS loads into the specified psp register
//*****************************************************************************
void CCodeGeneratorPSP::LoadRegister( EPspReg psp_reg, EN64Reg n64_reg, u32 lo_hi_idx )
{
	if( mRegisterCache.IsCached( n64_reg, lo_hi_idx ) )
	{
		EPspReg	cached_reg( mRegisterCache.GetCachedReg( n64_reg, lo_hi_idx ) );

		gTotalRegistersCached++;

		// Load the register if it's currently invalid
		if( !mRegisterCache.IsValid( n64_reg,lo_hi_idx ) )
		{
			GetRegisterValue( cached_reg, n64_reg, lo_hi_idx );
			mRegisterCache.MarkAsValid( n64_reg, lo_hi_idx, true );
		}

		// Copy the register if necessary
		if( psp_reg != cached_reg )
		{
			OR( psp_reg, cached_reg, PspReg_R0 );
		}
	}
	else if( n64_reg == N64Reg_R0 )
	{
		OR( psp_reg, PspReg_R0, PspReg_R0 );
	}
	else
	{
		gTotalRegistersUncached++;
		GetRegisterValue( psp_reg, n64_reg, lo_hi_idx );
	}
}

//*****************************************************************************
//	This function pulls in a cached register so that it can be used at a later point.
//	This is usally done when we have a branching instruction - it guarantees that
//	the register is valid regardless of whether or not the branch is taken.
//*****************************************************************************
void	CCodeGeneratorPSP::PrepareCachedRegister( EN64Reg n64_reg, u32 lo_hi_idx )
{
	if( mRegisterCache.IsCached( n64_reg, lo_hi_idx ) )
	{
		EPspReg	cached_reg( mRegisterCache.GetCachedReg( n64_reg, lo_hi_idx ) );

		// Load the register if it's currently invalid
		if( !mRegisterCache.IsValid( n64_reg,lo_hi_idx ) )
		{
			GetRegisterValue( cached_reg, n64_reg, lo_hi_idx );
			mRegisterCache.MarkAsValid( n64_reg, lo_hi_idx, true );
		}
	}
}

//*****************************************************************************
//
//*****************************************************************************
void CCodeGeneratorPSP::StoreRegister( EN64Reg n64_reg, u32 lo_hi_idx, EPspReg psp_reg )
{
	mRegisterCache.ClearKnownValue( n64_reg, lo_hi_idx );

	if( mRegisterCache.IsCached( n64_reg, lo_hi_idx ) )
	{
		EPspReg	cached_reg( mRegisterCache.GetCachedReg( n64_reg, lo_hi_idx ) );

		gTotalRegistersCached++;

		// Update our copy as necessary
		if( psp_reg != cached_reg )
		{
			OR( cached_reg, psp_reg, PspReg_R0 );
		}
		mRegisterCache.MarkAsDirty( n64_reg, lo_hi_idx, true );
		mRegisterCache.MarkAsValid( n64_reg, lo_hi_idx, true );
	}
	else
	{
		gTotalRegistersUncached++;

		SetVar( lo_hi_idx ? &gGPR[ n64_reg ]._u32_1 : &gGPR[ n64_reg ]._u32_0, psp_reg );

		mRegisterCache.MarkAsDirty( n64_reg, lo_hi_idx, false );
	}
}

//*****************************************************************************
//
//*****************************************************************************
void CCodeGeneratorPSP::SetRegister64( EN64Reg n64_reg, s32 lo_value, s32 hi_value )
{
	SetRegister( n64_reg, 0, lo_value );
	SetRegister( n64_reg, 1, hi_value );
}

//*****************************************************************************
//	Set the low 32 bits of a register to a known value (and hence the upper
//	32 bits are also known though sign extension)
//*****************************************************************************
inline void CCodeGeneratorPSP::SetRegister32s( EN64Reg n64_reg, s32 value )
{
	SetRegister64( n64_reg, value, value >= 0 ? 0 : 0xffffffff );
}

//*****************************************************************************
//
//*****************************************************************************
inline void CCodeGeneratorPSP::SetRegister( EN64Reg n64_reg, u32 lo_hi_idx, u32 value )
{
	mRegisterCache.SetKnownValue( n64_reg, lo_hi_idx, value );
	mRegisterCache.MarkAsDirty( n64_reg, lo_hi_idx, true );
	if( mRegisterCache.IsCached( n64_reg, lo_hi_idx ) )
	{
		mRegisterCache.MarkAsValid( n64_reg, lo_hi_idx, false );		// The actual cache is invalid though!
	}
}

//*****************************************************************************
//
//*****************************************************************************
void CCodeGeneratorPSP::UpdateRegister( EN64Reg n64_reg, EPspReg psp_reg, EUpdateRegOptions options, EPspReg scratch_reg )
{
	StoreRegisterLo( n64_reg, psp_reg );

	switch( options )
	{
	case URO_HI_SIGN_EXTEND:
		if( mRegisterCache.IsCached( n64_reg, 1 ) )
		{
			scratch_reg = mRegisterCache.GetCachedReg( n64_reg, 1 );
		}
		SRA( scratch_reg, psp_reg, 0x1f );		// Sign extend
		StoreRegisterHi( n64_reg, scratch_reg );
		break;
	case URO_HI_CLEAR:
		SetRegister( n64_reg, 1, 0 );
		break;
	default:
		DAEDALUS_ERROR( "Unhandled option" );
		NODEFAULT;
		break;
	}
}

//*****************************************************************************
//
//*****************************************************************************
EPspFloatReg	CCodeGeneratorPSP::GetFloatRegisterAndLoad( EN64FloatReg n64_reg )
{
	EPspFloatReg	psp_reg = EPspFloatReg( n64_reg );	// 1:1 mapping
	if( !mRegisterCache.IsFPValid( n64_reg ) )
	{
		GetFloatVar( psp_reg, &gCPUState.FPU[n64_reg]._f32_0 );
		mRegisterCache.MarkFPAsValid( n64_reg, true );
	}

	return psp_reg;
}

//*****************************************************************************
//
//*****************************************************************************
inline void CCodeGeneratorPSP::UpdateFloatRegister( EN64FloatReg n64_reg )
{
	mRegisterCache.MarkFPAsDirty( n64_reg, true );
	mRegisterCache.MarkFPAsValid( n64_reg, true );
}

//*****************************************************************************
//
//*****************************************************************************
const CN64RegisterCachePSP & CCodeGeneratorPSP::GetRegisterCacheFromHandle( RegisterSnapshotHandle snapshot ) const
{
	DAEDALUS_ASSERT( snapshot.Handle < mRegisterSnapshots.size(), "Invalid snapshot handle" );
	return mRegisterSnapshots[ snapshot.Handle ];
}

//*****************************************************************************
//	Flush a specific register back to memory if dirty.
//	Clears the dirty flag and invalidates the contents if specified
//*****************************************************************************
void CCodeGeneratorPSP::FlushRegister( CN64RegisterCachePSP & cache, EN64Reg n64_reg, u32 lo_hi_idx, bool invalidate )
{
	if( cache.IsDirty( n64_reg, lo_hi_idx ) )
	{
		if( cache.IsKnownValue( n64_reg, lo_hi_idx ) )
		{
			s32		known_value( cache.GetKnownValue( n64_reg, lo_hi_idx )._s32 );

			SetVar( lo_hi_idx ? &gGPR[ n64_reg ]._u32_1 : &gGPR[ n64_reg ]._u32_0, known_value );
		}
		else if( cache.IsCached( n64_reg, lo_hi_idx ) )
		{
			DAEDALUS_ASSERT( cache.IsValid( n64_reg, lo_hi_idx ), "Register is dirty but not valid?" );

			EPspReg	cached_reg( cache.GetCachedReg( n64_reg, lo_hi_idx ) );

			SetVar( lo_hi_idx ? &gGPR[ n64_reg ]._u32_1 : &gGPR[ n64_reg ]._u32_0, cached_reg );
		}
		else
		{
			DAEDALUS_ERROR( "Register is dirty, but not known or cached" );
		}

		// We're no longer dirty
		cache.MarkAsDirty( n64_reg, lo_hi_idx, false );
	}

	// Invalidate the register, so we pick up any values the function might have changed
	if( invalidate )
	{
		cache.ClearKnownValue( n64_reg, lo_hi_idx );
		if( cache.IsCached( n64_reg, lo_hi_idx ) )
		{
			cache.MarkAsValid( n64_reg, lo_hi_idx, false );
		}
	}
}

//*****************************************************************************
//	This function flushes all dirty registers back to memory
//	If the invalidate flag is set this also invalidates the known value/cached
//	register. This is primarily to ensure that we keep the register set
//	in a consistent set across calls to generic functions. Ideally we need
//	to reimplement generic functions with specialised code to avoid the flush.
//*****************************************************************************
void	CCodeGeneratorPSP::FlushAllRegisters( CN64RegisterCachePSP & cache, bool invalidate )
{
	// Skip r0
	for( u32 i = NUM_N64_REGS-1; i > 0; --i )
	{
		EN64Reg	n64_reg = EN64Reg( i );

		FlushRegister( cache, n64_reg, 0, invalidate );
		FlushRegister( cache, n64_reg, 1, invalidate );
	}

	FlushAllFloatingPointRegisters( cache, invalidate );
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::FlushAllFloatingPointRegisters( CN64RegisterCachePSP & cache, bool invalidate )
{
	for( u32 i = 0; i < NUM_N64_FP_REGS; ++i )
	{
		EN64FloatReg	n64_reg = EN64FloatReg( i );
		if( cache.IsFPDirty( n64_reg ) )
		{
			DAEDALUS_ASSERT( cache.IsFPValid( n64_reg ), "Register is dirty but not valid?" );

			EPspFloatReg	psp_reg = EPspFloatReg( n64_reg );

			SetFloatVar( &gCPUState.FPU[n64_reg]._f32_0, psp_reg );

			cache.MarkFPAsDirty( n64_reg, false );
		}

		if( invalidate )
		{
			// Invalidate the register, so we pick up any values the function might have changed
			cache.MarkFPAsValid( n64_reg, false );
		}
	}
}

//*****************************************************************************
//	This function flushes all dirty *temporary* registers back to memory, and
//	marks them as invalid.
//*****************************************************************************
void	CCodeGeneratorPSP::FlushAllTemporaryRegisters( CN64RegisterCachePSP & cache, bool invalidate )
{
	// Skip r0
	for( u32 i = NUM_N64_REGS-1; i > 0; --i )
	{
		EN64Reg	n64_reg = EN64Reg( i );

		if( cache.IsTemporary( n64_reg, 0 ) )
		{
			FlushRegister( cache, n64_reg, 0, invalidate );
		}
		if( cache.IsTemporary( n64_reg, 1 ) )
		{
			FlushRegister( cache, n64_reg, 1, invalidate );
		}

	}

	// XXXX some fp regs are preserved across function calls?
	FlushAllFloatingPointRegisters( cache, invalidate );
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::RestoreAllRegisters( CN64RegisterCachePSP & current_cache, CN64RegisterCachePSP & new_cache )
{
	// Skip r0
	for( u32 i = NUM_N64_REGS-1; i > 0; --i )
	{
		EN64Reg	n64_reg = EN64Reg( i );

		if( new_cache.IsValid( n64_reg, 0 ) && !current_cache.IsValid( n64_reg, 0 ) )
		{
			GetVar( new_cache.GetCachedReg( n64_reg, 0 ), &gGPR[ n64_reg ]._u32_0 );
		}
		if( new_cache.IsValid( n64_reg, 1 ) && !current_cache.IsValid( n64_reg, 1 ) )
		{
			GetVar( new_cache.GetCachedReg( n64_reg, 1 ), &gGPR[ n64_reg ]._u32_1 );
		}
	}

	// XXXX some fp regs are preserved across function calls?
	for( u32 i = 0; i < NUM_N64_FP_REGS; ++i )
	{
		EN64FloatReg	n64_reg = EN64FloatReg( i );
		if( new_cache.IsFPValid( n64_reg ) && !current_cache.IsFPValid( n64_reg ) )
		{
			EPspFloatReg	psp_reg = EPspFloatReg( n64_reg );

			GetFloatVar( psp_reg, &gCPUState.FPU[n64_reg]._f32_0 );
		}
	}
}

//*****************************************************************************
//
//*****************************************************************************
CJumpLocation CCodeGeneratorPSP::GenerateExitCode( u32 exit_address, u32 jump_address, u32 num_instructions, CCodeLabel next_fragment )
{
	//DAEDALUS_ASSERT( exit_address != u32( ~0 ), "Invalid exit address" );
	DAEDALUS_ASSERT( !next_fragment.IsSet() || jump_address == 0, "Shouldn't be specifying a jump address if we have a next fragment?" );

	if( (exit_address == mEntryAddress) & mLoopTop.IsSet() )
	{
		DAEDALUS_ASSERT( mUseFixedRegisterAllocation, "Have mLoopTop but unfixed register allocation?" );

		FlushAllFloatingPointRegisters( mRegisterCache, true );

		// Check if we're ok to continue, without flushing any registers
		GetVar( PspReg_T0, &gCPUState.CPUControl[C0_COUNT]._u32_0 );
		GetVar( PspReg_T1, (const u32*)&gCPUState.Events[0].mCount );

		//
		//	Pull in any registers which may have been flushed for whatever reason.
		//
		// Skip r0
		for( s32 i = NUM_N64_REGS-1; i >= 0; --i )
		{
			EN64Reg	n64_reg = EN64Reg( i+1 );

			if( mRegisterCache.IsDirty( n64_reg, 0 ) & mRegisterCache.IsKnownValue( n64_reg, 0 ) )
			{
				FlushRegister( mRegisterCache, n64_reg, 0, false );
			}
			if( mRegisterCache.IsDirty( n64_reg, 1 ) & mRegisterCache.IsKnownValue( n64_reg, 1 ) )
			{
				FlushRegister( mRegisterCache, n64_reg, 1, false );
			}

			PrepareCachedRegister( n64_reg, 0 );
			PrepareCachedRegister( n64_reg, 1 );
		}

		// Assuming we don't need to set CurrentPC/Delay flags before we branch to the top..

		ADDIU( PspReg_T0, PspReg_T0, num_instructions );
		SetVar( &gCPUState.CPUControl[C0_COUNT]._u32_0, PspReg_T0 );

		ADDIU( PspReg_T1, PspReg_T1, -s16(num_instructions) );

		//
		//	If the event counter is still positive, just jump directly to the top of our loop
		//
		BGTZ( PspReg_T1, mLoopTop, false );
		SetVar( (u32*)&gCPUState.Events[0].mCount, PspReg_T1 );		// ASSUMES store is done in just a single op.

		FlushAllRegisters( mRegisterCache, false );
		SetVar( &gCPUState.CurrentPC, exit_address );
		SetVar( &gCPUState.Delay, NO_DELAY );
		JAL( CCodeLabel( reinterpret_cast< const void * >( CPU_HANDLE_COUNT_INTERRUPT ) ), true );
		J( CCodeLabel( reinterpret_cast< const void * >( _ReturnFromDynaRec ) ), true );

		//
		//	Return an invalid jump location to indicate we've handled our own linking.
		//
		return CJumpLocation( NULL );
	}

	FlushAllRegisters( mRegisterCache, false );

	CCodeLabel	exit_routine;

	// Use the branch delay slot to load the second argument
	PspOpCode	op1;
	PspOpCode	op2;

	if( jump_address != 0 )
	{
		LoadConstant( PspReg_A0, num_instructions );
		LoadConstant( PspReg_A1, exit_address );
		GetLoadConstantOps( PspReg_A2, jump_address, &op1, &op2 );

		exit_routine = CCodeLabel( reinterpret_cast< const void * >( _DirectExitCheckDelay ) );
	}
	else
	{
		LoadConstant( PspReg_A0, num_instructions );
		GetLoadConstantOps( PspReg_A1, exit_address, &op1, &op2 );

		exit_routine = CCodeLabel( reinterpret_cast< const void * >( _DirectExitCheckNoDelay ) );
	}

	if( op2._u32 == 0 )
	{
		JAL( exit_routine, false );		// No branch delay
		AppendOp( op1 );
	}
	else
	{
		AppendOp( op1 );
		JAL( exit_routine, false );		// No branch delay
		AppendOp( op2 );
	}

	// This jump may be NULL, in which case we patch it below
	// This gets patched with a jump to the next fragment if the target is later found
	CJumpLocation jump_to_next_fragment( J( next_fragment, true ) );

	// Patch up the exit jump if the target hasn't been compiled yet
	if( !next_fragment.IsSet() )
	{
		PatchJumpLong( jump_to_next_fragment, CCodeLabel( reinterpret_cast< const void * >( _ReturnFromDynaRec ) ) );
	}
	return jump_to_next_fragment;

}

//*****************************************************************************
// Handle branching back to the interpreter after an ERET
//*****************************************************************************
void CCodeGeneratorPSP::GenerateEretExitCode( u32 num_instructions, CIndirectExitMap * p_map )
{
	FlushAllRegisters( mRegisterCache, false );

	// Eret is a bit bodged so we exit at PC + 4
	LoadConstant( PspReg_A0, num_instructions );
	LoadConstant( PspReg_A1, reinterpret_cast< s32 >( p_map ) );
	GetVar( PspReg_A2, &gCPUState.CurrentPC );

	J( CCodeLabel( reinterpret_cast< const void * >( _IndirectExitCheck ) ), false );
	ADDIU( PspReg_A2, PspReg_A2, 4 );		// Delay slot
}

//*****************************************************************************
// Handle branching back to the interpreter after an indirect jump
//*****************************************************************************
void CCodeGeneratorPSP::GenerateIndirectExitCode( u32 num_instructions, CIndirectExitMap * p_map )
{
	FlushAllRegisters( mRegisterCache, false );

	// New return address is in gCPUState.TargetPC
	PspOpCode op1, op2;
	GetLoadConstantOps(PspReg_A0, num_instructions, &op1, &op2);
	LoadConstant( PspReg_A1, reinterpret_cast< s32 >( p_map ) );
	GetVar( PspReg_A2, &gCPUState.TargetPC );

	if (op2._u32 == 0)
	{
		J( CCodeLabel( reinterpret_cast< const void * >( _IndirectExitCheck ) ), false );
		AppendOp(op1);
	}
	else
	{
		AppendOp(op1);
		J( CCodeLabel( reinterpret_cast< const void * >( _IndirectExitCheck ) ), false );
		AppendOp(op2);
	}
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::GenerateBranchHandler( CJumpLocation branch_handler_jump, RegisterSnapshotHandle snapshot )
{
	DAEDALUS_ASSERT( branch_handler_jump.IsSet(), "Why is the branch handler jump not set?" );

	CCodeGeneratorPSP::SetBufferA();
	CCodeLabel	current_label( GetAssemblyBuffer()->GetLabel() );

	PatchJumpLong( branch_handler_jump, current_label );

	mRegisterCache = GetRegisterCacheFromHandle( snapshot );

	CJumpLocation	jump_to_b( J( CCodeLabel( NULL ), true ) );
	CCodeGeneratorPSP::SetBufferB();
	current_label = GetAssemblyBuffer()->GetLabel();
	PatchJumpLong( jump_to_b, current_label );

}

//*****************************************************************************
//
//*****************************************************************************
CJumpLocation	CCodeGeneratorPSP::GenerateBranchAlways( CCodeLabel target )
{
	return J( target, true );
}

//*****************************************************************************
//
//*****************************************************************************
CJumpLocation	CCodeGeneratorPSP::GenerateBranchIfSet( const u32 * p_var, CCodeLabel target )
{
	GetVar( PspReg_T0, p_var );
	return BNE( PspReg_T0, PspReg_R0, target, true );
}

//*****************************************************************************
//
//*****************************************************************************
CJumpLocation	CCodeGeneratorPSP::GenerateBranchIfNotSet( const u32 * p_var, CCodeLabel target )
{
	GetVar( PspReg_T0, p_var );
	return BEQ( PspReg_T0, PspReg_R0, target, true );
}

//*****************************************************************************
//
//*****************************************************************************
CJumpLocation	CCodeGeneratorPSP::GenerateBranchIfEqual( const u32 * p_var, u32 value, CCodeLabel target )
{
	GetVar( PspReg_T0, p_var );
	LoadConstant( PspReg_T1, value );
	return BEQ( PspReg_T0, PspReg_T1, target, true );
}

//*****************************************************************************
//
//*****************************************************************************
CJumpLocation	CCodeGeneratorPSP::GenerateBranchIfNotEqual( const u32 * p_var, u32 value, CCodeLabel target )
{
	GetVar( PspReg_T0, p_var );
	LoadConstant( PspReg_T1, value );
	return BNE( PspReg_T0, PspReg_T1, target, true );
}

//*****************************************************************************
//
//*****************************************************************************
CJumpLocation	CCodeGeneratorPSP::GenerateBranchIfNotEqual( EPspReg reg_a, u32 value, CCodeLabel target )
{
	EPspReg	reg_b( reg_a == PspReg_T0 ? PspReg_T1 : PspReg_T0 );		// Make sure we don't use the same reg

	LoadConstant( reg_b, value );
	return BNE( reg_a, reg_b, target, true );
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::SetVar( u32 * p_var, EPspReg reg_src )
{
	DAEDALUS_ASSERT( reg_src != PspReg_AT, "Whoops, splattering source register" );

	EPspReg		reg_base;
	s16			base_offset;
	GetBaseRegisterAndOffset( p_var, &reg_base, &base_offset );

	SW( reg_src, reg_base, base_offset );
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::SetVar( u32 * p_var, u32 value )
{
	EPspReg		reg_value;

	if(value == 0)
	{
		reg_value = PspReg_R0;
	}
	else
	{
		LoadConstant( PspReg_T0, value );
		reg_value = PspReg_T0;
	}

	EPspReg		reg_base;
	s16			base_offset;
	GetBaseRegisterAndOffset( p_var, &reg_base, &base_offset );

	SW( reg_value, reg_base, base_offset );
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::SetFloatVar( f32 * p_var, EPspFloatReg reg_src )
{
	EPspReg		reg_base;
	s16			base_offset;
	GetBaseRegisterAndOffset( p_var, &reg_base, &base_offset );

	SWC1( reg_src, reg_base, base_offset );
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::GetVar( EPspReg dst_reg, const u32 * p_var )
{
	EPspReg		reg_base;
	s16			base_offset;
	GetBaseRegisterAndOffset( p_var, &reg_base, &base_offset );

	LW( dst_reg, reg_base, base_offset );
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::GetFloatVar( EPspFloatReg dst_reg, const f32 * p_var )
{
	EPspReg		reg_base;
	s16			base_offset;
	GetBaseRegisterAndOffset( p_var, &reg_base, &base_offset );

	LWC1( dst_reg, reg_base, base_offset );
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::GetBaseRegisterAndOffset( const void * p_address, EPspReg * p_reg, s16 * p_offset )
{
	s32		base_pointer_offset( reinterpret_cast< const u8 * >( p_address ) - mpBasePointer );
	if( base_pointer_offset > SHRT_MIN && base_pointer_offset < SHRT_MAX )
	{
		*p_reg = mBaseRegister;
		*p_offset = base_pointer_offset;
	}
	else
	{
		u32		address( reinterpret_cast< u32 >( p_address ) );
		u16		hi_bits( (u16)( address >> 16 ) );
		u16		lo_bits( (u16)( address ) );

		//
		//	Move up
		//
		/*if(lo_bits >= 0x8000)
		{
			hi_bits++;
		}*/

		hi_bits += lo_bits >> 15;

		s32		long_offset( address - ((s32)hi_bits<<16) );

		DAEDALUS_ASSERT( long_offset >= SHRT_MIN && long_offset <= SHRT_MAX, "Offset is out of range!" );

		s16		offset( (s16)long_offset );

		DAEDALUS_ASSERT( ((s32)hi_bits<<16) + offset == (s32)address, "Incorrect address calculation" );

		LUI( PspReg_AT, hi_bits );
		*p_reg = PspReg_AT;
		*p_offset = offset;
	}
}

//*****************************************************************************
//
//*****************************************************************************
/*void	CCodeGeneratorPSP::UpdateAddressAndDelay( u32 address, bool set_branch_delay )
{
	DAEDALUS_ASSERT( !set_branch_delay || (address != 0), "Have branch delay and no address?" );

	if( set_branch_delay )
	{
		SetVar( &gCPUState.Delay, EXEC_DELAY );
		mBranchDelaySet = true;
	}
	//else
	//{
	//	SetVar( &gCPUState.Delay, NO_DELAY );
	//}
	if( address != 0 )
	{
		SetVar( &gCPUState.CurrentPC, address );
	}
}*/

//*****************************************************************************
//	Generates instruction handler for the specified op code.
//	Returns a jump location if an exception handler is required
//*****************************************************************************
CJumpLocation	CCodeGeneratorPSP::GenerateOpCode( const STraceEntry& ti, bool branch_delay_slot, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump, StaticAnalysis::MemAcess memory )
{
	DAEDALUS_PROFILE( "CCodeGeneratorPSP::GenerateOpCode" );

	u32 address = ti.Address;
	OpCode op_code = ti.OpCode; 
	bool	handled( false );
	bool	is_nop( op_code._u32 == 0 );

	mBranchDelaySet = false;

	if( is_nop )
	{
		if( branch_delay_slot )
		{
			SetVar( &gCPUState.Delay, NO_DELAY );
		}
		return CJumpLocation();
	}

	mQuickLoad = memory;

	const EN64Reg	rs = EN64Reg( op_code.rs );
	const EN64Reg	rt = EN64Reg( op_code.rt );
	const EN64Reg	rd = EN64Reg( op_code.rd );
	const u32		sa = op_code.sa;
	const EN64Reg	base = EN64Reg( op_code.base );
	//const u32		jump_target( (address&0xF0000000) | (op_code.target<<2) );
	//const u32		branch_target( address + ( ((s32)(s16)op_code.immediate)<<2 ) + 4);
	const u32		ft = op_code.ft;

	//
	//	Look for opcodes we can handle manually
	//
	switch( op_code.op )
	{

	case OP_J:			/* nothing to do */		handled = true; break;
	case OP_JAL:		GenerateJAL( address );	handled = true; break;

	case OP_ADDI:		GenerateADDIU( rt, rs, s16( op_code.immediate ) );	handled = true; break;
	case OP_ADDIU:		GenerateADDIU( rt, rs, s16( op_code.immediate ) );	handled = true; break;
	case OP_SLTI:		GenerateSLTI( rt, rs, s16( op_code.immediate ) );	handled = true; break;
	case OP_SLTIU:		GenerateSLTIU( rt, rs, s16( op_code.immediate ) );	handled = true; break;

	case OP_ANDI:		GenerateANDI( rt, rs, op_code.immediate );			handled = true; break;
	case OP_ORI:		GenerateORI( rt, rs, op_code.immediate );			handled = true; break;
	case OP_XORI:		GenerateXORI( rt, rs, op_code.immediate );			handled = true; break;
	case OP_LUI:		GenerateLUI( rt, s16( op_code.immediate ) );		handled = true; break;

	case OP_LB:			GenerateLB( address, branch_delay_slot, rt, base, s16( op_code.immediate ) );	handled = true; break;
	case OP_LBU:		GenerateLBU( address, branch_delay_slot, rt, base, s16( op_code.immediate ) );	handled = true; break;
	case OP_LH:			GenerateLH( address, branch_delay_slot, rt, base, s16( op_code.immediate ) );	handled = true; break;
	case OP_LHU:		GenerateLHU( address, branch_delay_slot, rt, base, s16( op_code.immediate ) );	handled = true; break;
	case OP_LW:			GenerateLW( address, branch_delay_slot, rt, base, s16( op_code.immediate ) );	handled = true; break;
	case OP_LWC1:		GenerateLWC1( address, branch_delay_slot, ft, base, s16( op_code.immediate ) );	handled = true; break;

	case OP_SB:			GenerateSB( address, branch_delay_slot, rt, base, s16( op_code.immediate ) );	handled = true; break;
	case OP_SH:			GenerateSH( address, branch_delay_slot, rt, base, s16( op_code.immediate ) );	handled = true; break;
	case OP_SW:			GenerateSW( address, branch_delay_slot, rt, base, s16( op_code.immediate ) );	handled = true; break;
	case OP_SWC1:		GenerateSWC1( address, branch_delay_slot, ft, base, s16( op_code.immediate ) );	handled = true; break;

	case OP_BEQ:
	case OP_BEQL:
		GenerateBEQ( rs, rt, p_branch, p_branch_jump ); handled = true; break;
	case OP_BNE:
	case OP_BNEL:
		GenerateBNE( rs, rt, p_branch, p_branch_jump ); handled = true; break;
	case OP_BLEZ:
	case OP_BLEZL:
		GenerateBLEZ( rs, p_branch, p_branch_jump ); handled = true; break;
	case OP_BGTZ:
	case OP_BGTZL:
		GenerateBGTZ( rs, p_branch, p_branch_jump ); handled = true; break;


	case OP_CACHE:		GenerateCACHE( base, op_code.immediate, rt ); handled = true; break;

	case OP_REGIMM:
		switch( op_code.regimm_op )
		{
				// These can be handled by the same Generate function, as the 'likely' bit is handled elsewhere
		case RegImmOp_BLTZ:
		case RegImmOp_BLTZL:
			GenerateBLTZ( rs, p_branch, p_branch_jump ); handled = true; break;

		case RegImmOp_BGEZ:
		case RegImmOp_BGEZL:
			GenerateBGEZ( rs, p_branch, p_branch_jump ); handled = true; break;
		}
		break;

	case OP_SPECOP:
		switch( op_code.spec_op )
		{
		case SpecOp_SLL:	GenerateSLL( rd, rt, sa );	handled = true; break;
		case SpecOp_SRA:	GenerateSRA( rd, rt, sa );	handled = true; break;
		case SpecOp_SRL:	GenerateSRL( rd, rt, sa );	handled = true; break;
		case SpecOp_SLLV:	GenerateSLLV( rd, rs, rt );	handled = true; break;
		case SpecOp_SRAV:	GenerateSRAV( rd, rs, rt );	handled = true; break;
		case SpecOp_SRLV:	GenerateSRLV( rd, rs, rt );	handled = true; break;

		case SpecOp_JR:		GenerateJR( rs, p_branch, p_branch_jump );	handled = true; break;
		case SpecOp_JALR:	GenerateJALR( rs, rd, address, p_branch, p_branch_jump );	handled = true; break;

		case SpecOp_MFLO:	GenerateMFLO( rd );			handled = true; break;
		case SpecOp_MFHI:	GenerateMFHI( rd );			handled = true; break;
		case SpecOp_MTLO:	GenerateMTLO( rd );			handled = true; break;
		case SpecOp_MTHI:	GenerateMTHI( rd );			handled = true; break;

		case SpecOp_MULT:	GenerateMULT( rs, rt );		handled = true; break;
		case SpecOp_MULTU:	GenerateMULTU( rs, rt );	handled = true; break;
		case SpecOp_DIV:	GenerateDIV( rs, rt );		handled = true; break;
		case SpecOp_DIVU:	GenerateDIVU( rs, rt );		handled = true; break;

		case SpecOp_ADD:	GenerateADDU( rd, rs, rt );	handled = true; break;
		case SpecOp_ADDU:	GenerateADDU( rd, rs, rt );	handled = true; break;
		case SpecOp_SUB:	GenerateSUBU( rd, rs, rt );	handled = true; break;
		case SpecOp_SUBU:	GenerateSUBU( rd, rs, rt );	handled = true; break;

		case SpecOp_AND:	GenerateAND( rd, rs, rt );	handled = true; break;
		case SpecOp_OR:		GenerateOR( rd, rs, rt );	handled = true; break;
		case SpecOp_XOR:	GenerateXOR( rd, rs, rt );	handled = true; break;
		case SpecOp_NOR:	GenerateNOR( rd, rs, rt );	handled = true; break;

		case SpecOp_SLT:	GenerateSLT( rd, rs, rt );	handled = true; break;
		case SpecOp_SLTU:	GenerateSLTU( rd, rs, rt );	handled = true; break;

		case SpecOp_DADD:	GenerateDADDU( rd, rs, rt );	handled = true; break;
		case SpecOp_DADDU:	GenerateDADDU( rd, rs, rt );	handled = true; break;

		default:
			break;
		}
		break;

	case OP_COPRO1:
		switch( op_code.cop1_op )
		{
		case Cop1Op_MFC1:	GenerateMFC1( rt, op_code.fs ); handled = true; break;
		case Cop1Op_MTC1:	GenerateMTC1( op_code.fs, rt ); handled = true; break;

		case Cop1Op_CFC1:	GenerateCFC1( rt, op_code.fs ); handled = true; break;
		case Cop1Op_CTC1:	GenerateCTC1( op_code.fs, rt ); handled = true; break;


		case Cop1Op_DInstr:
			// Need branch delay?
			GenerateGenericR4300( op_code, R4300_GetInstructionHandler( op_code ) );
			handled = true;
			break;

		case Cop1Op_SInstr:
			switch( op_code.cop1_funct )
			{
			case Cop1OpFunc_ADD:	GenerateADD_S( op_code.fd, op_code.fs, op_code.ft ); handled = true; break;
			case Cop1OpFunc_SUB:	GenerateSUB_S( op_code.fd, op_code.fs, op_code.ft ); handled = true; break;
			case Cop1OpFunc_MUL:	GenerateMUL_S( op_code.fd, op_code.fs, op_code.ft ); handled = true; break;
			case Cop1OpFunc_DIV:	GenerateDIV_S( op_code.fd, op_code.fs, op_code.ft ); handled = true; break;
			case Cop1OpFunc_SQRT:	GenerateSQRT_S( op_code.fd, op_code.fs ); handled = true; break;
			case Cop1OpFunc_ABS:	GenerateABS_S( op_code.fd, op_code.fs ); handled = true; break;
			case Cop1OpFunc_MOV:	GenerateMOV_S( op_code.fd, op_code.fs ); handled = true; break;
			case Cop1OpFunc_NEG:	GenerateNEG_S( op_code.fd, op_code.fs ); handled = true; break;

			case Cop1OpFunc_TRUNC_W:	GenerateTRUNC_W_S( op_code.fd, op_code.fs ); handled = true; break;

			case Cop1OpFunc_CVT_W:		GenerateCVT_W_S( op_code.fd, op_code.fs ); handled = true; break;

			case Cop1OpFunc_CMP_F:		GenerateCMP_S( op_code.fs, Cop1OpFunc_CMP_F, op_code.ft ); handled = true; break;
			case Cop1OpFunc_CMP_UN:		GenerateCMP_S( op_code.fs, Cop1OpFunc_CMP_UN, op_code.ft ); handled = true; break;
			case Cop1OpFunc_CMP_EQ:		GenerateCMP_S( op_code.fs, Cop1OpFunc_CMP_EQ, op_code.ft ); handled = true; break;
			case Cop1OpFunc_CMP_UEQ:	GenerateCMP_S( op_code.fs, Cop1OpFunc_CMP_UEQ, op_code.ft ); handled = true; break;
			case Cop1OpFunc_CMP_OLT:	GenerateCMP_S( op_code.fs, Cop1OpFunc_CMP_OLT, op_code.ft ); handled = true; break;
			case Cop1OpFunc_CMP_ULT:	GenerateCMP_S( op_code.fs, Cop1OpFunc_CMP_ULT, op_code.ft ); handled = true; break;
			case Cop1OpFunc_CMP_OLE:	GenerateCMP_S( op_code.fs, Cop1OpFunc_CMP_OLE, op_code.ft ); handled = true; break;
			case Cop1OpFunc_CMP_ULE:	GenerateCMP_S( op_code.fs, Cop1OpFunc_CMP_ULE, op_code.ft ); handled = true; break;

			case Cop1OpFunc_CMP_SF:		GenerateCMP_S( op_code.fs, Cop1OpFunc_CMP_SF, op_code.ft ); handled = true; break;
			case Cop1OpFunc_CMP_NGLE:	GenerateCMP_S( op_code.fs, Cop1OpFunc_CMP_NGLE, op_code.ft ); handled = true; break;
			case Cop1OpFunc_CMP_SEQ:	GenerateCMP_S( op_code.fs, Cop1OpFunc_CMP_SEQ, op_code.ft ); handled = true; break;
			case Cop1OpFunc_CMP_NGL:	GenerateCMP_S( op_code.fs, Cop1OpFunc_CMP_NGL, op_code.ft ); handled = true; break;
			case Cop1OpFunc_CMP_LT:		GenerateCMP_S( op_code.fs, Cop1OpFunc_CMP_LT, op_code.ft ); handled = true; break;
			case Cop1OpFunc_CMP_NGE:	GenerateCMP_S( op_code.fs, Cop1OpFunc_CMP_NGE, op_code.ft ); handled = true; break;
			case Cop1OpFunc_CMP_LE:		GenerateCMP_S( op_code.fs, Cop1OpFunc_CMP_LE, op_code.ft ); handled = true; break;
			case Cop1OpFunc_CMP_NGT:	GenerateCMP_S( op_code.fs, Cop1OpFunc_CMP_NGT, op_code.ft ); handled = true; break;

			}
			break;
		case Cop1Op_WInstr:
			switch( op_code.cop1_funct )
			{
			case Cop1OpFunc_CVT_S:	GenerateCVT_S_W( op_code.fd, op_code.fs ); handled = true; break;
			}
			break;

		case Cop1Op_BCInstr:
			switch( op_code.cop1_bc )
			{
				// These can be handled by the same Generate function, as the 'likely' bit is handled elsewhere
			case Cop1BCOp_BC1F:
			case Cop1BCOp_BC1FL:
				GenerateBC1F( p_branch, p_branch_jump ); handled = true; break;
			case Cop1BCOp_BC1T:
			case Cop1BCOp_BC1TL:
				GenerateBC1T( p_branch, p_branch_jump ); handled = true; break;
			}
			break;
		default:
			break;
		}
		break;
	}

	if( !handled )
	{
		/*
		char msg[100];
		SprintOpCodeInfo( msg, address, op_code );
		printf( "Unhandled: 0x%08x %s\n", address, msg );
		*/

		//
		//	Default handling - call interpreting function
		//
		bool	need_pc( R4300_InstructionHandlerNeedsPC( op_code ) );

#if 1	//1-> less messing about...//Corn
		if( !need_pc )
		{
			address = 0;
			GenerateGenericR4300( op_code, R4300_GetInstructionHandler( op_code ) );
		}
		else
		{
			SetVar( &gCPUState.CurrentPC, address );
			if( branch_delay_slot )
			{
				SetVar( &gCPUState.Delay, EXEC_DELAY );
				mBranchDelaySet = true;
			}

			GenerateGenericR4300( op_code, R4300_GetInstructionHandler( op_code ) );
			// Make sure all dirty registers are flushed. NB - we don't invalidate them
			// to avoid reloading the contents if no exception was thrown.

			FlushAllRegisters( mRegisterCache, false );

			JAL( CCodeLabel( reinterpret_cast< const void * >( _ReturnFromDynaRecIfStuffToDo ) ), false );
			ORI( PspReg_A0, PspReg_R0, 0 );

		}
#else
		if( !need_pc )
		{
			address = 0;
		}

		UpdateAddressAndDelay( address, branch_delay_slot && need_pc );

		GenerateGenericR4300( op_code, R4300_GetInstructionHandler( op_code ) );

		bool generate_exception_handler( need_pc );
		if( generate_exception_handler )
		{
			// Make sure all dirty registers are flushed. NB - we don't invalidate them
			// to avoid reloading the contents if no exception was thrown.

			FlushAllRegisters( mRegisterCache, false );

			JAL( CCodeLabel( reinterpret_cast< const void * >( _ReturnFromDynaRecIfStuffToDo ) ), false );
			ORI( PspReg_A0, PspReg_R0, 0 );
		}
#endif
	}

	CCodeLabel		no_target( NULL );

	// Check whether we want to invert the status of this branch
	if( p_branch != NULL )
	{
		if(!handled)
		{
			//
			// Check if the branch has been taken
			//
			if( p_branch->Direct )
			{
				if( p_branch->ConditionalBranchTaken )
				{
					*p_branch_jump = GenerateBranchIfNotEqual( &gCPUState.Delay, DO_DELAY, no_target );
				}
				else
				{
					*p_branch_jump = GenerateBranchIfEqual( &gCPUState.Delay, DO_DELAY, no_target );
				}
			}
			else
			{
				// XXXX eventually just exit here, and skip default exit code below
				if( p_branch->Eret )
				{
					*p_branch_jump = GenerateBranchAlways( no_target );
				}
				else
				{
					*p_branch_jump = GenerateBranchIfNotEqual( &gCPUState.TargetPC, p_branch->TargetAddress, no_target );
				}
			}
		}
	}

	if( mBranchDelaySet )
	{
		SetVar( &gCPUState.Delay, NO_DELAY );
		mBranchDelaySet = false;
	}

	return CJumpLocation();
}

//*****************************************************************************
//
//	NB - This assumes that the address and branch delay have been set up before
//	calling this function.
//
//*****************************************************************************
void	CCodeGeneratorPSP::GenerateGenericR4300( OpCode op_code, CPU_Instruction p_instruction )
{
	// Flush out the dirty registers, and mark them all as invalid
	// Theoretically we could just flush the registers referenced in the call
	FlushAllRegisters( mRegisterCache, true );

	PspOpCode	op1;
	PspOpCode	op2;

	// Load the opcode in to A0
	GetLoadConstantOps( PspReg_A0, op_code._u32, &op1, &op2 );

	if( op2._u32 == 0 )
	{
		JAL( CCodeLabel( reinterpret_cast< const void * >( p_instruction ) ), false );		// No branch delay
		AppendOp( op1 );
	}
	else
	{
		AppendOp( op1 );
		JAL( CCodeLabel( reinterpret_cast< const void * >( p_instruction ) ), false );		// No branch delay
		AppendOp( op2 );
	}
}


//*****************************************************************************
//
//*****************************************************************************
CJumpLocation CCodeGeneratorPSP::ExecuteNativeFunction( CCodeLabel speed_hack, bool check_return )
{
	JAL( speed_hack, true );

	if( check_return )
		return BEQ(PspReg_V0, PspReg_R0, CCodeLabel(NULL), true);
	else
		return CJumpLocation(NULL);
	
}

//*****************************************************************************
//
//*****************************************************************************
inline bool	CCodeGeneratorPSP::GenerateDirectLoad( EPspReg psp_dst, EN64Reg base, s16 offset, OpCodeValue load_op, u32 swizzle )
{
	if(mRegisterCache.IsKnownValue( base, 0 ))
	{
		u32		base_address( mRegisterCache.GetKnownValue( base, 0 )._u32 );
		u32		address( (base_address + s32( offset )) ^ swizzle );

		s32		tableEntry = reinterpret_cast< s32 >( g_ReadAddressPointerLookupTable[address >> 18] ) + address;
		if(tableEntry >= 0)
		{
			void * p_memory( reinterpret_cast< void * >( tableEntry ) );

			//printf( "Loading from %s %08x + %04x (%08x) op %d\n", RegNames[ base ], base_address, u16(offset), address, load_op );

			EPspReg		reg_base;
			s16			base_offset;
			GetBaseRegisterAndOffset( p_memory, &reg_base, &base_offset );

			CAssemblyWriterPSP::LoadRegister( psp_dst, load_op, reg_base, base_offset );

			return true;
		}
		else
		{
			//printf( "Loading from %s %08x + %04x (%08x) - unhandled\n", RegNames[ base ], base_address, u16(offset), address );
		}
	}

	return false;
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::GenerateSlowLoad( u32 current_pc, EPspReg psp_dst, EPspReg reg_address, ReadMemoryFunction p_read_memory )
{
	// We need to flush all the regular registers AND invalidate all the temp regs.
	// NB: When we can flush registers through some kind of handle (i.e. when the
	// exception handling branch is taken), this won't be necessary

	CN64RegisterCachePSP current_regs( mRegisterCache );
	FlushAllRegisters( mRegisterCache, false );
	FlushAllTemporaryRegisters( mRegisterCache, true );

	//
	//
	//
	if( reg_address != PspReg_A0 )
	{
		OR( PspReg_A0, reg_address, PspReg_R0 );
	}
	PspOpCode	op1, op2;
	GetLoadConstantOps( PspReg_A1, current_pc, &op1, &op2 );


	if( op2._u32 == 0 )
	{
		JAL( CCodeLabel( reinterpret_cast< const void * >( p_read_memory ) ), false );
		AppendOp( op1 );
	}
	else
	{
		AppendOp( op1 );
		JAL( CCodeLabel( reinterpret_cast< const void * >( p_read_memory ) ), false );
		AppendOp( op2 );
	}

	// Restore all registers BEFORE copying back final value
	RestoreAllRegisters( mRegisterCache, current_regs );

	// Copy return value to the destination register if we're not using V0
	if( psp_dst != PspReg_V0 )
	{
		OR( psp_dst, PspReg_V0, PspReg_R0 );
	}

	// Restore the state of the registers
	mRegisterCache = current_regs;
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::GenerateLoad( u32 current_pc,
										 EPspReg psp_dst,
										 EN64Reg n64_base,
										 s16 offset,
										 OpCodeValue load_op,
										 u32 swizzle,
										 ReadMemoryFunction p_read_memory )
{
	//
	//	Check if the base pointer is a known value, in which case we can load directly.
	//
	if(GenerateDirectLoad( psp_dst, n64_base, offset, load_op, swizzle ))
	{
		return;
	}

	EPspReg		reg_base( GetRegisterAndLoadLo( n64_base, PspReg_A0 ) );
	EPspReg		reg_address( reg_base );

	if( (gDynarecStackOptimisation && n64_base == N64Reg_SP)  || (gMemoryAccessOptimisation && mQuickLoad == StaticAnalysis::Segment_8000 /*&& load_op != OP_LB*/))
	{
		if( swizzle != 0 )
		{
			if( offset != 0 )
			{
				ADDIU( PspReg_A0, reg_address, offset );
				reg_address = PspReg_A0;
				offset = 0;
			}

			XORI( PspReg_A0, reg_address, swizzle );
			reg_address = PspReg_A0;
		}

		ADDU( PspReg_A1, reg_address, gMemoryBaseReg );
		CAssemblyWriterPSP::LoadRegister( psp_dst, load_op, PspReg_A1, offset );
		return;
	}

	if( offset != 0 )
	{
		ADDIU( PspReg_A0, reg_address, offset );
		reg_address = PspReg_A0;
	}

	if( swizzle != 0 )
	{
		XORI( PspReg_A0, reg_address, swizzle );
		reg_address = PspReg_A0;
	}

	//
	//	If the fast load failed, we need to perform a 'slow' load going through a handler routine
	//	We may already be performing this instruction in the second buffer, in which
	//	case we don't bother with the transitions
	//
	if( CAssemblyWriterPSP::IsBufferA() )
	{
		//
		//	This is a very quick check for 0x80000000 < address < 0x80r00000
		//	If it's in range, we add the base pointer (already offset by 0x80000000)
		//	and dereference (the branch likely ensures that we only dereference
		//	in the situations where the condition holds)
		//
		SLT( PspReg_T0, reg_address, gMemUpperBoundReg );	// t1 = upper < address
		CJumpLocation branch( BEQ( PspReg_T0, PspReg_R0, CCodeLabel( NULL ), false ) );		// branch to jump to handler
		ADDU( PspReg_A1, reg_address, gMemoryBaseReg );
		CAssemblyWriterPSP::LoadRegister( psp_dst, load_op, PspReg_A1, 0 );

		CCodeLabel		continue_location( GetAssemblyBuffer()->GetLabel() );

		//
		//	Generate the handler code in the secondary buffer
		//
		CAssemblyWriterPSP::SetBufferB();

		CCodeLabel		handler_label( GetAssemblyBuffer()->GetLabel() );
		GenerateSlowLoad( current_pc, psp_dst, reg_address, p_read_memory );

		CJumpLocation	ret_handler( J( continue_location, true ) );
		CAssemblyWriterPSP::SetBufferA();

		//
		//	Keep track of the details we need to generate the jump to the distant handler function
		//
		mAddressCheckFixups.push_back( SAddressCheckFixup( branch, handler_label ) );
	}
	else
	{
		//
		//	This is a very quick check for 0x80000000 < address < 0x80r00000
		//	If it's in range, we add the base pointer (already offset by 0x80000000)
		//	and dereference (the branch likely ensures that we only dereference
		//	in the situations where the condition holds)
		//
		SLT( PspReg_T0, reg_address, gMemUpperBoundReg );	// t1 = upper < address
		ADDU( PspReg_A1, reg_address, gMemoryBaseReg );
		CJumpLocation branch( BNEL( PspReg_T0, PspReg_R0, CCodeLabel( NULL ), false ) );
		CAssemblyWriterPSP::LoadRegister( psp_dst, load_op, PspReg_A1, 0 );

		GenerateSlowLoad( current_pc, psp_dst, reg_address, p_read_memory );

		PatchJumpLong( branch, GetAssemblyBuffer()->GetLabel() );
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void CCodeGeneratorPSP::GenerateAddressCheckFixups()
{
	for( u32 i = 0; i < mAddressCheckFixups.size(); ++i )
	{
		GenerateAddressCheckFixup( mAddressCheckFixups[ i ] );
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void CCodeGeneratorPSP::GenerateAddressCheckFixup( const SAddressCheckFixup & fixup )
{
	CAssemblyWriterPSP::SetBufferA();

	//
	//	Fixup the branch we emitted so that it jumps to the handler routine.
	//	If possible, we try to branch directly.
	//	If that fails, we try to emit a stub function which jumps to the handler. Hopefully that's a bit closer
	//	If *that* fails, we have to revert to an unconditional jump to the handler routine. Expensive!!
	//

	if( PatchJumpLong( fixup.BranchToJump, fixup.HandlerAddress ) )
	{
		printf( "Wow, managed to jump directly to our handler\n" );
	}
	else
	{
		if( PatchJumpLong( fixup.BranchToJump, GetAssemblyBuffer()->GetLabel() ) )
		{
			//
			//	Emit a long jump to the handler function
			//
			J( fixup.HandlerAddress, true );
		}
		else
		{
			printf( "Warning, jump is too far for address handler function\n" );
			ReplaceBranchWithJump( fixup.BranchToJump, fixup.HandlerAddress );
		}
	}
}


//*****************************************************************************
//
//*****************************************************************************
inline bool	CCodeGeneratorPSP::GenerateDirectStore( EPspReg psp_src, EN64Reg base, s16 offset, OpCodeValue store_op, u32 swizzle )
{
	if(mRegisterCache.IsKnownValue( base, 0 ))
	{
		u32		base_address( mRegisterCache.GetKnownValue( base, 0 )._u32 );
		u32		address( (base_address + s32( offset )) ^ swizzle );

		s32 tableEntry = reinterpret_cast< s32 >( g_WriteAddressPointerLookupTable[address >> 18] ) + address;
		if(tableEntry >= 0)
		{
			void * p_memory( reinterpret_cast< void * >( tableEntry ) );

			//printf( "Storing to %s %08x + %04x (%08x) op %d\n", RegNames[ base ], base_address, u16(offset), address, store_op );

			EPspReg		reg_base;
			s16			base_offset;
			GetBaseRegisterAndOffset( p_memory, &reg_base, &base_offset );

			CAssemblyWriterPSP::StoreRegister( psp_src, store_op, reg_base, base_offset );

			return true;
		}
		else
		{
			//printf( "Storing to %s %08x + %04x (%08x) - unhandled\n", RegNames[ base ], base_address, u16(offset), address );
		}
	}

	return false;
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSlowStore( u32 current_pc, EPspReg psp_src, EPspReg reg_address, WriteMemoryFunction p_write_memory )
{
	// We need to flush all the regular registers AND invalidate all the temp regs.
	// NB: When we can flush registers through some kind of handle (i.e. when the
	// exception handling branch is taken), this won't be necessary
	CN64RegisterCachePSP current_regs( mRegisterCache );
	FlushAllRegisters( mRegisterCache, false );
	FlushAllTemporaryRegisters( mRegisterCache, true );

	if( reg_address != PspReg_A0 )
	{
		OR( PspReg_A0, reg_address, PspReg_R0 );
	}
	if( psp_src != PspReg_A1 )
	{
		OR( PspReg_A1, psp_src, PspReg_R0 );
	}
	PspOpCode	op1, op2;
	GetLoadConstantOps( PspReg_A2, current_pc, &op1, &op2 );


	if( op2._u32 == 0 )
	{
		JAL( CCodeLabel( reinterpret_cast< const void * >( p_write_memory ) ), false );		// Don't set branch delay slot
		AppendOp( op1 );
	}
	else
	{
		AppendOp( op1 );
		JAL( CCodeLabel( reinterpret_cast< const void * >( p_write_memory ) ), false );		// Don't set branch delay slot
		AppendOp( op2 );
	}

	// Restore all registers BEFORE copying back final value
	RestoreAllRegisters( mRegisterCache, current_regs );

	// Restore the state of the registers
	mRegisterCache = current_regs;
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeGeneratorPSP::GenerateStore( u32 current_pc,
										  EPspReg psp_src,
										  EN64Reg n64_base,
										  s32 offset,
										  OpCodeValue store_op,
										  u32 swizzle,
										  WriteMemoryFunction p_write_memory )
{
	//
	//	Check if the base pointer is a known value, in which case we can store directly.
	//
	if(GenerateDirectStore( psp_src, n64_base, offset, store_op, swizzle ))
	{
		return;
	}

	EPspReg		reg_base( GetRegisterAndLoadLo( n64_base, PspReg_A0 ) );
	EPspReg		reg_address( reg_base );

	if ( (gDynarecStackOptimisation && n64_base == N64Reg_SP)  || (gMemoryAccessOptimisation && mQuickLoad == StaticAnalysis::Segment_8000))
	{
		if( swizzle != 0 )
		{
			if( offset != 0 )
			{
				ADDIU( PspReg_A0, reg_address, offset );
				reg_address = PspReg_A0;
				offset = 0;
			}

			XORI( PspReg_A0, reg_address, swizzle );
			reg_address = PspReg_A0;
		}

		ADDU( PspReg_T1, reg_address, gMemoryBaseReg );
		CAssemblyWriterPSP::StoreRegister( psp_src, store_op, PspReg_T1, offset );
		return;
	}

	if( offset != 0 )
	{
		ADDIU( PspReg_A0, reg_address, offset );
		reg_address = PspReg_A0;
	}

	if( swizzle != 0 )
	{
		XORI( PspReg_A0, reg_address, swizzle );
		reg_address = PspReg_A0;
	}

	if( CAssemblyWriterPSP::IsBufferA() )
	{
		SLT( PspReg_T0, reg_address, gMemUpperBoundReg );	// t1 = upper < address
		CJumpLocation branch( BEQ( PspReg_T0, PspReg_R0, CCodeLabel( NULL ), false ) );
		ADDU( PspReg_T1, reg_address, gMemoryBaseReg );
		CAssemblyWriterPSP::StoreRegister( psp_src, store_op, PspReg_T1, 0 );

		CCodeLabel		continue_location( GetAssemblyBuffer()->GetLabel() );

		//
		//	Generate the handler code in the secondary buffer
		//
		CAssemblyWriterPSP::SetBufferB();

		CCodeLabel		handler_label( GetAssemblyBuffer()->GetLabel() );
		GenerateSlowStore( current_pc, psp_src, reg_address, p_write_memory );

		CJumpLocation	ret_handler( J( continue_location, true ) );
		CAssemblyWriterPSP::SetBufferA();


		//
		//	Keep track of the details we need to generate the jump to the distant handler function
		//
		mAddressCheckFixups.push_back( SAddressCheckFixup( branch, handler_label ) );
	}
	else
	{
		SLT( PspReg_T0, reg_address, gMemUpperBoundReg );	// t1 = upper < address
		ADDU( PspReg_T1, reg_address, gMemoryBaseReg );
		CJumpLocation branch( BNEL( PspReg_T0, PspReg_R0, CCodeLabel( NULL ), false ) );
		CAssemblyWriterPSP::StoreRegister( psp_src, store_op, PspReg_T1, 0 );

		GenerateSlowStore( current_pc, psp_src, reg_address, p_write_memory );

		PatchJumpLong( branch, GetAssemblyBuffer()->GetLabel() );
	}
}


//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateCACHE( EN64Reg base, s16 offset, u32 cache_op )
{
	//u32 cache_op  = op_code.rt;
	//u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );

	u32 cache = cache_op & 0x3;
	u32 action = (cache_op >> 2) & 0x7;

	// For instruction cache invalidation, make sure we let the CPU know so the whole
	// dynarec system can be invalidated
	if(cache == 0 && action == 0)// && address == 0x80000000)
	{
		FlushAllRegisters(mRegisterCache, true);
		LoadRegister(PspReg_A0, base, 0);
		ADDI(PspReg_A0, PspReg_A0, offset);
		JAL( CCodeLabel( reinterpret_cast< const void * >( CPU_InvalidateICacheRange ) ), false );
		ORI(PspReg_A1, PspReg_R0, 0x20);
	}
	//else
	//{
		// We don't care about data cache etc
	//}
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateJAL( u32 address )
{
	//gGPR[REG_ra]._s64 = (s64)(s32)(gCPUState.CurrentPC + 8);		// Store return address
	//u32	new_pc( (gCPUState.CurrentPC & 0xF0000000) | (op_code.target<<2) );
	//CPU_TakeBranch( new_pc, CPU_BRANCH_DIRECT );

	//EPspReg	reg_lo( GetRegisterNoLoadLo( N64Reg_RA, PspReg_T0 ) );
	//LoadConstant( reg_lo, address + 8 );
	//UpdateRegister( N64Reg_RA, reg_lo, URO_HI_SIGN_EXTEND, PspReg_T0 );
	SetRegister32s(N64Reg_RA, address + 8);
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateJR( EN64Reg rs, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump )
{
	//u32	new_pc( gGPR[ op_code.rs ]._u32_0 );
	//CPU_TakeBranch( new_pc, CPU_BRANCH_INDIRECT );

	EPspReg reg_lo( GetRegisterAndLoadLo( rs, PspReg_T0 ) );

	// Necessary? Could just directly compare reg_lo and constant p_branch->TargetAddress??
	//SetVar( &gCPUState.TargetPC, reg_lo );
	//SetVar( &gCPUState.Delay, DO_DELAY );
	//*p_branch_jump = GenerateBranchIfNotEqual( &gCPUState.TargetPC, p_branch->TargetAddress, CCodeLabel() );

	SetVar( &gCPUState.TargetPC, reg_lo );
	//SetVar( &gCPUState.Delay, DO_DELAY );
	*p_branch_jump = GenerateBranchIfNotEqual( reg_lo, p_branch->TargetAddress, CCodeLabel() );


}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateJALR( EN64Reg rs, EN64Reg rd, u32 address, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump )
{
	// Jump And Link
	//u32	new_pc( gGPR[ op_code.rs ]._u32_0 );
	//gGPR[ op_code.rd ]._s64 = (s64)(s32)(gCPUState.CurrentPC + 8);		// Store return address
	//EPspReg	savereg_lo( GetRegisterNoLoadLo( rd, PspReg_T0 ) );

	// Necessary? Could just directly compare reg_lo and constant p_branch->TargetAddress??
	//SetVar( &gCPUState.TargetPC, reg_lo );
	//SetVar( &gCPUState.Delay, DO_DELAY );
	//*p_branch_jump = GenerateBranchIfNotEqual( &gCPUState.TargetPC, p_branch->TargetAddress, CCodeLabel() );
	//LoadConstant( savereg_lo, address + 8 );
	//UpdateRegister( rd, savereg_lo, URO_HI_SIGN_EXTEND, PspReg_T0 );
	SetRegister32s(rd, address + 8);
	
	EPspReg reg_lo( GetRegisterAndLoadLo( rs, PspReg_T0 ) );
	SetVar( &gCPUState.TargetPC, reg_lo );
	*p_branch_jump = GenerateBranchIfNotEqual( reg_lo, p_branch->TargetAddress, CCodeLabel() );


}


//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateMFLO( EN64Reg rd )
{
	//gGPR[ op_code.rd ]._u64 = gCPUState.MultLo._u64;

	EPspReg	reg_lo( GetRegisterNoLoadLo( rd, PspReg_T0 ) );
	GetVar( reg_lo, &gCPUState.MultLo._u32_0 );
	StoreRegisterLo( rd, reg_lo );

	EPspReg	reg_hi( GetRegisterNoLoadHi( rd, PspReg_T0 ) );
	GetVar( reg_hi, &gCPUState.MultLo._u32_1 );
	StoreRegisterHi( rd, reg_hi );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateMFHI( EN64Reg rd )
{
	//gGPR[ op_code.rd ]._u64 = gCPUState.MultHi._u64;

	EPspReg	reg_lo( GetRegisterNoLoadLo( rd, PspReg_T0 ) );
	GetVar( reg_lo, &gCPUState.MultHi._u32_0 );
	StoreRegisterLo( rd, reg_lo );

	EPspReg	reg_hi( GetRegisterNoLoadHi( rd, PspReg_T0 ) );
	GetVar( reg_hi, &gCPUState.MultHi._u32_1 );
	StoreRegisterHi( rd, reg_hi );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateMTLO( EN64Reg rs )
{
	//gCPUState.MultLo._u64 = gGPR[ op_code.rs ]._u64;

	EPspReg	reg_lo( GetRegisterAndLoadLo( rs, PspReg_T0 ) );
	SetVar( &gCPUState.MultLo._u32_0, reg_lo );

	EPspReg	reg_hi( GetRegisterAndLoadHi( rs, PspReg_T0 ) );
	SetVar( &gCPUState.MultLo._u32_1, reg_hi );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateMTHI( EN64Reg rs )
{
	//gCPUState.MultHi._u64 = gGPR[ op_code.rs ]._u64;

	EPspReg	reg_lo( GetRegisterAndLoadLo( rs, PspReg_T0 ) );
	SetVar( &gCPUState.MultHi._u32_0, reg_lo );

	EPspReg	reg_hi( GetRegisterAndLoadHi( rs, PspReg_T0 ) );
	SetVar( &gCPUState.MultHi._u32_1, reg_hi );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateMULT( EN64Reg rs, EN64Reg rt )
{
	//s64 dwResult = (s64)gGPR[ op_code.rs ]._s32_0 * (s64)gGPR[ op_code.rt ]._s32_0;
	//gCPUState.MultLo = (s64)(s32)(dwResult);
	//gCPUState.MultHi = (s64)(s32)(dwResult >> 32);

	EPspReg	reg_lo_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );
	EPspReg	reg_lo_b( GetRegisterAndLoadLo( rt, PspReg_T1 ) );

	MULT( reg_lo_a, reg_lo_b );

	MFLO( PspReg_T0 );
	MFHI( PspReg_T1 );

	SetVar( &gCPUState.MultLo._u32_0, PspReg_T0 );
	SetVar( &gCPUState.MultHi._u32_0, PspReg_T1 );

	SRA( PspReg_T0, PspReg_T0, 0x1f );		// Sign extend
	SRA( PspReg_T1, PspReg_T1, 0x1f );		// Sign extend

	SetVar( &gCPUState.MultLo._u32_1, PspReg_T0 );
	SetVar( &gCPUState.MultHi._u32_1, PspReg_T1 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateMULTU( EN64Reg rs, EN64Reg rt )
{
	//u64 dwResult = (u64)gGPR[ op_code.rs ]._u32_0 * (u64)gGPR[ op_code.rt ]._u32_0;
	//gCPUState.MultLo = (s64)(s32)(dwResult);
	//gCPUState.MultHi = (s64)(s32)(dwResult >> 32);

	EPspReg	reg_lo_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );
	EPspReg	reg_lo_b( GetRegisterAndLoadLo( rt, PspReg_T1 ) );

	MULTU( reg_lo_a, reg_lo_b );

	MFLO( PspReg_T0 );
	MFHI( PspReg_T1 );

	SetVar( &gCPUState.MultLo._u32_0, PspReg_T0 );
	SetVar( &gCPUState.MultHi._u32_0, PspReg_T1 );

	SRA( PspReg_T0, PspReg_T0, 0x1f );		// Sign extend
	SRA( PspReg_T1, PspReg_T1, 0x1f );		// Sign extend

	SetVar( &gCPUState.MultLo._u32_1, PspReg_T0 );
	SetVar( &gCPUState.MultHi._u32_1, PspReg_T1 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateDIV( EN64Reg rs, EN64Reg rt )
{
	//s32 nDividend = gGPR[ op_code.rs ]._s32_0;
	//s32 nDivisor  = gGPR[ op_code.rt ]._s32_0;

	//if (nDivisor)
	//{
	//	gCPUState.MultLo._u64 = (s64)(s32)(nDividend / nDivisor);
	//	gCPUState.MultHi._u64 = (s64)(s32)(nDividend % nDivisor);
	//}

	EPspReg	reg_lo_rs( GetRegisterAndLoadLo( rs, PspReg_T0 ) );
	EPspReg	reg_lo_rt( GetRegisterAndLoadLo( rt, PspReg_T1 ) );

	CJumpLocation	branch( BEQ( reg_lo_rt, PspReg_R0, CCodeLabel(NULL), true ) );		// Can use branch delay for something?

	DIV( reg_lo_rs, reg_lo_rt );

	MFLO( PspReg_T0 );
	MFHI( PspReg_T1 );

	SetVar( &gCPUState.MultLo._u32_0, PspReg_T0 );
	SetVar( &gCPUState.MultHi._u32_0, PspReg_T1 );

	SRA( PspReg_T0, PspReg_T0, 0x1f );		// Sign extend
	SRA( PspReg_T1, PspReg_T1, 0x1f );		// Sign extend

	SetVar( &gCPUState.MultLo._u32_1, PspReg_T0 );
	SetVar( &gCPUState.MultHi._u32_1, PspReg_T1 );

	// Branch here - really should trigger exception!
	PatchJumpLong( branch, GetAssemblyBuffer()->GetLabel() );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateDIVU( EN64Reg rs, EN64Reg rt )
{
	//u32 dwDividend = gGPR[ op_code.rs ]._u32_0;
	//u32 dwDivisor  = gGPR[ op_code.rt ]._u32_0;

	//if (dwDivisor) {
	//	gCPUState.MultLo._u64 = (s64)(s32)(dwDividend / dwDivisor);
	//	gCPUState.MultHi._u64 = (s64)(s32)(dwDividend % dwDivisor);
	//}

	EPspReg	reg_lo_rs( GetRegisterAndLoadLo( rs, PspReg_T0 ) );
	EPspReg	reg_lo_rt( GetRegisterAndLoadLo( rt, PspReg_T1 ) );

	CJumpLocation	branch( BEQ( reg_lo_rt, PspReg_R0, CCodeLabel(NULL), true ) );		// Can use branch delay for something?

	DIVU( reg_lo_rs, reg_lo_rt );

	MFLO( PspReg_T0 );
	MFHI( PspReg_T1 );

	SetVar( &gCPUState.MultLo._u32_0, PspReg_T0 );
	SetVar( &gCPUState.MultHi._u32_0, PspReg_T1 );

	SRA( PspReg_T0, PspReg_T0, 0x1f );		// Sign extend
	SRA( PspReg_T1, PspReg_T1, 0x1f );		// Sign extend

	SetVar( &gCPUState.MultLo._u32_1, PspReg_T0 );
	SetVar( &gCPUState.MultHi._u32_1, PspReg_T1 );

	// Branch here - really should trigger exception!
	PatchJumpLong( branch, GetAssemblyBuffer()->GetLabel() );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateADDU( EN64Reg rd, EN64Reg rs, EN64Reg rt )
{
	if (mRegisterCache.IsKnownValue(rs, 0)
		& mRegisterCache.IsKnownValue(rt, 0))
	{
		SetRegister32s(rd, mRegisterCache.GetKnownValue(rs, 0)._s32
			+ mRegisterCache.GetKnownValue(rt, 0)._s32);
		return;
	}
	
	//gGPR[ op_code.rd ]._s64 = (s64)(s32)( gGPR[ op_code.rs ]._s32_0 + gGPR[ op_code.rt ]._s32_0 );
	EPspReg	reg_lo_d( GetRegisterNoLoadLo( rd, PspReg_T0 ) );
	EPspReg	reg_lo_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );
	EPspReg	reg_lo_b( GetRegisterAndLoadLo( rt, PspReg_T1 ) );
	ADDU( reg_lo_d, reg_lo_a, reg_lo_b );
	UpdateRegister( rd, reg_lo_d, URO_HI_SIGN_EXTEND, PspReg_T0 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSUBU( EN64Reg rd, EN64Reg rs, EN64Reg rt )
{
	if (mRegisterCache.IsKnownValue(rs, 0)
		& mRegisterCache.IsKnownValue(rt, 0))
	{
		SetRegister32s(rd, mRegisterCache.GetKnownValue(rs, 0)._s32
			- mRegisterCache.GetKnownValue(rt, 0)._s32);
		return;
	}

	//gGPR[ op_code.rd ]._s64 = (s64)(s32)( gGPR[ op_code.rs ]._s32_0 - gGPR[ op_code.rt ]._s32_0 );
	EPspReg	reg_lo_d( GetRegisterNoLoadLo( rd, PspReg_T0 ) );
	EPspReg	reg_lo_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );
	EPspReg	reg_lo_b( GetRegisterAndLoadLo( rt, PspReg_T1 ) );
	SUBU( reg_lo_d, reg_lo_a, reg_lo_b );
	UpdateRegister( rd, reg_lo_d, URO_HI_SIGN_EXTEND, PspReg_T0 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateDADDU( EN64Reg rd, EN64Reg rs, EN64Reg rt )
{
	//gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rt ]._u64 + gGPR[ op_code.rs ]._u64

	//890c250:	00c41021 	addu	d_lo,a_lo,b_lo
	//890c254:	0046402b 	sltu	t0,d_lo,a_lo
	//890c260:	ad220000 	sw		d_lo,0(t1)

	//890c258:	00e51821 	addu	d_hi,a_hi,b_hi
	//890c25c:	01031821 	addu	d_hi,t0,d_hi
	//890c268:	ad230004 	sw		d_hi,4(t1)


	EPspReg	reg_lo_d( GetRegisterNoLoadLo( rd, PspReg_V0 ) );
	EPspReg	reg_lo_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );
	EPspReg	reg_lo_b( GetRegisterAndLoadLo( rt, PspReg_T1 ) );

	ADDU( reg_lo_d, reg_lo_a, reg_lo_b );
	SLTU( PspReg_V1, reg_lo_d, reg_lo_a );		// Overflowed?
	StoreRegisterLo( rd, reg_lo_d );

	EPspReg	reg_hi_d( GetRegisterNoLoadHi( rd, PspReg_V0 ) );
	EPspReg	reg_hi_a( GetRegisterAndLoadHi( rs, PspReg_T0 ) );
	EPspReg	reg_hi_b( GetRegisterAndLoadHi( rt, PspReg_T1 ) );

	ADDU( reg_hi_d, reg_hi_a, reg_hi_b );
	ADDU( reg_hi_d, PspReg_V1, reg_hi_d );		// Add on overflow
	StoreRegisterHi( rd, reg_hi_d );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateAND( EN64Reg rd, EN64Reg rs, EN64Reg rt )
{
	//gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rs ]._u64 & gGPR[ op_code.rt ]._u64;
	//Note for some reason Banjo Kazooie doesn't like this... @Kreationz

	// Errrg Banjo seems fine, maybe something else caused the reported freezes? -Salvy
	//if (mRegisterCache.IsKnownValue(rs, 0) && mRegisterCache.IsKnownValue(rs, 1)
	//&&	mRegisterCache.IsKnownValue(rt, 0) && mRegisterCache.IsKnownValue(rt, 1))
	if (mRegisterCache.IsKnownValue(rs, 0) & mRegisterCache.IsKnownValue(rt, 0))
	{
		SetRegister64(rd, 
			mRegisterCache.GetKnownValue(rs, 0)._u32 & mRegisterCache.GetKnownValue(rt, 0)._u32,
			mRegisterCache.GetKnownValue(rs, 1)._u32 & mRegisterCache.GetKnownValue(rt, 1)._u32
			);
	}
	else if (rs == N64Reg_R0 || rt == N64Reg_R0)
	{
		SetRegister64( rd, 0, 0 );
	}
	else
	{
		// XXXX or into dest register
		EPspReg	reg_lo_d( GetRegisterNoLoadLo( rd, PspReg_T0 ) );
		EPspReg	reg_lo_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );
		EPspReg	reg_lo_b( GetRegisterAndLoadLo( rt, PspReg_T1 ) );
		AND( reg_lo_d, reg_lo_a, reg_lo_b );
		StoreRegisterLo( rd, reg_lo_d );

		EPspReg	reg_hi_d( GetRegisterNoLoadHi( rd, PspReg_T0 ) );
		EPspReg	reg_hi_a( GetRegisterAndLoadHi( rs, PspReg_T0 ) );
		EPspReg	reg_hi_b( GetRegisterAndLoadHi( rt, PspReg_T1 ) );
		AND( reg_hi_d, reg_hi_a, reg_hi_b );
		StoreRegisterHi( rd, reg_hi_d );
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateOR( EN64Reg rd, EN64Reg rs, EN64Reg rt )
{
	//gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rs ]._u64 | gGPR[ op_code.rt ]._u64;
	//if (mRegisterCache.IsKnownValue(rs, 0) && mRegisterCache.IsKnownValue(rs, 1) 
	//	&& mRegisterCache.IsKnownValue(rt, 0) && mRegisterCache.IsKnownValue(rt, 1))
	if (mRegisterCache.IsKnownValue(rs, 0) & mRegisterCache.IsKnownValue(rt, 0))
	{
		SetRegister64(rd, 
			mRegisterCache.GetKnownValue(rs, 0)._u32 | mRegisterCache.GetKnownValue(rt, 0)._u32,
			mRegisterCache.GetKnownValue(rs, 1)._u32 | mRegisterCache.GetKnownValue(rt, 1)._u32);
	}
	else if( rs == N64Reg_R0 )
	{
		// This case rarely seems to happen...
		// As RS is zero, the OR is just a copy of RT to RD.
		// Try to avoid loading into a temp register if the dest is cached
		EPspReg reg_lo_d( GetRegisterNoLoadLo( rd, PspReg_T0 ) );
		LoadRegisterLo( reg_lo_d, rt );
		StoreRegisterLo( rd, reg_lo_d );

		EPspReg reg_hi_d( GetRegisterNoLoadHi( rd, PspReg_T0 ) );
		LoadRegisterHi( reg_hi_d, rt );
		StoreRegisterHi( rd, reg_hi_d );
	}
	else if( rt == N64Reg_R0 )
	{
		// As RT is zero, the OR is just a copy of RS to RD.
		// Try to avoid loading into a temp register if the dest is cached
		EPspReg reg_lo_d( GetRegisterNoLoadLo( rd, PspReg_T0 ) );
		LoadRegisterLo( reg_lo_d, rs );
		StoreRegisterLo( rd, reg_lo_d );

		EPspReg reg_hi_d( GetRegisterNoLoadHi( rd, PspReg_T0 ) );
		LoadRegisterHi( reg_hi_d, rs );
		StoreRegisterHi( rd, reg_hi_d );
	}
	else
	{
		EPspReg	reg_lo_d( GetRegisterNoLoadLo( rd, PspReg_T0 ) );
		EPspReg	reg_lo_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );
		EPspReg	reg_lo_b( GetRegisterAndLoadLo( rt, PspReg_T1 ) );
		OR( reg_lo_d, reg_lo_a, reg_lo_b );
		StoreRegisterLo( rd, reg_lo_d );

		EPspReg	reg_hi_d( GetRegisterNoLoadHi( rd, PspReg_T0 ) );
		EPspReg	reg_hi_a( GetRegisterAndLoadHi( rs, PspReg_T0 ) );
		EPspReg	reg_hi_b( GetRegisterAndLoadHi( rt, PspReg_T1 ) );
		OR( reg_hi_d, reg_hi_a, reg_hi_b );
		StoreRegisterHi( rd, reg_hi_d );
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateXOR( EN64Reg rd, EN64Reg rs, EN64Reg rt )
{
	//gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rs ]._u64 ^ gGPR[ op_code.rt ]._u64;
	//if (mRegisterCache.IsKnownValue(rs, 0) && mRegisterCache.IsKnownValue(rs, 1) 
	//	&& mRegisterCache.IsKnownValue(rt, 0) && mRegisterCache.IsKnownValue(rt, 1))
	if (mRegisterCache.IsKnownValue(rs, 0) & mRegisterCache.IsKnownValue(rt, 0))
	{
		SetRegister64(rd, 
			mRegisterCache.GetKnownValue(rs, 0)._u32 ^ mRegisterCache.GetKnownValue(rt, 0)._u32,
			mRegisterCache.GetKnownValue(rs, 1)._u32 ^ mRegisterCache.GetKnownValue(rt, 1)._u32);

		return;
	}

	EPspReg	reg_lo_d( GetRegisterNoLoadLo( rd, PspReg_T0 ) );
	EPspReg	reg_lo_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );
	EPspReg	reg_lo_b( GetRegisterAndLoadLo( rt, PspReg_T1 ) );
	XOR( reg_lo_d, reg_lo_a, reg_lo_b );
	StoreRegisterLo( rd, reg_lo_d );

	EPspReg	reg_hi_d( GetRegisterNoLoadHi( rd, PspReg_T0 ) );
	EPspReg	reg_hi_a( GetRegisterAndLoadHi( rs, PspReg_T0 ) );
	EPspReg	reg_hi_b( GetRegisterAndLoadHi( rt, PspReg_T1 ) );
	XOR( reg_hi_d, reg_hi_a, reg_hi_b );
	StoreRegisterHi( rd, reg_hi_d );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateNOR( EN64Reg rd, EN64Reg rs, EN64Reg rt )
{
	//gGPR[ op_code.rd ]._u64 = ~(gGPR[ op_code.rs ]._u64 | gGPR[ op_code.rt ]._u64);
	//if (mRegisterCache.IsKnownValue(rs, 0) && mRegisterCache.IsKnownValue(rs, 1) 
	//	&& mRegisterCache.IsKnownValue(rt, 0) && mRegisterCache.IsKnownValue(rt, 1))
	if (mRegisterCache.IsKnownValue(rs, 0) & mRegisterCache.IsKnownValue(rt, 0))
	{
		SetRegister64(rd, 
			~(mRegisterCache.GetKnownValue(rs, 0)._u32 | mRegisterCache.GetKnownValue(rt, 0)._u32),
			~(mRegisterCache.GetKnownValue(rs, 1)._u32 | mRegisterCache.GetKnownValue(rt, 1)._u32));

		return;
	}

	EPspReg	reg_lo_d( GetRegisterNoLoadLo( rd, PspReg_T0 ) );
	EPspReg	reg_lo_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );
	EPspReg	reg_lo_b( GetRegisterAndLoadLo( rt, PspReg_T1 ) );
	NOR( reg_lo_d, reg_lo_a, reg_lo_b );
	StoreRegisterLo( rd, reg_lo_d );

	EPspReg	reg_hi_d( GetRegisterNoLoadHi( rd, PspReg_T0 ) );
	EPspReg	reg_hi_a( GetRegisterAndLoadHi( rs, PspReg_T0 ) );
	EPspReg	reg_hi_b( GetRegisterAndLoadHi( rt, PspReg_T1 ) );
	NOR( reg_hi_d, reg_hi_a, reg_hi_b );
	StoreRegisterHi( rd, reg_hi_d );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSLT( EN64Reg rd, EN64Reg rs, EN64Reg rt )
{
	// Because we have a branch here, we need to make sure that we have a consistent view
	// of the registers regardless of whether we take it or not. We pull in the lo halves of
	// the registers here so that they're Valid regardless of whether we take the branch or not
	PrepareCachedRegisterLo( rs );
	PrepareCachedRegisterLo( rt );

	// If possible, we write directly into the destination register. We have to be careful though -
	// if the destination register is the same as either of the source registers we have to use
	// a temporary instead, to avoid overwriting the contents.
	EPspReg reg_lo_d( GetRegisterNoLoadLo( rd, PspReg_T0 ) );

	if(rd == rs || rd == rt)
	{
		reg_lo_d = PspReg_T0;
	}

	EPspReg	reg_hi_a( GetRegisterAndLoadHi( rs, PspReg_T0 ) );
	EPspReg	reg_hi_b( GetRegisterAndLoadHi( rt, PspReg_T1 ) );

	CJumpLocation	branch( BNE( reg_hi_a, reg_hi_b, CCodeLabel(NULL), false ) );
	SLT( reg_lo_d, reg_hi_a, reg_hi_b );		// In branch delay slot

	// If the branch was not taken, it means that the high part of the registers was equal, so compare bottom half

	EPspReg	reg_lo_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );
	EPspReg	reg_lo_b( GetRegisterAndLoadLo( rt, PspReg_T1 ) );

	SLT( reg_lo_d, reg_lo_a, reg_lo_b );

	// Patch up the branch
	PatchJumpLong( branch, GetAssemblyBuffer()->GetLabel() );

	UpdateRegister( rd, reg_lo_d, URO_HI_CLEAR, PspReg_T0 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSLTU( EN64Reg rd, EN64Reg rs, EN64Reg rt )
{
	// Because we have a branch here, we need to make sure that we have a consistant view
	// of the registers regardless of whether we take it or not. We pull in the lo halves of
	// the registers here so that they're Valid regardless of whether we take the branch or not
	PrepareCachedRegisterLo( rs );
	PrepareCachedRegisterLo( rt );

	// If possible, we write directly into the destination register. We have to be careful though -
	// if the destination register is the same as either of the source registers we have to use
	// a temporary instead, to avoid overwriting the contents.
	EPspReg reg_lo_d( GetRegisterNoLoadLo( rd, PspReg_T0 ) );

	if(rd == rs || rd == rt)
	{
		reg_lo_d = PspReg_T0;
	}

	EPspReg	reg_hi_a( GetRegisterAndLoadHi( rs, PspReg_T0 ) );
	EPspReg	reg_hi_b( GetRegisterAndLoadHi( rt, PspReg_T1 ) );

	CJumpLocation	branch( BNE( reg_hi_a, reg_hi_b, CCodeLabel(NULL), false ) );
	SLTU( reg_lo_d, reg_hi_a, reg_hi_b );		// In branch delay slot

	// If the branch was not taken, it means that the high part of the registers was equal, so compare bottom half

	EPspReg	reg_lo_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );
	EPspReg	reg_lo_b( GetRegisterAndLoadLo( rt, PspReg_T1 ) );

	SLTU( reg_lo_d, reg_lo_a, reg_lo_b );

	// Patch up the branch
	PatchJumpLong( branch, GetAssemblyBuffer()->GetLabel() );

	UpdateRegister( rd, reg_lo_d, URO_HI_CLEAR, PspReg_T0 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateADDIU( EN64Reg rt, EN64Reg rs, s16 immediate )
{
	//gGPR[op_code.rt]._s64 = (s64)(s32)(gGPR[op_code.rs]._s32_0 + (s32)(s16)op_code.immediate);

	if( rs == N64Reg_R0 )
	{
		SetRegister32s( rt, immediate );
	}
	else if(mRegisterCache.IsKnownValue( rs, 0 ))
	{
		s32		known_value( mRegisterCache.GetKnownValue( rs, 0 )._s32 + (s32)immediate );
		SetRegister32s( rt, known_value );
	}
	else
	{
		EPspReg dst_reg( GetRegisterNoLoadLo( rt, PspReg_T0 ) );
		EPspReg	src_reg( GetRegisterAndLoadLo( rs, PspReg_T1 ) );
		ADDIU( dst_reg, src_reg, immediate );
		UpdateRegister( rt, dst_reg, URO_HI_SIGN_EXTEND, PspReg_T0 );
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateANDI( EN64Reg rt, EN64Reg rs, u16 immediate )
{
	//gGPR[op_code.rt]._u64 = gGPR[op_code.rs]._u64 & (u64)(u16)op_code.immediate;

	if(mRegisterCache.IsKnownValue( rs, 0 ))
	{
		s32		known_value_lo( mRegisterCache.GetKnownValue( rs, 0 )._u32 & (u32)immediate );
		s32		known_value_hi( 0 );

		SetRegister64( rt, known_value_lo, known_value_hi );
	}
	else
	{
		EPspReg dst_reg( GetRegisterNoLoadLo( rt, PspReg_T0 ) );
		EPspReg	src_reg( GetRegisterAndLoadLo( rs, PspReg_T1 ) );
		ANDI( dst_reg, src_reg, immediate );
		UpdateRegister( rt, dst_reg, URO_HI_CLEAR, PspReg_T0 );
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateORI( EN64Reg rt, EN64Reg rs, u16 immediate )
{
	//gGPR[op_code.rt]._u64 = gGPR[op_code.rs]._u64 | (u64)(u16)op_code.immediate;

	if( rs == N64Reg_R0 )
	{
			// If we're oring again 0, then we're just setting a constant value
			SetRegister64( rt, immediate, 0 );
	}
	else if(mRegisterCache.IsKnownValue( rs, 0 ))
	{
		s32		known_value_lo( mRegisterCache.GetKnownValue( rs, 0 )._u32 | (u32)immediate );
		s32		known_value_hi( mRegisterCache.GetKnownValue( rs, 1 )._u32 );

		SetRegister64( rt, known_value_lo, known_value_hi );
	} 
	else
	{
		EPspReg dst_reg( GetRegisterNoLoadLo( rt, PspReg_T0 ) );
		EPspReg	src_reg( GetRegisterAndLoadLo( rs, PspReg_T1 ) );
		ORI( dst_reg, src_reg, immediate );
		StoreRegisterLo( rt, dst_reg );

		// If the source/dest regs are different we need to copy the high bits across
		if(rt != rs)
		{
			EPspReg dst_reg_hi( GetRegisterNoLoadHi( rt, PspReg_T0 ) );
			LoadRegisterHi( dst_reg_hi, rs );
			StoreRegisterHi( rt, dst_reg_hi );
		}
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateXORI( EN64Reg rt, EN64Reg rs, u16 immediate )
{
	//gGPR[op_code.rt]._u64 = gGPR[op_code.rs]._u64 ^ (u64)(u16)op_code.immediate;

	if(mRegisterCache.IsKnownValue( rs, 0 ))
	{
		s32		known_value_lo( mRegisterCache.GetKnownValue( rs, 0 )._u32 ^ (u32)immediate );
		s32		known_value_hi( mRegisterCache.GetKnownValue( rs, 1 )._u32 );

		SetRegister64( rt, known_value_lo, known_value_hi );
	}
	else
	{
		EPspReg dst_reg( GetRegisterNoLoadLo( rt, PspReg_T0 ) );
		EPspReg	src_reg( GetRegisterAndLoadLo( rs, PspReg_T1 ) );
		XORI( dst_reg, src_reg, immediate );
		StoreRegisterLo( rt, dst_reg );

		// If the source/dest regs are different we need to copy the high bits across
		// (if they are the same, we're xoring 0 to the top half which is essentially a NOP)
		if(rt != rs)
		{
			EPspReg dst_reg_hi( GetRegisterNoLoadHi( rt, PspReg_T0 ) );
			LoadRegisterHi( dst_reg_hi, rs );
			StoreRegisterHi( rt, dst_reg_hi );
		}
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateLUI( EN64Reg rt, s16 immediate )
{
	//gGPR[op_code.rt]._s64 = (s64)(s32)((s32)(s16)op_code.immediate<<16);

	SetRegister32s( rt, s32( immediate ) << 16 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSLTI( EN64Reg rt, EN64Reg rs, s16 immediate )
{
	// Because we have a branch here, we need to make sure that we have a consistant view
	// of the register regardless of whether we take it or not. We pull in the lo halves of
	// the register here so that it's Valid regardless of whether we take the branch or not
	PrepareCachedRegisterLo( rs );

	// If possible, we write directly into the destination register. We have to be careful though -
	// if the destination register is the same as either of the source registers we have to use
	// a temporary instead, to avoid overwriting the contents.
	EPspReg reg_lo_d( GetRegisterNoLoadLo( rt, PspReg_T0 ) );

	if(rt == rs)
	{
		reg_lo_d = PspReg_T0;
	}

	CJumpLocation	branch;

	EPspReg		reg_hi_a( GetRegisterAndLoadHi( rs, PspReg_T0 ) );
	if( immediate >= 0 )
	{
		// Positive data - we can avoid a contant load here
		branch = BNE( reg_hi_a, PspReg_R0, CCodeLabel(NULL), false );
		SLTI( reg_lo_d, reg_hi_a, 0x0000 );		// In branch delay slot
	}
	else
	{
		// Negative data
		LoadConstant( PspReg_T1, -1 );
		branch = BNE( reg_hi_a, PspReg_T1, CCodeLabel(NULL), false );
		SLTI( reg_lo_d, reg_hi_a, 0xffff );		// In branch delay slot
	}

	// If the branch was not taken, it means that the high part of the registers was equal, so compare bottom half
	EPspReg	reg_lo_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );

	SLTI( reg_lo_d, reg_lo_a, immediate );

	// Patch up the branch
	PatchJumpLong( branch, GetAssemblyBuffer()->GetLabel() );

	UpdateRegister( rt, reg_lo_d, URO_HI_CLEAR, PspReg_T0 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSLTIU( EN64Reg rt, EN64Reg rs, s16 immediate )
{
	// Because we have a branch here, we need to make sure that we have a consistent view
	// of the register regardless of whether we take it or not. We pull in the lo halves of
	// the register here so that it's Valid regardless of whether we take the branch or not
	PrepareCachedRegisterLo( rs );

	// If possible, we write directly into the destination register. We have to be careful though -
	// if the destination register is the same as either of the source registers we have to use
	// a temporary instead, to avoid overwriting the contents.
	EPspReg reg_lo_d( GetRegisterNoLoadLo( rt, PspReg_T0 ) );

	if(rt == rs)
	{
		reg_lo_d = PspReg_T0;
	}

	CJumpLocation	branch;

	EPspReg		reg_hi_a( GetRegisterAndLoadHi( rs, PspReg_T0 ) );
	if( immediate >= 0 )
	{
		// Positive data - we can avoid a contant load here
		branch = BNE( reg_hi_a, PspReg_R0, CCodeLabel(NULL), false );
		SLTIU( reg_lo_d, reg_hi_a, 0x0000 );		// In branch delay slot
	}
	else
	{
		// Negative data
		LoadConstant( PspReg_T1, -1 );
		branch = BNE( reg_hi_a, PspReg_T1, CCodeLabel(NULL), false );
		SLTIU( reg_lo_d, reg_hi_a, 0xffff );		// In branch delay slot
	}

	// If the branch was not taken, it means that the high part of the registers was equal, so compare bottom half
	EPspReg	reg_lo_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );

	SLTIU( reg_lo_d, reg_lo_a, immediate );

	// Patch up the branch
	PatchJumpLong( branch, GetAssemblyBuffer()->GetLabel() );

	UpdateRegister( rt, reg_lo_d, URO_HI_CLEAR, PspReg_T0 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSLL( EN64Reg rd, EN64Reg rt, u32 sa )
{
	//gGPR[ op_code.rd ]._s64 = (s64)(s32)( (gGPR[ op_code.rt ]._u32_0 << op_code.sa) & 0xFFFFFFFF );
	if (mRegisterCache.IsKnownValue(rt, 0))
	{
		SetRegister32s(rd, (s32)(mRegisterCache.GetKnownValue(rt, 0)._u32 << sa));
		return;
	}
	
	EPspReg reg_lo_rd( GetRegisterNoLoadLo( rd, PspReg_T0 ) );
	EPspReg	reg_lo_rt( GetRegisterAndLoadLo( rt, PspReg_T0 ) );

	SLL( reg_lo_rd, reg_lo_rt, sa );
	UpdateRegister( rd, reg_lo_rd, URO_HI_SIGN_EXTEND, PspReg_T0 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSRL( EN64Reg rd, EN64Reg rt, u32 sa )
{
	//gGPR[ op_code.rd ]._s64 = (s64)(s32)( gGPR[ op_code.rt ]._u32_0 >> op_code.sa );
	if (mRegisterCache.IsKnownValue(rt, 0))
	{
		SetRegister32s(rd, (s32)(mRegisterCache.GetKnownValue(rt, 0)._u32 >> sa));
		return;
	}

	EPspReg reg_lo_rd( GetRegisterNoLoadLo( rd, PspReg_T0 ) );
	EPspReg	reg_lo_rt( GetRegisterAndLoadLo( rt, PspReg_T0 ) );

	SRL( reg_lo_rd, reg_lo_rt, sa );
	UpdateRegister( rd, reg_lo_rd, URO_HI_SIGN_EXTEND, PspReg_T0 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSRA( EN64Reg rd, EN64Reg rt, u32 sa )
{
	//gGPR[ op_code.rd ]._s64 = (s64)(s32)( gGPR[ op_code.rt ]._s32_0 >> op_code.sa );
	if (mRegisterCache.IsKnownValue(rt, 0))
	{
		SetRegister32s(rd, mRegisterCache.GetKnownValue(rt, 0)._s32 >> sa);
		return;
	}

	EPspReg reg_lo_rd( GetRegisterNoLoadLo( rd, PspReg_T0 ) );
	EPspReg	reg_lo_rt( GetRegisterAndLoadLo( rt, PspReg_T0 ) );

	SRA( reg_lo_rd, reg_lo_rt, sa );
	UpdateRegister( rd, reg_lo_rd, URO_HI_SIGN_EXTEND, PspReg_T0 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSLLV( EN64Reg rd, EN64Reg rs, EN64Reg rt )
{
	//gGPR[ op_code.rd ]._s64 = (s64)(s32)( (gGPR[ op_code.rt ]._u32_0 << ( gGPR[ op_code.rs ]._u32_0 & 0x1F ) ) & 0xFFFFFFFF );
	if (mRegisterCache.IsKnownValue(rs, 0)
		& mRegisterCache.IsKnownValue(rt, 0))
	{
		SetRegister32s(rd, (s32)(mRegisterCache.GetKnownValue(rt, 0)._u32 << 
			(mRegisterCache.GetKnownValue(rs, 0)._u32 & 0x1F)));
		return;
	}

	// PSP sllv does exactly the same op as n64- no need for masking
	EPspReg reg_lo_rd( GetRegisterNoLoadLo( rd, PspReg_T0 ) );
	EPspReg reg_lo_rs( GetRegisterAndLoadLo( rs, PspReg_T1 ) );
	EPspReg	reg_lo_rt( GetRegisterAndLoadLo( rt, PspReg_T0 ) );

	SLLV( reg_lo_rd, reg_lo_rs, reg_lo_rt );
	UpdateRegister( rd, reg_lo_rd, URO_HI_SIGN_EXTEND, PspReg_T0 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSRLV( EN64Reg rd, EN64Reg rs, EN64Reg rt )
{
	//gGPR[ op_code.rd ]._s64 = (s64)(s32)( gGPR[ op_code.rt ]._u32_0 >> ( gGPR[ op_code.rs ]._u32_0 & 0x1F ) );
	if (mRegisterCache.IsKnownValue(rs, 0)
		& mRegisterCache.IsKnownValue(rt, 0))
	{
		SetRegister32s(rd, (s32)(mRegisterCache.GetKnownValue(rt, 0)._u32 >> 
			(mRegisterCache.GetKnownValue(rs, 0)._u32 & 0x1F)));
		return;
	}

	// PSP srlv does exactly the same op as n64- no need for masking
	EPspReg reg_lo_rd( GetRegisterNoLoadLo( rd, PspReg_T0 ) );
	EPspReg reg_lo_rs( GetRegisterAndLoadLo( rs, PspReg_T1 ) );
	EPspReg	reg_lo_rt( GetRegisterAndLoadLo( rt, PspReg_T0 ) );

	SRLV( reg_lo_rd, reg_lo_rs, reg_lo_rt );
	UpdateRegister( rd, reg_lo_rd, URO_HI_SIGN_EXTEND, PspReg_T0 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSRAV( EN64Reg rd, EN64Reg rs, EN64Reg rt )
{
	//gGPR[ op_code.rd ]._s64 = (s64)(s32)( gGPR[ op_code.rt ]._s32_0 >> ( gGPR[ op_code.rs ]._u32_0 & 0x1F ) );
	if (mRegisterCache.IsKnownValue(rs, 0)
		& mRegisterCache.IsKnownValue(rt, 0))
	{
		SetRegister32s(rd, (s32)(mRegisterCache.GetKnownValue(rt, 0)._s32 >> 
			(mRegisterCache.GetKnownValue(rs, 0)._u32 & 0x1F)));
		return;
	}

	// PSP srlv does exactly the same op as n64- no need for masking
	EPspReg reg_lo_rd( GetRegisterNoLoadLo( rd, PspReg_T0 ) );
	EPspReg reg_lo_rs( GetRegisterAndLoadLo( rs, PspReg_T1 ) );
	EPspReg	reg_lo_rt( GetRegisterAndLoadLo( rt, PspReg_T0 ) );

	SRAV( reg_lo_rd, reg_lo_rs, reg_lo_rt );
	UpdateRegister( rd, reg_lo_rd, URO_HI_SIGN_EXTEND, PspReg_T0 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateLB( u32 address, bool set_branch_delay, EN64Reg rt, EN64Reg base, s32 offset )
{
	EPspReg	reg_dst( GetRegisterNoLoadLo( rt, PspReg_V0 ) );	// Use V0 to avoid copying return value if reg is not cached

	GenerateLoad( address, reg_dst, base, offset, OP_LB, 3, set_branch_delay ? ReadBitsDirectBD_s8 : ReadBitsDirect_s8 );	// NB this fills the whole of reg_dst

	UpdateRegister( rt, reg_dst, URO_HI_SIGN_EXTEND, PspReg_T0 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateLBU( u32 address, bool set_branch_delay, EN64Reg rt, EN64Reg base, s32 offset )
{
	EPspReg	reg_dst( GetRegisterNoLoadLo( rt, PspReg_V0 ) );	// Use V0 to avoid copying return value if reg is not cached

	GenerateLoad( address, reg_dst, base, offset, OP_LBU, 3, set_branch_delay ? ReadBitsDirectBD_u8 : ReadBitsDirect_u8 );	// NB this fills the whole of reg_dst

	UpdateRegister( rt, reg_dst, URO_HI_CLEAR, PspReg_T0 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateLH( u32 address, bool set_branch_delay, EN64Reg rt, EN64Reg base, s32 offset )
{
	EPspReg	reg_dst( GetRegisterNoLoadLo( rt, PspReg_V0 ) );	// Use V0 to avoid copying return value if reg is not cached

	GenerateLoad( address, reg_dst, base, offset, OP_LH, 2, set_branch_delay ? ReadBitsDirect_s16 : ReadBitsDirect_s16 );	// NB this fills the whole of reg_dst

	UpdateRegister( rt, reg_dst, URO_HI_SIGN_EXTEND, PspReg_T0 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateLHU( u32 address, bool set_branch_delay, EN64Reg rt, EN64Reg base, s32 offset )
{
	EPspReg	reg_dst( GetRegisterNoLoadLo( rt, PspReg_V0 ) );	// Use V0 to avoid copying return value if reg is not cached

	GenerateLoad( address, reg_dst, base, offset, OP_LHU, 2, set_branch_delay ? ReadBitsDirectBD_u16 : ReadBitsDirect_u16 );	// NB this fills the whole of reg_dst

	UpdateRegister( rt, reg_dst, URO_HI_CLEAR, PspReg_T0 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateLW( u32 address, bool set_branch_delay, EN64Reg rt, EN64Reg base, s16 offset )
{
	EPspReg	reg_dst( GetRegisterNoLoadLo( rt, PspReg_V0 ) );	// Use V0 to avoid copying return value if reg is not cached

	GenerateLoad( address, reg_dst, base, offset, OP_LW, 0, set_branch_delay ? ReadBitsDirectBD_u32 : ReadBitsDirect_u32 );

	UpdateRegister( rt, reg_dst, URO_HI_SIGN_EXTEND, PspReg_T0 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateLWC1( u32 address, bool set_branch_delay, u32 ft, EN64Reg base, s32 offset )
{
	EN64FloatReg	n64_ft = EN64FloatReg( ft );
	EPspFloatReg	psp_ft = EPspFloatReg( n64_ft );// 1:1 Mapping

	//u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );
	//value = Read32Bits(address);
	//gCPUState.FPU[op_code.ft]._s32_0 = value;

	// TODO: Actually perform LWC1 here
	EPspReg	reg_dst( PspReg_V0 );				// GenerateLoad is slightly more efficient when using V0
	GenerateLoad( address, reg_dst, base, offset, OP_LW, 0, set_branch_delay ? ReadBitsDirectBD_u32 : ReadBitsDirect_u32 );

	//SetVar( &gCPUState.FPU[ ft ]._u32_0, reg_dst );
	MTC1( psp_ft, reg_dst );
	UpdateFloatRegister( n64_ft );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSB( u32 current_pc, bool set_branch_delay, EN64Reg rt, EN64Reg base, s32 offset )
{
	EPspReg		reg_value( GetRegisterAndLoadLo( rt, PspReg_A1 ) );

	GenerateStore( current_pc, reg_value, base, offset, OP_SB, 3, set_branch_delay ? WriteBitsDirectBD_u8 : WriteBitsDirect_u8 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSH( u32 current_pc, bool set_branch_delay, EN64Reg rt, EN64Reg base, s32 offset )
{
	EPspReg		reg_value( GetRegisterAndLoadLo( rt, PspReg_A1 ) );

	GenerateStore( current_pc, reg_value, base, offset, OP_SH, 2, set_branch_delay ? WriteBitsDirectBD_u16 : WriteBitsDirect_u16 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSW( u32 current_pc, bool set_branch_delay, EN64Reg rt, EN64Reg base, s32 offset )
{
	EPspReg		reg_value( GetRegisterAndLoadLo( rt, PspReg_A1 ) );

	GenerateStore( current_pc, reg_value, base, offset, OP_SW, 0, set_branch_delay ? WriteBitsDirectBD_u32 : WriteBitsDirect_u32 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSWC1( u32 current_pc, bool set_branch_delay, u32 ft, EN64Reg base, s32 offset )
{
	EN64FloatReg	n64_ft = EN64FloatReg( ft );
	EPspFloatReg	psp_ft( GetFloatRegisterAndLoad( n64_ft ) );
	MFC1( PspReg_A1, psp_ft );

	GenerateStore( current_pc, PspReg_A1, base, offset, OP_SW, 0, set_branch_delay ? WriteBitsDirectBD_u32 : WriteBitsDirect_u32 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateMFC1( EN64Reg rt, u32 fs )
{
	//gGPR[ op_code.rt ]._s64 = (s64)(s32)gCPUState.FPU[fs]._s32_0

	EPspReg			reg_dst( GetRegisterNoLoadLo( rt, PspReg_T0 ) );
	EN64FloatReg	n64_fs = EN64FloatReg( fs );
	EPspFloatReg	psp_fs( GetFloatRegisterAndLoad( n64_fs ) );

	MFC1( reg_dst, psp_fs );
	UpdateRegister( rt, reg_dst, URO_HI_SIGN_EXTEND, PspReg_T0 );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateMTC1( u32 fs, EN64Reg rt )
{
	//gCPUState.FPU[fs]._s32_0 = gGPR[ op_code.rt ]._s32_0;

	EPspReg			psp_rt( GetRegisterAndLoadLo( rt, PspReg_T0 ) );
	EN64FloatReg	n64_fs = EN64FloatReg( fs );
	EPspFloatReg	psp_fs = EPspFloatReg( n64_fs );//1:1 Mapping

	MTC1( psp_fs, psp_rt );
	UpdateFloatRegister( n64_fs );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateCFC1( EN64Reg rt, u32 fs )
{
	//if( fs == 0 || fs == 31 )
	//Saves a compare //Corn
	if( !((fs + 1) & 0x1E) )
	{
		EPspReg			reg_dst( GetRegisterNoLoadLo( rt, PspReg_T0 ) );

		GetVar( reg_dst, &gCPUState.FPUControl[ fs ]._u32_0 );
		UpdateRegister( rt, reg_dst, URO_HI_SIGN_EXTEND, PspReg_T0 );
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateCTC1( u32 fs, EN64Reg rt )
{
	EPspReg			psp_rt_lo( GetRegisterAndLoadLo( rt, PspReg_T0 ) );
	SetVar( &gCPUState.FPUControl[ fs ]._u32_0, psp_rt_lo );

	EPspReg			psp_rt_hi( GetRegisterAndLoadHi( rt, PspReg_T0 ) );
	SetVar( &gCPUState.FPUControl[ fs ]._u32_1, psp_rt_hi );

	//XXXX TODO:
	// Change the rounding mode
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateBEQ( EN64Reg rs, EN64Reg rt, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump )
{
	DAEDALUS_ASSERT( p_branch != NULL, "No branch details?" );
	DAEDALUS_ASSERT( p_branch->Direct, "Indirect branch for BEQ?" );

	// One or other of these may be r0 - we don't really care for optimisation purposes though
	// as ultimately the register load regs factored out
	EPspReg		reg_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );
	EPspReg		reg_b( GetRegisterAndLoadLo( rt, PspReg_T1 ) );

	// XXXX This may actually need to be a 64 bit compare, but this is what R4300.cpp does

	if( p_branch->ConditionalBranchTaken )
	{
		// Flip the sign of the test -
		*p_branch_jump = BNE( reg_a, reg_b, CCodeLabel(NULL), true );
	}
	else
	{
		*p_branch_jump = BEQ( reg_a, reg_b, CCodeLabel(NULL), true );
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateBNE( EN64Reg rs, EN64Reg rt, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump )
{
	DAEDALUS_ASSERT( p_branch != NULL, "No branch details?" );
	DAEDALUS_ASSERT( p_branch->Direct, "Indirect branch for BNE?" );

	// One or other of these may be r0 - we don't really care for optimisation purposes though
	// as ultimately the register load regs factored out
	EPspReg		reg_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );
	EPspReg		reg_b( GetRegisterAndLoadLo( rt, PspReg_T1 ) );

	// XXXX This may actually need to be a 64 bit compare, but this is what R4300.cpp does

	if( p_branch->ConditionalBranchTaken )
	{
		// Flip the sign of the test -
		*p_branch_jump = BEQ( reg_a, reg_b, CCodeLabel(NULL), true );
	}
	else
	{
		*p_branch_jump = BNE( reg_a, reg_b, CCodeLabel(NULL), true );
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateBLEZ( EN64Reg rs, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump )
{
	DAEDALUS_ASSERT( p_branch != NULL, "No branch details?" );
	DAEDALUS_ASSERT( p_branch->Direct, "Indirect branch for BLEZ?" );

	EPspReg		reg_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );

	// XXXX This may actually need to be a 64 bit compare, but this is what R4300.cpp does

	if( p_branch->ConditionalBranchTaken )
	{
		// Flip the sign of the test -
		*p_branch_jump = BGTZ( reg_a, CCodeLabel(NULL), true );
	}
	else
	{
		*p_branch_jump = BLEZ( reg_a, CCodeLabel(NULL), true );
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateBGTZ( EN64Reg rs, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump )
{
	DAEDALUS_ASSERT( p_branch != NULL, "No branch details?" );
	DAEDALUS_ASSERT( p_branch->Direct, "Indirect branch for BGTZ?" );

	EPspReg		reg_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );

	// XXXX This may actually need to be a 64 bit compare, but this is what R4300.cpp does

	if( p_branch->ConditionalBranchTaken )
	{
		// Flip the sign of the test -
		*p_branch_jump = BLEZ( reg_a, CCodeLabel(NULL), true );
	}
	else
	{
		*p_branch_jump = BGTZ( reg_a, CCodeLabel(NULL), true );
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateBLTZ( EN64Reg rs, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump )
{
	DAEDALUS_ASSERT( p_branch != NULL, "No branch details?" );
	DAEDALUS_ASSERT( p_branch->Direct, "Indirect branch for BLTZ?" );

	EPspReg		reg_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );

	// XXXX This should actually need to be a 64 bit compare???

	if( p_branch->ConditionalBranchTaken )
	{
		// Flip the sign of the test -
		*p_branch_jump = BGEZ( reg_a, CCodeLabel(NULL), true );
	}
	else
	{
		*p_branch_jump = BLTZ( reg_a, CCodeLabel(NULL), true );
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateBGEZ( EN64Reg rs, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump )
{
	DAEDALUS_ASSERT( p_branch != NULL, "No branch details?" );
	DAEDALUS_ASSERT( p_branch->Direct, "Indirect branch for BGEZ?" );

	EPspReg		reg_a( GetRegisterAndLoadLo( rs, PspReg_T0 ) );

	// XXXX This should actually need to be a 64 bit compare???

	if( p_branch->ConditionalBranchTaken )
	{
		// Flip the sign of the test -
		*p_branch_jump = BLTZ( reg_a, CCodeLabel(NULL), true );
	}
	else
	{
		*p_branch_jump = BGEZ( reg_a, CCodeLabel(NULL), true );
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateBC1F( const SBranchDetails * p_branch, CJumpLocation * p_branch_jump )
{
	DAEDALUS_ASSERT( p_branch != NULL, "No branch details?" );
	DAEDALUS_ASSERT( p_branch->Direct, "Indirect branch for BC1F?" );

	GetVar( PspReg_T0, &gCPUState.FPUControl[31]._u32_0 );
	LoadConstant( PspReg_T1, FPCSR_C );
	AND( PspReg_T0, PspReg_T0, PspReg_T1 );

	if( p_branch->ConditionalBranchTaken )
	{
		// Flip the sign of the test -
		*p_branch_jump = BNE( PspReg_T0, PspReg_R0, CCodeLabel(NULL), true );
	}
	else
	{
		*p_branch_jump = BEQ( PspReg_T0, PspReg_R0, CCodeLabel(NULL), true );
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateBC1T( const SBranchDetails * p_branch, CJumpLocation * p_branch_jump )
{
	DAEDALUS_ASSERT( p_branch != NULL, "No branch details?" );
	DAEDALUS_ASSERT( p_branch->Direct, "Indirect branch for BC1T?" );

	GetVar( PspReg_T0, &gCPUState.FPUControl[31]._u32_0 );
	LoadConstant( PspReg_T1, FPCSR_C );
	AND( PspReg_T0, PspReg_T0, PspReg_T1 );

	if( p_branch->ConditionalBranchTaken )
	{
		// Flip the sign of the test -
		*p_branch_jump = BEQ( PspReg_T0, PspReg_R0, CCodeLabel(NULL), true );
	}
	else
	{
		*p_branch_jump = BNE( PspReg_T0, PspReg_R0, CCodeLabel(NULL), true );
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateADD_S( u32 fd, u32 fs, u32 ft )
{
	EN64FloatReg	n64_fs = EN64FloatReg( fs );
	EN64FloatReg	n64_ft = EN64FloatReg( ft );
	EN64FloatReg	n64_fd = EN64FloatReg( fd );

	EPspFloatReg	psp_fd = EPspFloatReg( n64_fd );//1:1 Mapping
	EPspFloatReg	psp_fs( GetFloatRegisterAndLoad( n64_fs ) );
	EPspFloatReg	psp_ft( GetFloatRegisterAndLoad( n64_ft ) );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	ADD_S( psp_fd, psp_fs, psp_ft );

	UpdateFloatRegister( n64_fd );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSUB_S( u32 fd, u32 fs, u32 ft )
{
	EN64FloatReg	n64_fs = EN64FloatReg( fs );
	EN64FloatReg	n64_ft = EN64FloatReg( ft );
	EN64FloatReg	n64_fd = EN64FloatReg( fd );

	EPspFloatReg	psp_fd = EPspFloatReg(n64_fd ); //1:1 Mapping
	EPspFloatReg	psp_fs( GetFloatRegisterAndLoad( n64_fs ) );
	EPspFloatReg	psp_ft( GetFloatRegisterAndLoad( n64_ft ) );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	SUB_S( psp_fd, psp_fs, psp_ft );

	UpdateFloatRegister( n64_fd );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateMUL_S( u32 fd, u32 fs, u32 ft )
{
	EN64FloatReg	n64_fs = EN64FloatReg( fs );
	EN64FloatReg	n64_ft = EN64FloatReg( ft );
	EN64FloatReg	n64_fd = EN64FloatReg( fd );

	EPspFloatReg	psp_fd = EPspFloatReg( n64_fd );
	EPspFloatReg	psp_fs( GetFloatRegisterAndLoad( n64_fs ) );
	EPspFloatReg	psp_ft( GetFloatRegisterAndLoad( n64_ft ) );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	MUL_S( psp_fd, psp_fs, psp_ft );

	UpdateFloatRegister( n64_fd );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateDIV_S( u32 fd, u32 fs, u32 ft )
{
	EN64FloatReg	n64_fs = EN64FloatReg( fs );
	EN64FloatReg	n64_ft = EN64FloatReg( ft );
	EN64FloatReg	n64_fd = EN64FloatReg( fd );

	EPspFloatReg	psp_fd = EPspFloatReg( n64_fd );
	EPspFloatReg	psp_fs( GetFloatRegisterAndLoad( n64_fs ) );
	EPspFloatReg	psp_ft( GetFloatRegisterAndLoad( n64_ft ) );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	DIV_S( psp_fd, psp_fs, psp_ft );

	UpdateFloatRegister( n64_fd );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSQRT_S( u32 fd, u32 fs )
{
	EN64FloatReg	n64_fs = EN64FloatReg( fs );
	EN64FloatReg	n64_fd = EN64FloatReg( fd );

	EPspFloatReg	psp_fd = EPspFloatReg( n64_fd );
	EPspFloatReg	psp_fs( GetFloatRegisterAndLoad( n64_fs ) );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	SQRT_S( psp_fd, psp_fs );

	UpdateFloatRegister( n64_fd );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateABS_S( u32 fd, u32 fs )
{
	EN64FloatReg	n64_fs = EN64FloatReg( fs );
	EN64FloatReg	n64_fd = EN64FloatReg( fd );

	EPspFloatReg	psp_fd = EPspFloatReg( n64_fd );
	EPspFloatReg	psp_fs( GetFloatRegisterAndLoad( n64_fs ) );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	ABS_S( psp_fd, psp_fs );

	UpdateFloatRegister( n64_fd );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateMOV_S( u32 fd, u32 fs )
{
	EN64FloatReg	n64_fs = EN64FloatReg( fs );
	EN64FloatReg	n64_fd = EN64FloatReg( fd );

	EPspFloatReg	psp_fd = EPspFloatReg( n64_fd );
	EPspFloatReg	psp_fs( GetFloatRegisterAndLoad( n64_fs ) );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	MOV_S( psp_fd, psp_fs );

	UpdateFloatRegister( n64_fd );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateNEG_S( u32 fd, u32 fs )
{
	EN64FloatReg	n64_fs = EN64FloatReg( fs );
	EN64FloatReg	n64_fd = EN64FloatReg( fd );

	EPspFloatReg	psp_fd = EPspFloatReg( n64_fd );
	EPspFloatReg	psp_fs( GetFloatRegisterAndLoad( n64_fs ) );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	NEG_S( psp_fd, psp_fs );

	UpdateFloatRegister( n64_fd );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateTRUNC_W_S( u32 fd, u32 fs )
{
	EN64FloatReg	n64_fs = EN64FloatReg( fs );
	EN64FloatReg	n64_fd = EN64FloatReg( fd );

	EPspFloatReg	psp_fd = EPspFloatReg( n64_fd );
	EPspFloatReg	psp_fs( GetFloatRegisterAndLoad( n64_fs ) );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	TRUNC_W_S( psp_fd, psp_fs );

	UpdateFloatRegister( n64_fd );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateCVT_W_S( u32 fd, u32 fs )
{
	EN64FloatReg	n64_fs = EN64FloatReg( fs );
	EN64FloatReg	n64_fd = EN64FloatReg( fd );

	EPspFloatReg	psp_fd = EPspFloatReg( n64_fd );
	EPspFloatReg	psp_fs( GetFloatRegisterAndLoad( n64_fs ) );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	CVT_W_S( psp_fd, psp_fs );

	UpdateFloatRegister( n64_fd );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateCMP_S( u32 fs, ECop1OpFunction cmp_op, u32 ft )
{
	EN64FloatReg	n64_fs = EN64FloatReg( fs );
	EN64FloatReg	n64_ft = EN64FloatReg( ft );

	EPspFloatReg	psp_fs( GetFloatRegisterAndLoad( n64_fs ) );
	EPspFloatReg	psp_ft( GetFloatRegisterAndLoad( n64_ft ) );

	CMP_S( psp_fs, cmp_op, psp_ft );
	GetVar( PspReg_T0, &gCPUState.FPUControl[31]._u32_0 );

	CCodeLabel	no_target( NULL );

	PspOpCode		op1;
	PspOpCode		op2;

	GetLoadConstantOps( PspReg_T1, FPCSR_C, &op1, &op2 );

	CJumpLocation	test_condition;

	// Insert a test to check the branch condition flag. Use the delay slot to load the constant
	if( op2._u32 == 0 )
	{
		test_condition = BC1F( no_target, false );
		AppendOp( op1 );
	}
	else
	{
		AppendOp( op1 );
		test_condition = BC1F( no_target, false );
		AppendOp( op2 );
	}

	CJumpLocation	branch_exit( BEQ( PspReg_R0, PspReg_R0, no_target, false ) );
	OR( PspReg_T0, PspReg_T0, PspReg_T1 );		// flat |= c

	CCodeLabel		condition_false( GetAssemblyBuffer()->GetLabel() );
	NOR( PspReg_T1, PspReg_T1, PspReg_R0 );		// c = !c
	AND( PspReg_T0, PspReg_T0, PspReg_T1 );		// flag &= !c

	CCodeLabel		exit_label( GetAssemblyBuffer()->GetLabel() );

	SetVar( &gCPUState.FPUControl[31]._u32_0, PspReg_T0 );

	PatchJumpLong( test_condition, condition_false );
	PatchJumpLong( branch_exit, exit_label );

}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateADD_D( u32 fd, u32 fs, u32 ft )
{
	NOT_IMPLEMENTED( __FUNCTION__ );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSUB_D( u32 fd, u32 fs, u32 ft )
{
	NOT_IMPLEMENTED( __FUNCTION__ );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateMUL_D( u32 fd, u32 fs, u32 ft )
{
	NOT_IMPLEMENTED( __FUNCTION__ );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateDIV_D( u32 fd, u32 fs, u32 ft )
{
	NOT_IMPLEMENTED( __FUNCTION__ );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateSQRT_D( u32 fd, u32 fs )
{
	NOT_IMPLEMENTED( __FUNCTION__ );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateMOV_D( u32 fd, u32 fs )
{
	NOT_IMPLEMENTED( __FUNCTION__ );
}

//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateNEG_D( u32 fd, u32 fs )
{
	NOT_IMPLEMENTED( __FUNCTION__ );
}


//*****************************************************************************
//
//*****************************************************************************
inline void	CCodeGeneratorPSP::GenerateCVT_S_W( u32 fd, u32 fs )
{
	EN64FloatReg	n64_fs = EN64FloatReg( fs );
	EN64FloatReg	n64_fd = EN64FloatReg( fd );

	EPspFloatReg	psp_fd = EPspFloatReg( n64_fd );
	EPspFloatReg	psp_fs( GetFloatRegisterAndLoad( n64_fs ) );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	CVT_S_W( psp_fd, psp_fs );

	UpdateFloatRegister( n64_fd );
}
