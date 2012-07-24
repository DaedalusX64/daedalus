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

#ifdef DAEDALUS_DEBUG_CONSOLE
	if (g_DaedalusConfig.WarnMemoryErrors)
	{
		CPU_Halt("Illegal Memory Access");
		DBGConsole_Msg(0, "Illegal Memory Access - Tried to Read From 0x%08x (PC: 0x%08x)", address, gCPUState.CurrentPC);
	}
#endif
	return g_pMemoryBuffers[MEM_UNUSED];

}

//*****************************************************************************
//
//*****************************************************************************
static void * Read_Noise( u32 address )
{
	//CPUHalt();
	//DBGConsole_Msg(0, "Reading noise (0x%08x)", address);
#ifdef DAEDALUS_DEBUG_CONSOLE
	static bool bWarned = false;
	if (!bWarned)
	{
		DBGConsole_Msg(0, "Reading noise (0x%08x) - sizing memory?", address);
		bWarned = true;
	}
#endif
	*(u32*)((u8 *)g_pMemoryBuffers[MEM_UNUSED] + 0) = pspFastRand();
	*(u32*)((u8 *)g_pMemoryBuffers[MEM_UNUSED] + 4) = pspFastRand();

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
		if (missing)
		{
			R4300_Exception_TLB(address, EXC_RMISS, UT_VEC);
			return g_pMemoryBuffers[MEM_UNUSED];
		}
		else
		{
			// should be invalid
			R4300_Exception_TLB(address, EXC_RMISS, E_VEC);
			return g_pMemoryBuffers[MEM_UNUSED];
		}
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
static void *Read_RAM_4Mb_8000_803F( u32 address )
{
	return g_pu8RamBase_8000 + address;
}

//*****************************************************************************
//
//*****************************************************************************
static void *Read_RAM_8Mb_8000_807F( u32 address )
{
	return g_pu8RamBase_8000 + address;
}

//*****************************************************************************
//
//*****************************************************************************
static void *Read_RAM_4Mb_A000_A03F( u32 address )
{
	return g_pu8RamBase_A000 + address;
}

//*****************************************************************************
//
//*****************************************************************************
static void *Read_RAM_8Mb_A000_A07F( u32 address )
{
	return g_pu8RamBase_A000 + address;
}


//*****************************************************************************
//
//*****************************************************************************
static void *Read_83F0_83F0( u32 address )
{

	// 0x83F0 0000 to 0x83FF FFFF  RDRAM registers
	if (MEMORY_BOUNDS_CHECKING((address&0x1FFFFFFF) < 0x04000000))
	{
		DPF( DEBUG_MEMORY_RDRAM_REG, "Reading from MEM_RD_REG: 0x%08x", address );

		//DBGConsole_Msg(0, "Reading from MEM_RD_REG: 0x%08x", address);
		return (u8 *)g_pMemoryBuffers[MEM_RD_REG0] + (address & 0xFF);
	}
	else
	{
		return ReadInvalid(address);
	}
}

//*****************************************************************************
//
//*****************************************************************************
static void *Read_8400_8400( u32 address )
{
	// 0x0400 0000 to 0x0400 FFFF  SP registers
	if (MEMORY_BOUNDS_CHECKING((address&0x1FFFFFFF) < 0x4002000))
	{
		DPF( DEBUG_MEMORY_SP_IMEM, "Reading from SP_MEM: 0x%08x", address );
		return (u8 *)g_pMemoryBuffers[MEM_SP_MEM] + (address & 0x1FFF);
	}
	else
	{
		return ReadInvalid(address);
	}
}


//*****************************************************************************
//
//*****************************************************************************
static void *Read_8404_8404( u32 address )
{
	if (MEMORY_BOUNDS_CHECKING((address&0x1FFFFFFF) < 0x04040020))
	{
		DPF( DEBUG_MEMORY_SP_REG, "Reading from SP_REG: 0x%08x", address );
		return (u8 *)g_pMemoryBuffers[MEM_SP_REG] + (address & 0xFF);
	} else {
		return ReadInvalid(address);
	}

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

#undef DISPLAY_DPC_READS
//*****************************************************************************
//
//*****************************************************************************
static void *Read_8410_841F( u32 address )
{
	// 0x0410 0000 to 0x041F FFFF DP Command Registers
	if (MEMORY_BOUNDS_CHECKING((address&0x1FFFFFFF) < 0x04100020))
	{
		DPF( DEBUG_MEMORY_DP, "Reading from DP_COMMAND_REG: 0x%08x", address );
		//DBGConsole_Msg(0, "Reading from DP_COMMAND_REG: 0x%08x", address);

		u32 offset = address & 0xFF;

 #ifdef DISPLAY_DPC_READS
		u32 value = *(u32*)((u8 *)g_pMemoryBuffers[MEM_DPC_REG] + offset);

		switch (DPC_BASE_REG + offset)
		{
			case DPC_START_REG:
				DBGConsole_Msg( 0, "Read from [WDPC_START_REG] 0x%08x", value );
				break;
			case DPC_END_REG:
				DBGConsole_Msg( 0, "Read from [WDPC_END_REG] 0x%08x", value );
				break;
			case DPC_CURRENT_REG:
			//	DBGConsole_Msg( 0, "Read from [WDPC_CURRENT_REG 0x%08x", value );
				break;
			case DPC_STATUS_REG:
				{
					//DPC_STATUS_XBUS_DMEM_DMA			0x0001
					//DPC_STATUS_DMA_BUSY				0x0100
				//	CPU_Halt("Status");
				//	DBGConsole_Msg( 0, "Read from [WDPC_STATUS_REG] 0x%08x", value );

					/*u32 flags = 0;

					DBGConsole_Msg( 0, "start, end, current: %08x %08x %08x", 
						Memory_DPC_GetRegister( DPC_START_REG ), Memory_DPC_GetRegister( DPC_END_REG ), Memory_DPC_GetRegister( DPC_CURRENT_REG ) );

					if ( Memory_DPC_GetRegister( DPC_START_REG ) != 0 )
						flags |= DPC_STATUS_START_VALID;

					if ( Memory_DPC_GetRegister( DPC_START_REG ) < Memory_DPC_GetRegister( DPC_END_REG ) )
						flags |= DPC_STATUS_END_VALID;

					Memory_DPC_SetRegisterBits( DPC_STATUS_REG, flags );*/
				}

				break;
			case DPC_CLOCK_REG:
				DBGConsole_Msg( 0, "Read from [WDPC_CLOCK_REG] 0x%08x", value );
				break;
			case DPC_BUFBUSY_REG: 
				DBGConsole_Msg( 0, "Read from [WDPC_BUFBUSY_REG:] 0x%08x", value );
				break;
			case DPC_PIPEBUSY_REG:
				DBGConsole_Msg( 0, "Read from [WDPC_PIPEBUSY_REG] 0x%08x", value );
				break;
			case DPC_TMEM_REG:
				DBGConsole_Msg( 0, "Read from [WDPC_TMEM_REG] 0x%08x", value );
				break;
			default:
				DBGConsole_Msg( 0, "Read from [WUnknown DPC reg]!" );
				break;
		}
#endif
		return (u8 *)g_pMemoryBuffers[MEM_DPC_REG] + offset;
	}
	else
	{
		DBGConsole_Msg( 0, "Read from DP Command Registers is unhandled (0x%08x, PC: 0x%08x)", address, gCPUState.CurrentPC );
		
		return ReadInvalid(address);
	}
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
	if (MEMORY_BOUNDS_CHECKING((address&0x1FFFFFFF) <= MI_LAST_REG))
	{
		DPF( DEBUG_MEMORY_MI, "Reading from MI Registers: 0x%08x", address );
		//if ((address & 0xFF) == 0x08)
		//	DBGConsole_Msg(0, "Reading from MI REG: 0x%08x", address);
		return (u8 *)g_pMemoryBuffers[MEM_MI_REG] + (address & 0xFF);
	}
	else
	{
		return ReadInvalid(address);
	}
}

//*****************************************************************************
// 0x0440 0000 to 0x044F FFFF Video Interface (VI) Registers
//*****************************************************************************
static void *Read_8440_844F( u32 address )
{
	if (MEMORY_BOUNDS_CHECKING((address&0x1FFFFFFF) <= VI_LAST_REG))
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
	else
	{
		return ReadInvalid(address);
	}
}

#undef DISPLAY_AI_READS
//*****************************************************************************
// 0x0450 0000 to 0x045F FFFF Audio Interface (AI) Registers
//*****************************************************************************
//static u32 g_dwAIBufferFullness = 0;
static void *Read_8450_845F( u32 address )
{
	if (MEMORY_BOUNDS_CHECKING((address&0x1FFFFFFF) <= AI_LAST_REG))
	{
		u32 offset = address & 0xFF;

#ifdef DISPLAY_AI_READS
		switch (AI_BASE_REG + offset)
		{
		case AI_DRAM_ADDR_REG:
			break;

		case AI_LEN_REG:
			break;

		case AI_CONTROL_REG:
			break;

		case AI_STATUS_REG:
			//Memory_AI_SetRegister(AI_STATUS_REG, 0);	// Not needed, and could cause possible issues with some games.
			break;

		case AI_DACRATE_REG:
			break;
		case AI_BITRATE_REG:
			break;

		}
#endif
		DPF( DEBUG_MEMORY_AI, "Reading from AI Registers: 0x%08x", address );
		return (u8 *)g_pMemoryBuffers[MEM_AI_REG] + offset;
	}
	else
	{
		return ReadInvalid(address);
	}
}


//*****************************************************************************
// 0x0460 0000 to 0x046F FFFF Peripheral Interface (PI) Registers
//*****************************************************************************
static void *Read_8460_846F( u32 address )
{
	if (MEMORY_BOUNDS_CHECKING((address&0x1FFFFFFF) <= PI_LAST_REG))
	{
		DPF( DEBUG_MEMORY_PI, "Reading from MEM_PI_REG: 0x%08x", address );
		return (u8 *)g_pMemoryBuffers[MEM_PI_REG] + (address & 0xFF);
	}
	else
	{
		return ReadInvalid(address);
	} 
}


//*****************************************************************************
// 0x0470 0000 to 0x047F FFFF RDRAM Interface (RI) Registers
//*****************************************************************************
static void *Read_8470_847F( u32 address )
{
	if (MEMORY_BOUNDS_CHECKING((address&0x1FFFFFFF) <= RI_LAST_REG))
	{
		DPF( DEBUG_MEMORY_RI, "Reading from MEM_RI_REG: 0x%08x", address );
		return (u8 *)g_pMemoryBuffers[MEM_RI_REG] + (address & 0xFF);
	}
	else
	{
		return ReadInvalid(address);
	} 
}

//*****************************************************************************
// 0x0480 0000 to 0x048F FFFF Serial Interface (SI) Registers
//*****************************************************************************
static void *Read_8480_848F( u32 address )
{
	if (MEMORY_BOUNDS_CHECKING((address&0x1FFFFFFF) <= SI_LAST_REG))
	{
#ifdef DAEDALUS_DEBUG_CONSOLE
		u32 offset = address & 0xFF;

		if (SI_BASE_REG + offset == SI_STATUS_REG)
		{
			// Init SI_STATUS_INTERRUPT bit!
			bool mi_si_int_set = (Memory_MI_GetRegister(MI_INTR_REG) & MI_INTR_SI) ? true : false;
			bool si_status_int_set = (Memory_SI_GetRegister(SI_STATUS_REG) & SI_STATUS_INTERRUPT) ? true : false;

			if ( mi_si_int_set != si_status_int_set )
			{
				DBGConsole_Msg(0, "SI_STATUS in inconsistant state! %08x" );
			}
		}
#endif
		DPF( DEBUG_MEMORY_SI, "Reading from MEM_SI_REG: 0x%08x", address );
		return (u8 *)g_pMemoryBuffers[MEM_SI_REG] + (address & 0xFF);
	}
	else
	{

		DBGConsole_Msg(0, "Read from SI Registers is unhandled (0x%08x, PC: 0x%08x)",
			address, gCPUState.CurrentPC);

		return ReadInvalid(address);
	} 
}

//*****************************************************************************
//
//*****************************************************************************
/*

#define	K0_TO_K1(x)	((u32)(x)|0xA0000000)	// kseg0 to kseg1 
#define	K1_TO_K0(x)	((u32)(x)&0x9FFFFFFF)	// kseg1 to kseg0 
#define	K0_TO_PHYS(x)	((u32)(x)&0x1FFFFFFF)	// kseg0 to physical 
#define	K1_TO_PHYS(x)	((u32)(x)&0x1FFFFFFF)	// kseg1 to physical 
#define	KDM_TO_PHYS(x)	((u32)(x)&0x1FFFFFFF)	// direct mapped to physical 
#define	PHYS_TO_K0(x)	((u32)(x)|0x80000000)	// physical to kseg0 
#define	PHYS_TO_K1(x)	((u32)(x)|0xA0000000)	// physical to kseg1 

#define PI_DOM2_ADDR1		0x05000000	// to 0x05FFFFFF 
#define PI_DOM1_ADDR1		0x06000000	// to 0x07FFFFFF 
#define PI_DOM2_ADDR2		0x08000000	// to 0x0FFFFFFF 
#define PI_DOM1_ADDR2		0x10000000	// to 0x1FBFFFFF 
#define PI_DOM1_ADDR3		0x1FD00000	// to 0x7FFFFFFF

0x0500_0000 .. 0x05ff_ffff	cartridge domain 2
0x0600_0000 .. 0x07ff_ffff	cartridge domain 1
0x0800_0000 .. 0x0fff_ffff	cartridge domain 2
0x1000_0000 .. 0x1fbf_ffff	cartridge domain 1

//0xa8010000 FlashROM
*/


static void * ReadROM( u32 address )
{
	void * p_mem( NULL );
	//0x10000000 | 0xA0000000 = 0xB0000000

	// Few things read from (0xbff00000)
	// Brunswick bowling

	// 0xb0ffb000

	u32 physical_addr;
	u32 offset;

	physical_addr = K0_TO_PHYS(address);		// & 0x1FFFFFFF;

	if (physical_addr >= PI_DOM2_ADDR1 && physical_addr < PI_DOM1_ADDR1)
	{
		//DBGConsole_Msg(0, "[GRead from SRAM (addr1)] 0x%08x", address);
		offset = physical_addr - PI_DOM2_ADDR1;
		if (offset < MemoryRegionSizes[MEM_SAVE])
		{
			p_mem = (u8 *)g_pMemoryBuffers[MEM_SAVE] + offset;
		}
	}
	else if (physical_addr >= PI_DOM1_ADDR1 && physical_addr < PI_DOM2_ADDR2)
	{
		//DBGConsole_Msg(0, "[GRead from Cart (addr1)] 0x%08x", address);
		offset = physical_addr - PI_DOM1_ADDR1;
		p_mem = RomBuffer::GetAddressRaw( offset );
	}
	else if (physical_addr >= PI_DOM2_ADDR2 && physical_addr < PI_DOM1_ADDR2)
	{
		//DBGConsole_Msg(0, "[GRead from FLASHRAM (addr2)] 0x%08x", address);
		offset = physical_addr - PI_DOM2_ADDR2;
		if (g_ROM.settings.SaveType != SAVE_TYPE_FLASH)
		{
			if (offset < MemoryRegionSizes[MEM_SAVE])
			{
				p_mem = (u8 *)g_pMemoryBuffers[MEM_SAVE] + offset;
			}
		}
		else
		{
			p_mem = (u8 *)&FlashStatus[0];
		}
	}
	else if (physical_addr >= PI_DOM1_ADDR2 && physical_addr < 0x1FBFFFFF)
	{
		//DBGConsole_Msg(0, "[GRead from Cart (addr2)] 0x%08x", address);
		offset = physical_addr - PI_DOM1_ADDR2;
		p_mem = RomBuffer::GetAddressRaw( offset );
#ifdef DAEDALUS_DEBUG_CONSOLE
		if( p_mem == NULL )
		{
			DBGConsole_Msg(0, "[GRead from Cart (addr2) out of range! (0x%08x)] 0x%08x",
			address, RomBuffer::GetRomSize());
		}
#endif
	}
	else if (physical_addr >= PI_DOM1_ADDR3 && physical_addr < 0x7FFFFFFF)
	{
		//DBGConsole_Msg(0, "[GRead from Cart (addr3)] 0x%08x", address);
		offset = physical_addr - PI_DOM1_ADDR3;
		p_mem = RomBuffer::GetAddressRaw( offset );
	}

	if( p_mem == NULL )
	{
		DBGConsole_Msg(0, "[WWarning, attempting to read from invalid Cart address (0x%08x)]", address);
		p_mem = Read_Noise(address);
	}

	return p_mem;
}

//*****************************************************************************
// 0x1FC0 0000 to 0x1FC0 07BF PIF Boot ROM
// 0x1FC0 07C0 to 0x1FC0 07FF PIF RAM
//*****************************************************************************
static void * Read_9FC0_9FCF( u32 address )
{
	u32 offset = address & 0x0FFF;
	//u32 cic = 0x91;

	//if( MEMORY_BOUNDS_CHECKING((address&0x1FFFFFFF) <= PIF_RAM_END) )
	//if( MEMORY_BOUNDS_CHECKING((address&0x1FFFFFFF) <= PIF_ROM_END) )

	DPF( DEBUG_MEMORY_PIF, "Reading from MEM_PIF: 0x%08x", address );
	return (u8 *)g_pMemoryBuffers[MEM_PIF_RAM] + offset;
}
