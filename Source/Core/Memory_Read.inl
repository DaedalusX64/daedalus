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


//*****************************************************************************
//
//*****************************************************************************
static void * ReadInvalid( u32 address )
{
	DPF( DEBUG_MEMORY, "Illegal Memory Access - Tried to Read From 0x%08x (PC: 0x%08x)", address, gCPUState.CurrentPC );

	if(address == 0xa5000508)
	{
		DBGConsole_Msg(0, "Reading noise (0x%08x) - sizing memory?", address);	
		*(u32*)((u8 *)g_pMemoryBuffers[MEM_UNUSED]) = ~0;
	}
	else
	{
		DBGConsole_Msg(0, "Illegal Memory Access - Tried to Read From 0x%08x (PC: 0x%08x)", address, gCPUState.CurrentPC);
		*(u32*)((u8 *)g_pMemoryBuffers[MEM_UNUSED]) = 0;
	}

	return g_pMemoryBuffers[MEM_UNUSED];
}

//*****************************************************************************
//
//*****************************************************************************
static void * ReadMapped( u32 address )
{
	bool missing;

#ifdef DAEDALUS_PROFILE_EXECUTION
	gTLBReadHit++;
#endif

	u32 physical_addr = TLBEntry::Translate(address, missing);
	if (physical_addr != 0)
	{
		return g_pu8RamBase + (physical_addr & 0x007FFFFF);
	}
	else
	{
		// should be invalid
		R4300_Exception_TLB(address, EXC_RMISS, missing ? UT_VEC : E_VEC);
		return g_pMemoryBuffers[MEM_UNUSED];
	}	
}

//*****************************************************************************
//
//*****************************************************************************

// Main memory
/*static void *Read_RAM_4Mb_8000_803F( u32 address )
{
	return g_pu8RamBase + (address & 0x003FFFFF);
}

static void *Read_RAM_8Mb_8000_807F( u32 address )
{
	return g_pu8RamBase + (address & 0x007FFFFF);
}*/

//*****************************************************************************
//
//*****************************************************************************
/*static void *Read_RAM_4Mb_8000_803F( u32 address )
{
	return g_pu8RamBase_8000 + address;
}

static void *Read_RAM_8Mb_8000_807F( u32 address )
{
	return g_pu8RamBase_8000 + address;
}

static void *Read_RAM_4Mb_A000_A03F( u32 address )
{
	return g_pu8RamBase_A000 + address;
}

static void *Read_RAM_8Mb_A000_A07F( u32 address )
{
	return g_pu8RamBase_A000 + address;
}
*/
//*****************************************************************************
//
//*****************************************************************************
static void *Read_8000_807F( u32 address )
{
	// Note: Mask is slighty different when EPAK isn't used 0x003FFFFF
	return (u8 *)g_pMemoryBuffers[MEM_RD_RAM] + (address & 0x007FFFFF);
}

//*****************************************************************************
//
//*****************************************************************************
static void *Read_83F0_83F0( u32 address )
{
	// 0x83F0 0000 to 0x83FF FFFF  RDRAM registers
	DPF( DEBUG_MEMORY_RDRAM_REG, "Reading from MEM_RD_REG: 0x%08x", address );
	return (u8 *)g_pMemoryBuffers[MEM_RD_REG0] + (address & 0xFF);
}

//*****************************************************************************
//
//*****************************************************************************
static void *Read_8400_8400( u32 address )
{
	// 0x0400 0000 to 0x0400 FFFF  SP registers
	DPF( DEBUG_MEMORY_SP_IMEM, "Reading from SP_MEM: 0x%08x", address );
	return (u8 *)g_pMemoryBuffers[MEM_SP_MEM] + (address & 0x1FFF);
}


//*****************************************************************************
//
//*****************************************************************************
static void *Read_8404_8404( u32 address )
{
	DPF( DEBUG_MEMORY_SP_REG, "Reading from SP_REG: 0x%08x", address );
	return (u8 *)g_pMemoryBuffers[MEM_SP_REG] + (address & 0xFF);
}

//*****************************************************************************
//
//*****************************************************************************
static void *Read_8408_8408( u32 address )
{
	// We don't support LLE RSP emulation in the PSP, so is ok to skip this reg -Salvy
	//
	return ReadInvalid(address);

	/*u32 offset = (address&0x1FFFFFFF) - 0x04080000;

	// 0x04080000 to 0x04080003  SP_PC_REG
	if (offset < 0x04)
	{
		DPF( DEBUG_MEMORY_SP_REG, "Reading from SP_PC_REG: 0x%08x", address );
		return (u8 *)&gRSPState.CurrentPC + offset;

	}
	// 0x04080004 to 0x04080007  SP_IBIST_REG
	else if (MEMORY_BOUNDS_CHECKING(offset < 0x08))
	{
		DPF( DEBUG_MEMORY_SP_REG, "Reading from SP_IBIST_REG: 0x%08x", address );
		DBGConsole_Msg( 0, "Reading from SP_IBIST_REG: 0x%08x", address );
		return ReadInvalid(address);
	}
	else
	{
		return ReadInvalid(address);
	}*/
}

//*****************************************************************************
//
//*****************************************************************************
static void *Read_8410_841F( u32 address )
{
	// 0x0410 0000 to 0x041F FFFF DP Command Registers
	DPF( DEBUG_MEMORY_DP, "Reading from DP_COMMAND_REG: 0x%08x", address );
	return (u8 *)g_pMemoryBuffers[MEM_DPC_REG] + (address & 0xFF);
}

//*****************************************************************************
//
//*****************************************************************************
static void *Read_8420_842F( u32 address )
{
	// 0x0420 0000 to 0x042F FFFF DP Span Registers
	DBGConsole_Msg( 0, "Read from DP Span Registers is unhandled (0x%08x, PC: 0x%08x)", address, gCPUState.CurrentPC );
	return ReadInvalid(address);
}

//*****************************************************************************
// 0x0430 0000 to 0x043F FFFF MIPS Interface (MI) Registers
//*****************************************************************************
static void *Read_8430_843F( u32 address )
{	
	DPF( DEBUG_MEMORY_MI, "Reading from MI Registers: 0x%08x", address );
	return (u8 *)g_pMemoryBuffers[MEM_MI_REG] + (address & 0xFF);
}

//*****************************************************************************
// 0x0440 0000 to 0x044F FFFF Video Interface (VI) Registers
//*****************************************************************************
static void *Read_8440_844F( u32 address )
{
	DPF( DEBUG_MEMORY_VI, "Reading from MEM_VI_REG: 0x%08x", address );
	if ((address & 0x1FFFFFFF) == VI_CURRENT_REG)
	{
		//u64 count_to_vbl = (VID_CLOCK-1) - (g_qwNextVBL - gCPUState.CPUControl[C0_COUNT]);
		//vi_pos = (u32)((count_to_vbl*512)/VID_CLOCK);
		u32 vi_pos = Memory_VI_GetRegister(VI_CURRENT_REG);

		vi_pos += 2;
		if (vi_pos >= 512)
			vi_pos = 0;

		//DBGConsole_Msg(0, "Reading vi pos: %d", vi_pos);
		Memory_VI_SetRegister(VI_CURRENT_REG, vi_pos);
	}
	return VI_REG_ADDRESS(address & 0x1FFFFFFF);
}

//*****************************************************************************
// 0x0450 0000 to 0x045F FFFF Audio Interface (AI) Registers
//*****************************************************************************
static void *Read_8450_845F( u32 address )
{
	u32 offset = address & 0xFF;
	DPF( DEBUG_MEMORY_AI, "Reading from AI Registers: 0x%08x", address );
	return (u8 *)g_pMemoryBuffers[MEM_AI_REG] + offset;
}

//*****************************************************************************
// 0x0460 0000 to 0x046F FFFF Peripheral Interface (PI) Registers
//*****************************************************************************
static void *Read_8460_846F( u32 address )
{
	DPF( DEBUG_MEMORY_PI, "Reading from MEM_PI_REG: 0x%08x", address );
	return (u8 *)g_pMemoryBuffers[MEM_PI_REG] + (address & 0xFF);
}


//*****************************************************************************
// 0x0470 0000 to 0x047F FFFF RDRAM Interface (RI) Registers
//*****************************************************************************
static void *Read_8470_847F( u32 address )
{
	DPF( DEBUG_MEMORY_RI, "Reading from MEM_RI_REG: 0x%08x", address );
	return (u8 *)g_pMemoryBuffers[MEM_RI_REG] + (address & 0xFF);
}

//*****************************************************************************
// 0x0480 0000 to 0x048F FFFF Serial Interface (SI) Registers
//*****************************************************************************
static void *Read_8480_848F( u32 address )
{
	DPF( DEBUG_MEMORY_SI, "Reading from MEM_SI_REG: 0x%08x", address );
	return (u8 *)g_pMemoryBuffers[MEM_SI_REG] + (address & 0xFF);
}

//*****************************************************************************
//
//*****************************************************************************
static void * ReadFlashRam( u32 address )
{
	u32 offset = address & 0xFF;
	if( g_ROM.settings.SaveType == SAVE_TYPE_FLASH && offset == 0 )
	{
		return (u8 *)&FlashStatus[0];
	}

	DBGConsole_Msg(0, "[GRead from FlashRam (0x%08x) is unhandled", address);
	return ReadInvalid(address);
}

//*****************************************************************************
//
//*****************************************************************************
static void * ReadROM( u32 address )
{
	if (g_RomWritten)
    {
        g_RomWritten = false;
		return (u8 *)&g_pWriteRom[0];
    }
	return RomBuffer::GetAddressRaw( (address & 0x03FFFFFF) );
}

//*****************************************************************************
// 0x1FC0 0000 to 0x1FC0 07BF PIF Boot ROM (Ignored)
// 0x1FC0 07C0 to 0x1FC0 07FF PIF RAM
//*****************************************************************************
static void * Read_9FC0_9FCF( u32 address )
{
	DAEDALUS_ASSERT(!(address - 0x7C0 & ~0x3F), "Read to PIF RAM (0x%08x) is invalid", address);

	DPF( DEBUG_MEMORY_PIF, "Reading from MEM_PIF: 0x%08x", address );
	// ToDO: SWAP_PIF
	return (u8 *)g_pMemoryBuffers[MEM_PIF_RAM] + (address & 0x3F);
}
