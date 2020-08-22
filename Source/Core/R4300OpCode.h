/*
Copyright (C) 2001,2006 StrmnNrmn

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

#ifndef CORE_R4300OPCODE_H_
#define CORE_R4300OPCODE_H_

#include "Base/Types.h"
#include "System/Endian.h"
#include "N64Reg.h"

enum OpCodeValue {
	OP_SPECOP		= 0,
	OP_REGIMM		= 1,
	OP_J			= 2,
	OP_JAL			= 3,
	OP_BEQ			= 4,
	OP_BNE			= 5,
	OP_BLEZ			= 6,
	OP_BGTZ			= 7,
	OP_ADDI			= 8,
	OP_ADDIU		= 9,
	OP_SLTI			= 10,
	OP_SLTIU		= 11,
	OP_ANDI			= 12,
	OP_ORI			= 13,
	OP_XORI			= 14,
	OP_LUI			= 15,
	OP_COPRO0		= 16,
	OP_COPRO1		= 17,
	OP_SRHACK1		= 18,	//hack OP
	OP_SRHACK2		= 19,	//hack OP
	OP_BEQL			= 20,
	OP_BNEL			= 21,
	OP_BLEZL		= 22,
	OP_BGTZL		= 23,
	OP_DADDI		= 24,
	OP_DADDIU		= 25,
	OP_LDL			= 26,
	OP_LDR			= 27,
	OP_PATCH		= 28,	//hack OP
	OP_SRHACK_UNOPT = 29,	//hack OP
	OP_SRHACK_OPT   = 30,	//hack OP
	OP_SRHACK_NOOPT = 31,	//hack OP
	OP_UNK4			= 29,	//unknown OP
	OP_UNK5			= 30,	//unknown OP
	OP_UNK6			= 31,	//unknown OP
	OP_LB			= 32,
	OP_LH			= 33,
	OP_LWL			= 34,
	OP_LW 			= 35,
	OP_LBU			= 36,
	OP_LHU			= 37,
	OP_LWR			= 38,
	OP_LWU			= 39,
	OP_SB			= 40,
	OP_SH			= 41,
	OP_SWL			= 42,
	OP_SW			= 43,
	OP_SDL			= 44,
	OP_SDR			= 45,
	OP_SWR			= 46,
	OP_CACHE		= 47,
	OP_LL			= 48,
	OP_LWC1			= 49,
	OP_UNK7			= 50,	//unknown OP
	OP_UNK8			= 51,	//unknown OP
	OP_LLD			= 52,
	OP_LDC1			= 53,
	OP_LDC2 		= 54,	//not used OP
	OP_LD			= 55,
	OP_SC 			= 56,
	OP_SWC1 		= 57,
	OP_DBG_BKPT		= 58,	//unknown OP
	OP_UNK10		= 59,	//unknown OP
	OP_SCD 			= 60,
	OP_SDC1 		= 61,
	OP_SDC2 		= 62,	//not used OP
	OP_SD			= 63
};

enum ESpecOp
{
	SpecOp_SLL				= 0,
//	SpecOp_UNK11			= 1,
	SpecOp_SRL				= 2,
	SpecOp_SRA				= 3,
	SpecOp_SLLV				= 4,
//	SpecOp_UNK12			= 5,
	SpecOp_SRLV				= 6,
	SpecOp_SRAV				= 7,
	SpecOp_JR				= 8,
	SpecOp_JALR				= 9,
/*	SpecOp_UNK13			= 10,
	SpecOp_UNK14			= 11,*/
	SpecOp_SYSCALL			= 12,
	SpecOp_BREAK			= 13,
//	SpecOp_UNK15			= 14,
	SpecOp_SYNC				= 15,
	SpecOp_MFHI				= 16,
	SpecOp_MTHI				= 17,
	SpecOp_MFLO				= 18,
	SpecOp_MTLO				= 19,
	SpecOp_DSLLV			= 20,
//	SpecOp_UNK16			= 21,
	SpecOp_DSRLV			= 22,
	SpecOp_DSRAV			= 23,
	SpecOp_MULT				= 24,
	SpecOp_MULTU			= 25,
	SpecOp_DIV				= 26,
	SpecOp_DIVU				= 27,
	SpecOp_DMULT			= 28,
	SpecOp_DMULTU			= 29,
	SpecOp_DDIV				= 30,
	SpecOp_DDIVU			= 31,
	SpecOp_ADD				= 32,
	SpecOp_ADDU				= 33,
	SpecOp_SUB				= 34,
	SpecOp_SUBU				= 35,
	SpecOp_AND				= 36,
	SpecOp_OR				= 37,
	SpecOp_XOR				= 38,
	SpecOp_NOR				= 39,
/*	SpecOp_UNK17			= 40,
	SpecOp_UNK18			= 41,*/
	SpecOp_SLT				= 42,
	SpecOp_SLTU				= 43,
	SpecOp_DADD				= 44,
	SpecOp_DADDU			= 45,
	SpecOp_DSUB				= 46,
	SpecOp_DSUBU			= 47,
	SpecOp_TGE				= 48,
	SpecOp_TGEU				= 49,
	SpecOp_TLT				= 50,
	SpecOp_TLTU				= 51,
	SpecOp_TEQ				= 52,
//	SpecOp_UNK19			= 53,
	SpecOp_TNE				= 54,
//	SpecOp_UNK20			= 55,
	SpecOp_DSLL				= 56,
//	SpecOp_UNK21			= 57,
	SpecOp_DSRL				= 58,
	SpecOp_DSRA				= 59,
	SpecOp_DSLL32			= 60,
//	SpecOp_UNK22			= 61,
	SpecOp_DSRL32			= 62,
	SpecOp_DSRA32			= 63,

};

enum ERegImmOp
{
	RegImmOp_BLTZ			= 0,
	RegImmOp_BGEZ			= 1,
	RegImmOp_BLTZL			= 2,
	RegImmOp_BGEZL			= 3,
	RegImmOp_TGEI			= 8,
	RegImmOp_TGEIU			= 9,
	RegImmOp_TLTI			= 10,
	RegImmOp_TLTIU			= 11,
	RegImmOp_TEQI			= 12,
	RegImmOp_TNEI			= 14,
	RegImmOp_BLTZAL			= 16,
	RegImmOp_BGEZAL			= 17,
	RegImmOp_BLTZALL		= 18,
	RegImmOp_BGEZALL		= 19
};

enum ECop0Op
{
	Cop0Op_MFC0		= 0,
	Cop0Op_MTC0		= 4,
	Cop0Op_TLB		= 16
};

enum TLBOpCodeValue {
	OP_TLBR = 1,
	OP_TLBWI = 2,
	OP_TLBWR = 6,
	OP_TLBBP = 8,
	OP_ERET = 24
};

enum ECop1Op
{
	Cop1Op_MFC1	= 0,
	Cop1Op_DMFC1	= 1,
	Cop1Op_CFC1	= 2,
	Cop1Op_MTC1	= 4,
	Cop1Op_DMTC1	= 5,
	Cop1Op_CTC1	= 6,

	Cop1Op_BCInstr = 8,
	Cop1Op_SInstr = 16,
	Cop1Op_DInstr = 17,
	Cop1Op_WInstr = 20,
	Cop1Op_LInstr = 21

};

enum ECop1OpFunction
{
	Cop1OpFunc_ADD = 0,
	Cop1OpFunc_SUB = 1,
	Cop1OpFunc_MUL = 2,
	Cop1OpFunc_DIV = 3,
	Cop1OpFunc_SQRT = 4,
	Cop1OpFunc_ABS = 5,
	Cop1OpFunc_MOV = 6,
	Cop1OpFunc_NEG = 7,

	Cop1OpFunc_ROUND_L = 8,
	Cop1OpFunc_TRUNC_L = 9,
	Cop1OpFunc_CEIL_L = 10,
	Cop1OpFunc_FLOOR_L = 11,
	Cop1OpFunc_ROUND_W = 12,
	Cop1OpFunc_TRUNC_W = 13,
	Cop1OpFunc_CEIL_W = 14,
	Cop1OpFunc_FLOOR_W = 15,

	Cop1OpFunc_CVT_S = 32,
	Cop1OpFunc_CVT_D = 33,
	Cop1OpFunc_CVT_W = 36,
	Cop1OpFunc_CVT_L = 37,

	Cop1OpFunc_CMP_F = 48,
	Cop1OpFunc_CMP_UN = 49,
	Cop1OpFunc_CMP_EQ = 50,
	Cop1OpFunc_CMP_UEQ = 51,
	Cop1OpFunc_CMP_OLT = 52,
	Cop1OpFunc_CMP_ULT = 53,
	Cop1OpFunc_CMP_OLE = 54,
	Cop1OpFunc_CMP_ULE = 55,

	Cop1OpFunc_CMP_SF = 56,
	Cop1OpFunc_CMP_NGLE = 57,
	Cop1OpFunc_CMP_SEQ = 58,
	Cop1OpFunc_CMP_NGL = 59,
	Cop1OpFunc_CMP_LT = 60,
	Cop1OpFunc_CMP_NGE = 61,
	Cop1OpFunc_CMP_LE = 62,
	Cop1OpFunc_CMP_NGT = 63,
};

enum ECop1BCOp
{
	Cop1BCOp_BC1F = 0,
	Cop1BCOp_BC1T = 1,
	Cop1BCOp_BC1FL = 2,
	Cop1BCOp_BC1TL = 3
};

struct OpCode
{
	union
	{
		u32 _u32;

#if (DAEDALUS_ENDIAN_MODE == DAEDALUS_ENDIAN_BIG)
		struct
		{
			unsigned op : 6;
			unsigned rs : 5;			// EN64Reg
			unsigned rt : 5;			// EN64Reg
			unsigned offset : 16;
		};

		struct
		{
			unsigned : 6;
			unsigned base : 5;			// EN64Reg
			unsigned : 5;
			unsigned immediate : 16;
		};

		struct
		{
			unsigned : 6;
			unsigned target : 26;
		};

		// SPECIAL
		struct
		{
			unsigned : 6;
			unsigned : 5;
			unsigned : 5;
			unsigned rd : 5;			// EN64Reg
			unsigned sa : 5;
			unsigned spec_op : 6;		// ESpecOp
		};

		// REGIMM
		struct
		{
			unsigned : 11;
			unsigned regimm_op : 5;		// ERegImmOp
			unsigned : 16;
		};

		// COP0 op
		struct
		{
			unsigned : 6;
			unsigned cop0_op : 5;		// ECop0Op
			unsigned : 21;
		};

		// COP1 op
		struct
		{
			unsigned : 6;
			unsigned cop1_op : 5;
			unsigned ft : 5;		// rt
			unsigned fs : 5;		// rd
			unsigned fd : 5;		// sa
			unsigned cop1_funct : 6;
		};


		struct
		{
			unsigned : 14;
			unsigned cop1_bc : 2;		// ECop1BCOp
			unsigned : 16;
		};

		struct
		{
			unsigned : 6;
			unsigned : 5;
			unsigned dest   : 5;
			unsigned : 5;
			unsigned del    : 4;
			signed   voffset : 7;
		};

		// COP0 TLB op
		struct
		{
			unsigned : 26;
			unsigned cop0tlb_funct : 6;
		};

		// COP2 vector op
		struct
		{
			unsigned : 26;
			unsigned cop2_funct : 6;
		};

		struct
		{
			unsigned : 6;
			unsigned bp_index : 26;
		};

		struct
		{
			unsigned : 6;
			unsigned dynarec_index : 26;
		};

		struct
		{
			unsigned : 6;
			unsigned trace_branch_index : 26;
		};

		struct
		{
			unsigned : 6;
			unsigned : 2;
			unsigned patch_index : 24;
		};
#elif (DAEDALUS_ENDIAN_MODE == DAEDALUS_ENDIAN_LITTLE)
		struct
		{
			unsigned offset : 16;
			unsigned rt : 5;			// EN64Reg
			unsigned rs : 5;			// EN64Reg
			unsigned op : 6;
		};

		struct
		{
			unsigned immediate : 16;
			unsigned : 5;
			unsigned base : 5;			// EN64Reg
			unsigned : 6;
		};

		struct
		{
			unsigned target : 26;
			unsigned : 6;
		};

		// SPECIAL
		struct
		{
			unsigned spec_op : 6;		// ESpecOp
			unsigned sa : 5;
			unsigned rd : 5;			// EN64Reg
			unsigned : 5;
			unsigned : 5;
			unsigned : 6;
		};

		// REGIMM
		struct
		{
			unsigned : 16;
			unsigned regimm_op : 5;		// ERegImmOp
			unsigned : 11;
		};

		// COP0 op
		struct
		{
			unsigned : 21;
			unsigned cop0_op : 5;		// ECop0Op
			unsigned : 6;
		};

		// COP1 op
		struct
		{
			unsigned cop1_funct : 6;
			unsigned fd : 5;		// sa
			unsigned fs : 5;		// rd
			unsigned ft : 5;		// rt
			unsigned cop1_op : 5;
			unsigned : 6;
		};


		struct
		{
			unsigned : 16;
			unsigned cop1_bc : 2;		// ECop1BCOp
			unsigned : 14;
		};

		struct
		{
			signed   voffset : 7;
			unsigned del    : 4;
			unsigned : 5;
			unsigned dest   : 5;
			unsigned : 5;
			unsigned : 6;
		};

		// COP0 TLB op
		struct
		{
			unsigned cop0tlb_funct : 6;
			unsigned : 26;
		};

		// COP2 vector op
		struct
		{
			unsigned cop2_funct : 6;
			unsigned : 26;
		};

		struct
		{
			unsigned bp_index : 26;
			unsigned : 6;
		};

		struct
		{
			unsigned dynarec_index : 26;
			unsigned : 6;
		};

		struct
		{
			unsigned trace_branch_index : 26;
			unsigned : 6;
		};

		struct
		{
			unsigned patch_index : 24;
			unsigned : 2;
			unsigned : 6;
		};
#else
#error No DAEDALUS_ENDIAN_MODE specified
#endif

	};
};

// Make sure we don't mess this up :)
DAEDALUS_STATIC_ASSERT( sizeof( OpCode ) == sizeof( u32 ) );

#endif // CORE_R4300OPCODE_H_
