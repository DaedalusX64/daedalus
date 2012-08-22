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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef DAEDALUSMEMORY_H__
#define DAEDALUSMEMORY_H__

#include "OSHLE/ultra_rcp.h"
#include "Utility/AtomicPrimitives.h"

enum MEMBANKTYPE
{
	MEM_UNUSED = 0,			// Simplifies code so that we don't have to check for illegal memory accesses

	MEM_RD_RAM,				// 8 or 4 Mb (4/8*1024*1024)
	
	MEM_SP_MEM,				// 0x2000

	MEM_PIF_RAM,			// 0x40

	MEM_RD_REG0,			// 0x30		// This has changed - used to be 1Mb
	MEM_SP_REG,				// 0x20
	MEM_DPC_REG,			// 0x20
	MEM_MI_REG,				// 0x10
	MEM_VI_REG,				// 0x38
	MEM_AI_REG,				// 0x18
	MEM_PI_REG,				// 0x34
	MEM_RI_REG,				// 0x20
	MEM_SI_REG,				// 0x1C

	MEM_SAVE,				// 0x20000 128KBytes EEPROM (512 bytes), 4X EEPROM (2Kbytes), SRAM (32Kbytes), FlashRAM (128Kbytes)
	MEM_MEMPACK,			// 0x20000 MEMPack 32Kbytes * 4 = 128KBytes
	
	NUM_MEM_BUFFERS
};


static const u32 MEMORY_4_MEG( 4*1024*1024 );
static const u32 MEMORY_8_MEG( 8*1024*1024 );
#define MAX_RAM_ADDRESS MEMORY_8_MEG

typedef void * (*mReadFunction )( u32 address );
typedef void (*mWriteFunction )( u32 address, u32 value );

struct MemFuncWrite
{
	u8			  *pWrite;
	mWriteFunction WriteFunc;
};

struct MemFuncRead
{
	u8			 *pRead;
	mReadFunction ReadFunc;
};

extern u32		gRamSize;
#ifdef DAEDALUS_PROFILE_EXECUTION
extern u32		gTLBReadHit;
extern u32		gTLBWriteHit;
#endif
extern void *	g_pMemoryBuffers[NUM_MEM_BUFFERS];
extern u32		MemoryRegionSizes[NUM_MEM_BUFFERS];

bool			Memory_Init();
void			Memory_Fini();
void			Memory_Reset();
void			Memory_Cleanup();


typedef void * (*MemFastFunction )( u32 address );
typedef void (*MemWriteValueFunction )( u32 address, u32 value );

#ifndef DAEDALUS_SILENT
typedef bool (*InternalMemFastFunction)( u32 address, void ** p_translated );
#endif

/* Modified by Lkb (24/8/2001)
   These tables were declared as pointers and dynamically allocated.
   However, by declaring them as pointers to access the tables the interpreter must use code like this:

   MOV EAX, DWORD PTR [address_of_the_variable_g_ReadAddressLookupTable]
   MOV EAX, DWORD PTR [EAX + desired_offset]

   Instead, by declaring them as integers the address of table is "known" at compile time
   (at load-time it may be relocated but the code referencing it will be patched)
   and the interpreter can use code like this:

   MOV EAX, DWORD PTR [address_of_the_array_g_ReadAddressLookupTable + desired_offset]

   Note that dynarec-generated code is not affected by this

   The exotic construction is required to ensure page-alignment

   Memory.cpp also changed appropriately
*/

// For debugging, it's more important to be able to use the debugger
#ifndef DAEDALUS_ALIGN_REGISTERS
extern MemFuncRead  g_MemoryLookupTableRead[0x4000];
extern MemFuncWrite g_MemoryLookupTableWrite[0x4000];
#ifndef DAEDALUS_SILENT
extern InternalMemFastFunction		InternalReadFastTable[0x4000];
#endif

#else // DAEDALUS_ALIGN_REGISTERS

#include "PushStructPack1.h"
ALIGNED_TYPE(struct, memory_tables_struct_t, PAGE_ALIGN)
{
	MemFuncRead						_g_MemoryLookupTableRead[0x4000];
	MemFuncWrite					_g_MemoryLookupTableWrite[0x4000];
#ifndef DAEDALUS_SILENT
	InternalMemFastFunction			_InternalReadFastTable[0x4000];
#endif
};
ALIGNED_EXTERN(memory_tables_struct_t, memory_tables_struct, PAGE_ALIGN);
#include "PopStructPack.h"


#define g_MemoryLookupTableRead (memory_tables_struct._g_MemoryLookupTableRead)
#define g_MemoryLookupTableWrite (memory_tables_struct._g_MemoryLookupTableWrite)

#ifndef DAEDALUS_SILENT
#define InternalReadFastTable (memory_tables_struct._InternalReadFastTable)
#endif
#endif // DAEDALUS_ALIGN_REGISTERS


///////////////////////////////////////////////////////////////////////////////////////
//
//

/* Added by Lkb (24/8/2001)
   These tables are used to implement a faster memory system similar to the one used in gnuboy (http://gnuboy.unix-fu.org/ - read docs/HACKING).
   However instead of testing for zero the entry like gnuboy, Daedalus checks the sign of the addition results.
   When the pointer table entry is valid, this should be faster since instead of MOV/TEST/ADD (1+1+1 uops) it uses just ADD mem (2 uops)
   But when the pointer table entry is invalid, it may be slower because it computes the address twice 

   # Old system:
   .intel_syntax
   MOV EAX, address
   MOV ECX, EAX
   SHR EAX, 18
   CALL DWORD PTR [g_ReadAddressLookupTable + EAX*4]
   # --> (for RAM)
   ADD ECX, [rambase_variable]
   MOV EAX, ECX
   RET

   # gnuboy system:
   .intel_syntax   
   MOV EAX, address
   MOV EDX, EAX
   SHR EDX, 18
   MOV ECX, [g_ReadAddressPointerLookupTable + EDX*4]
   TEST ECX, ECX
   JS pointer_null # usually not taken - thus forward branch
   ADD EAX, ECX
pointer_null_return_x:
#   [...] <rest of function code>
#   RET

pointer_null_x:
   MOV ECX, EAX
   CALL DWORD PTR [g_ReadAddressLookupTable + EDX*4]
#  --> (for RAM)
#  ADD ECX, [rambase_variable]
#  MOV EAX, ECX
#  RET
#  <--
   JMP pointer_null_return

   # New system:
   .intel_syntax   
   MOV EAX, address
   MOV EDX, EAX
   SHR EDX, 18
   ADD EAX, [g_ReadAddressPointerLookupTable + EDX*4]
   JS pointer_null # usually not taken - thus forward branch
pointer_null_return_x:
#   [...] <rest of function code>
#   RET

pointer_null_x:
   MOV ECX, address
   CALL DWORD PTR [g_ReadAddressLookupTable + EDX*4]
#  --> (for RAM)
#  ADD ECX, [rambase_variable]
#  MOV EAX, ECX
#  RET
#  <--
   JMP pointer_null_return
   
   Note however that the compiler may generate worse code.

   The old system is still usable (and it is required even if the new one is used since it will fallback to the old for access to memory-mapped hw registers and similar areas)

   TODO: instead of looking up TLB entries each time TLB-mapped memory is used, it is probably much faster to change the pointer table every time the TLB is modified
*/

#if 0
//Slow memory access
#define FuncTableReadAddress(address)  (void *)(g_MemoryLookupTableRead[(address)>>18].ReadFunc(address))
#define FuncTableWriteAddress(address, value)  (g_MemoryLookupTableWrite[(address)>>18].WriteFunc(address, value))

#define ReadAddress FuncTableReadAddress
#define WriteAddress FuncTableWriteAddress
#else
// Fast memory access
inline void* DAEDALUS_ATTRIBUTE_CONST ReadAddress( u32 address )
{
	const MemFuncRead & m( g_MemoryLookupTableRead[ address >> 18 ] );

	// Access through pointer with no function calls at all (Fast)
	if( m.pRead )	
		return (void*)( m.pRead + address );

	// Need to go through the HW access handlers or TLB (Slow)
	return m.ReadFunc( address );	
}

inline void WriteAddress( u32 address, u32 value )
{
	const MemFuncWrite & m( g_MemoryLookupTableWrite[ address >> 18 ] );

	// Access through pointer with no function calls at all (Fast)
	if( m.pWrite )	
	{
		*(u32*)( m.pWrite + address ) = value;
		return;
	}
	// Need to go through the HW access handlers or TLB (Slow)
	m.WriteFunc( address, value );	
}
#endif /* 0 */


#ifndef DAEDALUS_SILENT
inline bool Memory_GetInternalReadAddress(u32 address, void ** p_translated)
{
	return (InternalReadFastTable)[(address)>>18](address, p_translated);
}
#endif

//#define MEMORY_CHECK_ALIGN( address, align )	DAEDALUS_ASSERT( (address & ~(align-1)) == 0, "Unaligned memory access" )
#define MEMORY_CHECK_ALIGN( address, align )	

// Useful defines for making code look nicer:
#define g_pu8RamBase ((u8*)g_pMemoryBuffers[MEM_RD_RAM])
#define g_ps8RamBase ((s8*)g_pMemoryBuffers[MEM_RD_RAM])
#define g_pu16RamBase ((u16*)g_pMemoryBuffers[MEM_RD_RAM])
#define g_pu32RamBase ((u32*)g_pMemoryBuffers[MEM_RD_RAM])

#define g_pu8SpMemBase ((u8*)g_pMemoryBuffers[MEM_SP_MEM])
#define g_ps8SpMemBase ((s8*)g_pMemoryBuffers[MEM_SP_MEM])
#define g_pu16SpMemBase ((u16*)g_pMemoryBuffers[MEM_SP_MEM])
#define g_pu32SpMemBase ((u32*)g_pMemoryBuffers[MEM_SP_MEM])

#define g_pu8SpDmemBase	((u8*)g_pMemoryBuffers[MEM_SP_MEM] + SP_DMA_DMEM)
#define g_pu8SpImemBase	((u8*)g_pMemoryBuffers[MEM_SP_MEM] + SP_DMA_IMEM)

#define MEMORY_SIZE_RDRAM				0x400000
#define MEMORY_SIZE_EXRDRAM				0x400000
#define MEMORY_SIZE_RDRAM_DEFAULT		MEMORY_SIZE_RDRAM + MEMORY_SIZE_EXRDRAM
#define MEMORY_SIZE_RAMREGS0			0x30
#define MEMORY_SIZE_RAMREGS4			0x30
#define MEMORY_SIZE_RAMREGS8			0x30
#define MEMORY_SIZE_SPMEM				0x2000
#define MEMORY_SIZE_SPREG_1				0x24
#define MEMORY_SIZE_SPREG_2				0x8
#define MEMORY_SIZE_DPC					0x20
#define MEMORY_SIZE_DPS					0x10
#define MEMORY_SIZE_MI					0x10
#define MEMORY_SIZE_VI					0x50
#define MEMORY_SIZE_AI					0x18
#define MEMORY_SIZE_PI					0x4C
#define MEMORY_SIZE_RI					0x20
#define MEMORY_SIZE_SI					0x1C
#define MEMORY_SIZE_C2A1				0x8000
#define MEMORY_SIZE_C1A1				0x8000
#define MEMORY_SIZE_C2A2				0x20000
#define MEMORY_SIZE_GIO_REG				0x804
#define MEMORY_SIZE_C1A3				0x8000
#define MEMORY_SIZE_PIF					0x800
#define MEMORY_SIZE_DUMMY				0x10000

#define MEMORY_START_RDRAM		0x00000000
#define MEMORY_START_EXRDRAM	0x00400000
#define MEMORY_START_RAMREGS0	0x03F00000
#define MEMORY_START_RAMREGS4	0x03F04000
#define MEMORY_START_RAMREGS8	0x03F80000
#define MEMORY_START_SPMEM		0x04000000
#define MEMORY_START_SPREG_1	0x04040000
#define MEMORY_START_SPREG_2	0x04080000
#define MEMORY_START_DPC		0x04100000
#define MEMORY_START_DPS		0x04200000
#define MEMORY_START_MI			0x04300000
#define MEMORY_START_VI			0x04400000
#define MEMORY_START_AI			0x04500000
#define MEMORY_START_PI			0x04600000
#define MEMORY_START_RI			0x04700000
#define MEMORY_START_SI			0x04800000
#define MEMORY_START_C2A1		0x05000000
#define MEMORY_START_C1A1		0x06000000
#define MEMORY_START_C2A2		0x08000000
#define MEMORY_START_ROM_IMAGE	0x10000000
#define MEMORY_START_GIO		0x18000000
#define MEMORY_START_PIF		0x1FC00000
#define MEMORY_START_C1A3		0x1FD00000
#define MEMORY_START_DUMMY		0x1FFF0000

#define MEMORY_RDRAM			g_pMemoryBuffers[MEM_RD_RAM]
#define MEMORY_RAMREGS0			g_pMemoryBuffers[MEM_RD_REG0]
#define MEMORY_SPMEM			g_pMemoryBuffers[MEM_SP_MEM]
#define MEMORY_SPREG_1			g_pMemoryBuffers[MEM_SP_REG]
#define MEMORY_DPC				g_pMemoryBuffers[MEM_DPC_REG]

#define MEMORY_MI				g_pMemoryBuffers[MEM_MI_REG]
#define MEMORY_SI				g_pMemoryBuffers[MEM_SI_REG]
#define MEMORY_PI				g_pMemoryBuffers[MEM_PI_REG]
#define MEMORY_AI				g_pMemoryBuffers[MEM_AI_REG]
#define MEMORY_RI				g_pMemoryBuffers[MEM_RI_REG]
#define MEMORY_PIF				g_pMemoryBuffers[MEM_PIF_RAM]

// Little Endian
#define SWAP_PIF(x) (x >> 24) | ((x >> 8) & 0xFF00) | ((x & 0xFF00) << 8) | (x << 24)

//extern u8 * g_pu8RamBase_8000;
//extern u8 * g_pu8RamBase_A000;

#if 1	//inline vs macro
inline u64 Read64Bits( u32 address )				{ MEMORY_CHECK_ALIGN( address, 8 ); u64 data = *(u64 *)ReadAddress( address ); data = (data>>32) + (data<<32); return data; }
inline u32 Read32Bits( u32 address )				{ MEMORY_CHECK_ALIGN( address, 4 ); return *(u32 *)ReadAddress( address ); }
inline u16 Read16Bits( u32 address )				{ MEMORY_CHECK_ALIGN( address, 2 ); return *(u16 *)ReadAddress( address ^ U16_TWIDDLE ); }
inline u8 Read8Bits( u32 address )					{                                   return *(u8  *)ReadAddress( address ^ U8_TWIDDLE ); }
#else
inline u64 Read64Bits( u32 address )
{
	MEMORY_CHECK_ALIGN( address, 8 );
	union
	{
		u64 data;
		u32 dpart[2];
	}uni = {*(u64 *)ReadAddress( address )};
	return ((u64)uni.dpart[0] << 32) | uni.dpart[1];
}
#define Read32Bits( addr )							( *(u32 *)ReadAddress( (addr) ))
#define Read16Bits( addr )							( *(u16 *)ReadAddress( (addr) ^ U16_TWIDDLE ))
#define Read8Bits( addr )							( *(u8  *)ReadAddress( (addr) ^ U8_TWIDDLE ))
#endif

inline void Write64Bits( u32 address, u64 data )	{ MEMORY_CHECK_ALIGN( address, 8 ); *(u64 *)ReadAddress( address ) = (data>>32) + (data<<32); }
inline void Write32Bits( u32 address, u32 data )	{ MEMORY_CHECK_ALIGN( address, 4 ); WriteAddress(address, data); }
inline void Write16Bits( u32 address, u16 data )	{ MEMORY_CHECK_ALIGN( address, 2 ); *(u16 *)ReadAddress(address ^ U16_TWIDDLE) = data; }
inline void Write8Bits( u32 address, u8 data )		{                                   *(u8 *)ReadAddress(address ^ U8_TWIDDLE) = data;}

//inline void Write64Bits_NoSwizzle( u32 address, u64 data ){ MEMORY_CHECK_ALIGN( address, 8 ); *(u64 *)WriteAddress( address ) = (data>>32) + (data<<32); }
inline void Write32Bits_NoSwizzle( u32 address, u32 data )	{ MEMORY_CHECK_ALIGN( address, 4 ); WriteAddress(address, data); }
inline void Write16Bits_NoSwizzle( u32 address, u16 data )	{ MEMORY_CHECK_ALIGN( address, 2 ); *(u16 *)ReadAddress(address) = data; }
inline void Write8Bits_NoSwizzle( u32 address, u8 data )	{                                   *(u8 *)ReadAddress(address) = data;}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//                Register Macros                  //
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

#define REGISTER_FUNCTIONS( set, base_reg, memory_buffer )								\
	inline void Memory_##set##_SetRegister( u32 reg, u32 value )						\
	{																					\
		((u32 *)g_pMemoryBuffers[memory_buffer])[ (reg - base_reg) / 4 ] = value;		\
	}																					\
																						\
	inline u32 Memory_##set##_GetRegister( u32 reg )									\
	{																					\
		return ((u32 *)g_pMemoryBuffers[memory_buffer])[ (reg - base_reg) / 4 ];		\
	}																					\
																						\
	inline u32 Memory_##set##_SetRegisterBits( u32 reg, u32 and_bits, u32 or_bits )	\
	{																					\
		u32 * p( &((u32 *)g_pMemoryBuffers[memory_buffer])[ (reg - base_reg) / 4 ] );	\
		return AtomicBitSet( p, and_bits, or_bits );									\
	}																					\
																						\
	inline u32 Memory_##set##_SetRegisterBits( u32 reg, u32 value )						\
	{																					\
		u32 * p( &((u32 *)g_pMemoryBuffers[memory_buffer])[ (reg - base_reg) / 4 ] );	\
		return AtomicBitSet( p, 0xffffffff, value );									\
	}																					\
																						\
	inline u32 Memory_##set##_ClrRegisterBits( u32 reg, u32 value )						\
	{																					\
		u32 * p( &((u32 *)g_pMemoryBuffers[memory_buffer])[ (reg - base_reg) / 4 ] );	\
		return AtomicBitSet( p, ~value, 0 );											\
	}																					\
																						\
	inline u32 * set##_REG_ADDRESS( u32 reg )											\
	{																					\
		return &((u32 *)g_pMemoryBuffers[memory_buffer])[(reg - base_reg) / 4];			\
	}

REGISTER_FUNCTIONS( MI, MI_BASE_REG, MEM_MI_REG )
REGISTER_FUNCTIONS( SP, SP_BASE_REG, MEM_SP_REG )
REGISTER_FUNCTIONS( AI, AI_BASE_REG, MEM_AI_REG )
REGISTER_FUNCTIONS( VI, VI_BASE_REG, MEM_VI_REG )
REGISTER_FUNCTIONS( SI, SI_BASE_REG, MEM_SI_REG )
REGISTER_FUNCTIONS( PI, PI_BASE_REG, MEM_PI_REG )
REGISTER_FUNCTIONS( DPC, DPC_BASE_REG, MEM_DPC_REG )

#undef REGISTER_FUNCTIONS

#endif	// DAEDALUSMEMORY_H__
