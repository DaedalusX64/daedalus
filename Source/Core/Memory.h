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

// I've taken out the memory region checking (for now at least)
// I was getting some very strange bugs with the Memory_AllocRegion
// function (which I think was a compiler bug, but I wasn't sure).
// In any case, reads and writes to the hardware registers is 
// relatively rare, and so the actual speedup is likely to be very
// slight

// Seems to work fine now 8/8/11- Salvy
// For this work properly, make sure to set optimisation atleast -02, otherwise the compiler won't discard the unused code and will cause to overlap!

#ifdef DAEDALUS_PUBLIC_RELEASE
#define MEMORY_BOUNDS_CHECKING(x) 1
#else
#define MEMORY_BOUNDS_CHECKING(x) x
#endif

enum MEMBANKTYPE
{
	MEM_UNUSED = 0,			// Simplifies code so that we don't have to check for illegal memory accesses

	MEM_RD_RAM,				// 8 or 4 Mb (4/8*1024*1024)
	
	MEM_SP_MEM,			// 0x2000

	MEM_PIF_RAM,			// 0x7C0 + 0x40

	MEM_RD_REG0,			// 0x30		// This has changed - used to be 1Mb
	MEM_RD_REG4,			// 0x30
	MEM_RD_REG8,			// 0x30
	MEM_SP_REG,				// 0x20
	MEM_DPC_REG,			// 0x20
	MEM_DPS_REG,			// 0x10
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
typedef bool (*InternalMemFastFunction)( u32 address, void ** p_translated );

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
extern MemFastFunction				g_ReadAddressLookupTable[0x4000];
extern MemFastFunction				g_WriteAddressLookupTable[0x4000];
extern MemWriteValueFunction		g_WriteAddressValueLookupTable[0x4000];
extern InternalMemFastFunction		InternalReadFastTable[0x4000];
extern void*						g_ReadAddressPointerLookupTable[0x4000];
extern void*						g_WriteAddressPointerLookupTable[0x4000];

#else // DAEDALUS_ALIGN_REGISTERS

#include "PushStructPack1.h"
ALIGNED_TYPE(struct, memory_tables_struct_t, PAGE_ALIGN)
{
	MemFastFunction					_g_ReadAddressLookupTable[0x4000];
	MemFastFunction					_g_WriteAddressLookupTable[0x4000];
	MemWriteValueFunction			_g_WriteAddressValueLookupTable[0x4000];

	InternalMemFastFunction			_InternalReadFastTable[0x4000];

	void*							_g_ReadAddressPointerLookupTable[0x4000];
	void*							_g_WriteAddressPointerLookupTable[0x4000];
};
ALIGNED_EXTERN(memory_tables_struct_t, memory_tables_struct, PAGE_ALIGN);
#include "PopStructPack.h"


#define g_ReadAddressLookupTable (memory_tables_struct._g_ReadAddressLookupTable)
#define g_WriteAddressLookupTable (memory_tables_struct._g_WriteAddressLookupTable)
#define g_WriteAddressValueLookupTable (memory_tables_struct._g_WriteAddressValueLookupTable)
#define InternalReadFastTable (memory_tables_struct._InternalReadFastTable)
#define g_ReadAddressPointerLookupTable (memory_tables_struct._g_ReadAddressPointerLookupTable)
#define g_WriteAddressPointerLookupTable (memory_tables_struct._g_WriteAddressPointerLookupTable)

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

#define FuncTableReadAddress(address)  (void *)(g_ReadAddressLookupTable)[(address)>>18](address)
#define FuncTableWriteAddress(address)  (void *)(g_WriteAddressLookupTable)[(address)>>18](address)
#define FuncTableWriteValueAddress(address, value)  (g_WriteAddressValueLookupTable)[(address)>>18](address, value)

#if 0
#define ReadAddress FuncTableReadAddress
#define WriteAddress FuncTableWriteAddress
#define WriteValueAddress FuncTableWriteValueAddress
#else


inline u8 FastRead8bits(u32 address)
{

	return(*((u8 *)(g_ReadAddressLookupTable)[address>>18](address)));
}

#define FastWrite32Bits( address,  data)		*(u32 *)(g_WriteAddressValueLookupTable[address>>18](address, data)) = data;

inline void* ReadAddress( u32 address )
{
	s32 tableEntry = reinterpret_cast< s32 >( g_ReadAddressPointerLookupTable[address >> 18] ) + address;
	if(DAEDALUS_EXPECT_LIKELY(tableEntry >= 0))
	{
		return (void*)(tableEntry);
	}
	else
	{
		return FuncTableReadAddress( address );
	}
}

inline void* WriteAddress( u32 address )
{
	s32 tableEntry = reinterpret_cast< s32 >( g_WriteAddressPointerLookupTable[address >> 18] ) + address;
	if(DAEDALUS_EXPECT_LIKELY(tableEntry >= 0))
	{
		return (void*)(tableEntry);
	}
	else
	{
		return FuncTableWriteAddress(address);
	}
}

inline void WriteValueAddress( u32 address, u32 value )
{
	s32 tableEntry = reinterpret_cast< s32 >( g_WriteAddressPointerLookupTable[address >> 18] ) + address;

	if(DAEDALUS_EXPECT_LIKELY(tableEntry >= 0))
	{
		*(u32*)(tableEntry) = value;
	}
	else
	{
		FuncTableWriteValueAddress( address, value );
	}
}
#endif /* 0 */

inline bool Memory_GetInternalReadAddress(u32 address, void ** p_translated)
{
	return (InternalReadFastTable)[(address)>>18](address, p_translated);
}

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

extern u8 * g_pu8RamBase_8000;
extern u8 * g_pu8RamBase_A000;

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

inline void Write64Bits( u32 address, u64 data )	{ MEMORY_CHECK_ALIGN( address, 8 ); *(u64 *)WriteAddress( address ) = (data>>32) + (data<<32); }
inline void Write32Bits( u32 address, u32 data )	{ MEMORY_CHECK_ALIGN( address, 4 ); WriteValueAddress(address, data); }
inline void Write16Bits( u32 address, u16 data )	{ MEMORY_CHECK_ALIGN( address, 2 ); *(u16 *)WriteAddress(address ^ U16_TWIDDLE) = data; }
inline void Write8Bits( u32 address, u8 data )		{                                   *(u8 *)WriteAddress(address ^ U8_TWIDDLE) = data;}

//inline void Write64Bits_NoSwizzle( u32 address, u64 data ){ MEMORY_CHECK_ALIGN( address, 8 ); *(u64 *)WriteAddress( address ) = (data>>32) + (data<<32); }
inline void Write32Bits_NoSwizzle( u32 address, u32 data )	{ MEMORY_CHECK_ALIGN( address, 4 ); WriteValueAddress(address, data); }
inline void Write16Bits_NoSwizzle( u32 address, u16 data )	{ MEMORY_CHECK_ALIGN( address, 2 ); *(u16 *)WriteAddress(address) = data; }
inline void Write8Bits_NoSwizzle( u32 address, u8 data )	{                                   *(u8 *)WriteAddress(address) = data;}

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
