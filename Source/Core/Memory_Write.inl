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
static void * WriteInvalid( u32 address )
{
	DPF( DEBUG_MEMORY, "Illegal Memory Access Tried to Write To 0x%08x PC: 0x%08x", address, gCPUState.CurrentPC );
	if (g_DaedalusConfig.WarnMemoryErrors)
	{
		CPU_Halt("Illegal Memory Access");
		DBGConsole_Msg(0, "Illegal Memory Access: Tried to Write To 0x%08x (PC: 0x%08x)", address, gCPUState.CurrentPC);
	}

	return g_pMemoryBuffers[MEM_UNUSED];
}

//*****************************************************************************
//
//*****************************************************************************
static void *Write_Noise( u32 address )
{
	//static bool bWarned( false );
	//if (!bWarned)
	//{
	//	DBGConsole_Msg(0, "Writing noise (0x%08x) - sizing memory?", address);
	//	bWarned = true;
	//}
	return g_pMemoryBuffers[MEM_UNUSED];
}

//*****************************************************************************
//
//*****************************************************************************
static void * WriteMapped( u32 address )
{
	bool missing;

#ifdef DAEDALUS_PROFILE_EXECUTION
	gTLBWriteHit++;
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
			R4300_Exception_TLB_Refill( address, EXCEPTION_TLB_STORE );

			return g_pMemoryBuffers[MEM_UNUSED];
		}
		else
		{
			// should be invalid
			R4300_Exception_TLB_Invalid( address, EXCEPTION_TLB_STORE );

			return g_pMemoryBuffers[MEM_UNUSED];
		}
	}	
}



static void *Write_RAM_4Mb_8000_803F( u32 address )
{
	return g_pu8RamBase_8000 + address;
}

static void *Write_RAM_8Mb_8000_807F( u32 address )
{
	return g_pu8RamBase_8000 + address;
}

static void *Write_RAM_4Mb_A000_A03F( u32 address )
{
	return g_pu8RamBase_A000 + address;
}

static void *Write_RAM_8Mb_A000_A07F( u32 address )
{
	return g_pu8RamBase_A000 + address;
}

//*****************************************************************************
// 0x03F0 0000 to 0x03FF FFFF  RDRAM registers
//*****************************************************************************
static void *Write_83F0_83F0( u32 address )
{
	
	if (MEMORY_BOUNDS_CHECKING((address&0x1FFFFFFF) < 0x04000000))
	{
		DPF( DEBUG_MEMORY_RDRAM_REG, "Writing to MEM_RD_REG: 0x%08x", address );
	//	DBGConsole_Msg(0, "Writing to MEM_RD_REG: 0x%08x", address);

		return ((u8 *)g_pMemoryBuffers[MEM_RD_REG0] + (address & 0xFF));
	}
	else
	{
		return WriteInvalid( address );
	} 
}

//*****************************************************************************
// 0x0400 0000 to 0x0400 FFFF  SP memory
//*****************************************************************************
static void * Write_8400_8400( u32 address )
{
	if (MEMORY_BOUNDS_CHECKING((address&0x1FFFFFFF) <= SP_IMEM_END))
	{
		DPF( DEBUG_MEMORY_SP_IMEM, "Writing to SP_MEM: 0x%08x", address );

		return ((u8 *)g_pMemoryBuffers[MEM_SP_MEM] + (address & 0x1FFF));
	}
	else
	{	
		return WriteInvalid( address );
	}
}

