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

// Various stuff to map an address onto the correct memory region

#include "stdafx.h"
#include "Memory.h"

#include "CPU.h"
#include "DMA.h"
#include "RSP.h"
#include "Interrupt.h"
#include "ROM.h"
#include "ROMBuffer.h"

#include "Debug/Dump.h"		// Dump_GetSaveDirectory()
#include "Debug/DebugLog.h"
#include "Debug/DBGConsole.h"

#include "OSHLE/ultra_R4300.h"

#include "Plugins/AudioPlugin.h"
#include "Plugins/GraphicsPlugin.h"

#include "ConfigOptions.h"

static const u32	MAXIMUM_MEM_SIZE( MEMORY_8_MEG );

#undef min

#ifdef DAEDALUS_LOG
static void DisplayVIControlInfo( u32 control_reg );
#endif

void MemoryUpdateSPStatus( u32 flags );
static void MemoryUpdateDP( u32 value );
static void MemoryModeRegMI( u32 value );
static void MemoryUpdateMI( u32 value );
static void MemoryUpdatePI( u32 value );
static void MemoryUpdatePIF();

static void Memory_InitTables();
//*****************************************************************************
//
//*****************************************************************************
u32 MemoryRegionSizes[NUM_MEM_BUFFERS] =
{
//	8*1024,				// Allocate 8k bytes - a bit excessive but some of the internal functions assume it's there! (Don't need this much really)?
	0x04,				// This seems enough (Salvy)
	MAXIMUM_MEM_SIZE,	// RD_RAM
	0x2000,				// SP_MEM

	0x40,				// PIF_RAM

	//1*1024*1024,		// RD_REG	(Don't need this much really)?
	0x30,				// RD_REG0
	//0x30,				// RD_REG1	(Unused)
	//0x30,				// RD_REG2	(Unused)

	0x20,				// SP_REG
	0x08,				// SP_PC_REG
	0x20,				// DPC_REG
	//0x10,				// DPS_REG	(Unhandled)
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

// ROM write support
u32	  g_pWriteRom[2];
bool  g_RomWritten;
// Ram base, offset by 0x80000000 and 0xa0000000
//u8 * g_pu8RamBase_8000 = NULL;
//u8 * g_pu8RamBase_A000 = NULL;

// Flash RAM Support
extern u32 FlashStatus[2];
void Flash_DoCommand(u32);

//*****************************************************************************
//
//*****************************************************************************
#include "Memory_Read.inl"
#include "Memory_WriteValue.inl"
#include "Memory_ReadInternal.inl"


//*****************************************************************************
//
//*****************************************************************************
#ifndef DAEDALUS_ALIGN_REGISTERS
MemFuncRead  g_MemoryLookupTableRead[0x4000];
MemFuncWrite g_MemoryLookupTableWrite[0x4000];
#ifndef DAEDALUS_SILENT
InternalMemFastFunction InternalReadFastTable[0x4000];
#endif
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
	//u32 count = 0;
	for(u32 m = 0; m < NUM_MEM_BUFFERS; m++)
	{
		u32		region_size( MemoryRegionSizes[m] );
		// Skip zero sized areas. An example of this is the cart rom
		if(region_size > 0)
		{
			//count+=region_size;
			g_pMemoryBuffers[m] = new u8[region_size];
			//g_pMemoryBuffers[m] = Memory_AllocRegion(region_size);

			if(g_pMemoryBuffers[m] == NULL)
			{
				return false;
			}

			// Necessary?
			memset(g_pMemoryBuffers[m], 0, region_size);
			/*if (region_size < 0x100) // dirty, check if this is a I/O range
			{
				g_pMemoryBuffers[m] = MAKE_UNCACHED_PTR(g_pMemoryBuffers[m]);
			}*/
		}
	}
	//printf("%d bytes used of memory\n",count);
	g_RomWritten= false;
	//g_pu8RamBase_8000 = ((u8*)g_pMemoryBuffers[MEM_RD_RAM]) - 0x80000000;
	//g_pu8RamBase_A000 = ((u8*)g_pMemoryBuffers[MEM_RD_RAM]) - 0xa0000000;
	//g_pu8RamBase_A000 = ((u8*)MAKE_UNCACHED_PTR(g_pMemoryBuffers[MEM_RD_RAM])) - 0xa0000000;


	Memory_InitTables();

	return true;
}

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

//	g_pu8RamBase_8000 = NULL;
//	g_pu8RamBase_A000 = NULL;

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

	//s_nNumDmaTransfers = 0;
	//s_nTotalDmaTransferSize = 0;
	//s_nNumSPTransfers = 0;
	//s_nTotalSPTransferSize = 0;

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
static void Memory_Tlb_Hack(const void *p_rom_address)
{
	if(p_rom_address != NULL)
	{
	   u32 offset = 0;
	   switch(g_ROM.rh.CountryID)
	   {
	   case 0x45: offset = 0x34b30; break;
	   case 0x4A: offset = 0x34b70; break;
	   case 0x50: offset = 0x329f0; break;
	   default:
		   offset = 0x34b30;	// we can not handle
		   return;
	   }

	   u32 start_addr = 0x7F000000 >> 18;
	   u32 end_addr   = 0x7FFFFFFF >> 18;

	   u8 *pRead = (u8*)(reinterpret_cast< u32 >( p_rom_address) + offset - (start_addr << 18));

	   for (u32 i = start_addr; i <= end_addr; i++)
	   {
			g_MemoryLookupTableRead[i].pRead = pRead;
	   }
	}

	g_MemoryLookupTableRead[0x70000000 >> 18].pRead = (u8*)(reinterpret_cast< u32 >( g_pMemoryBuffers[MEM_RD_RAM]) - 0x70000000);
}

//*****************************************************************************
//
//*****************************************************************************
void Memory_InitFunc(u32 start, u32 size, const void * ReadRegion, const void * WriteRegion, mReadFunction ReadFunc, mWriteFunction WriteFunc)
{
	u32	start_addr = (start >> 18);
	u32	end_addr   = ((start + size - 1) >> 18);

	while(start_addr <= end_addr)
	{
		//printf("0x%08x\n",start_addr|(0x8000 >> 2));
		g_MemoryLookupTableRead[start_addr|(0x8000>>2)].ReadFunc= ReadFunc;
		g_MemoryLookupTableWrite[start_addr|(0x8000>>2)].WriteFunc = WriteFunc;

		g_MemoryLookupTableRead[start_addr|(0xA000>>2)].ReadFunc= ReadFunc;
		g_MemoryLookupTableWrite[start_addr|(0xA000>>2)].WriteFunc = WriteFunc;

		if(ReadRegion)
		{
			g_MemoryLookupTableRead[start_addr|(0x8000>>2)].pRead = (u8*)(reinterpret_cast< u32 >(ReadRegion) - (((start>>16)|0x8000) << 16));
			g_MemoryLookupTableRead[start_addr|(0xA000>>2)].pRead = (u8*)(reinterpret_cast< u32 >(ReadRegion) - (((start>>16)|0xA000) << 16));
		}

		if(WriteRegion)
		{
			g_MemoryLookupTableWrite[start_addr|(0x8000>>2)].pWrite = (u8*)(reinterpret_cast< u32 >(WriteRegion) - (((start>>16)|0x8000) << 16));
			g_MemoryLookupTableWrite[start_addr|(0xA000>>2)].pWrite = (u8*)(reinterpret_cast< u32 >(WriteRegion) - (((start>>16)|0xA000) << 16));
		}
		
		start_addr++;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void Memory_InitTables()
{
	u32 i;

	memset(g_MemoryLookupTableRead, 0, sizeof(MemFuncRead) * 0x4000);
	memset(g_MemoryLookupTableWrite, 0, sizeof(MemFuncWrite) * 0x4000);

	for(i = 0; i < (0x10000 >> 2); i++)
	{
		g_MemoryLookupTableRead[i].pRead = NULL;
		g_MemoryLookupTableWrite[i].pWrite = NULL;
	}

	// 0x00000000 - 0x7FFFFFFF Mapped Memory
	for(i = 0; i < (0x8000 >> 2); i++)
	{
		g_MemoryLookupTableRead[i].ReadFunc		= ReadMapped;
		g_MemoryLookupTableWrite[i].WriteFunc	= WriteValueMapped;
	}

	// Invalidate all entries, mapped regions are untouched (0x00000000 - 0x7FFFFFFF, 0xC0000000 - 0x10000000 )
	for(i = (0x8000 >> 2); i < (0xC000 >> 2); i++)
	{
		g_MemoryLookupTableRead[i].ReadFunc		= ReadInvalid;
		g_MemoryLookupTableWrite[i].WriteFunc	= WriteValueInvalid;
	}

	// 0xC0000000 - 0x10000000 Mapped Memory
	for(i = (0xC000 >> 2); i < (0x10000 >> 2); i++)
	{
		g_MemoryLookupTableRead[i].ReadFunc		= ReadMapped;
		g_MemoryLookupTableWrite[i].WriteFunc	= WriteValueMapped;
	}

	
	bool RomBaseKnown = RomBuffer::IsRomLoaded() && RomBuffer::IsRomAddressFixed();

	// This returns NULL if Rom isn't loaded or Rom base isn't fixed
	const void *	rom_address( RomBaseKnown ? RomBuffer::GetFixedRomBaseAddress() : NULL );

	u32				rom_size( RomBuffer::GetRomSize() );
	u32				ram_size( gRamSize );

	DBGConsole_Msg(0, "Initialising %s main memory", (ram_size == MEMORY_8_MEG) ? "8Mb" : "4Mb");

	// Init RDRAM
	// By default we init with EPAK (8Mb)
	Memory_InitFunc
	( 
		MEMORY_START_RDRAM, 
		MEMORY_SIZE_RDRAM_DEFAULT,
		MEMORY_RDRAM,
		MEMORY_RDRAM,
		Read_8000_807F, 
		WriteValue_8000_807F 
	);

	// Need to turn off the EPAK
	if (ram_size != MEMORY_8_MEG)
	{
		Memory_InitFunc
		( 
			MEMORY_START_EXRDRAM,
			MEMORY_SIZE_EXRDRAM, 
			NULL,
			NULL,
			ReadInvalid, 
			WriteValueInvalid 
		);
	}

	// RDRAM Reg
	Memory_InitFunc
	( 
		MEMORY_START_RAMREGS0, 
		MEMORY_SIZE_RAMREGS0, 
		MEMORY_RAMREGS0,	
		MEMORY_RAMREGS0,
		Read_83F0_83F0, 
		WriteValue_83F0_83F0 
	);


	// DMEM/IMEM
	Memory_InitFunc
	( 
		MEMORY_START_SPMEM, 
		MEMORY_SIZE_SPMEM, 
		MEMORY_SPMEM, 
		MEMORY_SPMEM,
		Read_8400_8400, 
		WriteValue_8400_8400
	);

	// SP Reg
	Memory_InitFunc
	( 
		MEMORY_START_SPREG_1, 
		MEMORY_SIZE_SPREG_1,
		MEMORY_SPREG_1, 
		NULL,
		Read_8404_8404,
		WriteValue_8404_8404 
	);

	// SP PC/OBOST
	Memory_InitFunc
	(
		MEMORY_START_SPREG_2,
		MEMORY_SIZE_SPREG_2, 
		MEMORY_SPREG_2,
		MEMORY_SPREG_2,
		Read_8408_8408,
		WriteValue_8408_8408 
	);
	// DPC Reg
	Memory_InitFunc
	( 
		MEMORY_START_DPC,
		MEMORY_SIZE_DPC, 
		MEMORY_DPC,
		NULL,
		Read_8410_841F, 
		WriteValue_8410_841F
	);	

	// DPS Reg
	Memory_InitFunc
	( 
		MEMORY_START_DPS, 
		MEMORY_SIZE_DPS, 
		NULL,
		NULL,
		Read_8420_842F,
		WriteValue_8420_842F 
	);	
	
	// MI reg
	Memory_InitFunc
	(
		MEMORY_START_MI,
		MEMORY_SIZE_MI, 
		MEMORY_MI, 
		NULL,
		Read_8430_843F, 
		WriteValue_8430_843F
	);	

	// VI Reg
	Memory_InitFunc
	( 
		MEMORY_START_VI, 
		MEMORY_SIZE_VI, 
		NULL,
		NULL,
		Read_8440_844F, 
		WriteValue_8440_844F
	);

	// AI Reg
	Memory_InitFunc
	( 
		MEMORY_START_AI, 
		MEMORY_SIZE_AI, 
		MEMORY_AI,
		NULL,
		Read_8450_845F, 
		WriteValue_8450_845F 
	);

	// PI Reg
	Memory_InitFunc
	( 
		MEMORY_START_PI,
		MEMORY_SIZE_PI,
		MEMORY_PI, 
		NULL,
		Read_8460_846F,
		WriteValue_8460_846F 
	);

	// RI Reg
	Memory_InitFunc
	(
		MEMORY_START_RI, 
		MEMORY_SIZE_RI,
		MEMORY_RI, 
		MEMORY_RI,
		Read_8470_847F, 
		WriteValue_8470_847F
	);

	// SI Reg
	Memory_InitFunc
	( 
		MEMORY_START_SI, 
		MEMORY_SIZE_SI, 
		MEMORY_SI,
		NULL,
		Read_8480_848F, 
		WriteValue_8480_848F
	);

	// Ignore C1A1 and C2A1
	// As a matter of fact handling C2A1 breaks Pokemon Stadium 1 and F-Zero U

	// Cartridge Domain 2 Address 1 (SRAM)
	/*Memory_InitFunc
	( 
		MEMORY_START_C2A1, 
		MEMORY_SIZE_C2A1, 
		NULL,
		NULL,
		ReadInvalid, 
		WriteValueInvalid
	);*/
	
	// Cartridge Domain 1 Address 1 (SRAM)
	/*Memory_InitFunc
	( 
		MEMORY_START_C1A1, 
		MEMORY_SIZE_C1A1,
		NULL,
		NULL,
		ReadInvalid, 
		WriteValueInvalid
	);*/

	// GIO reg (basically in the same segment as C1A1..)
	/*Memory_InitFunc
	( 
		MEMORY_START_GIO,
		MEMORY_SIZE_GIO_REG,
		NULL,
		NULL,
		ReadInvalid, 
		WriteValueInvalid
	);
	*/

	// PIF Reg
	Memory_InitFunc
	(
		MEMORY_START_PIF, 
		MEMORY_SIZE_PIF, 
		NULL,
		NULL,
		Read_9FC0_9FCF, 
		WriteValue_9FC0_9FCF
	);

	// Cartridge Domain 2 Address 2 (FlashRam) 
	// ToDo : FlashRam Read is at 0x800, and Flash Ram Write at 0x801
	Memory_InitFunc
	( 
		MEMORY_START_C2A2, 
		MEMORY_SIZE_C2A2,
		NULL,
		NULL,
		ReadFlashRam,   
		WriteValue_FlashRam
	);

	// Cartridge Domain 1 Address 2 (Rom)
	// Map ROM region if the address is fixed (Note Reads to Rom are very rare, actual speedup is unlikely)
	// Nb: We can do a hack if ROM address isn't fixed by allocating the first 8K bytes of the ROM, but yea I don't think is worth..
	Memory_InitFunc
	(
		MEMORY_START_ROM_IMAGE, 
		rom_size, 
		rom_address,
		NULL,
		ReadROM,	 
		WriteValue_ROM
	);

	// Hack the TLB Map per game
	if (g_ROM.GameHacks == GOLDEN_EYE) 
	{
		Memory_Tlb_Hack( rom_address );
	}

	// Debug only
#ifndef DAEDALUS_SILENT
	Memory_InitInternalTables( ram_size );
#endif
}

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
	else if (flags & SP_SET_HALT)
	{
		set_bits |= SP_STATUS_HALT;
		DAEDALUS_ASSERT( !gRSPHLEActive, "Setting halt with RSP HLE task running" );
		stop_rsp = true;
	}

	if (flags & SP_SET_INTR)	// Shouldn't ever set this?		
	{ 
		Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_SP); 
		R4300_Interrupt_UpdateCause3();
	}		
	else if (flags & SP_CLR_INTR)			
	{ 
		Memory_MI_ClrRegisterBits(MI_INTR_REG, MI_INTR_SP);
		R4300_Interrupt_UpdateCause3();
	}

	clr_bits |= (flags & SP_CLR_BROKE) >> 1;
	clr_bits |= ( flags & SP_CLR_SSTEP);
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
		DAEDALUS_ASSERT( (status & SP_STATUS_BROKE) == 0 ),"Unexpected RSP HLE status %08x", status );
#endif

		// Check for tasks whenever the RSP is started
		RSP_HLE_ProcessTask();
	}
	else if ( stop_rsp )
	{
		// As we handle all RSP via HLE, nothing to do here.
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

	// ToDO : Avoid branching
	if (flags & DPC_CLR_XBUS_DMEM_DMA)			dpc_status &= ~DPC_STATUS_XBUS_DMEM_DMA;
	if (flags & DPC_SET_XBUS_DMEM_DMA)			dpc_status |= DPC_STATUS_XBUS_DMEM_DMA;
	if (flags & DPC_CLR_FREEZE)					dpc_status &= ~DPC_STATUS_FREEZE;
	//if (flags & DPC_SET_FREEZE)				dpc_status |= DPC_STATUS_FREEZE;	// Thanks Lemmy! <= what's wrong with this? ~ Salvy
	if (flags & DPC_CLR_FLUSH)					dpc_status &= ~DPC_STATUS_FLUSH;
	if (flags & DPC_SET_FLUSH)					dpc_status |= DPC_STATUS_FLUSH;

	/*
	if (flags & DPC_CLR_TMEM_CTR)				Memory_DPC_SetRegister(DPC_TMEM_REG, 0);
	if (flags & DPC_CLR_PIPE_CTR)				Memory_DPC_SetRegister(DPC_PIPEBUSY_REG, 0);
	if (flags & DPC_CLR_CMD_CTR)				Memory_DPC_SetRegister(DPC_BUFBUSY_REG, 0);
	if (flags & DPC_CLR_CLOCK_CTR)				Memory_DPC_SetRegister(DPC_CLOCK_REG, 0);
	*/

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
	
	// Write back the value
	Memory_DPC_SetRegister(DPC_STATUS_REG, dpc_status);

}

//*****************************************************************************
//
//*****************************************************************************
void MemoryUpdateMI( u32 value )
{
	u32 mi_intr_mask_reg = Memory_MI_GetRegister(MI_INTR_MASK_REG);
	u32 mi_intr_reg		 = Memory_MI_GetRegister(MI_INTR_REG);

	u32 clr;
	u32 set;

	// From Corn - nicer way to avoid branching
	clr  = (value & MI_INTR_MASK_CLR_SP) >> 0;
	set  = (value & MI_INTR_MASK_SET_SP) >> 1;
	clr |= (value & MI_INTR_MASK_CLR_SI) >> 1;
	set |= (value & MI_INTR_MASK_SET_SI) >> 2;
	clr |= (value & MI_INTR_MASK_CLR_AI) >> 2;
	set |= (value & MI_INTR_MASK_SET_AI) >> 3;
	clr |= (value & MI_INTR_MASK_CLR_VI) >> 3;
	set |= (value & MI_INTR_MASK_SET_VI) >> 4;
	clr |= (value & MI_INTR_MASK_CLR_PI) >> 4;
	set |= (value & MI_INTR_MASK_SET_PI) >> 5;
	clr |= (value & MI_INTR_MASK_CLR_DP) >> 5;
	set |= (value & MI_INTR_MASK_SET_DP) >> 6;

	mi_intr_mask_reg &= ~clr;
	mi_intr_mask_reg |= set;

	Memory_MI_SetRegister( MI_INTR_MASK_REG, mi_intr_mask_reg );

	// Check if any interrupts are enabled now, and immediately trigger an interrupt
	//if(mi_intr_mask_reg & 0x0000003F & mi_intr_reg)
	if(mi_intr_mask_reg & mi_intr_reg)
	{
		R4300_Interrupt_UpdateCause3();
	}
}

//*****************************************************************************
//
//*****************************************************************************
void MemoryModeRegMI( u32 value )
{
	u32 mi_mode_reg = Memory_MI_GetRegister(MI_MODE_REG);

	// ToDO : Avoid branching
	if(value & MI_SET_RDRAM) 
		mi_mode_reg |= MI_MODE_RDRAM;
	else if(value & MI_CLR_RDRAM) 
		mi_mode_reg &= ~MI_MODE_RDRAM;

	if(value & MI_SET_INIT)
		mi_mode_reg |= MI_MODE_INIT;
    else if(value & MI_CLR_INIT)
		mi_mode_reg &= ~MI_MODE_INIT;

	if(value & MI_SET_EBUS) 
		mi_mode_reg |= MI_MODE_EBUS;
    else if(value & MI_CLR_EBUS) 
		mi_mode_reg &= ~MI_MODE_EBUS;

	Memory_MI_SetRegister( MI_MODE_REG, mi_mode_reg );

	if (value & MI_CLR_DP_INTR)
	{ 
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
	u8 * pPIFRam = (u8 *)g_pMemoryBuffers[MEM_PIF_RAM];
	u8 command = pPIFRam[ 0x3F ^ U8_TWIDDLE];
	if( command == 0x08 )
	{
		pPIFRam[ 0x3F ^ U8_TWIDDLE ] = 0x00; 

		DBGConsole_Msg( 0, "[GSI Interrupt control value: 0x%02x", command );
		Memory_SI_SetRegisterBits(SI_STATUS_REG, SI_STATUS_INTERRUPT);
		Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_SI);
		R4300_Interrupt_UpdateCause3();
	}
	else
	{
		DBGConsole_Msg( 0, "[GUnknown control value: 0x%02x", command );
	}

}
