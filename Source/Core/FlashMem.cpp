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

#include "stdafx.h"
#include "Debug/DBGConsole.h"
#include "Memory.h"
#include "DMA.h"
#include "ROM.h"
#include "Save.h"
#include "Debug/Dump.h"		// Dump_GetSaveDirectory()

u32 FlashStatus[2];
u32 FlashRAM_Offset;
u8  FlashBlock[128];

enum TFlashRam_Modes {
	FLASHRAM_MODE_NOPES = 0,
	FLASHRAM_MODE_ERASE = 1,
	FLASHRAM_MODE_WRITE,
	FLASHRAM_MODE_READ,
	FLASHRAM_MODE_STATUS,
};
TFlashRam_Modes FlashFlag = FLASHRAM_MODE_NOPES;

#define FLASH_COMMAND_XXX 0x

#define SetFlashStatus(x) FlashStatus[0] = (u32)((x)>> 32), FlashStatus[1] = (u32)(x)


bool DMA_FLASH_CopyToDRAM(u32 dest, u32 StartOffset, u32 len) 
{
	switch(FlashFlag)
	{
	case FLASHRAM_MODE_READ:
		{
			StartOffset = StartOffset << 1;
			DMA_HandleTransfer( g_pu8RamBase, dest, gRamSize, (u8*)g_pMemoryBuffers[MEM_SAVE], StartOffset, MemoryRegionSizes[MEM_SAVE], len );
			return true;
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
		DMA_HandleTransfer( FlashBlock, 0, 128, g_pu8RamBase, dest, gRamSize, len );
		return true;
	}
	return false;
}

void Flash_DoCommand(u32 FlashRAM_Command)
{
	switch (FlashRAM_Command & 0xFF000000) {
	case 0xD2000000: 
		switch (FlashFlag) {
			DBGConsole_Msg(0, "Writing %X to flash ram command register\nFlashFlag: %d",FlashRAM_Command,FlashFlag);
			case FLASHRAM_MODE_NOPES: 
				break;
			case FLASHRAM_MODE_READ: 
				break;
			case FLASHRAM_MODE_STATUS: 
				break;
			case FLASHRAM_MODE_ERASE:
				memset((u8*)g_pMemoryBuffers[MEM_SAVE] + FlashRAM_Offset, 0xFF, 128);
				Save::MarkSaveDirty();
				break;
			case FLASHRAM_MODE_WRITE:
				memcpy((u8*)g_pMemoryBuffers[MEM_SAVE] + FlashRAM_Offset, FlashBlock, 128);
				Save::MarkSaveDirty();
				break;
			default:
				DBGConsole_Msg(0, "Writing %X to flash ram command register\nFlashFlag: %d",FlashRAM_Command,FlashFlag);
		}
		FlashFlag = FLASHRAM_MODE_NOPES;
		break;
	case 0xE1000000: 
		FlashFlag = FLASHRAM_MODE_STATUS;
		SetFlashStatus(0x1111800100C20000LL);
		break;
	case 0xF0000000: 
		FlashFlag = FLASHRAM_MODE_READ;
		SetFlashStatus(0x11118004F0000000LL);
		break;
	case 0x4B000000:
		//FlashFlag = FLASHRAM_MODE_ERASE;
		FlashRAM_Offset = (FlashRAM_Command & 0xffff) * 128;
		break;
	case 0x78000000:
		FlashFlag = FLASHRAM_MODE_ERASE;
		SetFlashStatus(0x1111800800C20000LL);
		break;
	case 0xB4000000: 
		FlashFlag = FLASHRAM_MODE_WRITE; //????
		break;
	case 0xA5000000:
		FlashRAM_Offset = (FlashRAM_Command & 0xffff) * 128;
		SetFlashStatus(0x1111800400C20000LL);
		break;
	case 0x00000000:
		break;
	default:
		DBGConsole_Msg(0, "Writing %X to flash ram command register",FlashRAM_Command);
	}
}
