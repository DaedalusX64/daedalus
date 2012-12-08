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

//
// InternalRead is only used for debug puposes
//

#ifndef DAEDALUS_SILENT

//*****************************************************************************
//
//*****************************************************************************
static bool InternalReadInvalid( u32 address, void ** p_translated )
{
	//DBGConsole_Msg(0, "Illegal Internal Read of 0x%08x at 0x%08x", address, (u32)gCPUState.CurrentPC);
	*p_translated = g_pMemoryBuffers[MEM_UNUSED];
	return false;
}

//*****************************************************************************
//
//*****************************************************************************
static bool InternalReadMapped( u32 address, void ** p_translated )
{
	bool missing;

#ifdef DAEDALUS_PROFILE_EXECUTION
	gTLBReadHit++;
#endif

	u32 physical_addr = TLBEntry::Translate(address, missing);
	if (physical_addr != 0)
	{
		*p_translated = g_pu8RamBase + (physical_addr & 0x007FFFFF);

		return true;
	}
	else
	{
		if (missing)
		{
			// No valid TLB entry - throw TLB Refill Exception
#ifdef DAEDALUS_DEBUG_CONSOLE
			if (g_DaedalusConfig.WarnMemoryErrors)
			{
				DBGConsole_Msg(0, "Internal TLB Refill", address);
			}
#endif
			return InternalReadInvalid( address, p_translated );
		}
		else
		{
			// Throw TLB Invalid exception
#ifdef DAEDALUS_DEBUG_CONSOLE
			if (g_DaedalusConfig.WarnMemoryErrors)
			{
				DBGConsole_Msg(0, "Internal TLB Invalid");
			}
#endif
			return InternalReadInvalid( address, p_translated );
		}
	}
}


//*****************************************************************************
//
//*****************************************************************************
static bool InternalRead_4Mb_8000_803F( u32 address, void ** p_translated )
{
	*p_translated = g_pu8RamBase + (address & 0x003FFFFF);
	return true;
}

//*****************************************************************************
//
//*****************************************************************************
static bool InternalRead_8Mb_8000_807F( u32 address, void ** p_translated )
{
	*p_translated = g_pu8RamBase + (address & 0x007FFFFF);
	return true;
}

//*****************************************************************************
//
//*****************************************************************************
static bool InternalReadROM( u32 address, void ** p_translated )
{
	// Note: NOT 0x1FFFFFFF
	u32		offset( address & 0x00FFFFFF );

	*p_translated = RomBuffer::GetAddressRaw( offset );
	if( *p_translated != NULL )
	{
		//DPF(DEBUG_MEMORY, "Reading from ROM: 0x%08x", address);
		return true;
	}
	else
	{
		return InternalReadInvalid( address, p_translated );
	}
}

//*****************************************************************************
//
//*****************************************************************************
static bool InternalRead_8400_8400( u32 address, void ** p_translated )
{
	u32 offset;

	// 0x0400 0000 to 0x0400 FFFF  SP registers
	if ((address&0x1FFFFFFF) < 0x4002000)
	{
		DPF( DEBUG_MEMORY_SP_IMEM, "Reading from SP_MEM: 0x%08x", address );

		offset = address & 0x1FFF;

		*p_translated = (u8 *)g_pMemoryBuffers[MEM_SP_MEM] + offset;
		return true;
	}
	else
	{
		return InternalReadInvalid( address, p_translated );
	}
}

//*****************************************************************************
//
//*****************************************************************************
static bool InternalRead_9FC0_9FCF( u32 address, void ** p_translated )
{
	u32 offset;

	if ((address&0x1FFFFFFF) <= PIF_ROM_END)
	{
		DPF( DEBUG_MEMORY_PIF, "Reading from MEM_PIF_ROM: 0x%08x", address );

		offset = address & 0x0FFF;

		*p_translated = (u8 *)g_pMemoryBuffers[MEM_PIF_RAM] + offset;
		return true;
	}

	else if ((address&0x1FFFFFFF) <= PIF_RAM_END)
	{
		DPF( DEBUG_MEMORY_PIF, "Reading from MEM_PIF_RAM: 0x%08x", address );
		DBGConsole_Msg(0, "[WReading directly from PI ram]: 0x%08x!", address);

		offset = address & 0x0FFF;

		*p_translated = (u8 *)g_pMemoryBuffers[MEM_PIF_RAM] + offset;
		return true;
	}
	else
	{
		return InternalReadInvalid( address, p_translated );
	}
}
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

void Memory_InitInternalTables(u32 ram_size)
{
	memset(InternalReadFastTable, 0, sizeof(InternalMemFastFunction) * 0x4000);

	u32 i = 0;
	u32 entry = 0;
	u32 start_addr = 0;
	u32 end_addr = 0;

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

	// "Real"
	start_addr = 0x8000;
	end_addr = 0x8000 + ((ram_size>>16)-1);

	for (i = (start_addr>>2); i <= (end_addr>>2); i++)
	{
		InternalReadFastTable[i]  = InternalRead_8Mb_8000_807F;
	}


	// Shadow
	start_addr = 0xA000;
	end_addr = 0xA000 + ((ram_size>>16)-1);


	for (i = (start_addr>>2); i <= (end_addr>>2); i++)
	{
		InternalReadFastTable[i]  = InternalRead_4Mb_8000_803F;

	}
}


#endif
