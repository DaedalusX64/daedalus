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

#include "stdafx.h"
#include "R4300.h"

#include "Debug/DebugLog.h"
#include "CPU.h"
#include "Interrupt.h"
#include "DynaRec/TraceRecorder.h"

#include "OSHLE/ultra_R4300.h"

#include "Math/Math.h"	// VFPU Math

#include "ROM.h"
#include "Core/Registers.h"			// For REG_?? defines
#include "Debug/DBGConsole.h"
#include "ConfigOptions.h"

#include <pspfpu.h>

// ToDo Move these macros to a new header.
//
#define	R4300_CALL_MAKE_OP( var )	OpCode	var;	var._u32 = op_code_bits

//*************************************************************************************
//
//*************************************************************************************
#ifndef DAEDALUS_SILENT
#define CATCH_NAN_EXCEPTION(op, valX, valY) \
	if(pspFpuIsNaN(valX + valY)) \
	{ \
		DBGConsole_Msg( 0, "Should throw fp nan exception in %s ?",op ); \
		return; \
	}
#else
	#define CATCH_NAN_EXCEPTION(op, valX, valY)
#endif

#ifndef DAEDALUS_SILENT
inline void CHECK_R0( u32 op )
{
	if(gGPR[0]._u64 != 0) 
	{
		DBGConsole_Msg(0, "Warning: Attempted write to r0!"); \
		gGPR[0]._u64 = 0; // Ensure r0 is always zero (easier than trapping writes?)
		//return;
	}

	DAEDALUS_ASSERT( op, "Possible attempt to write to r0!");
}
#else
	inline void CHECK_R0( u32 op ) {}
#endif

#define CHECK_COP1_UNUSUABLE \
{ \
	if (!(gCPUState.CPUControl[C0_SR]._u32_0 & SR_CU1)) \
	{ \
		DBGConsole_Msg(0, "Thread accessing Cop1, throwing COP1 unusuable exception"); \
		R4300_Exception_CopUnusuable(); \
		return; \
	} \
}
//
//	Abstract away the different rounding modes between targets
//
enum ERoundingMode
{
	RM_ROUND = 0,
	RM_TRUNC,
	RM_CEIL,
	RM_FLOOR,
	RM_NUM_MODES,
};
static ERoundingMode	gRoundingMode( RM_ROUND );

// If the hardware doesn't support doubles in hardware - use 32 bits floats and accept the loss in precision
typedef f32 d64;


inline void SpeedHack( u32 pc, OpCode op )
{
	OpCode	next_op;

#ifdef DAEDALUS_ENABLE_DYNAREC
	if (gTraceRecorder.IsTraceActive())
		return;
#endif

	// TODO: Should maybe use some internal function, so we can account
	// for things like Branch/DelaySlot pair straddling a page boundary.
	next_op._u32 = Read32Bits( pc + 4 );

	// If nop, then this is a busy-wait for an interrupt
	if (next_op._u32 == 0)
	{
		// XXXX if we leave the counter at 1, then we always terminate traces with a delay slot active.
		// Need a more permenant fix to for this - i.e. making tracing more robust.
		CPU_SkipToNextEvent();
	}
	// XXXX check this....need to update count....
	// This is:
	// 0x7f0d01e8: BNEL      v0 != v1 --> 0x7f0d01e8
	// 0x7f0d01ec: ADDIU     v0 = v0 + 0x0004
	else if (op._u32 == 0x5443ffff && next_op._u32 == 0x24420004)
	{
		gGPR[REG_v0]._u64 = gGPR[REG_v1]._u64 - 4;
	}
	/*else
	{
		static bool warned = false;

		if (!warned)
		{
			DBGConsole_Msg(0, "Missed Speedhack 0x%08x", gCPUState.CurrentPC);
			warned = true;
		}
	}*/
}

//
//	A bit on FPU exceptions.
//		LWC1/LDC1/SWC1/SDC1		are unformatted, so no exceptions raised
//		MTC1/MFC1/DMTC1/DMFC1	are unformatted, so no exceptions raised
//		Word accessed must be 4 byte aligned, DWord accesses must be 8 byte aligned
//
//*****************************************************************************
//
//*****************************************************************************
inline void StoreFPR_Long( u32 reg, u64 value )
{
	REG64	r;
	r._u64 = value;
		
	// TODO - Don't think these need sign extending...
	gCPUState.FPU[reg+0]._u32_0 = r._u32_0;
	gCPUState.FPU[reg+1]._u32_0 = r._u32_1;
}

#define SIMULATESIG 0x12345678
//*****************************************************************************
//
//*****************************************************************************
inline u64 LoadFPR_Long( u32 reg )
{
	REG64 res;

	res._u32_1 = gCPUState.FPU[reg+1]._u32_0;
	res._u32_0 = gCPUState.FPU[reg+0]._u32_0;

	// Disabled, looks suspicious to me, I don't think it does what we want here? ~Salvy
	/*if (res._f64_unused == SIMULATESIG)
	{
		res._f64 = (f64)res._f64_sim;
	}*/

	return res._u64;
}

//*****************************************************************************
//
//*****************************************************************************
inline d64 LoadFPR_Double( u32 reg )
{
	REG64 res;

	res._u32_1 = gCPUState.FPU[reg+1]._u32_0;
	res._u32_0 = gCPUState.FPU[reg+0]._u32_0;

	if (res._f64_unused == SIMULATESIG)
	{
		// converted
		return (d64)res._f64_sim;
	}
	else
	{
		return (d64)res._f64;
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void StoreFPR_Double( u32 reg, d64 value )
{
	// This fixes Mario Party's draft mini game.
	// And green / static textures bug in Conker.
	if(gSimulateDoubleDisabled)
	{
		REG64 r; 
		r._f64 = pspFpuFloatToDouble( value );	//r._f64 = f32( value );	
		gCPUState.FPU[reg+0]._u32_0 = r._u32_0;
		gCPUState.FPU[reg+1]._u32_0 = r._u32_1;
	}
	else
	{
		gCPUState.FPU[reg+0]._u32_0 = SIMULATESIG;
		gCPUState.FPU[reg+1]._f32_0 = f32( value );
	}
}

//*****************************************************************************
//
//*****************************************************************************
// This might seen rebundant, but we have to include it otherwise buck bumble will not work with simulate doubles
// The reasons : won't work with float and needs double type for value << really important !
// Check R4300_Cop1_D_ADD for reference.
//
// ToDO : Simplify this or find a workaround for this hack..
void StoreFPR_Double_2( u32 reg, f64 value )
{
	REG64	r;
	r._f64 = f64( value );	// double.. float won't work for buck bumble

	// TODO - Don't think these need sign extending...
	gCPUState.FPU[reg+0]._u32_0 = r._u32_0;
	gCPUState.FPU[reg+1]._u32_0 = r._u32_1;

}

//*****************************************************************************
//
//*****************************************************************************

__forceinline s32 LoadFPR_Word( u32 reg )
{
	return gCPUState.FPU[reg]._s32_0;
}

__forceinline void StoreFPR_Word( u32 reg, s32 value )
{
	gCPUState.FPU[reg]._s32_0 = value;
}

__forceinline f32 LoadFPR_Single( u32 reg )
{
	return gCPUState.FPU[reg]._f32_0;
}

__forceinline void StoreFPR_Single( u32 reg, f32 value )
{
	gCPUState.FPU[reg]._f32_0 = value;
}

//*****************************************************************************
//
//	int -> float conversion routines
//
//*****************************************************************************

__forceinline f32 s32_to_f32( s32 x )
{
	return (f32)x;
}

__forceinline d64 s32_to_d64( s32 x )
{
	return (d64)x;
}

__forceinline f32 s64_to_f32( s64 x )
{
	return (f32)x;
}

__forceinline d64 s64_to_d64( s64 x )
{
	return (d64)x;
}

//*****************************************************************************
//
//	float -> float conversion routines
//
//*****************************************************************************

__forceinline d64 f32_to_d64( f32 x )
{
	return (d64)x;
}

__forceinline f32 d64_to_f32( d64 x )
{
	return (f32)x;
}

//*****************************************************************************
//
//	Float -> int conversion routines
//
//*****************************************************************************
static const PspFpuRoundMode		gNativeRoundingModes[ RM_NUM_MODES ] =
{
	PSP_FPU_RN,	// RM_ROUND,
	PSP_FPU_RZ,	// RM_TRUNC,
	PSP_FPU_RP,	// RM_CEIL,
	PSP_FPU_RM,	// RM_FLOOR,
};

inline void SET_ROUND_MODE( ERoundingMode mode )
{
	// I don't think anything is required here
}

inline s32 f32_to_s32_cvt( f32 x )					{ s32 r; asm volatile ( "cvt.w.s %1, %1\nmfc1 %0,%1\n" : "=r"(r) : "f"(x) ); return r; }
inline s32 f32_to_s32_trunc( f32 x )				{ s32 r; asm volatile ( "trunc.w.s %1, %1\nmfc1 %0,%1\n" : "=r"(r) : "f"(x) ); return r; }
inline s32 f32_to_s32_round( f32 x )				{ s32 r; asm volatile ( "round.w.s %1, %1\nmfc1 %0,%1\n" : "=r"(r) : "f"(x) ); return r; }
inline s32 f32_to_s32_ceil( f32 x )					{ s32 r; asm volatile ( "ceil.w.s  %1, %1\nmfc1 %0,%1\n" : "=r"(r) : "f"(x) ); return r; }
inline s32 f32_to_s32_floor( f32 x )				{ s32 r; asm volatile ( "floor.w.s %1, %1\nmfc1 %0,%1\n" : "=r"(r) : "f"(x) ); return r; }
inline s32 f32_to_s32( f32 x, ERoundingMode mode )	{ pspFpuSetRoundmode( gNativeRoundingModes[ mode ] ); return f32_to_s32_cvt( x ); }

inline s64 f32_to_s64_trunc( f32 x )				{ return (s64)x; }
inline s64 f32_to_s64_round( f32 x )				{ return (s64)( x + 0.5f ); }
inline s64 f32_to_s64_ceil( f32 x )					{ return (s64)ceilf( x ); }
inline s64 f32_to_s64_floor( f32 x )				{ return (s64)floorf( x ); }
inline s64 f32_to_s64( f32 x, ERoundingMode mode )	{ pspFpuSetRoundmode( gNativeRoundingModes[ mode ] ); return (s64)x; }	// XXXX Need to do a cvt really

inline s32 d64_to_s32_cvt( d64 x )					{ return f32_to_s32_cvt( (f32)x ); }
inline s32 d64_to_s32_trunc( d64 x )				{ return f32_to_s32_trunc( (f32)x ); }
inline s32 d64_to_s32_round( d64 x )				{ return f32_to_s32_round( (f32)x ); }
inline s32 d64_to_s32_ceil( d64 x )					{ return f32_to_s32_ceil( (f32)x ); }
inline s32 d64_to_s32_floor( d64 x )				{ return f32_to_s32_floor( (f32)x ); }
inline s32 d64_to_s32( d64 x, ERoundingMode mode )	{ pspFpuSetRoundmode( gNativeRoundingModes[ mode ] ); return d64_to_s32_cvt( (f32)x ); }

inline s64 d64_to_s64_trunc( d64 x )				{ return (s64)x; }
inline s64 d64_to_s64_round( d64 x )				{ return (s64)( x + 0.5f ); }
inline s64 d64_to_s64_ceil( d64 x )					{ return (s64)ceilf( x ); }
inline s64 d64_to_s64_floor( d64 x )				{ return (s64)floorf( x ); }
inline s64 d64_to_s64( d64 x, ERoundingMode mode )	{ pspFpuSetRoundmode( gNativeRoundingModes[ mode ] ); return (s64)x; }	// XXXX Need to do a cvt really


//static void CU1_R4300_CoPro1( R4300_CALL_SIGNATURE );
//static void R4300_CALL_TYPE CU1_R4300_LWC1( R4300_CALL_SIGNATURE );
//static void R4300_CALL_TYPE CU1_R4300_LDC1( R4300_CALL_SIGNATURE );
//static void R4300_CALL_TYPE CU1_R4300_SWC1( R4300_CALL_SIGNATURE );
//static void R4300_CALL_TYPE CU1_R4300_SDC1( R4300_CALL_SIGNATURE );

static void R4300_CALL_TYPE R4300_Cop1_BCInstr( R4300_CALL_SIGNATURE );
static void R4300_CALL_TYPE R4300_Cop1_SInstr( R4300_CALL_SIGNATURE );
static void R4300_CALL_TYPE R4300_Cop1_DInstr( R4300_CALL_SIGNATURE );
static void R4300_CALL_TYPE R4300_Cop1_WInstr( R4300_CALL_SIGNATURE );
static void R4300_CALL_TYPE R4300_Cop1_LInstr( R4300_CALL_SIGNATURE );
static void R4300_CALL_TYPE R4300_CoPro0( R4300_CALL_SIGNATURE );
static void R4300_CALL_TYPE R4300_CoPro1( R4300_CALL_SIGNATURE );
//static void R4300_CALL_TYPE R4300_CoPro1_Disabled( R4300_CALL_SIGNATURE );
static void R4300_CALL_TYPE R4300_Special( R4300_CALL_SIGNATURE );
static void R4300_CALL_TYPE R4300_RegImm( R4300_CALL_SIGNATURE );
static void R4300_CALL_TYPE R4300_Cop0_TLB( R4300_CALL_SIGNATURE );

static void R4300_CALL_TYPE R4300_LWC1( R4300_CALL_SIGNATURE );
static void R4300_CALL_TYPE R4300_LDC1( R4300_CALL_SIGNATURE );
static void R4300_CALL_TYPE R4300_SWC1( R4300_CALL_SIGNATURE );
static void R4300_CALL_TYPE R4300_SDC1( R4300_CALL_SIGNATURE );


//*****************************************************************************
//	Returns true if the specified opcode handler needs a valid entry in
//	gCPUState.CurrentPC to function correctly.
//	The PC must be set up for all branching instructions, but also
//	for any instruction that can potentially throw an exception (as it needs
//	to keep track of the location of the exception)
//	We could possibly get around this by explicitly setting the ErrorPC
//	AFTER the exception has been thrown. This might be a bit fiddly so
//	this function provides a conservative result for now.
//*****************************************************************************
bool	R4300_InstructionHandlerNeedsPC( OpCode op_code )
{
	switch( op_code.op )
	{

	case OP_ADDI:
	case OP_ADDIU:
	case OP_SLTI:
	case OP_SLTIU:
	case OP_ANDI:
	case OP_ORI:
	case OP_XORI:
	case OP_LUI:
	case OP_DADDI:
	case OP_DADDIU:
	case OP_CACHE:
		return false;

	case OP_SPECOP:
		//return R4300SpecialInstruction[ op_code.funct ];

		switch( op_code.spec_op )
		{
		case SpecOp_SLL:
		case SpecOp_SRL:
		case SpecOp_SRA:
		case SpecOp_SLLV:
		case SpecOp_SRLV:
		case SpecOp_SRAV:
		case SpecOp_MFHI:
		case SpecOp_MTHI:
		case SpecOp_MFLO:
		case SpecOp_MTLO:
		case SpecOp_DSLLV:
		case SpecOp_DSRLV:
		case SpecOp_DSRAV:
		case SpecOp_MULT:
		case SpecOp_MULTU:
		case SpecOp_DIV:		// Need to remove this if we can throw exception on divide by 0
		case SpecOp_DIVU:		// Ditto
		case SpecOp_DMULT:
		case SpecOp_DMULTU:
		case SpecOp_DDIV:		// Ditto
		case SpecOp_DDIVU:		// Ditto
		case SpecOp_ADD:		// Potentially can throw
		case SpecOp_ADDU:
		case SpecOp_SUB:		// Potentially can throw
		case SpecOp_SUBU:
		case SpecOp_AND:
		case SpecOp_OR:
		case SpecOp_XOR:
		case SpecOp_NOR:
		case SpecOp_SLT:
		case SpecOp_SLTU:
		case SpecOp_DADD:		// Potentially can throw
		case SpecOp_DADDU:
		case SpecOp_DSUB:		// Potentially can throw
		case SpecOp_DSUBU:
		case SpecOp_DSLL:
		case SpecOp_DSRL:
		case SpecOp_DSRA:
		case SpecOp_DSLL32:
		case SpecOp_DSRL32:
		case SpecOp_DSRA32:
			return false;
		default:
			break;
		}
		return true;

	case OP_REGIMM:
		// These are all traps or branches
		return true;

	case OP_COPRO0:
		// Possibly could return false for some of these
		return true;

	case OP_COPRO1:
		// Potentially these can all throw, if cop1 is disabled
		// We explicitly handle this in the dynarec (we check the usuable flag once
		// per fragment). Care needs to be take if this is used elsewhere.
		return false;

	default:
		//
		return true;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void R4300_CALL_TYPE R4300_SetSR( u32 new_value )
{
#ifndef DAEDALUS_SILENT
	if((gCPUState.CPUControl[C0_SR]._u32_0 & SR_FR) != (new_value & SR_FR))
	{
		DBGConsole_Msg(0, "[MChanging FP to %s, STATUS=%08X", (new_value & SR_FR) ? "64bit" : "32bit", (new_value & SR_FR));
	}
	/*
	if((gCPUState.CPUControl[C0_SR]._u32_0 & SR_UX) != (new_value & SR_UX))
	{
		DBGConsole_Msg(0, "[MChanging CPU to %s, STATUS=%08X", (new_value & SR_UX) ? "64bit" : "32bit", (new_value & SR_UX));
	}
	*/
#endif

	bool interrupts_enabled_before =(gCPUState.CPUControl[C0_SR]._u32_0 & SR_IE) != 0;

	gCPUState.CPUControl[C0_SR]._u32_0 = new_value;

	bool interrupts_enabled_after = (gCPUState.CPUControl[C0_SR]._u32_0 & SR_IE) != 0;

	/*
-	Really important note :
-	Disabling this flag fixes some games ex : Extreme-G XG2 and Tom and Jerry.
-	Also fixes Wayne Gretzky's 3D Hockey '98 to be zoomed in too far (really nasty glitch)
-	Still needs a bit of work, but so far we are on the right path now.
	*/

	// Too hackish.. disabling this check causes several games ex SSB to go crazy..
	// Also you need to restart the emu to restore the interrupts correctly
	/*if(!gCheckN64FPUsageDisable)	// Check FP usage
	{
		R4300_SetCop1Enable( (gCPUState.CPUControl[C0_SR]._u64 & SR_CU1) != 0 );
	}*/

	if ( !interrupts_enabled_before && interrupts_enabled_after )
	{
		if (gCPUState.CPUControl[C0_SR]._u32_0 & gCPUState.CPUControl[C0_CAUSE]._u32_0 & CAUSE_IPMASK)
		{
			gCPUState.AddJob( CPU_CHECK_INTERRUPTS );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

#define WARN_NOEXIST(inf)	{ DAEDALUS_ASSERT( false, "Instruction Unknown" ); }
#define WARN_NOIMPL(op)		{ DAEDALUS_ASSERT( false, "Instruction Not Implemented" ); }

static void R4300_CALL_TYPE R4300_Unk( R4300_CALL_SIGNATURE )     { WARN_NOEXIST("R4300_Unk"); }
/*
static void R4300_CALL_TYPE R4300_CoPro1_Disabled( R4300_CALL_SIGNATURE )
{
	// Cop1 Unusable
	DBGConsole_Msg(0, "Thread accessing Cop1, throwing COP1 unusuable exception");

	DAEDALUS_ASSERT( (gCPUState.CPUControl[C0_SR]._u64 & SR_CU1) == 0, "COP1 usable flag in inconsistant state!" );

	R4300_Exception_CopUnusuable();
}
*/
// These are the only unimplemented R4300 instructions now:
static void R4300_CALL_TYPE R4300_LL( R4300_CALL_SIGNATURE ) { WARN_NOIMPL("LL"); }
static void R4300_CALL_TYPE R4300_LLD( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("LLD"); }

static void R4300_CALL_TYPE R4300_SC( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("SC"); }
static void R4300_CALL_TYPE R4300_SCD( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("SCD"); }


static void R4300_CALL_TYPE R4300_LDC2( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("LDC2"); }
static void R4300_CALL_TYPE R4300_SDC2( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("SDC2"); }

static void R4300_CALL_TYPE R4300_RegImm_TGEI( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("TGEI"); }
static void R4300_CALL_TYPE R4300_RegImm_TGEIU( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("TGEIU"); }
static void R4300_CALL_TYPE R4300_RegImm_TLTI( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("TLTI"); }
static void R4300_CALL_TYPE R4300_RegImm_TLTIU( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("TLTIU"); }
static void R4300_CALL_TYPE R4300_RegImm_TEQI( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("TEQI"); }
static void R4300_CALL_TYPE R4300_RegImm_TNEI( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("TNEI"); }

static void R4300_CALL_TYPE R4300_RegImm_BLTZALL( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("BLTZALL"); }
static void R4300_CALL_TYPE R4300_RegImm_BGEZALL( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("BGEZALL"); }

static void R4300_CALL_TYPE R4300_Special_TGE( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("TGE"); }
static void R4300_CALL_TYPE R4300_Special_TGEU( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("TGEU"); }
static void R4300_CALL_TYPE R4300_Special_TLT( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("TLT"); }
static void R4300_CALL_TYPE R4300_Special_TLTU( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("TLTU"); }
static void R4300_CALL_TYPE R4300_Special_TEQ( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("TEQ"); }
static void R4300_CALL_TYPE R4300_Special_TNE( R4300_CALL_SIGNATURE ) {  WARN_NOIMPL("TNE"); }


static void R4300_CALL_TYPE R4300_DBG_Bkpt( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

#ifdef DAEDALUS_BREAKPOINTS_ENABLED
	// Entry is in lower 26 bits...
	u32 dwBreakPoint = op_code.bp_index;

	if (g_BreakPoints[dwBreakPoint].mEnabled &&
		!g_BreakPoints[dwBreakPoint].mTemporaryDisable)
	{
		// Set the temporary disable so we don't execute bp immediately again
		g_BreakPoints[dwBreakPoint].mTemporaryDisable = true;
		CPU_Halt("BreakPoint");
		DBGConsole_Msg(0, "[RBreakPoint at 0x%08x]", gCPUState.CurrentPC);

		// Decrement, so we move onto this instruction next
		DECREMENT_PC();
	}
	else
	{
		// If this was on, disable it
		g_BreakPoints[dwBreakPoint].mTemporaryDisable = false;

		OpCode	original_op( g_BreakPoints[dwBreakPoint].mOriginalOp );

		R4300Instruction[ original_op.op ]( original_op._u32 );
	}
#else

	DAEDALUS_ERROR( "How did we get here when breakpoints are disabled?" );

#endif

}

static void R4300_CALL_TYPE R4300_J( R4300_CALL_SIGNATURE ) 				// Jump
{
	R4300_CALL_MAKE_OP( op_code );

	u32 new_pc( (gCPUState.CurrentPC & 0xF0000000) | (op_code.target<<2) );

	// Doesn't do anything.. should be gCPUState.CurrentPC == gCPUState.TargetPC, but breaks PMario for some reasons
	/*if( new_pc == gCPUState.CurrentPC )
	{
		SpeedHack( gCPUState.CurrentPC, op_code );
	}*/
	CPU_TakeBranch( new_pc );
}

static void R4300_CALL_TYPE R4300_JAL( R4300_CALL_SIGNATURE ) 				// Jump And Link
{
	R4300_CALL_MAKE_OP( op_code );

	// Store return address
	gGPR[REG_ra]._s64 = (s64)(s32)(gCPUState.CurrentPC + 8);		// Store return address
	u32	new_pc( (gCPUState.CurrentPC & 0xF0000000) | (op_code.target<<2) );

	CPU_TakeBranch( new_pc );
}

static void R4300_CALL_TYPE R4300_BEQ( R4300_CALL_SIGNATURE ) 		// Branch on Equal
{
	R4300_CALL_MAKE_OP( op_code );

	DAEDALUS_ASSERT( (gGPR[op_code.rs]._u32_0 == gGPR[op_code.rt]._u32_0) == (gGPR[op_code.rs]._u64 == gGPR[op_code.rt]._u64), "Branching assumption invalid" )

	//branch if rs == rt
	if ( gGPR[op_code.rs]._u32_0 == gGPR[op_code.rt]._u32_0 )
	{
		s16 offset = (s16)op_code.immediate;

		if( offset == -1 )
		{
			SpeedHack( gCPUState.CurrentPC, op_code );
		}

		u32 new_pc( gCPUState.CurrentPC + ((s32)offset<<2) + 4 );
		CPU_TakeBranch( new_pc );
	}
}

static void R4300_CALL_TYPE R4300_BNE( R4300_CALL_SIGNATURE )             // Branch on Not Equal
{
	R4300_CALL_MAKE_OP( op_code );

	DAEDALUS_ASSERT( (gGPR[op_code.rs]._u32_0 != gGPR[op_code.rt]._u32_0) == (gGPR[op_code.rs]._u64 != gGPR[op_code.rt]._u64), "Branching assumption invalid" )

	//branch if rs <> rt
	if ( gGPR[op_code.rs]._u32_0 != gGPR[op_code.rt]._u32_0 )
	{
		s16 offset = (s16)op_code.immediate;

		if( offset == -1 )
		{
			SpeedHack( gCPUState.CurrentPC, op_code );
		}

		u32	new_pc( gCPUState.CurrentPC + ((s32)offset<<2) + 4 );
		CPU_TakeBranch( new_pc );
	}
}

static void R4300_CALL_TYPE R4300_BLEZ( R4300_CALL_SIGNATURE ) 			// Branch on Less than of Equal to Zero
{
	R4300_CALL_MAKE_OP( op_code );

	DAEDALUS_ASSERT( (gGPR[op_code.rs]._s32_0 <= 0) == (gGPR[op_code.rs]._s64 <= 0), "Branching assumption invalid" )

	//branch if rs <= 0
	//if ((s64)gGPR[op_code.rs] <= 0)
	if (gGPR[op_code.rs]._s32_0 <= 0)
	{
		s16 offset = (s16)op_code.immediate;

		if( offset == -1 )
		{
			SpeedHack( gCPUState.CurrentPC, op_code );
		}

		u32	new_pc( gCPUState.CurrentPC + ((s32)offset<<2) + 4 );
		CPU_TakeBranch( new_pc );
	}
}

static void R4300_CALL_TYPE R4300_BGTZ( R4300_CALL_SIGNATURE ) 			// Branch on Greater than Zero
{
	R4300_CALL_MAKE_OP( op_code );

	DAEDALUS_ASSERT( (gGPR[op_code.rs]._s32_0 > 0) == (gGPR[op_code.rs]._s64 > 0), "Branching assumption invalid" )

	//branch if rs > 0
	//if ((s64)gGPR[op_code.rs] > 0)
	if (gGPR[op_code.rs]._s32_0 > 0)
	{
		s16 offset = (s16)op_code.immediate;

		if( offset == -1 )
		{
			SpeedHack( gCPUState.CurrentPC, op_code );
		}

		u32 new_pc( gCPUState.CurrentPC + ((s32)offset<<2) + 4 );
		CPU_TakeBranch( new_pc );
	}
}


static void R4300_CALL_TYPE R4300_DADDI( R4300_CALL_SIGNATURE ) 			// Doubleword ADD Immediate
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	// Check for overflow
	// Reserved Instruction exception

	//rt = rs + immediate
	gGPR[op_code.rt]._s64 = gGPR[op_code.rs]._s64 + (s32)(s16)op_code.immediate;
}

static void R4300_CALL_TYPE R4300_DADDIU( R4300_CALL_SIGNATURE ) 			// Doubleword ADD Immediate Unsigned
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	// Reserved Instruction exception

	//rt = rs + immediate
	gGPR[op_code.rt]._s64 = gGPR[op_code.rs]._s64 + (s32)(s16)op_code.immediate;

}

static void R4300_CALL_TYPE R4300_ADDI( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	// Generates overflow exception

	//rt = rs + immediate
	gGPR[op_code.rt]._s64 = (s64)(s32)(gGPR[op_code.rs]._s32_0 + (s32)(s16)op_code.immediate);
}

static void R4300_CALL_TYPE R4300_ADDIU( R4300_CALL_SIGNATURE ) 		// Add Immediate Unsigned
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	//rt = rs + immediate
	gGPR[op_code.rt]._s64 = (s64)(s32)(gGPR[op_code.rs]._s32_0 + (s32)(s16)op_code.immediate);
}

static void R4300_CALL_TYPE R4300_SLTI( R4300_CALL_SIGNATURE ) 			// Set on Less Than Immediate
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	// Cast to s32s to ensure sign is taken into account
	if (gGPR[op_code.rs]._s64 < (s64)(s32)(s16)op_code.immediate)
	{
		gGPR[op_code.rt]._u64 = 1;
	}
	else
	{
#if 1 //1->default, 0->"hack" that speeds up SM64 when sound is off (just for reference) //Salvy
		gGPR[op_code.rt]._u64 = 0;
#else
		gGPR[op_code.rt]._u32_0 = 0;
#endif
	}
}

static void R4300_CALL_TYPE R4300_SLTIU( R4300_CALL_SIGNATURE ) 		// Set on Less Than Immediate Unsigned
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	// Cast to s32s to ensure sign is taken into account
	if (gGPR[op_code.rs]._u64 < (u64)(s64)(s32)(s16)op_code.immediate)
	{
		gGPR[op_code.rt]._u64 = 1;
	}
	else
	{
		gGPR[op_code.rt]._u64 = 0;
	}
}


static void R4300_CALL_TYPE R4300_ANDI( R4300_CALL_SIGNATURE ) 				// AND Immediate
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	//rt = rs & immediate
	gGPR[op_code.rt]._u64 = gGPR[op_code.rs]._u64 & (u64)(u16)op_code.immediate;
}


static void R4300_CALL_TYPE R4300_ORI( R4300_CALL_SIGNATURE ) 				// OR Immediate
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	//rt = rs | immediate
	gGPR[op_code.rt]._u64 = gGPR[op_code.rs]._u64 | (u64)(u16)op_code.immediate;
}

static void R4300_CALL_TYPE R4300_XORI( R4300_CALL_SIGNATURE ) 				// XOR Immediate
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	//rt = rs ^ immediate
	gGPR[op_code.rt]._u64 = gGPR[op_code.rs]._u64 ^ (u64)(u16)op_code.immediate;
}

static void R4300_CALL_TYPE R4300_LUI( R4300_CALL_SIGNATURE ) 				// Load Upper Immediate
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	gGPR[op_code.rt]._s64 = (s64)(s32)((s32)(s16)op_code.immediate<<16);
}

static void R4300_CALL_TYPE R4300_BEQL( R4300_CALL_SIGNATURE ) 			// Branch on Equal Likely
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if rs == rt
	if ( gGPR[op_code.rs]._u64 == gGPR[op_code.rt]._u64 )
	{
		s16 offset = (s16)op_code.immediate;

		if( offset == -1 )
		{
			SpeedHack( gCPUState.CurrentPC, op_code );
		}

		u32	new_pc( gCPUState.CurrentPC + ((s32)offset<<2) + 4 );
		CPU_TakeBranch( new_pc );
	}
	else
	{
		// Don't execute subsequent instruction
		INCREMENT_PC();
	}
}

static void R4300_CALL_TYPE R4300_BNEL( R4300_CALL_SIGNATURE ) 			// Branch on Not Equal Likely
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if rs <> rt
	if ( gGPR[op_code.rs]._u64 != gGPR[op_code.rt]._u64 )
	{
		s16 offset = (s16)op_code.immediate;

		if( offset == -1 )
		{
			SpeedHack( gCPUState.CurrentPC, op_code );
		}

		u32	new_pc( gCPUState.CurrentPC + ((s32)offset<<2) + 4 );
		CPU_TakeBranch( new_pc );
	}
	else
	{
		// Don't execute subsequent instruction
		INCREMENT_PC();
	}
}

static void R4300_CALL_TYPE R4300_BLEZL( R4300_CALL_SIGNATURE ) 		// Branch on Less than or Equal to Zero Likely
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if rs <= 0
	if ( gGPR[op_code.rs]._s64 <= 0 )
	{
		if (op_code.rs == N64Reg_R0)
		{
			SpeedHack( gCPUState.CurrentPC, op_code );
		}
		u32	new_pc( gCPUState.CurrentPC + ((s32)(s16)op_code.immediate<<2) + 4 );
		CPU_TakeBranch( new_pc );
	}
	else
	{
		// Don't execute subsequent instruction
		INCREMENT_PC();
	}
}

static void R4300_CALL_TYPE R4300_BGTZL( R4300_CALL_SIGNATURE ) 		// Branch on Greater than Zero Likely
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if rs > 0
	if ( gGPR[op_code.rs]._s64 > 0 )
	{
		u32	new_pc( gCPUState.CurrentPC + ((s32)(s16)op_code.immediate<<2) + 4 );
		CPU_TakeBranch( new_pc );
	}
	else
	{
		// Don't execute subsequent instruction
		INCREMENT_PC();
	}
}

static void R4300_CALL_TYPE R4300_LB( R4300_CALL_SIGNATURE ) 			// Load Byte
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );

	gGPR[op_code.rt]._s64 = (s64)(s8)Read8Bits(address);
}

static void R4300_CALL_TYPE R4300_LBU( R4300_CALL_SIGNATURE ) 			// Load Byte Unsigned -- Zero extend byte...
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate);

	gGPR[op_code.rt]._s64 = (s64)(u8)Read8Bits(address);
}

static void R4300_CALL_TYPE R4300_LH( R4300_CALL_SIGNATURE ) 		// Load Halfword
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );
	gGPR[op_code.rt]._s64 = (s64)(s16)Read16Bits(address);
}

static void R4300_CALL_TYPE R4300_LHU( R4300_CALL_SIGNATURE )			// Load Halfword Unsigned -- Zero extend word
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );

	gGPR[op_code.rt]._u64 = (u64)(u16)Read16Bits(address);
}


static void R4300_CALL_TYPE R4300_LWL( R4300_CALL_SIGNATURE ) 			// Load Word Left
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );
	u32 nMemory = Read32Bits(address & ~0x3);

	u32 nReg = gGPR[op_code.rt]._u32_0;

#if 1 //1-> tighter code, 0->old way //Corn	
	nReg = (nReg & ~(~0 << ((address & 0x3) << 3))) | (nMemory << ((address & 0x3) << 3));
#else
	switch (address % 4)
	{
        case 0: nReg = nMemory; break;
        case 1: nReg = ((nReg & 0x000000FF) | (nMemory << 8));  break;
        case 2: nReg = ((nReg & 0x0000FFFF) | (nMemory << 16)); break;
        case 3: nReg = ((nReg & 0x00FFFFFF) | (nMemory << 24)); break;
    }
#endif

	gGPR[op_code.rt]._s64 = (s64)(s32)nReg;
}

// Starcraft - not tested!
static void R4300_CALL_TYPE R4300_LDL( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );
	u64 nMemory = Read64Bits(address & ~0x7);

	u64 nReg = gGPR[op_code.rt]._u64;

#if 1 //1-> tighter code, 0->old way //Corn	
	nReg = (nReg & ~(~0LL << ((address & 0x7) << 3))) | (nMemory << ((address & 0x7) << 3));
#else
	switch (address % 8)
	{
        case 0: nReg = nMemory; break;
        case 1: nReg = ((nReg & 0x00000000000000FFLL) | (nMemory << 8));  break;
        case 2: nReg = ((nReg & 0x000000000000FFFFLL) | (nMemory << 16)); break;
        case 3: nReg = ((nReg & 0x0000000000FFFFFFLL) | (nMemory << 24)); break;
        case 4: nReg = ((nReg & 0x00000000FFFFFFFFLL) | (nMemory << 32)); break;
        case 5: nReg = ((nReg & 0x000000FFFFFFFFFFLL) | (nMemory << 40)); break;
        case 6: nReg = ((nReg & 0x0000FFFFFFFFFFFFLL) | (nMemory << 48)); break;
        case 7: nReg = ((nReg & 0x00FFFFFFFFFFFFFFLL) | (nMemory << 56)); break;
   }
#endif

	gGPR[op_code.rt]._u64 = nReg;
}


static void R4300_CALL_TYPE R4300_LWR( R4300_CALL_SIGNATURE ) 			// Load Word Right
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );
	u32 nMemory = Read32Bits(address & ~0x3);

	u32 nReg = gGPR[op_code.rt]._u32_0;

#if 1 //1-> tighter code, 0->old way //Corn	
	nReg = (nReg & (~0 << ( ((address & 0x3) + 1) << 3))) | (nMemory >> ((~address & 0x3) << 3));
#else
	switch (address % 4)
	{
        case 0: nReg = (nReg & 0xFFFFFF00) | (nMemory >> 24); break;
        case 1: nReg = (nReg & 0xFFFF0000) | (nMemory >> 16); break;
        case 2: nReg = (nReg & 0xFF000000) | (nMemory >>  8); break;
        case 3: nReg = nMemory; break;
    }
#endif

	gGPR[op_code.rt]._s64 = (s64)(s32)nReg;
}

// Starcraft - not tested!
static void R4300_CALL_TYPE R4300_LDR( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );
	u64 nMemory = Read64Bits(address & ~0x7);

	u64 nReg = gGPR[op_code.rt]._u64;

#if 1 //1-> tighter code, 0->old way //Corn	
	nReg = (nReg & (~0LL << ( ((address & 0x7) + 1) << 3))) | (nMemory >> ((~address & 0x7) << 3));
#else
	switch (address % 8)
	{
        case 0: nReg = (nReg & 0xFFFFFFFFFFFFFF00LL) | (nMemory >> 56); break;
        case 1: nReg = (nReg & 0xFFFFFFFFFFFF0000LL) | (nMemory >> 48); break;
        case 2: nReg = (nReg & 0xFFFFFFFFFF000000LL) | (nMemory >> 40); break;
        case 3: nReg = (nReg & 0xFFFFFFFF00000000LL) | (nMemory >> 32); break;
        case 4: nReg = (nReg & 0xFFFFFF0000000000LL) | (nMemory >> 24); break;
        case 5: nReg = (nReg & 0xFFFF000000000000LL) | (nMemory >> 16); break;
        case 6: nReg = (nReg & 0xFF00000000000000LL) | (nMemory >>  8); break;
        case 7: nReg = nMemory; break;
    }
#endif

	gGPR[op_code.rt]._u64 = nReg;
}


static void R4300_CALL_TYPE R4300_LW( R4300_CALL_SIGNATURE ) 			// Load Word
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	// This is for San Francisco 2049. An R0 errg.. otherwise it crashes when the race is about to start.
	if (op_code.rt == 0) 
	{
		DAEDALUS_ERROR("Attempted write to r0!");
		return;	// I think is better to trap it than override it
	}

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );
	gGPR[op_code.rt]._s64 = (s64)(s32)Read32Bits(address);
}

static void R4300_CALL_TYPE R4300_LWU( R4300_CALL_SIGNATURE ) 			// Load Word Unsigned
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );
	gGPR[op_code.rt]._u64 = (u64)(u32)Read32Bits(address);
}

static void R4300_CALL_TYPE R4300_SW( R4300_CALL_SIGNATURE ) 			// Store Word
{
	R4300_CALL_MAKE_OP( op_code );

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );

	Write32Bits(address, gGPR[op_code.rt]._u32_0);
}

static void R4300_CALL_TYPE R4300_SH( R4300_CALL_SIGNATURE ) 			// Store Halfword
{
	R4300_CALL_MAKE_OP( op_code );

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );
	Write16Bits(address, (u16)(gGPR[op_code.rt]._u32_0 & 0xffff));
}

static void R4300_CALL_TYPE R4300_SB( R4300_CALL_SIGNATURE ) 			// Store Byte
{
	R4300_CALL_MAKE_OP( op_code );

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );
	Write8Bits(address, (u8)(gGPR[op_code.rt]._u32_0 & 0xff));
}

static void R4300_CALL_TYPE R4300_SWL( R4300_CALL_SIGNATURE ) 			// Store Word Left
{
	R4300_CALL_MAKE_OP( op_code );

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );

	u32 dwMemory = Read32Bits(address & ~0x3);
	u32 dwReg = gGPR[op_code.rt]._u32_0;

#if 1 //1-> tighter code, 0->old way //Corn	
	u32 dwNew = (dwMemory & ((~0 << (((~address & 0x3) + 1 ) << 3)))) | (dwReg >> ((address & 0x3) << 3));
#else
	u32 dwNew;
	switch (address % 4)
	{
	case 0:	dwNew = dwReg; break;			// Aligned
	case 1:	dwNew = (dwMemory & 0xFF000000) | (dwReg >> 8 ); break;
	case 2:	dwNew = (dwMemory & 0xFFFF0000) | (dwReg >> 16); break;
	default:dwNew = (dwMemory & 0xFFFFFF00) | (dwReg >> 24); break;
	}
#endif

	Write32Bits(address & ~0x3, dwNew);
}

static void R4300_CALL_TYPE R4300_SWR( R4300_CALL_SIGNATURE ) 			// Store Word Right
{
	R4300_CALL_MAKE_OP( op_code );

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );

	u32 dwMemory = Read32Bits(address & ~0x3);
	u32 dwReg = gGPR[op_code.rt]._u32_0;

#if 1 //1-> tighter code, 0->old way //Corn	
	u32 dwNew = (dwMemory & ~(~0 << ((~address & 0x3) << 3))) | (dwReg << ((~address & 0x3) << 3));
#else
	u32 dwNew;
	switch (address % 4)
	{
	case 0:	dwNew = (dwMemory & 0x00FFFFFF) | (dwReg << 24); break;
	case 1:	dwNew = (dwMemory & 0x0000FFFF) | (dwReg << 16); break;
	case 2:	dwNew = (dwMemory & 0x000000FF) | (dwReg << 8); break;
	default:dwNew = dwReg; break;			// Aligned
	}
#endif

	Write32Bits(address & ~0x3, dwNew);

}

static void R4300_CALL_TYPE R4300_SDL( R4300_CALL_SIGNATURE )//CYRUS64
{
	R4300_CALL_MAKE_OP( op_code );

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate);

	u64 nMemory = Read64Bits(address & ~0x7);
	u64 nReg = gGPR[op_code.rt]._u64;

#if 1 //1-> tighter code, 0->old way //Corn	
	u64 nNew = (nMemory & ((~0LL << (((~address & 0x7) + 1 ) << 3)))) | (nReg >> ( (address & 0x7) << 3));
#else
	u64 nNew;
	switch (address % 8)
	{
	case 0:	nNew = nReg; break;			// Aligned
	case 1:	nNew = (nMemory & 0xFF00000000000000LL) | (nReg >> 8); break;
	case 2:	nNew = (nMemory & 0xFFFF000000000000LL) | (nReg >> 16); break;
	case 3:	nNew = (nMemory & 0xFFFFFF0000000000LL) | (nReg >> 24); break;
	case 4:	nNew = (nMemory & 0xFFFFFFFF00000000LL) | (nReg >> 32); break;
	case 5:	nNew = (nMemory & 0xFFFFFFFFFF000000LL) | (nReg >> 40); break;
	case 6:	nNew = (nMemory & 0xFFFFFFFFFFFF0000LL) | (nReg >> 48); break;
	default:nNew = (nMemory & 0xFFFFFFFFFFFFFF00LL) | (nReg >> 56); break;
	}
#endif

	Write64Bits(address & ~0x7, nNew);
}

static void R4300_CALL_TYPE R4300_SDR( R4300_CALL_SIGNATURE )//CYRUS64
{
	R4300_CALL_MAKE_OP( op_code );

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate);
	//DBGConsole_Msg(0,"Address:		0x%08x", dwAddress );

	u64 nMemory = Read64Bits(address & ~0x7);
	u64 nReg = gGPR[op_code.rt]._u64;
#if 1 //1-> tighter code, 0->old way //Corn	
	u64 nNew = (nMemory & ~(~0LL << ((~address & 0x7) << 3))) | (nReg << ((~address & 0x7) << 3));
#else
	u64 nNew;
	switch (address % 8)
	{
	case 0:	nNew = (nMemory & 0x00FFFFFFFFFFFFFFLL) | (nReg << 56); break;
	case 1:	nNew = (nMemory & 0x0000FFFFFFFFFFFFLL) | (nReg << 48); break;
	case 2:	nNew = (nMemory & 0x000000FFFFFFFFFFLL) | (nReg << 40); break;
	case 3:	nNew = (nMemory & 0x00000000FFFFFFFFLL) | (nReg << 32); break;
	case 4:	nNew = (nMemory & 0x0000000000FFFFFFLL) | (nReg << 24); break;
	case 5:	nNew = (nMemory & 0x000000000000FFFFLL) | (nReg << 16); break;
	case 6:	nNew = (nMemory & 0x00000000000000FFLL) | (nReg << 8); break;
	default:nNew = nReg; break;			// Aligned
	}
#endif

	Write64Bits(address & ~0x7, nNew);
}

/*
static const char * const gCacheNames[] =
{
	"I", "D", "SI", "SD"
};
*/
static void R4300_CALL_TYPE R4300_CACHE( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

//	return;

#ifdef DAEDALUS_ENABLE_DYNAREC
	u32 cache_op  = op_code.rt;

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );

	// Do Nothing
	u32 dwCache = cache_op & 0x3;
	u32 dwAction = (cache_op >> 2) & 0x7;

	if(dwCache == 0 && (dwAction == 0 || dwAction == 4))
	{
		//DBGConsole_Msg( 0, "Cache invalidate - forcibly dumping dynarec contents" );
		CPU_InvalidateICacheRange(address, 0x20);
	}

	//DBGConsole_Msg(0, "CACHE %s/%d, 0x%08x", gCacheNames[dwCache], dwAction, address);
#endif
}

static void R4300_CALL_TYPE R4300_LWC1( R4300_CALL_SIGNATURE ) 				// Load Word to Copro 1 (FPU)
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_COP1_UNUSUABLE

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );
	StoreFPR_Word( op_code.ft, Read32Bits(address) );
}


static void R4300_CALL_TYPE R4300_LDC1( R4300_CALL_SIGNATURE )				// Load Doubleword to Copro 1 (FPU)
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_COP1_UNUSUABLE

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );
	StoreFPR_Long( op_code.ft, Read64Bits(address));
}


static void R4300_CALL_TYPE R4300_LD( R4300_CALL_SIGNATURE ) 				// Load Doubleword
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );
	gGPR[op_code.rt]._u64 = Read64Bits(address);
}


static void R4300_CALL_TYPE R4300_SWC1( R4300_CALL_SIGNATURE ) 			// Store Word From Copro 1
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_COP1_UNUSUABLE

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );
	//Write32Bits(address, (u32)gCPUState.FPU[dwFT]);
	Write32Bits(address, LoadFPR_Word(op_code.ft));
}

static void R4300_CALL_TYPE R4300_SDC1( R4300_CALL_SIGNATURE )		// Store Doubleword From Copro 1
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_COP1_UNUSUABLE

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );

	Write64Bits(address, LoadFPR_Long(op_code.ft));
}


static void R4300_CALL_TYPE R4300_SD( R4300_CALL_SIGNATURE )			// Store Doubleword
{
	R4300_CALL_MAKE_OP( op_code );

	u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );
	Write64Bits(address, gGPR[op_code.rt]._u64);
}



/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

static void R4300_CALL_TYPE R4300_Special_Unk( R4300_CALL_SIGNATURE ) { WARN_NOEXIST("R4300_Special_Unk"); }
static void R4300_CALL_TYPE R4300_Special_SLL( R4300_CALL_SIGNATURE ) 		// Shift word Left Logical
{
	R4300_CALL_MAKE_OP( op_code );

	// NOP!
	if ( op_code._u32 == 0 ) return;

	CHECK_R0( op_code.rd );

	gGPR[ op_code.rd ]._s64 = (s64)(s32)( (gGPR[ op_code.rt ]._u32_0 << op_code.sa) & 0xFFFFFFFF );
}

static void R4300_CALL_TYPE R4300_Special_SRL( R4300_CALL_SIGNATURE ) 		// Shift word Right Logical
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	gGPR[ op_code.rd ]._s64 = (s64)(s32)( gGPR[ op_code.rt ]._u32_0 >> op_code.sa );
}

static void R4300_CALL_TYPE R4300_Special_SRA( R4300_CALL_SIGNATURE ) 		// Shift word Right Arithmetic
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	gGPR[ op_code.rd ]._s64 = (s64)(s32)( gGPR[ op_code.rt ]._s32_0 >> op_code.sa );
}

static void R4300_CALL_TYPE R4300_Special_SLLV( R4300_CALL_SIGNATURE ) 		// Shift word Left Logical Variable
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	gGPR[ op_code.rd ]._s64 = (s64)(s32)( (gGPR[ op_code.rt ]._u32_0 << ( gGPR[ op_code.rs ]._u32_0 & 0x1F ) ) & 0xFFFFFFFF );
}

static void R4300_CALL_TYPE R4300_Special_SRLV( R4300_CALL_SIGNATURE ) 		// Shift word Right Logical Variable
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	gGPR[ op_code.rd ]._s64 = (s64)(s32)( gGPR[ op_code.rt ]._u32_0 >> ( gGPR[ op_code.rs ]._u32_0 & 0x1F ) );
}

static void R4300_CALL_TYPE R4300_Special_SRAV( R4300_CALL_SIGNATURE ) 		// Shift word Right Arithmetic Variable
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	gGPR[ op_code.rd ]._s64 = (s64)(s32)( gGPR[ op_code.rt ]._s32_0 >> ( gGPR[ op_code.rs ]._u32_0 & 0x1F ) );
}

static void R4300_CALL_TYPE R4300_Special_JR( R4300_CALL_SIGNATURE ) 			// Jump Register
{
	R4300_CALL_MAKE_OP( op_code );

	u32	new_pc( gGPR[ op_code.rs ]._u32_0 );

	CPU_TakeBranch( new_pc );
}


static void R4300_CALL_TYPE R4300_Special_JALR( R4300_CALL_SIGNATURE ) 		// Jump and Link register
{
	R4300_CALL_MAKE_OP( op_code );

	// Jump And Link
	u32	new_pc( gGPR[ op_code.rs ]._u32_0 );

	gGPR[ op_code.rd ]._s64 = (s64)(s32)(gCPUState.CurrentPC + 8);		// Store return address;

	CPU_TakeBranch( new_pc );
}


static void R4300_CALL_TYPE R4300_Special_SYSCALL( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	R4300_Exception_Syscall();
}

static void R4300_CALL_TYPE R4300_Special_BREAK( R4300_CALL_SIGNATURE ) 	// BREAK
{
	R4300_CALL_MAKE_OP( op_code );

	DPF( DEBUG_INTR, "BREAK Called. PC: 0x%08x. COUNT: 0x%08x", gCPUState.CurrentPC, gCPUState.CPUControl[C0_COUNT]._u32_0 );
	R4300_Exception_Break();
}

static void R4300_CALL_TYPE R4300_Special_SYNC( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// Just ignore
}

static void R4300_CALL_TYPE R4300_Special_MFHI( R4300_CALL_SIGNATURE ) 			// Move From MultHI
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	gGPR[ op_code.rd ]._u64 = gCPUState.MultHi._u64;
}

static void R4300_CALL_TYPE R4300_Special_MTHI( R4300_CALL_SIGNATURE ) 			// Move To MultHI
{
	R4300_CALL_MAKE_OP( op_code );

	gCPUState.MultHi._u64 = gGPR[ op_code.rs ]._u64;
}

static void R4300_CALL_TYPE R4300_Special_MFLO( R4300_CALL_SIGNATURE ) 			// Move From MultLO
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	gGPR[ op_code.rd ]._u64 = gCPUState.MultLo._u64;
}

static void R4300_CALL_TYPE R4300_Special_MTLO( R4300_CALL_SIGNATURE ) 			// Move To MultLO
{
	R4300_CALL_MAKE_OP( op_code );

	gCPUState.MultLo._u64 = gGPR[ op_code.rs ]._u64;
}

// BEGIN MODIFIED BY Lkb - 8/jun/2001 - changed 0x1f to 0x3f because the value to be shifted is 64-bit long
static void R4300_CALL_TYPE R4300_Special_DSLLV( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	// Reserved Instruction exception
	gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rt ]._u64 << ( gGPR[ op_code.rs ]._u32_0 & 0x3F );
}

static void R4300_CALL_TYPE R4300_Special_DSRLV( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	// Reserved Instruction exception
	gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rt ]._u64 >> ( gGPR[ op_code.rs ]._u32_0 & 0x3F );
}

// Aeroguage uses!
static void R4300_CALL_TYPE R4300_Special_DSRAV( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	// Reserved Instruction exception
	gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rt ]._s64 >> ( gGPR[ op_code.rs ]._u32_0 & 0x3F );
}
// END MODIFIED BY Lkb - 8/jun/2001



static void R4300_CALL_TYPE R4300_Special_MULT( R4300_CALL_SIGNATURE ) 			// MULTiply Signed
{
	R4300_CALL_MAKE_OP( op_code );

	s64 dwResult = (s64)gGPR[ op_code.rs ]._s32_0 * (s64)gGPR[ op_code.rt ]._s32_0;
	gCPUState.MultLo._u64 = (s64)(s32)(dwResult & 0xffffffff);
	gCPUState.MultHi._u64 = (s64)(s32)(dwResult >> 32);

}

static void R4300_CALL_TYPE R4300_Special_MULTU( R4300_CALL_SIGNATURE ) 		// MULTiply Unsigned
{
	R4300_CALL_MAKE_OP( op_code );

	u64 dwResult = (u64)gGPR[ op_code.rs ]._u32_0 * (u64)gGPR[ op_code.rt ]._u32_0;
	gCPUState.MultLo._u64 = (s64)(s32)(dwResult & 0xffffffff);
	gCPUState.MultHi._u64 = (s64)(s32)(dwResult >> 32);
}

static void R4300_CALL_TYPE R4300_Special_DIV( R4300_CALL_SIGNATURE ) 			//DIVide
{
	R4300_CALL_MAKE_OP( op_code );

	s32 nDividend = gGPR[ op_code.rs ]._s32_0;
	s32 nDivisor  = gGPR[ op_code.rt ]._s32_0;

	if (nDivisor)
	{
		gCPUState.MultLo._u64 = (s64)(s32)(nDividend / nDivisor);
		gCPUState.MultHi._u64 = (s64)(s32)(nDividend % nDivisor);
	}
}

static void R4300_CALL_TYPE R4300_Special_DIVU( R4300_CALL_SIGNATURE ) 			// DIVide Unsigned
{
	R4300_CALL_MAKE_OP( op_code );

	u32 dwDividend = gGPR[ op_code.rs ]._u32_0;
	u32 dwDivisor  = gGPR[ op_code.rt ]._u32_0;

	if (dwDivisor) {
		gCPUState.MultLo._u64 = (s64)(s32)(dwDividend / dwDivisor);
		gCPUState.MultHi._u64 = (s64)(s32)(dwDividend % dwDivisor);
	}
}

static void R4300_CALL_TYPE R4300_Special_DMULT( R4300_CALL_SIGNATURE ) 		// Double Multiply
{
	R4300_CALL_MAKE_OP( op_code );

	// Reserved Instruction exception
	gCPUState.MultLo._u64 = gGPR[ op_code.rs ]._s64 * gGPR[ op_code.rt ]._s64;
	gCPUState.MultHi._u64 = 0;
}

static void R4300_CALL_TYPE R4300_Special_DMULTU( R4300_CALL_SIGNATURE ) 			// Double Multiply Unsigned
{
	R4300_CALL_MAKE_OP( op_code );

	// Reserved Instruction exception
	gCPUState.MultLo._u64 = gGPR[ op_code.rs ]._u64 * gGPR[ op_code.rt ]._u64;
	gCPUState.MultHi._u64 = 0;
}

static void R4300_CALL_TYPE R4300_Special_DDIV( R4300_CALL_SIGNATURE ) 				// Double Divide
{
	R4300_CALL_MAKE_OP( op_code );

	// Check if this operation can be done in 32bit rather than 64bit //Corn
	if( ((gGPR[op_code.rs]._u32_1 + (gGPR[op_code.rs]._u32_0 >> 31)) +
		 (gGPR[op_code.rt]._u32_1 + (gGPR[op_code.rt]._u32_0 >> 31)) == 0) )
	{	//32bit
		s32 qwDividend = gGPR[ op_code.rs ]._s32_0;
		s32 qwDivisor = gGPR[ op_code.rt ]._s32_0;

		// Reserved Instruction exception
		if (qwDivisor)
		{
			gCPUState.MultLo._u64 = qwDividend / qwDivisor;
			gCPUState.MultHi._u64 = qwDividend % qwDivisor;
		}
	}
	else
	{	//64bit
		s64 qwDividend = gGPR[ op_code.rs ]._s64;
		s64 qwDivisor = gGPR[ op_code.rt ]._s64;

		// Reserved Instruction exception
		if (qwDivisor)
		{
			gCPUState.MultLo._u64 = qwDividend / qwDivisor;
			gCPUState.MultHi._u64 = qwDividend % qwDivisor;
		}
	}
}

static void R4300_CALL_TYPE R4300_Special_DDIVU( R4300_CALL_SIGNATURE ) 			// Double Divide Unsigned
{
	R4300_CALL_MAKE_OP( op_code );

	// Check if this operation can be done in 32bit rather than 64bit //Corn
	if( (gGPR[op_code.rs]._u32_1 | gGPR[op_code.rt]._u32_1) == 0 )
	{	//32bit
		u32 qwDividend = gGPR[ op_code.rs ]._u32_0;
		u32 qwDivisor = gGPR[ op_code.rt ]._u32_0;

		// Reserved Instruction exception
		if (qwDivisor)
		{
			gCPUState.MultLo._u64 = qwDividend / qwDivisor;
			gCPUState.MultHi._u64 = qwDividend % qwDivisor;
		}
	}
	else
	{	//64bit
		u64 qwDividend = gGPR[ op_code.rs ]._u64;
		u64 qwDivisor = gGPR[ op_code.rt ]._u64;

		// Reserved Instruction exception
		if (qwDivisor)
		{
			gCPUState.MultLo._u64 = qwDividend / qwDivisor;
			gCPUState.MultHi._u64 = qwDividend % qwDivisor;
		}
	}
}

static void R4300_CALL_TYPE R4300_Special_ADD( R4300_CALL_SIGNATURE ) 			// ADD signed - may throw exception
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	// Can generate overflow exception
	gGPR[ op_code.rd ]._s64 = (s64)(s32)( gGPR[ op_code.rs ]._s32_0 + gGPR[ op_code.rt ]._s32_0 );
}

static void R4300_CALL_TYPE R4300_Special_ADDU( R4300_CALL_SIGNATURE ) 			// ADD Unsigned - doesn't throw exception
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	gGPR[ op_code.rd ]._s64 = (s64)(s32)( gGPR[ op_code.rs ]._s32_0 + gGPR[ op_code.rt ]._s32_0 );
}

static void R4300_CALL_TYPE R4300_Special_SUB( R4300_CALL_SIGNATURE ) 			// SUB Signed - may throw exception
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	// Can generate overflow exception
	gGPR[ op_code.rd ]._s64 = (s64)(s32)( gGPR[ op_code.rs ]._s32_0 - gGPR[ op_code.rt ]._s32_0 );
}


static void R4300_CALL_TYPE R4300_Special_SUBU( R4300_CALL_SIGNATURE ) 			// SUB Unsigned - doesn't throw exception
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	gGPR[ op_code.rd ]._s64 = (s64)(s32)( gGPR[ op_code.rs ]._s32_0 - gGPR[ op_code.rt ]._s32_0 );
}

static void R4300_CALL_TYPE R4300_Special_AND( R4300_CALL_SIGNATURE ) 				// logical AND
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rs ]._u64 & gGPR[ op_code.rt ]._u64;
}

static void R4300_CALL_TYPE R4300_Special_OR( R4300_CALL_SIGNATURE ) 				// logical OR
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rs ]._u64 | gGPR[ op_code.rt ]._u64;
}

static void R4300_CALL_TYPE R4300_Special_XOR( R4300_CALL_SIGNATURE ) 				// logical XOR
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rs ]._u64 ^ gGPR[ op_code.rt ]._u64;
}

static void R4300_CALL_TYPE R4300_Special_NOR( R4300_CALL_SIGNATURE ) 				// logical Not OR
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	gGPR[ op_code.rd ]._u64 = ~( gGPR[ op_code.rs ]._u64 | gGPR[ op_code.rt ]._u64 );
}

static void R4300_CALL_TYPE R4300_Special_SLT( R4300_CALL_SIGNATURE ) 				// Set on Less Than
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	// Cast to s32s to ensure sign is taken into account
	if ( gGPR[ op_code.rs ]._s64 < gGPR[ op_code.rt ]._s64 )
	{
		gGPR[ op_code.rd ]._u64 = 1;
	}
	else
	{
		gGPR[ op_code.rd ]._u64 = 0;
	}
}

static void R4300_CALL_TYPE R4300_Special_SLTU( R4300_CALL_SIGNATURE ) 				// Set on Less Than Unsigned
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	// Treated as unsigned....
	if ( gGPR[ op_code.rs ]._u64 < gGPR[ op_code.rt ]._u64 )
	{
		gGPR[ op_code.rd ]._u64 = 1;
	}
	else
	{
		gGPR[ op_code.rd ]._u64 = 0;
	}

}


static void R4300_CALL_TYPE R4300_Special_DADD( R4300_CALL_SIGNATURE )//CYRUS64
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	gGPR[ op_code.rd ]._s64 = gGPR[ op_code.rs ]._s64 + gGPR[ op_code.rt ]._s64;

}

static void R4300_CALL_TYPE R4300_Special_DADDU( R4300_CALL_SIGNATURE )//CYRUS64
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rs ]._u64 + gGPR[ op_code.rt ]._u64;

	//BUG FIX for Excite Bike - Salvy
	// I don't know why Excite bike only works if the operand is 32bit.. this for sure is wrong as docs say.
	// Also this causes Conker to fail to display a cutscene in the final boss!
	//gGPR[ op_code.rd ]._s64 = (s64)( gGPR[ op_code.rt ]._s32_0 + gGPR[ op_code.rs ]._s32_0 ); 

}

static void R4300_CALL_TYPE R4300_Special_DSUB( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	gGPR[ op_code.rd ]._s64 = gGPR[ op_code.rs ]._s64 - gGPR[ op_code.rt ]._s64;
}

static void R4300_CALL_TYPE R4300_Special_DSUBU( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	// The order of rs and rt was wrong! It should be rs - rt, not rt - rs!!
	// It caused several lock ups in games ex Animal Crossing, and Conker to crash in last boss
	// Also signed extended to be safe
	gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rs ]._u64 - gGPR[ op_code.rt ]._u64;
}

static void R4300_CALL_TYPE R4300_Special_DSLL( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	// Reserved Instruction exception
	gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rt ]._u64 << op_code.sa;
}

static void R4300_CALL_TYPE R4300_Special_DSRL( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	// Reserved Instruction exception
    gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rt ]._u64 >> op_code.sa;
}

static void R4300_CALL_TYPE R4300_Special_DSRA( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	// Reserved Instruction exception
	gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rt ]._s64 >> op_code.sa;
}

static void R4300_CALL_TYPE R4300_Special_DSLL32( R4300_CALL_SIGNATURE ) 			// Double Shift Left Logical 32
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	// Reserved Instruction exception
	gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rt ]._u64 << ( 32 + op_code.sa );
}

static void R4300_CALL_TYPE R4300_Special_DSRL32( R4300_CALL_SIGNATURE ) 			// Double Shift Right Logical 32
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	// Reserved Instruction exception
	gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rt ]._u64 >> ( 32 + op_code.sa );
}

static void R4300_CALL_TYPE R4300_Special_DSRA32( R4300_CALL_SIGNATURE ) 			// Double Shift Right Arithmetic 32
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rd );

	// Reserved Instruction exception
	gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rt ]._s64 >> ( 32 + op_code.sa );
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

static void R4300_CALL_TYPE R4300_RegImm_Unk( R4300_CALL_SIGNATURE ) {  WARN_NOEXIST("R4300_RegImm_Unk"); }


static void R4300_CALL_TYPE R4300_RegImm_BLTZ( R4300_CALL_SIGNATURE ) 			// Branch on Less than Zero
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if rs < 0
	if ( gGPR[ op_code.rs ]._s64 < 0 )
	{
		s16 offset = (s16)op_code.immediate;

		if( offset == -1 )
		{
			SpeedHack( gCPUState.CurrentPC, op_code );
		}

		u32	new_pc( gCPUState.CurrentPC + ((s32)offset<<2) + 4 );
		CPU_TakeBranch( new_pc );
	}
}

static void R4300_CALL_TYPE R4300_RegImm_BLTZL( R4300_CALL_SIGNATURE ) 			// Branch on Less than Zero Likely
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if rs < 0
	if ( gGPR[ op_code.rs ]._s64 < 0 )
	{
		u32	new_pc( gCPUState.CurrentPC + ((s32)(s16)op_code.immediate<<2) + 4 );
		CPU_TakeBranch( new_pc );
	}
	else
	{
		// Don't execute subsequent instruction
		INCREMENT_PC();
	}

}
static void R4300_CALL_TYPE R4300_RegImm_BLTZAL( R4300_CALL_SIGNATURE ) 		// Branch on Less than Zero And Link
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if rs >= 0
	// Store the return address even if branch not taken

	// Store return address
	gGPR[REG_ra]._s64 = (s64)(s32)(gCPUState.CurrentPC + 8);		// Store return address	

	if ( gGPR[ op_code.rs ]._s64 < 0 )
	{
		u32	new_pc( gCPUState.CurrentPC + ((s32)(s16)op_code.immediate<<2) + 4 );
		CPU_TakeBranch( new_pc );
	}
}

static void R4300_CALL_TYPE R4300_RegImm_BGEZ( R4300_CALL_SIGNATURE ) 			// Branch on Greater than or Equal to Zero
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if rs >= 0
	if ( gGPR[ op_code.rs ]._s64 >= 0 )
	{
		s16 offset = (s16)op_code.immediate;

		if( offset == -1 )
		{
			SpeedHack( gCPUState.CurrentPC, op_code );
		}

		u32	new_pc( gCPUState.CurrentPC + ((s32)offset<<2) + 4 );
		CPU_TakeBranch( new_pc );
	}
}

static void R4300_CALL_TYPE R4300_RegImm_BGEZL( R4300_CALL_SIGNATURE ) 			// Branch on Greater than or Equal to Zero Likely
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if rs >= 0
	if ( gGPR[ op_code.rs ]._s64 >= 0 )
	{
		u32	new_pc( gCPUState.CurrentPC + ((s32)(s16)op_code.immediate<<2) + 4 );
		CPU_TakeBranch( new_pc );
	}
	else
	{
		// Don't execute subsequent instruction
		INCREMENT_PC();
	}
}

static void R4300_CALL_TYPE R4300_RegImm_BGEZAL( R4300_CALL_SIGNATURE ) 		// Branch on Greater than or Equal to Zero And Link
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if rs >= 0
	// This always happens, even if branch not taken

	gGPR[REG_ra]._s64 = (s64)(s32)(gCPUState.CurrentPC + 8);		// Store return address	

	if ( gGPR[ op_code.rs ]._s64 >= 0 )
	{
		u32	new_pc( gCPUState.CurrentPC + ((s32)(s16)op_code.immediate<<2) + 4 );
		CPU_TakeBranch( new_pc );
	}
}



/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

static void R4300_CALL_TYPE R4300_Cop0_Unk( R4300_CALL_SIGNATURE ) { WARN_NOEXIST("R4300_Cop0_Unk"); }
static void R4300_CALL_TYPE R4300_TLB_Unk( R4300_CALL_SIGNATURE )  { WARN_NOEXIST("R4300_TLB_Unk"); }


static void R4300_CALL_TYPE R4300_Cop0_MFC0( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

#ifdef DAEDALUS_ENABLE_ASSERTS
	if ( op_code.fs == C0_CAUSE )
	{
		bool	mi_interrupt_set( (Memory_MI_GetRegister(MI_INTR_MASK_REG) & Memory_MI_GetRegister(MI_INTR_REG)) != 0 );
		bool	cause_int_3_set( (gCPUState.CPUControl[C0_CAUSE]._u32_0 & CAUSE_IP3) != 0 );

		DAEDALUS_ASSERT( mi_interrupt_set == cause_int_3_set, "CAUSE_IP3 inconsistant with MI_INTR_REG" );
	}
#endif

	if( op_code.fs == C0_RAND )	// Copy from FS to RT
	{
		u32 wired = gCPUState.CPUControl[C0_WIRED]._u32_0 & 0x1F;

		// Select a value between wired and 31.
		// We should use TLB least-recently used here too?
		gGPR[ op_code.rt ]._s64 = (pspFastRand()%(32-wired)) + wired;

		DBGConsole_Msg(0, "[MWarning reading MFC0 random register]");
	}
	else	// Should we handle C0_COUNT??
	{
		CHECK_R0( op_code.rt );

		// No specific handling needs for reads to these registers.
		gGPR[ op_code.rt ]._s64 = (s64)gCPUState.CPUControl[ op_code.fs ]._s32_0;
	}
}

// Move Word To CopReg
static void R4300_CALL_TYPE R4300_Cop0_MTC0( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// Copy from RT to FS
	u32 new_value = gGPR[ op_code.rt ]._u32_0;

	switch ( op_code.fs )
	{
		/*case C0_INX:
			gCPUState.CPUControl[C0_INX]._u64 = new_value & 0x8000003F;
			DBGConsole_Msg(0, "Setting Index register to 0x%08x", (u32)new_value);
			break;*/

		case C0_CONTEXT:
			DBGConsole_Msg(0, "Setting context register to 0x%08x", new_value);
			gCPUState.CPUControl[ op_code.fs ]._u32_0 = new_value;
			break;

		case C0_WIRED:
			// Set to top limit on write to wired
			gCPUState.CPUControl[C0_RAND]._u32_0 = 31;
			DBGConsole_Msg(0, "Setting Wired register to 0x%08x", new_value);
			gCPUState.CPUControl[C0_WIRED]._u32_0 = new_value;
			break;

		case C0_RAND:	
		case C0_BADVADDR:
		case C0_PRID:
		case C0_CACHE_ERR:			// Furthermore, this reg must return 0 on reads.
			// All these registers are read only - make sure that software doesn't write to them
			DBGConsole_Msg(0, "MTC0. Software attempted to write to read only reg %s: 0x%08x", Cop0RegNames[ op_code.fs ], new_value);
			break;

		case C0_CAUSE:
			// Only IP1:0 (Interrupt Pending) bits are software writeable.
			// On writes, set all others to 0. Is this correct?
			//  Other bits are CE (copro error) BD (branch delay), the other
			// Interrupt pendings and EscCode.

			DAEDALUS_ASSERT(new_value == 0, "CAUSE register invalid writing");

#ifndef DAEDALUS_SILENT
			if ( (new_value&~0x300) != (gCPUState.CPUControl[C0_CAUSE]._u32_0&~0x300)  )
			{
				DBGConsole_Msg( 0, "[MWas previously clobbering CAUSE REGISTER" );
			}
#endif
			DPF( DEBUG_REGS, "CAUSE set to 0x%08x (was: 0x%08x)", new_value, gGPR[ op_code.rt ]._u32_0 );
			gCPUState.CPUControl[C0_CAUSE]._u32_0 &= ~0x300;
			gCPUState.CPUControl[C0_CAUSE]._u32_0 |= new_value & 0x300;
			break;
		case C0_SR:
			// Software can enable/disable interrupts here. We check if Interrupt Enable is
			//  set, and if there are any pending interrupts. If there are, then we set the
			//  CHECK_POSTPONED_INTERRUPTS flag to make sure we check for interrupts that have
			//  occurred since we disabled interrupts
			R4300_SetSR(new_value);
			break;

		case C0_COUNT:
			{
				// See comments below for COMPARE.
				// When this register is set, we need to check whether the next timed interrupt will
				//  be due to vertical blank or COMPARE
				gCPUState.CPUControl[C0_COUNT]._u32_0 = new_value;
				DBGConsole_Msg(0, "Count set - setting int");
				// XXXX Do we need to update any existing events?
				break;
			}
		case C0_COMPARE:
			{
				// When the value of COUNT equals the value of COMPARE, IP7 of the CAUSE register is set
				// (Timer interrupt). Writing to this register clears the timer interrupt pending flag.
				CPU_SetCompare(new_value);
			}
			break;


		// Need to check CONFIG register writes - not all fields are writable.
		// This also sets Endianness mode.

		// WatchHi/WatchLo are used to create a Watch Trap. This may not need implementing, but we should
		// Probably provide a warning on writes, just so that we know
		/*case C0_WATCHLO:
			DBGConsole_Msg( 0, "[MWROTE TO WATCHLO REGISTER!" );
			gCPUState.CPUControl[ op_code.fs ]._u32_0 = new_value;
			break;
		case C0_WATCHHI:
			DBGConsole_Msg( 0, "[MWROTE TO WATCHHI REGISTER!" );
			gCPUState.CPUControl[ op_code.fs ]._u32_0 = new_value;
			break;*/

		default:
			// No specific handling needs for writes to these registers.
			gCPUState.CPUControl[ op_code.fs ]._u32_0 = new_value;
			break;
	}
}

// XXX don't think TLB needs to be 64bit - Salvy
// UpdateValue(u32 _pagemask, u32 _hi, u32 _pfno, u32 _pfne) - So yea no need for 64bit..
//
static void R4300_CALL_TYPE R4300_TLB_TLBR( R4300_CALL_SIGNATURE ) 				// TLB Read
{
	R4300_CALL_MAKE_OP( op_code );

	u32 index = gCPUState.CPUControl[C0_INX]._u32_0 & 0x1F;

	gCPUState.CPUControl[C0_PAGEMASK]._u32_0 = g_TLBs[index].mask;
	gCPUState.CPUControl[C0_ENTRYHI ]._u32_0 = g_TLBs[index].hi   & (~g_TLBs[index].pagemask);
	gCPUState.CPUControl[C0_ENTRYLO0]._u32_0 = g_TLBs[index].pfne | g_TLBs[index].g;
	gCPUState.CPUControl[C0_ENTRYLO1]._u32_0 = g_TLBs[index].pfno | g_TLBs[index].g;

	DPF( DEBUG_TLB, "TLBR: INDEX: 0x%04x. PAGEMASK: 0x%08x.", index, gCPUState.CPUControl[C0_PAGEMASK]._u32_0 );
	DPF( DEBUG_TLB, "      ENTRYHI: 0x%08x. ENTRYLO1: 0x%08x. ENTRYLO0: 0x%08x", gCPUState.CPUControl[C0_ENTRYHI]._u32_0, gCPUState.CPUControl[C0_ENTRYLO1]._u32_0, gCPUState.CPUControl[C0_ENTRYLO0]._u32_0 );
}


static void R4300_CALL_TYPE R4300_TLB_TLBWI( R4300_CALL_SIGNATURE )			// TLB Write Index
{
	R4300_CALL_MAKE_OP( op_code );

	u32 i = gCPUState.CPUControl[C0_INX]._u32_0 & 0x1F;

	DPF( DEBUG_TLB, "TLBWI: INDEX: 0x%04x. ", i );

	g_TLBs[i].UpdateValue(gCPUState.CPUControl[C0_PAGEMASK]._u32_0,
						gCPUState.CPUControl[C0_ENTRYHI ]._u32_0,
						gCPUState.CPUControl[C0_ENTRYLO1]._u32_0,
						gCPUState.CPUControl[C0_ENTRYLO0]._u32_0);
}

static void R4300_CALL_TYPE R4300_TLB_TLBWR( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	u32 i;
	u32 wired = gCPUState.CPUControl[C0_WIRED]._u32_0 & 0x1F;

	// Select a value for index between wired and 31
	
	// errrg the vfpu here for rand causes alot of overhead :/
	//i = vfpu_randf(wired, 32);
	
	i = (pspFastRand()%(32-wired)) + wired;

	DPF( DEBUG_TLB, "TLBWR: INDEX: 0x%04x. ", i );

	g_TLBs[i].UpdateValue(gCPUState.CPUControl[C0_PAGEMASK]._u32_0,
						gCPUState.CPUControl[C0_ENTRYHI ]._u32_0,
						gCPUState.CPUControl[C0_ENTRYLO1]._u32_0,
						gCPUState.CPUControl[C0_ENTRYLO0]._u32_0);
}


static void R4300_CALL_TYPE R4300_TLB_TLBP( R4300_CALL_SIGNATURE ) 				// TLB Probe
{
	R4300_CALL_MAKE_OP( op_code );

	bool bFound = false;

	u32 dwEntryHi = gCPUState.CPUControl[C0_ENTRYHI]._u32_0;

	DPF( DEBUG_TLB, "TLBP: ENTRYHI: 0x%08x", dwEntryHi );
	//DBGConsole_Msg(0, "TLBP: ENTRYHI: 0x%08x", dwEntryHi);

    for( u32 i = 0; i < 32; i++ )
	{
		if( ((g_TLBs[i].hi & TLBHI_VPN2MASK) ==
		     (dwEntryHi    & TLBHI_VPN2MASK)) &&
			(
				 (g_TLBs[i].g) ||
				((g_TLBs[i].hi & TLBHI_PIDMASK) ==
				 (dwEntryHi    & TLBHI_PIDMASK))
			) ) {
				DPF( DEBUG_TLB, "   Found matching TLB Entry - 0x%04x", i );
				gCPUState.CPUControl[C0_INX]._u64 = i;
				bFound = true;
				break;
            }
    }

	if (!bFound)
	{
		//DBGConsole_Msg(0, "   No matching TLB Entry Found for 0x%08x", dwEntryHi);
		DPF( DEBUG_TLB, "   No matching TLB Entry Found for 0x%08x", dwEntryHi );
		gCPUState.CPUControl[C0_INX]._u64 = TLBINX_PROBE;

		//CPUHalt();
	}
	/*else
	{
		//DBGConsole_Msg(0, "   Matching TLB Entry Found for 0x%08x", dwEntryHi);
		//CPUHalt();
	}*/

}

static void R4300_CALL_TYPE R4300_TLB_ERET( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	if( gCPUState.CPUControl[C0_SR]._u32_0 & SR_ERL )
	{
		// Returning from an error trap
		DPF(DEBUG_INTR, "ERET: Returning from error trap");
		CPU_SetPC( gCPUState.CPUControl[C0_ERROR_EPC]._u32_0 );
		gCPUState.CPUControl[C0_SR]._u32_0 &= ~SR_ERL;
	}
	else
	{
		DPF(DEBUG_INTR, "ERET: Returning from interrupt/exception");
		// Returning from an exception
		CPU_SetPC( gCPUState.CPUControl[C0_EPC]._u32_0 );
		gCPUState.CPUControl[C0_SR]._u32_0 &= ~SR_EXL;
	}
	// Point to previous instruction (as we increment the pointer immediately afterwards
	DECREMENT_PC();

	// Ensure we don't execute this in the delay slot
	gCPUState.Delay = NO_DELAY;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


static void R4300_CALL_TYPE R4300_Cop1_Unk( R4300_CALL_SIGNATURE )     { WARN_NOEXIST("R4300_Cop1_Unk"); }




static void R4300_CALL_TYPE R4300_Cop1_MTC1( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// Manual says top bits undefined after load
	StoreFPR_Word( op_code.fs,  gGPR[ op_code.rt ]._s32_0 );
}

static void R4300_CALL_TYPE R4300_Cop1_DMTC1( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// Manual says top bits undefined after load
	StoreFPR_Long( op_code.fs, gGPR[ op_code.rt ]._u64 );
}


static void R4300_CALL_TYPE R4300_Cop1_MFC1( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	// MFC1 in the manual says this is a sign-extended result
	gGPR[ op_code.rt ]._s64 = (s64)(s32)LoadFPR_Word( op_code.fs );

}

static void R4300_CALL_TYPE R4300_Cop1_DMFC1( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	gGPR[ op_code.rt ]._s64 = LoadFPR_Long( op_code.fs );
}


static void R4300_CALL_TYPE R4300_Cop1_CFC1( R4300_CALL_SIGNATURE ) 		// move Control word From Copro 1
{
	R4300_CALL_MAKE_OP( op_code );

	CHECK_R0( op_code.rt );

	// Only defined for reg 0 or 31
	if ( op_code.fs == 0 || op_code.fs == 31 )
	//Saves a compare //Corn
	//if( !((op_code.fs + 1) & 0x1E) )
	{
		gGPR[ op_code.rt ]._s64 = (s64)gCPUState.FPUControl[ op_code.fs ]._s32_0;
		//gGPR[ op_code.rt ]._s32_0 = gCPUState.FPUControl[ op_code.fs ]._s32_0;  //copy only low part
	}
}

static void R4300_CALL_TYPE R4300_Cop1_CTC1( R4300_CALL_SIGNATURE ) 		// move Control word To Copro 1
{
	R4300_CALL_MAKE_OP( op_code );

	DAEDALUS_ASSERT( op_code.fs != 0, "CTC1 : Reg zero unhandled");
	// Only defined for reg 0 or 31
	// TODO - Maybe an exception was raised?
	// Not needed for 0?
	/*if ( op_code.fs == 0 )
	{
		gCPUState.FPUControl[ op_code.fs ]._u64 = gGPR[ op_code.rt ]._u64;

	}*/
	//else if ( op_code.fs == 31 )
	if ( op_code.fs == 31 )
	{
		gCPUState.FPUControl[ op_code.fs ] = gGPR[ op_code.rt ];

		u32		fpcr( gCPUState.FPUControl[ op_code.fs ]._u32_0 );

		switch ( fpcr & FPCSR_RM_MASK )
		{
		case FPCSR_RM_RN:		gRoundingMode = RM_ROUND;	break;
		case FPCSR_RM_RZ:		gRoundingMode = RM_TRUNC;	break;
		case FPCSR_RM_RP:		gRoundingMode = RM_CEIL;	break;
		case FPCSR_RM_RM:		gRoundingMode = RM_FLOOR;	break;
		default:				NODEFAULT;
		}

		SET_ROUND_MODE( gRoundingMode );
	}
	//else
	//{
	//}

	// Now generate lots of exceptions :-)
}

static void R4300_CALL_TYPE R4300_BC1_BC1F( R4300_CALL_SIGNATURE )		// Branch on FPU False
{
	R4300_CALL_MAKE_OP( op_code );

	if ( !(gCPUState.FPUControl[31]._u64 & FPCSR_C) )
	{
		u32	new_pc( gCPUState.CurrentPC + (s32)(s16)op_code.immediate*4 + 4 );
		CPU_TakeBranch( new_pc );
	}
}

static void R4300_CALL_TYPE R4300_BC1_BC1T( R4300_CALL_SIGNATURE )	// Branch on FPU True
{
	R4300_CALL_MAKE_OP( op_code );

	if ( gCPUState.FPUControl[31]._u64 & FPCSR_C )
	{
		u32	new_pc( gCPUState.CurrentPC + (s32)(s16)op_code.immediate*4 + 4 );
		CPU_TakeBranch( new_pc );
	}
}

static void R4300_CALL_TYPE R4300_BC1_BC1FL( R4300_CALL_SIGNATURE )	// Branch on FPU False Likely
{
	R4300_CALL_MAKE_OP( op_code );

	if ( !(gCPUState.FPUControl[31]._u64 & FPCSR_C) )
	{
		u32	new_pc( gCPUState.CurrentPC + (s32)(s16)op_code.immediate*4 + 4 );
		CPU_TakeBranch( new_pc );
	}
	else
	{
		// Don't execute subsequent instruction
		INCREMENT_PC();
	}
}

static void R4300_CALL_TYPE R4300_BC1_BC1TL( R4300_CALL_SIGNATURE )		// Branch on FPU True Likely
{
	R4300_CALL_MAKE_OP( op_code );

	if ( gCPUState.FPUControl[31]._u64 & FPCSR_C )
	{
		u32	new_pc( gCPUState.CurrentPC + (s32)(s16)op_code.immediate*4 + 4 );
		CPU_TakeBranch( new_pc );
	}
	else
	{
		// Don't execute subsequent instruction
		INCREMENT_PC();
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// WORD FP Instrs /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


static void R4300_CALL_TYPE R4300_Cop1_W_CVT_S( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	s32 nTemp = LoadFPR_Word( op_code.fs );

	StoreFPR_Single( op_code.fd, s32_to_f32( nTemp ) );
}

static void R4300_CALL_TYPE R4300_Cop1_W_CVT_D( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	s32 nTemp = LoadFPR_Word( op_code.fs );

	// Convert using current rounding mode?

	StoreFPR_Double( op_code.fd, s32_to_d64( nTemp ) );
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// LONG FP Instrs /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


static void R4300_CALL_TYPE R4300_Cop1_L_CVT_S( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	s64 nTemp = LoadFPR_Long( op_code.fs );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	StoreFPR_Single( op_code.fd, s64_to_f32( nTemp ));
}

static void R4300_CALL_TYPE R4300_Cop1_L_CVT_D( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	s64 nTemp = LoadFPR_Long( op_code.fs );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	StoreFPR_Double( op_code.fd, s64_to_d64( nTemp ) );
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// Single FP Instrs //////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


static void R4300_CALL_TYPE R4300_Cop1_S_Unk( R4300_CALL_SIGNATURE ) { WARN_NOEXIST("R4300_Cop1_S_Unk"); }


static void R4300_CALL_TYPE R4300_Cop1_S_ADD( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// fd = fs+ft
	f32 fX = LoadFPR_Single( op_code.fs );
	f32 fY = LoadFPR_Single( op_code.ft );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	StoreFPR_Single( op_code.fd, fX + fY );
}

static void R4300_CALL_TYPE R4300_Cop1_S_SUB( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// fd = fs-ft
	f32 fX = LoadFPR_Single( op_code.fs );
	f32 fY = LoadFPR_Single( op_code.ft );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	StoreFPR_Single( op_code.fd, fX - fY );
}

static void R4300_CALL_TYPE R4300_Cop1_S_MUL( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// fd = fs*ft
	f32 fX = LoadFPR_Single( op_code.fs );
	f32 fY = LoadFPR_Single( op_code.ft );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	StoreFPR_Single( op_code.fd, fX * fY );
}

static void R4300_CALL_TYPE R4300_Cop1_S_DIV( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// fd = fs/ft
	f32 fDividend = LoadFPR_Single( op_code.fs );
	f32 fDivisor  = LoadFPR_Single( op_code.ft );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	// Should we handle if /0? GoldenEye007 and Exitebike does this.
	// Not sure if is worth to handle this, I have yet to see a game that fails due this..
#ifndef DAEDALUS_SILENT
	if ( fDivisor == 0 )
	{
		DAEDALUS_ERROR("Float divide by zero");
		//fDivisor = 1;
	}
#endif
	// Causes excitebike to freeze when entering the menu
	/*if ( fDivisor == 0 )
	{
		if ( gCPUState.FPUControl[ 31 ]._u32_0 & FPCSR_EZ )
		{
			// Exception
			DBGConsole_Msg( 0, "[MShould trigger FPU exception for /0 here" );
		}
		else
		{
			//DBGConsole_Msg( 0, "Float divide by zero, setting flag" );
			gCPUState.FPUControl[ 31 ]._u32_0 |= FPCSR_FZ;
		}
	}*/

	StoreFPR_Single( op_code.fd, fDividend / fDivisor );
}

static void R4300_CALL_TYPE R4300_Cop1_S_SQRT( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// fd = sqrt(fs)
	f32 fX = LoadFPR_Single( op_code.fs );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	StoreFPR_Single( op_code.fd, pspFpuSqrt(fX) );
}


static void R4300_CALL_TYPE R4300_Cop1_S_NEG( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// fd = -(fs)
	f32 fX = LoadFPR_Single( op_code.fs );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	StoreFPR_Single( op_code.fd, -fX );
}

static void R4300_CALL_TYPE R4300_Cop1_S_MOV( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// fd = fs
	f32 fValue = LoadFPR_Single( op_code.fs );

	// Just copy bits directly?
	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	StoreFPR_Single( op_code.fd, fValue );
}

static void R4300_CALL_TYPE R4300_Cop1_S_ABS( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	StoreFPR_Single( op_code.fd, pspFpuAbs(fX) );
}


static void R4300_CALL_TYPE R4300_Cop1_S_TRUNC_W( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );

	StoreFPR_Word( op_code.fd, f32_to_s32_trunc( fX ) );
}

static void R4300_CALL_TYPE R4300_Cop1_S_TRUNC_L( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );

	StoreFPR_Long( op_code.fd, f32_to_s64_trunc( fX ) );
}


static void R4300_CALL_TYPE R4300_Cop1_S_ROUND_W( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );

	StoreFPR_Word( op_code.fd, f32_to_s32_round( fX ) );
}

static void R4300_CALL_TYPE R4300_Cop1_S_ROUND_L( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );

	StoreFPR_Long( op_code.fd, f32_to_s64_round( fX ) );
}


static void R4300_CALL_TYPE R4300_Cop1_S_CEIL_W( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );

	StoreFPR_Word( op_code.fd, f32_to_s32_ceil( fX ) );
}

static void R4300_CALL_TYPE R4300_Cop1_S_CEIL_L( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );

	StoreFPR_Long( op_code.fd, f32_to_s64_ceil( fX ) );
}

static void R4300_CALL_TYPE R4300_Cop1_S_FLOOR_W( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );

	StoreFPR_Word( op_code.fd, f32_to_s32_floor( fX ) );
}

static void R4300_CALL_TYPE R4300_Cop1_S_FLOOR_L( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );

	StoreFPR_Long( op_code.fd, f32_to_s64_floor( fX ) );
}


// Convert single to long - this is used by WarGods
static void R4300_CALL_TYPE R4300_Cop1_S_CVT_L( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );

	StoreFPR_Long( op_code.fd, f32_to_s64( fX, gRoundingMode ) );
}

// Convert single to word...
static void R4300_CALL_TYPE R4300_Cop1_S_CVT_W( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// XXXX On the PSP, this seem sto be doing trunc.w.s rather than cvt.w.s
	f32 fX = LoadFPR_Single( op_code.fs );
	s32	sX = f32_to_s32( fX, gRoundingMode );

	StoreFPR_Word( op_code.fd, sX );
}

// Convert single to f64...
static void R4300_CALL_TYPE R4300_Cop1_S_CVT_D( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	f32 fX = LoadFPR_Single( op_code.fs );

	StoreFPR_Double( op_code.fd, f32_to_d64( fX ) );
}


//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_S_EQ( R4300_CALL_SIGNATURE ) 				// Compare for Equality
{
	R4300_CALL_MAKE_OP( op_code );

	// fs == ft?
	f32 fX = LoadFPR_Single( op_code.fs );
	f32 fY = LoadFPR_Single( op_code.ft );

	if ( fX == fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}

//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_S_LT( R4300_CALL_SIGNATURE ) 				// Compare for Equality
{
	R4300_CALL_MAKE_OP( op_code );

	// fs < ft?
	f32 fX = LoadFPR_Single( op_code.fs );
	f32 fY = LoadFPR_Single( op_code.ft );

	if ( fX < fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}

//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_S_NGE( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );
	f32 fY = LoadFPR_Single( op_code.ft );

	CATCH_NAN_EXCEPTION( "R4300_Cop1_S_NGE", fX, fY );

	if(fX < fY)
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;

}

//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_S_LE( R4300_CALL_SIGNATURE ) 				// Compare for Equality
{
	R4300_CALL_MAKE_OP( op_code );

	// fs <= ft?
	f32 fX = LoadFPR_Single( op_code.fs );
	f32 fY = LoadFPR_Single( op_code.ft );

	CATCH_NAN_EXCEPTION( "R4300_Cop1_S_LE", fX, fY );

	if ( fX <= fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;

}

//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_S_SEQ( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );
	f32 fY = LoadFPR_Single( op_code.ft );

	CATCH_NAN_EXCEPTION( "R4300_Cop1_S_SEQ", fX, fY );

	if(fX == fY)
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}

//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_S_UEQ( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );
	f32 fY = LoadFPR_Single( op_code.ft );

	CATCH_NAN_EXCEPTION( "R4300_Cop1_S_UEQ", fX, fY );

	if( pspFpuIsNaN(fX + fY) || fX == fY )
	//if( fX == fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}

//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_S_NGLE( R4300_CALL_SIGNATURE )	
{
	R4300_CALL_MAKE_OP( op_code );

#ifndef DAEDALUS_SILENT
	f32 fX = LoadFPR_Single( op_code.fs );
	f32 fY = LoadFPR_Single( op_code.ft );

	CATCH_NAN_EXCEPTION( "R4300_Cop1_S_NGLE", fX, fY );
#endif

	gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}

//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_S_OLE( R4300_CALL_SIGNATURE )	
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );
	f32 fY = LoadFPR_Single( op_code.ft );

	CATCH_NAN_EXCEPTION( "R4300_Cop1_S_OLE", fX, fY );

	if (!pspFpuIsNaN(fX + fY) && fX <= fY )
	//if ( fX <= fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}

//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_S_ULE( R4300_CALL_SIGNATURE )	
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );
	f32 fY = LoadFPR_Single( op_code.ft );

	if(pspFpuIsNaN(fX + fY) || fX <= fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}

//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_S_UN( R4300_CALL_SIGNATURE )	
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );
	f32 fY = LoadFPR_Single( op_code.ft );

	if( pspFpuIsNaN(fX + fY) )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}

//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_S_F( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}

//*****************************************************************************
//
//*****************************************************************************
// Blast Corps fails here.
static void R4300_CALL_TYPE R4300_Cop1_S_NGT( R4300_CALL_SIGNATURE ) 
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );
	f32 fY = LoadFPR_Single( op_code.ft );

	CATCH_NAN_EXCEPTION( "R4300_Cop1_S_NGT", fX, fY );

	if ( fX <= fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}

//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_S_ULT( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );
	f32 fY = LoadFPR_Single( op_code.ft );

	if( pspFpuIsNaN(fX + fY) || fX < fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}

//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_S_SF( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

#ifndef DAEDALUS_SILENT
	f32 fX = LoadFPR_Single( op_code.fs );
	f32 fY = LoadFPR_Single( op_code.ft );

	CATCH_NAN_EXCEPTION( "R4300_Cop1_S_SF", fX, fY );
#endif

	gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}

//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_S_NGL( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );
	f32 fY = LoadFPR_Single( op_code.ft );

	CATCH_NAN_EXCEPTION( "R4300_Cop1_S_NGL", fX, fY );

	if ( fX == fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}

//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_S_OLT( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Single( op_code.fs );
	f32 fY = LoadFPR_Single( op_code.ft );

	CATCH_NAN_EXCEPTION( "R4300_Cop1_S_OLT", fX, fY );

	if (!pspFpuIsNaN(fX + fY) && fX < fY )
	//if ( fX < fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


static void R4300_CALL_TYPE R4300_Cop1_D_Unk( R4300_CALL_SIGNATURE )		{ WARN_NOEXIST("R4300_Cop1_D_Unk"); }


static void R4300_CALL_TYPE R4300_Cop1_D_ABS( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	StoreFPR_Double( op_code.fd, pspFpuAbs(fX) );
}

static void R4300_CALL_TYPE R4300_Cop1_D_ADD( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// fd = fs+ft
	d64 fX = LoadFPR_Double( op_code.fs );
	d64 fY = LoadFPR_Double( op_code.ft );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	if (gSimulateDoubleDisabled)
	{
		StoreFPR_Double_2( op_code.fd, (f64)fX + (f64)fY );
	}
	else
	{
		StoreFPR_Double( op_code.fd, fX + fY );
	}

}
static void R4300_CALL_TYPE R4300_Cop1_D_SUB( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// fd = fs-ft
	d64 fX = LoadFPR_Double( op_code.fs );
	d64 fY = LoadFPR_Double( op_code.ft );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	StoreFPR_Double( op_code.fd, fX - fY );
}

static void R4300_CALL_TYPE R4300_Cop1_D_MUL( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// fd = fs*ft
	d64 fX = LoadFPR_Double( op_code.fs );
	d64 fY = LoadFPR_Double( op_code.ft );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	StoreFPR_Double( op_code.fd, fX * fY );
}

static void R4300_CALL_TYPE R4300_Cop1_D_DIV( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// fd = fs/ft
	d64 fDividend = LoadFPR_Double( op_code.fs );
	d64 fDivisor = LoadFPR_Double( op_code.ft );

#ifndef DAEDALUS_SILENT
	if ( fDivisor == 0 )
	{
		DAEDALUS_ERROR("Double divide by zero");
		//fDivisor = 1;
	}
#endif
	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	StoreFPR_Double( op_code.fd,  fDividend / fDivisor );
}

static void R4300_CALL_TYPE R4300_Cop1_D_SQRT( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// fd = sqrt(fs)
	d64 fX = LoadFPR_Double( op_code.fs );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	StoreFPR_Double( op_code.fd, pspFpuSqrt(fX) );
}


static void R4300_CALL_TYPE R4300_Cop1_D_NEG( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// fd = -(fs)
	d64 fX = LoadFPR_Double( op_code.fs );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	StoreFPR_Double( op_code.fd, -fX );
}

static void R4300_CALL_TYPE R4300_Cop1_D_MOV( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// fd = fs
	d64 fX = LoadFPR_Double( op_code.fs );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	StoreFPR_Double( op_code.fd, fX );

	// TODO - Just copy bits - don't interpret!
}

static void R4300_CALL_TYPE R4300_Cop1_D_TRUNC_W( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );

	StoreFPR_Word( op_code.fd, d64_to_s32_trunc( fX ) );
}

static void R4300_CALL_TYPE R4300_Cop1_D_TRUNC_L( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );

	StoreFPR_Long( op_code.fd, d64_to_s64_trunc( fX ) );
}


static void R4300_CALL_TYPE R4300_Cop1_D_ROUND_W( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );

	StoreFPR_Word( op_code.fd, d64_to_s32_round( fX ) );
}

static void R4300_CALL_TYPE R4300_Cop1_D_ROUND_L( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );

	StoreFPR_Long( op_code.fd, d64_to_s64_round( fX ) );
}


static void R4300_CALL_TYPE R4300_Cop1_D_CEIL_W( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );

	StoreFPR_Word( op_code.fd, d64_to_s32_ceil( fX ) );
}

static void R4300_CALL_TYPE R4300_Cop1_D_CEIL_L( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );

	StoreFPR_Long( op_code.fd, d64_to_s64_ceil( fX ) );
}
//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_FLOOR_W( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );

	StoreFPR_Word( op_code.fd, d64_to_s32_floor( fX ) );
}
//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_FLOOR_L( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );

	StoreFPR_Long( op_code.fd, d64_to_s64_floor( fX ) );
}
//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_CVT_S( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	f32 fX = LoadFPR_Double( op_code.fs );

	//SET_ROUND_MODE( gRoundingMode );		//XXXX Is this needed?

	StoreFPR_Single( op_code.fd, fX );
}
//*****************************************************************************
//
//*****************************************************************************
// Convert f64 to word...
static void R4300_CALL_TYPE R4300_Cop1_D_CVT_W( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );

	StoreFPR_Word( op_code.fd, d64_to_s32( fX, gRoundingMode ) );
}
//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_CVT_L( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );

	StoreFPR_Long( op_code.fd, d64_to_s64( fX, gRoundingMode ) );
}
//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_EQ( R4300_CALL_SIGNATURE )				// Compare for Equality
{
	R4300_CALL_MAKE_OP( op_code );

	// fs == ft?
	d64 fX = LoadFPR_Double( op_code.fs );
	d64 fY = LoadFPR_Double( op_code.ft );

	if( fX == fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}
//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_LE( R4300_CALL_SIGNATURE )				// Compare for Less Than or Equal
{
	R4300_CALL_MAKE_OP( op_code );

	// fs <= ft?
	d64 fX = LoadFPR_Double( op_code.fs );
	d64 fY = LoadFPR_Double( op_code.ft );

	if( fX <= fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;

}

//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_LT( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// fs < ft?
	d64 fX = LoadFPR_Double( op_code.fs );
	d64 fY = LoadFPR_Double( op_code.ft );

	if( fX < fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;

}
//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_F( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}
//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_UN( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );
	d64 fY = LoadFPR_Double( op_code.ft );

	if( pspFpuIsNaN(fX + fY) )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}
//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_UEQ( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );
	d64 fY = LoadFPR_Double( op_code.ft );

	if( pspFpuIsNaN(fX + fY) || fX == fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}
//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_OLT( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );
	d64 fY = LoadFPR_Double( op_code.ft );

	if( !pspFpuIsNaN(fX + fY) && fX < fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}
//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_ULT( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );
	d64 fY = LoadFPR_Double( op_code.ft );

	if( pspFpuIsNaN(fX + fY) || fX < fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}
//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_OLE( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );
	d64 fY = LoadFPR_Double( op_code.ft );

	if( !pspFpuIsNaN(fX + fY) && fX <= fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}
//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_ULE( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );
	d64 fY = LoadFPR_Double( op_code.ft );

	if( pspFpuIsNaN(fX + fY) || fX <= fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}
//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_SF( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

#ifndef DAEDALUS_SILENT
	d64 fX = LoadFPR_Double( op_code.fs );
	d64 fY = LoadFPR_Double( op_code.ft );

	CATCH_NAN_EXCEPTION( "R4300_Cop1_D_SF", fX, fY );
#endif

	gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}
//*****************************************************************************
// Same as above..
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_NGLE( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

#ifndef DAEDALUS_SILENT
	d64 fX = LoadFPR_Double( op_code.fs );
	d64 fY = LoadFPR_Double( op_code.ft );

	CATCH_NAN_EXCEPTION( "R4300_Cop1_D_NGLE", fX, fY );
#endif

	gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}
//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_SEQ( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );
	d64 fY = LoadFPR_Double( op_code.ft );

	CATCH_NAN_EXCEPTION( "R4300_Cop1_D_SEQ", fX, fY );

	if( fX == fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}

//*****************************************************************************
// Same as above..
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_NGL( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );
	d64 fY = LoadFPR_Double( op_code.ft );

	CATCH_NAN_EXCEPTION( "R4300_Cop1_D_NGL", fX, fY );

	if( fX == fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}
//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_NGE( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );
	d64 fY = LoadFPR_Double( op_code.ft );

	CATCH_NAN_EXCEPTION( "R4300_Cop1_D_NGE", fX, fY );

	if( fX < fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}
//*****************************************************************************
//
//*****************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_D_NGT( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	d64 fX = LoadFPR_Double( op_code.fs );
	d64 fY = LoadFPR_Double( op_code.ft );

	CATCH_NAN_EXCEPTION( "R4300_Cop1_D_NGT", fX, fY );

	if( fX <= fY )
		gCPUState.FPUControl[31]._u64 |= FPCSR_C;
	else
		gCPUState.FPUControl[31]._u64 &= ~FPCSR_C;
}

#include "R4300_Jump.inl"		// Jump table
//*****************************************************************************
//
//*****************************************************************************
CPU_Instruction	R4300_GetInstructionHandler( OpCode op_code )
{
	switch( op_code.op )
	{
	case OP_SPECOP:
		return R4300SpecialInstruction[ op_code.spec_op ];

	case OP_REGIMM:
		return R4300RegImmInstruction[ op_code.regimm_op ];

	case OP_COPRO0:
		switch( op_code.cop0_op )
		{
		case Cop0Op_TLB:
			return R4300TLBInstruction[ op_code.cop0tlb_funct ];
		default:
			return R4300Cop0Instruction[ op_code.cop0_op ];
		}

	case OP_COPRO1:
		// It is the responsibility of the caller to check whether the
		// copprocessor is enabled, and throw and exception accordingly.
		switch( op_code.cop1_op )
		{
		case Cop1Op_BCInstr:
			return R4300Cop1BC1Instruction[ op_code.cop1_bc ];
		case Cop1Op_SInstr:
			return R4300Cop1SInstruction[ op_code.cop1_funct ];
		case Cop1Op_DInstr:
				return R4300Cop1DInstruction_32[ op_code.cop1_funct ];

		}
		return R4300Cop1Instruction[ op_code.cop1_op ];

	default:
		return R4300Instruction[ op_code.op ];
	}
}
