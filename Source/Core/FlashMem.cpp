/*
Copyright (C) 2008-2009 Howard Su (howard0su@gmail.com)

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


#include "Base/Types.h"
#include "Debug/DBGConsole.h"
#include "Core/FlashMem.h"
#include "Core/Memory.h"
#include "Core/DMA.h"
#include "Core/ROM.h"
#include "Core/Save.h"

u32 FlashStatus[2];
u32 FlashRAM_Offset = 0;
static u8 FlashBlock[128];

enum TFlashRam_Types
{
    MX29L1100_ID = 0x00c2001e,
    MX29L1101_ID = 0x00c2001d,
    MN63F8MPN_ID = 0x003200f1,
};

enum TFlashRam_Modes 
{
	FLASHRAM_MODE_NOPES = 0,
	FLASHRAM_MODE_ERASE = 1,
	FLASHRAM_MODE_WRITE,
	FLASHRAM_MODE_READ,
	FLASHRAM_MODE_STATUS,
};
TFlashRam_Modes FlashFlag = FLASHRAM_MODE_NOPES;

void Flash_Init()
{
	FlashFlag = FLASHRAM_MODE_NOPES;
	FlashRAM_Offset = 0;
	FlashStatus[0] = 0;
	FlashStatus[1] = MX29L1100_ID;	// Default to MX29L1100 as it is the most common one and is what pj64 uses
}

bool DMA_FLASH_CopyToDRAM(u32 dest, u32 StartOffset, u32 len)
{
	switch(FlashFlag)
	{
	case FLASHRAM_MODE_READ:
		{
			StartOffset = StartOffset << 1;
			return DMA_HandleTransfer( g_pu8RamBase, dest, gRamSize, (u8*)g_pMemoryBuffers[MEM_SAVE], StartOffset, MemoryRegionSizes[MEM_SAVE], len );
		}
	case FLASHRAM_MODE_STATUS:
		DAEDALUS_ASSERT(len == sizeof(u32) * 2, "Len is not correct when fetch status.");
		*(u32 *)(g_pu8RamBase + dest) = FlashStatus[0];
		*(u32 *)(g_pu8RamBase + dest + sizeof(u32)) = FlashStatus[1];
		return true;
	default:
		return false;
	}
}

bool DMA_FLASH_CopyFromDRAM(u32 dest, u32 len)
{
	if(FlashFlag == FLASHRAM_MODE_WRITE )
	{
		return DMA_HandleTransfer( FlashBlock, 0, 128, g_pu8RamBase, dest, gRamSize, len );
	}
	return false;
}

void Flash_DoCommand(u32 FlashRAM_Command)
{
	switch (FlashRAM_Command & 0xFF000000) 
	{
	case 0xD2000000:
		switch (FlashFlag) 
		{
			//DBGConsole_Msg(0, "Writing to FlashRam command: 0x%08x - mode: %d",FlashRAM_Command, FlashFlag);
			case FLASHRAM_MODE_NOPES:
			case FLASHRAM_MODE_READ:
			case FLASHRAM_MODE_STATUS:
				break;
			case FLASHRAM_MODE_ERASE:
				memset((u8*)g_pMemoryBuffers[MEM_SAVE] + FlashRAM_Offset, 0xFF, 128);
				Save_MarkSaveDirty();
				break;
			case FLASHRAM_MODE_WRITE:
				memcpy((u8*)g_pMemoryBuffers[MEM_SAVE] + FlashRAM_Offset, FlashBlock, 128);
				Save_MarkSaveDirty();
				break;
			default:
				DBGConsole_Msg(0, "Warning: Unknown FlashRam mode: %d", FlashFlag);
				break;
		}
		FlashFlag = FLASHRAM_MODE_NOPES;
		break;
	case 0xE1000000:
		FlashFlag = FLASHRAM_MODE_STATUS;
		FlashStatus[0] = 0x11118001;
		break;
	case 0xF0000000:
		FlashFlag = FLASHRAM_MODE_READ;
		FlashStatus[0] = 0x11118004;
		break;
	case 0x4B000000:
		FlashRAM_Offset = (FlashRAM_Command & 0xffff) * 128;
		break;
	case 0x78000000:
		FlashFlag = FLASHRAM_MODE_ERASE;
		FlashStatus[0] = 0x11118008;
		break;
	case 0xB4000000:
		FlashFlag = FLASHRAM_MODE_WRITE;
		break;
	case 0xA5000000:
		FlashRAM_Offset = (FlashRAM_Command & 0xffff) * 128;
		FlashStatus[0] = 0x11118004;
		break;
	case 0x00000000:
		break;
	default:
		DBGConsole_Msg(0, "Warning: Unknown FlashRam command: 0x%08x",FlashRAM_Command);
		break;
	}
}
