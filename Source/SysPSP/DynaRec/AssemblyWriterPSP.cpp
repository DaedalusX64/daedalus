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
#include "AssemblyWriterPSP.h"

#include "Core/Registers.h"

#include "OSHLE/ultra_R4300.h"

#include <limits.h>


//	Get the opcodes for loading a 32 bit constant into the specified
//	register. If this can be performed in a single op, the second opcode is NOP.
//	This is useful for splitting a constant load and using the branch delay slot.

void	CAssemblyWriterPSP::GetLoadConstantOps( EPspReg reg, s32 value, PspOpCode * p_op1, PspOpCode * p_op2 )
{
	PspOpCode	op1 {};
	PspOpCode	op2 {};

	if ( value >= SHRT_MIN && value <= SHRT_MAX )
	{
		// ORI
		op1.op = OP_ADDIU;
		op1.rt = reg;
		op1.rs = PspReg_R0;
		op1.immediate = u16( value );

		// NOP
		op2._u32 = 0;
	}
	else
	{
		// It's too large - we need to load in two parts
		op1.op = OP_LUI;
		op1.rt = reg;
		op1.rs = PspReg_R0;
		op1.immediate = u16( value >> 16 );

		if ( u16( value ) != 0 )
		{
			op2.op = OP_ORI;
			op2.rt = reg;
			op2.rs = reg;
			op2.immediate = u16( value );
		}
		else
		{
			// NOP
			op2._u32 = 0;
		}
	}

	*p_op1 = op1;
	*p_op2 = op2;
}


//

void	CAssemblyWriterPSP::LoadConstant( EPspReg reg, s32 value )
{
	PspOpCode	op1 {};
	PspOpCode	op2 {};

	GetLoadConstantOps( reg, value, &op1, &op2 );

	AppendOp( op1 );

	// There may not be a second op if the low bits are 0, or the constant is small
	if( op2._u32 != 0 )
	{
		AppendOp( op2 );
	}
}


//

void	CAssemblyWriterPSP::LoadRegister( EPspReg reg_dst, OpCodeValue load_op, EPspReg reg_base, s16 offset )
{
	PspOpCode		op_code {};
	op_code._u32 = 0;

	op_code.op = load_op;
	op_code.rt = reg_dst;
	op_code.base = reg_base;
	op_code.immediate = offset;

	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::StoreRegister( EPspReg reg_src, OpCodeValue store_op, EPspReg reg_base, s16 offset )
{
	PspOpCode		op_code {};
	op_code._u32 = 0;

	op_code.op = store_op;
	op_code.rt = reg_src;
	op_code.base = reg_base;
	op_code.immediate = offset;

	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::NOP()
{
	PspOpCode		op_code {};
	op_code._u32 = 0;
	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::SW( EPspReg reg_src, EPspReg reg_base, s16 offset )
{
	StoreRegister( reg_src, OP_SW, reg_base, offset );
}


//

void	CAssemblyWriterPSP::SH( EPspReg reg_src, EPspReg reg_base, s16 offset )
{
	StoreRegister( reg_src, OP_SH, reg_base, offset );
}


//

void	CAssemblyWriterPSP::SB( EPspReg reg_src, EPspReg reg_base, s16 offset )
{
	StoreRegister( reg_src, OP_SB, reg_base, offset );
}


//

void	CAssemblyWriterPSP::LB( EPspReg reg_dst, EPspReg reg_base, s16 offset )
{
	LoadRegister( reg_dst, OP_LB, reg_base, offset );
}


//

void	CAssemblyWriterPSP::LBU( EPspReg reg_dst, EPspReg reg_base, s16 offset )
{
	LoadRegister( reg_dst, OP_LBU, reg_base, offset );
}


//

void	CAssemblyWriterPSP::LH( EPspReg reg_dst, EPspReg reg_base, s16 offset )
{
	LoadRegister( reg_dst, OP_LH, reg_base, offset );
}


//

void	CAssemblyWriterPSP::LHU( EPspReg reg_dst, EPspReg reg_base, s16 offset )
{
	LoadRegister( reg_dst, OP_LHU, reg_base, offset );
}


//

void	CAssemblyWriterPSP::LW( EPspReg reg_dst, EPspReg reg_base, s16 offset )
{
	LoadRegister( reg_dst, OP_LW, reg_base, offset );
}


//

void	CAssemblyWriterPSP::LWC1( EPspFloatReg reg_dst, EPspReg reg_base, s16 offset )
{
	PspOpCode		op_code {};
	op_code._u32 = 0;

	op_code.op = OP_LWC1;
	op_code.ft = reg_dst;
	op_code.base = reg_base;
	op_code.immediate = offset;

	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::SWC1( EPspFloatReg reg_src, EPspReg reg_base, s16 offset )
{
	PspOpCode		op_code {};
	op_code._u32 = 0;

	op_code.op = OP_SWC1;
	op_code.ft = reg_src;
	op_code.base = reg_base;
	op_code.immediate = offset;
	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::LUI( EPspReg reg, u16 value )
{
	PspOpCode	op_code {};
	op_code.op = OP_LUI;
	op_code.rt = reg;
	op_code.rs = PspReg_R0;
	op_code.immediate = value;
	AppendOp( op_code );
}


//

CJumpLocation	CAssemblyWriterPSP::JAL( CCodeLabel target, bool insert_delay )
{
	CJumpLocation	jump_location( mpCurrentBuffer->GetJumpLocation() );

	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_JAL;
	op_code.target = target.GetTargetU32() >> 2;
	AppendOp( op_code );

	if(insert_delay)
	{
		// Stuff a nop in the delay slot
		op_code._u32 = 0;
		AppendOp( op_code );
	}

	return jump_location;
}


//

CJumpLocation	CAssemblyWriterPSP::J( CCodeLabel target, bool insert_delay )
{
	CJumpLocation	jump_location( mpCurrentBuffer->GetJumpLocation() );

	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_J;
	op_code.target = target.GetTargetU32() >> 2;
	AppendOp( op_code );

	if(insert_delay)
	{
		// Stuff a nop in the delay slot
		op_code._u32 = 0;
		AppendOp( op_code );
	}

	return jump_location;
}


//

void 	CAssemblyWriterPSP::JR( EPspReg reg_link, bool insert_delay )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_SPECOP;
	op_code.spec_op = SpecOp_JR;
	op_code.rs = reg_link;
	AppendOp( op_code );

	if(insert_delay)
	{
		// Stuff a nop in the delay slot
		op_code._u32 = 0;
		AppendOp( op_code );
	}
}


//

CJumpLocation	CAssemblyWriterPSP::BranchOp( EPspReg a, OpCodeValue op, EPspReg b, CCodeLabel target, bool insert_delay )
{
	CJumpLocation	branch_location( mpCurrentBuffer->GetJumpLocation() );
	s32				offset( branch_location.GetOffset( target ) );

	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = op;
	op_code.rs = a;
	op_code.rt = b;
	op_code.offset = s16((offset - 4) >> 2);	// Adjust for incremented PC and ignore lower bits
	AppendOp( op_code );

	if(insert_delay)
	{
		// Stuff a nop in the delay slot
		op_code._u32 = 0;
		AppendOp( op_code );
	}

	return branch_location;
}


//

CJumpLocation	CAssemblyWriterPSP::BNE( EPspReg a, EPspReg b, CCodeLabel target, bool insert_delay )
{
	return BranchOp( a, OP_BNE, b, target, insert_delay );
}


//

CJumpLocation	CAssemblyWriterPSP::BEQ( EPspReg a, EPspReg b, CCodeLabel target, bool insert_delay )
{
	return BranchOp( a, OP_BEQ, b, target, insert_delay );
}


//

CJumpLocation	CAssemblyWriterPSP::BNEL( EPspReg a, EPspReg b, CCodeLabel target, bool insert_delay )
{
	return BranchOp( a, OP_BNEL, b, target, insert_delay );
}


//

CJumpLocation	CAssemblyWriterPSP::BEQL( EPspReg a, EPspReg b, CCodeLabel target, bool insert_delay )
{
	return BranchOp( a, OP_BEQL, b, target, insert_delay );
}


//

CJumpLocation	CAssemblyWriterPSP::BLEZ( EPspReg a, CCodeLabel target, bool insert_delay )
{
	return BranchOp( a, OP_BLEZ, PspReg_R0, target, insert_delay );
}


//

CJumpLocation	CAssemblyWriterPSP::BGTZ( EPspReg a, CCodeLabel target, bool insert_delay )
{
	return BranchOp( a, OP_BGTZ, PspReg_R0, target, insert_delay );
}


//

CJumpLocation	CAssemblyWriterPSP::BranchRegImmOp( EPspReg a, ERegImmOp op, CCodeLabel target, bool insert_delay )
{
	CJumpLocation	branch_location( mpCurrentBuffer->GetJumpLocation() );
	s32				offset( branch_location.GetOffset( target ) );

	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_REGIMM;
	op_code.rs = a;
	op_code.regimm_op = op;
	op_code.offset = s16((offset - 4) >> 2);	// Adjust for incremented PC and ignore lower bits
	AppendOp( op_code );

	if(insert_delay)
	{
		// Stuff a nop in the delay slot
		op_code._u32 = 0;
		AppendOp( op_code );
	}

	return branch_location;
}


//

CJumpLocation	CAssemblyWriterPSP::BLTZ( EPspReg a, CCodeLabel target, bool insert_delay )
{
	return BranchRegImmOp( a, RegImmOp_BLTZ, target, insert_delay );
}


//

CJumpLocation	CAssemblyWriterPSP::BGEZ( EPspReg a, CCodeLabel target, bool insert_delay )
{
	return BranchRegImmOp( a, RegImmOp_BGEZ, target, insert_delay );
}


//

CJumpLocation	CAssemblyWriterPSP::BLTZL( EPspReg a, CCodeLabel target, bool insert_delay )
{
	return BranchRegImmOp( a, RegImmOp_BLTZL, target, insert_delay );
}


//

CJumpLocation	CAssemblyWriterPSP::BGEZL( EPspReg a, CCodeLabel target, bool insert_delay )
{
	return BranchRegImmOp( a, RegImmOp_BGEZL, target, insert_delay );
}



//

void	CAssemblyWriterPSP::ADDI( EPspReg reg_dst, EPspReg reg_src, s16 value )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_ADDI;
	op_code.rt = reg_dst;
	op_code.rs = reg_src;
	op_code.immediate = value;
	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::ADDIU( EPspReg reg_dst, EPspReg reg_src, s16 value )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_ADDIU;
	op_code.rt = reg_dst;
	op_code.rs = reg_src;
	op_code.immediate = value;
	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::SLTI( EPspReg reg_dst, EPspReg reg_src, s16 value )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_SLTI;
	op_code.rt = reg_dst;
	op_code.rs = reg_src;
	op_code.immediate = value;
	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::SLTIU( EPspReg reg_dst, EPspReg reg_src, s16 value )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_SLTIU;
	op_code.rt = reg_dst;
	op_code.rs = reg_src;
	op_code.immediate = value;
	AppendOp( op_code );
}


//EXTract bit field. 0 >= pos/size <= 31  //Corn

void	CAssemblyWriterPSP::EXT( EPspReg reg_dst, EPspReg reg_src, u32 size, u32 lsb )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = 0x1F;
	op_code.rt = reg_dst;
	op_code.rs = reg_src;
	op_code.rd = size;	// = MSB - LSB
	op_code.sa = lsb;	// = LSB
	op_code.spec_op = 0;
	AppendOp( op_code );
}


//INSert bit field. 0 >= pos/size <= 31 //Corn

void	CAssemblyWriterPSP::INS( EPspReg reg_dst, EPspReg reg_src, u32 msb, u32 lsb )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = 0x1F;
	op_code.rt = reg_dst;
	op_code.rs = reg_src;
	op_code.rd = msb;	// = MSB
	op_code.sa = lsb;	// = LSB
	op_code.spec_op = 4;
	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::ANDI( EPspReg reg_dst, EPspReg reg_src, u16 immediate )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_ANDI;
	op_code.rt = reg_dst;
	op_code.rs = reg_src;
	op_code.immediate = immediate;
	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::ORI( EPspReg reg_dst, EPspReg reg_src, u16 immediate )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_ORI;
	op_code.rt = reg_dst;
	op_code.rs = reg_src;
	op_code.immediate = immediate;
	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::XORI( EPspReg reg_dst, EPspReg reg_src, u16 immediate )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_XORI;
	op_code.rt = reg_dst;
	op_code.rs = reg_src;
	op_code.immediate = immediate;
	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::SLL( EPspReg reg_dst, EPspReg reg_src, u32 shift )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_SPECOP;
	op_code.rd = reg_dst;
	op_code.rt = reg_src;
	op_code.sa = shift;
	op_code.spec_op = SpecOp_SLL;
	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::SRL( EPspReg reg_dst, EPspReg reg_src, u32 shift )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_SPECOP;
	op_code.rd = reg_dst;
	op_code.rt = reg_src;
	op_code.sa = shift;
	op_code.spec_op = SpecOp_SRL;
	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::SRA( EPspReg reg_dst, EPspReg reg_src, u32 shift )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_SPECOP;
	op_code.rd = reg_dst;
	op_code.rt = reg_src;
	op_code.sa = shift;
	op_code.spec_op = SpecOp_SRA;
	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::SpecOpLogical( EPspReg rd, EPspReg rs, ESpecOp op, EPspReg rt )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_SPECOP;
	op_code.rd = rd;
	op_code.rs = rs;
	op_code.rt = rt;
	op_code.spec_op = op;
	AppendOp( op_code );
}


//RD = RT << RS

void	CAssemblyWriterPSP::SLLV( EPspReg rd, EPspReg rs, EPspReg rt )
{
	SpecOpLogical( rd, rs, SpecOp_SLLV, rt );
}


//RD = RT >> RS

void	CAssemblyWriterPSP::SRLV( EPspReg rd, EPspReg rs, EPspReg rt )
{
	SpecOpLogical( rd, rs, SpecOp_SRLV, rt );
}


//RD = RT >> RS

void	CAssemblyWriterPSP::SRAV( EPspReg rd, EPspReg rs, EPspReg rt )
{
	SpecOpLogical( rd, rs, SpecOp_SRAV, rt );
}


//

void	CAssemblyWriterPSP::MFLO( EPspReg rd )
{
	SpecOpLogical( rd, PspReg_R0, SpecOp_MFLO, PspReg_R0 );
}


//

void	CAssemblyWriterPSP::MFHI( EPspReg rd )
{
	SpecOpLogical( rd, PspReg_R0, SpecOp_MFHI, PspReg_R0 );
}


//

void	CAssemblyWriterPSP::MULT( EPspReg rs, EPspReg rt )
{
	SpecOpLogical( PspReg_R0, rs, SpecOp_MULT, rt );
}


//

void	CAssemblyWriterPSP::MULTU( EPspReg rs, EPspReg rt )
{
	SpecOpLogical( PspReg_R0, rs, SpecOp_MULTU, rt );
}


//

void	CAssemblyWriterPSP::DIV( EPspReg rs, EPspReg rt )
{
	SpecOpLogical( PspReg_R0, rs, SpecOp_DIV, rt );
}


//

void	CAssemblyWriterPSP::DIVU( EPspReg rs, EPspReg rt )
{
	SpecOpLogical( PspReg_R0, rs, SpecOp_DIVU, rt );
}


//

void	CAssemblyWriterPSP::ADD( EPspReg rd, EPspReg rs, EPspReg rt )
{
	SpecOpLogical( rd, rs, SpecOp_ADD, rt );
}


//

void	CAssemblyWriterPSP::ADDU( EPspReg rd, EPspReg rs, EPspReg rt )
{
	SpecOpLogical( rd, rs, SpecOp_ADDU, rt );
}


//

void	CAssemblyWriterPSP::SUB( EPspReg rd, EPspReg rs, EPspReg rt )
{
	SpecOpLogical( rd, rs, SpecOp_SUB, rt );
}


//

void	CAssemblyWriterPSP::SUBU( EPspReg rd, EPspReg rs, EPspReg rt )
{
	SpecOpLogical( rd, rs, SpecOp_SUBU, rt );
}


//

void	CAssemblyWriterPSP::AND( EPspReg rd, EPspReg rs, EPspReg rt )
{
	SpecOpLogical( rd, rs, SpecOp_AND, rt );
}


//

void	CAssemblyWriterPSP::OR( EPspReg rd, EPspReg rs, EPspReg rt )
{
	SpecOpLogical( rd, rs, SpecOp_OR, rt );
}


//

void	CAssemblyWriterPSP::XOR( EPspReg rd, EPspReg rs, EPspReg rt )
{
	SpecOpLogical( rd, rs, SpecOp_XOR, rt );
}


//

void	CAssemblyWriterPSP::NOR( EPspReg rd, EPspReg rs, EPspReg rt )
{
	SpecOpLogical( rd, rs, SpecOp_NOR, rt );
}


//

void	CAssemblyWriterPSP::SLT( EPspReg rd, EPspReg rs, EPspReg rt )
{
	SpecOpLogical( rd, rs, SpecOp_SLT, rt );
}


//

void	CAssemblyWriterPSP::SLTU( EPspReg rd, EPspReg rs, EPspReg rt )
{
	SpecOpLogical( rd, rs, SpecOp_SLTU, rt );
}


//

void	CAssemblyWriterPSP::Cop1Op( ECop1Op cop1_op, EPspFloatReg fd, EPspFloatReg fs, ECop1OpFunction cop1_funct, EPspFloatReg ft )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_COPRO1;
	op_code.fd = fd;
	op_code.fs = fs;
	op_code.ft = ft;
	op_code.cop1_op = cop1_op;
	op_code.cop1_funct = cop1_funct;
	AppendOp( op_code );
}


//	Unary

void	CAssemblyWriterPSP::Cop1Op( ECop1Op cop1_op, EPspFloatReg fd, EPspFloatReg fs, ECop1OpFunction cop1_funct )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_COPRO1;
	op_code.fd = fd;
	op_code.fs = fs;
	op_code.cop1_op = cop1_op;
	op_code.cop1_funct = cop1_funct;
	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::CFC1( EPspReg rt, EPspFloatReg fs )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_COPRO1;
	op_code.rt = rt;
	op_code.fs = fs;
	op_code.cop1_op = Cop1Op_CFC1;
	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::MFC1( EPspReg rt, EPspFloatReg fs )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_COPRO1;
	op_code.rt = rt;
	op_code.fs = fs;
	op_code.cop1_op = Cop1Op_MFC1;
	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::MTC1( EPspFloatReg fs, EPspReg rt )
{
	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_COPRO1;
	op_code.rt = rt;
	op_code.fs = fs;
	op_code.cop1_op = Cop1Op_MTC1;
	AppendOp( op_code );
}


//

void	CAssemblyWriterPSP::ADD_S( EPspFloatReg fd, EPspFloatReg fs, EPspFloatReg ft )
{
	Cop1Op( Cop1Op_SInstr, fd, fs, Cop1OpFunc_ADD, ft );
}


//

void	CAssemblyWriterPSP::SUB_S( EPspFloatReg fd, EPspFloatReg fs, EPspFloatReg ft )
{
	Cop1Op( Cop1Op_SInstr, fd, fs, Cop1OpFunc_SUB, ft );
}


//

void	CAssemblyWriterPSP::MUL_S( EPspFloatReg fd, EPspFloatReg fs, EPspFloatReg ft )
{
	Cop1Op( Cop1Op_SInstr, fd, fs, Cop1OpFunc_MUL, ft );
}


//

void	CAssemblyWriterPSP::DIV_S( EPspFloatReg fd, EPspFloatReg fs, EPspFloatReg ft )
{
	Cop1Op( Cop1Op_SInstr, fd, fs, Cop1OpFunc_DIV, ft );
}


//

void	CAssemblyWriterPSP::SQRT_S( EPspFloatReg fd, EPspFloatReg fs )
{
	Cop1Op( Cop1Op_SInstr, fd, fs, Cop1OpFunc_SQRT );
}


//

void	CAssemblyWriterPSP::ABS_S( EPspFloatReg fd, EPspFloatReg fs )
{
	Cop1Op( Cop1Op_SInstr, fd, fs, Cop1OpFunc_ABS );
}


//

void	CAssemblyWriterPSP::MOV_S( EPspFloatReg fd, EPspFloatReg fs )
{
	Cop1Op( Cop1Op_SInstr, fd, fs, Cop1OpFunc_MOV );
}


//

void	CAssemblyWriterPSP::NEG_S( EPspFloatReg fd, EPspFloatReg fs )
{
	Cop1Op( Cop1Op_SInstr, fd, fs, Cop1OpFunc_NEG );
}


//

void	CAssemblyWriterPSP::TRUNC_W_S( EPspFloatReg fd, EPspFloatReg fs )
{
	Cop1Op( Cop1Op_SInstr, fd, fs, Cop1OpFunc_TRUNC_W );
}


//

void	CAssemblyWriterPSP::FLOOR_W_S( EPspFloatReg fd, EPspFloatReg fs )
{
	Cop1Op( Cop1Op_SInstr, fd, fs, Cop1OpFunc_FLOOR_W );
}

//

void	CAssemblyWriterPSP::CVT_W_S( EPspFloatReg fd, EPspFloatReg fs )
{
	Cop1Op( Cop1Op_SInstr, fd, fs, Cop1OpFunc_CVT_W );
}


//

void	CAssemblyWriterPSP::CMP_S( EPspFloatReg fs, ECop1OpFunction	cmp_op, EPspFloatReg ft )
{
	Cop1Op( Cop1Op_SInstr, PspFloatReg_F00, fs, cmp_op, ft );
}


//

void	CAssemblyWriterPSP::CVT_S_W( EPspFloatReg fd, EPspFloatReg fs )
{
	Cop1Op( Cop1Op_WInstr, fd, fs, Cop1OpFunc_CVT_S );
}


//

CJumpLocation	CAssemblyWriterPSP::BranchCop1( ECop1BCOp bc_op, CCodeLabel target, bool insert_delay )
{
	CJumpLocation	branch_location( mpCurrentBuffer->GetJumpLocation() );
	s32				offset( branch_location.GetOffset( target ) );

	PspOpCode	op_code {};
	op_code._u32 = 0;
	op_code.op = OP_COPRO1;
	op_code.cop1_op = Cop1Op_BCInstr;
	op_code.cop1_bc = bc_op;
	op_code.offset = s16((offset - 4) >> 2);	// Adjust for incremented PC and ignore lower bits
	AppendOp( op_code );

	if(insert_delay)
	{
		// Stuff a nop in the delay slot
		op_code._u32 = 0;
		AppendOp( op_code );
	}

	return branch_location;
}


//

CJumpLocation	CAssemblyWriterPSP::BC1F( CCodeLabel target, bool insert_delay )
{
	return BranchCop1( Cop1BCOp_BC1F, target, insert_delay );
}


//

CJumpLocation	CAssemblyWriterPSP::BC1T( CCodeLabel target, bool insert_delay )
{
	return BranchCop1( Cop1BCOp_BC1T, target, insert_delay );
}
