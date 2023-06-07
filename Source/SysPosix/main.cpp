/*
Copyright (C) 2012 StrmnNrmn

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

#include "Core/CPU.h"
#include "Debug/DBGConsole.h"
#include "Interface/RomDB.h"
#include "System/SystemInit.h"
#include "Test/BatchTest.h"
#include "System/IO.h"
#include "Config/ConfigOptions.h"
#include "Interface/RomIndex.h"
#include "Core/RomSettings.h"
#include "Interface/RomIndex.h"
#include "Core/Save.h"

#include <SDL2/SDL.h>
#include <vector>
#include <filesystem>
#include <iostream>
#include <map>

#ifdef DAEDALUS_LINUX
#include <linux/limits.h>
#endif

		std::filesystem::path filename;
	GameData romData; 


int main(int argc, char **argv)
{
	int result = 0;
	//ReadConfiguration();

       auto gameinfo = index("Roms");
	if (!System_Init())
	{
		fprintf(stderr, "System_Init failed\n");
		return 1;
	}


if (argc > 1) {
    std::string filenameToFind = argv[1];
    
    auto it = findGameByFilename(gameinfo, filenameToFind);
    if (it != gameinfo.end()) {
        const std::string& gameKey = it->first;
        romData = it->second;
        std::cout << "Game found with key: " << gameKey << std::endl;
        std::cout << "Game name: " << romData.gameName << std::endl;
        std::cout << "Preview image: " << romData.previewImage << std::endl;
		std::cout << "Save Type : " << static_cast<u32>(romData.saveType) << std::endl;
		System_Open( romData.file );
    } else {
        std::cout << "Game with filename \"" << filenameToFind << "\" not found." << std::endl;
    }
} else {
    std::cout << "Please provide a filename as a command-line argument." << std::endl;
}

			// System_Open( gamedata.file );
	
			//
			// Commit the preferences and roms databases before starting to run
			//
			// Get the Rom Info here

			//CPreferences::Get()->Commit();

			CPU_Run();
			System_Close();
		// }
	
	System_Finalize();
	return result;
}

//FIXME: All this stuff needs tidying

void Dynarec_ClearedCPUStuffToDo()
{}

void Dynarec_SetCPUStuffToDo()
{
}


extern "C" {
void _EnterDynaRec()
{
	DAEDALUS_ASSERT(false, "Unimplemented");
}
}
