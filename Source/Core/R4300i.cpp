/*
Copyright (C) 2001 StrmnNrmn
Copyright (C) 2011 Salvy6735

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

#define	R4300_CALL_MAKE_OP( var )	OpCode	var;	var._u32 = op_code_bits

#define ird				gGPR[op_code.rd]._s64
#define irt				gGPR[op_code.rt]._s64
#define irs				gGPR[op_code.rs]._s64
#define ibase			gGPR[op_code.base]._s64
#define ipc				gCPUState.CurrentPC

#define ihi				gCPUState.MultHi._s64
#define ilo				gCPUState.MultLo._s64

#define c0FD			gCPUState.CPUControl[ op_code.fd ]._u32_0
#define c0FS			gCPUState.CPUControl[ op_code.fs ]._u32_0
#define c0FT			gCPUState.CPUControl[ op_code.ft ]._u32_0

//ToDo : When FP is in 64bit, these need to extend to 64 entries!
#define cFD				gCPUState.FPU[op_code.rd]._u32_0
#define cFS				gCPUState.FPU[op_code.fs]._u32_0
#define cRT				gCPUState.FPU[op_code.rt]._u32_0
#define cFT				gCPUState.FPU[op_code.ft]._u32_0

#define	cCONFS			gCPUState.FPUControl[ op_code.fs ]._u32_0
#define	cCON31			gCPUState.FPUControl[31]._u32_0

#define itarget			( (ipc & 0xF0000000) | (op_code.target<<2) )
#define link(x)			{ gGPR[x]._s64 = (s32) (ipc + 8); }
#define iimmediate		((s16)op_code.immediate)
#define iaddress		(u32) ((s32) ibase + (s32) iimmediate)
#define delay_set(x) \
	{ \
		gCPUState.TargetPC = ipc + 4 + (x << 2); \
		gCPUState.Delay = DO_DELAY; \
	}
#define cpu_branch( x ) \
{ \
	gCPUState.TargetPC = x; \
	gCPUState.Delay = DO_DELAY; \
}


#define SUB_S64( a, b, c)	a = (s64)b - (s64)c
#define SUB_S32( a, b, c)	a = (s64) ((s32) b - (s32) c)
#define ADDI_S32( a, b, c)	a = (s64) ((s32) b + (s32) c)
#define ADDI_S64( a, b, c)	a = (s64)b + (s64)c
#define ANDI_U64( a, b, c)	a = (u64)b & (u64)c
#define XORI_U64( a, b, c)	a = (u64)b ^ (u64)c
#define ORI_U64( a, b, c)	a = (u64)b | (u64)c
#define SHIFT_U32( a, b, c)	a = (s64) (s32) ((u32) b << (u32)c)

inline void write_64bit_fpu_reg(u32 reg, u32 *val)
{
	gCPUState.FPU[reg]._u32_0 = val[0];
	gCPUState.FPU[reg + 1]._u32_0 = val[1];
}


#ifndef DAEDALUS_SILENT
inline void CHECK_R0( u32 op )
{
	if(gGPR[0]._u64 != 0)
	{
		DBGConsole_Msg(0, "Warning: Attempted write to r0!"); \
		gGPR[0]._u64 = 0; // Ensure r0 is always zero (easier than trapping writes?)
		return;
	}

	DAEDALUS_ASSERT( op, "Possible attempt to write to r0!");
}
#else
	inline void CHECK_R0( u32 op ) {}
#endif

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


inline void SpeedHack( u32 current_pc, OpCode op )
{
	OpCode	next_op;

#ifdef DAEDALUS_ENABLE_DYNAREC
	if (gTraceRecorder.IsTraceActive())
		return;
#endif

	// TODO: Should maybe use some internal function, so we can account
	// for things like Branch/DelaySlot pair straddling a page boundary.
	next_op._u32 = Read32Bits( current_pc + 4 );

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
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_DBG_Bkpt( R4300_CALL_SIGNATURE )
{

}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_J( R4300_CALL_SIGNATURE ) 				// Jump
{
	R4300_CALL_MAKE_OP( op_code );

	if( itarget == ipc )	SpeedHack( ipc, op_code );

	cpu_branch( itarget );
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_JAL( R4300_CALL_SIGNATURE ) 				// Jump And Link
{
	R4300_CALL_MAKE_OP( op_code );

	link( REG_ra );	// Store return address

	cpu_branch( itarget );
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_BEQ( R4300_CALL_SIGNATURE ) 		// Branch on Equal
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if irs == irt
	if ( (u64)irs == (u64)irt )
	{
		if( iimmediate == -1 )	SpeedHack( ipc, op_code );

		delay_set( iimmediate );
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_BNE( R4300_CALL_SIGNATURE )             // Branch on Not Equal
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if irs <> irt
	if ( (u64)irs != (u64)irt )
	{
		if( iimmediate == -1 )	SpeedHack( ipc, op_code );

		delay_set( iimmediate );
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_BLEZ( R4300_CALL_SIGNATURE ) 			// Branch on Less than of Equal to Zero
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if irs <= 0
	if (irs <= 0)
	{
		if( iimmediate == -1 )	SpeedHack( ipc, op_code );

		delay_set( iimmediate );
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_BGTZ( R4300_CALL_SIGNATURE ) 			// Branch on Greater than Zero
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if irs > 0
	if (irs > 0)
	{
		if( iimmediate == -1 )	SpeedHack( ipc, op_code );

		delay_set( iimmediate );
	}
}


//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_DADDI( R4300_CALL_SIGNATURE ) 			// Doubleword ADD Immediate
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	// Check for overflow
	// Reserved Instruction exception

	//irt = irs + immediate
	ADDI_S64( irt, irs, (s16)iimmediate );
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_DADDIU( R4300_CALL_SIGNATURE ) 			// Doubleword ADD Immediate Unsigned
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	// Reserved Instruction exception

	//irt = irs + immediate
	ADDI_S64( irt, irs, (s16)iimmediate );

}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_ADDI( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	// Generates overflow exception

	//irt = irs + immediate
	ADDI_S32( irt, irs, (s16)iimmediate );
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_ADDIU( R4300_CALL_SIGNATURE ) 		// Add Immediate Unsigned
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	//irt = irs + immediate
	ADDI_S32( irt, irs, (s16)iimmediate );
}


//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_SLTI( R4300_CALL_SIGNATURE ) 			// Set on Less Than Immediate
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	// Cast to s32s to ensure sign is taken into account
	if(irs < (s64) (s32) (s16) (u16) iimmediate)
		irt = 1;
	else
		irt = 0;
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_SLTIU( R4300_CALL_SIGNATURE ) 		// Set on Less Than Immediate Unsigned
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	// Cast to s32s to ensure sign is taken into account
	if((u64) irs < (u64) (s64) (s32) (s16) (u16) iimmediate)
		irt = 1;
	else
		irt = 0;
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_ANDI( R4300_CALL_SIGNATURE ) 				// AND Immediate
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	//irt = irs & immediate
	ANDI_U64( irt, irs, (u16)iimmediate );
}
//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_ORI( R4300_CALL_SIGNATURE ) 				// OR Immediate
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	//irt = irs | immediate
	ORI_U64( irt, irs, (u16)iimmediate );
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_XORI( R4300_CALL_SIGNATURE ) 				// XOR Immediate
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	//irt = irs ^ immediate
	XORI_U64( irt, irs, (u16)iimmediate );
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_LUI( R4300_CALL_SIGNATURE ) 				// Load Upper Immediate
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	irt = (s64) (s32) (iimmediate << (u32) 16);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_BEQL( R4300_CALL_SIGNATURE ) 			// Branch on Equal Likely
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if irs == irt
	if((u64) irs == (u64) irt)
	{
		if( iimmediate == -1 )	SpeedHack( ipc, op_code );

		delay_set( iimmediate );
	}
	else
	{
		// Don't execute subsequent instruction
		INCREMENT_PC();
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_BNEL( R4300_CALL_SIGNATURE ) 			// Branch on Not Equal Likely
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if irs <> irt
	if((u64) irs != (u64) irt)
	{
		if( iimmediate == -1 )	SpeedHack( ipc, op_code );

		delay_set( iimmediate );
	}
	else
	{
		// Don't execute subsequent instruction
		INCREMENT_PC();
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_BLEZL( R4300_CALL_SIGNATURE ) 		// Branch on Less than or Equal to Zero Likely
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if irs <= 0
	if (irs <= 0)
	{
		/*if( iimmediate == -1 )
		{
			SpeedHack( ipc, op_code );
		}

		if (op_code.rs == N64Reg_R0)
		{
			SpeedHack( ipc, op_code );
		}*/

		delay_set( iimmediate );
	}
	else
	{
		// Don't execute subsequent instruction
		INCREMENT_PC();
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_BGTZL( R4300_CALL_SIGNATURE ) 		// Branch on Greater than Zero Likely
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if irs > 0
	if ( irs > 0 )
	{
		if( iimmediate == -1 )	SpeedHack( ipc, op_code );

		delay_set( iimmediate );
	}
	else
	{
		// Don't execute subsequent instruction
		INCREMENT_PC();
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_LB( R4300_CALL_SIGNATURE ) 			// Load Byte
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	irt = (s64)(s8)Read8Bits(iaddress);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_LBU( R4300_CALL_SIGNATURE ) 			// Load Byte Unsigned -- Zero extend byte...
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	irt = (s64)(u8)Read8Bits(iaddress);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_LH( R4300_CALL_SIGNATURE ) 		// Load Halfword
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	irt = (s64)(s16)Read16Bits(iaddress);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_LHU( R4300_CALL_SIGNATURE )			// Load Halfword Unsigned -- Zero extend word
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	irt = (u64)(u16)Read16Bits(iaddress);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_LWL( R4300_CALL_SIGNATURE ) 			// Load Word Left
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	u32 nMemory = Read32Bits(iaddress & ~0x3);

	switch (iaddress & 3)
	{
        case 0: irt = (s64) (s32)nMemory; break;
        case 1: irt = (s64) (s32) ((((u32) irt) & 0x000000FF) | (nMemory << 8)); break;
        case 2: irt = (s64) (s32) ((((u32) irt) & 0x0000FFFF) | (nMemory << 16)); break;
        case 3: irt = (s64) (s32) ((((u32) irt) & 0x00FFFFFF) | (nMemory << 24)); break;
    }
}

//*************************************************************************************
//
//*************************************************************************************
// Starcraft - not tested!
static void R4300_CALL_TYPE R4300_LDL( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	DAEDALUS_ERROR("CHECK");
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_LWR( R4300_CALL_SIGNATURE ) 			// Load Word Right
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	u32 nMemory = Read32Bits(iaddress & ~0x3);

	switch (iaddress & 3)
	{
	case 3:	irt = (s64) (s32) nMemory;	break;
	case 2:	irt = (s64) (s32) ((((u32) irt) & 0xff000000) | (nMemory >> 8));	break;
	case 1:	irt = (s64) (s32) ((((u32) irt) & 0xffff0000) | (nMemory >> 16));	break;
	case 0:	irt = (s64) (s32) ((((u32) irt) & 0xffffff00) | (nMemory >> 24));	break;
	}
}

//*************************************************************************************
//
//*************************************************************************************
// Starcraft - not tested!
static void R4300_CALL_TYPE R4300_LDR( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	DAEDALUS_ERROR("CHECK");
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_LW( R4300_CALL_SIGNATURE ) 			// Load Word
{
	R4300_CALL_MAKE_OP( op_code );	//CHECK_R0( op_code.rt );


	// This is for San Francisco 2049. An R0 errg.. otherwise it crashes when the race is about to start.
	if (op_code.rt == 0)
	{
		DAEDALUS_ERROR("Attempted write to r0!");
		return;	// I think is better to trap it than override it
	}

	irt = (s64)(s32)Read32Bits(iaddress);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_LWU( R4300_CALL_SIGNATURE ) 			// Load Word Unsigned
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	irt = (u64)(u32)Read32Bits(iaddress);
}
//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_SW( R4300_CALL_SIGNATURE ) 			// Store Word
{
	R4300_CALL_MAKE_OP( op_code );

	Write32Bits(iaddress, (u32)irt);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_SH( R4300_CALL_SIGNATURE ) 			// Store Halfword
{
	R4300_CALL_MAKE_OP( op_code );

	Write16Bits(iaddress, (u16)(irt & 0xffff));
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_SB( R4300_CALL_SIGNATURE ) 			// Store Byte
{
	R4300_CALL_MAKE_OP( op_code );

	Write8Bits(iaddress, (u8)(irt & 0xff));
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_SWL( R4300_CALL_SIGNATURE ) 			// Store Word Left
{
	R4300_CALL_MAKE_OP( op_code );

	u32 nMemory = Read32Bits(iaddress & ~0x3);
	u32 nNew = 0;


	switch (iaddress & 3)
	{
	case 0:	nNew = (u32)irt; break;			// Aligned
	case 1:	nNew = (u32)(nMemory & 0xFF000000) | ((u32)irt >> 8 ); break;
	case 2:	nNew = (u32)(nMemory & 0xFFFF0000) | ((u32)irt >> 16); break;
	case 3:	nNew = (u32)(nMemory & 0xFFFFFF00) | ((u32)irt >> 24); break;
	}
	Write32Bits(iaddress & ~0x3, nNew);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_SWR( R4300_CALL_SIGNATURE ) 			// Store Word Right
{
	R4300_CALL_MAKE_OP( op_code );

	u32 nMemory = Read32Bits(iaddress & ~0x3);
	u32 nNew = 0;

	switch (iaddress & 3)
	{
	case 3:	nNew = (u32)irt; break;			// Aligned
	case 2:	nNew = (u32)(nMemory & 0x000000FF) | ((u32)irt >> 8 ); break;
	case 1:	nNew = (u32)(nMemory & 0x0000FFFF) | ((u32)irt >> 16); break;
	case 0:	nNew = (u32)(nMemory & 0x00FFFFFF) | ((u32)irt >> 24); break;
	}
	Write32Bits(iaddress & ~0x3, nNew);

}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_SDL( R4300_CALL_SIGNATURE )//CYRUS64
{
	R4300_CALL_MAKE_OP( op_code );

	DAEDALUS_ERROR("CHECK");
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_SDR( R4300_CALL_SIGNATURE )//CYRUS64
{
	R4300_CALL_MAKE_OP( op_code );

	DAEDALUS_ERROR("CHECK");
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_CACHE( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

//	return;

#ifdef DAEDALUS_ENABLE_DYNAREC
	u32 cache_op  = op_code.rt;

	// Do Nothing
	u32 cache = cache_op & 0x3;
	u32 action = (cache_op >> 2) & 0x7;

	if(cache == 0 && (action == 0 || action == 4))
	{
		//DBGConsole_Msg( 0, "Cache invalidate - forcibly dumping dynarec contents" );
		CPU_InvalidateICacheRange(iaddress, 0x20);
	}

	//DBGConsole_Msg(0, "CACHE %s/%d, 0x%08x", gCacheNames[dwCache], dwAction, iaddress);
#endif
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_LWC1( R4300_CALL_SIGNATURE ) 				// Load Word to Copro 1 (FPU)
{
	R4300_CALL_MAKE_OP( op_code );

	 *(u32 *)&cFT = Read32Bits(iaddress);	// check me
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_LDC1( R4300_CALL_SIGNATURE )				// Load Doubleword to Copro 1 (FPU)
{
	R4300_CALL_MAKE_OP( op_code );

	// CONKER BUGDIX - GREEN TEXTURES
	u32	reg[2];

	reg[0] = Read32Bits(iaddress + 4);
	reg[1] = Read32Bits(iaddress);
	write_64bit_fpu_reg(op_code.ft, (u32 *) &reg[0]);

	// Green textures are fixed only if we ignore FullLength aka writing the low and high bits in 32bit as the FP is in 32bit mode as above.
	// even though is setting the status reg to 64bit
	// Conker sets the status reg and FullLength is noted and thus this bug occurs in the old interpreter as well
	// I believe theres a deeper issue, NOTE I believe this is set in R4300_TLB_ERET and not R4300_SetSR since the status reg returns 0 there (32bit).
	//
	//*(u64*)&cFT = *(u64*)&Read64Bits(iaddress);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_LD( R4300_CALL_SIGNATURE ) 				// Load Doubleword
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	irt = Read64Bits(iaddress);
}

//*************************************************************************************
//
//*************************************************************************************

static void R4300_CALL_TYPE R4300_SWC1( R4300_CALL_SIGNATURE ) 			// Store Word From Copro 1
{
	R4300_CALL_MAKE_OP( op_code );

	Write32Bits(iaddress, (u32)cFT);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_SDC1( R4300_CALL_SIGNATURE )		// Store Doubleword From Copro 1
{
	R4300_CALL_MAKE_OP( op_code );

	/*u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );

	Write64Bits(address, LoadFPR_Long(op_code.ft));*/
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_SD( R4300_CALL_SIGNATURE )			// Store Doubleword
{
	R4300_CALL_MAKE_OP( op_code );

	Write64Bits(iaddress, irt);
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

//static void R4300_CALL_TYPE R4300_Special_Unk( R4300_CALL_SIGNATURE ) { WARN_NOEXIST("R4300_Special_Unk"); }
static void R4300_CALL_TYPE R4300_Special_SLL( R4300_CALL_SIGNATURE ) 		// Shift word Left Logical
{
	R4300_CALL_MAKE_OP( op_code );

	// NOP!
	if ( op_code._u32 == 0 ) return;

	CHECK_R0( op_code.rd );

	ird = (s64) (s32) ((u32) irt << op_code.sa);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_SRL( R4300_CALL_SIGNATURE ) 		// Shift word Right Logical
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	ird = (s64) (s32) ((u32) irt >> op_code.sa);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_SRA( R4300_CALL_SIGNATURE ) 		// Shift word Right Arithmetic
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	ird = (s64) ((s32) irt >> op_code.sa);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_SLLV( R4300_CALL_SIGNATURE ) 		// Shift word Left Logical Variable
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	SHIFT_U32( ird, irt, (irs & 0x1F) );
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_SRLV( R4300_CALL_SIGNATURE ) 		// Shift word Right Logical Variable
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	ird = (s64) (s32) ((u32) irt >> (((u32) irs) & 0x1F));
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_SRAV( R4300_CALL_SIGNATURE ) 		// Shift word Right Arithmetic Variable
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	ird = (s64) ((s32) irt >> (((u32) irs) & 0x1F));
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_JR( R4300_CALL_SIGNATURE ) 			// Jump Register
{
	R4300_CALL_MAKE_OP( op_code );

	cpu_branch( (u32)irs );
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_JALR( R4300_CALL_SIGNATURE ) 		// Jump and Link register
{
	R4300_CALL_MAKE_OP( op_code );

	// Store return address
	link( op_code.rd );

	cpu_branch( (u32)irs );
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_SYSCALL( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	R4300_Exception_Syscall();
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_BREAK( R4300_CALL_SIGNATURE ) 	// BREAK
{
	R4300_CALL_MAKE_OP( op_code );

	DPF( DEBUG_INTR, "BREAK Called. PC: 0x%08x. COUNT: 0x%08x", gCPUState.CurrentPC, gCPUState.CPUControl[C0_COUNT]._u32_0 );
	R4300_Exception_Break();
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_SYNC( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// Just ignore
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_MFHI( R4300_CALL_SIGNATURE ) 			// Move From MultHI
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	ird = ihi;
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_MTHI( R4300_CALL_SIGNATURE ) 			// Move To MultHI
{
	R4300_CALL_MAKE_OP( op_code );

	ihi = irs;
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_MFLO( R4300_CALL_SIGNATURE ) 			// Move From MultLO
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	ird = ilo;
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_MTLO( R4300_CALL_SIGNATURE ) 			// Move To MultLO
{
	R4300_CALL_MAKE_OP( op_code );

	ilo = irs;
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_DSLLV( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	// Reserved Instruction exception
	ird = (u64) irt << (((u32) irs) & 0x3F);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_DSRLV( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	// Reserved Instruction exception
	ird = (u64) irt >> (((u32) irs) & 0x3F);
}

//*************************************************************************************
//
//*************************************************************************************
// Aeroguage uses!
static void R4300_CALL_TYPE R4300_Special_DSRAV( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	// Reserved Instruction exception
	ird = irt >> (((u32) irs) & 0x3F);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_MULT( R4300_CALL_SIGNATURE ) 			// MULTiply Signed
{
	R4300_CALL_MAKE_OP( op_code );

	s64	result;

	result = (s64) (s32) irs * (s64) (s32) irt;

	ilo = (s64) (s32) result;
	ihi = (s64) (s32) (((u64) result) >> 32);

}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_MULTU( R4300_CALL_SIGNATURE ) 		// MULTiply Unsigned
{
	R4300_CALL_MAKE_OP( op_code );

	u64	result;

	result = (u64) (u32) irs * (u64) (u32) irt;
	ilo = (s64) (s32) result;
	ihi = (s64) (s32) (((u64) result) >> 32);

}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_DIV( R4300_CALL_SIGNATURE ) 			//DIVide
{
	R4300_CALL_MAKE_OP( op_code );

	s32 nDividend = (s32) irs;
	s32 nDivisor = (s32) irt;

	if(nDivisor != 0)
	{
		ilo = (s64) (s32) (nDividend / nDivisor);
		ihi = (s64) (s32) (nDividend % nDivisor);
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_DIVU( R4300_CALL_SIGNATURE ) 			// DIVide Unsigned
{
	R4300_CALL_MAKE_OP( op_code );

	u32 nDividend = (u32) irs;
	u32 nDivisor = (u32) irt;

	if(nDivisor != 0)
	{
		ilo = (s64) (s32) (nDividend / nDivisor);
		ihi = (s64) (s32) (nDividend % nDivisor);
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_DMULT( R4300_CALL_SIGNATURE ) 		// Double Multiply
{
	R4300_CALL_MAKE_OP( op_code );

	DAEDALUS_ERROR("DMULT");

	u64 op1, op2, op3, op4;
	u64 result1, result2, result3, result4;
	u64 temp1, temp2, temp3, temp4;
	int sign = 0;

	if (irs < 0)
	{
		op2 = -irs;
		sign = 1 - sign;
	}
	else
	{
		op2 = irs;
	}

	if (irt < 0)
	{
		op4 = -irt;
		sign = 1 - sign;
	}
	else
	{
		op4 = irt;
	}

	op1 = op2 & 0xFFFFFFFF;
	op2 = (op2 >> 32) & 0xFFFFFFFF;
	op3 = op4 & 0xFFFFFFFF;
	op4 = (op4 >> 32) & 0xFFFFFFFF;

	temp1 = op1 * op3;
	temp2 = (temp1 >> 32) + op1 * op4;
	temp3 = op2 * op3;
	temp4 = (temp3 >> 32) + op2 * op4;

	result1 = temp1 & 0xFFFFFFFF;
	result2 = temp2 + (temp3 & 0xFFFFFFFF);
	result3 = (result2 >> 32) + temp4;
	result4 = (result3 >> 32);

	ilo = result1 | (result2 << 32);
	ihi = (result3 & 0xFFFFFFFF) | (result4 << 32);
	if (sign)
	{
		ihi = ~ihi;
		if (!ilo)
			ihi++;
		else
			ilo = ~ilo + 1;
	}

}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_DMULTU( R4300_CALL_SIGNATURE ) 			// Double Multiply Unsigned
{
	R4300_CALL_MAKE_OP( op_code );

	DAEDALUS_ERROR("DMULTU");

	u64 op1, op2, op3, op4;
	u64 result1, result2, result3, result4;
	u64 temp1, temp2, temp3, temp4;

	op1 = irs & 0xFFFFFFFF;
	op2 = (irs >> 32) & 0xFFFFFFFF;
	op3 = irt & 0xFFFFFFFF;
	op4 = (irt >> 32) & 0xFFFFFFFF;

	temp1 = op1 * op3;
	temp2 = (temp1 >> 32) + op1 * op4;
	temp3 = op2 * op3;
	temp4 = (temp3 >> 32) + op2 * op4;

	result1 = temp1 & 0xFFFFFFFF;
	result2 = temp2 + (temp3 & 0xFFFFFFFF);
	result3 = (result2 >> 32) + temp4;
	result4 = (result3 >> 32);

	ilo = result1 | (result2 << 32);
	ihi = (result3 & 0xFFFFFFFF) | (result4 << 32);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_DDIV( R4300_CALL_SIGNATURE ) 				// Double Divide
{
	R4300_CALL_MAKE_OP( op_code );

	s64 nDividend = (s64) irs;
	s64 nDivisor = (s64) irt;

	// Reserved Instruction exception
	if(nDivisor != 0)
	{
		ilo = nDividend / nDivisor;
		ihi = nDividend % nDivisor;
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_DDIVU( R4300_CALL_SIGNATURE ) 			// Double Divide Unsigned
{
	R4300_CALL_MAKE_OP( op_code );

	u64 nDividend = (u64) irs;
	u64 nDivisor = (u64) irt;

	// Reserved Instruction exception
	if(nDivisor != 0)
	{
		ilo = nDividend / nDivisor;
		ihi = nDividend % nDivisor;
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_ADD( R4300_CALL_SIGNATURE ) 			// ADD signed - may throw exception
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	// Can generate overflow exception
	ird = (s64) ((s32) irs + (s32) irt);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_ADDU( R4300_CALL_SIGNATURE ) 			// ADD Unsigned - doesn't throw exception
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	ird = (s64) ((s32) irs + (s32) irt);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_SUB( R4300_CALL_SIGNATURE ) 			// SUB Signed - may throw exception
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	// Can generate overflow exception
	SUB_S32( ird , irs, irt );
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_SUBU( R4300_CALL_SIGNATURE ) 			// SUB Unsigned - doesn't throw exception
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	SUB_S32( ird , irs, irt );
}


//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_AND( R4300_CALL_SIGNATURE ) 				// logical AND
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	ANDI_U64( ird, irs, irt );
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_OR( R4300_CALL_SIGNATURE ) 				// logical OR
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	ORI_U64( ird, irs, irt );
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_XOR( R4300_CALL_SIGNATURE ) 				// logical XOR
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	XORI_U64( ird, irs, irt );
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_NOR( R4300_CALL_SIGNATURE ) 				// logical Not OR
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	ird = ~((u64) irs | (u64) irt);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_SLT( R4300_CALL_SIGNATURE ) 				// Set on Less Than
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	// Cast to s32s to ensure sign is taken into account
	if(irs < irt)
		ird = 1;
	else
		ird = 0;
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_SLTU( R4300_CALL_SIGNATURE ) 				// Set on Less Than Unsigned
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	// Treated as unsigned....
	if((u64) irs < (u64) irt)
		ird = 1;
	else
		ird = 0;
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_DADD( R4300_CALL_SIGNATURE )//CYRUS64
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	ADDI_S64( ird, irs, irt );

}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_DADDU( R4300_CALL_SIGNATURE )//CYRUS64
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	ADDI_S64( ird, irs, irt );
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_DSUB( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	SUB_S64( ird, irs, irt );
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_DSUBU( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	SUB_S64( ird, irs, irt );
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_DSLL( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	// Reserved Instruction exception
	ird = (u64) irt << op_code.sa;
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_DSRL( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	// Reserved Instruction exception
	ird = (u64) irt >> op_code.sa;
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_DSRA( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	// Reserved Instruction exception
	ird = irt >> op_code.sa;
}


static void R4300_CALL_TYPE R4300_Special_DSLL32( R4300_CALL_SIGNATURE ) 			// Double Shift Left Logical 32
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	// Reserved Instruction exception
	ird = (u64) irt << (32 + op_code.sa);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_DSRL32( R4300_CALL_SIGNATURE ) 			// Double Shift Right Logical 32
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	// Reserved Instruction exception
	ird = (u64) irt >> (op_code.sa + 32);
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Special_DSRA32( R4300_CALL_SIGNATURE ) 			// Double Shift Right Arithmetic 32
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rd );

	// Reserved Instruction exception
	//gGPR[ op_code.rd ]._u64 = gGPR[ op_code.rt ]._s64 >> ( 32 + op_code.sa );
#if 0
	int rd = op_code.rd;
    int rt = op_code.rt;

    * (u32 *) &gGPR[rd]._s64      = *((u32 *) &gGPR[rt]._s64 + 1);
    *((u32 *) &gGPR[rd]._s64 + 1) = *((u32 *) &gGPR[rt]._s64 + 1);

    *((s32 *) &gGPR[rd]._s64 + 1) >>= 31;
    *((s32 *) &gGPR[rd]._s64)     >>= op_code.sa;
#else
	ird = irt >> (op_code.sa + 32);
#endif

}
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

//static void R4300_CALL_TYPE R4300_RegImm_Unk( R4300_CALL_SIGNATURE ) {  WARN_NOEXIST("R4300_RegImm_Unk"); }
static void R4300_CALL_TYPE R4300_RegImm_BLTZ( R4300_CALL_SIGNATURE ) 			// Branch on Less than Zero
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if rs < 0
	if ( irs < 0 )
	{
		if( iimmediate == -1 )	SpeedHack( ipc, op_code );

		delay_set( iimmediate );
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_RegImm_BLTZL( R4300_CALL_SIGNATURE ) 			// Branch on Less than Zero Likely
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if rs < 0
	if ( irs < 0 )
	{
		//if( iimmediate == -1 )	SpeedHack( ipc, op_code ); // Check Me

		delay_set( iimmediate );
	}
	else
	{
		// Don't execute subsequent instruction
		INCREMENT_PC();
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_RegImm_BLTZAL( R4300_CALL_SIGNATURE ) 		// Branch on Less than Zero And Link
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if rs >= 0
	// Store the return address even if branch not taken

	// Store return address
	link( REG_ra );

	if ( irs < 0 )
	{
		//if( iimmediate == -1 )	SpeedHack( ipc, op_code );	// Check Me

		delay_set( iimmediate );
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_RegImm_BGEZ( R4300_CALL_SIGNATURE ) 			// Branch on Greater than or Equal to Zero
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if rs >= 0
	if ( irs >= 0 )
	{
		if( iimmediate == -1 )	SpeedHack( ipc, op_code );

		delay_set( iimmediate );
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_RegImm_BGEZL( R4300_CALL_SIGNATURE ) 			// Branch on Greater than or Equal to Zero Likely
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if rs >= 0
	if ( irs >= 0 )
	{
		delay_set( iimmediate );
	}
	else
	{
		// Don't execute subsequent instruction
		INCREMENT_PC();
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_RegImm_BGEZAL( R4300_CALL_SIGNATURE ) 		// Branch on Greater than or Equal to Zero And Link
{
	R4300_CALL_MAKE_OP( op_code );

	//branch if rs >= 0
	// This always happens, even if branch not taken

	// Store return address
	link( REG_ra );

	if ( irs >= 0 )
	{
		delay_set( iimmediate );
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

//static void R4300_CALL_TYPE R4300_Cop1_Unk( R4300_CALL_SIGNATURE )     { WARN_NOEXIST("R4300_Cop1_Unk"); }
static void R4300_CALL_TYPE R4300_Cop1_MTC1( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// Manual says top bits undefined after load
	* (u32 *) &cFS = (u32) irt;
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_DMTC1( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// Manual says top bits undefined after load
	 *(u64*)&cFS = irt;
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_MFC1( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	// MFC1 in the manual says this is a sign-extended result
	(*(s64 *) &irt) = (s64) (*((s32 *) &cFS));

}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_DMFC1( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	irt = *(u64*)&cFS;
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_CFC1( R4300_CALL_SIGNATURE ) 		// move Control word From Copro 1
{
	R4300_CALL_MAKE_OP( op_code );	CHECK_R0( op_code.rt );

	// Only defined for reg 0 or 31
	if ( op_code.fs == 0 || op_code.fs == 31 )
	{
		irt = (s64) (s32) cCONFS;
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_Cop1_CTC1( R4300_CALL_SIGNATURE ) 		// move Control word To Copro 1
{
	R4300_CALL_MAKE_OP( op_code );

	if ( op_code.fs == 31 )
	{
		cCON31 = (u32) irt;

		switch ( cCON31 & FPCSR_RM_MASK )
		{
		case FPCSR_RM_RN:		gRoundingMode = RM_ROUND;	break;
		case FPCSR_RM_RZ:		gRoundingMode = RM_TRUNC;	break;
		case FPCSR_RM_RP:		gRoundingMode = RM_CEIL;	break;
		case FPCSR_RM_RM:		gRoundingMode = RM_FLOOR;	break;
		default:				NODEFAULT;
		}

		SET_ROUND_MODE( gRoundingMode );
	}

	// Now generate lots of exceptions :-)
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_BC1_BC1F( R4300_CALL_SIGNATURE )		// Branch on FPU False
{
	R4300_CALL_MAKE_OP( op_code );

	if((((u32) cCON31 & FPCSR_C)) == 0)
	{
		//ToDO : SpeedHack?

		delay_set( iimmediate );
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_BC1_BC1T( R4300_CALL_SIGNATURE )	// Branch on FPU True
{
	R4300_CALL_MAKE_OP( op_code );

	if((((u32)cCON31 & FPCSR_C)) != 0)
	{
		//ToDO : SpeedHack?

		delay_set( iimmediate );
	}
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_BC1_BC1FL( R4300_CALL_SIGNATURE )	// Branch on FPU False Likely
{
	R4300_CALL_MAKE_OP( op_code );

	if((((u32) cCON31 & FPCSR_C)) == 0)
	{
		//ToDO : SpeedHack?

		delay_set( iimmediate );
	}
	else
	{
		// Don't execute subsequent instruction
		INCREMENT_PC();
	}
}
//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_BC1_BC1TL( R4300_CALL_SIGNATURE )		// Branch on FPU True Likely
{
	R4300_CALL_MAKE_OP( op_code );

	if((((u32)cCON31 & FPCSR_C)) != 0)
	{
		//ToDO : SpeedHack?

		delay_set( iimmediate )
	}
	else
	{
		// Don't execute subsequent instruction
		INCREMENT_PC();
	}
}
//*************************************************************************************
//
//*************************************************************************************
/*
static void R4300_CALL_TYPE R4300_Cop1_C_S_Generic( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// fs < ft?
	float	fX, fY;
	bool less, equal, unordered, cond, cond0, cond1, cond2, cond3;

	cond0 = (op_code._u32   ) & 0x1;
	cond1 = (op_code._u32>>1) & 0x1;
	cond2 = (op_code._u32>>2) & 0x1;
	cond3 = (op_code._u32>>3) & 0x1;

	fX = *((float *) &cFS);
	fY = *((float *) &cRT);

	if (pspFpuIsNaN(fX) || pspFpuIsNaN(fY))
	{
		//DBGConsole_Msg(0, "is nan");
		less = false;
		equal = false;
		unordered = true;

		// exception
		if (cond3)
		{
			// Exception
			printf( "Should throw fp nan exception?\n" );
			return;
		}
	}
	else
	{
		less  = (fX < fY);
		equal = (fX == fY);
		unordered = false;

	}

	cond = ((cond0 && unordered) || (cond1 && equal) || (cond2 && less));

    cCON31 &= ~FPCSR_C;

	if(cond)
		cCON31 |= FPCSR_C;

}

template < bool FullLength > static void R4300_CALL_TYPE R4300_Cop1_C_D_Generic( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	// fs < ft?
	float	fX, fY;
	bool less, equal, unordered, cond, cond0, cond1, cond2, cond3;

	cond0 = (op_code._u32   ) & 0x1;
	cond1 = (op_code._u32>>1) & 0x1;
	cond2 = (op_code._u32>>2) & 0x1;
	cond3 = (op_code._u32>>3) & 0x1;

	fX = *((double *) &cFS);
	fY = *((double *) &cRT);

	if (pspFpuIsNaN(fX) || pspFpuIsNaN(fY))
	{
		//DBGConsole_Msg(0, "is nan");
		less = false;
		equal = false;
		unordered = true;

		// exception
		if (cond3)
		{
			// Exception
			printf( "Should throw fp nan exception?\n" );
			return;
		}
	}
	else
	{
		less  = (fX < fY);
		equal = (fX == fY);
		unordered = false;

	}

	cond = ((cond0 && unordered) || (cond1 && equal) || (cond2 && less));

    cCON31 &= ~FPCSR_C;

	if(cond)
		cCON31 |= FPCSR_C;


}
*/
void R4300_New( OpCode op_code, u32 op_code_bits )
{

	switch( op_code.op )
	{
	case OP_J:			R4300_J( op_code_bits );		 break;
	case OP_JAL:		R4300_JAL( op_code_bits );		 break;

	case OP_ADDI:		R4300_ADDI( op_code_bits );		 break;
	case OP_ADDIU:		R4300_ADDIU( op_code_bits );	 break;
	case OP_SLTI:		R4300_SLTI( op_code_bits );		 break;
	case OP_SLTIU:		R4300_SLTIU( op_code_bits );	 break;

	case OP_ANDI:		R4300_ANDI( op_code_bits );		 break;
	case OP_ORI:		R4300_ORI( op_code_bits );		 break;
	case OP_XORI:		R4300_XORI( op_code_bits );		 break;
	case OP_LUI:		R4300_LUI( op_code_bits );		 break;

	case OP_LB:			R4300_LB( op_code_bits );		 break;
	case OP_LBU:		R4300_LBU( op_code_bits );		 break;
	case OP_LH:			R4300_LH( op_code_bits );		 break;
	case OP_LHU:		R4300_LHU( op_code_bits );		 break;
	case OP_LW:			R4300_LW( op_code_bits );		 break;
	case OP_LWC1:		R4300_LWC1( op_code_bits );		 break;
	case OP_LDC1:		R4300_LDC1( op_code_bits );		 break;
	case OP_LD:			R4300_LD( op_code_bits );		 break;

	case OP_SB:			R4300_SB( op_code_bits );		 break;
	case OP_SH:			R4300_SH( op_code_bits );		 break;
	case OP_SW:			R4300_SW( op_code_bits );		 break;
	case OP_SWC1:		R4300_SWC1( op_code_bits );		 break;
	case OP_SD:			R4300_SD( op_code_bits );		 break;

	case OP_BEQ:		R4300_BEQ( op_code_bits );		 break;
	case OP_BEQL:		R4300_BEQL( op_code_bits );		 break;
	case OP_BNE:		R4300_BNE( op_code_bits );		 break;
	case OP_BNEL:		R4300_BNEL( op_code_bits );		 break;
	case OP_BLEZ:		R4300_BLEZ( op_code_bits );		 break;
	case OP_BLEZL:		R4300_BLEZL( op_code_bits );	 break;
	case OP_BGTZ:		R4300_BGTZ( op_code_bits );		 break;
	case OP_BGTZL:		R4300_BGTZL( op_code_bits );	 break;

	case OP_CACHE:		R4300_CACHE( op_code_bits );	 break;

	case OP_REGIMM:
		switch( op_code.regimm_op )
		{
		case RegImmOp_BLTZ:		R4300_RegImm_BLTZ( op_code_bits );	 break;
		case RegImmOp_BLTZL:	R4300_RegImm_BLTZL( op_code_bits );	 break;

		case RegImmOp_BGEZ:		R4300_RegImm_BGEZ( op_code_bits );	 break;
		case RegImmOp_BGEZL:	R4300_RegImm_BGEZL( op_code_bits );	 break;

		case RegImmOp_BLTZAL:	R4300_RegImm_BLTZAL( op_code_bits ); break;
		case RegImmOp_BGEZAL:	R4300_RegImm_BGEZAL( op_code_bits ); break;
		}
		break;

	case OP_SPECOP:
		switch( op_code.spec_op )
		{
		case SpecOp_SLL:	R4300_Special_SLL( op_code_bits );  break;
		case SpecOp_SRA:	R4300_Special_SRA( op_code_bits );  break;
		case SpecOp_SRL:	R4300_Special_SRL( op_code_bits );  break;
		case SpecOp_SLLV:	R4300_Special_SLLV( op_code_bits ); break;
		case SpecOp_SRAV:	R4300_Special_SRAV( op_code_bits ); break;
		case SpecOp_SRLV:	R4300_Special_SRLV( op_code_bits ); break;

		case SpecOp_JR:		R4300_Special_JR( op_code_bits );   break;
		case SpecOp_JALR:	R4300_Special_JALR( op_code_bits ); break;

		case SpecOp_MFLO:	R4300_Special_MFLO( op_code_bits ); break;
		case SpecOp_MFHI:	R4300_Special_MFHI( op_code_bits ); break;
		case SpecOp_MTLO:	R4300_Special_MTLO( op_code_bits ); break;
		case SpecOp_MTHI:	R4300_Special_MTHI( op_code_bits ); break;

		//XXXX
		case SpecOp_DSLLV:	R4300_Special_DSLLV( op_code_bits ); break;
		case SpecOp_DSRLV:	R4300_Special_DSRLV( op_code_bits ); break;
		case SpecOp_DSRAV:	R4300_Special_DSRAV( op_code_bits ); break;

		case SpecOp_MULT:	R4300_Special_MULT( op_code_bits ); break;
		case SpecOp_MULTU:	R4300_Special_MULTU( op_code_bits ); break;

		case SpecOp_DMULT:	R4300_Special_DMULT( op_code_bits ); break;
		case SpecOp_DMULTU: R4300_Special_DMULTU( op_code_bits ); break;
		case SpecOp_DDIV:	R4300_Special_DDIV( op_code_bits ); break;
		case SpecOp_DDIVU:	R4300_Special_DDIVU( op_code_bits ); break;

		case SpecOp_DIV:	R4300_Special_DIV( op_code_bits ); break;
		case SpecOp_DIVU:	R4300_Special_DIVU( op_code_bits ); break;

		case SpecOp_ADD:	R4300_Special_ADD( op_code_bits ); break;
		case SpecOp_ADDU:	R4300_Special_ADDU( op_code_bits ); break;
		case SpecOp_SUB:	R4300_Special_SUB( op_code_bits ); break;
		case SpecOp_SUBU:	R4300_Special_SUBU( op_code_bits ); break;

		case SpecOp_AND:	R4300_Special_AND( op_code_bits );    break;
		case SpecOp_OR:		R4300_Special_OR( op_code_bits );     break;
		case SpecOp_XOR:	R4300_Special_XOR( op_code_bits );    break;
		case SpecOp_NOR:	R4300_Special_NOR( op_code_bits );    break;

		case SpecOp_SLT:	R4300_Special_SLT( op_code_bits );    break;
		case SpecOp_SLTU:	R4300_Special_SLTU( op_code_bits );	  break;

		case SpecOp_DADD:	R4300_Special_DADD( op_code_bits );   break;
		case SpecOp_DADDU:	R4300_Special_DADDU( op_code_bits );  break;
		case SpecOp_DSUB:	R4300_Special_DSUB( op_code_bits );   break;
		case SpecOp_DSUBU:	R4300_Special_DSUBU( op_code_bits );  break;

		case SpecOp_DSLL:	R4300_Special_DSLL( op_code_bits );	  break;
		case SpecOp_DSRL:	R4300_Special_DSRL( op_code_bits );	  break;
		case SpecOp_DSRA:	R4300_Special_DSRA( op_code_bits );	  break;
		case SpecOp_DSLL32: R4300_Special_DSLL32( op_code_bits ); break;
		case SpecOp_DSRL32: R4300_Special_DSRL32( op_code_bits ); break;
		case SpecOp_DSRA32: R4300_Special_DSRA32( op_code_bits ); break;

		default:
			printf(" %d\n",op_code.spec_op);
			break;
		}
		break;
/*
	case OP_COPRO0:
		switch( op_code.cop0_op )
		{
		case Cop0Op_MFC0:	R4300_Cop0_MFC0( op_code_bits ); break;
		case Cop0Op_MTC0:	R4300_Cop0_MTC0( op_code_bits ); break;
		default:	// TLB
			switch( op_code.cop0tlb_funct )
			{
			case OP_TLBR:	R4300_TLB_TLBR( op_code_bits );	 break;
			case OP_TLBWI:	R4300_TLB_TLBWI( op_code_bits ); break;
			case OP_TLBWR:	R4300_TLB_TLBWR( op_code_bits ); break;
			case OP_TLBBP:	R4300_TLB_TLBP( op_code_bits ); break;
			case OP_ERET:	R4300_TLB_ERET( op_code_bits );  break;
			default:
				printf(" %d\n",op_code.cop0tlb_funct);
				break;
			}
		}
		break;
*/
	default:
		R4300_ExecuteInstruction_Generic( op_code );
		break;
	}

}
