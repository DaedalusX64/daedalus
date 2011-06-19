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
#ifndef DAEDALUS_PUBLIC_RELEASE
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
#ifndef DAEDALUS_PUBLIC_RELEASE
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
