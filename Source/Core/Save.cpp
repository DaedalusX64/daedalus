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
#include "Debug/Dump.h"		// Dump_GetSaveDirectory()

#include "ConfigOptions.h"

static void InitMempackContent();

char	Save::szSaveFileName[MAX_PATH+1];
bool	Save::SaveDirty;
u32		Save::nSaveSize;

char	Save::szMempackFileName[MAX_PATH+1];
bool	Save::mempackDirty;


void Save::Reset()
{
	const char	*ext;
	FILE 		*fp;
	u8			buffer[2048];

	IO::Directory::EnsureExists(g_DaedalusConfig.szSaveDir);

	switch(g_ROM.settings.SaveType)
	{
	case SAVE_TYPE_EEP4K:
		ext = ".sav";
		nSaveSize = 4 * 1024;
		break;
	case SAVE_TYPE_EEP16K:
		ext = ".sav";
		nSaveSize = 16 * 1024;
		break;
	case SAVE_TYPE_SRAM:
		ext = ".sra";
		nSaveSize = 32 * 1024;
		break;
	case SAVE_TYPE_FLASH:
		ext = ".fla";
		nSaveSize = 128 * 1024;
		break;
	default:
		ext = "";
		nSaveSize = 0;
		break;
	}

	DAEDALUS_ASSERT( nSaveSize <=  MemoryRegionSizes[MEM_SAVE], "Save size is larger than allocated memory");
	SaveDirty = false;
	if (nSaveSize > 0)
	{
		Dump_GetSaveDirectory(szSaveFileName, g_ROM.szFileName, ext);
		DBGConsole_Msg(0, "Loading save from [C%s]", szSaveFileName);
		u8* pDst = (u8*)g_pMemoryBuffers[MEM_SAVE];

		fp = fopen(szSaveFileName, "rb");
		if (fp != NULL)
		{
			for ( u32 d = 0; d < nSaveSize; d += sizeof(buffer) )
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
			DBGConsole_Msg(0, "Save File [C%s] cannot be found.", szSaveFileName);
		}
	}
	// init mempack
	Dump_GetSaveDirectory(szMempackFileName, g_ROM.szFileName, ".mpk");
	DBGConsole_Msg(0, "Loading MemPack from [C%s]", szMempackFileName);
	fp = fopen(szMempackFileName, "rb");
	if (fp != NULL)
	{
		fread(g_pMemoryBuffers[MEM_MEMPACK], MemoryRegionSizes[MEM_MEMPACK], 1, fp);
		fclose(fp);
		mempackDirty = false;
	}
	else
	{
		DBGConsole_Msg(0, "MemPack File [C%s] cannot be found.", szMempackFileName);
		InitMempackContent();
		mempackDirty = true;
	}
}

void Save::Flush(bool force)
{
	if ((SaveDirty || force) && g_ROM.settings.SaveType != SAVE_TYPE_UNKNOWN)
	{
		u8		buffer[2048];
		u8 *	p_src = (u8*)g_pMemoryBuffers[MEM_SAVE];

		DBGConsole_Msg(0, "Saving to [C%s]", szSaveFileName);

		FILE *fp = fopen(szSaveFileName, "wb");
		if (fp != NULL)
		{
			for ( u32 d = 0; d < nSaveSize; d += sizeof(buffer) )
			{
				for ( u32 i = 0; i < sizeof(buffer); i++ )
				{
					buffer[i^U8_TWIDDLE] = p_src[d+i];
				}
				fwrite(buffer, 1, sizeof(buffer), fp);
			}
			fclose(fp);
		}
		SaveDirty = false;
	}

	if (mempackDirty || force)
	{
		DBGConsole_Msg(0, "Saving MemPack to [C%s]", szMempackFileName);

		FILE *fp = fopen(szMempackFileName, "wb");
		if (fp != NULL)
		{
			fwrite(g_pMemoryBuffers[MEM_MEMPACK], MemoryRegionSizes[MEM_MEMPACK], 1, fp);
			fclose(fp);
		}
		mempackDirty = false;
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
