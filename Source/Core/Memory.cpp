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

//Define to use TRUE REALITYs memory model
//Seems more compatible (Salvy/Corn)
#define REALITY_MEM

// Various stuff to map an address onto the correct memory region

#include "stdafx.h"
#include "Memory.h"

#include "CPU.h"
#include "DMA.h"
#include "RSP.h"
#include "RSP_HLE.h"
#include "Interrupt.h"
#include "ROM.h"
#include "ROMBuffer.h"
#include "Save.h"

#include "Math/MathUtil.h"

#include "Debug/Dump.h"		// Dump_GetSaveDirectory()
#include "Debug/DebugLog.h"
#include "Debug/DBGConsole.h"

#include "OSHLE/ultra_gbi.h"
#include "OSHLE/ultra_R4300.h"

#include "Plugins/AudioPlugin.h"
#include "Plugins/GraphicsPlugin.h"

#include "ConfigOptions.h"

static const u32	MAXIMUM_MEM_SIZE( MEMORY_8_MEG );

#undef min

#ifdef DAEDALUS_LOG
static void DisplayVIControlInfo( u32 control_reg );
#endif

static void MemoryUpdateSPStatus( u32 flags );
static void MemoryUpdateDP( u32 value );
static void MemoryModeRegMI( u32 value );
static void MemoryUpdateMI( u32 value );
static void MemoryUpdatePI( u32 value );
static void MemoryUpdatePIF();
//static void MemoryDoDP();

static void Memory_InitTables();

#ifdef REALITY_MEM
static void init_fast_mem();
#endif

//*****************************************************************************
//
//*****************************************************************************
u32 MemoryRegionSizes[NUM_MEM_BUFFERS] =
{
	8*1024,				// Allocate 8k bytes - a bit excessive but some of the internal functions assume it's there!
	MAXIMUM_MEM_SIZE,	// RD_RAM
	0x2000,				// SP_MEM

	0x800,				// PI_ROM/RAM

	//1*1024*1024,		// RD_REG Don't need this much really?
	0x30,				// RD_REG0
	0x30,				// RD_REG1
	0x30,				// RD_REG2

	0x20,				// SP_REG
	0x20,				// DPC_REG
	0x10,				// DPS_REG
	0x10,				// MI_REG
	0x38,				// VI_REG
	0x18,				// AI_REG
	0x34,				// PI_REG
	0x20,				// RI_REG
	0x1C,				// SI_REG

	0x20000,			// SAVE
	0x20000				// MEMPACK
};

//*****************************************************************************
//
//*****************************************************************************
u32			gRamSize( MAXIMUM_MEM_SIZE );	// Size of emulated RAM

#ifdef DAEDALUS_PROFILE_EXECUTION
u32			gTLBReadHit( 0 );
u32			gTLBWriteHit( 0 );
#endif

// Ram base, offset by 0x80000000 and 0xa0000000
u8 * g_pu8RamBase_8000 = NULL;
u8 * g_pu8RamBase_A000 = NULL;

// Flash RAM Support
extern u32 FlashStatus[2];
void Flash_DoCommand(u32);

//*****************************************************************************
//
//*****************************************************************************
#include "Memory_Read.inl"
#include "Memory_Write.inl"
#include "Memory_WriteValue.inl"
#include "Memory_ReadInternal.inl"

//*****************************************************************************
//
//*****************************************************************************
struct InternalMemMapEntry
{
	u32 mStartAddr, mEndAddr;
	InternalMemFastFunction InternalReadFastFunction;
};

// Physical ram: 0x80000000 upwards is set up when tables are initialised
InternalMemMapEntry InternalMemMapEntries[] =
{
	{ 0x0000, 0x7FFF, InternalReadMapped },			// Mapped Memory
	{ 0x8000, 0x807F, InternalReadInvalid },		// RDRAM - Initialised later
	{ 0x8080, 0x83FF, InternalReadInvalid },		// Cartridge Domain 2 Address 1
	{ 0x8400, 0x8400, InternalRead_8400_8400 },		// Cartridge Domain 2 Address 1
	{ 0x8404, 0x85FF, InternalReadInvalid },		// Cartridge Domain 2 Address 1
	{ 0x8600, 0x87FF, InternalReadROM },			// Cartridge Domain 1 Address 1
	{ 0x8800, 0x8FFF, InternalReadROM },			// Cartridge Domain 2 Address 2
	{ 0x9000, 0x9FBF, InternalReadROM },			// Cartridge Domain 1 Address 2
	{ 0x9FC0, 0x9FCF, InternalRead_9FC0_9FCF },		// pif RAM/ROM
	{ 0x9FD0, 0x9FFF, InternalReadROM },			// Cartridge Domain 1 Address 3

	{ 0xA000, 0xA07F, InternalReadInvalid },		// Physical RAM - Copy of above
	{ 0xA080, 0xA3FF, InternalReadInvalid },		// Unused
	{ 0xA400, 0xA400, InternalRead_8400_8400 },		// Unused
	{ 0xA404, 0xA4FF, InternalReadInvalid },		// Unused
	{ 0xA500, 0xA5FF, InternalReadROM },			// Cartridge Domain 2 Address 1
	{ 0xA600, 0xA7FF, InternalReadROM },			// Cartridge Domain 1 Address 1
	{ 0xA800, 0xAFFF, InternalReadROM },			// Cartridge Domain 2 Address 2
	{ 0xB000, 0xBFBF, InternalReadROM },			// Cartridge Domain 1 Address 2
	{ 0xBFC0, 0xBFCF, InternalRead_9FC0_9FCF },		// pif RAM/ROM
	{ 0xBFD0, 0xBFFF, InternalReadROM },			// Cartridge Domain 1 Address 3

	{ 0xC000, 0xDFFF, InternalReadMapped },			// Mapped Memory
	{ 0xE000, 0xFFFF, InternalReadMapped },			// Mapped Memory

	{ ~0,  ~0, NULL}
};


//*****************************************************************************
//
//*****************************************************************************
struct MemMapEntry
{
	u32 mStartAddr, mEndAddr;
	MemFastFunction ReadFastFunction;
	MemFastFunction WriteFastFunction;
	MemWriteValueFunction WriteValueFunction;
	unsigned int ReadRegion;
	unsigned int WriteRegion;
};

MemMapEntry MemMapEntries[] =
{
	{ 0x0000, 0x7FFF, ReadMapped,		WriteMapped,	WriteValueMapped,		0,			0 },			// Mapped Memory
	{ 0x8000, 0x807F, Read_Noise,		Write_Noise,	WriteValueNoise,		0,			0 },			// RDRAM - filled in with correct function later
	{ 0x8080, 0x83EF, Read_Noise,		Write_Noise,	WriteValueNoise,		0,			0 },			// Unused - electrical noise
	{ 0x83F0, 0x83F0, Read_83F0_83F0,	Write_83F0_83F0,WriteValue_83F0_83F0,	MEM_RD_REG0,	MEM_RD_REG0 },	// RDRAM reg
	{ 0x83F4, 0x83FF, ReadInvalid,		WriteInvalid,	WriteValueInvalid,		0,			0},				// Unused
	{ 0x8400, 0x8400, Read_8400_8400,	WriteInvalid,	WriteValue_8400_8400,	MEM_SP_MEM, MEM_SP_MEM },	// DMEM/IMEM
	{ 0x8404, 0x8404, Read_8404_8404,	Write_8400_8400,WriteValue_8404_8404,	MEM_SP_REG, 0 },			// SP Reg
	{ 0x8408, 0x8408, Read_8408_8408,	WriteInvalid,	WriteValue_8408_8408,	0,			0 },			// SP PC/IBIST
	{ 0x840C, 0x840F, ReadInvalid,		WriteInvalid,	WriteValueInvalid,		0,			0 },			// Unused
	{ 0x8410, 0x841F, Read_8410_841F,	WriteInvalid,	WriteValue_8410_841F,	0/*MEM_DPC_REG*/, 0 },		// DPC Reg
	{ 0x8420, 0x842F, Read_8420_842F,	WriteInvalid,	WriteValue_8420_842F,	0,			0 },			// DPS Reg
	{ 0x8430, 0x843F, Read_8430_843F,	WriteInvalid,	WriteValue_8430_843F,	MEM_MI_REG, 0 },			// MI Reg
	{ 0x8440, 0x844F, Read_8440_844F,	WriteInvalid,	WriteValue_8440_844F,	0,			0 },			// VI Reg
	{ 0x8450, 0x845F, Read_8450_845F,	WriteInvalid,	WriteValue_8450_845F,	0,			0 },			// AI Reg
	{ 0x8460, 0x846F, Read_8460_846F,	WriteInvalid,	WriteValue_8460_846F,	MEM_PI_REG, 0 },			// PI Reg
	{ 0x8470, 0x847F, Read_8470_847F,	WriteInvalid,	WriteValue_8470_847F,	MEM_RI_REG, MEM_RI_REG },	// RI Reg
	{ 0x8480, 0x848F, Read_8480_848F,	WriteInvalid,	WriteValue_8480_848F,	0,			0 },			// SI Reg
	{ 0x8490, 0x84FF, ReadInvalid,		WriteInvalid,	WriteValueInvalid,		0,			0 },			// Unused
	{ 0x8500, 0x85FF, ReadInvalid/*ReadROM*/,	WriteInvalid, WriteValue_Cartridge, 0,		0 },			// Cartridge Domain 2 Address 1
	{ 0x8600, 0x87FF, ReadROM,			WriteInvalid,	WriteValue_Cartridge,	0,			0 },			// Cartridge Domain 1 Address 1
	{ 0x8800, 0x8FFF, ReadROM,			WriteInvalid,	WriteValue_Cartridge,	0,			0 },			// Cartridge Domain 2 Address 2
	{ 0x9000, 0x9FBF, ReadROM,			WriteInvalid,	WriteValue_Cartridge,	0,			0 },			// Cartridge Domain 1 Address 2
	{ 0x9FC0, 0x9FCF, Read_9FC0_9FCF,	WriteInvalid,	WriteValue_9FC0_9FCF,	0,			0 },			// pif RAM/ROM
	{ 0x9FD0, 0x9FFF, ReadROM,			WriteInvalid,	WriteValue_Cartridge,	0,			0 },			// Cartridge Domain 1 Address 3

	{ 0xA000, 0xA07F, Read_Noise,		Write_Noise,	WriteValueNoise,		0,			0 },			// Physical RAM - Copy of above
	{ 0xA080, 0xA3EF, Read_Noise,		Write_Noise,	WriteValueNoise,		0,			0 },			// Unused
	{ 0xA3F0, 0xA3FF, Read_83F0_83F0,	Write_83F0_83F0,WriteValue_83F0_83F0,	MEM_RD_REG0,	MEM_RD_REG0 },	// RDRAM Reg
	//{ 0xA3F4, 0xA3FF, ReadInvalid,		WriteInvalid },		// Unused
	{ 0xA400, 0xA400, Read_8400_8400,	Write_8400_8400,	WriteValue_8400_8400, MEM_SP_MEM, MEM_SP_MEM },	// DMEM/IMEM
	{ 0xA404, 0xA404, Read_8404_8404,	WriteInvalid,	WriteValue_8404_8404,	MEM_SP_REG, 0 },			// SP Reg
	{ 0xA408, 0xA408, Read_8408_8408,	WriteInvalid,	WriteValue_8408_8408,	0,			0 },			// SP PC/OBOST
	{ 0xA40C, 0xA40F, ReadInvalid,		WriteInvalid,	WriteValueInvalid,		0,			0 },			// Unused
	{ 0xA410, 0xA41F, Read_8410_841F,	WriteInvalid,	WriteValue_8410_841F,	0/*MEM_DPC_REG*/, 0 },		// DPC Reg
	{ 0xA420, 0xA42F, Read_8420_842F,	WriteInvalid,	WriteValue_8420_842F,	0,			0 },			// DPS Reg

	{ 0xA430, 0xA43F, Read_8430_843F,	WriteInvalid,	WriteValue_8430_843F,	MEM_MI_REG, 0 },			// MI Reg
	{ 0xA440, 0xA44F, Read_8440_844F,	WriteInvalid,	WriteValue_8440_844F,	0,			0 },			// VI Reg
	{ 0xA450, 0xA45F, Read_8450_845F,	WriteInvalid,	WriteValue_8450_845F,	0,			0 },			// AI Reg
	{ 0xA460, 0xA46F, Read_8460_846F,	WriteInvalid,	WriteValue_8460_846F,	MEM_PI_REG, 0 },			// PI Reg
	{ 0xA470, 0xA47F, Read_8470_847F,	WriteInvalid,	WriteValue_8470_847F,	MEM_RI_REG, MEM_RI_REG },	// RI Reg
	{ 0xA480, 0xA48F, Read_8480_848F,	WriteInvalid,	WriteValue_8480_848F,	0,			0 },			// SI Reg
	{ 0xA490, 0xA4FF, ReadInvalid,		WriteInvalid,	WriteValueInvalid,		0,			0 },			// Unused
	{ 0xA500, 0xA5FF, ReadROM,			WriteInvalid,	WriteValue_Cartridge,	0,			0 },			// Cartridge Domain 2 Address 1
	{ 0xA600, 0xA7FF, ReadROM,			WriteInvalid,	WriteValue_Cartridge,	0,			0 },			// Cartridge Domain 1 Address 1
	{ 0xA800, 0xAFFF, ReadROM,			WriteInvalid,	WriteValue_Cartridge,	0,			0 },			// Cartridge Domain 2 Address 2
	{ 0xB000, 0xBFBF, ReadROM,			WriteInvalid,	WriteValue_Cartridge,	0,			0 },			// Cartridge Domain 1 Address 2
	{ 0xBFC0, 0xBFCF, Read_9FC0_9FCF,	WriteInvalid,	WriteValue_9FC0_9FCF,	0,			0 },			// pif RAM/ROM
	{ 0xBFD0, 0xBFFF, ReadROM,			WriteInvalid,	WriteValue_Cartridge,	0,			0 },			// Cartridge Domain 1 Address 3

	{ 0xC000, 0xDFFF, ReadMapped,		WriteMapped,	WriteValueMapped,		0,			0 },			// Mapped Memory
	{ 0xE000, 0xFFFF, ReadMapped,		WriteMapped,	WriteValueMapped,		0,			0 },			// Mapped Memory

	{ ~0,  ~0, NULL, NULL, 0,  0, 0 }																		// ?? Can we safely remove this?
};

//*****************************************************************************
//
//*****************************************************************************
#ifndef DAEDALUS_ALIGN_REGISTERS
MemFastFunction g_ReadAddressLookupTable[0x4000];
MemFastFunction g_WriteAddressLookupTable[0x4000];
MemWriteValueFunction g_WriteAddressValueLookupTable[0x4000];
InternalMemFastFunction InternalReadFastTable[0x4000];
void* g_ReadAddressPointerLookupTable[0x4000];
void* g_WriteAddressPointerLookupTable[0x4000];

#else // DAEDALUS_ALIGN_REGISTERS

ALIGNED_GLOBAL(memory_tables_struct_t, memory_tables_struct, PAGE_ALIGN);

#endif // DAEDALUS_ALIGN_REGISTERS

//*****************************************************************************
//
//*****************************************************************************
void * g_pMemoryBuffers[NUM_MEM_BUFFERS];

//*****************************************************************************
//
//*****************************************************************************
bool Memory_Init()
{
	gRamSize = MAXIMUM_MEM_SIZE;

	for(u32 m = 0; m < NUM_MEM_BUFFERS; m++)
	{
		u32		region_size( MemoryRegionSizes[m] );
		// Skip zero sized areas. An example of this is the cart rom
		if(region_size > 0)
		{
			g_pMemoryBuffers[m] = new u8[region_size];
			//g_pMemoryBuffers[m] = Memory_AllocRegion(region_size);

			if(g_pMemoryBuffers[m] == NULL)
			{
				return false;
			}

			// Necessary?
			//memset(g_pMemoryBuffers[m], 0, region_size);
			/*if (region_size < 0x100) // dirty, check if this is a I/O range
			{
				g_pMemoryBuffers[m] = MAKE_UNCACHED_PTR(g_pMemoryBuffers[m]);
			}*/
		}
	}

	g_pu8RamBase_8000 = ((u8*)g_pMemoryBuffers[MEM_RD_RAM]) - 0x80000000;
	//g_pu8RamBase_A000 = ((u8*)g_pMemoryBuffers[MEM_RD_RAM]) - 0xa0000000;
	g_pu8RamBase_A000 = ((u8*)MAKE_UNCACHED_PTR(g_pMemoryBuffers[MEM_RD_RAM])) - 0xa0000000;

	Memory_InitTables();

	return true;
}

#ifdef REALITY_MEM
/*****************************************************************************\
*                                                                             *
* The static fast memory access routines follow.                              *
* Including the init routine and all function pointer routines.               *
*                                                                             *
* I know that this section is VERY huge!!! But there is no serious way        *
* to do it not like this.                                                     *
*                                                                             *
\*****************************************************************************/

/**
 *
 * initialisation of memory function pointers
 * - called by alloc_n64_mem()
 *
**/

void Write32Bits1( u32 address, u32 data )	{ MEMORY_CHECK_ALIGN( address, 4 ); WriteValueAddress(address, data); }

static void init_fast_mem()
{
	int i;

/**
* 0x00000000 - 0x7fffffff: mapped memory
* 0x80000000 - 0x9fffffff: physical memory (cachable)
* 0xa0000000 - 0xbfffffff: physical memory (uncachable) (copy of the above!)
* 0xc0000000 - 0xdfffffff: mapped memory
* 0xe0000000 - 0xffffffff: mapped memory
*
* we don't care if the mem is cached or uncached :-)
**/

	//0x00000000 - 0x7fffffff: mapped memory 
	for( i = (0x0000>>2); i <= (0x7fff>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = ReadMapped;
		g_WriteAddressValueLookupTable[i] = WriteValueMapped;
	}
	//0x80000000 - 0x9fffffff: physical memory (cachable)
	//rdram
	for( i = (0x8000>>2); i <= (0x803f>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_RAM_4Mb_8000_803F;
		g_WriteAddressValueLookupTable[i] = WriteValue_RAM_4Mb_8000_803F;
	}
	 //not used 
	for( i = (0x8040>>2); i <= (0x83ef>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_Noise;
		g_WriteAddressValueLookupTable[i] = WriteValueNoise;
	}
	 //rdram reg 
	for( i = (0x83f0>>2); i <= (0x83f3>>2); i++ ) {
		g_ReadAddressLookupTable[i] = Read_83F0_83F0;
		g_WriteAddressValueLookupTable[i] = WriteValue_83F0_83F0;
	}
	 //not used 
	for( i = (0x83f4>>2); i <= (0x83ff>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_Noise;
		g_WriteAddressValueLookupTable[i] = WriteValueNoise;
	}
	 //dmem/imem 
	for( i = (0x8400>>2); i <= (0x8403>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_8400_8400;
		g_WriteAddressValueLookupTable[i] = WriteValue_8400_8400;
	}
	 //sp reg 
	for( i = (0x8404>>2); i <= (0x8407>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_8404_8404;
		g_WriteAddressValueLookupTable[i] = WriteValue_8404_8404;
	}
	 //sp pc/ibist 
	for( i = (0x8408>>2); i <= (0x840b>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_8408_8408;
		g_WriteAddressValueLookupTable[i] = WriteValue_8408_8408;
	}
	 //not used 
	for( i = (0x840c>>2); i <= (0x840f>>2); i++ )
	{
		g_ReadAddressLookupTable[i] = Read_Noise;
		g_WriteAddressValueLookupTable[i] = WriteValueNoise;
	}
	 //dpc reg 
	for( i = (0x8410>>2); i <= (0x841f>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_8410_841F;
		g_WriteAddressValueLookupTable[i] = WriteValue_8410_841F;
	}
	 //dps reg 
	for( i = (0x8420>>2); i <= (0x842f>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_8420_842F;
		g_WriteAddressValueLookupTable[i] = WriteValue_8420_842F;
	}
	/* mi reg */
	for( i = (0x8430>>2); i <= (0x843f>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_8430_843F;
		g_WriteAddressValueLookupTable[i] = WriteValue_8430_843F;
	}
	/* vi reg */
	for( i = (0x8440>>2); i <= (0x844f>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_8440_844F;
		g_WriteAddressValueLookupTable[i] = WriteValue_8440_844F;
	}
	/* ai reg */
	for( i = (0x8450>>2); i <= (0x845f>>2); i++ )
	{
		g_ReadAddressLookupTable[i] = Read_8450_845F;
		g_WriteAddressValueLookupTable[i] = WriteValue_8450_845F;
	}
	/* pi reg */
	for( i = (0x8460>>2); i <= (0x846f>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_8460_846F;
		g_WriteAddressValueLookupTable[i] = WriteValue_8460_846F;
	}
	/* ri reg */
	for( i = (0x8470>>2); i <= (0x847f>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_8470_847F;
		g_WriteAddressValueLookupTable[i] = WriteValue_8470_847F;
	}
	/* si reg */
	for( i = (0x8480>>2); i <= (0x848f>>2); i++ )
	{
		g_ReadAddressLookupTable[i] = Read_8480_848F;
		g_WriteAddressValueLookupTable[i] = WriteValue_8480_848F;
	}
	/* not used */
	for( i = (0x8490>>2); i <= (0x84ff>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_Noise;
		g_WriteAddressValueLookupTable[i] = WriteValueNoise;
	}
	/* cartridge domain 2 addr 1 */
	for( i = (0x8500>>2); i <= (0x85ff>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_Noise;
		g_WriteAddressValueLookupTable[i] = WriteValueNoise;
	}
	/* cartridge domain 1 addr 1 */
	for( i = (0x8600>>2); i <= (0x87ff>>2); i++ )
	{
		g_ReadAddressLookupTable[i] = ReadROM;
		g_WriteAddressValueLookupTable[i] = WriteValueNoise;
	}
	/* cartridge domain 2 addr 2 */
	for( i = (0x8800>>2); i <= (0x8fff>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = ReadROM;
		g_WriteAddressValueLookupTable[i] = WriteValueNoise;
	}
	/* cartridge domain 1 addr 2 */
	for( i = (0x9000>>2); i <= (0x9fbf>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = ReadROM;
		g_WriteAddressValueLookupTable[i] = WriteValueNoise;
	}
	/* pif ram/rom */
	for( i = (0x9fc0>>2); i <= (0x9fcf>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_9FC0_9FCF;
		g_WriteAddressValueLookupTable[i] = WriteValue_9FC0_9FCF;
	}
	/* cartridge domain 1 addr 3 */
	for( i = (0x9fd0>>2); i <= (0x9fff>>2); i++ )
	{
		g_ReadAddressLookupTable[i] = ReadROM;
		g_WriteAddressValueLookupTable[i] = WriteValueNoise;
	}
	/* 0xa0000000 - 0xbfffffff: physical memory (uncachable) */
	/* (copy of the above!) **/
	/* rdram */
	for( i = (0xa000>>2); i <= (0xa03f>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_RAM_4Mb_8000_803F;
		g_WriteAddressValueLookupTable[i] = WriteValue_RAM_4Mb_8000_803F;
	}
	/* not used */
	for( i = (0xa040>>2); i <= (0xa3ef>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_Noise;
		g_WriteAddressValueLookupTable[i] = WriteValueNoise;
	}
	/* rdram reg */
	for( i = (0xa3f0>>2); i <= (0xa3f3>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_83F0_83F0;
		g_WriteAddressValueLookupTable[i] = WriteValue_83F0_83F0;
	}
	/* not used */
	for( i = (0xa3f4>>2); i <= (0xa3ff>>2); i++ )
	{
		g_ReadAddressLookupTable[i] = Read_Noise;
		g_WriteAddressValueLookupTable[i] = WriteValueNoise;
	}
	/* dmem/imem */
	for( i = (0xa400>>2); i <= (0xa403>>2); i++ )
	{
		g_ReadAddressLookupTable[i] = Read_8400_8400;
		g_WriteAddressValueLookupTable[i] = WriteValue_8400_8400;
	}
	/* sp reg */
	for( i = (0xa404>>2); i <= (0xa407>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_8404_8404;
		g_WriteAddressValueLookupTable[i] = WriteValue_8404_8404;
	}
	/* sp pc/ibist */
	for( i = (0xa408>>2); i <= (0xa40b>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_8408_8408;
		g_WriteAddressValueLookupTable[i] = WriteValue_8408_8408;
	}
	/* not used */
	for( i = (0xa40c>>2); i <= (0xa40f>>2); i++ )
	{
		g_ReadAddressLookupTable[i] = Read_Noise;
		g_WriteAddressValueLookupTable[i] = WriteValueNoise;
	}
	/* dpc reg */
	for( i = (0xa410>>2); i <= (0xa41f>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_8410_841F;
		g_WriteAddressValueLookupTable[i] = WriteValue_8410_841F;
	}
	/* dps reg */
	for( i = (0xa420>>2); i <= (0xa42f>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_8420_842F;
		g_WriteAddressValueLookupTable[i] = WriteValue_8420_842F;
	}
	/* mi reg */
	for( i = (0xa430>>2); i <= (0xa43f>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_8430_843F;
		g_WriteAddressValueLookupTable[i] = WriteValue_8430_843F;
	}
	/* vi reg */
	for( i = (0xa440>>2); i <= (0xa44f>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_8440_844F;
		g_WriteAddressValueLookupTable[i] = WriteValue_8440_844F;
	}
	/* ai reg */
	for( i = (0xa450>>2); i <= (0xa45f>>2); i++ )
	{
		g_ReadAddressLookupTable[i] = Read_8450_845F;
		g_WriteAddressValueLookupTable[i] = WriteValue_8450_845F;
	}
	/* pi reg */
	for( i = (0xa460>>2); i <= (0xa46f>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_8460_846F;
		g_WriteAddressValueLookupTable[i] = WriteValue_8460_846F;
	}
	/* ri reg */
	for( i = (0xa470>>2); i <= (0xa47f>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_8470_847F;
		g_WriteAddressValueLookupTable[i] = WriteValue_8470_847F;
	}
	/* si reg */
	for( i = (0xa480>>2); i <= (0xa48f>>2); i++ )
	{
		g_ReadAddressLookupTable[i] = Read_8480_848F;
		g_WriteAddressValueLookupTable[i] = WriteValue_8480_848F;
	}
	/* not used */
	for( i = (0xa490>>2); i <= (0xa4ff>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_Noise;
		g_WriteAddressValueLookupTable[i] = WriteValueNoise;
	}
	/* cartridge domain 2 addr 1 */
	for( i = (0xa500>>2); i <= (0xa5ff>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_Noise;
		g_WriteAddressValueLookupTable[i] = WriteValueNoise;
	}
	/* cartridge domain 1 addr 1 */
	for( i = (0xa600>>2); i <= (0xa7ff>>2); i++ )
	{
		g_ReadAddressLookupTable[i] = ReadROM;
		g_WriteAddressValueLookupTable[i] = WriteValueNoise;
	}
	/* cartridge domain 2 addr 2 */
	for( i = (0xa800>>2); i <= (0xafff>>2); i++ )
	{
		g_ReadAddressLookupTable[i] = ReadROM;
		g_WriteAddressValueLookupTable[i] = WriteValueNoise;
	}
	/* cartridge domain 1 addr 2 */
	for( i = (0xb000>>2); i <= (0xbfbf>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = ReadROM;
		g_WriteAddressValueLookupTable[i] = WriteValueNoise;
	}
	/* pif ram/rom */
	for( i = (0xbfc0>>2); i <= (0xbfcf>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = Read_9FC0_9FCF;
		g_WriteAddressValueLookupTable[i] = WriteValue_9FC0_9FCF;
	}
	/* cartridge domain 1 addr 3 */
	for( i = (0xbfd0>>2); i <= (0xbfff>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = ReadROM;
		g_WriteAddressValueLookupTable[i] = WriteValueNoise;
	}
	/* 0xc0000000 - 0xdfffffff: mapped memory */
	/* mapped */
	for( i = (0xc000>>2); i <= (0xdfff>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = ReadMapped;
		g_WriteAddressValueLookupTable[i] = WriteValueMapped;
	}
	/* 0xe0000000 - 0xffffffff: mapped memory */
	/* mapped */
	for( i = (0xe000>>2); i <= (0xffff>>2); i++ ) 
	{
		g_ReadAddressLookupTable[i] = ReadMapped;
		g_WriteAddressValueLookupTable[i] = WriteValueMapped;
	}

} /* static void init_fast_mem() */
#endif

//*****************************************************************************
//
//*****************************************************************************
void Memory_Fini(void)
{
	DPF(DEBUG_MEMORY, "Freeing Memory");

	for(u32 m = 0; m < NUM_MEM_BUFFERS; m++)
	{
		if(g_pMemoryBuffers[m] != NULL)
		{
			delete [] (u8*)(g_pMemoryBuffers[m]);
			g_pMemoryBuffers[m] = NULL;
		}
	}

	g_pu8RamBase_8000 = NULL;
	g_pu8RamBase_A000 = NULL;

	memset( g_pMemoryBuffers, 0, sizeof( g_pMemoryBuffers ) );
}

//*****************************************************************************
//
//*****************************************************************************
void Memory_Reset()
{
	u32 i;
	u32 main_mem;

	if (g_ROM.settings.ExpansionPakUsage != PAK_UNUSED)
		main_mem = MEMORY_8_MEG;
	else
		main_mem = MEMORY_4_MEG;

	DBGConsole_Msg(0, "Reseting Memory - %d MB", main_mem/(1024*1024));

	s_nNumDmaTransfers = 0;
	s_nTotalDmaTransferSize = 0;
	s_nNumSPTransfers = 0;
	s_nTotalSPTransferSize = 0;

	if(main_mem > MAXIMUM_MEM_SIZE)
	{
		DBGConsole_Msg( 0, "Memory_Reset: Can't reset with more than %dMB ram", MAXIMUM_MEM_SIZE / (1024*1024) );
		main_mem = MAXIMUM_MEM_SIZE;
	}

	// Set memory size to specified value
	// Note that we do not reallocate the memory - we always have 8Mb!
	gRamSize = main_mem;

	// Reinit the tables - this will update the RAM pointers
	Memory_InitTables();

	// Required - virtual alloc gives zeroed memory but this is also used when resetting
	// Clear memory
	for (i = 0; i < NUM_MEM_BUFFERS; i++)
	{
		if ( g_pMemoryBuffers[i] )
		{
			memset(g_pMemoryBuffers[i], 0, MemoryRegionSizes[i]);
		}
	}

	gDMAUsed = false;
}

//*****************************************************************************
//
//*****************************************************************************
void Memory_Cleanup()
{
	
}

 //*****************************************************************************
 //
 //*****************************************************************************
static void Memory_Tlb_Hack()
{
	if(RomBuffer::IsRomLoaded() && RomBuffer::IsRomAddressFixed())
	{
	   u32 tlbOffset = 0;
	   switch(g_ROM.rh.CountryID)
	   {
	   case 0x45: tlbOffset = 0x34b30; break;
	   case 0x4A: tlbOffset = 0x34b70; break;
	   case 0x50: tlbOffset = 0x329f0; break;
	   default:
		   DAEDALUS_ERROR(" GE TLB out of bound !");	// we can not handle
		   return;
	   }

	   u32 start_addr = 0x7F000000 >> 18;
	   u32 end_addr   = 0x7FFFFFFF >> 18;
	   const void *    p_rom_address( RomBuffer::GetFixedRomBaseAddress() );

	   void *pointerLookupTableEntry = (void*)(reinterpret_cast< u32 >( p_rom_address) + tlbOffset - (start_addr << 18));

	   for (u32 i = start_addr; i <= end_addr; i++)
	   {
			g_ReadAddressPointerLookupTable[i] = pointerLookupTableEntry;
	   }
	}

	g_ReadAddressPointerLookupTable[0x70000000 >> 18] = (void*)(reinterpret_cast< u32 >( g_pMemoryBuffers[MEM_RD_RAM]) - 0x70000000);
}

//*****************************************************************************
//
//*****************************************************************************
static void Memory_InitTables_ROM_PointerTable( const void * p_rom_address, u32 rom_size, u32 startPhys, u32 endPhys )
{
	u32 start_addr = startPhys >> 16;
	u32 end_addr = (Min(endPhys, startPhys + rom_size) >> 16) - 1;
	u32 i;
	void* pointerLookupTableEntry;

	start_addr += 0x8000;
	end_addr += 0x8000;

	pointerLookupTableEntry = (void*)(reinterpret_cast< u32 >( p_rom_address ) - (start_addr << 16));

	for (i = (start_addr>>2); i <= (end_addr>>2); i++)
	{
		g_ReadAddressPointerLookupTable[i] = pointerLookupTableEntry;
	}

	start_addr += 0x2000;
	end_addr += 0x2000;

	pointerLookupTableEntry = (void*)(reinterpret_cast< u32 >( p_rom_address ) - (start_addr << 16));

	for (i = (start_addr>>2); i <= (end_addr>>2); i++)
	{
		g_ReadAddressPointerLookupTable[i] = pointerLookupTableEntry;
	}
}

//*****************************************************************************
//
//*****************************************************************************
static void * Memory_GetInvalidPointerTableEntry( int entry )
{
	return (void*)(0xf0000000 - (entry << 18));
}

//*****************************************************************************
//
//*****************************************************************************
void Memory_InitTables()
{
	u32 start_addr;
	u32 end_addr;
	u32 i;

	// 0x00000000 - 0x7FFFFFFF Mapped Memory
	u32 entry = 0;

	memset(g_ReadAddressLookupTable, 0, sizeof(MemFastFunction) * 0x4000);
	memset(g_WriteAddressLookupTable, 0, sizeof(MemFastFunction) * 0x4000);
	memset(g_WriteAddressValueLookupTable, 0, sizeof(MemWriteValueFunction) * 0x4000);
	memset(InternalReadFastTable, 0, sizeof(InternalMemFastFunction) * 0x4000);
	memset(g_ReadAddressPointerLookupTable, 0, sizeof(void*) * 0x4000);
	memset(g_WriteAddressPointerLookupTable, 0, sizeof(void*) * 0x4000);

#ifdef REALITY_MEM
	init_fast_mem();

	// these values cause the result of the addition to be negative (>= 0x80000000)
	for(i = 0; i < 0x4000; i++)
	{
		g_WriteAddressPointerLookupTable[i] = g_ReadAddressPointerLookupTable[i] = Memory_GetInvalidPointerTableEntry(i);
	}

#else
	// these values cause the result of the addition to be negative (>= 0x80000000)
	for(i = 0; i < 0x4000; i++)
	{
		g_WriteAddressPointerLookupTable[i] = g_ReadAddressPointerLookupTable[i] = Memory_GetInvalidPointerTableEntry(i);
	}

	while (MemMapEntries[entry].mStartAddr != u32(~0))
	{
		start_addr = MemMapEntries[entry].mStartAddr;
		end_addr = MemMapEntries[entry].mEndAddr;

#ifndef MEMORY_DO_BOUNDS_CHECKING // this kills bounds checking
		void* readPointerLookupTableEntry = MemMapEntries[entry].ReadRegion ? (void*)(reinterpret_cast< u32 >(g_pMemoryBuffers[MemMapEntries[entry].ReadRegion]) - (start_addr << 16)) : 0;
		void* writePointerLookupTableEntry = MemMapEntries[entry].WriteRegion ? (void*)(reinterpret_cast< u32 >(g_pMemoryBuffers[MemMapEntries[entry].WriteRegion]) - (start_addr << 16)) : 0;
#endif

		for (i = (start_addr>>2); i <= (end_addr>>2); i++)
		{
			g_ReadAddressLookupTable[i]  = MemMapEntries[entry].ReadFastFunction;
			g_WriteAddressLookupTable[i] = MemMapEntries[entry].WriteFastFunction;
			g_WriteAddressValueLookupTable[i] = MemMapEntries[entry].WriteValueFunction;

#ifndef MEMORY_DO_BOUNDS_CHECKING // this kills bounds checking
			if(MemMapEntries[entry].ReadRegion)
				g_ReadAddressPointerLookupTable[i] = readPointerLookupTableEntry;

			if(MemMapEntries[entry].WriteRegion)
				g_WriteAddressPointerLookupTable[i] = writePointerLookupTableEntry;
#endif
		}

		entry++;
	}
#endif	//REALITY_MEM

	entry = 0;
	while (InternalMemMapEntries[entry].mStartAddr != u32(~0))
	{
		start_addr = InternalMemMapEntries[entry].mStartAddr;
		end_addr = InternalMemMapEntries[entry].mEndAddr;

		for (i = (start_addr>>2); i <= (end_addr>>2); i++)
		{
			InternalReadFastTable[i]  = InternalMemMapEntries[entry].InternalReadFastFunction;
		}

		entry++;
	}

	// Check the tables
	/*for (i = 0; i < 0x4000; i++)
	{
		if (g_ReadAddressLookupTable[i] == NULL ||
			g_WriteAddressLookupTable[i] == NULL ||
			g_WriteAddressValueLookupTable[i] == NULL ||
			InternalReadFastTable[i] == NULL)
		{
			char str[300];
			wsprintf(str, "Warning: 0x%08x is null", i<<14);
			MessageBox(NULL, str, g_szDaedalusName, MB_OK);
		}
	}*/


	// Set up RDRAM areas:
	u32 ram_size = gRamSize;
	MemFastFunction pReadRam;
	MemFastFunction pWriteRam;
	MemWriteValueFunction pWriteValueRam;

	InternalMemFastFunction pInternalReadRam;

	void* pointerLookupTableEntry;

	if (ram_size == MEMORY_4_MEG)
	{
		DBGConsole_Msg(0, "Initialising 4Mb main memory");
		pReadRam = Read_RAM_4Mb_8000_803F;
		pWriteRam = Write_RAM_4Mb_8000_803F;
		pWriteValueRam = WriteValue_RAM_4Mb_8000_803F;

		pInternalReadRam = InternalRead_4Mb_8000_803F;
	}
	else
	{
		DBGConsole_Msg(0, "Initialising 8Mb main memory");
		pReadRam = Read_RAM_8Mb_8000_807F;
		pWriteRam = Write_RAM_8Mb_8000_807F;
		pWriteValueRam = WriteValue_RAM_8Mb_8000_807F;

		pInternalReadRam = InternalRead_8Mb_8000_807F;
	}

	// "Real"
	start_addr = 0x8000;
	end_addr = 0x8000 + ((ram_size>>16)-1);

	pointerLookupTableEntry = (void*)(reinterpret_cast< u32 >( g_pMemoryBuffers[MEM_RD_RAM] ) - (start_addr << 16));

	for (i = (start_addr>>2); i <= (end_addr>>2); i++)
	{
		g_ReadAddressLookupTable[i]  = pReadRam;
		g_WriteAddressLookupTable[i] = pWriteRam;
		g_WriteAddressValueLookupTable[i] = pWriteValueRam;

		InternalReadFastTable[i]  = pInternalReadRam;

		g_ReadAddressPointerLookupTable[i] = pointerLookupTableEntry;
		g_WriteAddressPointerLookupTable[i] = pointerLookupTableEntry;
	}


	// Shadow
	if (ram_size == MEMORY_4_MEG)
	{
		pReadRam = Read_RAM_4Mb_A000_A03F;
		pWriteRam = Write_RAM_4Mb_A000_A03F;
		pWriteValueRam = WriteValue_RAM_4Mb_A000_A03F;

		pInternalReadRam = InternalRead_4Mb_8000_803F;
	}
	else
	{
		pReadRam = Read_RAM_8Mb_A000_A07F;
		pWriteRam = Write_RAM_8Mb_A000_A07F;
		pWriteValueRam = WriteValue_RAM_8Mb_A000_A07F;

		pInternalReadRam = InternalRead_8Mb_8000_807F;
	}


	start_addr = 0xA000;
	end_addr = 0xA000 + ((ram_size>>16)-1);

	pointerLookupTableEntry = (void*)(reinterpret_cast< u32 >( g_pMemoryBuffers[MEM_RD_RAM] ) - (start_addr << 16));

	for (i = (start_addr>>2); i <= (end_addr>>2); i++)
	{
		g_ReadAddressLookupTable[i]  = pReadRam;
		g_WriteAddressLookupTable[i] = pWriteRam;
		g_WriteAddressValueLookupTable[i] = pWriteValueRam;

		InternalReadFastTable[i]  = pInternalReadRam;

		g_ReadAddressPointerLookupTable[i] = pointerLookupTableEntry;
		g_WriteAddressPointerLookupTable[i] = pointerLookupTableEntry;
	}

	// Map ROM regions here if the address is fixed
	if(RomBuffer::IsRomLoaded() && RomBuffer::IsRomAddressFixed())
	{
		const void *	p_rom_address( RomBuffer::GetFixedRomBaseAddress() );
		u32				rom_size( RomBuffer::GetRomSize() );

		Memory_InitTables_ROM_PointerTable( p_rom_address, rom_size, PI_DOM1_ADDR1, PI_DOM2_ADDR2 );
		Memory_InitTables_ROM_PointerTable( p_rom_address, rom_size, PI_DOM1_ADDR2, 0x1FC00000 );
		Memory_InitTables_ROM_PointerTable( p_rom_address, rom_size, PI_DOM1_ADDR3, 0x20000000 );
	}

	// Hack the TLB Map per game
	if (g_ROM.GameHacks == GOLDEN_EYE) 
	{
		Memory_Tlb_Hack();
	}
}

//#undef DISPLAY_RDP_COMMANDS
//*****************************************************************************
//
//*****************************************************************************
/*
void MemoryDoDP()
{
	u32 dpc_start	= Memory_DPC_GetRegister( DPC_START_REG );
	u32 dpc_current = Memory_DPC_GetRegister( DPC_CURRENT_REG );
	u32 dpc_end		= Memory_DPC_GetRegister( DPC_END_REG );

#ifdef DISPLAY_RDP_COMMANDS
	u32 dpc_status	= Memory_DPC_GetRegister( DPC_STATUS_REG );
	u32 dpc_clock	= Memory_DPC_GetRegister( DPC_CLOCK_REG );
	u32 dpc_bufbusy	= Memory_DPC_GetRegister( DPC_BUFBUSY_REG );
	u32 dpc_pipebusy= Memory_DPC_GetRegister( DPC_PIPEBUSY_REG );
	u32 dpc_tmem	= Memory_DPC_GetRegister( DPC_TMEM_REG );
#endif

	REG64 command;

	while ( dpc_current >= dpc_start && dpc_current < dpc_end )
	{
		command._u64 = Read64Bits( PHYS_TO_K0( dpc_current ) );

#ifdef DISPLAY_RDP_COMMANDS
		const char * desc = "Unknow";
		switch ( command._u8[7] )
		{
			case	0x00:							desc = "G_RDP_SPNOOP"; break;
			case	G_RDP_NOOP:						desc = "G_RDP_NOOP"; break;
			case	G_RDP_SETCIMG:					desc = "G_RDP_SETCIMG"; break;
			case	G_RDP_SETZIMG:					desc = "G_RDP_SETZIMG"; break;
			case	G_RDP_SETTIMG:					desc = "G_RDP_SETTIMG"; break;
			case	G_RDP_SETCOMBINE:				desc = "G_RDP_SETCOMBINE"; break;
			case	G_RDP_SETENVCOLOR:				desc = "G_RDP_SETENVCOLOR"; break;
			case	G_RDP_SETPRIMCOLOR:				desc = "G_RDP_SETPRIMCOLOR"; break;
			case	G_RDP_SETBLENDCOLOR:			desc = "G_RDP_SETBLENDCOLOR"; break;
			case	G_RDP_SETFOGCOLOR:				desc = "G_RDP_SETFOGCOLOR"; break;
			case	G_RDP_SETFILLCOLOR:				desc = "G_RDP_SETFILLCOLOR"; break;
			case	G_RDP_FILLRECT:					desc = "G_RDP_FILLRECT"; break;
			case	G_RDP_SETTILE:					desc = "G_RDP_SETTILE"; break;
			case	G_RDP_LOADTILE:					desc = "G_RDP_LOADTILE"; break;
			case	G_RDP_LOADBLOCK:				desc = "G_RDP_LOADBLOCK"; break;
			case	G_RDP_SETTILESIZE:				desc = "G_RDP_SETTILESIZE"; break;
			case	G_RDP_LOADTLUT:					desc = "G_RDP_LOADTLUT"; break;
			case	G_RDP_RDPSETOTHERMODE:			desc = "G_RDP_RDPSETOTHERMODE"; break;
			case	G_RDP_SETPRIMDEPTH:				desc = "G_RDP_SETPRIMDEPTH"; break;
			case	G_RDP_SETSCISSOR:				desc = "G_RDP_SETSCISSOR"; break;
			case	G_RDP_SETCONVERT:				desc = "G_RDP_SETCONVERT"; break;
			case	G_RDP_SETKEYR:					desc = "G_RDP_SETKEYR"; break;
			case	G_RDP_SETKEYGB:					desc = "G_RDP_SETKEYGB"; break;
			case	G_RDP_RDPFULLSYNC:				desc = "G_RDP_RDPFULLSYNC"; break;
			case	G_RDP_RDPTILESYNC:				desc = "G_RDP_RDPTILESYNC"; break;
			case	G_RDP_RDPPIPESYNC:				desc = "G_RDP_RDPPIPESYNC"; break;
			case	G_RDP_RDPLOADSYNC:				desc = "G_RDP_RDPLOADSYNC"; break;
			case	G_RDP_TEXRECTFLIP:				desc = "G_RDP_TEXRECTFLIP"; break;
			case	G_RDP_TEXRECT:					desc = "G_RDP_TEXRECT"; break;
			case	G_RDP_TRI_FILL:					desc = "G_RDP_TRI_FILL"; break;
			case	G_RDP_TRI_SHADE:				desc = "G_RDP_TRI_SHADE"; break;
			case	G_RDP_TRI_TXTR:					desc = "G_RDP_TRI_TXTR"; break;
			case	G_RDP_TRI_SHADE_TXTR:			desc = "G_RDP_TRI_SHADE_TXTR"; break;
			case	G_RDP_TRI_FILL_ZBUFF:			desc = "G_RDP_TRI_FILL_ZBUFF"; break;
			case	G_RDP_TRI_SHADE_ZBUFF:			desc = "G_RDP_TRI_SHADE_ZBUFF"; break;
			case	G_RDP_TRI_TXTR_ZBUFF:			desc = "G_RDP_TRI_TXTR_ZBUFF"; break;
			case	G_RDP_TRI_SHADE_TXTR_ZBUFF:		desc = "G_TRI_SHADE_TXTR_ZBUFF"; break;
			default:							
				;
		}

		DBGConsole_Msg(0, "DP: S: %08x C: %08x E: %08x Cmd:%08x%08x %s", dpc_start, dpc_current, dpc_end, command._u32_1, command._u32_0, desc );
#endif

		if ( command._u8[7] == G_RDP_RDPFULLSYNC )
		{
			DBGConsole_Msg( 0, "FullSync: Executing DP interrupt" );

			Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_DP);
			R4300_Interrupt_UpdateCause3();
		}

		dpc_current += 8;
	}


//	DBGConsole_Msg(0, "DP: S: %08x C: %08x E: %08x", dpc_start, dpc_current, dpc_end );
//	DBGConsole_Msg(0, "    Status: %08x Clock: %08x", dpc_status, dpc_clock );
//	DBGConsole_Msg(0, "    BufBusy: %08x PipeBusy: %08x Tmem: %08x", dpc_bufbusy, dpc_pipebusy, dpc_tmem );


	Memory_DPC_SetRegister( DPC_CURRENT_REG, dpc_current );

//	Memory_DPC_ClrRegisterBits(DPC_STATUS_REG, DPC_STATUS_DMA_BUSY);
//	Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_DP);
//	R4300_Interrupt_UpdateCause3();

}
*/
//*****************************************************************************
//
//*****************************************************************************
/*void MemoryDoAI()
{
#ifdef DAEDALUS_LOG
	u32 ai_src_reg  = Memory_AI_GetRegister(AI_DRAM_ADDR_REG) & 0xfffff8;
	u32 ai_len_reg  = Memory_AI_GetRegister(AI_LEN_REG) & 0x3fff8;
	u32 ai_dacrate = Memory_AI_GetRegister(AI_DACRATE_REG) + 1;
	u32 ai_bitrate = Memory_AI_GetRegister(AI_BITRATE_REG) + 1;
	u32 ai_dma_enabled = Memory_AI_GetRegister(AI_CONTROL_REG);

	u32 frequency = (VI_NTSC_CLOCK/ai_dacrate);	// Might be divided by bytes/sample

	if (ai_dma_enabled == false)
	{
		return;
	}

	u16 *p_src = (u16 *)(g_pu8RamBase + ai_src_reg);

	DPF( DEBUG_MEMORY_AI, "AI: Playing %d bytes of data from 0x%08x", ai_len_reg, ai_src_reg );
	DPF( DEBUG_MEMORY_AI, "    Bitrate: %d. DACRate: 0x%08x, Freq: %d", ai_bitrate, ai_dacrate, frequency );
	DPF( DEBUG_MEMORY_AI, "    DMA: 0x%08x", ai_dma_enabled );

	//TODO - Ensure ranges are OK.

	// Set length to 0
	//Memory_AI_SetRegister(AI_LEN_REG, 0);
	//g_dwNextAIInterrupt = (u32)gCPUState.CPUControl[C0_COUNT] + ((VID_CLOCK*30)*ai_len_reg)/(22050*4);

	//Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_AI);
	//R4300_Interrupt_UpdateCause3();
#endif
}*/

//*****************************************************************************
//
//*****************************************************************************
void MemoryUpdateSPStatus( u32 flags )
{
#ifdef DAEDALUS_ENABLE_ASSERTS
	u32		status( Memory_SP_GetRegister( SP_STATUS_REG ) );
	DAEDALUS_ASSERT( !gRSPHLEActive || (status & SP_STATUS_HALT) == 0, "HLE active (%d), but HALT set (%08x)", gRSPHLEActive, status );
#endif

#ifdef DEBUG_SP_STATUS_REG
	DBGConsole_Msg( 0, "----------" );
	if (flags & SP_CLR_HALT)				DBGConsole_Msg( 0, "SP: Clearing Halt" );
	if (flags & SP_SET_HALT)				DBGConsole_Msg( 0, "SP: Setting Halt" );
	if (flags & SP_CLR_BROKE)				DBGConsole_Msg( 0, "SP: Clearing Broke" );
	// No SP_SET_BROKE
	if (flags & SP_CLR_INTR)				DBGConsole_Msg( 0, "SP: Clearing Interrupt" );
	if (flags & SP_SET_INTR)				DBGConsole_Msg( 0, "SP: Setting Interrupt" );
	if (flags & SP_CLR_SSTEP)				DBGConsole_Msg( 0, "SP: Clearing Single Step" );
	if (flags & SP_SET_SSTEP)				DBGConsole_Msg( 0, "SP: Setting Single Step" );
	if (flags & SP_CLR_INTR_BREAK)			DBGConsole_Msg( 0, "SP: Clearing Interrupt on break" );
	if (flags & SP_SET_INTR_BREAK)			DBGConsole_Msg( 0, "SP: Setting Interrupt on break" );
	if (flags & SP_CLR_SIG0)				DBGConsole_Msg( 0, "SP: Clearing Sig0 (Yield)" );
	if (flags & SP_SET_SIG0)				DBGConsole_Msg( 0, "SP: Setting Sig0 (Yield)" );
	if (flags & SP_CLR_SIG1)				DBGConsole_Msg( 0, "SP: Clearing Sig1 (Yielded)" );
	if (flags & SP_SET_SIG1)				DBGConsole_Msg( 0, "SP: Setting Sig1 (Yielded)" );
	if (flags & SP_CLR_SIG2)				DBGConsole_Msg( 0, "SP: Clearing Sig2 (TaskDone)" );
	if (flags & SP_SET_SIG2)				DBGConsole_Msg( 0, "SP: Setting Sig2 (TaskDone)" );
	if (flags & SP_CLR_SIG3)				DBGConsole_Msg( 0, "SP: Clearing Sig3" );
	if (flags & SP_SET_SIG3)				DBGConsole_Msg( 0, "SP: Setting Sig3" );
	if (flags & SP_CLR_SIG4)				DBGConsole_Msg( 0, "SP: Clearing Sig4" );
	if (flags & SP_SET_SIG4)				DBGConsole_Msg( 0, "SP: Setting Sig4" );
	if (flags & SP_CLR_SIG5)				DBGConsole_Msg( 0, "SP: Clearing Sig5" );
	if (flags & SP_SET_SIG5)				DBGConsole_Msg( 0, "SP: Setting Sig5" );
	if (flags & SP_CLR_SIG6)				DBGConsole_Msg( 0, "SP: Clearing Sig6" );
	if (flags & SP_SET_SIG6)				DBGConsole_Msg( 0, "SP: Setting Sig6" );
	if (flags & SP_CLR_SIG7)				DBGConsole_Msg( 0, "SP: Clearing Sig7" );
	if (flags & SP_SET_SIG7)				DBGConsole_Msg( 0, "SP: Setting Sig7" );
#endif

	// If !HALT && !BROKE

	bool start_rsp = false;
	bool stop_rsp = false;

	u32		clr_bits = 0;
	u32		set_bits = 0;

	if (flags & SP_CLR_HALT)
	{
		clr_bits |= SP_STATUS_HALT;
		DAEDALUS_ASSERT( !gRSPHLEActive, "Clearing halt with RSP HLE task running" );
		start_rsp = true;
	}

	if (flags & SP_SET_HALT)
	{
		set_bits |= SP_STATUS_HALT;
		DAEDALUS_ASSERT( !gRSPHLEActive, "Setting halt with RSP HLE task running" );
		stop_rsp = true;
	}

#if 1 //1->Pipelined, 0->Branch
	if (flags & SP_SET_INTR)				{ Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_SP); R4300_Interrupt_UpdateCause3(); }		// Shouldn't ever set this?
	else if (flags & SP_CLR_INTR)			{ Memory_MI_ClrRegisterBits(MI_INTR_REG, MI_INTR_SP); R4300_Interrupt_UpdateCause3(); }

	clr_bits |= (flags & SP_CLR_BROKE) >> 1;
	clr_bits |= (flags & SP_CLR_SSTEP);
	clr_bits |= (flags & SP_CLR_INTR_BREAK) >> 1;
	clr_bits |= (flags & SP_CLR_SIG0) >> 2;
	clr_bits |= (flags & SP_CLR_SIG1) >> 3;
	clr_bits |= (flags & SP_CLR_SIG2) >> 4;
	clr_bits |= (flags & SP_CLR_SIG3) >> 5;
	clr_bits |= (flags & SP_CLR_SIG4) >> 6;
	clr_bits |= (flags & SP_CLR_SIG5) >> 7;
	clr_bits |= (flags & SP_CLR_SIG6) >> 8;
	clr_bits |= (flags & SP_CLR_SIG7) >> 9;

	set_bits |= (flags & SP_SET_SSTEP) >> 1;
	set_bits |= (flags & SP_SET_INTR_BREAK) >> 2;
	set_bits |= (flags & SP_SET_SIG0) >> 3;
	set_bits |= (flags & SP_SET_SIG1) >> 4;
	set_bits |= (flags & SP_SET_SIG2) >> 5;
	set_bits |= (flags & SP_SET_SIG3) >> 6;
	set_bits |= (flags & SP_SET_SIG4) >> 7;
	set_bits |= (flags & SP_SET_SIG5) >> 8;
	set_bits |= (flags & SP_SET_SIG6) >> 9;
	set_bits |= (flags & SP_SET_SIG7) >> 10;

#else
	if (flags & SP_CLR_BROKE)				clr_bits |= SP_STATUS_BROKE;
	// No SP_SET_BROKE
	if (flags & SP_CLR_INTR)				{ Memory_MI_ClrRegisterBits(MI_INTR_REG, MI_INTR_SP); R4300_Interrupt_UpdateCause3(); }
	if (flags & SP_SET_INTR)				{ Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_SP); R4300_Interrupt_UpdateCause3(); }		// Shouldn't ever set this?
	if (flags & SP_CLR_SSTEP)				clr_bits |= SP_STATUS_SSTEP;
	if (flags & SP_SET_SSTEP)				set_bits |= SP_STATUS_SSTEP;
	if (flags & SP_CLR_INTR_BREAK)			clr_bits |= SP_STATUS_INTR_BREAK;
	if (flags & SP_SET_INTR_BREAK)			set_bits |= SP_STATUS_INTR_BREAK;
	if (flags & SP_CLR_SIG0)				clr_bits |= SP_STATUS_SIG0;
	if (flags & SP_SET_SIG0)				set_bits |= SP_STATUS_SIG0;
	if (flags & SP_CLR_SIG1)				clr_bits |= SP_STATUS_SIG1;
	if (flags & SP_SET_SIG1)				set_bits |= SP_STATUS_SIG1;
	if (flags & SP_CLR_SIG2)				clr_bits |= SP_STATUS_SIG2;
	if (flags & SP_SET_SIG2)				set_bits |= SP_STATUS_SIG2;
	if (flags & SP_CLR_SIG3)				clr_bits |= SP_STATUS_SIG3;
	if (flags & SP_SET_SIG3)				set_bits |= SP_STATUS_SIG3;
	if (flags & SP_CLR_SIG4)				clr_bits |= SP_STATUS_SIG4;
	if (flags & SP_SET_SIG4)				set_bits |= SP_STATUS_SIG4;
	if (flags & SP_CLR_SIG5)				clr_bits |= SP_STATUS_SIG5;
	if (flags & SP_SET_SIG5)				set_bits |= SP_STATUS_SIG5;
	if (flags & SP_CLR_SIG6)				clr_bits |= SP_STATUS_SIG6;
	if (flags & SP_SET_SIG6)				set_bits |= SP_STATUS_SIG6;
	if (flags & SP_CLR_SIG7)				clr_bits |= SP_STATUS_SIG7;
	if (flags & SP_SET_SIG7)				set_bits |= SP_STATUS_SIG7;
#endif

#ifdef DAEDAULUS_ENABLEASSERTS
	u32 new_status = Memory_SP_SetRegisterBits( SP_STATUS_REG, ~clr_bits, set_bits );
#else
	Memory_SP_SetRegisterBits( SP_STATUS_REG, ~clr_bits, set_bits );
#endif
	//
	// We execute the task here, after we've written to the SP status register.
	//
	if( start_rsp )
	{
#ifdef DAEDAULUS_ENABLEASSERTS
		DAEDALUS_ASSERT( !gRSPHLEActive, "RSP HLE already active. Status was %08x, now %08x", status, new_status );
#endif

		// Check for tasks whenever the RSP is started
		RSP_HLE_ProcessTask();
	}
	else if ( stop_rsp )
	{
		DAEDALUS_ASSERT( !RSP_IsRunningHLE(), "Stopping RSP while HLE task still running. Not good!" );
	}
}

#undef DISPLAY_DPC_WRITES
//*****************************************************************************
//
//*****************************************************************************
void MemoryUpdateDP( u32 flags )
{
	// Ignore address, as this is only called with DPC_STATUS_REG write
	// DBGConsole_Msg(0, "DP Status: 0x%08x", flags);

	u32 dpc_status  =  Memory_DPC_GetRegister(DPC_STATUS_REG);

	if (flags & DPC_CLR_XBUS_DMEM_DMA)			dpc_status &= ~DPC_STATUS_XBUS_DMEM_DMA;
	if (flags & DPC_SET_XBUS_DMEM_DMA)			dpc_status |= DPC_STATUS_XBUS_DMEM_DMA;
	if (flags & DPC_CLR_FREEZE)					dpc_status &= ~DPC_STATUS_FREEZE;
	//if (flags & DPC_SET_FREEZE)				dpc_status |= DPC_STATUS_FREEZE;	// Thanks Lemmy! <= what's wrong with this? ~ Salvy
	if (flags & DPC_CLR_FLUSH)					dpc_status &= ~DPC_STATUS_FLUSH;
	if (flags & DPC_SET_FLUSH)					dpc_status |= DPC_STATUS_FLUSH;

	// These should be ignored ! - Salvy
	/*if (flags & DPC_CLR_TMEM_CTR)				Memory_DPC_SetRegister(DPC_TMEM_REG, 0);
	if (flags & DPC_CLR_PIPE_CTR)				Memory_DPC_SetRegister(DPC_PIPEBUSY_REG, 0);
	if (flags & DPC_CLR_CMD_CTR)				Memory_DPC_SetRegister(DPC_BUFBUSY_REG, 0);
	if (flags & DPC_CLR_CLOCK_CTR)				Memory_DPC_SetRegister(DPC_CLOCK_REG, 0);*/

#ifdef DISPLAY_DPC_WRITES
	if ( flags & DPC_CLR_XBUS_DMEM_DMA )		DBGConsole_Msg( 0, "DPC_CLR_XBUS_DMEM_DMA" );
	if ( flags & DPC_SET_XBUS_DMEM_DMA )		DBGConsole_Msg( 0, "DPC_SET_XBUS_DMEM_DMA" );
	if ( flags & DPC_CLR_FREEZE )				DBGConsole_Msg( 0, "DPC_CLR_FREEZE" );
	if ( flags & DPC_SET_FREEZE )				DBGConsole_Msg( 0, "DPC_SET_FREEZE" );
	if ( flags & DPC_CLR_FLUSH )				DBGConsole_Msg( 0, "DPC_CLR_FLUSH" );
	if ( flags & DPC_SET_FLUSH )				DBGConsole_Msg( 0, "DPC_SET_FLUSH" );
	if ( flags & DPC_CLR_TMEM_CTR )				DBGConsole_Msg( 0, "DPC_CLR_TMEM_CTR" );
	if ( flags & DPC_CLR_PIPE_CTR )				DBGConsole_Msg( 0, "DPC_CLR_PIPE_CTR" );
	if ( flags & DPC_CLR_CMD_CTR )				DBGConsole_Msg( 0, "DPC_CLR_CMD_CTR" );
	if ( flags & DPC_CLR_CLOCK_CTR )			DBGConsole_Msg( 0, "DPC_CLR_CLOCK_CTR" );

	DBGConsole_Msg( 0, "Modified DPC_STATUS_REG - now %08x", dpc_status );
#endif
	
	// Do we need this?
	// Write back the value
	//Memory_DPC_SetRegister(DPC_STATUS_REG, dpc_status);

}

//*****************************************************************************
//
//*****************************************************************************
void MemoryUpdateMI( u32 value )
{
	u32 mi_intr_mask_reg = Memory_MI_GetRegister(MI_INTR_MASK_REG);
	u32 mi_intr_reg		 = Memory_MI_GetRegister(MI_INTR_REG);

	//bool interrupts_live_before((mi_intr_mask_reg & mi_intr_reg) != 0);

#if 1 //1->pipelined, 0->branch version //Corn
	u32 SET_MASK;
	u32 CLR_MASK;

	CLR_MASK  = (value & MI_INTR_MASK_CLR_SP) >> 0;
	SET_MASK  = (value & MI_INTR_MASK_SET_SP) >> 1;
	CLR_MASK |= (value & MI_INTR_MASK_CLR_SI) >> 1;
	SET_MASK |= (value & MI_INTR_MASK_SET_SI) >> 2;
	CLR_MASK |= (value & MI_INTR_MASK_CLR_AI) >> 2;
	SET_MASK |= (value & MI_INTR_MASK_SET_AI) >> 3;
	CLR_MASK |= (value & MI_INTR_MASK_CLR_VI) >> 3;
	SET_MASK |= (value & MI_INTR_MASK_SET_VI) >> 4;
	CLR_MASK |= (value & MI_INTR_MASK_CLR_PI) >> 4;
	SET_MASK |= (value & MI_INTR_MASK_SET_PI) >> 5;
	CLR_MASK |= (value & MI_INTR_MASK_CLR_DP) >> 5;
	SET_MASK |= (value & MI_INTR_MASK_SET_DP) >> 6;

	mi_intr_mask_reg &= ~CLR_MASK;
	mi_intr_mask_reg |= SET_MASK;

#else
	if((value & MI_INTR_MASK_SET_SP)) mi_intr_mask_reg |= MI_INTR_MASK_SP;
	else if((value & MI_INTR_MASK_CLR_SP)) mi_intr_mask_reg &= ~MI_INTR_MASK_SP;

	if((value & MI_INTR_MASK_SET_SI)) mi_intr_mask_reg |= MI_INTR_MASK_SI;
	else if((value & MI_INTR_MASK_CLR_SI)) mi_intr_mask_reg &= ~MI_INTR_MASK_SI;
 
	if((value & MI_INTR_MASK_SET_AI)) mi_intr_mask_reg |= MI_INTR_MASK_AI;
    else if((value & MI_INTR_MASK_CLR_AI)) mi_intr_mask_reg &= ~MI_INTR_MASK_AI;

	if((value & MI_INTR_MASK_SET_VI)) mi_intr_mask_reg |= MI_INTR_MASK_VI;
    else if((value & MI_INTR_MASK_CLR_VI)) mi_intr_mask_reg &= ~MI_INTR_MASK_VI;

	if((value & MI_INTR_MASK_SET_PI)) mi_intr_mask_reg |= MI_INTR_MASK_PI;
    else if((value & MI_INTR_MASK_CLR_PI)) mi_intr_mask_reg &= ~MI_INTR_MASK_PI;

	if((value & MI_INTR_MASK_SET_DP)) mi_intr_mask_reg |= MI_INTR_MASK_DP;
    else if((value & MI_INTR_MASK_CLR_DP)) mi_intr_mask_reg &= ~MI_INTR_MASK_DP;
#endif

	if(mi_intr_mask_reg & 0x0000003F & mi_intr_reg)
	{
		Memory_MI_SetRegister( MI_INTR_REG, mi_intr_reg );	
		Memory_MI_SetRegister( MI_INTR_MASK_REG, mi_intr_mask_reg );

		R4300_Interrupt_UpdateCause3();
	}
}

//*****************************************************************************
//
//*****************************************************************************
void MemoryModeRegMI( u32 value )
{
	u32 mi_mode_reg = Memory_MI_GetRegister(MI_MODE_REG);

	if(value & MI_SET_RDRAM) mi_mode_reg |= MI_MODE_RDRAM;
	else if(value & MI_CLR_RDRAM) mi_mode_reg &= ~MI_MODE_RDRAM;

	if(value & MI_SET_INIT) mi_mode_reg |= MI_MODE_INIT;
    else if(value & MI_CLR_INIT) mi_mode_reg &= ~MI_MODE_INIT;

	if(value & MI_SET_EBUS) mi_mode_reg |= MI_MODE_EBUS;
    else if(value & MI_CLR_EBUS) mi_mode_reg &= ~MI_MODE_EBUS;

	if (value & MI_CLR_DP_INTR)
	{ 
		//Only MI_CLR_DP_INTR needs to clear our interrupts
		//
		Memory_MI_ClrRegisterBits(MI_INTR_REG, MI_INTR_DP); 
		R4300_Interrupt_UpdateCause3(); 
	}

}

#ifdef DAEDALUS_LOG
//*****************************************************************************
//
//*****************************************************************************
static void DisplayVIControlInfo( u32 control_reg )
{
	DPF( DEBUG_VI, "VI Control:", (control_reg & VI_CTRL_GAMMA_DITHER_ON) ? "On" : "Off" );

	const char *szMode = "Disabled/Unknown";
	     if ((control_reg & VI_CTRL_TYPE_16) == VI_CTRL_TYPE_16) szMode = "16-bit";
	else if ((control_reg & VI_CTRL_TYPE_32) == VI_CTRL_TYPE_32) szMode = "32-bit";

	DPF( DEBUG_VI, "         ColorDepth: %s", szMode );
	DPF( DEBUG_VI, "         Gamma Dither: %s", (control_reg & VI_CTRL_GAMMA_DITHER_ON) ? "On" : "Off" );
	DPF( DEBUG_VI, "         Gamma: %s", (control_reg & VI_CTRL_GAMMA_ON) ? "On" : "Off" );
	DPF( DEBUG_VI, "         Divot: %s", (control_reg & VI_CTRL_DIVOT_ON) ? "On" : "Off" );
	DPF( DEBUG_VI, "         Interlace: %s", (control_reg & VI_CTRL_SERRATE_ON) ? "On" : "Off" );
	DPF( DEBUG_VI, "         AAMask: 0x%02x", (control_reg&VI_CTRL_ANTIALIAS_MASK)>>8 );
	DPF( DEBUG_VI, "         DitherFilter: %s", (control_reg & VI_CTRL_DITHER_FILTER_ON) ? "On" : "Off" );

}
#endif

//*****************************************************************************
//
//*****************************************************************************
void MemoryUpdatePI( u32 value )
{
	if (value & PI_STATUS_RESET)
	{
		// What to do when is busy?

		DPF( DEBUG_PI, "PI: Resetting Status. PC: 0x%08x", gCPUState.CurrentPC );
		// Reset PIC here
		Memory_PI_SetRegister(PI_STATUS_REG, 0);
	}
	if (value & PI_STATUS_CLR_INTR)
	{
		DPF( DEBUG_PI, "PI: Clearing interrupt flag. PC: 0x%08x", gCPUState.CurrentPC );
		Memory_MI_ClrRegisterBits(MI_INTR_REG, MI_INTR_PI);
		R4300_Interrupt_UpdateCause3();
	}
}

//*****************************************************************************
//	The PIF control byte has been written to - process this command
//*****************************************************************************
void MemoryUpdatePIF()
{
	u8 * pPIFRom = (u8 *)g_pMemoryBuffers[MEM_PIF_RAM];
	u8 * pPIFRam = (u8 *)g_pMemoryBuffers[MEM_PIF_RAM] + 0x7C0;

	u8 command = pPIFRam[ 0x3F^U8_TWIDDLE ];

	switch ( command )
	{
	case 0x01:		// Standard block
		DBGConsole_Msg( 0, "[GStandard execute block control value: 0x%02x", command );
		break;
	case 0x08:
		pPIFRam[ 0x3F^U8_TWIDDLE ] = 0x00; 

		Memory_SI_SetRegisterBits(SI_STATUS_REG, SI_STATUS_INTERRUPT);
		Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_SI);
		R4300_Interrupt_UpdateCause3();
		DBGConsole_Msg( 0, "[GSI Interrupt control value: 0x%02x", command );
		break;
	case 0x10:
		memset( pPIFRom, 0, 0x7C0 );
		DBGConsole_Msg( 0, "[GClear ROM control value: 0x%02x", command );
		break;
	case 0x30:
		pPIFRam[ 0x3F^U8_TWIDDLE ] = 0x80;		
		DBGConsole_Msg( 0, "[GSet 0x80 control value: 0x%02x", command );
		break;
	case 0xC0:
		memset( pPIFRam, 0, 0x40);
		DBGConsole_Msg( 0, "[GClear PIF Ram control value: 0x%02x", command );
		break;
	default:
		DBGConsole_Msg( 0, "[GUnknown control value: 0x%02x", command );
		break;
	}

}
//*****************************************************************************
//
//*****************************************************************************
