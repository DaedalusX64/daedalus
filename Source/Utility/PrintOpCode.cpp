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

// Code to format opcodes and display them nicely
#include "stdafx.h"

//
//	Exclude this from public release builds to save a little on the elf size
//
#ifndef DAEDALUS_SILENT

#include "Core/R4300OpCode.h"
#include "PrintOpCode.h"

#include "OSHLE/patch.h"		// For Definition of GetCorrectOp


static const char *Cop1BC1OpCodeNames[4] = {
	"BC1F", "BC1T",
	"BC1FL", "BC1TL"
};



static void SprintOp_Unk( char * str, u32 address, OpCode op );

static void SprintOp_Special( char * str, u32 address, OpCode op );
static void SprintOp_RegImm( char * str, u32 address, OpCode op );
static void SprintOp_J( char * str, u32 address, OpCode op );
static void SprintOp_JAL( char * str, u32 address, OpCode op );
static void SprintOp_BEQ( char * str, u32 address, OpCode op );
static void SprintOp_BNE( char * str, u32 address, OpCode op );
static void SprintOp_BLEZ( char * str, u32 address, OpCode op );
static void SprintOp_BGTZ( char * str, u32 address, OpCode op );
static void SprintOp_ADDI( char * str, u32 address, OpCode op );
static void SprintOp_ADDIU( char * str, u32 address, OpCode op );
static void SprintOp_SLTI( char * str, u32 address, OpCode op );
static void SprintOp_SLTIU( char * str, u32 address, OpCode op );
static void SprintOp_ANDI( char * str, u32 address, OpCode op );
static void SprintOp_ORI( char * str, u32 address, OpCode op );
static void SprintOp_XORI( char * str, u32 address, OpCode op );
static void SprintOp_LUI( char * str, u32 address, OpCode op );
static void SprintOp_CoPro0( char * str, u32 address, OpCode op );
static void SprintOp_CoPro1( char * str, u32 address, OpCode op );
static void SprintOp_UnOpt( char * str, u32 address, OpCode op );
static void SprintOp_Opt( char * str, u32 address, OpCode op );
static void SprintOp_NoOpt( char * str, u32 address, OpCode op );
static void SprintOp_BEQL( char * str, u32 address, OpCode op );
static void SprintOp_BNEL( char * str, u32 address, OpCode op );
static void SprintOp_BLEZL( char * str, u32 address, OpCode op );
static void SprintOp_BGTZL( char * str, u32 address, OpCode op );
static void SprintOp_DADDI( char * str, u32 address, OpCode op );
static void SprintOp_DADDIU( char * str, u32 address, OpCode op );
static void SprintOp_LDL( char * str, u32 address, OpCode op );
static void SprintOp_LDR( char * str, u32 address, OpCode op );
static void SprintOp_Patch( char * str, u32 address, OpCode op );
static void SprintOp_LB( char * str, u32 address, OpCode op );
static void SprintOp_LH( char * str, u32 address, OpCode op );
static void SprintOp_LWL( char * str, u32 address, OpCode op );
static void SprintOp_LW( char * str, u32 address, OpCode op );
static void SprintOp_LBU( char * str, u32 address, OpCode op );
static void SprintOp_LHU( char * str, u32 address, OpCode op );
static void SprintOp_LWR( char * str, u32 address, OpCode op );
static void SprintOp_LWU( char * str, u32 address, OpCode op );
static void SprintOp_SB( char * str, u32 address, OpCode op );
static void SprintOp_SH( char * str, u32 address, OpCode op );
static void SprintOp_SWL( char * str, u32 address, OpCode op );
static void SprintOp_SW( char * str, u32 address, OpCode op );
static void SprintOp_SDL( char * str, u32 address, OpCode op );
static void SprintOp_SDR( char * str, u32 address, OpCode op );
static void SprintOp_SWR( char * str, u32 address, OpCode op );
static void SprintOp_CACHE( char * str, u32 address, OpCode op );
static void SprintOp_LL( char * str, u32 address, OpCode op );
static void SprintOp_LWC1( char * str, u32 address, OpCode op );
static void SprintOp_LLD( char * str, u32 address, OpCode op );
static void SprintOp_LDC1( char * str, u32 address, OpCode op );
static void SprintOp_LDC2( char * str, u32 address, OpCode op );
static void SprintOp_LD( char * str, u32 address, OpCode op );
static void SprintOp_SC( char * str, u32 address, OpCode op );
static void SprintOp_SWC1( char * str, u32 address, OpCode op );
static void SprintOp_SCD( char * str, u32 address, OpCode op );
static void SprintOp_SDC1( char * str, u32 address, OpCode op );
static void SprintOp_SDC2( char * str, u32 address, OpCode op );
static void SprintOp_SD( char * str, u32 address, OpCode op );



static void SprintOp_Special_Unk( char * str, u32 address, OpCode op );

static void SprintOp_Special_SLL( char * str, u32 address, OpCode op );
static void SprintOp_Special_SRL( char * str, u32 address, OpCode op );
static void SprintOp_Special_SRA( char * str, u32 address, OpCode op );
static void SprintOp_Special_SLLV( char * str, u32 address, OpCode op );
static void SprintOp_Special_SRLV( char * str, u32 address, OpCode op );
static void SprintOp_Special_SRAV( char * str, u32 address, OpCode op );
static void SprintOp_Special_JR( char * str, u32 address, OpCode op );
static void SprintOp_Special_JALR( char * str, u32 address, OpCode op );
static void SprintOp_Special_SYSCALL( char * str, u32 address, OpCode op );
static void SprintOp_Special_BREAK( char * str, u32 address, OpCode op );
static void SprintOp_Special_SYNC( char * str, u32 address, OpCode op );
static void SprintOp_Special_MFHI( char * str, u32 address, OpCode op );
static void SprintOp_Special_MTHI( char * str, u32 address, OpCode op );
static void SprintOp_Special_MFLO( char * str, u32 address, OpCode op );
static void SprintOp_Special_MTLO( char * str, u32 address, OpCode op );
static void SprintOp_Special_DSLLV( char * str, u32 address, OpCode op );
static void SprintOp_Special_DSRLV( char * str, u32 address, OpCode op );
static void SprintOp_Special_DSRAV( char * str, u32 address, OpCode op );
static void SprintOp_Special_MULT( char * str, u32 address, OpCode op );
static void SprintOp_Special_MULTU( char * str, u32 address, OpCode op );
static void SprintOp_Special_DIV( char * str, u32 address, OpCode op );
static void SprintOp_Special_DIVU( char * str, u32 address, OpCode op );
static void SprintOp_Special_DMULT( char * str, u32 address, OpCode op );
static void SprintOp_Special_DMULTU( char * str, u32 address, OpCode op );
static void SprintOp_Special_DDIV( char * str, u32 address, OpCode op );
static void SprintOp_Special_DDIVU( char * str, u32 address, OpCode op );
static void SprintOp_Special_ADD( char * str, u32 address, OpCode op );
static void SprintOp_Special_ADDU( char * str, u32 address, OpCode op );
static void SprintOp_Special_SUB( char * str, u32 address, OpCode op );
static void SprintOp_Special_SUBU( char * str, u32 address, OpCode op );
static void SprintOp_Special_AND( char * str, u32 address, OpCode op );
static void SprintOp_Special_OR( char * str, u32 address, OpCode op );
static void SprintOp_Special_XOR( char * str, u32 address, OpCode op );
static void SprintOp_Special_NOR( char * str, u32 address, OpCode op );
static void SprintOp_Special_SLT( char * str, u32 address, OpCode op );
static void SprintOp_Special_SLTU( char * str, u32 address, OpCode op );
static void SprintOp_Special_DADD( char * str, u32 address, OpCode op );
static void SprintOp_Special_DADDU( char * str, u32 address, OpCode op );
static void SprintOp_Special_DSUB( char * str, u32 address, OpCode op );
static void SprintOp_Special_DSUBU( char * str, u32 address, OpCode op );
static void SprintOp_Special_TGE( char * str, u32 address, OpCode op );
static void SprintOp_Special_TGEU( char * str, u32 address, OpCode op );
static void SprintOp_Special_TLT( char * str, u32 address, OpCode op );
static void SprintOp_Special_TLTU( char * str, u32 address, OpCode op );
static void SprintOp_Special_TEQ( char * str, u32 address, OpCode op );
static void SprintOp_Special_TNE( char * str, u32 address, OpCode op );
static void SprintOp_Special_DSLL( char * str, u32 address, OpCode op );
static void SprintOp_Special_DSRL( char * str, u32 address, OpCode op );
static void SprintOp_Special_DSRA( char * str, u32 address, OpCode op );
static void SprintOp_Special_DSLL32( char * str, u32 address, OpCode op );
static void SprintOp_Special_DSRL32( char * str, u32 address, OpCode op );
static void SprintOp_Special_DSRA32( char * str, u32 address, OpCode op );


static void SprintOp_RegImm_Unk( char * str, u32 address, OpCode op );

static void SprintOp_RegImm_BLTZ( char * str, u32 address, OpCode op );
static void SprintOp_RegImm_BGEZ( char * str, u32 address, OpCode op );
static void SprintOp_RegImm_BLTZL( char * str, u32 address, OpCode op );
static void SprintOp_RegImm_BGEZL( char * str, u32 address, OpCode op );

static void SprintOp_RegImm_TGEI( char * str, u32 address, OpCode op );
static void SprintOp_RegImm_TGEIU( char * str, u32 address, OpCode op );
static void SprintOp_RegImm_TLTI( char * str, u32 address, OpCode op );
static void SprintOp_RegImm_TLTIU( char * str, u32 address, OpCode op );
static void SprintOp_RegImm_TEQI( char * str, u32 address, OpCode op );
static void SprintOp_RegImm_TNEI( char * str, u32 address, OpCode op );
static void SprintOp_RegImm_BLTZAL( char * str, u32 address, OpCode op );
static void SprintOp_RegImm_BGEZAL( char * str, u32 address, OpCode op );
static void SprintOp_RegImm_BLTZALL( char * str, u32 address, OpCode op );
static void SprintOp_RegImm_BGEZALL( char * str, u32 address, OpCode op );


static void SprintOp_Cop0_Unk( char * str, u32 address, OpCode op );
static void SprintOp_Cop0_MFC0( char * str, u32 address, OpCode op );
static void SprintOp_Cop0_MTC0( char * str, u32 address, OpCode op );
static void SprintOp_Cop0_TLB( char * str, u32 address, OpCode op );

static void SprintOp_TLB_Unk( char * str, u32 address, OpCode op );
static void SprintOp_TLB_TLBR( char * str, u32 address, OpCode op );
static void SprintOp_TLB_TLBWI( char * str, u32 address, OpCode op );
static void SprintOp_TLB_TLBWR( char * str, u32 address, OpCode op );
static void SprintOp_TLB_TLBP( char * str, u32 address, OpCode op );
static void SprintOp_TLB_ERET( char * str, u32 address, OpCode op );

static void SprintOp_Cop1_Unk( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_MFC1( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_DMFC1( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_CFC1( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_MTC1( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_DMTC1( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_CTC1( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_BCInstr( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_SInstr( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_DInstr( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_WInstr( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_LInstr( char * str, u32 address, OpCode op );


static void SprintOp_Cop1_S_Unk( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_ADD( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_SUB( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_MUL( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_DIV( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_SQRT( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_ABS( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_MOV( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_NEG( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_ROUND_L( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_TRUNC_L( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_CEIL_L( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_FLOOR_L( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_ROUND_W( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_TRUNC_W( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_CEIL_W( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_FLOOR_W( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_CVT_D( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_CVT_W( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_CVT_L( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_F( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_UN( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_EQ( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_UEQ( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_OLT( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_ULT( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_OLE( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_ULE( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_SF( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_NGLE( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_SEQ( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_NGL( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_LT( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_NGE( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_LE( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_S_NGT( char * str, u32 address, OpCode op );

static void SprintOp_Cop1_D_Unk( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_ADD( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_SUB( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_MUL( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_DIV( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_SQRT( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_ABS( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_MOV( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_NEG( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_ROUND_L( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_TRUNC_L( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_CEIL_L( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_FLOOR_L( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_ROUND_W( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_TRUNC_W( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_CEIL_W( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_FLOOR_W( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_CVT_S( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_CVT_W( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_CVT_L( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_F( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_UN( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_EQ( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_UEQ( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_OLT( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_ULT( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_OLE( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_ULE( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_SF( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_NGLE( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_SEQ( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_NGL( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_LT( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_NGE( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_LE( char * str, u32 address, OpCode op );
static void SprintOp_Cop1_D_NGT( char * str, u32 address, OpCode op );








static void SprintRSPOp_Unk( char * str, u32 address, OpCode op );

static void SprintRSPOp_Special( char * str, u32 address, OpCode op );
static void SprintRSPOp_RegImm( char * str, u32 address, OpCode op );
static void SprintRSPOp_CoPro0( char * str, u32 address, OpCode op );
static void SprintRSPOp_CoPro2( char * str, u32 address, OpCode op );


static void SprintRSPOp_J( char * str, u32 address, OpCode op );
static void SprintRSPOp_JAL( char * str, u32 address, OpCode op );
static void SprintRSPOp_BEQ( char * str, u32 address, OpCode op );
static void SprintRSPOp_BNE( char * str, u32 address, OpCode op );
static void SprintRSPOp_BLEZ( char * str, u32 address, OpCode op );
static void SprintRSPOp_BGTZ( char * str, u32 address, OpCode op );
static void SprintRSPOp_ADDI( char * str, u32 address, OpCode op );
static void SprintRSPOp_ADDIU( char * str, u32 address, OpCode op );
static void SprintRSPOp_SLTI( char * str, u32 address, OpCode op );
static void SprintRSPOp_SLTIU( char * str, u32 address, OpCode op );
static void SprintRSPOp_ANDI( char * str, u32 address, OpCode op );
static void SprintRSPOp_ORI( char * str, u32 address, OpCode op );
static void SprintRSPOp_XORI( char * str, u32 address, OpCode op );
static void SprintRSPOp_LUI( char * str, u32 address, OpCode op );
static void SprintRSPOp_LB( char * str, u32 address, OpCode op );
static void SprintRSPOp_LH( char * str, u32 address, OpCode op );
static void SprintRSPOp_LW( char * str, u32 address, OpCode op );
static void SprintRSPOp_LBU( char * str, u32 address, OpCode op );
static void SprintRSPOp_LHU( char * str, u32 address, OpCode op );
static void SprintRSPOp_SB( char * str, u32 address, OpCode op );
static void SprintRSPOp_SH( char * str, u32 address, OpCode op );
static void SprintRSPOp_SW( char * str, u32 address, OpCode op );
static void SprintRSPOp_LWC2( char * str, u32 address, OpCode op );
static void SprintRSPOp_SWC2( char * str, u32 address, OpCode op );


static void SprintRSPOp_Special_Unk( char * str, u32 address, OpCode op );

static void SprintRSPOp_Special_SLL( char * str, u32 address, OpCode op );
static void SprintRSPOp_Special_SRL( char * str, u32 address, OpCode op );
static void SprintRSPOp_Special_SRA( char * str, u32 address, OpCode op );
static void SprintRSPOp_Special_SLLV( char * str, u32 address, OpCode op );
static void SprintRSPOp_Special_SRLV( char * str, u32 address, OpCode op );
static void SprintRSPOp_Special_SRAV( char * str, u32 address, OpCode op );
static void SprintRSPOp_Special_JR( char * str, u32 address, OpCode op );
static void SprintRSPOp_Special_JALR( char * str, u32 address, OpCode op );
static void SprintRSPOp_Special_BREAK( char * str, u32 address, OpCode op );
static void SprintRSPOp_Special_ADD( char * str, u32 address, OpCode op );
static void SprintRSPOp_Special_ADDU( char * str, u32 address, OpCode op );
static void SprintRSPOp_Special_SUB( char * str, u32 address, OpCode op );
static void SprintRSPOp_Special_SUBU( char * str, u32 address, OpCode op );
static void SprintRSPOp_Special_AND( char * str, u32 address, OpCode op );
static void SprintRSPOp_Special_OR( char * str, u32 address, OpCode op );
static void SprintRSPOp_Special_XOR( char * str, u32 address, OpCode op );
static void SprintRSPOp_Special_NOR( char * str, u32 address, OpCode op );
static void SprintRSPOp_Special_SLT( char * str, u32 address, OpCode op );
static void SprintRSPOp_Special_SLTU( char * str, u32 address, OpCode op );


static void SprintRSPOp_RegImm_Unk( char * str, u32 address, OpCode op );

static void SprintRSPOp_RegImm_BLTZ( char * str, u32 address, OpCode op );
static void SprintRSPOp_RegImm_BGEZ( char * str, u32 address, OpCode op );
static void SprintRSPOp_RegImm_BLTZAL( char * str, u32 address, OpCode op );
static void SprintRSPOp_RegImm_BGEZAL( char * str, u32 address, OpCode op );


static void SprintRSPOp_Cop0_Unk( char * str, u32 address, OpCode op );
static void SprintRSPOp_Cop0_MFC0( char * str, u32 address, OpCode op );
static void SprintRSPOp_Cop0_MTC0( char * str, u32 address, OpCode op );


static void SprintRSPOp_LWC2_Unk( char * str, u32 address, OpCode op );

static void SprintRSPOp_LWC2_LSV( char * str, u32 address, OpCode op );
static void SprintRSPOp_LWC2_LLV( char * str, u32 address, OpCode op );
static void SprintRSPOp_LWC2_LDV( char * str, u32 address, OpCode op );
static void SprintRSPOp_LWC2_LQV( char * str, u32 address, OpCode op );
static void SprintRSPOp_LWC2_LTV( char * str, u32 address, OpCode op );
static void SprintRSPOp_LWC2_LBV( char * str, u32 address, OpCode op );
static void SprintRSPOp_LWC2_LRV( char * str, u32 address, OpCode op );
static void SprintRSPOp_LWC2_LPV( char * str, u32 address, OpCode op );
static void SprintRSPOp_LWC2_LUV( char * str, u32 address, OpCode op );
static void SprintRSPOp_LWC2_LHV( char * str, u32 address, OpCode op );
static void SprintRSPOp_LWC2_LFV( char * str, u32 address, OpCode op );
static void SprintRSPOp_LWC2_LWV( char * str, u32 address, OpCode op );

static void SprintRSPOp_SWC2_Unk( char * str, u32 address, OpCode op );

static void SprintRSPOp_SWC2_SSV( char * str, u32 address, OpCode op );
static void SprintRSPOp_SWC2_SLV( char * str, u32 address, OpCode op );
static void SprintRSPOp_SWC2_SDV( char * str, u32 address, OpCode op );
static void SprintRSPOp_SWC2_SQV( char * str, u32 address, OpCode op );
static void SprintRSPOp_SWC2_STV( char * str, u32 address, OpCode op );
static void SprintRSPOp_SWC2_SBV( char * str, u32 address, OpCode op );
static void SprintRSPOp_SWC2_SRV( char * str, u32 address, OpCode op );
static void SprintRSPOp_SWC2_SPV( char * str, u32 address, OpCode op );
static void SprintRSPOp_SWC2_SUV( char * str, u32 address, OpCode op );
static void SprintRSPOp_SWC2_SHV( char * str, u32 address, OpCode op );
static void SprintRSPOp_SWC2_SFV( char * str, u32 address, OpCode op );
static void SprintRSPOp_SWC2_SWV( char * str, u32 address, OpCode op );

static void SprintRSPOp_Cop2_Unk( char * str, u32 address, OpCode op );
static void SprintRSPOp_Cop2_MFC2( char * str, u32 address, OpCode op );
static void SprintRSPOp_Cop2_MTC2( char * str, u32 address, OpCode op );
static void SprintRSPOp_Cop2_VOP( char * str, u32 address, OpCode op );



static void SprintRSPOp_Vop_Unk( char * str, u32 address, OpCode op );

static void SprintRSPOp_Vop_VMULF( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VMULU( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VRNDP( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VMULQ( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VMUDL( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VMUDM( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VMUDN( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VMUDH( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VMACF( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VMACU( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VRNDN( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VMACQ( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VMADL( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VMADM( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VMADN( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VMADH( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VADD( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VSUB( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VSUT( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VABS( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VADDC( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VSUBC( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VADDB( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VSUBB( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VACCB( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VSUCB( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VSAD( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VSAC( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VSUM( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VSAW( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VLT( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VEQ( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VNE( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VGE( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VCL( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VCH( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VCR( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VMRG( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VAND( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VNAND( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VOR( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VNOR( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VXOR( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VNXOR( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VRCP( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VRCPL( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VRCPH( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VMOV( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VRSQ( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VRSQL( char * str, u32 address, OpCode op );
static void SprintRSPOp_Vop_VRSQH( char * str, u32 address, OpCode op );



typedef void (*SprintOpInstruction)( char * str, u32 address, OpCode op );

// Opcode Jump Table
SprintOpInstruction SprintOp_Instructions[64] =
{
	SprintOp_Special, SprintOp_RegImm, SprintOp_J, SprintOp_JAL, SprintOp_BEQ, SprintOp_BNE, SprintOp_BLEZ, SprintOp_BGTZ,
	SprintOp_ADDI, SprintOp_ADDIU, SprintOp_SLTI, SprintOp_SLTIU, SprintOp_ANDI, SprintOp_ORI, SprintOp_XORI, SprintOp_LUI,
	SprintOp_CoPro0, SprintOp_CoPro1, SprintOp_Unk, SprintOp_Unk, SprintOp_BEQL, SprintOp_BNEL, SprintOp_BLEZL, SprintOp_BGTZL,
	SprintOp_DADDI, SprintOp_DADDIU, SprintOp_LDL, SprintOp_LDR, SprintOp_Patch, SprintOp_UnOpt, SprintOp_Opt, SprintOp_NoOpt,
	SprintOp_LB, SprintOp_LH, SprintOp_LWL, SprintOp_LW, SprintOp_LBU, SprintOp_LHU, SprintOp_LWR, SprintOp_LWU,
	SprintOp_SB, SprintOp_SH, SprintOp_SWL, SprintOp_SW, SprintOp_SDL, SprintOp_SDR, SprintOp_SWR, SprintOp_CACHE,
	SprintOp_LL, SprintOp_LWC1, SprintOp_Unk, SprintOp_Unk, SprintOp_LLD, SprintOp_LDC1, SprintOp_LDC2, SprintOp_LD,
	SprintOp_SC, SprintOp_SWC1, SprintOp_Unk, SprintOp_Unk, SprintOp_SCD, SprintOp_SDC1, SprintOp_SDC2, SprintOp_SD
};

SprintOpInstruction SprintOp_SpecialInstructions[64] =
{
	SprintOp_Special_SLL, SprintOp_Special_Unk, SprintOp_Special_SRL, SprintOp_Special_SRA, SprintOp_Special_SLLV, SprintOp_Special_Unk, SprintOp_Special_SRLV, SprintOp_Special_SRAV,
	SprintOp_Special_JR, SprintOp_Special_JALR, SprintOp_Special_Unk, SprintOp_Special_Unk, SprintOp_Special_SYSCALL, SprintOp_Special_BREAK, SprintOp_Special_Unk, SprintOp_Special_SYNC,
	SprintOp_Special_MFHI, SprintOp_Special_MTHI, SprintOp_Special_MFLO, SprintOp_Special_MTLO, SprintOp_Special_DSLLV, SprintOp_Special_Unk, SprintOp_Special_DSRLV, SprintOp_Special_DSRAV,
	SprintOp_Special_MULT, SprintOp_Special_MULTU, SprintOp_Special_DIV, SprintOp_Special_DIVU, SprintOp_Special_DMULT, SprintOp_Special_DMULTU, SprintOp_Special_DDIV, SprintOp_Special_DDIVU,
	SprintOp_Special_ADD, SprintOp_Special_ADDU, SprintOp_Special_SUB, SprintOp_Special_SUBU, SprintOp_Special_AND, SprintOp_Special_OR, SprintOp_Special_XOR, SprintOp_Special_NOR,
	SprintOp_Special_Unk, SprintOp_Special_Unk, SprintOp_Special_SLT, SprintOp_Special_SLTU, SprintOp_Special_DADD, SprintOp_Special_DADDU, SprintOp_Special_DSUB, SprintOp_Special_DSUBU,
	SprintOp_Special_TGE, SprintOp_Special_TGEU, SprintOp_Special_TLT, SprintOp_Special_TLTU, SprintOp_Special_TEQ, SprintOp_Special_Unk, SprintOp_Special_TNE, SprintOp_Special_Unk,
	SprintOp_Special_DSLL, SprintOp_Special_Unk, SprintOp_Special_DSRL, SprintOp_Special_DSRA, SprintOp_Special_DSLL32, SprintOp_Special_Unk, SprintOp_Special_DSRL32, SprintOp_Special_DSRA32
};

SprintOpInstruction SprintOp_RegImmInstructions[32] =
{
	SprintOp_RegImm_BLTZ,   SprintOp_RegImm_BGEZ,   SprintOp_RegImm_BLTZL,   SprintOp_RegImm_BGEZL,   SprintOp_RegImm_Unk,  SprintOp_RegImm_Unk, SprintOp_RegImm_Unk,  SprintOp_RegImm_Unk,
	SprintOp_RegImm_TGEI,   SprintOp_RegImm_TGEIU,  SprintOp_RegImm_TLTI,    SprintOp_RegImm_TLTIU,   SprintOp_RegImm_TEQI, SprintOp_RegImm_Unk, SprintOp_RegImm_TNEI, SprintOp_RegImm_Unk,
	SprintOp_RegImm_BLTZAL, SprintOp_RegImm_BGEZAL, SprintOp_RegImm_BLTZALL, SprintOp_RegImm_BGEZALL, SprintOp_RegImm_Unk,  SprintOp_RegImm_Unk, SprintOp_RegImm_Unk,  SprintOp_RegImm_Unk,
	SprintOp_RegImm_Unk,    SprintOp_RegImm_Unk,    SprintOp_RegImm_Unk,     SprintOp_RegImm_Unk,     SprintOp_RegImm_Unk,  SprintOp_RegImm_Unk, SprintOp_RegImm_Unk,  SprintOp_RegImm_Unk
};


SprintOpInstruction SprintOp_Cop0Instructions[32] =
{
	SprintOp_Cop0_MFC0, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_MTC0, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk,
	SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk,
	SprintOp_Cop0_TLB, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk,
	SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk, SprintOp_Cop0_Unk,
};


SprintOpInstruction SprintOp_TLBInstructions[64] =
{
	SprintOp_TLB_Unk, SprintOp_TLB_TLBR, SprintOp_TLB_TLBWI, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_TLBWR, SprintOp_TLB_Unk,
	SprintOp_TLB_TLBP, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk,
	SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk,
	SprintOp_TLB_ERET, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk,
	SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk,
	SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk,
	SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk,
	SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk, SprintOp_TLB_Unk,
};

SprintOpInstruction SprintOp_Cop1Instructions[32] =
{
	SprintOp_Cop1_MFC1,    SprintOp_Cop1_DMFC1,  SprintOp_Cop1_CFC1, SprintOp_Cop1_Unk, SprintOp_Cop1_MTC1,   SprintOp_Cop1_DMTC1,  SprintOp_Cop1_CTC1, SprintOp_Cop1_Unk,
	SprintOp_Cop1_BCInstr, SprintOp_Cop1_Unk,    SprintOp_Cop1_Unk,  SprintOp_Cop1_Unk, SprintOp_Cop1_Unk,    SprintOp_Cop1_Unk,    SprintOp_Cop1_Unk,  SprintOp_Cop1_Unk,
	SprintOp_Cop1_SInstr,  SprintOp_Cop1_DInstr, SprintOp_Cop1_Unk,  SprintOp_Cop1_Unk, SprintOp_Cop1_WInstr, SprintOp_Cop1_LInstr, SprintOp_Cop1_Unk,  SprintOp_Cop1_Unk,
	SprintOp_Cop1_Unk,     SprintOp_Cop1_Unk,    SprintOp_Cop1_Unk,  SprintOp_Cop1_Unk, SprintOp_Cop1_Unk,    SprintOp_Cop1_Unk,    SprintOp_Cop1_Unk,  SprintOp_Cop1_Unk
};

SprintOpInstruction SprintOp_Cop1SInstruction[64] =
{
	SprintOp_Cop1_S_ADD,     SprintOp_Cop1_S_SUB,     SprintOp_Cop1_S_MUL,    SprintOp_Cop1_S_DIV,     SprintOp_Cop1_S_SQRT,    SprintOp_Cop1_S_ABS,     SprintOp_Cop1_S_MOV,    SprintOp_Cop1_S_NEG,
	SprintOp_Cop1_S_ROUND_L, SprintOp_Cop1_S_TRUNC_L,	SprintOp_Cop1_S_CEIL_L, SprintOp_Cop1_S_FLOOR_L, SprintOp_Cop1_S_ROUND_W, SprintOp_Cop1_S_TRUNC_W, SprintOp_Cop1_S_CEIL_W, SprintOp_Cop1_S_FLOOR_W,
	SprintOp_Cop1_S_Unk,     SprintOp_Cop1_S_Unk,     SprintOp_Cop1_S_Unk,    SprintOp_Cop1_S_Unk,     SprintOp_Cop1_S_Unk,     SprintOp_Cop1_S_Unk,     SprintOp_Cop1_S_Unk,    SprintOp_Cop1_S_Unk,
	SprintOp_Cop1_S_Unk,     SprintOp_Cop1_S_Unk,     SprintOp_Cop1_S_Unk,    SprintOp_Cop1_S_Unk,     SprintOp_Cop1_S_Unk,     SprintOp_Cop1_S_Unk,     SprintOp_Cop1_S_Unk,    SprintOp_Cop1_S_Unk,
	SprintOp_Cop1_S_Unk,     SprintOp_Cop1_S_CVT_D,   SprintOp_Cop1_S_Unk,    SprintOp_Cop1_S_Unk,     SprintOp_Cop1_S_CVT_W,   SprintOp_Cop1_S_CVT_L,   SprintOp_Cop1_S_Unk,    SprintOp_Cop1_S_Unk,
	SprintOp_Cop1_S_Unk,     SprintOp_Cop1_S_Unk,     SprintOp_Cop1_S_Unk,    SprintOp_Cop1_S_Unk,     SprintOp_Cop1_S_Unk,     SprintOp_Cop1_S_Unk,     SprintOp_Cop1_S_Unk,    SprintOp_Cop1_S_Unk,
	SprintOp_Cop1_S_F,       SprintOp_Cop1_S_UN,      SprintOp_Cop1_S_EQ,     SprintOp_Cop1_S_UEQ,     SprintOp_Cop1_S_OLT,     SprintOp_Cop1_S_ULT,     SprintOp_Cop1_S_OLE,    SprintOp_Cop1_S_ULE,
	SprintOp_Cop1_S_SF,      SprintOp_Cop1_S_NGLE,    SprintOp_Cop1_S_SEQ,    SprintOp_Cop1_S_NGL,     SprintOp_Cop1_S_LT,      SprintOp_Cop1_S_NGE,     SprintOp_Cop1_S_LE,     SprintOp_Cop1_S_NGT
};

SprintOpInstruction SprintOp_Cop1DInstruction[64] =
{
	SprintOp_Cop1_D_ADD,     SprintOp_Cop1_D_SUB,     SprintOp_Cop1_D_MUL, SprintOp_Cop1_D_DIV, SprintOp_Cop1_D_SQRT, SprintOp_Cop1_D_ABS, SprintOp_Cop1_D_MOV, SprintOp_Cop1_D_NEG,
	SprintOp_Cop1_D_ROUND_L, SprintOp_Cop1_D_TRUNC_L,	SprintOp_Cop1_D_CEIL_L, SprintOp_Cop1_D_FLOOR_L, SprintOp_Cop1_D_ROUND_W, SprintOp_Cop1_D_TRUNC_W, SprintOp_Cop1_D_CEIL_W, SprintOp_Cop1_D_FLOOR_W,
	SprintOp_Cop1_D_Unk,     SprintOp_Cop1_D_Unk,     SprintOp_Cop1_D_Unk, SprintOp_Cop1_D_Unk, SprintOp_Cop1_D_Unk, SprintOp_Cop1_D_Unk, SprintOp_Cop1_D_Unk, SprintOp_Cop1_D_Unk,
	SprintOp_Cop1_D_Unk,     SprintOp_Cop1_D_Unk,     SprintOp_Cop1_D_Unk, SprintOp_Cop1_D_Unk, SprintOp_Cop1_D_Unk, SprintOp_Cop1_D_Unk, SprintOp_Cop1_D_Unk, SprintOp_Cop1_D_Unk,
	SprintOp_Cop1_D_CVT_S,   SprintOp_Cop1_D_Unk,     SprintOp_Cop1_D_Unk, SprintOp_Cop1_D_Unk, SprintOp_Cop1_D_CVT_W, SprintOp_Cop1_D_CVT_L, SprintOp_Cop1_D_Unk, SprintOp_Cop1_D_Unk,
	SprintOp_Cop1_D_Unk,     SprintOp_Cop1_D_Unk,     SprintOp_Cop1_D_Unk, SprintOp_Cop1_D_Unk, SprintOp_Cop1_D_Unk, SprintOp_Cop1_D_Unk, SprintOp_Cop1_D_Unk, SprintOp_Cop1_D_Unk,
	SprintOp_Cop1_D_F,       SprintOp_Cop1_D_UN,      SprintOp_Cop1_D_EQ, SprintOp_Cop1_D_UEQ, SprintOp_Cop1_D_OLT, SprintOp_Cop1_D_ULT, SprintOp_Cop1_D_OLE, SprintOp_Cop1_D_ULE,
	SprintOp_Cop1_D_SF,      SprintOp_Cop1_D_NGLE,    SprintOp_Cop1_D_SEQ, SprintOp_Cop1_D_NGL, SprintOp_Cop1_D_LT, SprintOp_Cop1_D_NGE, SprintOp_Cop1_D_LE, SprintOp_Cop1_D_NGT
};



#define BranchAddress(op, address) (    (address)+4 + (s16)(((op).immediate))*4)
#define JumpTarget(op, address)    (   ((address) & 0xF0000000) | (((op).target)<<2)   )

void SprintOp_Unk( char * str, u32 address, OpCode op ) { sprintf(str, "Op_Unk?"); }

void SprintOp_Special( char * str, u32 address, OpCode op )		{ SprintOp_SpecialInstructions[op.spec_op]( str, address, op ); }
void SprintOp_RegImm( char * str, u32 address, OpCode op )		{ SprintOp_RegImmInstructions[op.rt]( str, address, op ); }
void SprintOp_CoPro0( char * str, u32 address, OpCode op )		{ SprintOp_Cop0Instructions[op.cop0_op]( str, address, op ); }
void SprintOp_CoPro1( char * str, u32 address, OpCode op )		{ SprintOp_Cop1Instructions[op.cop1_op]( str, address, op ); }

void SprintOp_UnOpt( char * str, u32 address, OpCode op )		{ sprintf(str, "SRHack UnOpt"); }
void SprintOp_Opt( char * str, u32 address, OpCode op )			{ sprintf(str, "SRHack Opt"); }

void SprintOp_NoOpt( char * str, u32 address, OpCode op )		{   if( op.spec_op == 0 )
																	{
																		sprintf(str, "EXT       %s = %s [%d] [%d]", RegNames[op.rt], RegNames[op.rs], op.rd, op.sa);
																	}
																	else if( op.spec_op == 4 )
																	{
																		sprintf(str, "INS       %s = %s [%d] [%d]", RegNames[op.rt], RegNames[op.rs], op.rd, op.sa);
																	}
																	else
																	{
																		sprintf(str, "SRHack NoOpt");
																	}
																}

void SprintOp_Patch( char * str, u32 address, OpCode op )		{ sprintf(str, "Patch");


//Patch_GetJumpAddressName(JumpTarget(op, address)) );

}

void SprintOp_J( char * str, u32 address, OpCode op )
{
	const char * p_name( "?" );
#ifdef DAEDALUS_ENABLE_OS_HOOKS
	p_name = Patch_GetJumpAddressName(JumpTarget(op, address));
#endif
	sprintf(str, "J         0x%08x        %s", JumpTarget(op, address), p_name );
}
void SprintOp_JAL( char * str, u32 address, OpCode op )
{
	const char * p_name( "?" );
#ifdef DAEDALUS_ENABLE_OS_HOOKS
	p_name = Patch_GetJumpAddressName(JumpTarget(op, address));
#endif
	sprintf(str, "JAL       0x%08x        %s", JumpTarget(op, address), p_name );
}
void SprintOp_BEQ( char * str, u32 address, OpCode op ) {
						if (op.rs == 0 && op.rt == 0)             sprintf(str, "B         --> 0x%08x", address+4 + (s16)op.immediate*4);
						else                                      sprintf(str, "BEQ       %s == %s --> 0x%08x", RegNames[op.rs], RegNames[op.rt], BranchAddress(op, address)); }
void SprintOp_BNE( char * str, u32 address, OpCode op )			{ sprintf(str, "BNE       %s != %s --> 0x%08x", RegNames[op.rs], RegNames[op.rt], BranchAddress(op, address)); }
void SprintOp_BLEZ( char * str, u32 address, OpCode op )		{ sprintf(str, "BLEZ      %s <= 0 --> 0x%08x", RegNames[op.rs], BranchAddress(op, address)); }
void SprintOp_BGTZ( char * str, u32 address, OpCode op )		{ sprintf(str, "BGTZ      %s > 0 --> 0x%08x", RegNames[op.rs], BranchAddress(op, address)); }
void SprintOp_ADDI( char * str, u32 address, OpCode op )		{ sprintf(str, "ADDI      %s = %s + 0x%04x", RegNames[op.rt], RegNames[op.rs], op.immediate); }
void SprintOp_ADDIU( char * str, u32 address, OpCode op )		{ sprintf(str, "ADDIU     %s = %s + 0x%04x", RegNames[op.rt], RegNames[op.rs], op.immediate); }
void SprintOp_SLTI( char * str, u32 address, OpCode op )		{ sprintf(str, "SLTI      %s = (%s < 0x%04x)", RegNames[op.rt], RegNames[op.rs], op.immediate); }
void SprintOp_SLTIU( char * str, u32 address, OpCode op )		{ sprintf(str, "SLTIU     %s = (%s < 0x%04x)", RegNames[op.rt], RegNames[op.rs], op.immediate); }
void SprintOp_ANDI( char * str, u32 address, OpCode op )		{ sprintf(str, "ANDI      %s = %s & 0x%04x", RegNames[op.rt], RegNames[op.rs], op.immediate); }
void SprintOp_ORI( char * str, u32 address, OpCode op )			{ sprintf(str, "ORI       %s = %s | 0x%04x", RegNames[op.rt], RegNames[op.rs], op.immediate); }
void SprintOp_XORI( char * str, u32 address, OpCode op )		{ sprintf(str, "XORI      %s = %s ^ 0x%04x", RegNames[op.rt], RegNames[op.rs], op.immediate); }
void SprintOp_LUI( char * str, u32 address, OpCode op )			{ sprintf(str, "LUI       %s = 0x%08x", RegNames[op.rt], op.immediate<<16); }
void SprintOp_BEQL( char * str, u32 address, OpCode op )		{ sprintf(str, "BEQL      %s == %s --> 0x%08x", RegNames[op.rs], RegNames[op.rt], BranchAddress(op, address)); }
void SprintOp_BNEL( char * str, u32 address, OpCode op )		{ sprintf(str, "BNEL      %s != %s --> 0x%08x", RegNames[op.rs], RegNames[op.rt], BranchAddress(op, address)); }
void SprintOp_BLEZL( char * str, u32 address, OpCode op )		{ sprintf(str, "BLEZL     %s <= 0 --> 0x%08x", RegNames[op.rs], BranchAddress(op, address)); }
void SprintOp_BGTZL( char * str, u32 address, OpCode op )		{ sprintf(str, "BGTZL     %s > 0 --> 0x%08x", RegNames[op.rs], BranchAddress(op, address)); }
void SprintOp_DADDI( char * str, u32 address, OpCode op )		{ sprintf(str, "DADDI     %s = %s + 0x%04x", RegNames[op.rt], RegNames[op.rs], op.immediate); }
void SprintOp_DADDIU( char * str, u32 address, OpCode op )		{ sprintf(str, "DADDIU    %s = %s + 0x%04x", RegNames[op.rt], RegNames[op.rs], op.immediate); }
void SprintOp_LDL( char * str, u32 address, OpCode op )			{ sprintf(str, "LDL       %s <- 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_LDR( char * str, u32 address, OpCode op )			{ sprintf(str, "LDR       %s <- 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_LB( char * str, u32 address, OpCode op )			{ sprintf(str, "LB        %s <- 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_LH( char * str, u32 address, OpCode op )			{ sprintf(str, "LH        %s <- 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_LWL( char * str, u32 address, OpCode op )			{ sprintf(str, "LWL       %s <- 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_LW( char * str, u32 address, OpCode op )			{ sprintf(str, "LW        %s <- 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_LBU( char * str, u32 address, OpCode op )			{ sprintf(str, "LBU       %s <- 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_LHU( char * str, u32 address, OpCode op )			{ sprintf(str, "LHU       %s <- 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_LWR( char * str, u32 address, OpCode op )			{ sprintf(str, "LWR       %s <- 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_LWU( char * str, u32 address, OpCode op )			{ sprintf(str, "LWU       %s <- 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_SB( char * str, u32 address, OpCode op )			{ sprintf(str, "SB        %s -> 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_SH( char * str, u32 address, OpCode op )			{ sprintf(str, "SH        %s -> 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_SWL( char * str, u32 address, OpCode op )			{ sprintf(str, "SWL       %s -> 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_SW( char * str, u32 address, OpCode op )			{ sprintf(str, "SW        %s -> 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_SDL( char * str, u32 address, OpCode op )			{ sprintf(str, "SDL       %s -> 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_SDR( char * str, u32 address, OpCode op )			{ sprintf(str, "SDR       %s -> 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_SWR( char * str, u32 address, OpCode op )			{ sprintf(str, "SWR       %s -> 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_CACHE( char * str, u32 address, OpCode op )		{ sprintf(str, "CACHE     0x%02x,0x%04x(%s)", op.rt, op.immediate, RegNames[op.rs]); }
void SprintOp_LL( char * str, u32 address, OpCode op )			{ sprintf(str, "LL        %s <- 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_LWC1( char * str, u32 address, OpCode op )		{ sprintf(str, "LWC1      FP%02d <- 0x%04x(%s)", op.ft, op.immediate, RegNames[op.rs]); }
void SprintOp_LLD( char * str, u32 address, OpCode op )			{ sprintf(str, "LLD       %s <- 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_LDC1( char * str, u32 address, OpCode op )		{ sprintf(str, "LDC1      FP%02d <- 0x%04x(%s)", op.ft, op.immediate, RegNames[op.rs]); }
void SprintOp_LDC2( char * str, u32 address, OpCode op )		{ sprintf(str, "LDC2      GR%02d <- 0x%04x(%s)", op.ft, op.immediate, RegNames[op.rs]); }
void SprintOp_LD( char * str, u32 address, OpCode op )			{ sprintf(str, "LD        %s <- 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_SC( char * str, u32 address, OpCode op )			{ sprintf(str, "SC        %s -> 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_SWC1( char * str, u32 address, OpCode op )		{ sprintf(str, "SWC1      FP%02d -> 0x%04x(%s)", op.ft, op.immediate, RegNames[op.rs]); }
void SprintOp_SCD( char * str, u32 address, OpCode op )			{ sprintf(str, "SCD       %s -> 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintOp_SDC1( char * str, u32 address, OpCode op )		{ sprintf(str, "SDC1      FP%02d -> 0x%04x(%s)", op.ft, op.immediate, RegNames[op.rs]); }
void SprintOp_SDC2( char * str, u32 address, OpCode op )		{ sprintf(str, "SDC2      GR%02d -> 0x%04x(%s)", op.ft, op.immediate, RegNames[op.rs]); }
void SprintOp_SD( char * str, u32 address, OpCode op )			{ sprintf(str, "SD        %s -> 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }


void SprintOp_Special_Unk( char * str, u32 address, OpCode op )		{ sprintf(str, "Special_Unk?"); }

void SprintOp_Special_SLL( char * str, u32 address, OpCode op )			{
								if (op._u32 == 0)						  sprintf(str, "NOP");
								else									  sprintf(str, "SLL       %s = %s << 0x%04x", RegNames[op.rd], RegNames[op.rt], op.sa); }
void SprintOp_Special_SRL( char * str, u32 address, OpCode op )			{ sprintf(str, "SRL       %s = %s >> 0x%04x", RegNames[op.rd], RegNames[op.rt], op.sa); }
void SprintOp_Special_SRA( char * str, u32 address, OpCode op )			{ sprintf(str, "SRA       %s = %s >> 0x%04x", RegNames[op.rd], RegNames[op.rt], op.sa); }
void SprintOp_Special_SLLV( char * str, u32 address, OpCode op )		{ sprintf(str, "SLLV      %s = %s << %s", RegNames[op.rd], RegNames[op.rt], RegNames[op.rs]); }
void SprintOp_Special_SRLV( char * str, u32 address, OpCode op )		{ sprintf(str, "SRLV      %s = %s >> %s", RegNames[op.rd], RegNames[op.rt], RegNames[op.rs]); }
void SprintOp_Special_SRAV( char * str, u32 address, OpCode op )		{ sprintf(str, "SRAV      %s = %s >> %s", RegNames[op.rd], RegNames[op.rt], RegNames[op.rs]); }
void SprintOp_Special_JR( char * str, u32 address, OpCode op )		{ sprintf(str, "JR        %s", RegNames[op.rs]); }
void SprintOp_Special_JALR( char * str, u32 address, OpCode op )		{ sprintf(str, "JALR      %s, %s", RegNames[op.rd], RegNames[op.rs]); }
void SprintOp_Special_SYSCALL( char * str, u32 address, OpCode op )	{ sprintf(str, "SYSCALL"); }
void SprintOp_Special_BREAK( char * str, u32 address, OpCode op )	{ sprintf(str, "BREAK     0x%08x", (op._u32>>6)&0xFFFFF); }
void SprintOp_Special_SYNC( char * str, u32 address, OpCode op )		{ sprintf(str, "SYNC"); }
void SprintOp_Special_MFHI( char * str, u32 address, OpCode op )		{ sprintf(str, "MFHI      %s", RegNames[op.rd]); }
void SprintOp_Special_MTHI( char * str, u32 address, OpCode op )		{ sprintf(str, "MTHI      %s", RegNames[op.rs]); }
void SprintOp_Special_MFLO( char * str, u32 address, OpCode op )		{ sprintf(str, "MFLO      %s", RegNames[op.rd]); }
void SprintOp_Special_MTLO( char * str, u32 address, OpCode op )		{ sprintf(str, "MTLO      %s", RegNames[op.rs]); }
void SprintOp_Special_DSLLV( char * str, u32 address, OpCode op )		{ sprintf(str, "DSLLV     %s = %s << %s", RegNames[op.rd], RegNames[op.rt], RegNames[op.rs]); }
void SprintOp_Special_DSRLV( char * str, u32 address, OpCode op )		{ sprintf(str, "DSRLV     %s = %s >> %s", RegNames[op.rd], RegNames[op.rt], RegNames[op.rs]); }
void SprintOp_Special_DSRAV( char * str, u32 address, OpCode op )		{ sprintf(str, "DSRAV     %s = %s >> %s", RegNames[op.rd], RegNames[op.rt], RegNames[op.rs]); }
void SprintOp_Special_MULT( char * str, u32 address, OpCode op )		{ sprintf(str, "MULT      %s * %s", RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_MULTU( char * str, u32 address, OpCode op )		{ sprintf(str, "MULTU     %s * %s", RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_DIV( char * str, u32 address, OpCode op )			{ sprintf(str, "DIV       %s / %s", RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_DIVU( char * str, u32 address, OpCode op )		{ sprintf(str, "DIVU      %s / %s", RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_DMULT( char * str, u32 address, OpCode op )		{ sprintf(str, "DMULT     %s * %s", RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_DMULTU( char * str, u32 address, OpCode op )		{ sprintf(str, "DMULTU    %s * %s", RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_DDIV( char * str, u32 address, OpCode op )		{ sprintf(str, "DDIV      %s / %s", RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_DDIVU( char * str, u32 address, OpCode op )		{ sprintf(str, "DDIVU     %s / %s", RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_ADD( char * str, u32 address, OpCode op )			{ sprintf(str, "ADD       %s = %s + %s", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_ADDU( char * str, u32 address, OpCode op )		{ sprintf(str, "ADDU      %s = %s + %s", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_SUB( char * str, u32 address, OpCode op )			{ sprintf(str, "SUB       %s = %s - %s", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_SUBU( char * str, u32 address, OpCode op )		{ sprintf(str, "SUBU      %s = %s - %s", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_AND( char * str, u32 address, OpCode op )			{ sprintf(str, "AND       %s = %s & %s", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_OR( char * str, u32 address, OpCode op )			{
							if (op.rs == 0 && op.rt == 0)	  sprintf(str, "CLEAR     %s = 0", RegNames[op.rd]);
 							else if (op.rt == 0)					  sprintf(str, "MOV       %s = %s", RegNames[op.rd], RegNames[op.rs]);
							else										  sprintf(str, "OR        %s = %s | %s", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_XOR( char * str, u32 address, OpCode op )			{ sprintf(str, "XOR       %s = %s ^ %s", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_NOR( char * str, u32 address, OpCode op )			{ sprintf(str, "NOR       %s = ~(%s | %s)", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_SLT( char * str, u32 address, OpCode op )			{ sprintf(str, "SLT       %s = (%s<%s)", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_SLTU( char * str, u32 address, OpCode op )		{ sprintf(str, "SLTU      %s = (%s<%s)", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_DADD( char * str, u32 address, OpCode op )		{ sprintf(str, "DADD      %s = %s + %s", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_DADDU( char * str, u32 address, OpCode op )		{ sprintf(str, "DADDU     %s = %s + %s", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_DSUB( char * str, u32 address, OpCode op )		{ sprintf(str, "DSUB      %s = %s - %s", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_DSUBU( char * str, u32 address, OpCode op )		{ sprintf(str, "DSUBU     %s = %s - %s", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_DSLL( char * str, u32 address, OpCode op )		{ sprintf(str, "DSLL      %s = %s << 0x%04x", RegNames[op.rd], RegNames[op.rt], op.sa); }
void SprintOp_Special_DSRL( char * str, u32 address, OpCode op )		{ sprintf(str, "DSRL      %s = %s >> 0x%04x", RegNames[op.rd], RegNames[op.rt], op.sa); }
void SprintOp_Special_DSRA( char * str, u32 address, OpCode op )		{ sprintf(str, "DSRA      %s = %s >> 0x%04x", RegNames[op.rd], RegNames[op.rt], op.sa); }
void SprintOp_Special_DSLL32( char * str, u32 address, OpCode op )		{ sprintf(str, "DSLL32    %s = %s << 0x%04x", RegNames[op.rd], RegNames[op.rt], op.sa+32); }
void SprintOp_Special_DSRL32( char * str, u32 address, OpCode op )		{ sprintf(str, "DSRL32    %s = %s >> 0x%04x", RegNames[op.rd], RegNames[op.rt], op.sa+32); }
void SprintOp_Special_DSRA32( char * str, u32 address, OpCode op )		{ sprintf(str, "DSRA32    %s = %s >> 0x%04x", RegNames[op.rd], RegNames[op.rt], op.sa+32); }

void SprintOp_Special_TGE( char * str, u32 address, OpCode op )			{ sprintf(str, "TGE       %s >= %s", RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_TGEU( char * str, u32 address, OpCode op )		{ sprintf(str, "TGEU      %s >= %s", RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_TLT( char * str, u32 address, OpCode op )			{ sprintf(str, "TLT       %s < %s", RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_TLTU( char * str, u32 address, OpCode op )		{ sprintf(str, "TLTU      %s < %s", RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_TEQ( char * str, u32 address, OpCode op )			{ sprintf(str, "TEQ       %s == %s", RegNames[op.rs], RegNames[op.rt]); }
void SprintOp_Special_TNE( char * str, u32 address, OpCode op )			{ sprintf(str, "TNE       %s != %s", RegNames[op.rs], RegNames[op.rt]); }

void SprintOp_RegImm_Unk( char * str, u32 address, OpCode op )			{ sprintf(str, "RegImm_Unk?"); }

void SprintOp_RegImm_BLTZ( char * str, u32 address, OpCode op )			{ sprintf(str, "BLTZ      %s < 0 --> 0x%08x", RegNames[op.rs], BranchAddress(op, address)); }
void SprintOp_RegImm_BGEZ( char * str, u32 address, OpCode op )			{ sprintf(str, "BGEZ      %s >= 0 --> 0x%08x", RegNames[op.rs], BranchAddress(op, address)); }
void SprintOp_RegImm_BLTZL( char * str, u32 address, OpCode op )		{ sprintf(str, "BLTZL     %s < 0 --> 0x%08x", RegNames[op.rs], BranchAddress(op, address)); }
void SprintOp_RegImm_BGEZL( char * str, u32 address, OpCode op )		{ sprintf(str, "BGEZL     %s >= 0 --> 0x%08x", RegNames[op.rs], BranchAddress(op, address)); }
void SprintOp_RegImm_BLTZAL( char * str, u32 address, OpCode op )		{ sprintf(str, "BLTZAL    %s < 0 --> 0x%08x", RegNames[op.rs], BranchAddress(op, address)); }
void SprintOp_RegImm_BGEZAL( char * str, u32 address, OpCode op )		{ sprintf(str, "BGEZAL    %s >= 0 --> 0x%08x", RegNames[op.rs], BranchAddress(op, address)); }
void SprintOp_RegImm_BLTZALL( char * str, u32 address, OpCode op )		{ sprintf(str, "BLTZALL   %s < 0 --> 0x%08x", RegNames[op.rs], BranchAddress(op, address)); }
void SprintOp_RegImm_BGEZALL( char * str, u32 address, OpCode op )		{ sprintf(str, "BGEZALL   %s >= 0 --> 0x%08x", RegNames[op.rs], BranchAddress(op, address)); }

void SprintOp_RegImm_TGEI( char * str, u32 address, OpCode op )			{ sprintf(str, "TGEI      %s >= 0x%04x", RegNames[op.rs], op.immediate); }
void SprintOp_RegImm_TGEIU( char * str, u32 address, OpCode op )		{ sprintf(str, "TGEIU     %s >= 0x%04x", RegNames[op.rs], op.immediate); }
void SprintOp_RegImm_TLTI( char * str, u32 address, OpCode op )			{ sprintf(str, "TLTI      %s < 0x%04x", RegNames[op.rs], op.immediate); }
void SprintOp_RegImm_TLTIU( char * str, u32 address, OpCode op )		{ sprintf(str, "TLTIU     %s < 0x%04x", RegNames[op.rs], op.immediate); }
void SprintOp_RegImm_TEQI( char * str, u32 address, OpCode op )			{ sprintf(str, "TEQI      %s == 0x%04x", RegNames[op.rs], op.immediate); }
void SprintOp_RegImm_TNEI( char * str, u32 address, OpCode op )			{ sprintf(str, "TNEI      %s != 0x%04x", RegNames[op.rs], op.immediate); }


void SprintOp_Cop0_Unk( char * str, u32 address, OpCode op )			{ sprintf(str, "Cop0_Unk?"); }
void SprintOp_Cop0_MFC0( char * str, u32 address, OpCode op )			{ sprintf(str, "MFC0      %s <- %s", RegNames[op.rt], Cop0RegNames[op.fs]); }
void SprintOp_Cop0_MTC0( char * str, u32 address, OpCode op )			{ sprintf(str, "MTC0      %s -> %s", RegNames[op.rt], Cop0RegNames[op.fs]); }
void SprintOp_Cop0_TLB( char * str, u32 address, OpCode op )			{ SprintOp_TLBInstructions[op.cop0tlb_funct](str, address, op); }


void SprintOp_TLB_Unk( char * str, u32 address, OpCode op )				{ sprintf(str, "TLB_Unk?"); }
void SprintOp_TLB_TLBR( char * str, u32 address, OpCode op )			{ sprintf(str, "TLBR"); }
void SprintOp_TLB_TLBWI( char * str, u32 address, OpCode op )			{ sprintf(str, "TLBWI"); }
void SprintOp_TLB_TLBWR( char * str, u32 address, OpCode op )			{ sprintf(str, "TLBWR"); }
void SprintOp_TLB_TLBP( char * str, u32 address, OpCode op )			{ sprintf(str, "TLBP"); }
void SprintOp_TLB_ERET( char * str, u32 address, OpCode op )			{ sprintf(str, "ERET"); }

void SprintOp_Cop1_Unk( char * str, u32 address, OpCode op )			{ sprintf(str, "Cop1_Unk?"); }
void SprintOp_Cop1_MFC1( char * str, u32 address, OpCode op )			{ sprintf(str, "MFC1      %s <- FP%02d", RegNames[op.rt], op.fs); }
void SprintOp_Cop1_DMFC1( char * str, u32 address, OpCode op )			{ sprintf(str, "DMFC1     %s <- FP%02d", RegNames[op.rt], op.fs); }
void SprintOp_Cop1_CFC1( char * str, u32 address, OpCode op )			{ sprintf(str, "CFC1      %s <- CCR%02d", RegNames[op.rt], op.rd); }
void SprintOp_Cop1_MTC1( char * str, u32 address, OpCode op )			{ sprintf(str, "MTC1      %s -> FP%02d", RegNames[op.rt], op.fs); }
void SprintOp_Cop1_DMTC1( char * str, u32 address, OpCode op )			{ sprintf(str, "DMTC1     %s -> FP%02d", RegNames[op.rt], op.fs); }
void SprintOp_Cop1_CTC1( char * str, u32 address, OpCode op )			{ sprintf(str, "CTC1      %s -> CCR%02d", RegNames[op.rt], op.rd); }
void SprintOp_Cop1_BCInstr( char * str, u32 address, OpCode op )		{ sprintf(str, "%-10.10s%08x", Cop1BC1OpCodeNames[op.cop1_bc], BranchAddress(op, address)); }


void SprintOp_Cop1_SInstr( char * str, u32 address, OpCode op ) { SprintOp_Cop1SInstruction[op.cop1_funct](str, address, op); }

void SprintOp_Cop1_S_Unk( char * str, u32 address, OpCode op )			{ sprintf(str, "Cop1_S_Unk?"); }
void SprintOp_Cop1_S_ADD( char * str, u32 address, OpCode op )			{ sprintf(str, "ADD.S     FP%02d = FP%02d + FP%02d", op.fd, op.fs, op.ft); }
void SprintOp_Cop1_S_SUB( char * str, u32 address, OpCode op )			{ sprintf(str, "SUB.S     FP%02d = FP%02d - FP%02d", op.fd, op.fs, op.ft); }
void SprintOp_Cop1_S_MUL( char * str, u32 address, OpCode op )			{ sprintf(str, "MUL.S     FP%02d = FP%02d * FP%02d", op.fd, op.fs, op.ft); }
void SprintOp_Cop1_S_DIV( char * str, u32 address, OpCode op )			{ sprintf(str, "DIV.S     FP%02d = FP%02d / FP%02d", op.fd, op.fs, op.ft); }
void SprintOp_Cop1_S_SQRT( char * str, u32 address, OpCode op )			{ sprintf(str, "SQRT.S    FP%02d = sqrt(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_S_ABS( char * str, u32 address, OpCode op )			{ sprintf(str, "ABS.S     FP%02d = fabs(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_S_MOV( char * str, u32 address, OpCode op )			{ sprintf(str, "MOV.S     FP%02d = FP%02d", op.fd, op.fs); }
void SprintOp_Cop1_S_NEG( char * str, u32 address, OpCode op )			{ sprintf(str, "NEG.S     FP%02d = -FP%02d", op.fd, op.fs); }
void SprintOp_Cop1_S_ROUND_L( char * str, u32 address, OpCode op )		{ sprintf(str, "RND.L.S   FP%02d = round(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_S_TRUNC_L( char * str, u32 address, OpCode op )		{ sprintf(str, "TRUN.L.S  FP%02d = trunc(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_S_CEIL_L( char * str, u32 address, OpCode op )		{ sprintf(str, "CEIL.L.S  FP%02d = ceil(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_S_FLOOR_L( char * str, u32 address, OpCode op )		{ sprintf(str, "FLR.L.S   FP%02d = floor(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_S_ROUND_W( char * str, u32 address, OpCode op )		{ sprintf(str, "RND.W.S   FP%02d = round(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_S_TRUNC_W( char * str, u32 address, OpCode op )		{ sprintf(str, "TRUN.W.S  FP%02d = trunc(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_S_CEIL_W( char * str, u32 address, OpCode op )		{ sprintf(str, "CEIL.W.S  FP%02d = ceil(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_S_FLOOR_W( char * str, u32 address, OpCode op )		{ sprintf(str, "FLR.W.S   FP%02d = floor(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_S_CVT_D( char * str, u32 address, OpCode op )		{ sprintf(str, "CVT.D.S   FP%02d = (d)FP%02d", op.fd, op.fs); }
void SprintOp_Cop1_S_CVT_W( char * str, u32 address, OpCode op )		{ sprintf(str, "CVT.W.S   FP%02d = (w)FP%02d", op.fd, op.fs); }
void SprintOp_Cop1_S_CVT_L( char * str, u32 address, OpCode op )		{ sprintf(str, "CVT.L.S   FP%02d = (l)FP%02d", op.fd, op.fs); }

void SprintOp_Cop1_S_F( char * str, u32 address, OpCode op )			{ sprintf(str, "C.F.S     FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_S_UN( char * str, u32 address, OpCode op )			{ sprintf(str, "C.UN.S    FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_S_EQ( char * str, u32 address, OpCode op )			{ sprintf(str, "C.EQ.S    FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_S_UEQ( char * str, u32 address, OpCode op )			{ sprintf(str, "C.UEQ.S   FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_S_OLT( char * str, u32 address, OpCode op )			{ sprintf(str, "C.OLT.S   FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_S_ULT( char * str, u32 address, OpCode op )			{ sprintf(str, "C.ULT.S   FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_S_OLE( char * str, u32 address, OpCode op )			{ sprintf(str, "C.OLE.S   FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_S_ULE( char * str, u32 address, OpCode op )			{ sprintf(str, "C.ULE.S   FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_S_SF( char * str, u32 address, OpCode op )			{ sprintf(str, "C.SF.S    FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_S_NGLE( char * str, u32 address, OpCode op )			{ sprintf(str, "C.NGLE.S  FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_S_SEQ( char * str, u32 address, OpCode op )			{ sprintf(str, "C.SEQ.S   FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_S_NGL( char * str, u32 address, OpCode op )			{ sprintf(str, "C.NGL.S   FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_S_LT( char * str, u32 address, OpCode op )			{ sprintf(str, "C.LT.S    FP%02d < FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_S_NGE( char * str, u32 address, OpCode op )			{ sprintf(str, "C.NGE.S   FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_S_LE( char * str, u32 address, OpCode op ) 			{ sprintf(str, "C.LE.S    FP%02d <= FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_S_NGT( char * str, u32 address, OpCode op ) 			{ sprintf(str, "C.NGT.S   FP%02d ? FP%02d", op.fs, op.ft); }

void SprintOp_Cop1_DInstr( char * str, u32 address, OpCode op )			{ SprintOp_Cop1DInstruction[op.cop1_funct](str, address, op); }

void SprintOp_Cop1_D_Unk( char * str, u32 address, OpCode op )			{ sprintf(str, "Cop1_D_Unk?"); }
void SprintOp_Cop1_D_ADD( char * str, u32 address, OpCode op )			{ sprintf(str, "ADD.D     FP%02d = FP%02d + FP%02d", op.fd, op.fs, op.ft); }
void SprintOp_Cop1_D_SUB( char * str, u32 address, OpCode op )			{ sprintf(str, "SUB.D     FP%02d = FP%02d - FP%02d", op.fd, op.fs, op.ft); }
void SprintOp_Cop1_D_MUL( char * str, u32 address, OpCode op )			{ sprintf(str, "MUL.D     FP%02d = FP%02d * FP%02d", op.fd, op.fs, op.ft); }
void SprintOp_Cop1_D_DIV( char * str, u32 address, OpCode op )			{ sprintf(str, "DIV.D     FP%02d = FP%02d / FP%02d", op.fd, op.fs, op.ft); }
void SprintOp_Cop1_D_SQRT( char * str, u32 address, OpCode op )			{ sprintf(str, "SQRT.D    FP%02d = sqrt(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_D_ABS( char * str, u32 address, OpCode op )			{ sprintf(str, "ABS.D     FP%02d = fabs(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_D_MOV( char * str, u32 address, OpCode op )			{ sprintf(str, "MOV.D     FP%02d = FP%02d", op.fd, op.fs); }
void SprintOp_Cop1_D_NEG( char * str, u32 address, OpCode op )			{ sprintf(str, "NEG.D     FP%02d = -FP%02d", op.fd, op.fs); }
void SprintOp_Cop1_D_ROUND_L( char * str, u32 address, OpCode op )		{ sprintf(str, "RND.L.D   FP%02d = round(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_D_TRUNC_L( char * str, u32 address, OpCode op )		{ sprintf(str, "TRUN.L.D  FP%02d = trunc(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_D_CEIL_L( char * str, u32 address, OpCode op )		{ sprintf(str, "CEIL.L.D  FP%02d = ceil(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_D_FLOOR_L( char * str, u32 address, OpCode op )		{ sprintf(str, "FLR.L.D   FP%02d = floor(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_D_ROUND_W( char * str, u32 address, OpCode op )		{ sprintf(str, "RND.W.D   FP%02d = round(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_D_TRUNC_W( char * str, u32 address, OpCode op )		{ sprintf(str, "TRUN.W.D  FP%02d = trunc(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_D_CEIL_W( char * str, u32 address, OpCode op )		{ sprintf(str, "CEIL.W.D  FP%02d = ceil(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_D_FLOOR_W( char * str, u32 address, OpCode op )		{ sprintf(str, "FLR.W.D   FP%02d = floor(FP%02d)", op.fd, op.fs); }
void SprintOp_Cop1_D_CVT_S( char * str, u32 address, OpCode op )		{ sprintf(str, "CVT.S.D   FP%02d = (s)FP%02d", op.fd, op.fs); }
void SprintOp_Cop1_D_CVT_W( char * str, u32 address, OpCode op )		{ sprintf(str, "CVT.W.D   FP%02d = (w)FP%02d", op.fd, op.fs); }
void SprintOp_Cop1_D_CVT_L( char * str, u32 address, OpCode op )		{ sprintf(str, "CVT.L.D   FP%02d = (l)FP%02d", op.fd, op.fs); }

void SprintOp_Cop1_D_F( char * str, u32 address, OpCode op )			{ sprintf(str, "C.F.D     FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_D_UN( char * str, u32 address, OpCode op )			{ sprintf(str, "C.UN.D    FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_D_EQ( char * str, u32 address, OpCode op )			{ sprintf(str, "C.EQ.D    FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_D_UEQ( char * str, u32 address, OpCode op )			{ sprintf(str, "C.UEQ.D   FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_D_OLT( char * str, u32 address, OpCode op )			{ sprintf(str, "C.OLT.D   FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_D_ULT( char * str, u32 address, OpCode op )			{ sprintf(str, "C.ULT.D   FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_D_OLE( char * str, u32 address, OpCode op )			{ sprintf(str, "C.OLE.D   FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_D_ULE( char * str, u32 address, OpCode op )			{ sprintf(str, "C.ULE.D   FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_D_SF( char * str, u32 address, OpCode op )			{ sprintf(str, "C.DF.D    FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_D_NGLE( char * str, u32 address, OpCode op )			{ sprintf(str, "C.NGLE.D  FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_D_SEQ( char * str, u32 address, OpCode op )			{ sprintf(str, "C.DEQ.D   FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_D_NGL( char * str, u32 address, OpCode op )			{ sprintf(str, "C.NGL.D   FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_D_LT( char * str, u32 address, OpCode op )			{ sprintf(str, "C.LT.D    FP%02d < FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_D_NGE( char * str, u32 address, OpCode op )			{ sprintf(str, "C.NGE.D   FP%02d ? FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_D_LE( char * str, u32 address, OpCode op ) 			{ sprintf(str, "C.LE.D    FP%02d <= FP%02d", op.fs, op.ft); }
void SprintOp_Cop1_D_NGT( char * str, u32 address, OpCode op ) 			{ sprintf(str, "C.NGT.D   FP%02d ? FP%02d", op.fs, op.ft); }


void SprintOp_Cop1_WInstr( char * str, u32 address, OpCode op )
{
	sprintf(str, "%-10.10sFP%02d,FP%02d", Cop1WOpCodeNames[op.cop1_funct], op.fd, op.fs);

}
void SprintOp_Cop1_LInstr( char * str, u32 address, OpCode op )
{
	sprintf(str, "%-10.10sFP%02d,FP%02d", Cop1LOpCodeNames[op.cop1_funct], op.fd, op.fs);
}






//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////// RSP /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


// Opcode Jump Table
SprintOpInstruction SprintRSPOp_Instructions[64] = {
	SprintRSPOp_Special, SprintRSPOp_RegImm, SprintRSPOp_J,      SprintRSPOp_JAL,   SprintRSPOp_BEQ,  SprintRSPOp_BNE, SprintRSPOp_BLEZ, SprintRSPOp_BGTZ,
	SprintRSPOp_ADDI,    SprintRSPOp_ADDIU,  SprintRSPOp_SLTI,   SprintRSPOp_SLTIU, SprintRSPOp_ANDI, SprintRSPOp_ORI, SprintRSPOp_XORI, SprintRSPOp_LUI,
	SprintRSPOp_CoPro0,  SprintRSPOp_Unk,    SprintRSPOp_CoPro2, SprintRSPOp_Unk,   SprintRSPOp_Unk,  SprintRSPOp_Unk, SprintRSPOp_Unk,  SprintRSPOp_Unk,
	SprintRSPOp_Unk,     SprintRSPOp_Unk,    SprintRSPOp_Unk,    SprintRSPOp_Unk,   SprintRSPOp_Unk,  SprintRSPOp_Unk, SprintRSPOp_Unk,  SprintRSPOp_Unk,
	SprintRSPOp_LB,      SprintRSPOp_LH,     SprintRSPOp_Unk,    SprintRSPOp_LW,    SprintRSPOp_LBU,  SprintRSPOp_LHU, SprintRSPOp_Unk,  SprintRSPOp_Unk,
	SprintRSPOp_SB,      SprintRSPOp_SH,     SprintRSPOp_Unk,    SprintRSPOp_SW,    SprintRSPOp_Unk,  SprintRSPOp_Unk, SprintRSPOp_Unk,  SprintRSPOp_Unk,
	SprintRSPOp_Unk,     SprintRSPOp_Unk,    SprintRSPOp_LWC2,   SprintRSPOp_Unk,   SprintRSPOp_Unk,  SprintRSPOp_Unk, SprintRSPOp_Unk,  SprintRSPOp_Unk,
	SprintRSPOp_Unk,     SprintRSPOp_Unk,    SprintRSPOp_SWC2,   SprintRSPOp_Unk,   SprintRSPOp_Unk,  SprintRSPOp_Unk, SprintRSPOp_Unk,  SprintRSPOp_Unk
};

SprintOpInstruction SprintRSPOp_SpecialInstructions[64] = {
	SprintRSPOp_Special_SLL, SprintRSPOp_Special_Unk,  SprintRSPOp_Special_SRL, SprintRSPOp_Special_SRA,  SprintRSPOp_Special_SLLV, SprintRSPOp_Special_Unk,   SprintRSPOp_Special_SRLV, SprintRSPOp_Special_SRAV,
	SprintRSPOp_Special_JR,  SprintRSPOp_Special_JALR, SprintRSPOp_Special_Unk, SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk,  SprintRSPOp_Special_BREAK, SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk,
	SprintRSPOp_Special_Unk, SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk, SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk,   SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk,
	SprintRSPOp_Special_Unk, SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk, SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk,   SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk,
	SprintRSPOp_Special_ADD, SprintRSPOp_Special_ADDU, SprintRSPOp_Special_SUB, SprintRSPOp_Special_SUBU, SprintRSPOp_Special_AND,  SprintRSPOp_Special_OR,    SprintRSPOp_Special_XOR,  SprintRSPOp_Special_NOR,
	SprintRSPOp_Special_Unk, SprintRSPOp_Special_Unk,  SprintRSPOp_Special_SLT, SprintRSPOp_Special_SLTU, SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk,   SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk,
	SprintRSPOp_Special_Unk, SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk, SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk,   SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk,
	SprintRSPOp_Special_Unk, SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk, SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk,   SprintRSPOp_Special_Unk,  SprintRSPOp_Special_Unk
};

SprintOpInstruction SprintRSPOp_RegImmInstructions[32] = {
	SprintRSPOp_RegImm_BLTZ,   SprintRSPOp_RegImm_BGEZ,   SprintRSPOp_RegImm_Unk,  SprintRSPOp_RegImm_Unk, SprintRSPOp_RegImm_Unk, SprintRSPOp_RegImm_Unk, SprintRSPOp_RegImm_Unk, SprintRSPOp_RegImm_Unk,
	SprintRSPOp_RegImm_Unk,    SprintRSPOp_RegImm_Unk,    SprintRSPOp_RegImm_Unk,  SprintRSPOp_RegImm_Unk, SprintRSPOp_RegImm_Unk, SprintRSPOp_RegImm_Unk, SprintRSPOp_RegImm_Unk, SprintRSPOp_RegImm_Unk,
	SprintRSPOp_RegImm_BLTZAL, SprintRSPOp_RegImm_BGEZAL, SprintRSPOp_RegImm_Unk,  SprintRSPOp_RegImm_Unk, SprintRSPOp_RegImm_Unk, SprintRSPOp_RegImm_Unk, SprintRSPOp_RegImm_Unk, SprintRSPOp_RegImm_Unk,
	SprintRSPOp_RegImm_Unk,    SprintRSPOp_RegImm_Unk,    SprintRSPOp_RegImm_Unk,  SprintRSPOp_RegImm_Unk, SprintRSPOp_RegImm_Unk, SprintRSPOp_RegImm_Unk, SprintRSPOp_RegImm_Unk, SprintRSPOp_RegImm_Unk
};

SprintOpInstruction SprintRSPOp_Cop0Instructions[32] = {
	SprintRSPOp_Cop0_MFC0, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_MTC0, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk,
	SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk,
	SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk,
	SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk, SprintRSPOp_Cop0_Unk,
};

SprintOpInstruction SprintRSPOp_LWC2Instructions[32] = {
	SprintRSPOp_LWC2_LBV, SprintRSPOp_LWC2_LSV, SprintRSPOp_LWC2_LLV, SprintRSPOp_LWC2_LDV, SprintRSPOp_LWC2_LQV, SprintRSPOp_LWC2_LRV, SprintRSPOp_LWC2_LPV, SprintRSPOp_LWC2_LUV,
	SprintRSPOp_LWC2_LHV, SprintRSPOp_LWC2_LFV, SprintRSPOp_LWC2_LWV, SprintRSPOp_LWC2_LTV, SprintRSPOp_LWC2_Unk, SprintRSPOp_LWC2_Unk, SprintRSPOp_LWC2_Unk, SprintRSPOp_LWC2_Unk,
	SprintRSPOp_LWC2_Unk, SprintRSPOp_LWC2_Unk, SprintRSPOp_LWC2_Unk, SprintRSPOp_LWC2_Unk, SprintRSPOp_LWC2_Unk, SprintRSPOp_LWC2_Unk, SprintRSPOp_LWC2_Unk, SprintRSPOp_LWC2_Unk,
	SprintRSPOp_LWC2_Unk, SprintRSPOp_LWC2_Unk, SprintRSPOp_LWC2_Unk, SprintRSPOp_LWC2_Unk, SprintRSPOp_LWC2_Unk, SprintRSPOp_LWC2_Unk, SprintRSPOp_LWC2_Unk, SprintRSPOp_LWC2_Unk,
};

SprintOpInstruction SprintRSPOp_SWC2Instructions[32] = {
	SprintRSPOp_SWC2_SBV, SprintRSPOp_SWC2_SSV, SprintRSPOp_SWC2_SLV, SprintRSPOp_SWC2_SDV, SprintRSPOp_SWC2_SQV, SprintRSPOp_SWC2_SRV, SprintRSPOp_SWC2_SPV, SprintRSPOp_SWC2_SUV,
	SprintRSPOp_SWC2_SHV, SprintRSPOp_SWC2_SFV, SprintRSPOp_SWC2_SWV, SprintRSPOp_SWC2_STV, SprintRSPOp_SWC2_Unk, SprintRSPOp_SWC2_Unk, SprintRSPOp_SWC2_Unk, SprintRSPOp_SWC2_Unk,
	SprintRSPOp_SWC2_Unk, SprintRSPOp_SWC2_Unk, SprintRSPOp_SWC2_Unk, SprintRSPOp_SWC2_Unk, SprintRSPOp_SWC2_Unk, SprintRSPOp_SWC2_Unk, SprintRSPOp_SWC2_Unk, SprintRSPOp_SWC2_Unk,
	SprintRSPOp_SWC2_Unk, SprintRSPOp_SWC2_Unk, SprintRSPOp_SWC2_Unk, SprintRSPOp_SWC2_Unk, SprintRSPOp_SWC2_Unk, SprintRSPOp_SWC2_Unk, SprintRSPOp_SWC2_Unk, SprintRSPOp_SWC2_Unk,
};

SprintOpInstruction SprintRSPOp_Cop2Instructions[64] =
{
	SprintRSPOp_Cop2_MFC2, SprintRSPOp_Cop2_Unk, SprintRSPOp_Cop2_Unk, SprintRSPOp_Cop2_Unk, SprintRSPOp_Cop2_MTC2, SprintRSPOp_Cop2_Unk, SprintRSPOp_Cop2_Unk, SprintRSPOp_Cop2_Unk,
	SprintRSPOp_Cop2_Unk,  SprintRSPOp_Cop2_Unk, SprintRSPOp_Cop2_Unk, SprintRSPOp_Cop2_Unk, SprintRSPOp_Cop2_Unk, SprintRSPOp_Cop2_Unk, SprintRSPOp_Cop2_Unk, SprintRSPOp_Cop2_Unk,
	SprintRSPOp_Cop2_VOP,  SprintRSPOp_Cop2_VOP, SprintRSPOp_Cop2_VOP, SprintRSPOp_Cop2_VOP, SprintRSPOp_Cop2_VOP, SprintRSPOp_Cop2_VOP, SprintRSPOp_Cop2_VOP, SprintRSPOp_Cop2_VOP,
	SprintRSPOp_Cop2_VOP,  SprintRSPOp_Cop2_VOP, SprintRSPOp_Cop2_VOP, SprintRSPOp_Cop2_VOP, SprintRSPOp_Cop2_VOP, SprintRSPOp_Cop2_VOP, SprintRSPOp_Cop2_VOP, SprintRSPOp_Cop2_VOP
};

SprintOpInstruction SprintRSPOp_VopInstructions[64] = {
	SprintRSPOp_Vop_VMULF, SprintRSPOp_Vop_VMULU, SprintRSPOp_Vop_VRNDP, SprintRSPOp_Vop_VMULQ, SprintRSPOp_Vop_VMUDL, SprintRSPOp_Vop_VMUDM, SprintRSPOp_Vop_VMUDN, SprintRSPOp_Vop_VMUDH,
	SprintRSPOp_Vop_VMACF, SprintRSPOp_Vop_VMACU, SprintRSPOp_Vop_VRNDN, SprintRSPOp_Vop_VMACQ, SprintRSPOp_Vop_VMADL, SprintRSPOp_Vop_VMADM, SprintRSPOp_Vop_VMADN, SprintRSPOp_Vop_VMADH,
	SprintRSPOp_Vop_VADD,  SprintRSPOp_Vop_VSUB,  SprintRSPOp_Vop_VSUT,  SprintRSPOp_Vop_VABS,  SprintRSPOp_Vop_VADDC, SprintRSPOp_Vop_VSUBC, SprintRSPOp_Vop_VADDB, SprintRSPOp_Vop_VSUBB,
	SprintRSPOp_Vop_VACCB, SprintRSPOp_Vop_VSUCB, SprintRSPOp_Vop_VSAD,  SprintRSPOp_Vop_VSAC,  SprintRSPOp_Vop_VSUM,  SprintRSPOp_Vop_VSAW,  SprintRSPOp_Vop_Unk,   SprintRSPOp_Vop_Unk,
	SprintRSPOp_Vop_VLT,   SprintRSPOp_Vop_VEQ,   SprintRSPOp_Vop_VNE,   SprintRSPOp_Vop_VGE,   SprintRSPOp_Vop_VCL,   SprintRSPOp_Vop_VCH,   SprintRSPOp_Vop_VCR,   SprintRSPOp_Vop_VMRG,
	SprintRSPOp_Vop_VAND,  SprintRSPOp_Vop_VNAND, SprintRSPOp_Vop_VOR,   SprintRSPOp_Vop_VNOR,  SprintRSPOp_Vop_VXOR,  SprintRSPOp_Vop_VNXOR, SprintRSPOp_Vop_Unk,   SprintRSPOp_Vop_Unk,
	SprintRSPOp_Vop_VRCP,  SprintRSPOp_Vop_VRCPL, SprintRSPOp_Vop_VRCPH, SprintRSPOp_Vop_VMOV,  SprintRSPOp_Vop_VRSQ,  SprintRSPOp_Vop_VRSQL, SprintRSPOp_Vop_VRSQH, SprintRSPOp_Vop_Unk,
	SprintRSPOp_Vop_Unk,   SprintRSPOp_Vop_Unk,   SprintRSPOp_Vop_Unk,   SprintRSPOp_Vop_Unk,   SprintRSPOp_Vop_Unk,   SprintRSPOp_Vop_Unk,   SprintRSPOp_Vop_Unk,   SprintRSPOp_Vop_Unk
};

#define RSPJumpTarget(op, address) (((op).target)<<2)



void SprintRSPOp_Unk( char * str, u32 address, OpCode op )		{ sprintf(str, "?"); }


void SprintRSPOp_Special( char * str, u32 address, OpCode op )	{ SprintRSPOp_SpecialInstructions[op.spec_op](str, address, op); }
void SprintRSPOp_RegImm( char * str, u32 address, OpCode op )	{ SprintRSPOp_RegImmInstructions[op.regimm_op](str, address, op); }
void SprintRSPOp_CoPro0( char * str, u32 address, OpCode op )	{ SprintRSPOp_Cop0Instructions[op.cop0_op](str, address, op); }
void SprintRSPOp_CoPro2( char * str, u32 address, OpCode op )	{ SprintRSPOp_Cop2Instructions[op.cop1_op](str, address, op); }
void SprintRSPOp_LWC2( char * str, u32 address, OpCode op )		{ SprintRSPOp_LWC2Instructions[op.rd](str, address, op); }
void SprintRSPOp_SWC2( char * str, u32 address, OpCode op )		{ SprintRSPOp_SWC2Instructions[op.rd](str, address, op); }

void SprintRSPOp_Cop2_VOP( char * str, u32 address, OpCode op )	{ SprintRSPOp_VopInstructions[op.cop2_funct](str, address, op); }

void SprintRSPOp_J( char * str, u32 address, OpCode op )		{ sprintf(str, "J         0x%04x        %s", RSPJumpTarget(op, address), "?" ); }
void SprintRSPOp_JAL( char * str, u32 address, OpCode op )		{ sprintf(str, "JAL       0x%04x        %s", RSPJumpTarget(op, address), "?" ); }
void SprintRSPOp_BEQ( char * str, u32 address, OpCode op ) {
						if (op.rs == 0 && op.rt == 0)             sprintf(str, "B         --> 0x%04x", address+4 + (s16)op.immediate*4);
						else                                      sprintf(str, "BEQ       %s == %s --> 0x%04x", RegNames[op.rs], RegNames[op.rt], BranchAddress(op, address)); }
void SprintRSPOp_BNE( char * str, u32 address, OpCode op )		{ sprintf(str, "BNE       %s != %s --> 0x%04x", RegNames[op.rs], RegNames[op.rt], BranchAddress(op, address)); }
void SprintRSPOp_BLEZ( char * str, u32 address, OpCode op )		{ sprintf(str, "BLEZ      %s <= 0 --> 0x%04x", RegNames[op.rs], BranchAddress(op, address)); }
void SprintRSPOp_BGTZ( char * str, u32 address, OpCode op )		{ sprintf(str, "BGTZ      %s > 0 --> 0x%04x", RegNames[op.rs], BranchAddress(op, address)); }
void SprintRSPOp_ADDI( char * str, u32 address, OpCode op )		{ sprintf(str, "ADDI      %s = %s + 0x%04x", RegNames[op.rt], RegNames[op.rs], op.immediate); }
void SprintRSPOp_ADDIU( char * str, u32 address, OpCode op )	{ sprintf(str, "ADDIU     %s = %s + 0x%04x", RegNames[op.rt], RegNames[op.rs], op.immediate); }
void SprintRSPOp_SLTI( char * str, u32 address, OpCode op )		{ sprintf(str, "SLTI      %s = (%s < 0x%04x)", RegNames[op.rt], RegNames[op.rs], op.immediate); }
void SprintRSPOp_SLTIU( char * str, u32 address, OpCode op )	{ sprintf(str, "SLTIU     %s = (%s < 0x%04x)", RegNames[op.rt], RegNames[op.rs], op.immediate); }
void SprintRSPOp_ANDI( char * str, u32 address, OpCode op )		{ sprintf(str, "ANDI      %s = %s & 0x%04x", RegNames[op.rt], RegNames[op.rs], op.immediate); }
void SprintRSPOp_ORI( char * str, u32 address, OpCode op )		{ sprintf(str, "ORI       %s = %s | 0x%04x", RegNames[op.rt], RegNames[op.rs], op.immediate); }
void SprintRSPOp_XORI( char * str, u32 address, OpCode op )		{ sprintf(str, "XORI      %s = %s ^ 0x%04x", RegNames[op.rt], RegNames[op.rs], op.immediate); }
void SprintRSPOp_LUI( char * str, u32 address, OpCode op )		{ sprintf(str, "LUI       %s = 0x%08x", RegNames[op.rt], op.immediate<<16); }
void SprintRSPOp_LB( char * str, u32 address, OpCode op )		{ sprintf(str, "LB        %s <- 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintRSPOp_LH( char * str, u32 address, OpCode op )		{ sprintf(str, "LH        %s <- 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintRSPOp_LW( char * str, u32 address, OpCode op )		{ sprintf(str, "LW        %s <- 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintRSPOp_LBU( char * str, u32 address, OpCode op )		{ sprintf(str, "LBU       %s <- 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintRSPOp_LHU( char * str, u32 address, OpCode op )		{ sprintf(str, "LHU       %s <- 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintRSPOp_SB( char * str, u32 address, OpCode op )		{ sprintf(str, "SB        %s -> 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintRSPOp_SH( char * str, u32 address, OpCode op )		{ sprintf(str, "SH        %s -> 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }
void SprintRSPOp_SW( char * str, u32 address, OpCode op )		{ sprintf(str, "SW        %s -> 0x%04x(%s)", RegNames[op.rt], op.immediate, RegNames[op.rs]); }

void SprintRSPOp_Special_Unk( char * str, u32 address, OpCode op )		{ sprintf(str, "?"); }

void SprintRSPOp_Special_SLL( char * str, u32 address, OpCode op )		{
								if (op._u32 == 0)						  sprintf(str, "NOP");
								else									  sprintf(str, "SLL       %s = %s << 0x%04x", RegNames[op.rd], RegNames[op.rt], op.sa); }
void SprintRSPOp_Special_SRL( char * str, u32 address, OpCode op )		{ sprintf(str, "SRL       %s = %s >> 0x%04x", RegNames[op.rd], RegNames[op.rt], op.sa); }
void SprintRSPOp_Special_SRA( char * str, u32 address, OpCode op )		{ sprintf(str, "SRA       %s = %s >> 0x%04x", RegNames[op.rd], RegNames[op.rt], op.sa); }
void SprintRSPOp_Special_SLLV( char * str, u32 address, OpCode op )		{ sprintf(str, "SLLV      %s = %s << %s", RegNames[op.rd], RegNames[op.rt], RegNames[op.rs]); }
void SprintRSPOp_Special_SRLV( char * str, u32 address, OpCode op )		{ sprintf(str, "SRLV      %s = %s >> %s", RegNames[op.rd], RegNames[op.rt], RegNames[op.rs]); }
void SprintRSPOp_Special_SRAV( char * str, u32 address, OpCode op )		{ sprintf(str, "SRAV      %s = %s >> %s", RegNames[op.rd], RegNames[op.rt], RegNames[op.rs]); }
void SprintRSPOp_Special_JR( char * str, u32 address, OpCode op )		{ sprintf(str, "JR        %s", RegNames[op.rs]); }
void SprintRSPOp_Special_JALR( char * str, u32 address, OpCode op )		{ sprintf(str, "JALR      %s, %s", RegNames[op.rd], RegNames[op.rs]); }
void SprintRSPOp_Special_BREAK( char * str, u32 address, OpCode op )	{ sprintf(str, "BREAK     0x%08x", (op._u32>>6)&0xFFFFF); }
void SprintRSPOp_Special_ADD( char * str, u32 address, OpCode op )		{ sprintf(str, "ADD       %s = %s + %s", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintRSPOp_Special_ADDU( char * str, u32 address, OpCode op )		{ sprintf(str, "ADDU      %s = %s + %s", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintRSPOp_Special_SUB( char * str, u32 address, OpCode op )		{ sprintf(str, "SUB       %s = %s - %s", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintRSPOp_Special_SUBU( char * str, u32 address, OpCode op )		{ sprintf(str, "SUBU      %s = %s - %s", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintRSPOp_Special_AND( char * str, u32 address, OpCode op )		{ sprintf(str, "AND       %s = %s & %s", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintRSPOp_Special_OR( char * str, u32 address, OpCode op )		{
											if (op.rs == 0 && op.rt == 0) sprintf(str, "CLEAR     %s = 0", RegNames[op.rd]);
 											else if (op.rt == 0)		  sprintf(str, "MOV       %s = %s", RegNames[op.rd], RegNames[op.rs]);
											else						  sprintf(str, "OR        %s = %s | %s", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintRSPOp_Special_XOR( char * str, u32 address, OpCode op )		{ sprintf(str, "XOR       %s = %s ^ %s", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintRSPOp_Special_NOR( char * str, u32 address, OpCode op )		{ sprintf(str, "NOR       %s = ~(%s | %s)", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintRSPOp_Special_SLT( char * str, u32 address, OpCode op )		{ sprintf(str, "SLT       %s = (%s<%s)", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }
void SprintRSPOp_Special_SLTU( char * str, u32 address, OpCode op )		{ sprintf(str, "SLTU      %s = (%s<%s)", RegNames[op.rd], RegNames[op.rs], RegNames[op.rt]); }




void SprintRSPOp_RegImm_Unk( char * str, u32 address, OpCode op )		{ sprintf(str, "?"); }

void SprintRSPOp_RegImm_BLTZ( char * str, u32 address, OpCode op )		{ sprintf(str, "BLTZ      %s < 0 --> 0x%04x", RegNames[op.rs], BranchAddress(op, address)); }
void SprintRSPOp_RegImm_BGEZ( char * str, u32 address, OpCode op )		{ sprintf(str, "BGEZ      %s >= 0 --> 0x%04x", RegNames[op.rs], BranchAddress(op, address)); }
void SprintRSPOp_RegImm_BLTZAL( char * str, u32 address, OpCode op )	{ sprintf(str, "BLTZAL    %s < 0 --> 0x%04x", RegNames[op.rs], BranchAddress(op, address)); }
void SprintRSPOp_RegImm_BGEZAL( char * str, u32 address, OpCode op )	{ sprintf(str, "BGEZAL    %s >= 0 --> 0x%04x", RegNames[op.rs], BranchAddress(op, address)); }


void SprintRSPOp_Cop0_Unk( char * str, u32 address, OpCode op )			{ sprintf(str, "?"); }

static const char * const g_szRSPControlReg[16] =
{
	"SP_MEM_ADDR_REG",
	"SP_DRAM_ADDR_REG",
	"SP_RD_LEN_REG",
	"SP_WR_LEN_REG",
	"SP_STATUS_REG",
	"SP_DMA_FULL_REG",
	"SP_DMA_BUSY_REG",
	"SP_SEMAPHORE_REG",
	"DPC_START_REG",
	"DPC_END_REG",
	"DPC_CURRENT_REG",
	"DPC_STATUS_REG",
	"DPC_CLOCK_REG",
	"DPC_BUFBUSY_REG",
	"DPC_PIPEBUSY_REG",
	"DPC_TMEM_REG"
};




void SprintRSPOp_Cop0_MFC0( char * str, u32 address, OpCode op )		{ sprintf(str, "MFC0      %s <- [%s]", RegNames[op.rt], g_szRSPControlReg[op.fs&0x0F]); }
void SprintRSPOp_Cop0_MTC0( char * str, u32 address, OpCode op )		{ sprintf(str, "MTC0      %s -> [%s]", RegNames[op.rt], g_szRSPControlReg[op.fs&0x0F]); }

void SprintRSPOp_LWC2_Unk( char * str, u32 address, OpCode op )			{ sprintf(str, "?"); }

void SprintRSPOp_LWC2_LBV( char * str, u32 address, OpCode op )			{ sprintf(str, "LBV       vec%02d[%d] <- 0x%04x(%s)", op.rt, op.del, op.voffset<<0, RegNames[op.base]); }
void SprintRSPOp_LWC2_LSV( char * str, u32 address, OpCode op )			{ sprintf(str, "LSV       vec%02d[%d] <- 0x%04x(%s)", op.rt, op.del, op.voffset<<1, RegNames[op.base]); }
void SprintRSPOp_LWC2_LLV( char * str, u32 address, OpCode op )			{ sprintf(str, "LLV       vec%02d[%d] <- 0x%04x(%s)", op.rt, op.del, op.voffset<<2, RegNames[op.base]); }
void SprintRSPOp_LWC2_LDV( char * str, u32 address, OpCode op )			{ sprintf(str, "LDV       vec%02d[%d] <- 0x%04x(%s)", op.rt, op.del, op.voffset<<3, RegNames[op.base]); }
void SprintRSPOp_LWC2_LQV( char * str, u32 address, OpCode op )			{ sprintf(str, "LQV       vec%02d[%d] <- 0x%04x(%s)", op.rt, op.del, op.voffset<<4, RegNames[op.base]); }
void SprintRSPOp_LWC2_LTV( char * str, u32 address, OpCode op )			{ sprintf(str, "LTV       vec%02d[%d] <- 0x%04x(%s)", op.rt, op.del, op.voffset<<4, RegNames[op.base]); }
void SprintRSPOp_LWC2_LRV( char * str, u32 address, OpCode op )			{ sprintf(str, "LRV       vec%02d[%d] <- 0x%04x(%s)", op.rt, op.del, op.voffset<<4, RegNames[op.base]); }
void SprintRSPOp_LWC2_LPV( char * str, u32 address, OpCode op )			{ sprintf(str, "LPV       vec%02d[%d] <- 0x%04x(%s)", op.rt, op.del, op.voffset<<3, RegNames[op.base]); }
void SprintRSPOp_LWC2_LUV( char * str, u32 address, OpCode op )			{ sprintf(str, "LUV       vec%02d[%d] <- 0x%04x(%s)", op.rt, op.del, op.voffset<<3, RegNames[op.base]); }
void SprintRSPOp_LWC2_LHV( char * str, u32 address, OpCode op )			{ sprintf(str, "LHV       vec%02d[%d] <- 0x%04x(%s)", op.rt, op.del, op.voffset<<4, RegNames[op.base]); }
void SprintRSPOp_LWC2_LFV( char * str, u32 address, OpCode op )			{ sprintf(str, "LFV       vec%02d[%d] <- 0x%04x(%s)", op.rt, op.del, op.voffset<<4, RegNames[op.base]); }
void SprintRSPOp_LWC2_LWV( char * str, u32 address, OpCode op )			{ sprintf(str, "LWV       vec%02d[%d] <- 0x%04x(%s)", op.rt, op.del, op.voffset<<4, RegNames[op.base]); }

void SprintRSPOp_SWC2_Unk( char * str, u32 address, OpCode op )			{ sprintf(str, "?"); }

void SprintRSPOp_SWC2_SBV( char * str, u32 address, OpCode op )			{ sprintf(str, "SBV       vec%02d[%d] -> 0x%04x(%s)", op.rt, op.del, op.voffset<<0, RegNames[op.base]); }
void SprintRSPOp_SWC2_SSV( char * str, u32 address, OpCode op )			{ sprintf(str, "SSV       vec%02d[%d] -> 0x%04x(%s)", op.rt, op.del, op.voffset<<1, RegNames[op.base]); }
void SprintRSPOp_SWC2_SLV( char * str, u32 address, OpCode op )			{ sprintf(str, "SLV       vec%02d[%d] -> 0x%04x(%s)", op.rt, op.del, op.voffset<<2, RegNames[op.base]); }
void SprintRSPOp_SWC2_SDV( char * str, u32 address, OpCode op )			{ sprintf(str, "SDV       vec%02d[%d] -> 0x%04x(%s)", op.rt, op.del, op.voffset<<3, RegNames[op.base]); }
void SprintRSPOp_SWC2_SQV( char * str, u32 address, OpCode op )			{ sprintf(str, "SQV       vec%02d[%d] -> 0x%04x(%s)", op.rt, op.del, op.voffset<<4, RegNames[op.base]); }
void SprintRSPOp_SWC2_STV( char * str, u32 address, OpCode op )			{ sprintf(str, "STV       vec%02d[%d] -> 0x%04x(%s)", op.rt, op.del, op.voffset<<4, RegNames[op.base]); }
void SprintRSPOp_SWC2_SRV( char * str, u32 address, OpCode op )			{ sprintf(str, "SRV       vec%02d[%d] -> 0x%04x(%s)", op.rt, op.del, op.voffset<<4, RegNames[op.base]); }
void SprintRSPOp_SWC2_SPV( char * str, u32 address, OpCode op )			{ sprintf(str, "SPV       vec%02d[%d] -> 0x%04x(%s)", op.rt, op.del, op.voffset<<3, RegNames[op.base]); }
void SprintRSPOp_SWC2_SUV( char * str, u32 address, OpCode op )			{ sprintf(str, "SUV       vec%02d[%d] -> 0x%04x(%s)", op.rt, op.del, op.voffset<<3, RegNames[op.base]); }
void SprintRSPOp_SWC2_SHV( char * str, u32 address, OpCode op )			{ sprintf(str, "SHV       vec%02d[%d] -> 0x%04x(%s)", op.rt, op.del, op.voffset<<4, RegNames[op.base]); }
void SprintRSPOp_SWC2_SFV( char * str, u32 address, OpCode op )			{ sprintf(str, "SFV       vec%02d[%d] -> 0x%04x(%s)", op.rt, op.del, op.voffset<<4, RegNames[op.base]); }
void SprintRSPOp_SWC2_SWV( char * str, u32 address, OpCode op )			{ sprintf(str, "SWV       vec%02d[%d] -> 0x%04x(%s)", op.rt, op.del, op.voffset<<4, RegNames[op.base]); }

void SprintRSPOp_Cop2_Unk( char * str, u32 address, OpCode op )			{ sprintf(str, "?"); }
void SprintRSPOp_Cop2_MFC2( char * str, u32 address, OpCode op )		{ sprintf(str, "MFC2      %s <- vec%d[%d]", RegNames[op.rt], op.rd, op.sa>>1); }
void SprintRSPOp_Cop2_MTC2( char * str, u32 address, OpCode op )		{ sprintf(str, "MTC2      %s -> vec%d[%d]", RegNames[op.rt], op.rd, op.sa>>1); }

void SprintRSPOp_Vop_Unk( char * str, u32 address, OpCode op )			{ sprintf(str, "?"); }


static const char * gVSelName[16] =
{
	"",			// 0		// 0a??
	"",			// 1
	"0q",		// 2
	"1q",		// 3
	"0h",		// 4
	"1h",		// 5
	"2h",		// 6
	"3h",		// 7
	"0",		// 8
	"1",		// 9
	"2 ",		// 10
	"3 ",		// 11
	"4 ",		// 12
	"5 ",		// 13
	"6 ",		// 14
	"7 "		// 15
};

void SprintRSPOp_Vop_VMULF( char * str, u32 address, OpCode op )		{ sprintf(str, "VMULF     vec%02d  = vec%02d * vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VMULU( char * str, u32 address, OpCode op )		{ sprintf(str, "VMULU     vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VRNDP( char * str, u32 address, OpCode op )		{ sprintf(str, "VRNDP     vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VMULQ( char * str, u32 address, OpCode op )		{ sprintf(str, "VMULQ     vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VMUDL( char * str, u32 address, OpCode op )		{ sprintf(str, "VMUDL     vec%02d  = ( acc  = vec%02d * vec%02d[%s]       )", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VMUDM( char * str, u32 address, OpCode op )		{ sprintf(str, "VMUDM     vec%02d  = ( acc  = vec%02d * vec%02d[%s] >> 16 )", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VMUDN( char * str, u32 address, OpCode op )		{ sprintf(str, "VMUDN     vec%02d  = ( acc  = vec%02d * vec%02d[%s]       ) >> 16", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VMUDH( char * str, u32 address, OpCode op )		{ sprintf(str, "VMUDH     vec%02d  = ( acc  = vec%02d * vec%02d[%s] >> 16 ) >> 16", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }

void SprintRSPOp_Vop_VMACF( char * str, u32 address, OpCode op )		{ sprintf(str, "VMACF     vec%02d += vec%02d * vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VMACU( char * str, u32 address, OpCode op )		{ sprintf(str, "VMACU     vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VRNDN( char * str, u32 address, OpCode op )		{ sprintf(str, "VRNDN     vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VMACQ( char * str, u32 address, OpCode op )		{ sprintf(str, "VMACQ     vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VMADL( char * str, u32 address, OpCode op )		{ sprintf(str, "VMADL     vec%02d  = ( acc += vec%02d * vec%02d[%s]       )", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VMADM( char * str, u32 address, OpCode op )		{ sprintf(str, "VMADM     vec%02d  = ( acc += vec%02d * vec%02d[%s] >> 16 )", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VMADN( char * str, u32 address, OpCode op )		{ sprintf(str, "VMADN     vec%02d  = ( acc += vec%02d * vec%02d[%s]       ) >> 16", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VMADH( char * str, u32 address, OpCode op )		{ sprintf(str, "VMADH     vec%02d  = ( acc += vec%02d * vec%02d[%s] >> 16 ) >> 16", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }

void SprintRSPOp_Vop_VADD( char * str, u32 address, OpCode op )			{ sprintf(str, "VADD      vec%02d  = vec%02d + vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VSUB( char * str, u32 address, OpCode op )			{ sprintf(str, "VSUB      vec%02d  = vec%02d - vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VSUT( char * str, u32 address, OpCode op )			{ sprintf(str, "VSUT      vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VABS( char * str, u32 address, OpCode op )			{ sprintf(str, "VABS      vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VADDC( char * str, u32 address, OpCode op )		{ sprintf(str, "VADDC     vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VSUBC( char * str, u32 address, OpCode op )		{ sprintf(str, "VSUBC     vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VADDB( char * str, u32 address, OpCode op )		{ sprintf(str, "VADDB     vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VSUBB( char * str, u32 address, OpCode op )		{ sprintf(str, "VSUBB     vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }

void SprintRSPOp_Vop_VACCB( char * str, u32 address, OpCode op )		{ sprintf(str, "VACCB     vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VSUCB( char * str, u32 address, OpCode op )		{ sprintf(str, "VSUCB     vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VSAD( char * str, u32 address, OpCode op )			{ sprintf(str, "VSAD      vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VSAC( char * str, u32 address, OpCode op )			{ sprintf(str, "VSAC      vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VSUM( char * str, u32 address, OpCode op )			{ sprintf(str, "VSUM      vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VSAW( char * str, u32 address, OpCode op )			{ sprintf(str, "VSAW      vec%02d [%d] = vec%02d, vec%02d", op.sa, (op.rs & 0xf), op.rd, op.rt ); }

void SprintRSPOp_Vop_VLT( char * str, u32 address, OpCode op )			{ sprintf(str, "VLT       vec%02d  = vec%02d <  vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VEQ( char * str, u32 address, OpCode op )			{ sprintf(str, "VEQ       vec%02d  = vec%02d == vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VNE( char * str, u32 address, OpCode op )			{ sprintf(str, "VNE       vec%02d  = vec%02d != vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VGE( char * str, u32 address, OpCode op )			{ sprintf(str, "VGE       vec%02d  = vec%02d >= vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VCL( char * str, u32 address, OpCode op )			{ sprintf(str, "VCL       vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VCH( char * str, u32 address, OpCode op )			{ sprintf(str, "VCH       vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VCR( char * str, u32 address, OpCode op )			{ sprintf(str, "VCR       vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VMRG( char * str, u32 address, OpCode op )			{ sprintf(str, "VMRG      vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VAND( char * str, u32 address, OpCode op )			{ sprintf(str, "VAND      vec%02d  = vec%02d & vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VNAND( char * str, u32 address, OpCode op )		{ sprintf(str, "VNAND     vec%02d  = ~(vec%02d & vec%02d[%s])", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VOR( char * str, u32 address, OpCode op )			{ sprintf(str, "VOR       vec%02d  = vec%02d | vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VNOR( char * str, u32 address, OpCode op )			{ sprintf(str, "VNOR      vec%02d  = ~(vec%02d | vec%02d[%s])", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VXOR( char * str, u32 address, OpCode op )			{ sprintf(str, "VXOR      vec%02d  = vec%02d ^ vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VNXOR( char * str, u32 address, OpCode op )		{ sprintf(str, "VNXOR     vec%02d  = ~(vec%02d ^ vec%02d[%s])", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VRCP( char * str, u32 address, OpCode op )			{ sprintf(str, "VRCP      vec%02d [%d], vec%02d[%s]", op.sa, (op.rd & 0x7), op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VRCPL( char * str, u32 address, OpCode op )		{ sprintf(str, "VRCPL     vec%02d [%d],  vec%02d[%s]", op.sa, (op.rd & 0x7), op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VRCPH( char * str, u32 address, OpCode op )		{ sprintf(str, "VRCPH     vec%02d [%d],  vec%02d[%s]", op.sa, (op.rd & 0x7), op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VMOV( char * str, u32 address, OpCode op )			{ sprintf(str, "VMOV      vec%02d  = vec%02d ? vec%02d[%s]", op.sa, op.rd, op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VRSQ( char * str, u32 address, OpCode op )			{ sprintf(str, "VRSQ      vec%02d [%d],  vec%02d[%s]", op.sa, (op.rd & 0x7), op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VRSQL( char * str, u32 address, OpCode op )		{ sprintf(str, "VRSQL     vec%02d [%d],  vec%02d[%s]", op.sa, (op.rd & 0x7), op.rt, gVSelName[ op.rs & 0xf ]); }
void SprintRSPOp_Vop_VRSQH( char * str, u32 address, OpCode op )		{ sprintf(str, "VRSQH     vec%02d [%d],  vec%02d[%s]", op.sa, (op.rd & 0x7), op.rt, gVSelName[ op.rs & 0xf ]); }



void SprintOpCodeInfo( char *str, u32 address, OpCode op )
{
	op = GetCorrectOp( op );
	SprintOp_Instructions[ op.op ]( str, address, op );
}

void SprintRSPOpCodeInfo(char *str, u32 address, OpCode op)
{
	SprintRSPOp_Instructions[ op.op ]( str, address, op );
}

#endif // DAEDALUS_SILENT

