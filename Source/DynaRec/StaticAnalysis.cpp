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

#include "StaticAnalysis.h"

#include "Core/R4300OpCode.h"
#include "Core/CPU.h"
#include "Core/ROM.h"

using namespace StaticAnalysis;

namespace
{

void RegFPRRead( u32 r ) {}
void RegFPRWrite( u32 r ) {}


typedef void (*StaticAnalysisFunction )( OpCode op_code, RegisterUsage & recorder );


// Forward declarations
void StaticAnalysis_Special( OpCode op_code, RegisterUsage & recorder );
void StaticAnalysis_RegImm( OpCode op_code, RegisterUsage & recorder );
void StaticAnalysis_CoPro0( OpCode op_code, RegisterUsage & recorder );
void StaticAnalysis_CoPro1( OpCode op_code, RegisterUsage & recorder );
void StaticAnalysis_Cop0_TLB( OpCode op_code, RegisterUsage & recorder );
void StaticAnalysis_Cop1_BCInstr( OpCode op_code, RegisterUsage & recorder );
void StaticAnalysis_Cop1_SInstr( OpCode op_code, RegisterUsage & recorder );
void StaticAnalysis_Cop1_DInstr( OpCode op_code, RegisterUsage & recorder );

void StaticAnalysis_Unk( OpCode op_code, RegisterUsage & recorder )
{
}

// These are the only unimplemented R4300 instructions now:
void StaticAnalysis_LL( OpCode op_code, RegisterUsage & recorder ) {}
void StaticAnalysis_LLD( OpCode op_code, RegisterUsage & recorder ) {}

void StaticAnalysis_SC( OpCode op_code, RegisterUsage & recorder ) {}
void StaticAnalysis_SCD( OpCode op_code, RegisterUsage & recorder ) {}

void StaticAnalysis_LDC2( OpCode op_code, RegisterUsage & recorder ) {}
void StaticAnalysis_SDC2( OpCode op_code, RegisterUsage & recorder ) {}

void StaticAnalysis_RegImm_TGEI( OpCode op_code, RegisterUsage & recorder ) {}
void StaticAnalysis_RegImm_TGEIU( OpCode op_code, RegisterUsage & recorder ) {}
void StaticAnalysis_RegImm_TLTI( OpCode op_code, RegisterUsage & recorder ) {}
void StaticAnalysis_RegImm_TLTIU( OpCode op_code, RegisterUsage & recorder ) {}
void StaticAnalysis_RegImm_TEQI( OpCode op_code, RegisterUsage & recorder ) {}
void StaticAnalysis_RegImm_TNEI( OpCode op_code, RegisterUsage & recorder ) {}

void StaticAnalysis_RegImm_BLTZALL( OpCode op_code, RegisterUsage & recorder ) {}
void StaticAnalysis_RegImm_BGEZALL( OpCode op_code, RegisterUsage & recorder ) {}

void StaticAnalysis_Special_TGE( OpCode op_code, RegisterUsage & recorder ) {}
void StaticAnalysis_Special_TGEU( OpCode op_code, RegisterUsage & recorder ) {}
void StaticAnalysis_Special_TLT( OpCode op_code, RegisterUsage & recorder ) {}
void StaticAnalysis_Special_TLTU( OpCode op_code, RegisterUsage & recorder ) {}
void StaticAnalysis_Special_TEQ( OpCode op_code, RegisterUsage & recorder ) {}
void StaticAnalysis_Special_TNE( OpCode op_code, RegisterUsage & recorder ) {}


void StaticAnalysis_J( OpCode op_code, RegisterUsage & recorder ) 				// Jump
{
	// No registers used
}

void StaticAnalysis_JAL( OpCode op_code, RegisterUsage & recorder ) 				// Jump And Link
{
	recorder.Record( RegDstUse( N64Reg_RA ) );
}

void StaticAnalysis_BEQ( OpCode op_code, RegisterUsage & recorder ) 		// Branch on Equal
{
	recorder.Record( RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_BNE( OpCode op_code, RegisterUsage & recorder )             // Branch on Not Equal
{
	recorder.Record( RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_BLEZ( OpCode op_code, RegisterUsage & recorder ) 			// Branch on Less than of Equal to Zero
{
	recorder.Record( RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_BGTZ( OpCode op_code, RegisterUsage & recorder ) 			// Branch on Greater than Zero
{
	recorder.Record( RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_DADDI( OpCode op_code, RegisterUsage & recorder ) 			// Doubleword ADD Immediate
{
	recorder.Record( RegDstUse( op_code.rt ), RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_DADDIU( OpCode op_code, RegisterUsage & recorder ) 			// Doubleword ADD Immediate Unsigned
{
	recorder.Record( RegDstUse( op_code.rt ), RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_ADDI( OpCode op_code, RegisterUsage & recorder )
{
	recorder.Record( RegDstUse( op_code.rt ), RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_ADDIU( OpCode op_code, RegisterUsage & recorder ) 		// Add Immediate Unsigned
{
	recorder.Record( RegDstUse( op_code.rt ), RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_SLTI( OpCode op_code, RegisterUsage & recorder ) 			// Set on Less Than Immediate
{
	recorder.Record( RegDstUse( op_code.rt ), RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_SLTIU( OpCode op_code, RegisterUsage & recorder ) 		// Set on Less Than Immediate Unsigned
{
	recorder.Record( RegDstUse( op_code.rt ), RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_ANDI( OpCode op_code, RegisterUsage & recorder ) 				// AND Immediate
{
	recorder.Record( RegDstUse( op_code.rt ), RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_ORI( OpCode op_code, RegisterUsage & recorder ) 				// OR Immediate
{
	recorder.Record( RegDstUse( op_code.rt ), RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_XORI( OpCode op_code, RegisterUsage & recorder ) 				// XOR Immediate
{
	recorder.Record( RegDstUse( op_code.rt ), RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_LUI( OpCode op_code, RegisterUsage & recorder ) 				// Load Upper Immediate
{
	recorder.Record( RegDstUse( op_code.rt ), RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_BEQL( OpCode op_code, RegisterUsage & recorder ) 			// Branch on Equal Likely
{
	recorder.Record( RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_BNEL( OpCode op_code, RegisterUsage & recorder ) 			// Branch on Not Equal Likely
{
	recorder.Record( RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_BLEZL( OpCode op_code, RegisterUsage & recorder ) 		// Branch on Less than or Equal to Zero Likely
{
	recorder.Record( RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_BGTZL( OpCode op_code, RegisterUsage & recorder ) 		// Branch on Greater than Zero Likely
{
	recorder.Record( RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_LB( OpCode op_code, RegisterUsage & recorder ) 			// Load Byte
{
	recorder.Record( RegBaseUse( op_code.base ), RegDstUse( op_code.rt ) );
	//Should be safe to skip adding "op_code.offset" to check for inrage in RDRAM //Corn
	//recorder.Access( gCPUState.CPU[op_code.base]._u32_0 + op_code.offset ); // Don't do optimisation for LB, otherwise Mario 64 won't work :/
}

void StaticAnalysis_LBU( OpCode op_code, RegisterUsage & recorder ) 			// Load Byte Unsigned -- Zero extend byte...
{
	recorder.Record( RegBaseUse( op_code.base ), RegDstUse( op_code.rt ) );
	//Should be safe to skip adding "op_code.offset" to check for inrage in RDRAM //Corn
	if( !g_ROM.DISABLE_LBU_OPT ) recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );	//PD and Banjo dont like this
}

void StaticAnalysis_LH( OpCode op_code, RegisterUsage & recorder ) 		// Load Halfword
{
	recorder.Record( RegBaseUse( op_code.base ), RegDstUse( op_code.rt ) );
	//Should be safe to skip adding "op_code.offset" to check for inrage in RDRAM //Corn
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_LHU( OpCode op_code, RegisterUsage & recorder )			// Load Halfword Unsigned -- Zero extend word
{
	recorder.Record( RegBaseUse( op_code.base ), RegDstUse( op_code.rt ) );
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_LWL( OpCode op_code, RegisterUsage & recorder ) 			// Load Word Left
{
	recorder.Record( RegBaseUse( op_code.base ), RegDstUse( op_code.rt ) );
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_LDL( OpCode op_code, RegisterUsage & recorder )
{
	recorder.Record( RegBaseUse( op_code.base ), RegDstUse( op_code.rt ) );
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_LWR( OpCode op_code, RegisterUsage & recorder ) 			// Load Word Right
{
	recorder.Record( RegBaseUse( op_code.base ), RegDstUse( op_code.rt ) );
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_LDR( OpCode op_code, RegisterUsage & recorder )
{
	recorder.Record( RegBaseUse( op_code.base ), RegDstUse( op_code.rt ) );
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_LW( OpCode op_code, RegisterUsage & recorder ) 			// Load Word
{
	recorder.Record( RegBaseUse( op_code.base ), RegDstUse( op_code.rt ) );

	// Causes P Mario to BSOD in intro
	//Should be safe to skip adding "op_code.offset" to check for inrage in RDRAM //Corn
	if( g_ROM.GameHacks != PMARIO ) recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );	// Breaks Paper Mario
}

void StaticAnalysis_LWU( OpCode op_code, RegisterUsage & recorder ) 			// Load Word Unsigned
{
	recorder.Record( RegBaseUse( op_code.base ), RegDstUse( op_code.rt ) );
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_LD( OpCode op_code, RegisterUsage & recorder ) 				// Load Doubleword
{
	recorder.Record( RegBaseUse( op_code.base ), RegDstUse( op_code.rt ) );
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_SW( OpCode op_code, RegisterUsage & recorder ) 			// Store Word
{
	recorder.Record( RegBaseUse( op_code.base ), RegSrcUse( op_code.rt ) );
	//Should be safe to skip adding "op_code.offset" to check for inrage in RDRAM //Corn
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_SH( OpCode op_code, RegisterUsage & recorder ) 			// Store Halfword
{
	recorder.Record( RegBaseUse( op_code.base ), RegSrcUse( op_code.rt ) );

	// Causes Zelda MM to BSOD when you enter clock town
	if( g_ROM.GameHacks != ZELDA_MM ) recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_SB( OpCode op_code, RegisterUsage & recorder ) 			// Store Byte
{
	recorder.Record( RegBaseUse( op_code.base ), RegSrcUse( op_code.rt ) );
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_SWL( OpCode op_code, RegisterUsage & recorder ) 			// Store Word Left
{
	recorder.Record( RegBaseUse( op_code.base ), RegSrcUse( op_code.rt ) );
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_SWR( OpCode op_code, RegisterUsage & recorder ) 			// Store Word Right
{
	recorder.Record( RegBaseUse( op_code.base ), RegSrcUse( op_code.rt ) );
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_SD( OpCode op_code, RegisterUsage & recorder )			// Store Doubleword
{
	recorder.Record( RegBaseUse( op_code.base ), RegSrcUse( op_code.rt ) );
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_SDL( OpCode op_code, RegisterUsage & recorder )
{
	recorder.Record( RegBaseUse( op_code.base ), RegSrcUse( op_code.rt ) );
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_SDR( OpCode op_code, RegisterUsage & recorder )
{
	recorder.Record( RegBaseUse( op_code.base ), RegSrcUse( op_code.rt ) );
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_CACHE( OpCode op_code, RegisterUsage & recorder )
{
	recorder.Record( RegBaseUse( op_code.base ) );
}

void StaticAnalysis_LWC1( OpCode op_code, RegisterUsage & recorder ) 				// Load Word to Copro 1 (FPU)
{
	RegFPRWrite( op_code.ft );
	recorder.Record( RegBaseUse( op_code.base ) );
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_LDC1( OpCode op_code, RegisterUsage & recorder )				// Load Doubleword to Copro 1 (FPU)
{
	RegFPRWrite( op_code.ft );
	recorder.Record( RegBaseUse( op_code.base ) );
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_SWC1( OpCode op_code, RegisterUsage & recorder ) 			// Store Word From Copro 1
{
	RegFPRRead( op_code.ft );
	recorder.Record( RegBaseUse( op_code.base ) );
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

void StaticAnalysis_SDC1( OpCode op_code, RegisterUsage & recorder )		// Store Doubleword From Copro 1
{
	RegFPRRead( op_code.ft );
	recorder.Record( RegBaseUse( op_code.base ) );
	recorder.Access( gCPUState.CPU[op_code.base]._u32_0 );
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

void StaticAnalysis_Special_Unk( OpCode op_code, RegisterUsage & recorder )
{
	// Just ignore
}

void StaticAnalysis_Special_SLL( OpCode op_code, RegisterUsage & recorder ) 		// Shift word Left Logical
{
	if (op_code._u32 == 0)
		return;
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_SRL( OpCode op_code, RegisterUsage & recorder ) 		// Shift word Right Logical
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_SRA( OpCode op_code, RegisterUsage & recorder ) 		// Shift word Right Arithmetic
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_SLLV( OpCode op_code, RegisterUsage & recorder ) 		// Shift word Left Logical Variable
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_SRLV( OpCode op_code, RegisterUsage & recorder ) 		// Shift word Right Logical Variable
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_SRAV( OpCode op_code, RegisterUsage & recorder ) 		// Shift word Right Arithmetic Variable
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_JR( OpCode op_code, RegisterUsage & recorder ) 			// Jump Register
{
	recorder.Record( RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_Special_JALR( OpCode op_code, RegisterUsage & recorder ) 		// Jump and Link register
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_Special_SYSCALL( OpCode op_code, RegisterUsage & recorder )
{
}

void StaticAnalysis_Special_BREAK( OpCode op_code, RegisterUsage & recorder ) 	// BREAK
{
}

void StaticAnalysis_Special_SYNC( OpCode op_code, RegisterUsage & recorder )
{
}

void StaticAnalysis_Special_MFHI( OpCode op_code, RegisterUsage & recorder ) 			// Move From MultHI
{
	recorder.Record( RegDstUse( op_code.rd ) );
}

void StaticAnalysis_Special_MTHI( OpCode op_code, RegisterUsage & recorder ) 			// Move To MultHI
{
	recorder.Record( RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_Special_MFLO( OpCode op_code, RegisterUsage & recorder ) 			// Move From MultLO
{
	recorder.Record( RegDstUse( op_code.rd ) );
}

void StaticAnalysis_Special_MTLO( OpCode op_code, RegisterUsage & recorder ) 			// Move To MultLO
{
	recorder.Record( RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_Special_DSLLV( OpCode op_code, RegisterUsage & recorder )
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_DSRLV( OpCode op_code, RegisterUsage & recorder )
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_DSRAV( OpCode op_code, RegisterUsage & recorder )
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_MULT( OpCode op_code, RegisterUsage & recorder ) 			// MULTiply Signed
{
	recorder.Record( RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_MULTU( OpCode op_code, RegisterUsage & recorder ) 		// MULTiply Unsigned
{
	recorder.Record( RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_DIV( OpCode op_code, RegisterUsage & recorder ) 			//DIVide
{
	recorder.Record( RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_DIVU( OpCode op_code, RegisterUsage & recorder ) 			// DIVide Unsigned
{
	recorder.Record( RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_DMULT( OpCode op_code, RegisterUsage & recorder ) 		// Double Multiply
{
	recorder.Record( RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_DMULTU( OpCode op_code, RegisterUsage & recorder ) 			// Double Multiply Unsigned
{
	recorder.Record( RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_DDIV( OpCode op_code, RegisterUsage & recorder ) 				// Double Divide
{
	recorder.Record( RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_DDIVU( OpCode op_code, RegisterUsage & recorder ) 			// Double Divide Unsigned
{
	recorder.Record( RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_ADD( OpCode op_code, RegisterUsage & recorder ) 			// ADD signed - may throw exception
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_ADDU( OpCode op_code, RegisterUsage & recorder ) 			// ADD Unsigned - doesn't throw exception
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_SUB( OpCode op_code, RegisterUsage & recorder ) 			// SUB Signed - may throw exception
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_SUBU( OpCode op_code, RegisterUsage & recorder ) 			// SUB Unsigned - doesn't throw exception
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_AND( OpCode op_code, RegisterUsage & recorder ) 				// logical AND
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_OR( OpCode op_code, RegisterUsage & recorder ) 				// logical OR
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_XOR( OpCode op_code, RegisterUsage & recorder ) 				// logical XOR
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_NOR( OpCode op_code, RegisterUsage & recorder ) 				// logical Not OR
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_SLT( OpCode op_code, RegisterUsage & recorder ) 				// Set on Less Than
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_SLTU( OpCode op_code, RegisterUsage & recorder ) 				// Set on Less Than Unsigned
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_DADD( OpCode op_code, RegisterUsage & recorder )//CYRUS64
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_DADDU( OpCode op_code, RegisterUsage & recorder )//CYRUS64
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_DSUB( OpCode op_code, RegisterUsage & recorder )
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_DSUBU( OpCode op_code, RegisterUsage & recorder )
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rs ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_DSLL( OpCode op_code, RegisterUsage & recorder )
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_DSRL( OpCode op_code, RegisterUsage & recorder )
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_DSRA( OpCode op_code, RegisterUsage & recorder )
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_DSLL32( OpCode op_code, RegisterUsage & recorder ) 			// Double Shift Left Logical 32
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_DSRL32( OpCode op_code, RegisterUsage & recorder ) 			// Double Shift Right Logical 32
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Special_DSRA32( OpCode op_code, RegisterUsage & recorder ) 			// Double Shift Right Arithmetic 32
{
	recorder.Record( RegDstUse( op_code.rd ), RegSrcUse( op_code.rt ) );
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

void StaticAnalysis_RegImm_BLTZ( OpCode op_code, RegisterUsage & recorder ) 			// Branch on Less than Zero
{
	recorder.Record( RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_RegImm_BLTZL( OpCode op_code, RegisterUsage & recorder ) 			// Branch on Less than Zero Likely
{
	recorder.Record( RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_RegImm_BLTZAL( OpCode op_code, RegisterUsage & recorder ) 		// Branch on Less than Zero And Link
{
	recorder.Record( RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_RegImm_BGEZ( OpCode op_code, RegisterUsage & recorder ) 			// Branch on Greater than or Equal to Zero
{
	recorder.Record( RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_RegImm_BGEZL( OpCode op_code, RegisterUsage & recorder ) 			// Branch on Greater than or Equal to Zero Likely
{
	recorder.Record( RegSrcUse( op_code.rs ) );
}

void StaticAnalysis_RegImm_BGEZAL( OpCode op_code, RegisterUsage & recorder ) 		// Branch on Greater than or Equal to Zero And Link
{
	recorder.Record( RegSrcUse( op_code.rs ) );
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

void StaticAnalysis_Cop0_MFC0( OpCode op_code, RegisterUsage & recorder )
{
	recorder.Record( RegDstUse( op_code.rt ) );
}

// Move Word To CopReg
void StaticAnalysis_Cop0_MTC0( OpCode op_code, RegisterUsage & recorder )
{
	recorder.Record( RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_TLB_TLBR( OpCode op_code, RegisterUsage & recorder ) 				// TLB Read
{
}

void StaticAnalysis_TLB_TLBWI( OpCode op_code, RegisterUsage & recorder )			// TLB Write Index
{
}

void StaticAnalysis_TLB_TLBWR( OpCode op_code, RegisterUsage & recorder )
{
}

void StaticAnalysis_TLB_TLBP( OpCode op_code, RegisterUsage & recorder ) 				// TLB Probe
{
}

void StaticAnalysis_TLB_ERET( OpCode op_code, RegisterUsage & recorder )
{
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

void StaticAnalysis_Cop1_MTC1( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fs );
	recorder.Record( RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Cop1_DMTC1( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fs );
	recorder.Record( RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_Cop1_MFC1( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRRead( op_code.fs );
	recorder.Record( RegDstUse( op_code.rt ) );
}

void StaticAnalysis_Cop1_DMFC1( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRRead( op_code.fs );
	recorder.Record( RegDstUse( op_code.rt ) );
}

void StaticAnalysis_Cop1_CFC1( OpCode op_code, RegisterUsage & recorder ) 		// move Control word From Copro 1
{
	recorder.Record( RegDstUse( op_code.rt ) );
}

void StaticAnalysis_Cop1_CTC1( OpCode op_code, RegisterUsage & recorder ) 		// move Control word To Copro 1
{
	recorder.Record( RegSrcUse( op_code.rt ) );
}

void StaticAnalysis_BC1_BC1F( OpCode op_code, RegisterUsage & recorder )		// Branch on FPU False
{
}

void StaticAnalysis_BC1_BC1T( OpCode op_code, RegisterUsage & recorder )	// Branch on FPU True
{
}

void StaticAnalysis_BC1_BC1FL( OpCode op_code, RegisterUsage & recorder )	// Branch on FPU False Likely
{
}

void StaticAnalysis_BC1_BC1TL( OpCode op_code, RegisterUsage & recorder )		// Branch on FPU True Likely
{
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// WORD FP Instrs /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

void StaticAnalysis_Cop1_W_CVT_S( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_W_CVT_D( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_WInstr( OpCode op_code, RegisterUsage & recorder )
{
	switch ( op_code.cop1_funct )
	{
		case Cop1OpFunc_CVT_S:
			StaticAnalysis_Cop1_W_CVT_S( op_code, recorder );
			return;
		case Cop1OpFunc_CVT_D:
			StaticAnalysis_Cop1_W_CVT_D( op_code, recorder );
			return;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// LONG FP Instrs /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

void StaticAnalysis_Cop1_L_CVT_S( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_L_CVT_D( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_LInstr( OpCode op_code, RegisterUsage & recorder )
{
	switch ( op_code.cop1_funct )
	{
		case Cop1OpFunc_CVT_S:
			StaticAnalysis_Cop1_L_CVT_S( op_code, recorder );
			return;
		case Cop1OpFunc_CVT_D:
			StaticAnalysis_Cop1_L_CVT_D( op_code, recorder );
			return;
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// Single FP Instrs //////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

void StaticAnalysis_Cop1_S_ADD( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
	RegFPRRead( op_code.ft );
}

void StaticAnalysis_Cop1_S_SUB( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
	RegFPRRead( op_code.ft );
}

void StaticAnalysis_Cop1_S_MUL( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
	RegFPRRead( op_code.ft );
}

void StaticAnalysis_Cop1_S_DIV( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
	RegFPRRead( op_code.ft );
}

void StaticAnalysis_Cop1_S_SQRT( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_S_NEG( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_S_MOV( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_S_ABS( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_S_TRUNC_W( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_S_TRUNC_L( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_S_ROUND_W( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_S_ROUND_L( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_S_CEIL_W( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_S_CEIL_L( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_S_FLOOR_W( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_S_FLOOR_L( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_S_CVT_L( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_S_CVT_W( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_S_CVT_D( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_S_EQ( OpCode op_code, RegisterUsage & recorder ) 				// Compare for Equality
{
	RegFPRRead( op_code.fs );
	RegFPRRead( op_code.ft );
}

void StaticAnalysis_Cop1_S_LT( OpCode op_code, RegisterUsage & recorder ) 				// Compare for Equality
{
	RegFPRRead( op_code.fs );
	RegFPRRead( op_code.ft );
}

void StaticAnalysis_Cop1_Compare_S( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRRead( op_code.fs );
	RegFPRRead( op_code.ft );
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

void StaticAnalysis_Cop1_D_ADD( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
	RegFPRRead( op_code.ft );
}

void StaticAnalysis_Cop1_D_SUB( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
	RegFPRRead( op_code.ft );
}

void StaticAnalysis_Cop1_D_MUL( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
	RegFPRRead( op_code.ft );
}

void StaticAnalysis_Cop1_D_DIV( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
	RegFPRRead( op_code.ft );
}

void StaticAnalysis_Cop1_D_ABS( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_D_SQRT( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_D_NEG( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_D_MOV( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_D_TRUNC_W( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_D_TRUNC_L( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_D_ROUND_W( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_D_ROUND_L( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_D_CEIL_W( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_D_CEIL_L( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_D_FLOOR_W( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_D_FLOOR_L( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_D_CVT_S( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_D_CVT_W( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_D_CVT_L( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRWrite( op_code.fd );
	RegFPRRead( op_code.fs );
}

void StaticAnalysis_Cop1_Compare_D( OpCode op_code, RegisterUsage & recorder )
{
	RegFPRRead( op_code.fs );
	RegFPRRead( op_code.ft );
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

//*************************************************************************************
// Opcode Jump Table
//*************************************************************************************
StaticAnalysisFunction gStaticAnalysisInstruction[64] =
{
	StaticAnalysis_Special, StaticAnalysis_RegImm, StaticAnalysis_J, StaticAnalysis_JAL, StaticAnalysis_BEQ, StaticAnalysis_BNE, StaticAnalysis_BLEZ, StaticAnalysis_BGTZ,
	StaticAnalysis_ADDI, StaticAnalysis_ADDIU, StaticAnalysis_SLTI, StaticAnalysis_SLTIU, StaticAnalysis_ANDI, StaticAnalysis_ORI, StaticAnalysis_XORI, StaticAnalysis_LUI,
	StaticAnalysis_CoPro0, StaticAnalysis_CoPro1, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_BEQL, StaticAnalysis_BNEL, StaticAnalysis_BLEZL, StaticAnalysis_BGTZL,
	StaticAnalysis_DADDI, StaticAnalysis_DADDIU, StaticAnalysis_LDL, StaticAnalysis_LDR, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk,
	StaticAnalysis_LB, StaticAnalysis_LH, StaticAnalysis_LWL, StaticAnalysis_LW, StaticAnalysis_LBU, StaticAnalysis_LHU, StaticAnalysis_LWR, StaticAnalysis_LWU,
	StaticAnalysis_SB, StaticAnalysis_SH, StaticAnalysis_SWL, StaticAnalysis_SW, StaticAnalysis_SDL, StaticAnalysis_SDR, StaticAnalysis_SWR, StaticAnalysis_CACHE,
	StaticAnalysis_LL, StaticAnalysis_LWC1, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_LLD, StaticAnalysis_LDC1, StaticAnalysis_LDC2, StaticAnalysis_LD,
	StaticAnalysis_SC, StaticAnalysis_SWC1, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_SCD, StaticAnalysis_SDC1, StaticAnalysis_SDC2, StaticAnalysis_SD
};

//*************************************************************************************
// SpecialOpCode Jump Table
//*************************************************************************************
StaticAnalysisFunction gStaticAnalysisSpecialInstruction[64] =
{
	StaticAnalysis_Special_SLL, StaticAnalysis_Special_Unk, StaticAnalysis_Special_SRL, StaticAnalysis_Special_SRA, StaticAnalysis_Special_SLLV, StaticAnalysis_Special_Unk, StaticAnalysis_Special_SRLV, StaticAnalysis_Special_SRAV,
	StaticAnalysis_Special_JR, StaticAnalysis_Special_JALR, StaticAnalysis_Special_Unk, StaticAnalysis_Special_Unk, StaticAnalysis_Special_SYSCALL, StaticAnalysis_Special_BREAK, StaticAnalysis_Special_Unk, StaticAnalysis_Special_SYNC,
	StaticAnalysis_Special_MFHI, StaticAnalysis_Special_MTHI, StaticAnalysis_Special_MFLO, StaticAnalysis_Special_MTLO, StaticAnalysis_Special_DSLLV, StaticAnalysis_Special_Unk, StaticAnalysis_Special_DSRLV, StaticAnalysis_Special_DSRAV,
	StaticAnalysis_Special_MULT, StaticAnalysis_Special_MULTU, StaticAnalysis_Special_DIV, StaticAnalysis_Special_DIVU, StaticAnalysis_Special_DMULT, StaticAnalysis_Special_DMULTU, StaticAnalysis_Special_DDIV, StaticAnalysis_Special_DDIVU,
	StaticAnalysis_Special_ADD, StaticAnalysis_Special_ADDU, StaticAnalysis_Special_SUB, StaticAnalysis_Special_SUBU, StaticAnalysis_Special_AND, StaticAnalysis_Special_OR, StaticAnalysis_Special_XOR, StaticAnalysis_Special_NOR,
	StaticAnalysis_Special_Unk, StaticAnalysis_Special_Unk, StaticAnalysis_Special_SLT, StaticAnalysis_Special_SLTU, StaticAnalysis_Special_DADD, StaticAnalysis_Special_DADDU, StaticAnalysis_Special_DSUB, StaticAnalysis_Special_DSUBU,
	StaticAnalysis_Special_TGE, StaticAnalysis_Special_TGEU, StaticAnalysis_Special_TLT, StaticAnalysis_Special_TLTU, StaticAnalysis_Special_TEQ, StaticAnalysis_Special_Unk, StaticAnalysis_Special_TNE, StaticAnalysis_Special_Unk,
	StaticAnalysis_Special_DSLL, StaticAnalysis_Special_Unk, StaticAnalysis_Special_DSRL, StaticAnalysis_Special_DSRA, StaticAnalysis_Special_DSLL32, StaticAnalysis_Special_Unk, StaticAnalysis_Special_DSRL32, StaticAnalysis_Special_DSRA32
};

void StaticAnalysis_Special( OpCode op_code, RegisterUsage & recorder )
{
	gStaticAnalysisSpecialInstruction[ op_code.spec_op ]( op_code, recorder );
}

//*************************************************************************************
//
//*************************************************************************************
StaticAnalysisFunction gStaticAnalysisRegImmInstruction[32] =
{

	StaticAnalysis_RegImm_BLTZ,   StaticAnalysis_RegImm_BGEZ,   StaticAnalysis_RegImm_BLTZL,   StaticAnalysis_RegImm_BGEZL,   StaticAnalysis_Unk,  StaticAnalysis_Unk, StaticAnalysis_Unk,  StaticAnalysis_Unk,
	StaticAnalysis_RegImm_TGEI,   StaticAnalysis_RegImm_TGEIU,  StaticAnalysis_RegImm_TLTI,    StaticAnalysis_RegImm_TLTIU,   StaticAnalysis_RegImm_TEQI, StaticAnalysis_Unk, StaticAnalysis_RegImm_TNEI, StaticAnalysis_Unk,
	StaticAnalysis_RegImm_BLTZAL, StaticAnalysis_RegImm_BGEZAL, StaticAnalysis_RegImm_BLTZALL, StaticAnalysis_RegImm_BGEZALL, StaticAnalysis_Unk,  StaticAnalysis_Unk, StaticAnalysis_Unk,  StaticAnalysis_Unk,
	StaticAnalysis_Unk,    StaticAnalysis_Unk,    StaticAnalysis_Unk,     StaticAnalysis_Unk,     StaticAnalysis_Unk,  StaticAnalysis_Unk, StaticAnalysis_Unk,  StaticAnalysis_Unk
};

void StaticAnalysis_RegImm( OpCode op_code, RegisterUsage & recorder )
{
	gStaticAnalysisRegImmInstruction[ op_code.regimm_op ]( op_code, recorder );
}

//*************************************************************************************
// COP0 Jump Table
//*************************************************************************************
StaticAnalysisFunction gStaticAnalysisCop0Instruction[32] =
{
	StaticAnalysis_Cop0_MFC0, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Cop0_MTC0, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk,
	StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk,
	StaticAnalysis_Cop0_TLB, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk,
	StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk,
};

void StaticAnalysis_CoPro0( OpCode op_code, RegisterUsage & recorder )
{
	gStaticAnalysisCop0Instruction[ op_code.cop0_op ]( op_code, recorder );
}

//*************************************************************************************
// TLBOpCode Jump Table
//*************************************************************************************
StaticAnalysisFunction gStaticAnalysisTLBInstruction[64] =
{
	StaticAnalysis_Unk, StaticAnalysis_TLB_TLBR, StaticAnalysis_TLB_TLBWI, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_TLB_TLBWR, StaticAnalysis_Unk,
	StaticAnalysis_TLB_TLBP, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk,
	StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk,
	StaticAnalysis_TLB_ERET, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk,
	StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk,
	StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk,
	StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk,
	StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk,
};

void StaticAnalysis_Cop0_TLB( OpCode op_code, RegisterUsage & recorder )
{
	gStaticAnalysisTLBInstruction[ op_code.cop0tlb_funct ]( op_code, recorder );
}


//*************************************************************************************
// COP1 Jump Table
//*************************************************************************************
StaticAnalysisFunction gStaticAnalysisCop1Instruction[32] =
{
	StaticAnalysis_Cop1_MFC1,    StaticAnalysis_Cop1_DMFC1,  StaticAnalysis_Cop1_CFC1, StaticAnalysis_Unk, StaticAnalysis_Cop1_MTC1,   StaticAnalysis_Cop1_DMTC1,  StaticAnalysis_Cop1_CTC1, StaticAnalysis_Unk,
	StaticAnalysis_Cop1_BCInstr, StaticAnalysis_Unk,    StaticAnalysis_Unk,  StaticAnalysis_Unk, StaticAnalysis_Unk,    StaticAnalysis_Unk,    StaticAnalysis_Unk,  StaticAnalysis_Unk,
	StaticAnalysis_Cop1_SInstr,  StaticAnalysis_Cop1_DInstr, StaticAnalysis_Unk,  StaticAnalysis_Unk, StaticAnalysis_Cop1_WInstr, StaticAnalysis_Cop1_LInstr, StaticAnalysis_Unk,  StaticAnalysis_Unk,
	StaticAnalysis_Unk,     StaticAnalysis_Unk,    StaticAnalysis_Unk,  StaticAnalysis_Unk, StaticAnalysis_Unk,    StaticAnalysis_Unk,    StaticAnalysis_Unk,  StaticAnalysis_Unk
};

void StaticAnalysis_CoPro1( OpCode op_code, RegisterUsage & recorder )
{
	gStaticAnalysisCop1Instruction[ op_code.cop1_op ]( op_code, recorder );
}

//*************************************************************************************
//
//*************************************************************************************
StaticAnalysisFunction gStaticAnalysisCop1BC1Instruction[4] =
{
	StaticAnalysis_BC1_BC1F, StaticAnalysis_BC1_BC1T, StaticAnalysis_BC1_BC1FL, StaticAnalysis_BC1_BC1TL
};

void StaticAnalysis_Cop1_BCInstr( OpCode op_code, RegisterUsage & recorder )
{
	gStaticAnalysisCop1BC1Instruction[ op_code.cop1_bc ]( op_code, recorder );
}

//*************************************************************************************
// Single Jump Table
//*************************************************************************************
StaticAnalysisFunction gStaticAnalysisCop1SInstruction[64] =
{
	StaticAnalysis_Cop1_S_ADD,     StaticAnalysis_Cop1_S_SUB,     StaticAnalysis_Cop1_S_MUL,    StaticAnalysis_Cop1_S_DIV,     StaticAnalysis_Cop1_S_SQRT,    StaticAnalysis_Cop1_S_ABS,     StaticAnalysis_Cop1_S_MOV,    StaticAnalysis_Cop1_S_NEG,
	StaticAnalysis_Cop1_S_ROUND_L, StaticAnalysis_Cop1_S_TRUNC_L,	StaticAnalysis_Cop1_S_CEIL_L, StaticAnalysis_Cop1_S_FLOOR_L, StaticAnalysis_Cop1_S_ROUND_W, StaticAnalysis_Cop1_S_TRUNC_W, StaticAnalysis_Cop1_S_CEIL_W, StaticAnalysis_Cop1_S_FLOOR_W,
	StaticAnalysis_Unk,     StaticAnalysis_Unk,     StaticAnalysis_Unk,    StaticAnalysis_Unk,     StaticAnalysis_Unk,     StaticAnalysis_Unk,     StaticAnalysis_Unk,    StaticAnalysis_Unk,
	StaticAnalysis_Unk,     StaticAnalysis_Unk,     StaticAnalysis_Unk,    StaticAnalysis_Unk,     StaticAnalysis_Unk,     StaticAnalysis_Unk,     StaticAnalysis_Unk,    StaticAnalysis_Unk,
	StaticAnalysis_Unk,     StaticAnalysis_Cop1_S_CVT_D,   StaticAnalysis_Unk,    StaticAnalysis_Unk,     StaticAnalysis_Cop1_S_CVT_W,   StaticAnalysis_Cop1_S_CVT_L,   StaticAnalysis_Unk,    StaticAnalysis_Unk,
	StaticAnalysis_Unk,     StaticAnalysis_Unk,     StaticAnalysis_Unk,    StaticAnalysis_Unk,     StaticAnalysis_Unk,     StaticAnalysis_Unk,     StaticAnalysis_Unk,    StaticAnalysis_Unk,
	StaticAnalysis_Cop1_Compare_S,  StaticAnalysis_Cop1_Compare_S, StaticAnalysis_Cop1_Compare_S, StaticAnalysis_Cop1_Compare_S, StaticAnalysis_Cop1_Compare_S,     StaticAnalysis_Cop1_Compare_S,     StaticAnalysis_Cop1_Compare_S,    StaticAnalysis_Cop1_Compare_S,
	StaticAnalysis_Cop1_Compare_S,  StaticAnalysis_Cop1_Compare_S, StaticAnalysis_Cop1_Compare_S, StaticAnalysis_Cop1_Compare_S, StaticAnalysis_Cop1_Compare_S,      StaticAnalysis_Cop1_Compare_S,     StaticAnalysis_Cop1_Compare_S,     StaticAnalysis_Cop1_Compare_S,
};

void StaticAnalysis_Cop1_SInstr( OpCode op_code, RegisterUsage & recorder )
{
	gStaticAnalysisCop1SInstruction[ op_code.cop1_funct ]( op_code, recorder );
}

//*************************************************************************************
// Double Jump Table
//*************************************************************************************
StaticAnalysisFunction gStaticAnalysisCop1DInstruction[64] =
{
	StaticAnalysis_Cop1_D_ADD,     StaticAnalysis_Cop1_D_SUB,     StaticAnalysis_Cop1_D_MUL, StaticAnalysis_Cop1_D_DIV, StaticAnalysis_Cop1_D_SQRT, StaticAnalysis_Cop1_D_ABS, StaticAnalysis_Cop1_D_MOV, StaticAnalysis_Cop1_D_NEG,
	StaticAnalysis_Cop1_D_ROUND_L, StaticAnalysis_Cop1_D_TRUNC_L, StaticAnalysis_Cop1_D_CEIL_L, StaticAnalysis_Cop1_D_FLOOR_L, StaticAnalysis_Cop1_D_ROUND_W, StaticAnalysis_Cop1_D_TRUNC_W, StaticAnalysis_Cop1_D_CEIL_W, StaticAnalysis_Cop1_D_FLOOR_W,
	StaticAnalysis_Unk,		StaticAnalysis_Unk,     StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk,
	StaticAnalysis_Unk,     StaticAnalysis_Unk,     StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk,
	StaticAnalysis_Cop1_D_CVT_S,   StaticAnalysis_Unk,     StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Cop1_D_CVT_W, StaticAnalysis_Cop1_D_CVT_L, StaticAnalysis_Unk, StaticAnalysis_Unk,
	StaticAnalysis_Unk,     StaticAnalysis_Unk,     StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk, StaticAnalysis_Unk,
	StaticAnalysis_Cop1_Compare_D,  StaticAnalysis_Cop1_Compare_D, StaticAnalysis_Cop1_Compare_D, StaticAnalysis_Cop1_Compare_D, StaticAnalysis_Cop1_Compare_D,     StaticAnalysis_Cop1_Compare_D,     StaticAnalysis_Cop1_Compare_D,    StaticAnalysis_Cop1_Compare_D,
	StaticAnalysis_Cop1_Compare_D,  StaticAnalysis_Cop1_Compare_D, StaticAnalysis_Cop1_Compare_D, StaticAnalysis_Cop1_Compare_D, StaticAnalysis_Cop1_Compare_D,      StaticAnalysis_Cop1_Compare_D,     StaticAnalysis_Cop1_Compare_D,     StaticAnalysis_Cop1_Compare_D,
};

void StaticAnalysis_Cop1_DInstr( OpCode op_code, RegisterUsage & recorder )
{
	gStaticAnalysisCop1DInstruction[ op_code.cop1_funct ]( op_code, recorder );
}


}

namespace StaticAnalysis
{

void		Analyse( OpCode op_code, RegisterUsage & reg_usage )
{
	gStaticAnalysisInstruction[ op_code.op ]( op_code, reg_usage );
}

}
