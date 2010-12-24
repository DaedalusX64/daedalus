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

//
// Important Note:
//	Various chunks of this code/data are derived from Zilmar's RSP plugin,
//	which has also been used as a reference for fixing bugs in original
//	code. Many thanks to Zilmar for releasing the source to this!
//

// RSP Processor stuff
#include "stdafx.h"

#include "CPU.h"
#include "Interrupt.h"
#include "Memory.h"
#include "Registers.h"
#include "R4300.h"
#include "RSP.h"

#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"

#include "OSHLE/ultra_rcp.h"
#include "OSHLE/ultra_R4300.h"

#define RSP_ASSERT_Q( e )			DAEDALUS_ASSERT_Q( e )

volatile bool gRSPHLEActive = false;

//*****************************************************************************
//
//*****************************************************************************
// We need similar registers to the main CPU
ALIGNED_GLOBAL(SRSPState, gRSPState, CACHE_ALIGN);

#define gRegRSP		gRSPState.CPU
#define gVectRSP	gRSPState.COP2
#define gAccuRSP	gRSPState.Accumulator
#define gFlagsRSP	gRSPState.COP2Control
#define RSPPC		gRSPState.CurrentPC
#define g_nRSPDelay	gRSPState.Delay
#define g_dwNewRSPPC gRSPState.TargetPC


REG64 EleSpec[32], Indx[32];


//*****************************************************************************
//
//*****************************************************************************
typedef void (*RSPInstruction)( OpCode op );


static void RSP_Unk( OpCode op );
static void RSP_Special( OpCode op );
static void RSP_RegImm( OpCode op );
static void RSP_J( OpCode op );
static void RSP_JAL( OpCode op );
static void RSP_BEQ( OpCode op );
static void RSP_BNE( OpCode op );
static void RSP_BLEZ( OpCode op );
static void RSP_BGTZ( OpCode op );
static void RSP_ADDI( OpCode op );
static void RSP_ADDIU( OpCode op );
static void RSP_SLTI( OpCode op );
static void RSP_SLTIU( OpCode op );
static void RSP_ANDI( OpCode op );
static void RSP_ORI( OpCode op );
static void RSP_XORI( OpCode op );
static void RSP_LUI( OpCode op );
static void RSP_CoPro0( OpCode op );
static void RSP_CoPro2( OpCode op );
static void RSP_LB( OpCode op );
static void RSP_LH( OpCode op );
static void RSP_LW( OpCode op );
static void RSP_LBU( OpCode op );
static void RSP_LHU( OpCode op );
static void RSP_SB( OpCode op );
static void RSP_SH( OpCode op );
static void RSP_SW( OpCode op );
static void RSP_LWC2( OpCode op );
static void RSP_SWC2( OpCode op );


static void RSP_Special_Unk( OpCode op );
static void RSP_Special_SLL( OpCode op );
static void RSP_Special_SRL( OpCode op );
static void RSP_Special_SRA( OpCode op );
static void RSP_Special_SLLV( OpCode op );
static void RSP_Special_SRLV( OpCode op );
static void RSP_Special_SRAV( OpCode op );
static void RSP_Special_JR( OpCode op );
static void RSP_Special_JALR( OpCode op );
static void RSP_Special_BREAK( OpCode op );
static void RSP_Special_ADD( OpCode op );
static void RSP_Special_ADDU( OpCode op );
static void RSP_Special_SUB( OpCode op );
static void RSP_Special_SUBU( OpCode op );
static void RSP_Special_AND( OpCode op );
static void RSP_Special_OR( OpCode op );
static void RSP_Special_XOR( OpCode op );
static void RSP_Special_NOR( OpCode op );
static void RSP_Special_SLT( OpCode op );
static void RSP_Special_SLTU( OpCode op );

static void RSP_RegImm_Unk( OpCode op );
static void RSP_RegImm_BLTZ( OpCode op );
static void RSP_RegImm_BGEZ( OpCode op );
static void RSP_RegImm_BLTZAL( OpCode op );
static void RSP_RegImm_BGEZAL( OpCode op );


static void RSP_Cop0_Unk( OpCode op );
static void RSP_Cop0_MFC0( OpCode op );
static void RSP_Cop0_MTC0( OpCode op );

static void RSP_LWC2_Unk( OpCode op );
static void RSP_LWC2_LBV( OpCode op );
static void RSP_LWC2_LSV( OpCode op );
static void RSP_LWC2_LLV( OpCode op );
static void RSP_LWC2_LDV( OpCode op );
static void RSP_LWC2_LQV( OpCode op );
static void RSP_LWC2_LRV( OpCode op );
static void RSP_LWC2_LPV( OpCode op );
static void RSP_LWC2_LUV( OpCode op );
static void RSP_LWC2_LHV( OpCode op );
static void RSP_LWC2_LFV( OpCode op );
static void RSP_LWC2_LWV( OpCode op );
static void RSP_LWC2_LTV( OpCode op );

static void RSP_SWC2_Unk( OpCode op );
static void RSP_SWC2_SBV( OpCode op );
static void RSP_SWC2_SSV( OpCode op );
static void RSP_SWC2_SLV( OpCode op );
static void RSP_SWC2_SDV( OpCode op );
static void RSP_SWC2_SQV( OpCode op );
static void RSP_SWC2_SRV( OpCode op );
static void RSP_SWC2_SPV( OpCode op );
static void RSP_SWC2_SUV( OpCode op );
static void RSP_SWC2_SHV( OpCode op );
static void RSP_SWC2_SFV( OpCode op );
static void RSP_SWC2_SWV( OpCode op );
static void RSP_SWC2_STV( OpCode op );

static void RSP_Cop2_Unk( OpCode op );
static void RSP_Cop2_MFC2( OpCode op );
static void RSP_Cop2_CFC2( OpCode op );
static void RSP_Cop2_MTC2( OpCode op );
static void RSP_Cop2_CTC2( OpCode op );
static void RSP_Cop2_Vector( OpCode op );

static void RSP_Cop2V_Unk( OpCode op );
static void RSP_Cop2V_VMULF( OpCode op );
static void RSP_Cop2V_VMULU( OpCode op );
static void RSP_Cop2V_VRNDP( OpCode op );
static void RSP_Cop2V_VMULQ( OpCode op );
static void RSP_Cop2V_VMUDL( OpCode op );
static void RSP_Cop2V_VMUDM( OpCode op );
static void RSP_Cop2V_VMUDN( OpCode op );
static void RSP_Cop2V_VMUDH( OpCode op );
static void RSP_Cop2V_VMACF( OpCode op );
static void RSP_Cop2V_VMACU( OpCode op );
static void RSP_Cop2V_VRNDN( OpCode op );
static void RSP_Cop2V_VMACQ( OpCode op );
static void RSP_Cop2V_VMADL( OpCode op );
static void RSP_Cop2V_VMADM( OpCode op );
static void RSP_Cop2V_VMADN( OpCode op );
static void RSP_Cop2V_VMADH( OpCode op );
static void RSP_Cop2V_VADD( OpCode op );
static void RSP_Cop2V_VSUB( OpCode op );
static void RSP_Cop2V_VSUT( OpCode op );
static void RSP_Cop2V_VABS( OpCode op );
static void RSP_Cop2V_VADDC( OpCode op );
static void RSP_Cop2V_VSUBC( OpCode op );
static void RSP_Cop2V_VADDB( OpCode op );
static void RSP_Cop2V_VSUBB( OpCode op );
static void RSP_Cop2V_VACCB( OpCode op );
static void RSP_Cop2V_VSUCB( OpCode op );
static void RSP_Cop2V_VSAD( OpCode op );
static void RSP_Cop2V_VSAC( OpCode op );
static void RSP_Cop2V_VSUM( OpCode op );
static void RSP_Cop2V_VSAW( OpCode op );
static void RSP_Cop2V_VLT( OpCode op );
static void RSP_Cop2V_VEQ( OpCode op );
static void RSP_Cop2V_VNE( OpCode op );
static void RSP_Cop2V_VGE( OpCode op );
static void RSP_Cop2V_VCL( OpCode op );
static void RSP_Cop2V_VCH( OpCode op );
static void RSP_Cop2V_VCR( OpCode op );
static void RSP_Cop2V_VMRG( OpCode op );
static void RSP_Cop2V_VAND( OpCode op );
static void RSP_Cop2V_VNAND( OpCode op );
static void RSP_Cop2V_VOR( OpCode op );
static void RSP_Cop2V_VNOR( OpCode op );
static void RSP_Cop2V_VXOR( OpCode op );
static void RSP_Cop2V_VNXOR( OpCode op );
static void RSP_Cop2V_VRCP( OpCode op );
static void RSP_Cop2V_VRCPL( OpCode op );
static void RSP_Cop2V_VRCPH( OpCode op );
static void RSP_Cop2V_VMOV( OpCode op );
static void RSP_Cop2V_VRSQ( OpCode op );
static void RSP_Cop2V_VRSQL( OpCode op );
static void RSP_Cop2V_VRSQH( OpCode op );



/*
    CPU: Instructions encoded by opcode field.
    31---------26---------------------------------------------------0
    |  opcode   |                                                   |
    ------6----------------------------------------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
000 | *1    | *2    |   J   |  JAL  |  BEQ  |  BNE  | BLEZ  | BGTZ  |
001 | ADDI  | ADDIU | SLTI  | SLTIU | ANDI  |  ORI  | XORI  |  LUI  |
010 | *3    |  ---  |  *4   |  ---  |  ---  |  ---  |  ---  |  ---  |
011 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
100 |  LB   |  LH   |  ---  |  LW   |  LBU  |  LHU  |  ---  |  ---  |
101 |  SB   |  SH   |  ---  |  SW   |  ---  |  ---  |  ---  |  ---  |
110 |  ---  |  ---  | *LWC2 |  ---  |  ---  |  ---  |  ---  |  ---  |
111 |  ---  |  ---  | *SWC2 |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
     *1 = SPECIAL, see SPECIAL list    *2 = REGIMM, see REGIMM list
     *3 = COP0                         *4 = COP2
     *LWC2 = RSP Load instructions     *SWC2 = RSP Store instructions
*/


// Opcode Jump Table
RSPInstruction RSP_Instruction[64] = {
	RSP_Special, RSP_RegImm, RSP_J, RSP_JAL, RSP_BEQ, RSP_BNE, RSP_BLEZ, RSP_BGTZ,
	RSP_ADDI, RSP_ADDIU, RSP_SLTI, RSP_SLTIU, RSP_ANDI, RSP_ORI, RSP_XORI, RSP_LUI,
	RSP_CoPro0, RSP_Unk, RSP_CoPro2, RSP_Unk, RSP_Unk, RSP_Unk, RSP_Unk, RSP_Unk,
	RSP_Unk, RSP_Unk, RSP_Unk, RSP_Unk, RSP_Unk, RSP_Unk, RSP_Unk, RSP_Unk,
	RSP_LB, RSP_LH, RSP_Unk, RSP_LW, RSP_LBU, RSP_LHU, RSP_Unk, RSP_Unk,
	RSP_SB, RSP_SH, RSP_Unk, RSP_SW, RSP_Unk, RSP_Unk, RSP_Unk, RSP_Unk,
	RSP_Unk, RSP_Unk, RSP_LWC2, RSP_Unk, RSP_Unk, RSP_Unk, RSP_Unk, RSP_Unk,
	RSP_Unk, RSP_Unk, RSP_SWC2, RSP_Unk, RSP_Unk, RSP_Unk, RSP_Unk, RSP_Unk
};


/*
    RSP SPECIAL: Instr. encoded by function field when opcode field = SPECIAL.
    31---------26-----------------------------------------5---------0
    | = SPECIAL |                                         | function|
    ------6----------------------------------------------------6-----
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
000 |  SLL  |  ---  |  SRL  |  SRA  | SLLV  |  ---  | SRLV  | SRAV  |
001 |  JR   |  JALR |  ---  |  ---  |  ---  | BREAK |  ---  |  ---  |
010 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
011 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
100 |  ADD  | ADDU  |  SUB  | SUBU  |  AND  |  OR   |  XOR  |  NOR  |
101 |  ---  |  ---  |  SLT  | SLTU  |  ---  |  ---  |  ---  |  ---  |
110 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
111 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
 */

// SpecialOpCode Jump Table
RSPInstruction RSPSpecialInstruction[64] = {
	RSP_Special_SLL, RSP_Special_Unk,  RSP_Special_SRL, RSP_Special_SRA,  RSP_Special_SLLV, RSP_Special_Unk,   RSP_Special_SRLV, RSP_Special_SRAV,
	RSP_Special_JR,  RSP_Special_JALR, RSP_Special_Unk, RSP_Special_Unk,  RSP_Special_Unk,  RSP_Special_BREAK, RSP_Special_Unk,  RSP_Special_Unk,
	RSP_Special_Unk, RSP_Special_Unk,  RSP_Special_Unk, RSP_Special_Unk,  RSP_Special_Unk,  RSP_Special_Unk,   RSP_Special_Unk,  RSP_Special_Unk,
	RSP_Special_Unk, RSP_Special_Unk,  RSP_Special_Unk, RSP_Special_Unk,  RSP_Special_Unk,  RSP_Special_Unk,   RSP_Special_Unk,  RSP_Special_Unk,
	RSP_Special_ADD, RSP_Special_ADDU, RSP_Special_SUB, RSP_Special_SUBU, RSP_Special_AND,  RSP_Special_OR,    RSP_Special_XOR,  RSP_Special_NOR,
	RSP_Special_Unk, RSP_Special_Unk,  RSP_Special_SLT, RSP_Special_SLTU, RSP_Special_Unk,  RSP_Special_Unk,   RSP_Special_Unk,  RSP_Special_Unk,
	RSP_Special_Unk, RSP_Special_Unk,  RSP_Special_Unk, RSP_Special_Unk,  RSP_Special_Unk,  RSP_Special_Unk,   RSP_Special_Unk,  RSP_Special_Unk,
	RSP_Special_Unk, RSP_Special_Unk,  RSP_Special_Unk, RSP_Special_Unk,  RSP_Special_Unk,  RSP_Special_Unk,   RSP_Special_Unk,  RSP_Special_Unk
};


/*
    REGIMM: Instructions encoded by the rt field when opcode field = REGIMM.
    31---------26----------20-------16------------------------------0
    | = REGIMM  |          |   rt    |                              |
    ------6---------------------5------------------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
 00 | BLTZ  | BGEZ  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 01 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 10 |BLTZAL |BGEZAL |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 11 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
 */

RSPInstruction RSPRegImmInstruction[32] = {

	RSP_RegImm_BLTZ,   RSP_RegImm_BGEZ,   RSP_RegImm_Unk, RSP_RegImm_Unk, RSP_RegImm_Unk, RSP_RegImm_Unk, RSP_RegImm_Unk, RSP_RegImm_Unk,
	RSP_RegImm_Unk,    RSP_RegImm_Unk,    RSP_RegImm_Unk, RSP_RegImm_Unk, RSP_RegImm_Unk, RSP_RegImm_Unk, RSP_RegImm_Unk, RSP_RegImm_Unk,
	RSP_RegImm_BLTZAL, RSP_RegImm_BGEZAL, RSP_RegImm_Unk, RSP_RegImm_Unk, RSP_RegImm_Unk, RSP_RegImm_Unk, RSP_RegImm_Unk, RSP_RegImm_Unk,
	RSP_RegImm_Unk,    RSP_RegImm_Unk,    RSP_RegImm_Unk, RSP_RegImm_Unk, RSP_RegImm_Unk, RSP_RegImm_Unk, RSP_RegImm_Unk, RSP_RegImm_Unk
};



/*
    COP0: Instructions encoded by the fmt field when opcode = COP0.
    31--------26-25------21 ----------------------------------------0
    |  = COP0   |   fmt   |                                         |
    ------6----------5-----------------------------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
 00 | MFC0  |  ---  |  ---  |  ---  | MTC0  |  ---  |  ---  |  ---  |
 01 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 10 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 11 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
*/

// COP0 Jump Table
RSPInstruction RSPCop0Instruction[32] = {
	RSP_Cop0_MFC0, RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_MTC0, RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk,
	RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk,
	RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk,
	RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk, RSP_Cop0_Unk,
};



/*
    RSP Load: Instr. encoded by rd field when opcode field = LWC2
    31---------26-------------------15-------11---------------------0
    |  110010   |                   |   rd   |                      |
    ------6-----------------------------5----------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
 00 |  LBV  |  LSV  |  LLV  |  LDV  |  LQV  |  LRV  |  LPV  |  LUV  |
 01 |  LHV  |  LFV  |  LWV  |  LTV  |  ---  |  ---  |  ---  |  ---  |
 10 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 11 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
*/

// LWC2 Jump Table
RSPInstruction RSPLWC2Instruction[32] = {
	RSP_LWC2_LBV, RSP_LWC2_LSV, RSP_LWC2_LLV, RSP_LWC2_LDV, RSP_LWC2_LQV, RSP_LWC2_LRV, RSP_LWC2_LPV, RSP_LWC2_LUV,
	RSP_LWC2_LHV, RSP_LWC2_LFV, RSP_LWC2_LWV, RSP_LWC2_LTV, RSP_LWC2_Unk, RSP_LWC2_Unk, RSP_LWC2_Unk, RSP_LWC2_Unk,
	RSP_LWC2_Unk, RSP_LWC2_Unk, RSP_LWC2_Unk, RSP_LWC2_Unk, RSP_LWC2_Unk, RSP_LWC2_Unk, RSP_LWC2_Unk, RSP_LWC2_Unk,
	RSP_LWC2_Unk, RSP_LWC2_Unk, RSP_LWC2_Unk, RSP_LWC2_Unk, RSP_LWC2_Unk, RSP_LWC2_Unk, RSP_LWC2_Unk, RSP_LWC2_Unk,
};

/*
    RSP Store: Instr. encoded by rd field when opcode field = SWC2
    31---------26-------------------15-------11---------------------0
    |  111010   |                   |   rd   |                      |
    ------6-----------------------------5----------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
 00 |  SBV  |  SSV  |  SLV  |  SDV  |  SQV  |  SRV  |  SPV  |  SUV  |
 01 |  SHV  |  SFV  |  SWV  |  STV  |  ---  |  ---  |  ---  |  ---  |
 10 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 11 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
*/

// SWC2 Jump Table
RSPInstruction RSPSWC2Instruction[32] = {
	RSP_SWC2_SBV, RSP_SWC2_SSV, RSP_SWC2_SLV, RSP_SWC2_SDV, RSP_SWC2_SQV, RSP_SWC2_SRV, RSP_SWC2_SPV, RSP_SWC2_SUV,
	RSP_SWC2_SHV, RSP_SWC2_SFV, RSP_SWC2_SWV, RSP_SWC2_STV, RSP_SWC2_Unk, RSP_SWC2_Unk, RSP_SWC2_Unk, RSP_SWC2_Unk,
	RSP_SWC2_Unk, RSP_SWC2_Unk, RSP_SWC2_Unk, RSP_SWC2_Unk, RSP_SWC2_Unk, RSP_SWC2_Unk, RSP_SWC2_Unk, RSP_SWC2_Unk,
	RSP_SWC2_Unk, RSP_SWC2_Unk, RSP_SWC2_Unk, RSP_SWC2_Unk, RSP_SWC2_Unk, RSP_SWC2_Unk, RSP_SWC2_Unk, RSP_SWC2_Unk,
};


/*
    COP2: Instructions encoded by the fmt field when opcode = COP2.
    31--------26-25------21 ----------------------------------------0
    |  = COP2   |   fmt   |                                         |
    ------6----------5-----------------------------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
 00 | MFC2  |  ---  | CFC2  |  ---  | MTC2  |  ---  | CTC2  |  ---  |
 01 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 10 |  *1   |  *1   |  *1   |  *1   |  *1   |  *1   |  *1   |  *1   |
 11 |  *1   |  *1   |  *1   |  *1   |  *1   |  *1   |  *1   |  *1   |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
     *1 = Vector opcode
*/

RSPInstruction RSPCop2Instruction[32] = {
	RSP_Cop2_MFC2, RSP_Cop2_Unk, RSP_Cop2_CFC2, RSP_Cop2_Unk, RSP_Cop2_MTC2, RSP_Cop2_Unk, RSP_Cop2_CTC2, RSP_Cop2_Unk,
	RSP_Cop2_Unk, RSP_Cop2_Unk, RSP_Cop2_Unk, RSP_Cop2_Unk, RSP_Cop2_Unk, RSP_Cop2_Unk, RSP_Cop2_Unk, RSP_Cop2_Unk,
	RSP_Cop2_Vector, RSP_Cop2_Vector, RSP_Cop2_Vector, RSP_Cop2_Vector, RSP_Cop2_Vector, RSP_Cop2_Vector, RSP_Cop2_Vector, RSP_Cop2_Vector,
	RSP_Cop2_Vector, RSP_Cop2_Vector, RSP_Cop2_Vector, RSP_Cop2_Vector, RSP_Cop2_Vector, RSP_Cop2_Vector, RSP_Cop2_Vector, RSP_Cop2_Vector
};

/*
    Vector opcodes: Instr. encoded by the function field when opcode = COP2.
    31---------26---25------------------------------------5---------0
    |  = COP2   | 1 |                                     | function|
    ------6-------1--------------------------------------------6-----
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
000 | VMULF | VMULU | VRNDP | VMULQ | VMUDL | VMUDM | VMUDN | VMUDH |
001 | VMACF | VMACU | VRNDN | VMACQ | VMADL | VMADM | VMADN | VMADH |
010 | VADD  | VSUB  | VSUT? | VABS  | VADDC | VSUBC | VADDB?| VSUBB?|
011 | VACCB?| VSUCB?| VSAD? | VSAC? | VSUM? | VSAW  |  ---  |  ---  |
100 |  VLT  |  VEQ  |  VNE  |  VGE  |  VCL  |  VCH  |  VCR  | VMRG  |
101 | VAND  | VNAND |  VOR  | VNOR  | VXOR  | VNXOR |  ---  |  ---  |
110 | VRCP  | VRCPL | VRCPH | VMOV  | VRSQ  | VRSQL | VRSQH |  ---  |
110 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
    Comment: Those with a ? in the end of them may not exist
*/
RSPInstruction RSPCop2VInstruction[64] = {
	RSP_Cop2V_VMULF, RSP_Cop2V_VMULU, RSP_Cop2V_VRNDP, RSP_Cop2V_VMULQ, RSP_Cop2V_VMUDL, RSP_Cop2V_VMUDM, RSP_Cop2V_VMUDN, RSP_Cop2V_VMUDH,
	RSP_Cop2V_VMACF, RSP_Cop2V_VMACU, RSP_Cop2V_VRNDN, RSP_Cop2V_VMACQ, RSP_Cop2V_VMADL, RSP_Cop2V_VMADM, RSP_Cop2V_VMADN, RSP_Cop2V_VMADH,
	RSP_Cop2V_VADD,  RSP_Cop2V_VSUB,  RSP_Cop2V_VSUT,  RSP_Cop2V_VABS,  RSP_Cop2V_VADDC, RSP_Cop2V_VSUBC, RSP_Cop2V_VADDB, RSP_Cop2V_VSUBB,
	RSP_Cop2V_VACCB, RSP_Cop2V_VSUCB, RSP_Cop2V_VSAD,  RSP_Cop2V_VSAC,  RSP_Cop2V_VSUM,  RSP_Cop2V_VSAW,  RSP_Cop2V_Unk,   RSP_Cop2V_Unk,
	RSP_Cop2V_VLT,   RSP_Cop2V_VEQ,   RSP_Cop2V_VNE,   RSP_Cop2V_VGE,   RSP_Cop2V_VCL,   RSP_Cop2V_VCH,   RSP_Cop2V_VCR,   RSP_Cop2V_VMRG,
	RSP_Cop2V_VAND,  RSP_Cop2V_VNAND, RSP_Cop2V_VOR,   RSP_Cop2V_VNOR,  RSP_Cop2V_VXOR,  RSP_Cop2V_VNXOR, RSP_Cop2V_Unk,   RSP_Cop2V_Unk,
	RSP_Cop2V_VRCP,  RSP_Cop2V_VRCPL, RSP_Cop2V_VRCPH, RSP_Cop2V_VMOV,  RSP_Cop2V_VRSQ,  RSP_Cop2V_VRSQL, RSP_Cop2V_VRSQH, RSP_Cop2V_Unk,
	RSP_Cop2V_Unk,   RSP_Cop2V_Unk,   RSP_Cop2V_Unk,   RSP_Cop2V_Unk,   RSP_Cop2V_Unk,   RSP_Cop2V_Unk,   RSP_Cop2V_Unk,   RSP_Cop2V_Unk
};

/*
	vadd   $v1, $v2, $v3          0 1 2 3 4 5 6 7
	vadd   $v1, $v2, $v3[0q]      0 1 2 3 0 1 2 3
	vadd   $v1, $v2, $v3[1q]      4 5 6 7 4 5 6 7
	vadd   $v1, $v2, $v3[0h]      0 1 0 1 0 1 0 1
	vadd   $v1, $v2, $v3[1h]      2 3 2 3 2 3 2 3
	vadd   $v1, $v2, $v3[2h]      4 5 4 5 4 5 4 5
	vadd   $v1, $v2, $v3[3h]      6 7 6 7 6 7 6 7
	vadd   $v1, $v2, $v3[0]       0 0 0 0 0 0 0 0
	vadd   $v1, $v2, $v3[1]       1 1 1 1 1 1 1 1
	vadd   $v1, $v2, $v3[2]       2 2 2 2 2 2 2 2
	vadd   $v1, $v2, $v3[3]       3 3 3 3 3 3 3 3
	vadd   $v1, $v2, $v3[4]       4 4 4 4 4 4 4 4
	vadd   $v1, $v2, $v3[5]       5 5 5 5 5 5 5 5
	vadd   $v1, $v2, $v3[6]       6 6 6 6 6 6 6 6
	vadd   $v1, $v2, $v3[7]       7 7 7 7 7 7 7 7
*/

#define LOAD_VECTORS(dwA, dwB, dwPattern)						\
	for (u32 __i = 0; __i < 8; __i++) {								\
		g_wTemp[0]._u16[__i] = gVectRSP[dwA]._u16[__i];							\
		g_wTemp[1]._u16[__i] = gVectRSP[dwB]._u16[g_Pattern[dwPattern][__i]];	\
	}

static const u32 g_Pattern[16][8] = {
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	//?
	{ 0, 0, 2, 2, 4, 4, 6, 6 },
	{ 1, 1, 3, 3, 5, 5, 7, 7 },

	{ 0, 0, 0, 0, 4, 4, 4, 4 },
	{ 1, 1, 1, 1, 5, 5, 5, 5 },
	{ 2, 2, 2, 2, 6, 6, 6, 6 },
	{ 3, 3, 3, 3, 7, 7, 7, 7 },

	{ 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 2, 2, 2, 2, 2, 2, 2, 2 },
	{ 3, 3, 3, 3, 3, 3, 3, 3 },
	{ 4, 4, 4, 4, 4, 4, 4, 4 },
	{ 5, 5, 5, 5, 5, 5, 5, 5 },
	{ 6, 6, 6, 6, 6, 6, 6, 6 },
	{ 7, 7, 7, 7, 7, 7, 7, 7 }
};

//*****************************************************************************
//
//*****************************************************************************
void RSP_Reset(void)
{
	memset( &gRSPState, 0, sizeof(gRSPState));
	memset( EleSpec, 0, sizeof( EleSpec ) );
	memset( Indx, 0, sizeof( Indx ) );

	EleSpec[16]._u64 = 0x0001020304050607LL; /* None */
	EleSpec[17]._u64 = 0x0001020304050607LL; /* None */
	EleSpec[18]._u64 = 0x0000020204040606LL; /* 0q */
	EleSpec[19]._u64 = 0x0101030305050707LL; /* 1q */
	EleSpec[20]._u64 = 0x0000000004040404LL; /* 0h */
	EleSpec[21]._u64 = 0x0101010105050505LL; /* 1h */
	EleSpec[22]._u64 = 0x0202020206060606LL; /* 2h */
	EleSpec[23]._u64 = 0x0303030307070707LL; /* 3h */
	EleSpec[24]._u64 = 0x0000000000000000LL; /* 0 */
	EleSpec[25]._u64 = 0x0101010101010101LL; /* 1 */
	EleSpec[26]._u64 = 0x0202020202020202LL; /* 2 */
	EleSpec[27]._u64 = 0x0303030303030303LL; /* 3 */
	EleSpec[28]._u64 = 0x0404040404040404LL; /* 4 */
	EleSpec[29]._u64 = 0x0505050505050505LL; /* 5 */
	EleSpec[30]._u64 = 0x0606060606060606LL; /* 6 */
	EleSpec[31]._u64 = 0x0707070707070707LL; /* 7 */

	Indx[16]._u64 = 0x0001020304050607LL; // None		0x 0001020304050607
	Indx[17]._u64 = 0x0001020304050607LL; // None		0x 0001020304050607
	Indx[18]._u64 = 0x0103050700020406LL; // 0q		0x01030507 00020406
	Indx[19]._u64 = 0x0002040601030507LL; // 1q		0x00020406 01030507
	Indx[20]._u64 = 0x0102030506070004LL; // 0h		0x010203050607 0004
	Indx[21]._u64 = 0x0002030406070105LL; // 1h		0x000203040607 0105
	Indx[22]._u64 = 0x0001030405070206LL; // 2h		0x000103040507 0206
	Indx[23]._u64 = 0x0001020405060307LL; // 3h		0x000102040506 0307
	Indx[24]._u64 = 0x0102030405060700LL; // 0		0x01020304050607 00
	Indx[25]._u64 = 0x0002030405060701LL; // 1		0x00020304050607 01
	Indx[26]._u64 = 0x0001030405060702LL; // 2		0x00010304050607 02
	Indx[27]._u64 = 0x0001020405060703LL; // 3		0x00010204050607 03
	Indx[28]._u64 = 0x0001020305060704LL; // 4		0x00010203050607 04
	Indx[29]._u64 = 0x0001020304060705LL; // 5		0x00010203040607 05
	Indx[30]._u64 = 0x0001020304050706LL; // 6		0x00010203040507 06
	Indx[31]._u64 = 0x0001020304050607LL; // 7		0x00010203040506 07

		//Indx[02]    = 0x01 03 05 07 00 02 04 06		0q
		//EleSpec[02] = 0x07 07 05 05 03 03 01 01

/*06 / 07
04 / 05
02 / 03
00 / 01
07 / 07
05 / 05
03 / 03
01 / 01*/


	for ( u32 i = 16; i < 32; i++ )
	{
		u32 count;

		// Swap 0 for 7, 1 for 6 etc
		for ( count = 0; count < 8; count++ )
		{
			Indx[i]._u8[count]    = 7 - Indx[i]._u8[count];
			EleSpec[i]._u8[count] = 7 - EleSpec[i]._u8[count];
		}

		// Reverse the order
		for (count = 0; count < 4; count ++)
		{
			// Swap Indx[i][count] and Indx[i][7-count]
			u8 temp                = Indx[i]._u8[count];
			Indx[i]._u8[count]     = Indx[i]._u8[7 - count];
			Indx[i]._u8[7 - count] = temp;
		}

	}
/*
	for ( i = 16; i < 32; i++ )
	{
		//Indx[00] = 0x0001020304050607		None
		//Indx[01] = 0x0001020304050607		None
		//Indx[02] = 0x0103050700020406		0q
		//Indx[03] = 0x0002040601030507		1q
		//Indx[04] = 0x0307000102040506		0h
		//Indx[05] = 0x0206000103040507		1h
		//Indx[06] = 0x0105000203040607		2h
		//Indx[07] = 0x0004010203050607		3h
		//Indx[08] = 0x0700010203040506		0
		//Indx[09] = 0x0600010203040507		1
		//Indx[10] = 0x0500010203040607		2
		//Indx[11] = 0x0400010203050607		3
		//Indx[12] = 0x0300010204050607		4
		//Indx[13] = 0x0200010304050607		5
		//Indx[14] = 0x0100020304050607		6
		//Indx[15] = 0x0001020304050607		7
		DBGConsole_Msg( 0, "Indx[[%02d[] = 0x%08x%08x", i - 16, Indx[i]._u32[1], Indx[i]._u32_0 );
	}
	for ( i = 16; i < 32; i++ )
	{
		//EleSpec[00] = 0x0706050403020100
		//EleSpec[01] = 0x0706050403020100
		//EleSpec[02] = 0x0707050503030101
		//EleSpec[03] = 0x0606040402020000
		//EleSpec[04] = 0x0707070703030303
		//EleSpec[05] = 0x0606060602020202
		//EleSpec[06] = 0x0505050501010101
		//EleSpec[07] = 0x0404040400000000
		//EleSpec[08] = 0x0707070707070707
		//EleSpec[09] = 0x0606060606060606
		//EleSpec[10] = 0x0505050505050505
		//EleSpec[11] = 0x0404040404040404
		//EleSpec[12] = 0x0303030303030303
		//EleSpec[13] = 0x0202020202020202
		//EleSpec[14] = 0x0101010101010101
		//EleSpec[15] = 0x0000000000000000

		DBGConsole_Msg( 0, "EleSpec[[%02d[] = 0x%08x%08x", i - 16, EleSpec[i]._u32[1], EleSpec[i]._u32_0 );
	}
*/

	DAEDALUS_ASSERT( !gRSPHLEActive, "Resetting RSP with HLE active" );
	Memory_SP_SetRegister(SP_STATUS_REG, SP_STATUS_HALT);			// SP is halted
}

//*****************************************************************************
//
//*****************************************************************************
bool RSP_IsRunning()
{
	return (Memory_SP_GetRegister( SP_STATUS_REG ) & SP_STATUS_HALT) == 0;
}

//*****************************************************************************
//
//*****************************************************************************
bool RSP_IsRunningLLE()
{
	return RSP_IsRunning() && !gRSPHLEActive;
}

//*****************************************************************************
//
//*****************************************************************************
bool RSP_IsRunningHLE()
{
	return RSP_IsRunning() && gRSPHLEActive;
}


//*****************************************************************************
//
//*****************************************************************************
void RSP_Step()
{
	DAEDALUS_ASSERT( RSP_IsRunning(), "Why is the RSP stepping when it is halted?" );
	DAEDALUS_ASSERT( !gRSPHLEActive, "Why is the RSP stepping when HLE is active?" );

	// Fetch next instruction
	u32 pc = RSPPC;
	OpCode op;
	op._u32 = *(u32 *)(g_pu8SpMemBase + ((pc & 0x0FFF) | 0x1000));

	RSP_Instruction[ op.op ]( op );

	switch (g_nRSPDelay)
	{
		case DO_DELAY:
			// We've got a delayed instruction to execute. Increment
			// PC as normal, so that subsequent instruction is executed
			RSPPC = RSPPC + 4;
			g_nRSPDelay = EXEC_DELAY;
			break;
		case EXEC_DELAY:
			// We've just executed the delayed instr. Now carry out jump
			//  as stored in g_dwNewRSPPC;
			RSPPC = g_dwNewRSPPC;
			g_nRSPDelay = NO_DELAY;
			break;
		case NO_DELAY:
			// Normal operation - just increment the PC
			RSPPC = RSPPC + 4;
			break;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_DumpVector(u32 reg)
{
	reg &= 0x1f;

	DBGConsole_Msg(0, "Vector%d", reg);
	for(u32 i = 0; i < 8; i++)
	{
		DBGConsole_Msg(0, "%d: 0x%04x", i, gVectRSP[reg]._u16[i]);
	}
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_DumpVectors(u32 reg1, u32 reg2, u32 reg3)
{
	reg1 &= 0x1f;
	reg2 &= 0x1f;
	reg3 &= 0x1f;

	DBGConsole_Msg(0, "    Vec%2d\tVec%2d\tVec%2d", reg1, reg2, reg3);
	for(u32 i = 0; i < 8; i++)
	{
		DBGConsole_Msg(0, "%d: 0x%04x\t0x%04x\t0x%04x",
			i, gVectRSP[reg1]._u16[i], gVectRSP[reg2]._u16[i], gVectRSP[reg3]._u16[i]);
	}
}


//*****************************************************************************
//
//*****************************************************************************
#define WARN_NOEXIST(inf)	DAEDALUS_ERROR( "Instruction Unkown: " inf )
#define WARN_NOIMPL(op)		DAEDALUS_ERROR( "Instruction Not Implemented: " op )

//*****************************************************************************
//
//*****************************************************************************
void RSP_Unk( OpCode op )		{ WARN_NOEXIST("RSP_Unk"); }
void RSP_Special( OpCode op )	{ RSPSpecialInstruction[op.spec_op]( op ); }
void RSP_RegImm( OpCode op )	{ RSPRegImmInstruction[op.regimm_op]( op ); }
void RSP_CoPro0( OpCode op )	{ RSPCop0Instruction[op.cop0_op]( op ); }
void RSP_CoPro2( OpCode op )	{ RSPCop2Instruction[op.cop1_op]( op ); }
void RSP_LWC2( OpCode op )		{ RSPLWC2Instruction[op.rd]( op ); }
void RSP_SWC2( OpCode op )		{ RSPSWC2Instruction[op.rd]( op ); }

//*****************************************************************************
// Jump
//*****************************************************************************
void RSP_J( OpCode op )
{
	// Jump
	g_dwNewRSPPC = op.target << 2;
	g_nRSPDelay = DO_DELAY;
}

//*****************************************************************************
// Jump And Link
//*****************************************************************************
void RSP_JAL( OpCode op )
{
	gRegRSP[31]._u32 = RSPPC + 8;		// Store return address

	g_dwNewRSPPC = op.target << 2;
	g_nRSPDelay = DO_DELAY;
}

//*****************************************************************************
// Branch on Equal
//*****************************************************************************
void RSP_BEQ( OpCode op )
{
	//branch if rs == rt
	//g_nRSPDelay = DO_DELAY;
	if ( gRegRSP[op.rs]._u32 == gRegRSP[op.rt]._u32 )
	{
		g_nRSPDelay = DO_DELAY;
		g_dwNewRSPPC = RSPPC + ( (s32)(s16)op.offset << 2 ) + 4 ;
	}
	/*else
	{
		g_dwNewRSPPC = ( RSPPC + 8 ) & 0x0FFC;
	}*/
}

//*****************************************************************************
// Branch on Not Equal
//*****************************************************************************
void RSP_BNE( OpCode op )
{
	//branch if rs != rt
	if ( gRegRSP[op.rs]._u32 != gRegRSP[op.rt]._u32 )
	{
		g_dwNewRSPPC = RSPPC + ( (s32)(s16)op.offset << 2 ) + 4;
		g_nRSPDelay = DO_DELAY;
	}
}

//*****************************************************************************
// Branch on Less than or Equal to Zero
//*****************************************************************************
void RSP_BLEZ( OpCode op )
{
	//branch if rs <= 0
	if ( gRegRSP[op.rs]._s32 <= 0 )
	{
		g_dwNewRSPPC = RSPPC + ( (s32)(s16)op.offset << 2 ) + 4;
		g_nRSPDelay = DO_DELAY;
	}
}

//*****************************************************************************
// Branch on Greater Than to Zero
//*****************************************************************************
void RSP_BGTZ( OpCode op )
{
	//branch if rs > 0
	if ( gRegRSP[op.rs]._s32 > 0 )
	{
		g_dwNewRSPPC = RSPPC + ( (s32)(s16)op.offset << 2 ) + 4;
		g_nRSPDelay = DO_DELAY;
	}
}

//*****************************************************************************
// ADD Immediate
//*****************************************************************************
void RSP_ADDI( OpCode op )
{
	RSP_ASSERT_Q( op.rt != 0 );

	//rt = rs + immediate
	gRegRSP[op.rt]._s32 = gRegRSP[op.rs]._s32 + (s32)(s16)op.immediate;
}

//*****************************************************************************
// ADD Immediate Unsigned -- Same as above, but doesn't generate exception
//*****************************************************************************
void RSP_ADDIU( OpCode op )
{
	RSP_ASSERT_Q( op.rt != 0 );

	//rt = rs + immediate
	gRegRSP[op.rt]._u32 = gRegRSP[op.rs]._u32 + (u32)(s16)op.immediate;
}

//*****************************************************************************
// Set on Less Than Immediate
//*****************************************************************************
void RSP_SLTI( OpCode op )
{
	RSP_ASSERT_Q( op.rt != 0 );

	if (gRegRSP[op.rs]._s32 < (s16)op.immediate)
	{
		gRegRSP[op.rt]._u32 = 1;
	}
	else
	{
		gRegRSP[op.rt]._u32 = 0;
	}

}

//*****************************************************************************
// Set on Less Than Immediate Unsigned
//*****************************************************************************
void RSP_SLTIU( OpCode op )
{
	RSP_ASSERT_Q( op.rt != 0 );

	if (gRegRSP[op.rs]._u32 < (u32)(s32)(s16)op.immediate)
	{
		gRegRSP[op.rt]._u32 = 1;
	}
	else
	{
		gRegRSP[op.rt]._u32 = 0;
	}
}

//*****************************************************************************
// AND Immediate
//*****************************************************************************
void RSP_ANDI( OpCode op )
{
	RSP_ASSERT_Q( op.rt != 0 );

	//rt = rs & immediate
	gRegRSP[op.rt]._u32 = gRegRSP[op.rs]._u32 & op.immediate;
}

//*****************************************************************************
// OR Immediate
//*****************************************************************************
void RSP_ORI( OpCode op )
{
	RSP_ASSERT_Q( op.rt != 0 );

	//rt = rs | immediate
	gRegRSP[op.rt]._u32 = gRegRSP[op.rs]._u32 | op.immediate;
}

//*****************************************************************************
// XOR Immediate
//*****************************************************************************
void RSP_XORI( OpCode op )
{
	RSP_ASSERT_Q( op.rt != 0 );

	//rt = rs ^ immediate
	gRegRSP[op.rt]._u32 = gRegRSP[op.rs]._u32 ^ op.immediate;
}

//*****************************************************************************
// Load Upper Immediate
//*****************************************************************************
void RSP_LUI( OpCode op )
{
	RSP_ASSERT_Q( op.rt != 0 );

	gRegRSP[op.rt]._u32 = (s32)op.immediate << 16;
}

#define MAKE_ADDRESS( base, offset )		((u32)(gRegRSP[(base)]._s32 + (s32)(s16)(offset)) & 0x0FFF)

//*****************************************************************************
// Load Byte (signed)
//*****************************************************************************
void RSP_LB( OpCode op )
{
	RSP_ASSERT_Q( op.rt != 0 );

	u32 address = MAKE_ADDRESS( op.base, op.offset );

	gRegRSP[op.rt]._s32 = g_ps8SpMemBase[ address ^ 0x3 ];
}

//*****************************************************************************
// Load Byte (unsigned)
//*****************************************************************************
void RSP_LBU( OpCode op )
{
	RSP_ASSERT_Q( op.rt != 0 );

	u32 address = MAKE_ADDRESS( op.base, op.offset );

	gRegRSP[op.rt]._u32 = g_pu8SpMemBase[ address ^ 0x3 ];
}

//*****************************************************************************
// Load Half-Word (signed)
//*****************************************************************************
void RSP_LH( OpCode op )
{
	RSP_ASSERT_Q( op.rt != 0 );

	u32 address = MAKE_ADDRESS( op.base, op.offset );

	if ( address & 0x1 )
	{
		// Unaligned LH
		RSP_ASSERT_Q( address <= 0xFFE );

		gRegRSP[op.rt]._u8[1] = g_pu8SpMemBase[(address + 0) ^ U8_TWIDDLE];
		gRegRSP[op.rt]._u8[0] = g_pu8SpMemBase[(address + 1) ^ U8_TWIDDLE];

		gRegRSP[op.rt]._s32 = gRegRSP[op.rt]._s16[0];
	}
	else
	{
		gRegRSP[op.rt]._s32 = *(s16*)( g_pu8SpMemBase + (address^0x2) );
	}

}

//*****************************************************************************
// Load Half-Word (unsigned)
//*****************************************************************************
void RSP_LHU( OpCode op )
{
	RSP_ASSERT_Q( op.rt != 0 );

	u32 address = MAKE_ADDRESS( op.base, op.offset );

	if ( address & 0x1 )
	{
		RSP_ASSERT_Q( address <= 0xFFE );

		// Unaligned LHU
		gRegRSP[op.rt]._u8[1] = g_pu8SpMemBase[(address + 0) ^ U8_TWIDDLE];
		gRegRSP[op.rt]._u8[0] = g_pu8SpMemBase[(address + 1) ^ U8_TWIDDLE];

		gRegRSP[op.rt]._u32 = gRegRSP[op.rt]._u16[0];
	}
	else
	{
		gRegRSP[op.rt]._u32 = *(u16*)( g_pu8SpMemBase + (address^0x2) );
	}
}

//*****************************************************************************
// Load Word
//*****************************************************************************
void RSP_LW( OpCode op )
{
	RSP_ASSERT_Q( op.rt != 0 );

	u32 address = MAKE_ADDRESS( op.base, op.offset );

	if ( address & 0x3 )
	{
		// Unaligned LW
		RSP_ASSERT_Q( address <= 0xFFC );

		gRegRSP[op.rt]._u8[3] = g_pu8SpMemBase[(address + 0) ^ U8_TWIDDLE];
		gRegRSP[op.rt]._u8[2] = g_pu8SpMemBase[(address + 1) ^ U8_TWIDDLE];
		gRegRSP[op.rt]._u8[1] = g_pu8SpMemBase[(address + 2) ^ U8_TWIDDLE];
		gRegRSP[op.rt]._u8[0] = g_pu8SpMemBase[(address + 3) ^ U8_TWIDDLE];
	}
	else
	{
		gRegRSP[op.rt]._u32 = *(u32*)( g_pu8SpMemBase + address );				// Read from DMEM
	}

}


//*****************************************************************************
// Store Byte
//*****************************************************************************
void RSP_SB( OpCode op )
{
	u32 address = MAKE_ADDRESS( op.base, op.offset );

	g_pu8SpMemBase[ address^0x3 ] = gRegRSP[op.rt]._u8[0];
}

//*****************************************************************************
// Store Half-Word
//*****************************************************************************
void RSP_SH( OpCode op )
{
	u32 address = MAKE_ADDRESS( op.base, op.offset );

	if ( address & 0x1 )
	{
		// Unaligned SH
		RSP_ASSERT_Q( address <= 0xFFE );

		g_pu8SpMemBase[(address + 0) ^ U8_TWIDDLE] = gRegRSP[op.rt]._u8[1];
		g_pu8SpMemBase[(address + 1) ^ U8_TWIDDLE] = gRegRSP[op.rt]._u8[0];
	}
	else
	{
		*(u16*)( g_pu8SpMemBase + (address^0x2) ) = gRegRSP[op.rt]._u16[0];
	}
}

//*****************************************************************************
// Store Word
//*****************************************************************************
void RSP_SW( OpCode op )
{
	u32 address = MAKE_ADDRESS( op.base, op.offset );

	if ( address & 0x3 )
	{
		// Unaligned SW
		RSP_ASSERT_Q( address <= 0xFFC );

		g_pu8SpMemBase[(address + 0) ^ U8_TWIDDLE] = gRegRSP[op.rt]._u8[3];
		g_pu8SpMemBase[(address + 1) ^ U8_TWIDDLE] = gRegRSP[op.rt]._u8[2];
		g_pu8SpMemBase[(address + 2) ^ U8_TWIDDLE] = gRegRSP[op.rt]._u8[1];
		g_pu8SpMemBase[(address + 3) ^ U8_TWIDDLE] = gRegRSP[op.rt]._u8[0];
	}
	else
	{
		*(u32*)( g_pu8SpMemBase + address ) = gRegRSP[op.rt]._u32;				// Write to DMEM
	}

}

//*****************************************************************************
//
//*****************************************************************************
void RSP_Special_Unk( OpCode op )
{
	WARN_NOEXIST( "RSP_Special_Unk" );
}

//*****************************************************************************
// Shift word Left Logical
//*****************************************************************************
void RSP_Special_SLL( OpCode op )
{
	RSP_ASSERT_Q( op._u32 == 0 || op.rd != 0 );

	if (op._u32 != 0)
		gRegRSP[op.rd]._u32 = gRegRSP[op.rt]._u32 << op.sa;
}

//*****************************************************************************
// Shift word Right Logical
//*****************************************************************************
void RSP_Special_SRL( OpCode op )
{
	RSP_ASSERT_Q( op.rd != 0 );

	gRegRSP[op.rd]._u32 = gRegRSP[op.rt]._u32 >> op.sa;
}

//*****************************************************************************
// Shift word Right Arithmetic
//*****************************************************************************
void RSP_Special_SRA( OpCode op )
{
	RSP_ASSERT_Q( op.rd != 0 );

	gRegRSP[op.rd]._s32 = gRegRSP[op.rt]._s32 >> op.sa;
}

//*****************************************************************************
// Shift word Left Logical Variable
//*****************************************************************************
void RSP_Special_SLLV( OpCode op )
{
	RSP_ASSERT_Q( op.rd != 0 );

	gRegRSP[op.rd]._u32 = gRegRSP[op.rt]._u32 << ( gRegRSP[op.rs]._u32 & 0x1F );
}

//*****************************************************************************
// Shift word Right Logical Variable
//*****************************************************************************
void RSP_Special_SRLV( OpCode op )
{
	RSP_ASSERT_Q( op.rd != 0 );

	gRegRSP[op.rd]._u32 = gRegRSP[op.rt]._u32 >> ( gRegRSP[op.rs]._u32 & 0x1F );
}

//*****************************************************************************
// Shift word Right Arithmetic Variable
//*****************************************************************************
void RSP_Special_SRAV( OpCode op )
{
	RSP_ASSERT_Q( op.rd != 0 );

	gRegRSP[op.rd]._s32 = gRegRSP[op.rt]._s32 >> ( gRegRSP[op.rs]._u32 & 0x1F );
}

//*****************************************************************************
// Jump Register
//*****************************************************************************
void RSP_Special_JR( OpCode op )
{
	g_nRSPDelay = DO_DELAY;
	g_dwNewRSPPC = gRegRSP[op.rs]._u32;
}

//*****************************************************************************
// Jump And Link Register
//*****************************************************************************
void RSP_Special_JALR( OpCode op )
{
	g_nRSPDelay = DO_DELAY;
	g_dwNewRSPPC = gRegRSP[op.rs]._u32;

	// Store return address
	gRegRSP[op.rd]._u32 = RSPPC + 8;
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_Special_BREAK( OpCode op )
{
	//
	// Break/Halt the RSP
	//
	Memory_SP_SetRegisterBits(SP_STATUS_REG, SP_STATUS_BROKE|SP_STATUS_HALT);

	//
	// We've set the SP_STATUS_BROKE flag - better check if it causes an interrupt
	//
	if( Memory_SP_GetRegister(SP_STATUS_REG) & SP_STATUS_INTR_BREAK )
	{
		Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_SP);
		R4300_Interrupt_UpdateCause3();
	}

	//
	// We've finished running the RSP - better select the right core.
	//
	CPU_SelectCore();
}

//*****************************************************************************
// Add (signed)
//*****************************************************************************
void RSP_Special_ADD( OpCode op )
{
	RSP_ASSERT_Q( op.rd != 0 );

	gRegRSP[op.rd]._s32 = gRegRSP[op.rs]._s32 + gRegRSP[op.rt]._s32;
}

//*****************************************************************************
// Add (unsigned)
//*****************************************************************************
void RSP_Special_ADDU( OpCode op )
{
	RSP_ASSERT_Q( op.rd != 0 );

	gRegRSP[op.rd]._u32 = gRegRSP[op.rs]._u32 + gRegRSP[op.rt]._u32;
}

//*****************************************************************************
// Subtract (signed)
//*****************************************************************************
void RSP_Special_SUB( OpCode op )
{
	RSP_ASSERT_Q( op.rd != 0 );

	gRegRSP[op.rd]._s32 = gRegRSP[op.rs]._s32 - gRegRSP[op.rt]._s32;
}

//*****************************************************************************
// Subtract (unsigned)
//*****************************************************************************
void RSP_Special_SUBU( OpCode op )
{
	RSP_ASSERT_Q( op.rd != 0 );

	gRegRSP[op.rd]._u32 = gRegRSP[op.rs]._u32 - gRegRSP[op.rt]._u32;
}

//*****************************************************************************
// And
//*****************************************************************************
void RSP_Special_AND( OpCode op )
{
	RSP_ASSERT_Q( op.rd != 0 );

	gRegRSP[op.rd]._u32 = gRegRSP[op.rs]._u32 & gRegRSP[op.rt]._u32;
}


//*****************************************************************************
// Or
//*****************************************************************************
void RSP_Special_OR( OpCode op )
{
	RSP_ASSERT_Q( op.rd != 0 );

	gRegRSP[op.rd]._u32 = gRegRSP[op.rs]._u32 | gRegRSP[op.rt]._u32;
}


//*****************************************************************************
// Exclusive Or
//*****************************************************************************
void RSP_Special_XOR( OpCode op )
{
	RSP_ASSERT_Q( op.rd != 0 );

	gRegRSP[op.rd]._u32 = gRegRSP[op.rs]._u32 ^ gRegRSP[op.rt]._u32;
}

//*****************************************************************************
// Not-Or
//*****************************************************************************
void RSP_Special_NOR( OpCode op ) 			// logical NOT OR
{
	RSP_ASSERT_Q( op.rd != 0 );

	gRegRSP[op.rd]._u32 = ~( gRegRSP[op.rs]._u32 | gRegRSP[op.rt]._u32 );
}


//*****************************************************************************
// Set on Less Than
//*****************************************************************************
void RSP_Special_SLT( OpCode op )
{
	RSP_ASSERT_Q( op.rd != 0 );

	if ( gRegRSP[op.rs]._s32 < gRegRSP[op.rt]._s32 )
	{
		gRegRSP[op.rd]._u32 = 1;
	}
	else
	{
		gRegRSP[op.rd]._u32 = 0;
	}
}


//*****************************************************************************
// Set on Less Than Unsigned
//*****************************************************************************
void RSP_Special_SLTU( OpCode op )
{
	RSP_ASSERT_Q( op.rd != 0 );

	if ( gRegRSP[op.rs]._u32 < gRegRSP[op.rt]._u32 )
	{
		gRegRSP[op.rd]._u32 = 1;
	}
	else
	{
		gRegRSP[op.rd]._u32 = 0;
	}
}


//*****************************************************************************
//
//*****************************************************************************
void RSP_RegImm_Unk( OpCode op ){ WARN_NOEXIST("RSP_RegImm_Unk"); }


//*****************************************************************************
// Branch on Less than Zero
//*****************************************************************************
void RSP_RegImm_BLTZ( OpCode op )
{
	//branch if rs < 0
	if ( gRegRSP[op.rs]._s32 < 0 )
	{
		g_dwNewRSPPC = RSPPC + ( (s32)(s16)op.offset << 2 ) + 4;
		g_nRSPDelay = DO_DELAY;
	}
}

//*****************************************************************************
// Branch on Greater than or Equal to than Zero
//*****************************************************************************
void RSP_RegImm_BGEZ( OpCode op )
{
	//branch if rs < 0
	if ( gRegRSP[op.rs]._s32 >= 0 )
	{
		g_dwNewRSPPC = RSPPC + ( (s32)(s16)op.offset << 2 ) + 4;
		g_nRSPDelay = DO_DELAY;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_RegImm_BLTZAL( OpCode op )
{
	//branch if rs < 0
	// Store return address. This always happens, even if branch not taken
	gRegRSP[31]._u32 = RSPPC + 8;

	if ( gRegRSP[op.rs]._s32 < 0 )
	{
		g_dwNewRSPPC = RSPPC + ( (s32)(s16)op.offset << 2 ) + 4;
		g_nRSPDelay = DO_DELAY;
	}
}


//*****************************************************************************
//
//*****************************************************************************
void RSP_RegImm_BGEZAL( OpCode op )
{
	//branch if rs >= 0
	// Store return address. This always happens, even if branch not taken
	gRegRSP[31]._u32 = RSPPC + 8;

	if ( gRegRSP[op.rs]._s32 >= 0 )
	{
		g_dwNewRSPPC = RSPPC + ( (s32)(s16)op.offset << 2 ) + 4;
		g_nRSPDelay = DO_DELAY;
	}

}

//*****************************************************************************
//
//*****************************************************************************
void RSP_Cop0_Unk( OpCode op ) {WARN_NOEXIST("RSP_Cop0_Unk"); }


//*****************************************************************************
// Move From Cop0
//*****************************************************************************
void RSP_Cop0_MFC0( OpCode op )
{
	//SP_MEM_ADDR_REG		// 0
	//SP_DRAM_ADDR_REG		// 1
	//SP_RD_LEN_REG			// 2
	//SP_WR_LEN_REG			// 3
	//SP_STATUS_REG			// 4
	//SP_DMA_FULL_REG		// 5
	//SP_DMA_BUSY_REG		// 6
	//SP_SEMAPHORE_REG		// 7

	//DPC_START_REG			// 8
	//DPC_END_REG			// 9
	//DPC_CURRENT_REG		// 10
	//DPC_STATUS_REG		// 11
	//DPC_CLOCK_REG			// 12
	//DPC_BUFBUSY_REG		// 13
	//DPC_PIPEBUSY_REG		// 14
	//DPC_TMEM_REG			// 15

	RSP_ASSERT_Q( op.rt != 0 );

	if (op.rd < 8)	gRegRSP[op.rt]._u32 = Read32Bits(0x84040000 | ( (op.rd        ) << 2) );	// SP Regs
	else			gRegRSP[op.rt]._u32 = Read32Bits(0x84100000 | ( (op.rd & ~0x08) << 2) );	// DP Command Regs.
}

//*****************************************************************************
// Move To Cop0
//*****************************************************************************
void RSP_Cop0_MTC0( OpCode op )
{
	/*if ( op.rd == 9 )
	{
		extern bool gLogSpDMA;
		gLogSpDMA = true;
		CPU_Halt( "MTC DPC_END_REG" );
	}*/

	if (op.rd < 8)	Write32Bits(0x84040000 | ( (op.rd        ) << 2), gRegRSP[op.rt]._u32);	// SP Regs
	else			Write32Bits(0x84100000 | ( (op.rd & ~0x08) << 2), gRegRSP[op.rt]._u32);	// DP Command Regs.
}


//*****************************************************************************
//
//*****************************************************************************
void RSP_LWC2_Unk( OpCode op ) { WARN_NOEXIST("RSP_LWC2_Unk"); }
void RSP_SWC2_Unk( OpCode op ) { WARN_NOEXIST("RSP_SWC2_Unk"); }

void RSP_LWC2_LHV( OpCode op ) { WARN_NOIMPL("RSP: LHV"); }
void RSP_LWC2_LFV( OpCode op ) { WARN_NOIMPL("RSP: LFV"); }
void RSP_LWC2_LWV( OpCode op ) { WARN_NOIMPL("RSP: LWV"); }


//*****************************************************************************
// Added 08/03/2002, not checked
//*****************************************************************************
void RSP_LWC2_LPV( OpCode op )
{
	const u32 element = op.del;

	u32 address = ( gRegRSP[op.base]._u32 + (u32)( op.voffset << 3 ) ) & 0x0FFF;

	gVectRSP[op.rt]._s16[7] = g_pu8SpMemBase[((address + ((16 - element + 0) & 0xF))^U8_TWIDDLE) & 0x0FFF] << 8;
	gVectRSP[op.rt]._s16[6] = g_pu8SpMemBase[((address + ((16 - element + 1) & 0xF))^U8_TWIDDLE) & 0x0FFF] << 8;
	gVectRSP[op.rt]._s16[5] = g_pu8SpMemBase[((address + ((16 - element + 2) & 0xF))^U8_TWIDDLE) & 0x0FFF] << 8;
	gVectRSP[op.rt]._s16[4] = g_pu8SpMemBase[((address + ((16 - element + 3) & 0xF))^U8_TWIDDLE) & 0x0FFF] << 8;
	gVectRSP[op.rt]._s16[3] = g_pu8SpMemBase[((address + ((16 - element + 4) & 0xF))^U8_TWIDDLE) & 0x0FFF] << 8;
	gVectRSP[op.rt]._s16[2] = g_pu8SpMemBase[((address + ((16 - element + 5) & 0xF))^U8_TWIDDLE) & 0x0FFF] << 8;
	gVectRSP[op.rt]._s16[1] = g_pu8SpMemBase[((address + ((16 - element + 6) & 0xF))^U8_TWIDDLE) & 0x0FFF] << 8;
	gVectRSP[op.rt]._s16[0] = g_pu8SpMemBase[((address + ((16 - element + 7) & 0xF))^U8_TWIDDLE) & 0x0FFF] << 8;
}


//*****************************************************************************
// Added 08/03/2002, not checked
//*****************************************************************************
void RSP_SWC2_SPV( OpCode op )
{
	const u32 element = op.del;

	u32 address = ( gRegRSP[op.base]._u32 + (u32)( op.voffset << 3 ) ) & 0x0FFF;

	for ( u32 e = element; e < (8 + element); e++ )
	{
		if ( (e & 0xF) < 8 )
		{
			g_pu8SpMemBase[(address ^ U8_TWIDDLE) & 0x0FFF] = gVectRSP[op.rt]._u8[15 - ((e & 0xF) << 1)];
		}
		else
		{
			g_pu8SpMemBase[(address ^ U8_TWIDDLE) & 0x0FFF] = (gVectRSP[op.rt]._u8[15 - ((e & 0x7) << 1)] << 1) +
				                                              (gVectRSP[op.rt]._u8[14 - ((e & 0x7) << 1)] >> 7);
		}

		address++;
	}
}

//*****************************************************************************
// Added 08/03/2002, not checked
//*****************************************************************************
void RSP_LWC2_LUV( OpCode op )
{
	const u32 element = op.del;

	u32 address = ( gRegRSP[op.base]._u32 + (u32)( op.voffset << 3 ) ) & 0x0FFF;

	gVectRSP[op.rt]._s16[7] = g_pu8SpMemBase[((address + ((16 - element + 0) & 0xF))^U8_TWIDDLE) & 0x0FFF] << 7;
	gVectRSP[op.rt]._s16[6] = g_pu8SpMemBase[((address + ((16 - element + 1) & 0xF))^U8_TWIDDLE) & 0x0FFF] << 7;
	gVectRSP[op.rt]._s16[5] = g_pu8SpMemBase[((address + ((16 - element + 2) & 0xF))^U8_TWIDDLE) & 0x0FFF] << 7;
	gVectRSP[op.rt]._s16[4] = g_pu8SpMemBase[((address + ((16 - element + 3) & 0xF))^U8_TWIDDLE) & 0x0FFF] << 7;
	gVectRSP[op.rt]._s16[3] = g_pu8SpMemBase[((address + ((16 - element + 4) & 0xF))^U8_TWIDDLE) & 0x0FFF] << 7;
	gVectRSP[op.rt]._s16[2] = g_pu8SpMemBase[((address + ((16 - element + 5) & 0xF))^U8_TWIDDLE) & 0x0FFF] << 7;
	gVectRSP[op.rt]._s16[1] = g_pu8SpMemBase[((address + ((16 - element + 6) & 0xF))^U8_TWIDDLE) & 0x0FFF] << 7;
	gVectRSP[op.rt]._s16[0] = g_pu8SpMemBase[((address + ((16 - element + 7) & 0xF))^U8_TWIDDLE) & 0x0FFF] << 7;

}

//*****************************************************************************
// Added 08/03/2002, not checked
//*****************************************************************************
void RSP_SWC2_SUV( OpCode op )
{
	const u32 element = op.del;

	u32 address = ( gRegRSP[op.base]._u32 + (u32)( op.voffset << 3 ) ) & 0x0FFF;

	for ( u32 e = element; e < (8 + element); e++ )
	{
		if ( (e & 0xF) < 8 )
		{
			g_pu8SpMemBase[(address ^ U8_TWIDDLE) & 0xFFF] = (gVectRSP[op.rt]._u8[15 - ((e & 0x7) << 1)] << 1) +
				                                             (gVectRSP[op.rt]._u8[14 - ((e & 0x7) << 1)] >> 7);
		}
		else
		{
			g_pu8SpMemBase[(address ^ U8_TWIDDLE) & 0x0FFF] = gVectRSP[op.rt]._u8[15 - ((e & 0x7) << 1)];
		}
		address++;
	}
}



//*****************************************************************************
// Added 06/03/2002, not checked
//*****************************************************************************
void RSP_LWC2_LRV( OpCode op )
{
	const u32 element = op.del;

	u32 address = ( gRegRSP[op.base]._u32 + (u32)( op.voffset << 4 ) ) & 0x0FFF;

	int length, offset;

	offset = (address & 0xF) - 1;
	length = (address & 0xF) - element;
	address &= 0xFF0;

	for ( u32 e = element; e < (length + element); e++ )
	{
		gVectRSP[op.rt]._u8[offset - e] = g_pu8SpMemBase[(address ^ U8_TWIDDLE) & 0x0FFF];
		address++;
	}
}

//*****************************************************************************
// Added 06/03/2002, not checked
//*****************************************************************************
void RSP_SWC2_SRV( OpCode op )
{
	const u32 element = op.del;

	u32 address = ( gRegRSP[op.base]._u32 + (u32)( op.voffset << 4 ) ) & 0x0FFF;

	int length, offset;

	length = (address & 0xF);
	offset = (0x10 - length) & 0xF;
	address &= 0xFF0;

	for ( u32 e = element; e < (length + element); e++ )
	{
		g_pu8SpMemBase[(address ^ U8_TWIDDLE) & 0x0FFF] = gVectRSP[op.rt]._u8[15 - ((e + offset) & 0xF)];
		address++;
	}

}



//*****************************************************************************
//
//*****************************************************************************
void RSP_LWC2_LTV( OpCode op )
{
	const u32 element = op.del;

	u32 address = ( gRegRSP[op.base]._u32 + (u32)( op.voffset << 4 ) ) & 0x0FFF;

	int del;

	u32 length = 8;

	if (length > u32(32 - op.rt))
		length = u32(32 - op.rt);

	address = ((address + 8) & 0xFF0) + (element & 0x1);

	for ( u32 count = 0; count < length; count++ )
	{
		del = ((8 - (element >> 1) + count) << 1) & 0xF;
		gVectRSP[op.rt + count]._u8[15 - del] = g_pu8SpMemBase[(address + 0) ^ U8_TWIDDLE];
		gVectRSP[op.rt + count]._u8[14 - del] = g_pu8SpMemBase[(address + 1) ^ U8_TWIDDLE];
		address += 2;
	}


}

//*****************************************************************************
//
//*****************************************************************************
void RSP_SWC2_STV( OpCode op )
{
	const u32 element = op.del;

	u32 address = ( gRegRSP[op.base]._u32 + (u32)( op.voffset << 4 ) ) & 0x0FFF;

	int del;

	u32 length = 8;

	if (length > u32(32 - op.rt))
		length = u32(32 - op.rt);

	length = length << 1;
	del = element >> 1;

	for ( u32 count = 0; count < length; count += 2)
	{
		g_pu8SpMemBase[((address + 0) ^ U8_TWIDDLE) & 0x0FFF] = gVectRSP[op.rt + del]._u8[15 - count];
		g_pu8SpMemBase[((address + 1) ^ U8_TWIDDLE) & 0x0FFF] = gVectRSP[op.rt + del]._u8[14 - count];
		del = (del + 1) & 7;
		address += 2;
	}

}






//*****************************************************************************
// Updated 05/03/2002, not checked
//*****************************************************************************
void RSP_LWC2_LBV( OpCode op )
{
	u32 address = ( gRegRSP[op.base]._u32 + (u32)( op.voffset << 0 ) ) & 0x0FFF;

	gVectRSP[op.rt]._u8[15 - op.del] = g_pu8SpMemBase[(address ^ U8_TWIDDLE) & 0x0FFF];
}

//*****************************************************************************
// Updated 05/03/2002, not checked
//*****************************************************************************
void RSP_SWC2_SBV( OpCode op )
{
	u32 address = ( gRegRSP[op.base]._u32 + (u32)( op.voffset << 0 ) ) & 0x0FFF;

	g_pu8SpMemBase[( address ^ U8_TWIDDLE ) & 0x0FFF] = gVectRSP[op.rt]._u8[15 - op.del];
}

//*****************************************************************************
// Updated 05/03/2002, not checked
//*****************************************************************************
void RSP_LWC2_LSV( OpCode op )
{
	const u32 element = op.del;

	u32 address = ( gRegRSP[op.base]._u32 + (u32)( op.voffset << 1 ) ) & 0x0FFF;

	u32 length = 2;

	if (length > 16 - element)
		length = 16 - element;

	for ( u32 e = element; e < (length + element); e++ )
	{
		gVectRSP[op.rt]._u8[15 - e] = g_pu8SpMemBase[(address ^ U8_TWIDDLE) & 0x0FFF];
		address++;
	}
}

//*****************************************************************************
// Updated 20/01/2002, not checked
//*****************************************************************************
void RSP_SWC2_SSV( OpCode op )
{
	const u32 element = op.del;

	u32 address = ( gRegRSP[op.base]._u32 + (u32)( op.voffset << 1 ) ) & 0x0FFF;

	u32 length = 2;

	for ( u32 e = element; e < (length + element); e++ )
	{
		g_pu8SpMemBase[(address ^ U8_TWIDDLE) & 0x0FFF] = gVectRSP[op.rt]._u8[15 - (e & 0xF)];
		address++;
	}
}

//*****************************************************************************
// Updated 05/03/2002, not checked
//*****************************************************************************
void RSP_LWC2_LLV( OpCode op )
{
	const u32 element = op.del;

	u32 address = ( gRegRSP[op.base]._u32 + (u32)( op.voffset << 2 ) ) & 0x0FFF;

	u32 length = 4;

	if (length > 16 - element)
		length = 16 - element;

	for ( u32 e = element; e < (length + element); e++ )
	{
		gVectRSP[op.rt]._u8[15 - e] = g_pu8SpMemBase[(address ^ U8_TWIDDLE) & 0x0FFF];
		address++;
	}
}

//*****************************************************************************
// Updated 05/03/2002, not checked
//*****************************************************************************
void RSP_SWC2_SLV( OpCode op ) 		// Store word from vector (32bits)
{
	const u32 element = op.del;

	u32 address = ( gRegRSP[op.base]._u32 + (u32)( op.voffset << 2 ) ) & 0x0FFF;

	u32 length = 4;

	for ( u32 e = element; e < (length + element); e++ )
	{
		g_pu8SpMemBase[(address ^U8_TWIDDLE) & 0x0FFF] = gVectRSP[op.rt]._u8[15 - (e & 0xF)];
		address++;
	}
}



//*****************************************************************************
// Updated 20/01/2002, not checked
//*****************************************************************************
void RSP_LWC2_LDV( OpCode op ) 			// Load Doubleword into Vector
{
	const u32 element = op.del;

	u32 address = ( gRegRSP[op.base]._u32 + (u32)( op.voffset << 3 ) ) & 0x0FFF;

	u32 length = 8;

	if (length > 16 - element)
		length = 16 - element;

	for ( u32 e = element; e < (length + element); e++ )
	{
		gVectRSP[op.rt]._u8[15 - e] = g_pu8SpMemBase[(address ^ U8_TWIDDLE) & 0x0FFF];
		address++;
	}
}


//*****************************************************************************
// Updated 20/01/2002, not checked
//*****************************************************************************
void RSP_SWC2_SDV( OpCode op ) 			// Store doubleword from vector (64 bits)
{
	const u32 element = op.del;

	u32 address = ( gRegRSP[op.base]._u32 + (u32)( op.voffset << 3 ) ) & 0x0FFF;

	u32 length = 8;

	for ( u32 e = element; e < (length + element); e++ )
	{
		g_ps8SpMemBase[(address ^ U8_TWIDDLE) & 0x0FFF] = gVectRSP[op.rt]._s8[15 - (e & 0xF)];
		address++;
	}
}



//*****************************************************************************
// Updated 20/01/2002, not checked
//*****************************************************************************
void RSP_LWC2_LQV( OpCode op )
{
	const u32 dest = op.rt;
	const u32 element = op.del;

	u32 address = ( gRegRSP[op.base]._u32 + (u32)( op.voffset << 4 ) ) & 0x0FFF;

	u32 length = ( ( address + 16 ) & ~0xF ) - address;
	if (length > 16 - element)
		length = 16 - element;

	for ( u32 e = element; e < (length + element); e++ )
	{
		gVectRSP[dest]._s8[15 - e] = g_ps8SpMemBase[(address ^ U8_TWIDDLE) & 0x0FFF];
		address++;
	}
}

//*****************************************************************************
// Updated 20/01/2002, not checked
//*****************************************************************************
void RSP_SWC2_SQV( OpCode op )
{
	const u32 vect = op.rt;
	const u32 element = op.del;

	u32 address = ( gRegRSP[op.base]._u32 + (u32)( op.voffset << 4 ) ) & 0x0FFF;

	u32 length = ( ( address + 16 ) & ~0xF ) - address;

	for ( u32 e = element; e < (length + element); e++ )
	{
		g_ps8SpMemBase[(address ^ U8_TWIDDLE) & 0x0FFF] = gVectRSP[vect]._u8[15 - (e & 0xF)];
		address++;
	}
}


//*****************************************************************************
//
//*****************************************************************************
void RSP_SWC2_SHV( OpCode op ) { WARN_NOIMPL("RSP: SHV"); }
void RSP_SWC2_SFV( OpCode op ) { WARN_NOIMPL("RSP: SFV"); }
void RSP_SWC2_SWV( OpCode op ) { WARN_NOIMPL("RSP: SWV"); }


void RSP_Cop2_Unk( OpCode op )	{ WARN_NOEXIST("RSP_Cop2_Unk"); }
void RSP_Cop2_Vector( OpCode op )	{ RSPCop2VInstruction[op.cop2_funct]( op ); }

void RSP_Cop2V_Unk( OpCode op )		{ WARN_NOEXIST("RSP_Cop2V_Unk"); }
void RSP_Cop2V_VMULU( OpCode op )	{ WARN_NOIMPL("RSP: VMULU"); }
void RSP_Cop2V_VMACU( OpCode op )	{ WARN_NOIMPL("RSP: VMACU"); }
void RSP_Cop2V_VRNDP( OpCode op )	{ WARN_NOIMPL("RSP: VRNDP"); }
void RSP_Cop2V_VMULQ( OpCode op )	{ WARN_NOIMPL("RSP: VMULQ"); }
void RSP_Cop2V_VRNDN( OpCode op )	{ WARN_NOIMPL("RSP: VRNDN"); }
void RSP_Cop2V_VMACQ( OpCode op )	{ WARN_NOIMPL("RSP: VMACQ"); }

//*****************************************************************************
// Added 06/03/2002, not checked
//*****************************************************************************
void RSP_Cop2_MFC2( OpCode op )
{
	const u32 element = (op.sa >> 1);

	gRegRSP[op.rt]._s8[1] = gVectRSP[op.rd]._s8[15 - element];
	gRegRSP[op.rt]._s8[0] = gVectRSP[op.rd]._s8[15 - ((element + 1) % 0xF)];
	gRegRSP[op.rt]._s32 = gRegRSP[op.rt]._s16[0];

}

//*****************************************************************************
// Added 20/01/2002, not checked
//*****************************************************************************
void RSP_Cop2_MTC2( OpCode op )
{
	const u32 element = 15 - ( op.sa >> 1 );

	gVectRSP[op.rd]._s8[element] = gRegRSP[op.rt]._s8[1];
	if (element != 0)
	{
		gVectRSP[op.rd]._s8[element - 1] = gRegRSP[op.rt]._s8[0];
	}
}

//*****************************************************************************
// Added 30/03/2002, not checked
//*****************************************************************************
void RSP_Cop2_CTC2( OpCode op )
{
	switch ( op.rd & 0x03 )
	{
		case 0: gFlagsRSP[0]._s16[0] = gRegRSP[op.rt]._s16[0]; break;
		case 1: gFlagsRSP[1]._s16[0] = gRegRSP[op.rt]._s16[0]; break;
		case 2: gFlagsRSP[2]._s8[0]  = gRegRSP[op.rt]._s8[0]; break;
		case 3: gFlagsRSP[2]._s8[0]  = gRegRSP[op.rt]._s8[0]; break;		//!!?? Not [3]??
	}
}

//*****************************************************************************
// Added 20/01/2002, not checked
//*****************************************************************************
void RSP_Cop2_CFC2( OpCode op )
{
	switch ( op.rd & 0x03 )
	{
		case 0: gRegRSP[op.rt]._s32 = gFlagsRSP[0]._s16[0]; break;
		case 1: gRegRSP[op.rt]._s32 = gFlagsRSP[1]._s16[0]; break;
		case 2: gRegRSP[op.rt]._s32 = gFlagsRSP[2]._s16[0]; break;
		case 3: gRegRSP[op.rt]._s32 = gFlagsRSP[2]._s16[0]; break;		//!!?? Not [3]??
	}
}


//*****************************************************************************
// Added 20/01/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VMULF( OpCode op )
{
	REG32 temp;

	for ( u32 v = 0; v < 8; v++ )
	{
		u32 el  = Indx[op.rs]._u8[v];						// Indx[n] is a permutation of 0..7
		u32 del = EleSpec[op.rs]._u8[el];

		if ( gVectRSP[op.rd]._u16[el] != 0x8000 ||
			 gVectRSP[op.rt]._u16[del] != 0x8000 )
		{
			temp._s32 = ( (s32)gVectRSP[op.rd]._s16[el] * (s32)gVectRSP[op.rt]._s16[del] ) << 1;
			temp._u32 += 0x8000;

			gAccuRSP[el]._s16[2] = temp._s16[1];
			gAccuRSP[el]._s16[1] = temp._s16[0];

			// Sign extend the top 16 bits of the accumulator
			if ( gAccuRSP[el]._s16[2] < 0 )
				gAccuRSP[el]._s16[3] = -1;
			else
				gAccuRSP[el]._s16[3] = 0;

			gVectRSP[op.sa]._s16[el] = gAccuRSP[el]._s16[2];
		}
		else
		{
			temp._s32 = 0x80000000;
			gAccuRSP[el]._u16[3] = 0;
			gAccuRSP[el]._u16[2] = 0x8000;
			gAccuRSP[el]._u16[1] = 0x8000;
			gVectRSP[op.sa]._s16[el] = 0x7FFF;
		}
	}
}

//*****************************************************************************
// Added 20/01/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VMACF( OpCode op )
{
	REG32 temp;

	for ( u32 v = 0; v < 8; v++ )
	{
		u32 el = Indx[op.rs]._u8[v];
		u32 del = EleSpec[op.rs]._u8[el];

		temp._s32 = (s32)gVectRSP[op.rd]._s16[el] * (s32)(u32)gVectRSP[op.rt]._s16[del];
		gAccuRSP[el]._s64 += ((s64)temp._s32) << 17;
		if ( gAccuRSP[el]._s16[3] < 0 )
		{
			if ( gAccuRSP[el]._u16[3] != 0xFFFF )
			{
				gVectRSP[op.sa]._s16[el] = (u16)0x8000;
			}
			else
			{
				if ( gAccuRSP[el]._s16[2] >= 0 )
					gVectRSP[op.sa]._s16[el] = (u16)0x8000;
				else
					gVectRSP[op.sa]._s16[el] = gAccuRSP[el]._s16[2];
			}
		}
		else
		{
			if ( gAccuRSP[el]._u16[3] != 0 )
			{
				gVectRSP[op.sa]._s16[el] = 0x7FFF;
			}
			else
			{
				if ( gAccuRSP[el]._s16[2] < 0 )
					gVectRSP[op.sa]._s16[el] = 0x7FFF;
				else
					gVectRSP[op.sa]._s16[el] = gAccuRSP[el]._s16[2];
			}
		}
	}

}


//*****************************************************************************
// Added 05/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VMUDH( OpCode op )
{
	int el, del;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		gAccuRSP[el]._s32_1 = (s32)gVectRSP[op.rd]._s16[el] * (s32)gVectRSP[op.rt]._s16[del]; 
		gAccuRSP[el]._s16[1] = 0;
		if (gAccuRSP[el]._s16[3] < 0)
		{
			if (gAccuRSP[el]._u16[3] != 0xFFFF)
			{
				gVectRSP[op.sa]._s16[el] = (u16)0x8000;
			}
			else
			{
				if (gAccuRSP[el]._s16[2] >= 0)
					gVectRSP[op.sa]._s16[el] = (u16)0x8000;
				else
					gVectRSP[op.sa]._s16[el] = gAccuRSP[el]._s16[2];
			}
		}
		else
		{
			if (gAccuRSP[el]._u16[3] != 0)
			{
				gVectRSP[op.sa]._s16[el] = 0x7FFF;
			}
			else
			{
				if (gAccuRSP[el]._s16[2] < 0)
					gVectRSP[op.sa]._s16[el] = 0x7FFF;
				else
					gVectRSP[op.sa]._s16[el] = gAccuRSP[el]._s16[2];
			}
		}
	}
}

//*****************************************************************************
// Added 05/03/2002, not checked
//*****************************************************************************
//0xa4001210: <0x4b018606> VMUDN     vec24  = ( acc  = vec16 * vec01[0 ]       ) >> 16
void RSP_Cop2V_VMUDN( OpCode op )
{
	int el, del;
	REG32 temp;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		temp._u32 = (u32)gVectRSP[op.rd]._u16[el] * (u32)((s32)gVectRSP[op.rt]._s16[del]);
		if (temp._s32 < 0)
			gAccuRSP[el]._s16[3] = -1;
		else
			gAccuRSP[el]._s16[3] = 0;

		gAccuRSP[el]._s16[2] = temp._s16[1];
		gAccuRSP[el]._s16[1] = temp._s16[0];
		gVectRSP[op.sa]._s16[el] = gAccuRSP[el]._s16[1];
	}
}

//*****************************************************************************
// Added 05/03/2002, not checked
//*****************************************************************************
// Same as VMUDH, but accumulates
void RSP_Cop2V_VMADN( OpCode op )
{
	int el, del;
	REG32 temp, temp2;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		temp._u32 = (u32)gVectRSP[op.rd]._u16[el] * (u32)((s32)gVectRSP[op.rt]._s16[del]);
		temp2._u32 = temp._u16[0] + gAccuRSP[el]._u16[1];
		gAccuRSP[el]._s16[1] = temp2._s16[0];
		temp2._u32 = temp._u16[1] + gAccuRSP[el]._u16[2] + temp2._u16[1];
		gAccuRSP[el]._s16[2] = temp2._s16[0];
		gAccuRSP[el]._s16[3] += temp2._s16[1];

		if (temp._s32 < 0)
		{
			gAccuRSP[el]._s16[3] -= 1;
		}

		if (gAccuRSP[el]._s16[3] < 0)
		{
			if (gAccuRSP[el]._u16[3] != 0xFFFF)
			{
				gVectRSP[op.sa]._s16[el] = 0;
			}
			else
			{
				if (gAccuRSP[el]._s16[2] >= 0)
					gVectRSP[op.sa]._s16[el] = 0;
				else
					gVectRSP[op.sa]._s16[el] = gAccuRSP[el]._s16[1];
			}
		}
		else
		{
			if (gAccuRSP[el]._u16[3] != 0)
			{
				gVectRSP[op.sa]._u16[el] = 0xFFFF;
			}
			else
			{
				if (gAccuRSP[el]._s16[2] < 0)
					gVectRSP[op.sa]._u16[el] = 0xFFFF;
				else
					gVectRSP[op.sa]._s16[el] = gAccuRSP[el]._s16[1];
			}
		}
	}

}


//*****************************************************************************
// Added 05/03/2002, not checked
//*****************************************************************************
// Same as VMUDH, but accumulates
//1648: <4BFF108F>  vmadh      vec02  = ( acc+= vec02 * vec31[7 ] >>16)>>16
void RSP_Cop2V_VMADH( OpCode op )
{
	int el, del;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		gAccuRSP[el]._s32_1 += (s32)gVectRSP[op.rd]._s16[el] * (s32)gVectRSP[op.rt]._s16[del]; 
		if (gAccuRSP[el]._s16[3] < 0)
		{
			if (gAccuRSP[el]._u16[3] != 0xFFFF)
			{
				gVectRSP[op.sa]._s16[el] = (u16)0x8000;
			}
			else
			{
				if (gAccuRSP[el]._s16[2] >= 0)
					gVectRSP[op.sa]._s16[el] = (u16)0x8000;
				else
					gVectRSP[op.sa]._s16[el] = gAccuRSP[el]._s16[2];
			}
		}
		else
		{
			if (gAccuRSP[el]._u16[3] != 0)
			{
				gVectRSP[op.sa]._s16[el] = 0x7FFF;
			}
			else
			{
				if (gAccuRSP[el]._s16[2] < 0)
					gVectRSP[op.sa]._s16[el] = 0x7FFF;
				else
					gVectRSP[op.sa]._s16[el] = gAccuRSP[el]._s16[2];
			}
		}
	}

}


//*****************************************************************************
// Added 05/03/2002, not checked
//*****************************************************************************
//0xa40016b0: <0x4b04cc05> VMUDM     vec16  = ( acc  = vec25 * vec04[0 ] >> 16 )
void RSP_Cop2V_VMUDM( OpCode op )
{
	int el, del;
	REG32 temp;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		temp._u32 = (u32)((s32)gVectRSP[op.rd]._s16[el]) * (u32)gVectRSP[op.rt]._u16[del];
		if (temp._s32 < 0)
			gAccuRSP[el]._s16[3] = -1;
		else
			gAccuRSP[el]._s16[3] = 0;

		gAccuRSP[el]._s16[2] = temp._s16[1];
		gAccuRSP[el]._s16[1] = temp._s16[0];
		gVectRSP[op.sa]._s16[el] = gAccuRSP[el]._s16[2];
	}
}

//*****************************************************************************
// Added 05/03/2002, not checked
//*****************************************************************************
//15E0: <4A05174D>  vmadm      vec29  = ( acc+= vec02 * vec05[0a] >>16)
void RSP_Cop2V_VMADM( OpCode op )
{
	int el, del;
	REG32 temp, temp2;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		temp._u32 = (u32)((s32)gVectRSP[op.rd]._s16[el]) * (u32)gVectRSP[op.rt]._u16[del];
		temp2._u32 = temp._u16[0] + gAccuRSP[el]._u16[1];
		gAccuRSP[el]._s16[1] = temp2._s16[0];
		temp2._u32 = temp._u16[1] + gAccuRSP[el]._u16[2] + temp2._u16[1];
		gAccuRSP[el]._s16[2] = temp2._s16[0];
		gAccuRSP[el]._s16[3] += temp2._s16[1];

		if (temp._s32 < 0)
		{
			gAccuRSP[el]._s16[3] -= 1;
		}

		if (gAccuRSP[el]._s16[3] < 0)
		{
			if (gAccuRSP[el]._u16[3] != 0xFFFF)
			{
				gVectRSP[op.sa]._s16[el] = (u16)0x8000;
			}
			else
			{
				if (gAccuRSP[el]._s16[2] >= 0)
					gVectRSP[op.sa]._s16[el] = (u16)0x8000;
				else
					gVectRSP[op.sa]._s16[el] = gAccuRSP[el]._s16[2];
			}
		}
		else
		{
			if (gAccuRSP[el]._u16[3] != 0)
			{
				gVectRSP[op.sa]._s16[el] = 0x7FFF;
			}
			else
			{
				if (gAccuRSP[el]._s16[2] < 0)
					gVectRSP[op.sa]._s16[el] = 0x7FFF;
				else
					gVectRSP[op.sa]._s16[el] = gAccuRSP[el]._s16[2];

			}
		}
	}
}

//*****************************************************************************
//
//*****************************************************************************
//15D8: <4A05AF44>  vmudl      vec29  = ( acc = vec21 * vec05[0a]     )
void RSP_Cop2V_VMUDL( OpCode op )
{
	int el, del;
	REG32 temp;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		temp._u32 = (u32)gVectRSP[op.rd]._u16[el] * (u32)gVectRSP[op.rt]._u16[del];
		gAccuRSP[el]._s32_1 = 0;
		gAccuRSP[el]._s16[1] = temp._s16[1];
		gVectRSP[op.sa]._s16[el] = gAccuRSP[el]._s16[1];
	}
}


//*****************************************************************************
// Added 05/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VMADL( OpCode op )
{
	int el, del;
	REG32 temp, temp2;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		temp._u32 = (u32)gVectRSP[op.rd]._u16[el] * (u32)gVectRSP[op.rt]._u16[del];
		temp2._u32 = temp._u16[1] + gAccuRSP[el]._u16[1];
		gAccuRSP[el]._s16[1] = temp2._s16[0];
		temp2._u32 = gAccuRSP[el]._u16[2] + temp2._u16[1];
		gAccuRSP[el]._s16[2] = temp2._s16[0];
		gAccuRSP[el]._s16[3] += temp2._s16[1];

		if (gAccuRSP[el]._s16[3] < 0)
		{
			if (gAccuRSP[el]._u16[3] != 0xFFFF)
			{
				gVectRSP[op.sa]._s16[el] = 0;
			}
			else
			{
				if (gAccuRSP[el]._s16[2] >= 0)
					gVectRSP[op.sa]._s16[el] = 0;
				else
					gVectRSP[op.sa]._s16[el] = gAccuRSP[el]._s16[1];
			}
		}
		else
		{
			if (gAccuRSP[el]._u16[3] != 0)
			{
				gVectRSP[op.sa]._u16[el] = 0xFFFF;
			}
			else
			{
				if (gAccuRSP[el]._s16[2] < 0)
					gVectRSP[op.sa]._u16[el] = 0xFFFF;
				else
					gVectRSP[op.sa]._s16[el] = gAccuRSP[el]._s16[1];
			}
		}
	}

}



//*****************************************************************************
//
//*****************************************************************************
void RSP_Cop2V_VADD( OpCode op )
{
	int el, del;
	REG32 temp;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		temp._s32 = (s32)gVectRSP[op.rd]._s16[el] + (s32)gVectRSP[op.rt]._s16[del] + ((gFlagsRSP[0]._u32 >> (7 - el)) & 0x1);
		gAccuRSP[el]._s16[1] = temp._s16[0];
		if ((temp._s16[0] & 0x8000) == 0)
		{
			if (temp._s16[1] != 0)
				gVectRSP[op.sa]._s16[el] = (u16)0x8000;
			else
				gVectRSP[op.sa]._s16[el] = temp._s16[0];
		}
		else
		{
			if (temp._s16[1] != -1 )
				gVectRSP[op.sa]._s16[el] = 0x7FFF;
			else
				gVectRSP[op.sa]._s16[el] = temp._s16[0];
		}
	}
	gFlagsRSP[0]._u32 = 0;
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_Cop2V_VADDC( OpCode op )
{
	int el, del;
	REG32 temp;

	gFlagsRSP[0]._u32 = 0;
	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		temp._u32 = (s32)gVectRSP[op.rd]._u16[el] + (s32)gVectRSP[op.rt]._u16[del];
		gAccuRSP[el]._s16[1] = temp._s16[0];
		gVectRSP[op.sa]._s16[el] = temp._s16[0];

		if (temp._u32 & 0xffff0000)
		{
			gFlagsRSP[0]._u32 |= ( 1 << (7 - el) );
		}
	}
}



//*****************************************************************************
// Added 05/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VSUB( OpCode op )
{
	int el, del;
	REG32 temp;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		temp._s32 = (s32)gVectRSP[op.rd]._s16[el] - (s32)gVectRSP[op.rt]._s16[del] - ((gFlagsRSP[0]._s32 >> (7 - el)) & 0x1);
		gAccuRSP[el]._s16[1] = temp._s16[0];

		if ((temp._s16[0] & 0x8000) == 0)
		{
			if (temp._s16[1] != 0)
				gVectRSP[op.sa]._s16[el] = (u16)0x8000;
			else
				gVectRSP[op.sa]._s16[el] = temp._s16[0];
		}
		else
		{
			if (temp._s16[1] != -1 )
				gVectRSP[op.sa]._s16[el] = 0x7FFF;
			else
				gVectRSP[op.sa]._s16[el] = temp._s16[0];
		}
	}
	gFlagsRSP[0]._u32 = 0;
}

//*****************************************************************************
// Added 06/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VSUBC( OpCode op )
{
	int el, del;
	REG32 temp;

	gFlagsRSP[0]._u32 = 0;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		temp._u32 = (s32)gVectRSP[op.rd]._u16[el] - (s32)gVectRSP[op.rt]._u16[del];
		gAccuRSP[el]._s16[1] = temp._s16[0];
		gVectRSP[op.sa]._s16[el] = temp._s16[0];

		if (temp._s16[0] != 0)
		{
			gFlagsRSP[0]._u32 |= ( 1 << (15 - el) );
		}

		if (temp._u32 & 0xffff0000)
		{
			gFlagsRSP[0]._u32 |= ( 1 << (7 - el) );
		}
	}
}




//*****************************************************************************
//
//*****************************************************************************
void RSP_Cop2V_VSUT( OpCode op )	{ WARN_NOIMPL("RSP: VSUT"); }
void RSP_Cop2V_VADDB( OpCode op )	{ WARN_NOIMPL("RSP: VADDB"); }
void RSP_Cop2V_VSUBB( OpCode op )	{ WARN_NOIMPL("RSP: VACCB"); }
void RSP_Cop2V_VACCB( OpCode op )	{ WARN_NOIMPL("RSP: VACCB"); }
void RSP_Cop2V_VSUCB( OpCode op )	{ WARN_NOIMPL("RSP: VSUCB"); }
void RSP_Cop2V_VSAD( OpCode op )	{ WARN_NOIMPL("RSP: VSAD"); }
void RSP_Cop2V_VSAC( OpCode op )	{ WARN_NOIMPL("RSP: VSAC"); }
void RSP_Cop2V_VSUM( OpCode op )	{ WARN_NOIMPL("RSP: VSUM"); }
void RSP_Cop2V_VEQ( OpCode op )		{ WARN_NOIMPL("RSP: VEQ"); }
void RSP_Cop2V_VNE( OpCode op )		{ WARN_NOIMPL("RSP: VNE"); }

// Vector reciprocal

REG32 gReciprocal;
REG32 gReciprocalResult;

//*****************************************************************************
// Added 06/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VRCP( OpCode op )
{
	gReciprocalResult._s32 = gVectRSP[op.rt]._s16[EleSpec[op.rs]._s8[(op.rd & 0x7)]];

	if (gReciprocalResult._u32 == 0)
	{
		gReciprocalResult._u32 = 0x7FFFFFFF;
	}
	else
	{
		int bit;
		bool neg;

		if (gReciprocalResult._s32 < 0)
		{
			neg = true;
			gReciprocalResult._s32 = ~gReciprocalResult._s32 + 1;
		}
		else
		{
			neg = false;
		}

		for (bit = 15; bit > 0; bit--)
		{
			if ((gReciprocalResult._u32 & (1 << bit)))
			{
				gReciprocalResult._u32 &= (0xFFC0 >> (15 - bit) );
				break;
			}
		}

		gReciprocalResult._s32 = (s32)((0x7FFFFFFF / (f32)gReciprocalResult._s32));

		for (bit = 31; bit > 0; bit--)
		{
			if ((gReciprocalResult._u32 & (1 << bit)))
			{
				gReciprocalResult._u32 &= (0xFFFF8000 >> (31 - bit) );
				break;
			}
		}

		if ( neg )
		{
			gReciprocalResult._u32 = ~gReciprocalResult._u32;
		}
	}

	for ( u32 v = 0; v < 8; v++ )
	{
		gAccuRSP[v]._s16[1] = gVectRSP[op.rt]._u16[EleSpec[op.rs]._s8[v]];
	}
	gVectRSP[op.sa]._s16[7 - (op.rd & 0x7)] = gReciprocalResult._u16[0];
}

//*****************************************************************************
// Added 06/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VRCPL( OpCode op )
{

	gReciprocalResult._u32 = gVectRSP[op.rt]._u16[EleSpec[op.rs]._s8[(op.rd & 0x7)]] | gReciprocal._s32;
	if (gReciprocalResult._u32 == 0)
	{
		gReciprocalResult._u32 = 0x7FFFFFFF;
	}
	else
	{
		s32 bit;
		bool neg;

		if (gReciprocalResult._s32 < 0)
		{
			neg = true;
			if (gReciprocalResult._u16[1] == 0xFFFF && gReciprocalResult._s16[0] < 0) {
				gReciprocalResult._s32 = ~gReciprocalResult._s32 + 1;
			}
			else
			{
				gReciprocalResult._s32 = ~gReciprocalResult._s32;
			}
		}
		else
		{
			neg = false;
		}

		for ( bit = 31; bit > 0; bit--)
		{
			if ((gReciprocalResult._u32 & (1 << bit)))
			{
				gReciprocalResult._u32 &= (0xFFC00000 >> (31 - bit) );
				break;
			}
		}

		gReciprocalResult._s32 = 0x7FFFFFFF / gReciprocalResult._s32;

		for ( bit = 31; bit > 0; bit--)
		{
			if ((gReciprocalResult._u32 & (1 << bit)))
			{
				gReciprocalResult._u32 &= (0xFFFF8000 >> (31 - bit) );
				break;
			}
		}

		if ( neg )
		{
			gReciprocalResult._u32 = ~gReciprocalResult._u32;
		}
	}

	for ( u32 v = 0; v < 8; v++ )
	{
		gAccuRSP[v]._s16[1] = gVectRSP[op.rt]._u16[EleSpec[op.rs]._s8[v]];
	}
	gVectRSP[op.sa]._s16[7 - (op.rd & 0x7)] = gReciprocalResult._u16[0];
}


//*****************************************************************************
// Added 06/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VRCPH( OpCode op )
{
	gReciprocal._u16[1] = gVectRSP[op.rt]._u16[EleSpec[op.rs]._s8[(op.rd & 0x7)]];

	for ( u32 v = 0; v < 8; v++ )
	{
		gAccuRSP[v]._u16[1] = gVectRSP[op.rt]._u16[EleSpec[op.rs]._s8[v]];
	}

	gVectRSP[op.sa]._u16[7 - (op.rd & 0x7)] = gReciprocalResult._u16[1];
}





// Vector Square root
void RSP_Cop2V_VRSQ( OpCode op )	{ WARN_NOIMPL("RSP: VRSQ"); }
void RSP_Cop2V_VRSQL( OpCode op )	{ WARN_NOIMPL("RSP: VRSQL"); }
void RSP_Cop2V_VRSQH( OpCode op )	{ WARN_NOIMPL("RSP: VRSQH"); }

//*****************************************************************************
// Added 10/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VABS( OpCode op )
{
	int el, del;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		if (gVectRSP[op.rd]._s16[el] > 0)
		{
			gVectRSP[op.sa]._s16[el] = gVectRSP[op.rt]._u16[del];
		}
		else if (gVectRSP[op.rd]._s16[el] < 0)
		{
			if (gVectRSP[op.rt]._u16[del] == 0x8000)
				gVectRSP[op.sa]._s16[el] = 0x7FFF;
			else
				gVectRSP[op.sa]._s16[el] = gVectRSP[op.rt]._s16[del] * -1;
		}
		else
		{
			gVectRSP[op.sa]._s16[el] = 0;
		}

		gAccuRSP[el]._s16[1] = gVectRSP[op.sa]._s16[el];
	}
}

//*****************************************************************************
// Added 06/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VMOV( OpCode op )
{
	gVectRSP[op.sa]._u16[7 - (op.rd & 0x7)] = gVectRSP[op.rt]._u16[EleSpec[op.rs]._s8[(op.rd & 0x7)]];
}


//*****************************************************************************
// Added 06/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VMRG( OpCode op )
{
	int el, del;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		// Select element from op.rd or op.rt depending on flag
		if ((gFlagsRSP[1]._u32 & ( 1 << (7 - el))) != 0)
		{
			gVectRSP[op.sa]._u16[el] = gVectRSP[op.rd]._u16[el];
		}
		else
		{
			gVectRSP[op.sa]._u16[el] = gVectRSP[op.rt]._u16[del];
		}
	}
}

//*****************************************************************************
// Added 06/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VSAW( OpCode op )
{
	// Copy from accumulator
	switch ((op.rs & 0xF))
	{
	case 8:
		gVectRSP[op.sa]._s16[0] = gAccuRSP[0]._s16[3];
		gVectRSP[op.sa]._s16[1] = gAccuRSP[1]._s16[3];
		gVectRSP[op.sa]._s16[2] = gAccuRSP[2]._s16[3];
		gVectRSP[op.sa]._s16[3] = gAccuRSP[3]._s16[3];
		gVectRSP[op.sa]._s16[4] = gAccuRSP[4]._s16[3];
		gVectRSP[op.sa]._s16[5] = gAccuRSP[5]._s16[3];
		gVectRSP[op.sa]._s16[6] = gAccuRSP[6]._s16[3];
		gVectRSP[op.sa]._s16[7] = gAccuRSP[7]._s16[3];
		break;
	case 9:
		gVectRSP[op.sa]._s16[0] = gAccuRSP[0]._s16[2];
		gVectRSP[op.sa]._s16[1] = gAccuRSP[1]._s16[2];
		gVectRSP[op.sa]._s16[2] = gAccuRSP[2]._s16[2];
		gVectRSP[op.sa]._s16[3] = gAccuRSP[3]._s16[2];
		gVectRSP[op.sa]._s16[4] = gAccuRSP[4]._s16[2];
		gVectRSP[op.sa]._s16[5] = gAccuRSP[5]._s16[2];
		gVectRSP[op.sa]._s16[6] = gAccuRSP[6]._s16[2];
		gVectRSP[op.sa]._s16[7] = gAccuRSP[7]._s16[2];
		break;
	case 10:
		gVectRSP[op.sa]._s16[0] = gAccuRSP[0]._s16[1];
		gVectRSP[op.sa]._s16[1] = gAccuRSP[1]._s16[1];
		gVectRSP[op.sa]._s16[2] = gAccuRSP[2]._s16[1];
		gVectRSP[op.sa]._s16[3] = gAccuRSP[3]._s16[1];
		gVectRSP[op.sa]._s16[4] = gAccuRSP[4]._s16[1];
		gVectRSP[op.sa]._s16[5] = gAccuRSP[5]._s16[1];
		gVectRSP[op.sa]._s16[6] = gAccuRSP[6]._s16[1];
		gVectRSP[op.sa]._s16[7] = gAccuRSP[7]._s16[1];
		break;
	default:
		gVectRSP[op.sa]._u64[0] = 0;
		gVectRSP[op.sa]._u64[1] = 0;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_Cop2V_VCR( OpCode op )
{
	int count, el, del;

	gFlagsRSP[0]._u32 = 0;
	gFlagsRSP[1]._u32 = 0;
	gFlagsRSP[2]._u32 = 0;

	for (count = 0;count < 8; count++)
	{
		el = Indx[op.rs]._s8[count];
		del = EleSpec[op.rs]._s8[el];

		if ((gVectRSP[op.rd]._s16[el] ^ gVectRSP[op.rt]._s16[del]) < 0)
		{
			if (gVectRSP[op.rt]._s16[del] < 0)
			{
				gFlagsRSP[1]._u32 |= ( 1 << (15 - el));
			}

			if (gVectRSP[op.rd]._s16[el] + gVectRSP[op.rt]._s16[del] <= 0)
			{
				gAccuRSP[el]._s16[1] = ~gVectRSP[op.rt]._u16[del];
				gFlagsRSP[1]._u32 |= ( 1 << (7 - el));
			}
			else
			{
				gAccuRSP[el]._s16[1] = gVectRSP[op.rd]._s16[el];
			}

		}
		else
		{
			if (gVectRSP[op.rt]._s16[del] < 0)
			{
				gFlagsRSP[1]._u32 |= ( 1 << (7 - el));
			}

			if (gVectRSP[op.rd]._s16[el] - gVectRSP[op.rt]._s16[del] >= 0)
			{
				gAccuRSP[el]._s16[1] = gVectRSP[op.rt]._u16[del];
				gFlagsRSP[1]._u32 |= ( 1 << (15 - el));
			}
			else
			{
				gAccuRSP[el]._s16[1] = gVectRSP[op.rd]._s16[el];
			}
		}

		gVectRSP[op.sa]._s16[el] = gAccuRSP[el]._s16[1];
	}
}


//*****************************************************************************
// Added 06/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VGE( OpCode op )
{
	int el, del;

	gFlagsRSP[1]._u32 = 0;
	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		if (gVectRSP[op.rd]._s16[el] == gVectRSP[op.rt]._s16[del])
		{
			gVectRSP[op.sa]._s16[el] = gVectRSP[op.rd]._u16[el];
			if ( (gFlagsRSP[0]._u32 & (0x101 << (7 - el))) == (u16)(0x101 << (7 - el)))
			{
				gFlagsRSP[1]._u32 &= ~( 1 << (7 - el) );
			}
			else
			{
				gFlagsRSP[1]._u32 |= ( 1 << (7 - el) );
			}
		}
		else if (gVectRSP[op.rd]._s16[el] > gVectRSP[op.rt]._s16[del])
		{
			gVectRSP[op.sa]._s16[el] = gVectRSP[op.rd]._u16[el];
			gFlagsRSP[1]._u32 |= ( 1 << (7 - el) );
		}
		else
		{
			gVectRSP[op.sa]._s16[el] = gVectRSP[op.rt]._u16[del];
			gFlagsRSP[1]._u32 &= ~( 1 << (7 - el) );
		}
		gAccuRSP[el]._s16[1] = gVectRSP[op.sa]._s16[el];
	}
	gFlagsRSP[0]._u32 = 0;
}

//*****************************************************************************
// Added 06/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VLT( OpCode op )
{
	int el, del;

	gFlagsRSP[1]._u32 = 0;
	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		if (gVectRSP[op.rd]._s16[el] < gVectRSP[op.rt]._s16[del])
		{
			gVectRSP[op.sa]._s16[el] = gVectRSP[op.rd]._u16[el];
			gFlagsRSP[1]._u32 |= ( 1 << (7 - el) );
		}
		else if (gVectRSP[op.rd]._s16[el] != gVectRSP[op.rt]._s16[del])
		{
			gVectRSP[op.sa]._s16[el] = gVectRSP[op.rt]._u16[del];
			gFlagsRSP[1]._u32 &= ~( 1 << (7 - el) );
		}
		else
		{
			gVectRSP[op.sa]._s16[el] = gVectRSP[op.rd]._u16[el];
			if ( (gFlagsRSP[0]._u32 & (0x101 << (7 - el))) == (u16)(0x101 << (7 - el)))
			{
				gFlagsRSP[1]._u32 |= ( 1 << (7 - el) );
			}
			else
			{
				gFlagsRSP[1]._u32 &= ~( 1 << (7 - el) );
			}
		}
		gAccuRSP[el]._s16[1] = gVectRSP[op.sa]._s16[el];
	}
	gFlagsRSP[0]._u32 = 0;
}

//*****************************************************************************
// Added 05/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VCL( OpCode op )
{
	int el, del;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		if ((gFlagsRSP[0]._u32 & ( 1 << (7 - el))) != 0 )
		{
			if ((gFlagsRSP[0]._u32 & ( 1 << (15 - el))) != 0 )
			{
				if ((gFlagsRSP[1]._u32 & ( 1 << (7 - el))) != 0 )
					gAccuRSP[el]._s16[1] = -gVectRSP[op.rt]._u16[del];
				else
					gAccuRSP[el]._s16[1] = gVectRSP[op.rd]._s16[el];

			}
			else
			{
				if ((gFlagsRSP[2]._u32 & ( 1 << (7 - el))))
				{
					if ( gVectRSP[op.rd]._u16[el] + gVectRSP[op.rt]._u16[del] > 0x10000)
					{
						gAccuRSP[el]._s16[1] = gVectRSP[op.rd]._s16[el];
						gFlagsRSP[1]._u32 &= ~(1 << (7 - el));
					}
					else
					{
						gAccuRSP[el]._s16[1] = -gVectRSP[op.rt]._u16[del];
						gFlagsRSP[1]._u32 |= (1 << (7 - el));
					}
				}
				else
				{
					if (gVectRSP[op.rt]._u16[del] + gVectRSP[op.rd]._u16[el] != 0)
					{
						gAccuRSP[el]._s16[1] = gVectRSP[op.rd]._s16[el];
						gFlagsRSP[1]._u32 &= ~(1 << (7 - el));
					}
					else
					{
						gAccuRSP[el]._s16[1] = -gVectRSP[op.rt]._u16[del];
						gFlagsRSP[1]._u32 |= (1 << (7 - el));
					}
				}
			}
		}
		else
		{
			if ((gFlagsRSP[0]._u32 & ( 1 << (15 - el))) != 0 )
			{
				if ((gFlagsRSP[1]._u32 & ( 1 << (15 - el))) != 0 )
					gAccuRSP[el]._s16[1] = gVectRSP[op.rt]._s16[del];
				else
					gAccuRSP[el]._s16[1] = gVectRSP[op.rd]._s16[el];

			}
			else
			{
				if ( gVectRSP[op.rd]._u16[el] - gVectRSP[op.rt]._u16[del] >= 0)
				{
					gAccuRSP[el]._s16[1] = gVectRSP[op.rt]._u16[del];
					gFlagsRSP[1]._u32 |= (1 << (15 - el));
				}
				else
				{
					gAccuRSP[el]._s16[1] = gVectRSP[op.rd]._s16[el];
					gFlagsRSP[1]._u32 &= ~(1 << (15 - el));
				}
			}
		}
		gVectRSP[op.sa]._s16[el] = gAccuRSP[el]._s16[1];
	}
	gFlagsRSP[0]._u32 = 0;
	gFlagsRSP[2]._u32 = 0;
}

//*****************************************************************************
// Added 05/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VCH( OpCode op )
{
	int el, del;

	gFlagsRSP[0]._u32 = 0;
	gFlagsRSP[1]._u32 = 0;
	gFlagsRSP[2]._u32 = 0;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		if ((gVectRSP[op.rd]._s16[el] ^ gVectRSP[op.rt]._s16[del]) < 0)
		{
			gFlagsRSP[0]._u32 |= ( 1 << (7 - el));

			if (gVectRSP[op.rt]._s16[del] < 0)
			{
				gFlagsRSP[1]._u32 |= ( 1 << (15 - el));
			}

			if (gVectRSP[op.rd]._s16[el] + gVectRSP[op.rt]._s16[del] <= 0)
			{
				if (gVectRSP[op.rd]._s16[el] + gVectRSP[op.rt]._s16[del] == -1)
				{
					gFlagsRSP[2]._u32 |= ( 1 << (7 - el));
				}
				gFlagsRSP[1]._u32 |= ( 1 << (7 - el));
				gVectRSP[op.sa]._s16[el] = -gVectRSP[op.rt]._u16[del];
			}
			else
			{
				gVectRSP[op.sa]._s16[el] = gVectRSP[op.rd]._s16[el];
			}

			if (gVectRSP[op.rd]._s16[el] + gVectRSP[op.rt]._s16[del] != 0)
			{
				if (gVectRSP[op.rd]._s16[el] != ~gVectRSP[op.rt]._s16[del])
				{
					gFlagsRSP[0]._u32 |= ( 1 << (15 - el));
				}
			}
		}
		else
		{
			if (gVectRSP[op.rt]._s16[del] < 0)
			{
				gFlagsRSP[1]._u32 |= ( 1 << (7 - el));
			}

			if (gVectRSP[op.rd]._s16[el] - gVectRSP[op.rt]._s16[del] >= 0)
			{
				gFlagsRSP[1]._u32 |= ( 1 << (15 - el));
				gVectRSP[op.sa]._s16[el] = gVectRSP[op.rt]._u16[del];
			}
			else
			{
				gVectRSP[op.sa]._s16[el] = gVectRSP[op.rd]._s16[el];
			}

			if (gVectRSP[op.rd]._s16[el] - gVectRSP[op.rt]._s16[del] != 0)
			{
				if (gVectRSP[op.rd]._s16[el] != ~gVectRSP[op.rt]._s16[del])
				{
					gFlagsRSP[0]._u32 |= ( 1 << (15 - el));
				}
			}
		}
		gAccuRSP[el]._s16[1] = gVectRSP[op.sa]._s16[el];
	}
}


//*****************************************************************************
// Added 05/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VAND( OpCode op )
{
	int el, del;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		gVectRSP[op.sa]._s16[el] = gVectRSP[op.rd]._s16[el] & gVectRSP[op.rt]._s16[del];
		gAccuRSP[el]._s16[1] = gVectRSP[op.sa]._s16[el];
	}
}

//*****************************************************************************
// Added 05/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VNAND( OpCode op )
{
	int el, del;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		gVectRSP[op.sa]._s16[el] = ~(gVectRSP[op.rd]._s16[el] & gVectRSP[op.rt]._s16[del]);
		gAccuRSP[el]._s16[1] = gVectRSP[op.sa]._s16[el];
	}
}

//*****************************************************************************
// Added 05/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VOR( OpCode op )
{
	int el, del;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		gVectRSP[op.sa]._s16[el] = gVectRSP[op.rd]._s16[el] | gVectRSP[op.rt]._s16[del];
		gAccuRSP[el]._s16[1] = gVectRSP[op.sa]._s16[el];
	}
}

//*****************************************************************************
// Added 05/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VNOR( OpCode op )
{
	int el, del;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		gVectRSP[op.sa]._s16[el] = ~(gVectRSP[op.rd]._s16[el] | gVectRSP[op.rt]._s16[del]);
		gAccuRSP[el]._s16[1] = gVectRSP[op.sa]._s16[el];
	}
}

//*****************************************************************************
// Added 05/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VXOR( OpCode op )
{
	int el, del;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		gVectRSP[op.sa]._s16[el] = gVectRSP[op.rd]._s16[el] ^ gVectRSP[op.rt]._s16[del];
		gAccuRSP[el]._s16[1] = gVectRSP[op.sa]._s16[el];
	}

}

//*****************************************************************************
// Added 05/03/2002, not checked
//*****************************************************************************
void RSP_Cop2V_VNXOR( OpCode op )
{
	int el, del;

	for ( u32 v = 0; v < 8; v++ )
	{
		el = Indx[op.rs]._s8[v];
		del = EleSpec[op.rs]._s8[el];

		gVectRSP[op.sa]._s16[el] = ~(gVectRSP[op.rd]._s16[el] ^ gVectRSP[op.rt]._s16[del]);
		gAccuRSP[el]._s16[1] = gVectRSP[op.sa]._s16[el];
	}
}

