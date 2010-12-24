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




/*
    CPU: Instructions encoded by opcode field.
    31---------26---------------------------------------------------0
    |  opcode   |                                                   |
    ------6----------------------------------------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
000 | *1    | *2    | J     | JAL   | BEQ   | BNE   | BLEZ  | BGTZ  |
001 | ADDI  | ADDIU | SLTI  | SLTIU | ANDI  | ORI   | XORI  | LUI   |
010 | *3    | *4    |  ---  |  ---  | BEQL  | BNEL  | BLEZL | BGTZL |
011 | DADDI |DADDIU | LDL   | LDR   | Patch |SRHackU|SRHackO|SRHackN|
100 | LB    | LH    | LWL   | LW    | LBU   | LHU   | LWR   | LWU   |
101 | SB    | SH    | SWL   | SW    | SDL   | SDR   | SWR   | CACHE |
110 | LL    | LWC1  |  ---  |  ---  | LLD   | LDC1  | LDC2  | LD    |
111 | SC    | SWC1  | DBkpt |  ---  | SCD   | SDC1  | SDC2  | SD    |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
     *1 = SPECIAL, see SPECIAL list    *2 = REGIMM, see REGIMM list
     *3 = COP0                         *4 = COP1
	// Patch, SRHackU, SRHackO, SRHackN are all optimisation hacks
	// DBkpt is a debugging breakpoint
};
*/

// Opcode Jump Table
CPU_Instruction R4300Instruction[64] = {
	R4300_Special, R4300_RegImm, R4300_J, R4300_JAL, R4300_BEQ, R4300_BNE, R4300_BLEZ, R4300_BGTZ,
	R4300_ADDI, R4300_ADDIU, R4300_SLTI, R4300_SLTIU, R4300_ANDI, R4300_ORI, R4300_XORI, R4300_LUI,
	R4300_CoPro0, R4300_CoPro1, R4300_Unk, R4300_Unk, R4300_BEQL, R4300_BNEL, R4300_BLEZL, R4300_BGTZL,
	R4300_DADDI, R4300_DADDIU, R4300_LDL, R4300_LDR, R4300_Unk, R4300_Unk, R4300_Unk, R4300_Unk,
	R4300_LB, R4300_LH, R4300_LWL, R4300_LW, R4300_LBU, R4300_LHU, R4300_LWR, R4300_LWU,
	R4300_SB, R4300_SH, R4300_SWL, R4300_SW, R4300_SDL, R4300_SDR, R4300_SWR, R4300_CACHE,
	R4300_LL, R4300_LWC1, R4300_Unk, R4300_Unk, R4300_LLD, R4300_LDC1, R4300_LDC2, R4300_LD,
	R4300_SC, R4300_SWC1, R4300_DBG_Bkpt, R4300_Unk, R4300_SCD, R4300_SDC1, R4300_SDC2, R4300_SD
};

/*
    SPECIAL: Instr. encoded by function field when opcode field = SPECIAL.
    31---------26------------------------------------------5--------0
    | = SPECIAL |                                         | function|
    ------6----------------------------------------------------6-----
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
000 | SLL   |  ---  | SRL   | SRA   | SLLV  |  ---  | SRLV  | SRAV  |
001 | JR    | JALR  |  ---  |  ---  |SYSCALL| BREAK |  ---  | SYNC  |
010 | MFHI  | MTHI  | MFLO  | MTLO  | DSLLV |  ---  | DSRLV | DSRAV |
011 | MULT  | MULTU | DIV   | DIVU  | DMULT | DMULTU| DDIV  | DDIVU |
100 | ADD   | ADDU  | SUB   | SUBU  | AND   | OR    | XOR   | NOR   |
101 |  ---  |  ---  | SLT   | SLTU  | DADD  | DADDU | DSUB  | DSUBU |
110 | TGE   | TGEU  | TLT   | TLTU  | TEQ   |  ---  | TNE   |  ---  |
111 | DSLL  |  ---  | DSRL  | DSRA  |DSLL32 |  ---  |DSRL32 |DSRA32 |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
*/


// SpecialOpCode Jump Table
CPU_Instruction R4300SpecialInstruction[64] = {
	R4300_Special_SLL, R4300_Special_Unk, R4300_Special_SRL, R4300_Special_SRA, R4300_Special_SLLV, R4300_Special_Unk, R4300_Special_SRLV, R4300_Special_SRAV,
	R4300_Special_JR, R4300_Special_JALR, R4300_Special_Unk, R4300_Special_Unk, R4300_Special_SYSCALL, R4300_Special_BREAK, R4300_Special_Unk, R4300_Special_SYNC,
	R4300_Special_MFHI, R4300_Special_MTHI, R4300_Special_MFLO, R4300_Special_MTLO, R4300_Special_DSLLV, R4300_Special_Unk, R4300_Special_DSRLV, R4300_Special_DSRAV,
	R4300_Special_MULT, R4300_Special_MULTU, R4300_Special_DIV, R4300_Special_DIVU, R4300_Special_DMULT, R4300_Special_DMULTU, R4300_Special_DDIV, R4300_Special_DDIVU,
	R4300_Special_ADD, R4300_Special_ADDU, R4300_Special_SUB, R4300_Special_SUBU, R4300_Special_AND, R4300_Special_OR, R4300_Special_XOR, R4300_Special_NOR,
	R4300_Special_Unk, R4300_Special_Unk, R4300_Special_SLT, R4300_Special_SLTU, R4300_Special_DADD, R4300_Special_DADDU, R4300_Special_DSUB, R4300_Special_DSUBU,
	R4300_Special_TGE, R4300_Special_TGEU, R4300_Special_TLT, R4300_Special_TLTU, R4300_Special_TEQ, R4300_Special_Unk, R4300_Special_TNE, R4300_Special_Unk,
	R4300_Special_DSLL, R4300_Special_Unk, R4300_Special_DSRL, R4300_Special_DSRA, R4300_Special_DSLL32, R4300_Special_Unk, R4300_Special_DSRL32, R4300_Special_DSRA32
};

/*
    REGIMM: Instructions encoded by the rt field when opcode field = REGIMM.
    31---------26----------20-------16------------------------------0
    | = REGIMM  |          |   rt    |                              |
    ------6---------------------5------------------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
 00 | BLTZ  | BGEZ  | BLTZL | BGEZL |  ---  |  ---  |  ---  |  ---  |
 01 | TGEI  | TGEIU | TLTI  | TLTIU | TEQI  |  ---  | TNEI  |  ---  |
 10 | BLTZAL| BGEZAL|BLTZALL|BGEZALL|  ---  |  ---  |  ---  |  ---  |
 11 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
 */

CPU_Instruction R4300RegImmInstruction[32] = {

	R4300_RegImm_BLTZ,   R4300_RegImm_BGEZ,   R4300_RegImm_BLTZL,   R4300_RegImm_BGEZL,   R4300_RegImm_Unk,  R4300_RegImm_Unk, R4300_RegImm_Unk,  R4300_RegImm_Unk,
	R4300_RegImm_TGEI,   R4300_RegImm_TGEIU,  R4300_RegImm_TLTI,    R4300_RegImm_TLTIU,   R4300_RegImm_TEQI, R4300_RegImm_Unk, R4300_RegImm_TNEI, R4300_RegImm_Unk,
	R4300_RegImm_BLTZAL, R4300_RegImm_BGEZAL, R4300_RegImm_BLTZALL, R4300_RegImm_BGEZALL, R4300_RegImm_Unk,  R4300_RegImm_Unk, R4300_RegImm_Unk,  R4300_RegImm_Unk,
	R4300_RegImm_Unk,    R4300_RegImm_Unk,    R4300_RegImm_Unk,     R4300_RegImm_Unk,     R4300_RegImm_Unk,  R4300_RegImm_Unk, R4300_RegImm_Unk,  R4300_RegImm_Unk
};


/*
    COP0: Instructions encoded by the fmt field when opcode = COP0.
    31--------26-25------21 ----------------------------------------0
    |  = COP0   |   fmt   |                                         |
    ------6----------5-----------------------------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
 00 | MFC0  |  ---  |  ---  |  ---  | MTC0  |  ---  |  ---  |  ---  |
 01 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 10 | *1    |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 11 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
     *1 = TLB instr, see TLB list
*/

// COP0 Jump Table
CPU_Instruction R4300Cop0Instruction[32] = {
	R4300_Cop0_MFC0, R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_MTC0, R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk,
	R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk,
	R4300_Cop0_TLB, R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk,
	R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk, R4300_Cop0_Unk,
};


/*
    TLB: Instructions encoded by the function field when opcode
         = COP0 and fmt = TLB.
    31--------26-25------21 -------------------------------5--------0
    |  = COP0   |  = TLB  |                               | function|
    ------6----------5-----------------------------------------6-----
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
000 |  ---  | TLBR  | TLBWI |  ---  |  ---  |  ---  | TLBWR |  ---  |
001 | TLBP  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
010 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
011 | ERET  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
100 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
101 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
110 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
111 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|

*/

// TLBOpCode Jump Table
CPU_Instruction R4300TLBInstruction[64] = {
	R4300_TLB_Unk, R4300_TLB_TLBR, R4300_TLB_TLBWI, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_TLBWR, R4300_TLB_Unk,
	R4300_TLB_TLBP, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk,
	R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk,
	R4300_TLB_ERET, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk,
	R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk,
	R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk,
	R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk,
	R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk, R4300_TLB_Unk,
};


/*
    COP1: Instructions encoded by the fmt field when opcode = COP1.
    31--------26-25------21 ----------------------------------------0
    |  = COP1   |   fmt   |                                         |
    ------6----------5-----------------------------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
 00 | MFC1  | DMFC1 | CFC1  |  ---  | MTC1  | DMTC1 | CTC1  |  ---  |
 01 | *1    |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 10 | *2    | *3    |  ---  |  ---  | *4    | *5    |  ---  |  ---  |
 11 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
     *1 = BC instructions, see BC1 list
     *2 = S instr, see FPU list            *3 = D instr, see FPU list
     *4 = W instr, see FPU list            *5 = L instr, see FPU list
*/

// COP1 Jump Table
CPU_Instruction R4300Cop1Instruction[32] = {
	R4300_Cop1_MFC1,    R4300_Cop1_DMFC1,  R4300_Cop1_CFC1, R4300_Cop1_Unk, R4300_Cop1_MTC1,   R4300_Cop1_DMTC1,  R4300_Cop1_CTC1, R4300_Cop1_Unk,
	R4300_Cop1_BCInstr, R4300_Cop1_Unk,    R4300_Cop1_Unk,  R4300_Cop1_Unk, R4300_Cop1_Unk,    R4300_Cop1_Unk,    R4300_Cop1_Unk,  R4300_Cop1_Unk,
	R4300_Cop1_SInstr,  R4300_Cop1_DInstr, R4300_Cop1_Unk,  R4300_Cop1_Unk, R4300_Cop1_WInstr, R4300_Cop1_LInstr, R4300_Cop1_Unk,  R4300_Cop1_Unk,
	R4300_Cop1_Unk,     R4300_Cop1_Unk,    R4300_Cop1_Unk,  R4300_Cop1_Unk, R4300_Cop1_Unk,    R4300_Cop1_Unk,    R4300_Cop1_Unk,  R4300_Cop1_Unk
};

/*
    BC1: Instructions encoded by the nd and tf fields when opcode
         = COP1 and fmt = BC
    31--------26-25------21 ---17--16-------------------------------0
    |  = COP1   |  = BC   |    |nd|tf|                              |
    ------6----------5-----------1--1--------------------------------
    |---0---|---1---| tf
  0 | BC1F  | BC1T  |
  1 | BC1FL | BC1TL |
 nd |-------|-------|
*/

CPU_Instruction R4300Cop1BC1Instruction[4] = {
	R4300_BC1_BC1F, R4300_BC1_BC1T, R4300_BC1_BC1FL, R4300_BC1_BC1TL
};


/*
    FPU: Instructions encoded by the function field when opcode = COP1
         and fmt = S
    31--------26-25------21 -------------------------------5--------0
    |  = COP1   |  = S    |                               | function|
    ------6----------5-----------------------------------------6-----
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
000 | ADD   | SUB   | MUL   | DIV   | SQRT  | ABS   | MOV   | NEG   |
001 |ROUND.L|TRUNC.L| CEIL.L|FLOOR.L|ROUND.W|TRUNC.W| CEIL.W|FLOOR.W|
010 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
011 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
100 |  ---  | CVT.D |  ---  |  ---  | CVT.W | CVT.L |  ---  |  ---  |
101 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
110 | C.F   | C.UN  | C.EQ  | C.UEQ | C.OLT | C.ULT | C.OLE | C.ULE |
111 | C.SF  | C.NGLE| C.SEQ | C.NGL | C.LT  | C.NGE | C.LE  | C.NGT |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
*/

// Single Jump Table
CPU_Instruction R4300Cop1SInstruction[64] = {
	R4300_Cop1_S_ADD,     R4300_Cop1_S_SUB,     R4300_Cop1_S_MUL,    R4300_Cop1_S_DIV,     R4300_Cop1_S_SQRT,    R4300_Cop1_S_ABS,     R4300_Cop1_S_MOV,    R4300_Cop1_S_NEG,
	R4300_Cop1_S_ROUND_L, R4300_Cop1_S_TRUNC_L,	R4300_Cop1_S_CEIL_L, R4300_Cop1_S_FLOOR_L, R4300_Cop1_S_ROUND_W, R4300_Cop1_S_TRUNC_W, R4300_Cop1_S_CEIL_W, R4300_Cop1_S_FLOOR_W,
	R4300_Cop1_S_Unk,     R4300_Cop1_S_Unk,     R4300_Cop1_S_Unk,    R4300_Cop1_S_Unk,     R4300_Cop1_S_Unk,     R4300_Cop1_S_Unk,     R4300_Cop1_S_Unk,    R4300_Cop1_S_Unk, 
	R4300_Cop1_S_Unk,     R4300_Cop1_S_Unk,     R4300_Cop1_S_Unk,    R4300_Cop1_S_Unk,     R4300_Cop1_S_Unk,     R4300_Cop1_S_Unk,     R4300_Cop1_S_Unk,    R4300_Cop1_S_Unk, 
	R4300_Cop1_S_Unk,     R4300_Cop1_S_CVT_D,   R4300_Cop1_S_Unk,    R4300_Cop1_S_Unk,     R4300_Cop1_S_CVT_W,   R4300_Cop1_S_CVT_L,   R4300_Cop1_S_Unk,    R4300_Cop1_S_Unk,
	R4300_Cop1_S_Unk,     R4300_Cop1_S_Unk,     R4300_Cop1_S_Unk,    R4300_Cop1_S_Unk,     R4300_Cop1_S_Unk,     R4300_Cop1_S_Unk,     R4300_Cop1_S_Unk,    R4300_Cop1_S_Unk, 
	R4300_Cop1_S_F,       R4300_Cop1_S_UN,      R4300_Cop1_S_EQ,     R4300_Cop1_S_UEQ,     R4300_Cop1_S_OLT,     R4300_Cop1_S_ULT,     R4300_Cop1_S_OLE,    R4300_Cop1_S_ULE,
	R4300_Cop1_S_SF,      R4300_Cop1_S_NGLE,    R4300_Cop1_S_SEQ,    R4300_Cop1_S_NGL,     R4300_Cop1_S_LT,      R4300_Cop1_S_NGE,     R4300_Cop1_S_LE,     R4300_Cop1_S_NGT
};

/*
    FPU: Instructions encoded by the function field when opcode = COP1
         and fmt = D
    31--------26-25------21 -------------------------------5--------0
    |  = COP1   |  = D    |                               | function|
    ------6----------5-----------------------------------------6-----
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
000 | ADD   | SUB   | MUL   | DIV   | SQRT  | ABS   | MOV   | NEG   |
001 |ROUND.L|TRUNC.L| CEIL.L|FLOOR.L|ROUND.W|TRUNC.W| CEIL.W|FLOOR.W|
010 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
011 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
100 | CVT.S |  ---  |  ---  |  ---  | CVT.W | CVT.L |  ---  |  ---  |
101 |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
110 | C.F   | C.UN  | C.EQ  | C.UEQ | C.OLT | C.ULT | C.OLE | C.ULE |
111 | C.SF  | C.NGLE| C.SEQ | C.NGL | C.LT  | C.NGE | C.LE  | C.NGT |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
*/

// Double Jump Table
CPU_Instruction R4300Cop1DInstruction_64[64] = {
    R4300_Cop1_D_ADD<true>,     R4300_Cop1_D_SUB<true>,     R4300_Cop1_D_MUL<true>,    R4300_Cop1_D_DIV<true>,     R4300_Cop1_D_SQRT<true>,    R4300_Cop1_D_ABS<true>,     R4300_Cop1_D_MOV<true>,    R4300_Cop1_D_NEG<true>,
    R4300_Cop1_D_ROUND_L<true>, R4300_Cop1_D_TRUNC_L<true>, R4300_Cop1_D_CEIL_L<true>, R4300_Cop1_D_FLOOR_L<true>, R4300_Cop1_D_ROUND_W<true>, R4300_Cop1_D_TRUNC_W<true>, R4300_Cop1_D_CEIL_W<true>, R4300_Cop1_D_FLOOR_W<true>,
    R4300_Cop1_D_Unk<true>,     R4300_Cop1_D_Unk<true>,     R4300_Cop1_D_Unk<true>,    R4300_Cop1_D_Unk<true>,     R4300_Cop1_D_Unk<true>,     R4300_Cop1_D_Unk<true>,     R4300_Cop1_D_Unk<true>,    R4300_Cop1_D_Unk<true>, 
    R4300_Cop1_D_Unk<true>,     R4300_Cop1_D_Unk<true>,     R4300_Cop1_D_Unk<true>,    R4300_Cop1_D_Unk<true>,     R4300_Cop1_D_Unk<true>,     R4300_Cop1_D_Unk<true>,     R4300_Cop1_D_Unk<true>,    R4300_Cop1_D_Unk<true>, 
    R4300_Cop1_D_CVT_S<true>,   R4300_Cop1_D_Unk<true>,     R4300_Cop1_D_Unk<true>,    R4300_Cop1_D_Unk<true>,     R4300_Cop1_D_CVT_W<true>,   R4300_Cop1_D_CVT_L<true>,   R4300_Cop1_D_Unk<true>,    R4300_Cop1_D_Unk<true>,
    R4300_Cop1_D_Unk<true>,     R4300_Cop1_D_Unk<true>,     R4300_Cop1_D_Unk<true>,    R4300_Cop1_D_Unk<true>,     R4300_Cop1_D_Unk<true>,     R4300_Cop1_D_Unk<true>,     R4300_Cop1_D_Unk<true>,    R4300_Cop1_D_Unk<true>, 
    R4300_Cop1_D_F<true>,       R4300_Cop1_D_UN<true>,      R4300_Cop1_D_EQ<true>,     R4300_Cop1_D_UEQ<true>,     R4300_Cop1_D_OLT<true>,     R4300_Cop1_D_ULT<true>,     R4300_Cop1_D_OLE<true>,    R4300_Cop1_D_ULE<true>,
    R4300_Cop1_D_SF<true>,      R4300_Cop1_D_NGLE<true>,    R4300_Cop1_D_SEQ<true>,    R4300_Cop1_D_NGL<true>,     R4300_Cop1_D_LT<true>,      R4300_Cop1_D_NGE<true>,     R4300_Cop1_D_LE<true>,     R4300_Cop1_D_NGT<true>,
};

CPU_Instruction R4300Cop1DInstruction_32[64] =
{
    R4300_Cop1_D_ADD<false>,     R4300_Cop1_D_SUB<false>,     R4300_Cop1_D_MUL<false>,    R4300_Cop1_D_DIV<false>,     R4300_Cop1_D_SQRT<false>,    R4300_Cop1_D_ABS<false>,     R4300_Cop1_D_MOV<false>,    R4300_Cop1_D_NEG<false>,
    R4300_Cop1_D_ROUND_L<false>, R4300_Cop1_D_TRUNC_L<false>, R4300_Cop1_D_CEIL_L<false>, R4300_Cop1_D_FLOOR_L<false>, R4300_Cop1_D_ROUND_W<false>, R4300_Cop1_D_TRUNC_W<false>, R4300_Cop1_D_CEIL_W<false>, R4300_Cop1_D_FLOOR_W<false>,
    R4300_Cop1_D_Unk<false>,     R4300_Cop1_D_Unk<false>,     R4300_Cop1_D_Unk<false>,    R4300_Cop1_D_Unk<false>,     R4300_Cop1_D_Unk<false>,     R4300_Cop1_D_Unk<false>,     R4300_Cop1_D_Unk<false>,    R4300_Cop1_D_Unk<false>, 
    R4300_Cop1_D_Unk<false>,     R4300_Cop1_D_Unk<false>,     R4300_Cop1_D_Unk<false>,    R4300_Cop1_D_Unk<false>,     R4300_Cop1_D_Unk<false>,     R4300_Cop1_D_Unk<false>,     R4300_Cop1_D_Unk<false>,    R4300_Cop1_D_Unk<false>, 
    R4300_Cop1_D_CVT_S<false>,   R4300_Cop1_D_Unk<false>,     R4300_Cop1_D_Unk<false>,    R4300_Cop1_D_Unk<false>,     R4300_Cop1_D_CVT_W<false>,   R4300_Cop1_D_CVT_L<false>,   R4300_Cop1_D_Unk<false>,    R4300_Cop1_D_Unk<false>,
    R4300_Cop1_D_Unk<false>,     R4300_Cop1_D_Unk<false>,     R4300_Cop1_D_Unk<false>,    R4300_Cop1_D_Unk<false>,     R4300_Cop1_D_Unk<false>,     R4300_Cop1_D_Unk<false>,     R4300_Cop1_D_Unk<false>,    R4300_Cop1_D_Unk<false>, 
    R4300_Cop1_D_F<false>,       R4300_Cop1_D_UN<false>,      R4300_Cop1_D_EQ<false>,     R4300_Cop1_D_UEQ<false>,     R4300_Cop1_D_OLT<false>,     R4300_Cop1_D_ULT<false>,     R4300_Cop1_D_OLE<false>,    R4300_Cop1_D_ULE<false>,
    R4300_Cop1_D_SF<false>,      R4300_Cop1_D_NGLE<false>,    R4300_Cop1_D_SEQ<false>,    R4300_Cop1_D_NGL<false>,     R4300_Cop1_D_LT<false>,      R4300_Cop1_D_NGE<false>,     R4300_Cop1_D_LE<false>,     R4300_Cop1_D_NGT<false>,
};


void R4300_CALL_TYPE R4300_Special( R4300_CALL_SIGNATURE ) { R4300_CALL_MAKE_OP( op_code ); R4300SpecialInstruction[ op_code.spec_op ]( R4300_CALL_ARGUMENTS ); }
void R4300_CALL_TYPE R4300_RegImm( R4300_CALL_SIGNATURE )  { R4300_CALL_MAKE_OP( op_code ); R4300RegImmInstruction[ op_code.regimm_op ]( R4300_CALL_ARGUMENTS );     }
void R4300_CALL_TYPE R4300_CoPro0( R4300_CALL_SIGNATURE )  { R4300_CALL_MAKE_OP( op_code ); R4300Cop0Instruction[ op_code.cop0_op ]( R4300_CALL_ARGUMENTS );  }
void R4300_CALL_TYPE R4300_CoPro1( R4300_CALL_SIGNATURE )  { R4300_CALL_MAKE_OP( op_code ); R4300Cop1Instruction[ op_code.cop1_op ]( R4300_CALL_ARGUMENTS );  }
void R4300_CALL_TYPE R4300_Cop0_TLB( R4300_CALL_SIGNATURE ) { R4300_CALL_MAKE_OP( op_code ); R4300TLBInstruction[ op_code.cop0tlb_funct ]( R4300_CALL_ARGUMENTS ); }
void R4300_CALL_TYPE R4300_Cop1_BCInstr( R4300_CALL_SIGNATURE ) { R4300_CALL_MAKE_OP( op_code ); R4300Cop1BC1Instruction[ op_code.cop1_bc ]( R4300_CALL_ARGUMENTS ); }
void R4300_CALL_TYPE R4300_Cop1_SInstr( R4300_CALL_SIGNATURE )  { R4300_CALL_MAKE_OP( op_code ); R4300Cop1SInstruction[ op_code.cop1_funct ]( R4300_CALL_ARGUMENTS ); }
void R4300_CALL_TYPE R4300_Cop1_DInstr( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	if (gCPUState.CPUControl[C0_SR]._u32_0 & SR_FR)
	{
		R4300Cop1DInstruction_64[ op_code.cop1_funct ]( R4300_CALL_ARGUMENTS );
	}
	else
	{
		R4300Cop1DInstruction_32[ op_code.cop1_funct ]( R4300_CALL_ARGUMENTS );
	}
}

void R4300_CALL_TYPE R4300_Cop1_LInstr( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	switch ( op_code.cop1_funct )
	{
		case Cop1OpFunc_CVT_S:
			R4300_Cop1_L_CVT_S( R4300_CALL_ARGUMENTS );
			return;
		case Cop1OpFunc_CVT_D:
			R4300_Cop1_L_CVT_D( R4300_CALL_ARGUMENTS );
			return;
	}
}


void R4300_CALL_TYPE R4300_Cop1_WInstr( R4300_CALL_SIGNATURE )
{
	R4300_CALL_MAKE_OP( op_code );

	switch ( op_code.cop1_funct )
	{
		case Cop1OpFunc_CVT_S:
			R4300_Cop1_W_CVT_S( R4300_CALL_ARGUMENTS );
			return;
		case Cop1OpFunc_CVT_D:
			R4300_Cop1_W_CVT_D( R4300_CALL_ARGUMENTS );
			return;
	}
	WARN_NOEXIST("R4300_Cop1_WInstr_Unk");
}
