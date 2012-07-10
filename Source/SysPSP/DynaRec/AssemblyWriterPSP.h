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

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef ASSEMBLYWRITERPSP_H_
#define ASSEMBLYWRITERPSP_H_

#include "DynaRec/AssemblyBuffer.h"
#include "DynarecTargetPSP.h"

#include "Core/R4300OpCode.h"

class CAssemblyWriterPSP
{
	public:
		CAssemblyWriterPSP( CAssemblyBuffer * p_buffer_a, CAssemblyBuffer * p_buffer_b )
			:	mpCurrentBuffer( p_buffer_a )
			,	mpAssemblyBufferA( p_buffer_a )
			,	mpAssemblyBufferB( p_buffer_b )
		{
		}

	public:
		CAssemblyBuffer *	GetAssemblyBuffer() const		{ return mpCurrentBuffer; }
		void				Finalise()
		{
			mpCurrentBuffer = NULL;
			mpAssemblyBufferA = NULL;
			mpAssemblyBufferB = NULL;
		}

		void		SetBufferA()
		{
			mpCurrentBuffer = mpAssemblyBufferA;
		}
		void		SetBufferB()
		{
			mpCurrentBuffer = mpAssemblyBufferB;
		}
		bool		IsBufferA() const
		{
			return mpCurrentBuffer == mpAssemblyBufferA;
		}

	// XXXX
	private:
	public:
		void				LoadConstant( EPspReg reg, s32 value );

		void				LoadRegister( EPspReg reg_dst, OpCodeValue load_op, EPspReg reg_base, s16 offset );
		void				StoreRegister( EPspReg reg_src, OpCodeValue store_op, EPspReg reg_base, s16 offset );

		void				NOP();
		void				LUI( EPspReg reg, u16 value );

		void				SW( EPspReg reg_src, EPspReg reg_base, s16 offset );
		void				SH( EPspReg reg_src, EPspReg reg_base, s16 offset );
		void				SB( EPspReg reg_src, EPspReg reg_base, s16 offset );

		void				LB( EPspReg reg_dst, EPspReg reg_base, s16 offset );
		void				LBU( EPspReg reg_dst, EPspReg reg_base, s16 offset );
		void				LH( EPspReg reg_dst, EPspReg reg_base, s16 offset );
		void				LHU( EPspReg reg_dst, EPspReg reg_base, s16 offset );
		void				LW( EPspReg reg_dst, EPspReg reg_base, s16 offset );

		void				LWC1( EPspFloatReg reg_dst, EPspReg reg_base, s16 offset );
		void				SWC1( EPspFloatReg reg_src, EPspReg reg_base, s16 offset );

		CJumpLocation		JAL( CCodeLabel target, bool insert_delay );
		CJumpLocation		J( CCodeLabel target, bool insert_delay );

		void 				JR( EPspReg reg_link, bool insert_delay );

		CJumpLocation		BNE( EPspReg a, EPspReg b, CCodeLabel target, bool insert_delay );
		CJumpLocation		BEQ( EPspReg a, EPspReg b, CCodeLabel target, bool insert_delay );
		CJumpLocation		BNEL( EPspReg a, EPspReg b, CCodeLabel target, bool insert_delay );
		CJumpLocation		BEQL( EPspReg a, EPspReg b, CCodeLabel target, bool insert_delay );
		CJumpLocation		BLEZ( EPspReg a, CCodeLabel target, bool insert_delay );
		CJumpLocation		BGTZ( EPspReg a, CCodeLabel target, bool insert_delay );

		CJumpLocation		BLTZ( EPspReg a, CCodeLabel target, bool insert_delay );
		CJumpLocation		BGEZ( EPspReg a, CCodeLabel target, bool insert_delay );
		CJumpLocation		BLTZL( EPspReg a, CCodeLabel target, bool insert_delay );
		CJumpLocation		BGEZL( EPspReg a, CCodeLabel target, bool insert_delay );

		void				EXT( EPspReg reg_dst, EPspReg reg_src, u32 size, u32 lsb );
		void				INS( EPspReg reg_dst, EPspReg reg_src, u32 msb, u32 lsb );
		void				ADDI( EPspReg reg_dst, EPspReg reg_src, s16 value );
		void				ADDIU( EPspReg reg_dst, EPspReg reg_src, s16 value );
		void				SLTI( EPspReg reg_dst, EPspReg reg_src, s16 value );
		void				SLTIU( EPspReg reg_dst, EPspReg reg_src, s16 value );

		void				ANDI( EPspReg reg_dst, EPspReg reg_src, u16 immediate );
		void				ORI( EPspReg reg_dst, EPspReg reg_src, u16 immediate );
		void				XORI( EPspReg reg_dst, EPspReg reg_src, u16 immediate );

		void				SLL( EPspReg reg_dst, EPspReg reg_src, u32 shift );
		void				SRL( EPspReg reg_dst, EPspReg reg_src, u32 shift );
		void				SRA( EPspReg reg_dst, EPspReg reg_src, u32 shift );
		void				SLLV( EPspReg rd, EPspReg rs, EPspReg rt );
		void				SRLV( EPspReg rd, EPspReg rs, EPspReg rt );
		void				SRAV( EPspReg rd, EPspReg rs, EPspReg rt );

		void				MFLO( EPspReg rd );
		void				MFHI( EPspReg rd );
		void				MULT( EPspReg rs, EPspReg rt );
		void				MULTU( EPspReg rs, EPspReg rt );
		void				DIV( EPspReg rs, EPspReg rt );
		void				DIVU( EPspReg rs, EPspReg rt );

		void				ADD( EPspReg rd, EPspReg rs, EPspReg rt );
		void				ADDU( EPspReg rd, EPspReg rs, EPspReg rt );
		void				SUB( EPspReg rd, EPspReg rs, EPspReg rt );
		void				SUBU( EPspReg rd, EPspReg rs, EPspReg rt );
		void				AND( EPspReg rd, EPspReg rs, EPspReg rt );
		void				OR( EPspReg rd, EPspReg rs, EPspReg rt );
		void				XOR( EPspReg rd, EPspReg rs, EPspReg rt );
		void				NOR( EPspReg rd, EPspReg rs, EPspReg rt );

		void				SLT( EPspReg rd, EPspReg rs, EPspReg rt );
		void				SLTU( EPspReg rd, EPspReg rs, EPspReg rt );

		void				Cop1Op( ECop1Op cop1_op, EPspFloatReg fd, EPspFloatReg fs, ECop1OpFunction cop1_funct, EPspFloatReg ft );
		void				Cop1Op( ECop1Op cop1_op, EPspFloatReg fd, EPspFloatReg fs, ECop1OpFunction cop1_funct );

		void				CFC1( EPspReg rt, EPspFloatReg fs );

		void				MFC1( EPspReg rt, EPspFloatReg fs );
		void				MTC1( EPspFloatReg fs, EPspReg rt );

		void				ADD_S( EPspFloatReg fd, EPspFloatReg fs, EPspFloatReg ft );
		void				SUB_S( EPspFloatReg fd, EPspFloatReg fs, EPspFloatReg ft );
		void				MUL_S( EPspFloatReg fd, EPspFloatReg fs, EPspFloatReg ft );
		void				DIV_S( EPspFloatReg fd, EPspFloatReg fs, EPspFloatReg ft );

		void				SQRT_S( EPspFloatReg fd, EPspFloatReg fs );
		void				ABS_S( EPspFloatReg fd, EPspFloatReg fs );
		void				MOV_S( EPspFloatReg fd, EPspFloatReg fs );
		void				NEG_S( EPspFloatReg fd, EPspFloatReg fs );

		void				TRUNC_W_S( EPspFloatReg fd, EPspFloatReg fs );
		void				CVT_W_S( EPspFloatReg fd, EPspFloatReg fs );

		void				CMP_S( EPspFloatReg fs, ECop1OpFunction	cmp_op, EPspFloatReg ft );

		void				CVT_S_W( EPspFloatReg fd, EPspFloatReg fs );

		CJumpLocation		BranchCop1( ECop1BCOp bc_op, CCodeLabel target, bool insert_delay );
		CJumpLocation		BC1F( CCodeLabel target, bool insert_delay );
		CJumpLocation		BC1T( CCodeLabel target, bool insert_delay );

		static	void		GetLoadConstantOps( EPspReg reg, s32 value, PspOpCode * p_op1, PspOpCode * p_op2 );

		inline void AppendOp( PspOpCode op )
		{
			mpCurrentBuffer->EmitDWORD( op._u32 );
		}

	private:
		CJumpLocation		BranchOp( EPspReg a, OpCodeValue op, EPspReg b, CCodeLabel target, bool insert_delay );
		CJumpLocation		BranchRegImmOp( EPspReg a, ERegImmOp op, CCodeLabel target, bool insert_delay );
		void				SpecOpLogical( EPspReg rd, EPspReg rs, ESpecOp op, EPspReg rt );

	private:
		CAssemblyBuffer *				mpCurrentBuffer;
		CAssemblyBuffer *				mpAssemblyBufferA;
		CAssemblyBuffer *				mpAssemblyBufferB;
};

#endif // ASSEMBLYWRITERPSP_H_
