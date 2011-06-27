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


#define SUB_S32( a, b, c)	a = (s64) ((s32) b - (s32) c)
#define ADDI_S32( a, b, c)	a = (s64) ((s32) b + (s32) c)
#define ADDI_S64( a, b, c)	a = (s64)b + (s64)c
#define ANDI_U64( a, b, c)	a = (u64)b & (u64)c
#define XORI_U64( a, b, c)	a = (u64)b ^ (u64)c
#define ORI_U64( a, b, c)	a = (u64)b | (u64)c
#define SHIFT_U32( a, b, c)	a = (s64) (s32) ((u32) b << (u32)c)

//ToDo : 32bit 64 items needed, not 64bit 32 items (cheaper on the PSP)
#define ifd				gCPUState.FPU[op_code.rd]._u64
#define ift				gCPUState.FPU[op_code.rt]._u64
#define ifs				gCPUState.FPU[op_code.rs]._u64

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

	 *(u32 *)&ift = Read32Bits(iaddress);	// check me
}

//*************************************************************************************
//
//*************************************************************************************
static void R4300_CALL_TYPE R4300_LDC1( R4300_CALL_SIGNATURE )				// Load Doubleword to Copro 1 (FPU)
{
	R4300_CALL_MAKE_OP( op_code );

	 *(u64*)&ift = Read64Bits(iaddress);
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

	//u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );
	//Write32Bits(address, (u32)gCPUState.FPU[dwFT]);
	//Write32Bits(address, LoadFPR_Word(op_code.ft));
}

static void R4300_CALL_TYPE R4300_SDC1( R4300_CALL_SIGNATURE )		// Store Doubleword From Copro 1
{
	R4300_CALL_MAKE_OP( op_code );

	/*u32 address = (u32)( gGPR[op_code.base]._s32_0 + (s32)(s16)op_code.immediate );

	Write64Bits(address, LoadFPR_Long(op_code.ft));*/
}


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