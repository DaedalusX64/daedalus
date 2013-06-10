/*
Copyright (C) 2001,2005 StrmnNrmn

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

#pragma once

#ifndef SYSW32_DYNAREC_X86_ASSEMBLYWRITERX86_H_
#define SYSW32_DYNAREC_X86_ASSEMBLYWRITERX86_H_

#include "DynaRec/AssemblyBuffer.h"
#include "DynarecTargetX86.h"

class CAssemblyWriterX86
{
	public:
		CAssemblyWriterX86( CAssemblyBuffer * p_buffer )
			:	mpAssemblyBuffer( p_buffer )
		{
		}

	public:
		CAssemblyBuffer *	GetAssemblyBuffer() const									{ return mpAssemblyBuffer; }
		void				SetAssemblyBuffer( CAssemblyBuffer * p_buffer )				{ mpAssemblyBuffer = p_buffer; }

	// XXXX
	private:
	public:
				// I sometimes use this to force an exceptions so I can analyse the generated code
				inline void CRASH(u32 x)
				{
					EmitWORD(0x05c7);
					EmitDWORD(0x00000000);
					EmitDWORD(x); //c7 05 xx+0 xx xx xx ef be ad de
				}

				inline void NOP()
				{
					EmitBYTE(0x90);
				}

				inline void INT3()
				{
					EmitBYTE(0xcc);
				}

				void				PUSH(EIntelReg reg);
				void				POP(EIntelReg reg);
				void				PUSHI( u32 value );

				void				ADD(EIntelReg reg1, EIntelReg reg2);				// add	reg1, reg2
				void				SUB(EIntelReg reg1, EIntelReg reg2);
				void				MUL_EAX_MEM(void * mem);							// mul	eax, dword ptr[ mem ]
				void				MUL(EIntelReg reg);									// mul	eax, reg

				void				ADD_REG_MEM_IDXx4( EIntelReg destreg, void * mem, EIntelReg idxreg );	// add reg, dword ptr[ mem + idx*4 ]

				void				AND(EIntelReg reg1, EIntelReg reg2);
				void				AND_EAX( u32 mask );								// and		eax, 0x00FF	- Mask off top bits!

				void				OR(EIntelReg reg1, EIntelReg reg2);
				void				XOR(EIntelReg reg1, EIntelReg reg2);
				void				NOT(EIntelReg reg1);

				void				ADDI(EIntelReg reg, s32 data);
				void				ADCI(EIntelReg reg, s32 data);
				void				ANDI(EIntelReg reg, u32 data);
				void				ORI(EIntelReg reg, u32 data);
				void				XOR_I32(EIntelReg reg, u32 data);
				void				XOR_I8(EIntelReg reg, u8 data);

				void				SHLI(EIntelReg reg, u8 sa);
				void				SHRI(EIntelReg reg, u8 sa);
				void				SARI(EIntelReg reg, u8 sa);

				void				CMP(EIntelReg reg1, EIntelReg reg2);
				void				TEST(EIntelReg reg1, EIntelReg reg2);
				void				TEST_AH( u8 flags );
				void				CMPI(EIntelReg reg, u32 data);
				void				CMP_MEM32_I32(const void *p_mem, u32 data);			// cmp		dword ptr p_mem, data
				void				CMP_MEM32_I8(const void *p_mem, u8 data);			// cmp		dword ptr p_mem, data

				void				SETL(EIntelReg reg);
				void				SETB(EIntelReg reg);

				void				JS(s32 off);
				void				JA(u8 off);
				void				JB(u8 off);
				void				JAE(u8 off);
				void				JE(u8 off);
				void				JNE(u8 off);
				void				JL(u8 off);
				void				JGE(u8 off);
				void				JLE(u8 off);
				void				JG(u8 off);
				void				JMP_SHORT(u8 off);

				CJumpLocation		JMPLong( CCodeLabel target );
				CJumpLocation		JNELong( CCodeLabel target );
				CJumpLocation		JELong( CCodeLabel target );

				void				JMP_REG( EIntelReg reg );
				void				CALL_REG( EIntelReg reg );
				CJumpLocation		CALL( CCodeLabel target );

				void				CALL_MEM_PLUS_REGx4( void * mem, EIntelReg reg );	// call dword ptr [mem + reg*4]
				void				RET();

				static inline bool IsValidMov8Reg( EIntelReg isrc )
				{
					return isrc <= EBX_CODE;
				}

				void				CDQ();

				void				MOV(EIntelReg reg1, EIntelReg reg2);				// mov  reg1, reg2
				void				MOVSX(EIntelReg reg1, EIntelReg reg2, bool _8bit);	// movsx reg1, reg2
				void				MOVZX(EIntelReg reg1, EIntelReg reg2, bool _8bit);	// movzx reg1, reg2
				void				MOV_MEM_REG(void * mem, EIntelReg isrc);			// mov dword ptr[ mem ], reg
				void				MOV_REG_MEM(EIntelReg reg, const void * mem);		// mov reg, dword ptr[ mem ]

				void				MOV8_MEM_BASE_REG( EIntelReg ibase, EIntelReg isrc );		// mov byte ptr [base], src
				void				MOV8_REG_MEM_BASE( EIntelReg idst, EIntelReg ibase );		// mov dst, byte ptr [base]

				void				MOV16_MEM_BASE_REG( EIntelReg ibase, EIntelReg isrc );		// mov word ptr [base], src
				void				MOV16_REG_MEM_BASE( EIntelReg idst, EIntelReg ibase );		// mov dst, word ptr[base] e.g. mov ax, word ptr [eax]

				void				MOV_MEM_BASE_OFFSET32_REG( EIntelReg ibase, s32 offset, EIntelReg isrc );		// mov dword ptr [base + nnnnnnnn], src
				void				MOV_MEM_BASE_OFFSET8_REG( EIntelReg ibase, s8 offset, EIntelReg isrc );		// mov dword ptr [base + nn], src
				void				MOV_MEM_BASE_OFFSET_REG( EIntelReg ibase, s32 offset, EIntelReg isrc );
				void				MOV_MEM_BASE_REG( EIntelReg ibase, EIntelReg isrc );							// mov dword ptr [base], src

				void				MOV_REG_MEM_BASE_OFFSET32( EIntelReg idst, EIntelReg ibase, s32 offset );		// mov dst, dword ptr [base + nnnnnnnn]
				void				MOV_REG_MEM_BASE_OFFSET8( EIntelReg idst, EIntelReg ibase, s8 offset );		// mov dst, dword ptr [base + nn]
				void				MOV_REG_MEM_BASE_OFFSET( EIntelReg idst, EIntelReg ibase, s32 offset );
				void				MOV_REG_MEM_BASE( EIntelReg idst, EIntelReg ibase );							// mov dst, dword ptr [base]

				void				MOVI(EIntelReg reg, u32 data);						// mov reg, data
				void				MOVI_MEM(void * mem, u32 data);						// mov dword ptr[ mem ], data
				void				MOVI_MEM8(void * mem, u8 data);						// mov byte ptr[ mem ], data

				void				MOVSX8( EIntelReg idst, EIntelReg isrc );			// movsx	dst, src		(e.g. movsx eax, al)
				void				MOVSX16( EIntelReg idst, EIntelReg isrc );			// movsx	dst, src		(e.g. movsx eax, ax)

				void				FSQRT();
				void				FCHS();

				void				FILD_MEM( void * pmem );
				void				FLD_MEMp32( void * pmem );
				void				FSTP_MEMp32( void * pmem );
				void				FLD_MEMp64( void * memlo, void * memhi );
				void				FSTP_MEMp64( void * memlo, void * memhi );
				void				FISTP_MEMp( void * pmem );

				void				FLD( u32 i );
				void				FXCH( u32 i );
				void				FCOMP( u32 i );

				void				FNSTSW();
				void				FFREE( u32 i );
				void				FINCSTP();
				void				FSQRTp();

				void				FCHSp();
				void				FADD( u32 i );
				void				FSUB( u32 i );
				void				FMUL( u32 i );
				void				FDIV( u32 i );

	private:
				CJumpLocation		JumpConditionalLong( CCodeLabel target, u8 jump_type );


		inline void EmitBYTE(u8 byte)
		{
			mpAssemblyBuffer->EmitBYTE( byte );
		}

		inline void EmitWORD(u16 word)
		{
			mpAssemblyBuffer->EmitWORD( word );
		}

		inline void EmitDWORD(u32 dword)
		{
			mpAssemblyBuffer->EmitDWORD( dword );
		}

	private:
		CAssemblyBuffer *				mpAssemblyBuffer;
};

#endif // SYSW32_DYNAREC_X86_ASSEMBLYWRITERX86_H_
