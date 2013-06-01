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

#include "ROM.h"
#include "Memory.h"
#include "Save.h"

#include "Utility/IO.h"

#include "Debug/DBGConsole.h"
#include "Debug/Dump.h"

#include "ConfigOptions.h"

static void InitMempackContent();

static IO::Filename		gSaveFileName;
static bool				gSaveDirty;
static u32				gSaveSize;
static IO::Filename		gMempackFileName;
static bool				gMempackDirty;

void Save::MarkSaveDirty()
{
	gSaveDirty = true;
}

void Save::MarkMempackDirty()
{
	gMempackDirty = true;
}

bool Save::Reset()
{
	const char	*ext;
	FILE 		*fp;
	u8			buffer[2048];

	IO::Directory::EnsureExists(g_DaedalusConfig.szSaveDir);

	switch(g_ROM.settings.SaveType)
	{
	case SAVE_TYPE_EEP4K:
		ext = ".sav";
		gSaveSize = 4 * 1024;
		break;
	case SAVE_TYPE_EEP16K:
		ext = ".sav";
		gSaveSize = 16 * 1024;
		break;
	case SAVE_TYPE_SRAM:
		ext = ".sra";
		gSaveSize = 32 * 1024;
		break;
	case SAVE_TYPE_FLASH:
		ext = ".fla";
		gSaveSize = 128 * 1024;
		break;
	default:
		ext = "";
		gSaveSize = 0;
		break;
	}

	DAEDALUS_ASSERT( gSaveSize <=  MemoryRegionSizes[MEM_SAVE], "Save size is larger than allocated memory");
	gSaveDirty = false;
	if (gSaveSize > 0)
	{
		Dump_GetSaveDirectory(gSaveFileName, g_ROM.szFileName, ext);
		DBGConsole_Msg(0, "Loading save from [C%s]", gSaveFileName);
		u8* pDst = (u8*)g_pMemoryBuffers[MEM_SAVE];

		fp = fopen(gSaveFileName, "rb");
		if (fp != NULL)
		{
			for ( u32 d = 0; d < gSaveSize; d += sizeof(buffer) )
			{
				fread(buffer, sizeof(buffer), 1, fp);

				for ( u32 i = 0; i < sizeof(buffer); i++ )
				{
					pDst[d+i] = buffer[i^U8_TWIDDLE];
				}
			}
			fclose(fp);
		}
		else
		{
			DBGConsole_Msg(0, "Save File [C%s] cannot be found.", gSaveFileName);
		}
	}
	// init mempack
	Dump_GetSaveDirectory(gMempackFileName, g_ROM.szFileName, ".mpk");
	DBGConsole_Msg(0, "Loading MemPack from [C%s]", gMempackFileName);
	fp = fopen(gMempackFileName, "rb");
	if (fp != NULL)
	{
		fread(g_pMemoryBuffers[MEM_MEMPACK], MemoryRegionSizes[MEM_MEMPACK], 1, fp);
		fclose(fp);
		gMempackDirty = false;
	}
	else
	{
		DBGConsole_Msg(0, "MemPack File [C%s] cannot be found.", gMempackFileName);
		InitMempackContent();
		gMempackDirty = true;
	}

	return true;
}

void Save::Flush(bool force)
{
	if ((gSaveDirty || force) && g_ROM.settings.SaveType != SAVE_TYPE_UNKNOWN)
	{
		u8		buffer[2048];
		u8 *	p_src = (u8*)g_pMemoryBuffers[MEM_SAVE];

		DBGConsole_Msg(0, "Saving to [C%s]", gSaveFileName);

		FILE *fp = fopen(gSaveFileName, "wb");
		if (fp != NULL)
		{
			for ( u32 d = 0; d < gSaveSize; d += sizeof(buffer) )
			{
				for ( u32 i = 0; i < sizeof(buffer); i++ )
				{
					buffer[i^U8_TWIDDLE] = p_src[d+i];
				}
				fwrite(buffer, 1, sizeof(buffer), fp);
			}
			fclose(fp);
		}
		gSaveDirty = false;
	}

	if (gMempackDirty || force)
	{
		DBGConsole_Msg(0, "Saving MemPack to [C%s]", gMempackFileName);

		FILE *fp = fopen(gMempackFileName, "wb");
		if (fp != NULL)
		{
			fwrite(g_pMemoryBuffers[MEM_MEMPACK], MemoryRegionSizes[MEM_MEMPACK], 1, fp);
			fclose(fp);
		}
		gMempackDirty = false;
	}
}

// Mempack Stuffs

//
// Initialisation values taken from PJ64
//

static void InitMempackContent()
{
	const u8 MempackInitilize[] =
	{
		0x81,0x01,0x02,0x03, 0x04,0x05,0x06,0x07, 0x08,0x09,0x0a,0x0b, 0x0C,0x0D,0x0E,0x0F,
		0x10,0x11,0x12,0x13, 0x14,0x15,0x16,0x17, 0x18,0x19,0x1A,0x1B, 0x1C,0x1D,0x1E,0x1F,
		0xFF,0xFF,0xFF,0xFF, 0x05,0x1A,0x5F,0x13, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
		0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0x01,0xFF, 0x66,0x25,0x99,0xCD,
		0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
		0xFF,0xFF,0xFF,0xFF, 0x05,0x1A,0x5F,0x13, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
		0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0x01,0xFF, 0x66,0x25,0x99,0xCD,
		0xFF,0xFF,0xFF,0xFF, 0x05,0x1A,0x5F,0x13, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
		0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0x01,0xFF, 0x66,0x25,0x99,0xCD,
		0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
		0xFF,0xFF,0xFF,0xFF, 0x05,0x1A,0x5F,0x13, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
		0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0x01,0xFF, 0x66,0x25,0x99,0xCD,
		0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
		0x00,0x71,0x00,0x03, 0x00,0x03,0x00,0x03, 0x00,0x03,0x00,0x03, 0x00,0x03,0x00,0x03,
	};

	size_t dataPtr;
	u8* mempack;

	for ( dataPtr = 0; dataPtr < MemoryRegionSizes[MEM_MEMPACK]; dataPtr += 32 * 1024)
	{
		mempack = (u8*)g_pMemoryBuffers[MEM_MEMPACK] + dataPtr;
		for ( u32 count2 = 0; count2 < 0x8000; count2 += 2 )
		{
			mempack[count2 + 0] = 0x00;
			mempack[count2 + 1] = 0x03;
		}
		memcpy(mempack,MempackInitilize,sizeof(MempackInitilize));

		DAEDALUS_ASSERT(dataPtr + 0x8000 <= MemoryRegionSizes[MEM_MEMPACK], "extend pointer");
		DAEDALUS_ASSERT(dataPtr + sizeof(MempackInitilize) <= MemoryRegionSizes[MEM_MEMPACK], "extend pointer");
	}
}
