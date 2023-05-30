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

#include "BuildOptions.h"
#include "Base/Types.h"

#include "Core/ROM.h"
#include "Core/Memory.h"
#include "Core/Save.h"
#include "Config/ConfigOptions.h"
#include "Debug/DBGConsole.h"
#include "Debug/Dump.h"
#include "System/IO.h"
#include "Interface/RomIndex.h"


#include <iostream>
#include <fstream> 
#include <vector> 

static void InitMempackContent();

static std::filesystem::path gSaveFileName;
static bool				gSaveDirty;
static u32				gSaveSize;
static std::filesystem::path	gMempackFileName;
static bool				gMempackDirty;
static std::filesystem::path ext; 


constexpr std::size_t BUFFER_SIZE = 2048;

bool Save_Reset()
{
	switch (data.saveType)
	{
		std::cout << "Accessing Save Reset " << std::endl;
	case ESaveType::EEP4K:
		ext /= ".sav";
		gSaveSize = 4 * 1024;
		break;
	case ESaveType::EEP16K:
		ext /= ".sav";
		gSaveSize = 16 * 1024;
		break;
	case ESaveType::SRAM:
		ext /= ".sra";
		gSaveSize = 32 * 1024;
		break;
	case ESaveType::FLASH:
		ext /= ".fla";
		gSaveSize = 128 * 1024;
		break;
	default:
		ext /= "";
		gSaveSize = 0;
		break;
			std::cout << "Extension is: " << ext << std::endl;
	}
	std::cout << "Game Name in Save " << data.gameName << std::endl;
	DAEDALUS_ASSERT( gSaveSize <= MemoryRegionSizes[MEM_SAVE], "Save size is larger than allocated memory");
gSaveFileName = Save_As( g_ROM.mFileName, ext, "SaveGames");
    std::ifstream infile(gSaveFileName, std::ios::binary);
std::cout << "Game Name: "<<  data.gameName << std::endl;

gSaveDirty = false;

if (gSaveSize > 0)
{

    if (infile.is_open())
    {
        DBGConsole_Msg(0, "Loading save from [C%s]", g_ROM.mFileName.c_str());

        u8 buffer[BUFFER_SIZE];
        u8* dst = (u8*)g_pMemoryBuffers[MEM_SAVE];

        while (infile.read((char*)buffer, BUFFER_SIZE))
        {
            for (u32 i = 0; i < BUFFER_SIZE; i++)
            {
                dst[infile.gcount() - BUFFER_SIZE + i] = buffer[i ^ U8_TWIDDLE];
            }
        }
        infile.close();
    }
    else
    {
        DBGConsole_Msg(0, "Save File [C%s] cannot be found.", gSaveFileName.c_str());
    }
}


	// init mempack, we always assume the presence of the mempack for simplicity 
	{	
		gMempackFileName = Save_As(g_ROM.mFileName, ".mpk", "SaveGames/");
		FILE * fp = fopen(gSaveFileName.c_str(), "rb");
		if (fp != nullptr)
		{
			DBGConsole_Msg(0, "Loading MemPack from [C%s]", gSaveFileName.c_str());
			fread(g_pMemoryBuffers[MEM_MEMPACK], MemoryRegionSizes[MEM_MEMPACK], 1, fp);
			fclose(fp);
			gMempackDirty = false;
		}
		else
		{
			DBGConsole_Msg(0, "MemPack File [C%s] cannot be found.", gSaveFileName.c_str());
			InitMempackContent();
			gMempackDirty = true;
		}
	}

	return true;
}

void Save_Fini()
{
	gSaveDirty = true;
	gMempackDirty = true;
	
	Save_Flush();
}

void Save_MarkSaveDirty()
{
	gSaveDirty = true;
}

void Save_MarkMempackDirty()
{
	gMempackDirty = true;
}
void Save_Flush() {
    if (gSaveDirty && data.saveType != ESaveType::NONE) {
        std::cout << "Saving to [" << gSaveFileName << "]" << std::endl;

        std::ofstream outfile(gSaveFileName, std::ios::out | std::ios::binary);
        if (outfile.is_open()) {
            auto src = reinterpret_cast<u8*>(g_pMemoryBuffers[MEM_SAVE]);
            std::vector<u8> buffer(BUFFER_SIZE);

            for (std::size_t d = 0; d < gSaveSize; d += BUFFER_SIZE) {
                std::transform(src + d, src + d + BUFFER_SIZE, buffer.begin(), [](u8 val) {
                    return val ^ U8_TWIDDLE;
                });
                outfile.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
            }
            outfile.close();
        }
        gSaveDirty = false;
    }

    if (gMempackDirty) {
        std::cout << "Saving MemPack to [" << gMempackFileName << "]" << std::endl;

        std::ofstream outfile(gMempackFileName, std::ios::out | std::ios::binary);
        if (outfile.is_open()) {
            outfile.write(reinterpret_cast<char*>(g_pMemoryBuffers[MEM_MEMPACK]), MemoryRegionSizes[MEM_MEMPACK]);
            outfile.close();
        }
        gMempackDirty = false;
    }
}
// Mempack Stuffs

//
// Initialisation values taken from PJ64
//
static const u8 gMempackInitialize[] =
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
	0x00,0x71,0x00,0x03, 0x00,0x03,0x00,0x03, 0x00,0x03,0x00,0x03, 0x00,0x03,0x00,0x03
};

static void InitMempackContent()
{
	for (size_t dst_off = 0; dst_off < MemoryRegionSizes[MEM_MEMPACK]; dst_off += 32 * 1024)
	{
		u8 * mempack = (u8*)g_pMemoryBuffers[MEM_MEMPACK] + dst_off;

		memcpy(mempack, gMempackInitialize, 272);

		for (u32 i = 272; i < 0x8000; i += 2)
		{
			mempack[i]   = 0x00;
			mempack[i+1] = 0x03;
		}

		DAEDALUS_ASSERT(dst_off + 0x8000 <= MemoryRegionSizes[MEM_MEMPACK], "Buffer overflow");
		DAEDALUS_ASSERT(dst_off + sizeof(gMempackInitialize) <= MemoryRegionSizes[MEM_MEMPACK], "Buffer overflow");
	}
}
