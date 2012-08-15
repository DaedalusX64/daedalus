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
static void WriteValueInvalid( u32 address, u32 value )
{
	DPF( DEBUG_MEMORY, "Illegal Memory Access Tried to Write To 0x%08x PC: 0x%08x", address, gCPUState.CurrentPC );
	DBGConsole_Msg(0, "Illegal Memory Access: Tried to Write To 0x%08x (PC: 0x%08x)", address, gCPUState.CurrentPC);
}

//*****************************************************************************
//
//*****************************************************************************
static void WriteValueMapped( u32 address, u32 value )
{
	bool missing;

#ifdef DAEDALUS_PROFILE_EXECUTION
	gTLBWriteHit++;
#endif

	u32 physical_addr = TLBEntry::Translate(address, missing);
	if (physical_addr != 0)
	{
		*(u32*)(g_pu8RamBase + (physical_addr & 0x007FFFFF)) = value;
	}
	else
	{
		// should be invalid
		R4300_Exception_TLB(address, EXC_WMISS, missing ? UT_VEC : E_VEC);

		*(u32*)g_pMemoryBuffers[MEM_UNUSED] = value;
	}	
}

/*
static void WriteValue_RAM_4Mb_8000_803F( u32 address, u32 value )
{
	*(u32 *)(g_pu8RamBase_8000 + address) = value;
}

static void WriteValue_RAM_8Mb_8000_807F( u32 address, u32 value )
{
	*(u32 *)(g_pu8RamBase_8000 + address) = value;
}

static void WriteValue_RAM_4Mb_A000_A03F( u32 address, u32 value )
{
	*(u32 *)(g_pu8RamBase_A000 + address) = value;
}

static void WriteValue_RAM_8Mb_A000_A07F( u32 address, u32 value )
{
	*(u32 *)(g_pu8RamBase_A000 + address) = value;
}
*/
static void WriteValue_8000_807F( u32 address, u32 value )
{
	// Note: Mask is slighty different when EPAK isn't used 0x003FFFFF
	*(u32 *)((u8 *)g_pMemoryBuffers[MEM_RD_RAM] + (address & 0x007FFFFF)) = value;
}

//*****************************************************************************
// 0x03F0 0000 to 0x03FF FFFF  RDRAM registers
//*****************************************************************************
static void WriteValue_83F0_83F0( u32 address, u32 value )
{	
	DPF( DEBUG_MEMORY_RDRAM_REG, "Writing to MEM_RD_REG: 0x%08x", address );
	*(u32 *)((u8 *)g_pMemoryBuffers[MEM_RD_REG0] + (address & 0xFF)) = value;
}

//*****************************************************************************
// 0x0400 0000 to 0x0400 FFFF  SP registers
//*****************************************************************************
static void WriteValue_8400_8400( u32 address, u32 value )
{
	DPF( DEBUG_MEMORY_SP_IMEM, "Writing to SP_MEM: 0x%08x", address );
	*(u32 *)((u8 *)g_pMemoryBuffers[MEM_SP_MEM] + (address & 0x1FFF)) = value;
}

//*****************************************************************************
//
//*****************************************************************************
static void WriteValue_8404_8404( u32 address, u32 value )
{
	DPF( DEBUG_MEMORY_SP_REG, "Writing to SP_REG: 0x%08x/0x%08x", address, value );
	u32 offset = address & 0xFF;

	switch (SP_BASE_REG + offset)
	{
	case SP_MEM_ADDR_REG:
	case SP_DRAM_ADDR_REG:
	case SP_SEMAPHORE_REG: //TODO - Make this do something?
		*(u32 *)((u8 *)g_pMemoryBuffers[MEM_SP_REG] + offset) = value;
		break;

	case SP_RD_LEN_REG:
		*(u32 *)((u8 *)g_pMemoryBuffers[MEM_SP_REG] + offset) = value;
		DMA_SP_CopyFromRDRAM();
		break;

	case SP_WR_LEN_REG:
		*(u32 *)((u8 *)g_pMemoryBuffers[MEM_SP_REG] + offset) = value;
		DMA_SP_CopyToRDRAM();
		break;

	case SP_STATUS_REG:
		MemoryUpdateSPStatus( value );
		break;
/*
	case SP_DMA_FULL_REG:
	case SP_DMA_BUSY_REG:
		// Prevent writing to read-only mem
		break;
*/
	}
}

//*****************************************************************************
// 0x04080000 to 0x04080003  SP_PC_REG
// 0x04080004 to 0x04080007  SP_IBIST_REG
//*****************************************************************************
static void WriteValue_8408_8408( u32 address, u32 value )
{
	// We don't support LLE RSP emulation in the PSP, so is ok to skip this reg -Salvy
	//
	//WriteValueInvalid(address, value); // No worth the extra jump since is jsut an empty func imo
#if 0
	u32 offset = (address&0x1FFFFFFF) - 0x04080000;

	if (offset == 0)
	{
		DPF( DEBUG_MEMORY_SP_REG, "Writing to SP_PC_REG: 0x%08x", value );

		gRSPState.CurrentPC = value;
	}
	else if ( offset ==4 )
	{
		DPF( DEBUG_MEMORY_SP_REG, "Writing to SP_IBIST_REG: 0x%08x", value );
		DBGConsole_Msg(0, "Writing to SP_IBIST_REG: 0x%08x", value);

		//*(u32 *)((u8*)g_pMemoryBuffers[MEM_SP_PC_REG] + 0x4) = value;
	}
	else
	{
		WriteValueInvalid(address, value);
	}
#endif
}

//*****************************************************************************
// 0x0410 0000 to 0x041F FFFF DP Command Registers
//*****************************************************************************
static void WriteValue_8410_841F( u32 address, u32 value )
{
	DPF( DEBUG_MEMORY_DP, "Writing to DP_COMMAND_REG: 0x%08x", address );
	u32 offset = address & 0xFF;

	switch (DPC_BASE_REG + offset)
	{
	case DPC_START_REG:
		// Write value to current reg too
		Memory_DPC_SetRegister( DPC_START_REG, value );
		Memory_DPC_SetRegister( DPC_CURRENT_REG, value );
		//DBGConsole_Msg( 0, "Wrote to [WDPC_START_REG] 0x%08x", value );
		break;

	case DPC_END_REG:
		Memory_DPC_SetRegister( DPC_END_REG, value );
		//
		// ToDo : Implement ProcessRDPList (LLE DList)
		//
		DBGConsole_Msg( 0, "Wrote to [WDPC_END_REG] 0x%08x", value );
		// ToDo : Should trigger for an interrupt?
		Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_DP);	
		break;

	case DPC_STATUS_REG:
		// Set flags etc
		MemoryUpdateDP( value );
		break;
/*
	case DPC_CURRENT_REG:// - Read Only, do we need to handle this?
	case DPC_CLOCK_REG: //- Read Only
	case DPC_BUFBUSY_REG: //- Read Only
	case DPC_PIPEBUSY_REG: //- Read Only
	case DPC_TMEM_REG: //- Read Only
		DBGConsole_Msg( 0, "Wrote to read only DPC reg" );
		break;
*/
	}
}

//*****************************************************************************
// 0x0420 0000 to 0x042F FFFF DP Span Registers
//*****************************************************************************
static void WriteValue_8420_842F( u32 address, u32 value )
{
	
	DBGConsole_Msg(0, "Write to DP Span Registers is unhandled (0x%08x, PC: 0x%08x)",
		address, gCPUState.CurrentPC);

	WriteValueInvalid(address, value);
}


//*****************************************************************************
// 0x0430 0000 to 0x043F FFFF MIPS Interface (MI) Registers
//*****************************************************************************
static void WriteValue_8430_843F( u32 address, u32 value )
{
	DPF( DEBUG_MEMORY_MI, "Writing to MI Registers: 0x%08x", address );
	u32 offset = address & 0xFF;

	switch (MI_BASE_REG + offset)
	{
	case MI_INIT_MODE_REG:
		MemoryModeRegMI( value );
		break;
	case MI_INTR_MASK_REG:
		MemoryUpdateMI( value );
		break;
	// Read Only
	/*
	case MI_VERSION_REG:
	case MI_INTR_REG:
		break;
	*/
	}
}

//*****************************************************************************
// 0x0440 0000 to 0x044F FFFF Video Interface (VI) Registers
//*****************************************************************************
static void WriteValue_8440_844F( u32 address, u32 value )
{
	u32 offset = address & 0xFF;

	switch (VI_BASE_REG + offset)
	{
	case VI_CONTROL_REG:
		DPF( DEBUG_VI, "VI_CONTROL_REG set to 0x%08x", value );
#ifdef DAEDALUS_LOG
		DisplayVIControlInfo(value);
#endif
		if ( gGraphicsPlugin != NULL )
		{
			gGraphicsPlugin->ViStatusChanged();
		}
		break;

	case VI_WIDTH_REG:
		DPF( DEBUG_VI, "VI_WIDTH_REG set to %d pixels", value );
		if ( gGraphicsPlugin != NULL )
		{
			gGraphicsPlugin->ViWidthChanged();
		}
		break;

	case VI_CURRENT_REG:
		DPF( DEBUG_VI, "VI_CURRENT_REG set to 0x%08x", value );

		// Any write clears interrupt line...
		DPF( DEBUG_VI, "VI: Clearing interrupt flag. PC: 0x%08x", gCPUState.CurrentPC );

		Memory_MI_ClrRegisterBits(MI_INTR_REG, MI_INTR_VI);
		R4300_Interrupt_UpdateCause3();
		return;
		//break;
	}

	Memory_VI_SetRegister(VI_BASE_REG + offset, value);
}

//*****************************************************************************
// 0x0450 0000 to 0x045F FFFF Audio Interface (AI) Registers
//*****************************************************************************
static void WriteValue_8450_845F( u32 address, u32 value )
{
	DPF( DEBUG_MEMORY_AI, "Writing to AI Registers: 0x%08x", address );
	u32 offset = address & 0xFF;

	switch (AI_BASE_REG + offset)
	{
	case AI_DRAM_ADDR_REG: // 64bit aligned
	case AI_CONTROL_REG:
	case AI_BITRATE_REG:
		*(u32 *)((u8 *)g_pMemoryBuffers[MEM_AI_REG] + offset) = value;
		break;

	case AI_LEN_REG:
		// LS 3 bits ignored
		*(u32 *)((u8 *)g_pMemoryBuffers[MEM_AI_REG] + offset) = value;

		if (g_pAiPlugin != NULL)
			g_pAiPlugin->LenChanged();
		//MemoryDoAI();
		break;
	/*
	case AI_CONTROL_REG:
		// DMA enable/Disable
		//if (g_dwAIBufferFullness == 2)
		*(u32 *)((u8 *)g_pMemoryBuffers[MEM_AI_REG] + offset) = value;
		//MemoryDoAI();
		break;*/

	case AI_STATUS_REG:
		Memory_MI_ClrRegisterBits(MI_INTR_REG, MI_INTR_AI);
		R4300_Interrupt_UpdateCause3();
		break;

	case AI_DACRATE_REG:
		*(u32 *)((u8 *)g_pMemoryBuffers[MEM_AI_REG] + offset) = value;

		if (g_pAiPlugin != NULL)
		{
			//When we set PAL mode for PAL games it corrects the sound pitch to the same as
			//NTSC games but it will also limit FPS 2% higher as well //Corn
			//
			g_pAiPlugin->DacrateChanged( g_ROM.TvType ? CAudioPlugin::ST_NTSC : CAudioPlugin::ST_PAL );
		}
		break;
	}
}


//*****************************************************************************
// 0x0460 0000 to 0x046F FFFF Peripheral Interface (PI) Registers
//*****************************************************************************
static void WriteValue_8460_846F( u32 address, u32 value )
{
	u32 offset = address & 0xFF;
	switch (PI_BASE_REG + offset)
	{
	case PI_DRAM_ADDR_REG:
	case PI_CART_ADDR_REG:
		*(u32 *)((u8 *)g_pMemoryBuffers[MEM_PI_REG] + offset) = value;
		break;

	case PI_RD_LEN_REG:
		*(u32 *)((u8 *)g_pMemoryBuffers[MEM_PI_REG] + offset) = value;
		DMA_PI_CopyFromRDRAM();
		break;

	case PI_WR_LEN_REG:
		*(u32 *)((u8 *)g_pMemoryBuffers[MEM_PI_REG] + offset) = value;
		// Do memory transfer
		DMA_PI_CopyToRDRAM();
		break;

	case PI_STATUS_REG:
		MemoryUpdatePI( value );
		break;

	default:
		DAEDALUS_ERROR("Unhandled PI write");
		break;
	}

}

//*****************************************************************************
// 0x0470 0000 to 0x047F FFFF RDRAM Interface (RI) Registers
//*****************************************************************************
static void WriteValue_8470_847F( u32 address, u32 value )
{
	DPF( DEBUG_MEMORY_RI, "Writing to MEM_RI_REG: 0x%08x", address );
	*(u32 *)((u8 *)g_pMemoryBuffers[MEM_RI_REG] + (address & 0xFF)) = value;
}


//*****************************************************************************
// 0x0480 0000 to 0x048F FFFF Serial Interface (SI) Registers
//*****************************************************************************
static void WriteValue_8480_848F( u32 address, u32 value )
{
	DPF( DEBUG_MEMORY_SI, "Writing to MEM_SI_REG: 0x%08x", address );

	u32 offset = address & 0xFF;

	switch (SI_BASE_REG + offset)
	{
	case SI_DRAM_ADDR_REG:
		*(u32 *)((u8 *)g_pMemoryBuffers[MEM_SI_REG] + offset) = value;
		break;
			
	case SI_PIF_ADDR_RD64B_REG:
		// Trigger DRAM -> PIF DMA
		*(u32 *)((u8 *)g_pMemoryBuffers[MEM_SI_REG] + offset) = value;
		DMA_SI_CopyToDRAM();
		break;

		// Reserved Registers here!
	case SI_PIF_ADDR_WR64B_REG:
		// Trigger DRAM -> PIF DMA
		*(u32 *)((u8 *)g_pMemoryBuffers[MEM_SI_REG] + offset) = value;
		DMA_SI_CopyFromDRAM();
		break;

	case SI_STATUS_REG:	
		// Any write to this reg clears interrupts
		Memory_SI_ClrRegisterBits(SI_STATUS_REG, SI_STATUS_INTERRUPT);
		Memory_MI_ClrRegisterBits(MI_INTR_REG, MI_INTR_SI);
		R4300_Interrupt_UpdateCause3();
		break;
	}		
}

//*****************************************************************************
// 0x1FC0 0000 to 0x1FC0 07BF PIF Boot ROM (Ignored)
// 0x1FC0 07C0 to 0x1FC0 07FF PIF RAM
//*****************************************************************************
static void WriteValue_9FC0_9FCF( u32 address, u32 value )
{
	u32 offset = address & 0x3F;
	DAEDALUS_ASSERT(!(address - 0x7C0 & ~0x3F), "Read to PIF RAM (0x%08x) is invalid", address);
	DPF( DEBUG_MEMORY_PIF, "Writing to MEM_PIF_RAM: 0x%08x", address );
	
	*(u32 *)((u8 *)g_pMemoryBuffers[MEM_PIF_RAM] + offset) = value;
	if (offset == 0x3C)
	{
		MemoryUpdatePIF();
	}
}

//*****************************************************************************
//
//*****************************************************************************
static void WriteValue_FlashRam( u32 address, u32 value )
{
	u32 offset = address & 0xFF;
	if( g_ROM.settings.SaveType == SAVE_TYPE_FLASH && offset == 0 )
	{
		Flash_DoCommand( value );
		return;
	}

	DBGConsole_Msg(0, "[GWrite to FlashRam (0x%08x) is unhandled", address);
	WriteValueInvalid(address, value);
}

//*****************************************************************************
//
//*****************************************************************************
static void WriteValue_ROM( u32 address, u32 value )
{
	// Write to ROM support
	// A Bug's Life and Toy Story 2 write to ROM, add support by storing written value which is used when reading from Rom.  
	g_pWriteRom[0] = value;	g_RomWritten = true;
}
