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

#include "stdafx.h"
#include "AssemblyWriterX86.h"

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::PUSH(EIntelReg reg)
{
	EmitBYTE(0x50 | reg);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::POP(EIntelReg reg)
{
	EmitBYTE(0x58 | reg);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::PUSHI( u32 value )
{
	EmitBYTE( 0x68 );
	EmitDWORD( value );
}

//*****************************************************************************
//	add	reg1, reg2
//*****************************************************************************
void	CAssemblyWriterX86::ADD(EIntelReg reg1, EIntelReg reg2)
{
	EmitBYTE(0x03);
	EmitBYTE(0xc0 | (reg1<<3) | reg2);
}

//*****************************************************************************
//	sub	reg1, reg2
//*****************************************************************************
void	CAssemblyWriterX86::SUB(EIntelReg reg1, EIntelReg reg2)
{
	EmitBYTE(0x2b);
	EmitBYTE(0xc0 | (reg1<<3) | reg2);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::MUL_EAX_MEM(void * mem)
{
	EmitBYTE(0xf7);
	EmitBYTE(0x25);
	EmitDWORD((u32)mem);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::MUL(EIntelReg reg)
{
	EmitBYTE(0xf7);
	EmitBYTE(0xe0 | reg);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::ADD_REG_MEM_IDXx4( EIntelReg destreg, void * mem, EIntelReg idxreg )
{
	EmitBYTE(0x03);
	EmitBYTE(0x04 | (destreg << 3));
	EmitBYTE(0x85 | (idxreg<<3));
	EmitDWORD((u32)mem);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::AND(EIntelReg reg1, EIntelReg reg2)
{
	if (reg1 != reg2)
	{
		EmitBYTE(0x23);
		EmitBYTE(0xc0 | (reg1<<3) | reg2);
	}
}

//*****************************************************************************
// and		eax, 0x00FF	- Mask off top bits!
// This is slightly more efficient than ANDI( EAX_CODE, data )
//*****************************************************************************
void	CAssemblyWriterX86::AND_EAX( u32 mask )
{
	EmitBYTE(0x25);
	EmitDWORD(mask);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::OR(EIntelReg reg1, EIntelReg reg2)
{
	if (reg1 != reg2)
	{
		EmitBYTE(0x0B);
		EmitBYTE(0xc0 | (reg1<<3) | reg2);
	}
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::XOR(EIntelReg reg1, EIntelReg reg2)
{
	EmitBYTE(0x33);
	EmitBYTE(0xc0 | (reg1<<3) | reg2);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::NOT(EIntelReg reg1)
{
	EmitBYTE(0xf7);
	EmitBYTE(0xd0 | reg1);
}

//*****************************************************************************
// Use short form (0x83c0) if data is just one byte!
//*****************************************************************************
void CAssemblyWriterX86::ADDI(EIntelReg reg, s32 data)
{
	if (data == 0)
		return;

	if (data <= 127 && data > -127)
	{
		EmitBYTE(0x83);
		EmitBYTE(0xc0 | reg);
		EmitBYTE((u8)data);
	}
	else
	{
		EmitBYTE(0x81);
		EmitBYTE(0xc0 | reg);
		EmitDWORD(data);
	}
	
}

//*****************************************************************************
// Use short form (0x83d0) if data is just one byte!
//*****************************************************************************
void CAssemblyWriterX86::ADCI(EIntelReg reg, s32 data)
{
	/*if (reg == EAX_CODE)
	{
		EmitBYTE(0x15);
		EmitDWORD(data)
	}
	else*/
	if (data <= 127 && data > -127)
	{
		EmitBYTE(0x83);
		EmitBYTE(0xd0 | reg);
		EmitBYTE((u8)(data));
	}
	else
	{
		EmitBYTE(0x81);
		EmitBYTE(0xd0 | reg);
		EmitDWORD(data);
	}
}

//*****************************************************************************
//
//*****************************************************************************
void CAssemblyWriterX86::ANDI(EIntelReg reg, u32 data)
{
	/*if (reg == EAX_CODE)
		EmitBYTE(0x25);
	else */
	{
		EmitBYTE(0x81);
		EmitBYTE(0xe0 | reg);
	}
	EmitDWORD(data);
}

//*****************************************************************************
//
//*****************************************************************************
void CAssemblyWriterX86::ORI(EIntelReg reg, u32 data)
{
	/*if (reg == EAX_CODE)
		EmitBYTE(0x0D);
	else*/
	{
		EmitBYTE(0x81);
		EmitBYTE(0xc8 | reg);
	}
	EmitDWORD(data);
}

//*****************************************************************************
//
//*****************************************************************************
void CAssemblyWriterX86::XOR_I32(EIntelReg reg, u32 data)
{

	/*if (reg == EAX_CODE)
		EmitBYTE(0x35);
	else */
	{
		EmitBYTE(0x81);
		EmitBYTE(0xf0 | reg);
	}
	EmitDWORD(data);
}

//*****************************************************************************
//
//*****************************************************************************
void CAssemblyWriterX86::XOR_I8(EIntelReg reg, u8 data)
{
	EmitBYTE(0x83);
	EmitBYTE(0xf0 | reg);
	EmitBYTE(data);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::SHLI(EIntelReg reg, u8 sa)
{
	EmitBYTE(0xc1);
	EmitBYTE(0xe0 | reg);
	EmitBYTE(sa);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::SHRI(EIntelReg reg, u8 sa)
{
	EmitBYTE(0xc1);
	EmitBYTE(0xe8 | reg);
	EmitBYTE(sa);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::SARI(EIntelReg reg, u8 sa)
{
	EmitBYTE(0xc1);
	EmitBYTE(0xf8 | reg);
	EmitBYTE(sa);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::CMP(EIntelReg reg1, EIntelReg reg2)
{
	EmitBYTE(0x3b);
	EmitBYTE(0xc0 | (reg1<<3) | reg2);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::TEST(EIntelReg reg1, EIntelReg reg2)
{
	EmitBYTE(0x85);
	EmitBYTE(0xc0 | (reg1<<3) | reg2);
}

//*****************************************************************************
// test		ah, 41
//*****************************************************************************
void CAssemblyWriterX86::TEST_AH( u8 flags )
{
	EmitBYTE(0xf6);
	EmitBYTE(0xc4);
	EmitBYTE(flags);
}

//*****************************************************************************
//
//*****************************************************************************
void CAssemblyWriterX86::CMPI(EIntelReg reg, u32 data)
{
	EmitBYTE(0x81);
	EmitBYTE(0xf8 | reg);
	EmitDWORD(data);
}

//*****************************************************************************
//	cmp		dword ptr p_mem, data
//*****************************************************************************
void CAssemblyWriterX86::CMP_MEM32_I32(const void *p_mem, u32 data)
{
	EmitBYTE(0x81);
	EmitBYTE(0x3d);
	EmitDWORD(reinterpret_cast< u32 >( p_mem ));
	EmitDWORD(data);
}

//*****************************************************************************
//	cmp		dword ptr p_mem, data
//*****************************************************************************
void CAssemblyWriterX86::CMP_MEM32_I8(const void *p_mem, u8 data)
{
	EmitBYTE(0x83);
	EmitBYTE(0x3d);
	EmitDWORD(reinterpret_cast< u32 >( p_mem ));
	EmitBYTE(data);
}

//*****************************************************************************
//
//*****************************************************************************
void CAssemblyWriterX86::SETL(EIntelReg reg)
{
	EmitWORD(0x9c0f);
	EmitBYTE(0xc0 | reg);
}

//*****************************************************************************
//
//*****************************************************************************
void CAssemblyWriterX86::SETB(EIntelReg reg)
{
	EmitWORD(0x920f);
	EmitBYTE(0xc0 | reg);
}

//*****************************************************************************
//
//*****************************************************************************
void CAssemblyWriterX86::JS(s32 off)
{
	EmitBYTE(0x0f);
	EmitBYTE(0x88);
	EmitDWORD( u32(off) );
}

//*****************************************************************************
//
//*****************************************************************************
void CAssemblyWriterX86::JA(u8 off)
{
	EmitWORD((off<<8) | 0x77);
}

//*****************************************************************************
//
//*****************************************************************************
void CAssemblyWriterX86::JB(u8 off)
{
	EmitWORD((off<<8) | 0x72);
}

//*****************************************************************************
//
//*****************************************************************************
void CAssemblyWriterX86::JAE(u8 off)
{
	EmitWORD((off<<8) | 0x73);
}

//*****************************************************************************
//
//*****************************************************************************
void CAssemblyWriterX86::JE(u8 off)
{
	EmitWORD((off<<8) | 0x74);
}

//*****************************************************************************
//
//*****************************************************************************
void CAssemblyWriterX86::JNE(u8 off)
{
	EmitWORD((off<<8) | 0x75);
}

//*****************************************************************************
//
//*****************************************************************************
void CAssemblyWriterX86::JL(u8 off)
{
	EmitWORD((off<<8) | 0x7C);
}

//*****************************************************************************
//
//*****************************************************************************
void CAssemblyWriterX86::JGE(u8 off)
{
	EmitWORD((off<<8) | 0x7D);
}

//*****************************************************************************
//
//*****************************************************************************
void CAssemblyWriterX86::JLE(u8 off)
{
	EmitWORD((off<<8) | 0x7E);
}

//*****************************************************************************
//
//*****************************************************************************
void CAssemblyWriterX86::JG(u8 off)
{
	EmitWORD((off<<8) | 0x7F);
}

//*****************************************************************************
//
//*****************************************************************************
void CAssemblyWriterX86::JMP_SHORT(u8 off)
{
	EmitWORD((off<<8) | 0xeb);
}

//*****************************************************************************
//
//*****************************************************************************
CJumpLocation CAssemblyWriterX86::JumpConditionalLong( CCodeLabel target, u8 jump_type )
{
	const u32	JUMP_LONG_LENGTH = 6;

	CJumpLocation	jump_location( mpAssemblyBuffer->GetJumpLocation() );
	s32				offset( jump_location.GetOffset( target ) - JUMP_LONG_LENGTH );

	EmitBYTE( 0x0f );
	EmitBYTE( jump_type );		//
	EmitDWORD( offset );

	return jump_location;
}

//*****************************************************************************
//
//*****************************************************************************
CJumpLocation CAssemblyWriterX86::JELong( CCodeLabel target )
{
	return JumpConditionalLong( target, 0x84 );
}

//*****************************************************************************
//
//*****************************************************************************
CJumpLocation CAssemblyWriterX86::JNELong( CCodeLabel target )
{
	return JumpConditionalLong( target, 0x85 );
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::JMP_REG( EIntelReg reg )
{
	EmitWORD( ((0xe0|reg)<<8) | 0xff );
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::CALL_REG( EIntelReg reg )
{
	EmitWORD(((0xd0|reg)<<8) | 0xff);
}

//*****************************************************************************
//
//*****************************************************************************
CJumpLocation	CAssemblyWriterX86::CALL( CCodeLabel target )
{
	const u32	CALL_LONG_LENGTH = 5;

	CJumpLocation	jump_location( mpAssemblyBuffer->GetJumpLocation() );
	s32				offset( jump_location.GetOffset( target ) - CALL_LONG_LENGTH );

	EmitBYTE( 0xe8 );
	EmitDWORD( offset );

	return jump_location;
}

//*****************************************************************************
//
//*****************************************************************************
CJumpLocation	CAssemblyWriterX86::JMPLong( CCodeLabel target )
{
	const u32	JUMP_DIRECT_LONG_LENGTH = 5;

	CJumpLocation	jump_location( mpAssemblyBuffer->GetJumpLocation() );
	s32				offset( jump_location.GetOffset( target ) - JUMP_DIRECT_LONG_LENGTH );

	EmitBYTE(0xe9);
	EmitDWORD( static_cast< u32 >( offset ) );

	return jump_location;
}

//*****************************************************************************
// call dword ptr [mem + reg*4]
//*****************************************************************************
void	CAssemblyWriterX86::CALL_MEM_PLUS_REGx4( void * mem, EIntelReg reg )
{
	EmitBYTE(0xFF);
	EmitBYTE(0x14);
	EmitBYTE(0x85 | (reg<<3));
	EmitDWORD((u32)mem);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::RET()
{
	EmitBYTE(0xC3);
}

//*****************************************************************************
// mov reg1, reg2
//*****************************************************************************
void	CAssemblyWriterX86::MOV(EIntelReg reg1, EIntelReg reg2)
{
	if (reg1 != reg2)
	{
		EmitBYTE(0x8b);
		EmitBYTE(0xc0 | (reg1<<3) | reg2);
	}
}

//*****************************************************************************
// movsx reg1, reg2
//*****************************************************************************
void	CAssemblyWriterX86::MOVSX(EIntelReg reg1, EIntelReg reg2, bool _8bit)
{
	EmitBYTE(0x0f);
	EmitBYTE(_8bit ? 0xBE : 0xBF);
	EmitBYTE(0xc0 | (reg1<<3) | reg2);
}

//*****************************************************************************
// movzx reg1, reg2
//*****************************************************************************
void	CAssemblyWriterX86::MOVZX(EIntelReg reg1, EIntelReg reg2, bool _8bit)
{
	EmitBYTE(0x0f);
	EmitBYTE(_8bit ? 0xB6 : 0xB7);
	EmitBYTE(0xc0 | (reg1<<3) | reg2);
}

//*****************************************************************************
// mov dword ptr[ mem ], reg
//*****************************************************************************
void	CAssemblyWriterX86::MOV_MEM_REG(void * mem, EIntelReg isrc)
{
	/*if (reg == EAX_CODE)
		EmitBYTE(0xa3);
	else*/
	{
		EmitBYTE(0x89);
		EmitBYTE((isrc<<3) | 0x05);
	}
	EmitDWORD((u32)mem);
}

//*****************************************************************************
// mov reg, dword ptr[ mem ]
//*****************************************************************************
void	CAssemblyWriterX86::MOV_REG_MEM(EIntelReg reg, const void * mem)
{
	/*if (reg == EAX_CODE)
		EmitBYTE(0xa1);
	else*/
	{
		EmitBYTE(0x8b);
		EmitBYTE((reg<<3) | 0x05);
	}

	EmitDWORD((u32)mem);
}

//*****************************************************************************
// mov byte ptr [base], src
//*****************************************************************************
void	CAssemblyWriterX86::MOV8_MEM_BASE_REG( EIntelReg ibase, EIntelReg isrc )
{
	DAEDALUS_ASSERT( IsValidMov8Reg( isrc ), "Invalid source register for 8 bit move" );
	EmitBYTE(0x88);
	EmitBYTE(0x00 | (isrc<<3) | ibase);
}

//*****************************************************************************
// mov dst, byte ptr [base]
//*****************************************************************************
void	CAssemblyWriterX86::MOV8_REG_MEM_BASE( EIntelReg idst, EIntelReg ibase )
{
	DAEDALUS_ASSERT( IsValidMov8Reg( idst ), "Invalid destination register for 8 bit move" );

	EmitBYTE(0x8A);
	EmitBYTE(0x00 | (idst<<3) | ibase);
}

//*****************************************************************************
// mov word ptr [base], src
//*****************************************************************************
void	CAssemblyWriterX86::MOV16_MEM_BASE_REG( EIntelReg ibase, EIntelReg isrc )
{
	EmitBYTE(0x66);
	EmitBYTE(0x89);
	EmitBYTE(0x00 | (isrc<<3) | ibase);
}

//*****************************************************************************
// mov dst, word ptr[base] e.g. mov ax, word ptr [eax]
//*****************************************************************************
void	CAssemblyWriterX86::MOV16_REG_MEM_BASE( EIntelReg idst, EIntelReg ibase )
{
	EmitBYTE(0x66);
	EmitBYTE(0x8B);
	EmitBYTE(0x00 | (idst<<3) | ibase);
}

//*****************************************************************************
// mov dword ptr [base + nnnnnnnn], src
//*****************************************************************************
void	CAssemblyWriterX86::MOV_MEM_BASE_OFFSET32_REG( EIntelReg ibase, s32 offset, EIntelReg isrc )
{
	EmitBYTE(0x89);
	EmitBYTE(0x80 | (isrc<<3) | ibase);
	EmitDWORD((u32)offset);
}

//*****************************************************************************
// mov dword ptr [base + nn], src
//*****************************************************************************
void	CAssemblyWriterX86::MOV_MEM_BASE_OFFSET8_REG( EIntelReg ibase, s8 offset, EIntelReg isrc )
{
	EmitBYTE(0x89);
	EmitBYTE(0x40 | (isrc<<3) | ibase);
	EmitBYTE((u8)offset);
}

//*****************************************************************************
// mov dword ptr [base], src
//*****************************************************************************
void	CAssemblyWriterX86::MOV_MEM_BASE_REG( EIntelReg ibase, EIntelReg isrc )
{
	EmitBYTE(0x89);
	EmitBYTE(0x00 | (isrc<<3) | ibase);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::MOV_MEM_BASE_OFFSET_REG( EIntelReg ibase, s32 offset, EIntelReg isrc )
{
	if(offset == 0)
	{
		MOV_MEM_BASE_REG( ibase, isrc );
	}
	else if (offset < 128 && offset > -128)
	{
		MOV_MEM_BASE_OFFSET8_REG( ibase, (s8)offset, isrc );
	}
	else
	{
		MOV_MEM_BASE_OFFSET32_REG( ibase, offset, isrc );
	}
}

//*****************************************************************************
// mov dst, dword ptr [base + nnnnnnnn]
//*****************************************************************************
void	CAssemblyWriterX86::MOV_REG_MEM_BASE_OFFSET32( EIntelReg idst, EIntelReg ibase, s32 offset )
{
	EmitBYTE(0x8B);
	EmitBYTE(0x80 | (idst<<3) | ibase);
	EmitDWORD((u32)offset);
}

//*****************************************************************************
// mov dst, dword ptr [base + nn]
//*****************************************************************************
void	CAssemblyWriterX86::MOV_REG_MEM_BASE_OFFSET8( EIntelReg idst, EIntelReg ibase, s8 offset )
{
	EmitBYTE(0x8B);
	EmitBYTE(0x40 | (idst<<3) | ibase);
	EmitBYTE((u8)offset);
}

//*****************************************************************************
// mov dst, dword ptr [base]
//*****************************************************************************
void	CAssemblyWriterX86::MOV_REG_MEM_BASE( EIntelReg idst, EIntelReg ibase )
{
	EmitBYTE(0x8B);
	EmitBYTE(0x00 | (idst<<3) | ibase);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::MOV_REG_MEM_BASE_OFFSET( EIntelReg idst, EIntelReg ibase, s32 offset )
{
	if(offset == 0)
	{
		MOV_REG_MEM_BASE( idst, ibase );
	}
	else if(offset < 128 && offset > -128)
	{
		MOV_REG_MEM_BASE_OFFSET8( idst, ibase, (s8)offset );
	}
	else
	{
		MOV_REG_MEM_BASE_OFFSET32( idst, ibase, offset );
	}
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::MOVI(EIntelReg reg, u32 data)
{
	EmitBYTE(0xB8 | reg);
	EmitDWORD(data);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::MOVI_MEM(void * mem, u32 data)
{
	EmitBYTE(0xc7);
	EmitBYTE(0x05);
	EmitDWORD((u32)mem);
	EmitDWORD(data);
}

//*****************************************************************************
//	mov		byte ptr mem, data
//*****************************************************************************
void	CAssemblyWriterX86::MOVI_MEM8(void * mem, u8 data)
{
	EmitBYTE(0xc6);
	EmitBYTE(0x05);
	EmitDWORD((u32)mem);
	EmitBYTE(data);
}

//*****************************************************************************
// movsx	dst, src		(e.g. movsx eax, al)
//*****************************************************************************
void	CAssemblyWriterX86::MOVSX8( EIntelReg idst, EIntelReg isrc )
{
	DAEDALUS_ASSERT( IsValidMov8Reg( isrc ), "Invalid source register for 8 bit move" );
	EmitBYTE(0x0F);
	EmitBYTE(0xBE);
	EmitBYTE(0xC0 | (idst<<3) | isrc);
}

//*****************************************************************************
// movsx	dst, src		(e.g. movsx eax, ax)
//*****************************************************************************
void	CAssemblyWriterX86::MOVSX16( EIntelReg idst, EIntelReg isrc )
{
	EmitBYTE(0x0F);
	EmitBYTE(0xBF);
	EmitBYTE(0xC0 | (idst<<3) | isrc);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::FSQRT()
{
	EmitWORD(0xfad9);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::FCHS()
{
	EmitWORD(0xe0d9);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::FILD_MEM( void * pmem )
{
	EmitWORD(0x05db);
	EmitDWORD( u32(pmem) );
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::FLD_MEMp32( void * pmem )
{
	EmitWORD(0x05d9);
	EmitDWORD( u32(pmem) );
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::FSTP_MEMp32( void * pmem )
{
	EmitWORD( 0x1dd9);
	EmitDWORD( u32(pmem) );
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::FLD_MEMp64( void * memlo, void * memhi )
{
	static s64 longtemp;

	MOV_REG_MEM(EAX_CODE, (u8*)(memlo) );
	MOV_REG_MEM(EDX_CODE, (u8*)(memhi) );
	MOV_MEM_REG(((u8*)&longtemp) + 0, EAX_CODE);
	MOV_MEM_REG(((u8*)&longtemp) + 4, EDX_CODE);
	EmitWORD(0x05dd);
	EmitDWORD( u32(&longtemp) );
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::FSTP_MEMp64( void * memlo, void * memhi )
{
	static s64 longtemp;

	EmitWORD(0x1ddd);
	EmitDWORD( u32(&longtemp) );
	MOV_REG_MEM(EAX_CODE, ((u8*)(&longtemp))+0);
	MOV_REG_MEM(EDX_CODE, ((u8*)(&longtemp))+4);
	MOV_MEM_REG(((u8*)(memlo)), EAX_CODE);
	MOV_MEM_REG(((u8*)(memhi)), EDX_CODE);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::FISTP_MEMp( void * pmem )
{
	EmitWORD( 0x1ddb );
	EmitDWORD((u32)pmem);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::FLD( u32 i )
{
	EmitWORD((u16)((0xc0 + (i))<<8) |(0xd9));
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::FXCH( u32 i )
{
	EmitWORD((u16)((0xc8 + (i))<<8) |(0xd9));
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::FCOMP( u32 i )
{
	EmitWORD((u16)((0xd8 + (i))<<8) |(0xd8));
}

//*****************************************************************************
//fnstsw      ax
//*****************************************************************************
void	CAssemblyWriterX86::FNSTSW()
{
	EmitWORD(0xe0df);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::FFREE( u32 i )
{
	EmitWORD((u16)((0xc0 + (i))<<8) |(0xdd));
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::FINCSTP()
{
	EmitWORD(0xf7d9);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::FSQRTp()
{
	EmitWORD(0xfad9);
}

//*****************************************************************************
//
//*****************************************************************************
void	CAssemblyWriterX86::FCHSp()
{
	EmitWORD(0xe0d9);
}

//*****************************************************************************
// fadd st(0),st(i)
//*****************************************************************************
void	CAssemblyWriterX86::FADD( u32 i )
{
	EmitWORD((u16)((0xc0 + (i))<<8) |(0xd8));
}

//*****************************************************************************
// fsub st(0),st(i)
//*****************************************************************************
void	CAssemblyWriterX86::FSUB( u32 i )
{
	EmitWORD((u16)((0xe0 + (i))<<8) |(0xd8));
}

//*****************************************************************************
// fmul st(0),st(i)
//*****************************************************************************
void	CAssemblyWriterX86::FMUL( u32 i )
{
	EmitWORD((u16)((0xc8 + (i))<<8) |(0xd8));
}

//*****************************************************************************
// fdiv st(0),st(i)
//*****************************************************************************
void	CAssemblyWriterX86::FDIV( u32 i )
{
	EmitWORD((u16)((0xf0 + (i))<<8) |(0xd8));
}

void	CAssemblyWriterX86::CDQ()
{
	EmitBYTE(0x99);
}