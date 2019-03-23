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

static void * ReadInvalid( u32 address )
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DPF( DEBUG_MEMORY, "Illegal Memory Access - Tried to Read From 0x%08x (PC: 0x%08x)", address, gCPUState.CurrentPC );
	DBGConsole_Msg(0, "Illegal Memory Access - Tried to Read From 0x%08x (PC: 0x%08x)", address, gCPUState.CurrentPC);
	#endif

	u8* temp {(u8 *)g_pMemoryBuffers[MEM_UNUSED]};

	// 0xa5000508 is 64DD region.. set -1 fixes F-Zero U
	*(u32*)(temp) = (address == 0xa5000508) ? ~0 : 0;
	return temp;
}

static void * ReadMapped( u32 address )
{
	bool missing;

#ifdef DAEDALUS_PROFILE_EXECUTION
	gTLBReadHit++;
#endif

	u32 physical_addr {TLBEntry::Translate(address, missing)};
	if (physical_addr != 0)
	{
		return g_pu8RamBase + (physical_addr & 0x007FFFFF);
	}
	else
	{
		R4300_Exception_TLB(address, EXC_RMISS, missing ? UT_VEC : E_VEC);
		return g_pMemoryBuffers[MEM_UNUSED];
	}
}

static void * Read_8000_807F( u32 address )
{
	// Note: Mask is slighty different when EPAK isn't used 0x003FFFFF
	return (u8 *)g_pMemoryBuffers[MEM_RD_RAM] + (address & 0x007FFFFF);
}

// 0x83F0 0000 to 0x83FF FFFF  RDRAM registers
static void * Read_83F0_83F0( u32 address )
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DPF( DEBUG_MEMORY_RDRAM_REG, "Reading from MEM_RD_REG: 0x%08x", address );
	#endif
	return (u8 *)g_pMemoryBuffers[MEM_RD_REG0] + (address & 0xFF);
}

// 0x0400 0000 to 0x0400 FFFF  SP registers
static void * Read_8400_8400( u32 address )
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DPF( DEBUG_MEMORY_SP_IMEM, "Reading from SP_MEM: 0x%08x", address );
	#endif
	return (u8 *)g_pMemoryBuffers[MEM_SP_MEM] + (address & 0x1FFF);
}

static void * Read_8404_8404( u32 address )
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DPF( DEBUG_MEMORY_SP_REG, "Reading from SP_REG: 0x%08x", address );
	#endif
	return (u8 *)g_pMemoryBuffers[MEM_SP_REG] + (address & 0xFF);
}

static void * Read_8408_8408( u32 address )
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DPF( DEBUG_MEMORY_SP_REG, "Reading from SP_PC_REG: 0x%08x", address );
	#endif
	return (u8 *)g_pMemoryBuffers[MEM_SP_PC_REG] + (address & 0xFF);
}

// 0x0410 0000 to 0x041F FFFF DP Command Registers
static void * Read_8410_841F( u32 address )
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DPF( DEBUG_MEMORY_DP, "Reading from DP_COMMAND_REG: 0x%08x", address );
	#endif
	return (u8 *)g_pMemoryBuffers[MEM_DPC_REG] + (address & 0xFF);
}

// 0x0420 0000 to 0x042F FFFF DP Span Registers
static void * Read_8420_842F( u32 address )
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DBGConsole_Msg( 0, "Read from DP Span Registers is unhandled (0x%08x, PC: 0x%08x)", address, gCPUState.CurrentPC );
	#endif
	return ReadInvalid(address);
}

// 0x0430 0000 to 0x043F FFFF MIPS Interface (MI) Registers
static void * Read_8430_843F( u32 address )
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DPF( DEBUG_MEMORY_MI, "Reading from MI Registers: 0x%08x", address );
	#endif
	return (u8 *)g_pMemoryBuffers[MEM_MI_REG] + (address & 0xFF);
}

// 0x0440 0000 to 0x044F FFFF Video Interface (VI) Registers
static void * Read_8440_844F( u32 address )
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DPF( DEBUG_MEMORY_VI, "Reading from MEM_VI_REG: 0x%08x", address );
	#endif
	u32 offset {address & 0xFF};
	u8* temp = (u8 *)g_pMemoryBuffers[MEM_VI_REG] + offset;

	if (offset == 0x10)	// VI_CURRENT_REG
	{
		//u64 count_to_vbl = (VID_CLOCK-1) - (g_qwNextVBL - gCPUState.CPUControl[C0_COUNT]);
		//vi_pos = (u32)((count_to_vbl*512)/VID_CLOCK);
		u32 vi_pos {Memory_VI_GetRegister(VI_CURRENT_REG)};
		vi_pos = (vi_pos + 2) % 512;

		//DBGConsole_Msg(0, "Reading vi pos: %d", vi_pos);
		*(u32*)temp = vi_pos;
	}
	return temp;
}

// 0x0450 0000 to 0x045F FFFF Audio Interface (AI) Registers
static void * Read_8450_845F( u32 address )
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DPF( DEBUG_MEMORY_AI, "Reading from AI Registers: 0x%08x", address );
	#endif
	return (u8 *)g_pMemoryBuffers[MEM_AI_REG] + (address & 0xFF);
}

// 0x0460 0000 to 0x046F FFFF Peripheral Interface (PI) Registers
static void * Read_8460_846F( u32 address )
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DPF( DEBUG_MEMORY_PI, "Reading from MEM_PI_REG: 0x%08x", address );
	#endif
	return (u8 *)g_pMemoryBuffers[MEM_PI_REG] + (address & 0xFF);
}


// 0x0470 0000 to 0x047F FFFF RDRAM Interface (RI) Registers
static void * Read_8470_847F( u32 address )
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DPF( DEBUG_MEMORY_RI, "Reading from MEM_RI_REG: 0x%08x", address );
	#endif
	return (u8 *)g_pMemoryBuffers[MEM_RI_REG] + (address & 0xFF);
}

// 0x0480 0000 to 0x048F FFFF Serial Interface (SI) Registers
static void * Read_8480_848F( u32 address )
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DPF( DEBUG_MEMORY_SI, "Reading from MEM_SI_REG: 0x%08x", address );
	#endif
	return (u8 *)g_pMemoryBuffers[MEM_SI_REG] + (address & 0xFF);
}

static void * ReadFlashRam( u32 address )
{
	u32 offset {address & 0xFF};
	if (g_ROM.settings.SaveType == SAVE_TYPE_FLASH && offset == 0)
	{
		if ((address&0x1FFFFFFF) == FLASHRAM_READ_ADDR)
			return (u8 *)&FlashStatus[0];
	}
#ifdef DAEDALUS_DEBUG_CONSOLE
	DBGConsole_Msg(0, "[GRead from FlashRam (0x%08x) is invalid", address);
	#endif
	return ReadInvalid(address);
}

static void * ReadROM( u32 address )
{
	if (g_RomWritten)
	{
		g_RomWritten = false;
		return (u8 *)&g_pWriteRom;
	}
	return RomBuffer::GetAddressRaw( (address & 0x03FFFFFF) );
}

// 0x1FC0 0000 to 0x1FC0 07BF PIF Boot ROM (Ignored)
// 0x1FC0 07C0 to 0x1FC0 07FF PIF RAM
static void * Read_9FC0_9FCF( u32 address )
{
	u32 offset {address & 0x0FFF};
	u32 pif_ram_offset {address & 0x3F};

	// Reading PIF ROM or outside PIF RAM
	if ((offset < 0x7C0) || (offset > 0x7FF))
	{
		#ifdef DAEDALUS_DEBUG_CONSOLE
		DBGConsole_Msg(0, "[GRead from PIF (0x%08x) is invalid", address);
		#endif
		return g_pMemoryBuffers[MEM_UNUSED];
	}
#ifdef DAEDALUS_DEBUG_CONSOLE
	DPF( DEBUG_MEMORY_PIF, "Reading from MEM_PIF: 0x%08x", address );
	#endif
	return (u8 *)g_pMemoryBuffers[MEM_PIF_RAM] + pif_ram_offset;
}
