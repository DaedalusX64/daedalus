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

#include "DMA.h"
#include "Memory.h"
#include "RSP.h"
#include "RSP_HLE.h"
#include "CPU.h"
#include "ROM.h"
#include "ROMBuffer.h"
#include "PIF.h"
#include "Interrupt.h"
#include "Save.h"

#include "../SysPSP/Utility/FastMemcpy.h"

#include "Debug/DebugLog.h"
#include "Debug/DBGConsole.h"

#include "OSHLE/OSTask.h"
#include "OSHLE/patch.h"

bool gDMAUsed = false;
//*****************************************************************************
// 
//*****************************************************************************
void DMA_SP_CopyFromRDRAM()
{
	u32 spmem_address_reg = Memory_SP_GetRegister(SP_MEM_ADDR_REG);
	u32 rdram_address_reg = Memory_SP_GetRegister(SP_DRAM_ADDR_REG);
	u32 rdlen_reg         = Memory_SP_GetRegister(SP_RD_LEN_REG);
	u32 splen			 = (rdlen_reg & 0xFFF) + 1;	//[0-11] is length to transfer

	DAEDALUS_ASSERT( (splen & 0x1) == 0, "Warning, PI DMA DRAM from SP, odd length = %d", splen);

	// Ignore IMEM for speed (we don't use low-level RSP anyways on the PSP)
	if((spmem_address_reg & 0x1000) == 0)
	{
		memcpy_vfpu_BE(&g_pu8SpMemBase[(spmem_address_reg & 0xFFF)],
					   &g_pu8RamBase[(rdram_address_reg & 0xFFFFFF)], splen); 
	}

	//Clear the DMA Busy
	Memory_SP_SetRegister(SP_DMA_BUSY_REG, 0);
	Memory_SP_ClrRegisterBits(SP_STATUS_REG, SP_STATUS_DMA_BUSY);

}

//*****************************************************************************
//
//*****************************************************************************
void DMA_SP_CopyToRDRAM()
{
	u32 spmem_address_reg = Memory_SP_GetRegister(SP_MEM_ADDR_REG);
	u32 rdram_address_reg = Memory_SP_GetRegister(SP_DRAM_ADDR_REG);
	u32 wrlen_reg         = Memory_SP_GetRegister(SP_WR_LEN_REG);
	u32 splen			 = (wrlen_reg & 0xFFF) + 1;	//[0-11] is length to transfer

	DAEDALUS_ASSERT( (splen & 0x1) == 0, "Warning, PI DMA DRAM to SP, odd length = %d", splen)

	// Ignore IMEM for speed (we don't use low-level RSP anyways on the PSP)
	if((spmem_address_reg & 0x1000) == 0)
	{
		memcpy_vfpu_BE(&g_pu8RamBase[(rdram_address_reg & 0xFFFFFF)],
					   &g_pu8SpMemBase[(spmem_address_reg & 0xFFF)], splen);
	}

	//Clear the DMA Busy
	Memory_SP_SetRegister(SP_DMA_BUSY_REG, 0);
	Memory_SP_ClrRegisterBits(SP_STATUS_REG, SP_STATUS_DMA_BUSY);

}

//*****************************************************************************
// Copy 64bytes from DRAM to PIF_RAM
//*****************************************************************************
void DMA_SI_CopyFromDRAM( )
{
	u32 mem = Memory_SI_GetRegister(SI_DRAM_ADDR_REG) & 0x1fffffff;
	u8 * p_dst = (u8 *)g_pMemoryBuffers[MEM_PIF_RAM];
	u8 * p_src = g_pu8RamBase + mem;

	DPF( DEBUG_MEMORY_PIF, "DRAM (0x%08x) -> PIF Transfer ", mem );
	
	u32* p_dst32=(u32*)p_dst;
	u32* p_scr32=(u32*)p_src;

	// Fuse 4 reads and 4 writes to just one which is a lot faster - Corn
	for(u32 i = 0; i < 16; i++)
	{
		p_dst32[i] = SWAP_PIF( p_scr32[i] );
	}

	Memory_SI_SetRegisterBits(SI_STATUS_REG, SI_STATUS_INTERRUPT);
	Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_SI);
	R4300_Interrupt_UpdateCause3();
}

//*****************************************************************************
// Copy 64bytes to DRAM from PIF_RAM
//*****************************************************************************
void DMA_SI_CopyToDRAM( )
{
	// Check controller status!
	CController::Get()->Process();

	u32 mem = Memory_SI_GetRegister(SI_DRAM_ADDR_REG) & 0x1fffffff;
	u8 * p_src = (u8 *)g_pMemoryBuffers[MEM_PIF_RAM];
	u8 * p_dst = g_pu8RamBase + mem;

	DPF( DEBUG_MEMORY_PIF, "PIF -> DRAM (0x%08x) Transfer ", mem );

	u32* p_dst32=(u32*)p_dst;
	u32* p_scr32=(u32*)p_src;

	// Fuse 4 reads and 4 writes to just one which is a lot faster - Corn
	for(u32 i = 0; i < 16; i++)
	{
		p_dst32[i] = SWAP_PIF( p_scr32[i] );
	}

	Memory_SI_SetRegisterBits(SI_STATUS_REG, SI_STATUS_INTERRUPT);
	Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_SI);

	//Skipping this IRQ fixes allows Body Harvest, Nightmare Creatures and Cruisn' USA to boot but make Animal crossing fail
	//ToDo: Delay SI
	//
	if (g_ROM.GameHacks != BODY_HARVEST) 
		R4300_Interrupt_UpdateCause3();
}

/*
#define PI_DOM2_ADDR1		0x05000000	// to 0x05FFFFFF
#define PI_DOM1_ADDR1		0x06000000	// to 0x07FFFFFF
#define PI_DOM2_ADDR2		0x08000000	// to 0x0FFFFFFF
#define PI_DOM1_ADDR2		0x10000000	// to 0x1FBFFFFF
#define PI_DOM1_ADDR3		0x1FD00000	// to 0x7FFFFFFF
*/

#define IsDom1Addr1( x )		( (x) >= PI_DOM1_ADDR1 && (x) < PI_DOM2_ADDR2 )
#define IsDom1Addr2( x )		( (x) >= PI_DOM1_ADDR2 && (x) < 0x1FBFFFFF )
#define IsDom1Addr3( x )		( (x) >= PI_DOM1_ADDR3 && (x) < 0x7FFFFFFF )
#define IsDom2Addr1( x )		( (x) >= PI_DOM2_ADDR1 && (x) < PI_DOM1_ADDR1 )
#define IsDom2Addr2( x )		( (x) >= 0x08000000 && (x) < 0x10000000 )

//*****************************************************************************
//
//*****************************************************************************
bool DMA_HandleTransfer( u8 * p_dst, u32 dst_offset, u32 dst_size, const u8 * p_src, u32 src_offset, u32 src_size, u32 length )
{
	if( ( s32( length ) <= 0 ) ||
		(src_offset + length) > src_size ||
		(dst_offset + length) > dst_size )
	{
		return false;
	}

#if 1 //1->new, 0->old
	//Doesn't like that we use VFPU here //Corn
	//Little Endian
	// We only have to fiddle the bytes when
	// a) the src is not word aligned
	// b) the dst is not word aligned
	// c) the length is not a multiple of 4 (although we can copy most directly)
	// If the source/dest are word aligned, we can simply copy most of the
	// words using memcpy. Any remaining bytes are then copied individually
	if( !(dst_offset & 0x3) & !(src_offset & 0x3) )
	{
		// Optimise for u32 alignment - do multiple of four using memcpy
		u32 block_length(length & ~0x3);

		// Might be 0 if xref is less than 4 bytes in total
		// memcpy_vfpu seems to fail here, maybe still needs some tweak for border copy cases to work properly //Corn
		if( block_length ) memcpy(&p_dst[dst_offset],  (void*)&p_src[src_offset], block_length);

		// Do remainder - this is only 0->3 bytes
		for(u32 i = block_length; i < length; ++i)
		{
			p_dst[(i + dst_offset)^U8_TWIDDLE] = p_src[(i + src_offset)^U8_TWIDDLE];
		}
	}
	else
	{
		memcpy_cpu_LE(&p_dst[dst_offset],  (void*)&p_src[src_offset], length);
	}

#else
	//Todo:Try to optimize futher Little Endian code
	//Little Endian
	// We only have to fiddle the bytes when
	// a) the src is not word aligned
	// b) the dst is not word aligned
	// c) the length is not a multiple of 4 (although we can copy most directly)
	// If the source/dest are word aligned, we can simply copy most of the
	// words using memcpy. Any remaining bytes are then copied individually
	if((dst_offset & 0x3) == 0 && (src_offset & 0x3) == 0)
	{
		// Optimise for u32 alignment - do multiple of four using memcpy
		u32 block_length(length & ~0x3);

		// Might be 0 if xref is less than 4 bytes in total
		// memcpy_vfpu seems to fail here, maybe still needs some tweak for border copy cases to work properly //Corn
		//if(block_length)
			memcpy(&p_dst[dst_offset],  (void*)&p_src[src_offset], block_length);

		// Do remainder - this is only 0->3 bytes
		for(u32 i = block_length; i < length; ++i)
		{
			p_dst[(i + dst_offset)^U8_TWIDDLE] = p_src[(i + src_offset)^U8_TWIDDLE];
		}
	}
	else
	{
		for(u32 i = 0; i < length; ++i)
		{
			p_dst[(i + dst_offset)^U8_TWIDDLE] = p_src[(i + src_offset)^U8_TWIDDLE];
		}
	}
#endif
	return true;
}

//*****************************************************************************
//
//*****************************************************************************
void DMA_PI_CopyToRDRAM()
{
	u32 mem_address  = Memory_PI_GetRegister(PI_DRAM_ADDR_REG) & 0x00FFFFFF;
	u32 cart_address = Memory_PI_GetRegister(PI_CART_ADDR_REG)  & 0xFFFFFFFF;
	u32 pi_length_reg = (Memory_PI_GetRegister(PI_WR_LEN_REG) & 0xFFFFFFFF) + 1;

	DPF( DEBUG_MEMORY_PI, "PI: Copying %d bytes of data from 0x%08x to 0x%08x", pi_length_reg, cart_address, mem_address );

	if (cart_address < 0x10000000)
    {
		if (cart_address >= 0x08000000 && cart_address < 0x08010000)
		{
           	const u8 *	p_src( (const u8 *)g_pMemoryBuffers[MEM_SAVE] );
			u32			src_size( ( MemoryRegionSizes[MEM_SAVE] ) );
			cart_address -= PI_DOM2_ADDR2;

			if (g_ROM.settings.SaveType != SAVE_TYPE_FLASH)
				DMA_HandleTransfer( g_pu8RamBase, mem_address, gRamSize, p_src, cart_address, src_size, pi_length_reg );
			else
				DMA_FLASH_CopyToDRAM(mem_address, cart_address, pi_length_reg);
		}
		/*else if (cart_address >= 0x06000000 && cart_address < 0x08000000)
		{
			DBGConsole_Msg(0, "[YReading from Cart domain 1/addr1] (Ignored)");
		}
		else
		{
			DBGConsole_Msg(0, "[YUnknown PI Address 0x%08x]", cart_address);
		}*/
	}
	else
	{
		// for paper mario
		// Doesn't seem to be needed?
		/*if (cart_address >= 0x1fc00000)
		{
			Memory_PI_ClrRegisterBits(PI_STATUS_REG, PI_STATUS_DMA_BUSY);
			Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_PI);
			R4300_Interrupt_UpdateCause3();
			return;
		}*/

		//DBGConsole_Msg(0, "[YReading from Cart domain 1/addr2]");
		cart_address -= PI_DOM1_ADDR2;
		CPU_InvalidateICacheRange( 0x80000000 | mem_address, pi_length_reg );
		RomBuffer::CopyToRam( g_pu8RamBase, mem_address, gRamSize, cart_address, pi_length_reg );

		if (!gDMAUsed)
		{ 
#ifdef DAEDALUS_ENABLE_OS_HOOKS
			// Note the rom is only scanned when the ROM jumps to the game boot address
			// ToDO: try to reapply patches - certain roms load in more of the OS after a number of transfers ?
			Patch_ApplyPatches();
#endif			
			gDMAUsed = true;
			
			u32 addr = (g_ROM.cic_chip != CIC_6105) ? 0x80000318 : 0x800003F0;
			Write32Bits(addr, gRamSize);

			// For reference DK64 hack : This allows DK64 to work
			// Note1: IMEM transfers are required! (We ignore IMEM transfers for speed, see DMA_SP_*)
			// Note2: Make sure to change EEPROM to 4k too! Otherwise DK64 hangs after the intro. 
			//Write32Bits(0x802FE1C0, 0xAD170014);
		}
	}

	Memory_PI_ClrRegisterBits(PI_STATUS_REG, PI_STATUS_DMA_BUSY);
	Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_PI);
	R4300_Interrupt_UpdateCause3();
}

//*****************************************************************************
//
//*****************************************************************************
void DMA_PI_CopyFromRDRAM()
{
	u32 mem_address  = Memory_PI_GetRegister(PI_DRAM_ADDR_REG) & 0xFFFFFFFF;
	u32 cart_address = Memory_PI_GetRegister(PI_CART_ADDR_REG)  & 0xFFFFFFFF;
	u32 pi_length_reg = (Memory_PI_GetRegister(PI_RD_LEN_REG)  & 0xFFFFFFFF) + 1;

	DPF(DEBUG_MEMORY_PI, "PI: Copying %d bytes of data from 0x%08x to 0x%08x", pi_length_reg, mem_address, cart_address );

	/*
	if(pi_length_reg & 0x1)
	{
		DBGConsole_Msg(0, "PI Copy RDRAM to CART %db from %08X to %08X", pi_length_reg, cart_address|0xA0000000, mem_address);
		DBGConsole_Msg(0, "Warning, PI DMA, odd length");

		// Tonic Trouble triggers this !

		pi_length_reg ++;
	}
	*/

	// Only care for DOM2/ADDR2
	// PI_DOM2_ADDR2 (FlashRAM) && < 0x08010000 (SRAM)
	//
	if(cart_address >= PI_DOM2_ADDR2 && cart_address < 0x08010000)
	{
		u8 *	p_dst( (u8 *)g_pMemoryBuffers[MEM_SAVE] );
		u32		dst_size( MemoryRegionSizes[MEM_SAVE] );
		cart_address -= PI_DOM2_ADDR2;

		DBGConsole_Msg(0, "[YWriting to Cart domain 2/addr2 0x%08x]", cart_address);

		if (g_ROM.settings.SaveType != SAVE_TYPE_FLASH)
			DMA_HandleTransfer( p_dst, cart_address, dst_size, g_pu8RamBase, mem_address, gRamSize, pi_length_reg );
		else
			DMA_FLASH_CopyFromDRAM(mem_address, pi_length_reg);

		Save::MarkSaveDirty();
	}
	else
	{
		DBGConsole_Msg(0, "[YUnknown PI Address 0x%08x]", cart_address);
	}

	Memory_PI_ClrRegisterBits(PI_STATUS_REG, PI_STATUS_DMA_BUSY);
	Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_PI);
	R4300_Interrupt_UpdateCause3();

}

