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


#include "Base/Types.h"

#include "Core/CPU.h"
#include "Debug/DBGConsole.h"
#include "Interface/RomDB.h"
#include "System/SystemInit.h"
#include "Test/BatchTest.h"
#include "System/IO.h"
#include "Config/ConfigOptions.h"


#include <SDL2/SDL.h>
#include <vector>
#include <filesystem>
#include <iostream>
#include <algorithm>

#ifdef DAEDALUS_LINUX
#include <linux/limits.h>
#endif

int main(int argc, char **argv)
{
	int result = 0;

	//ReadConfiguration();

	if (!System_Init())
	{
		fprintf(stderr, "System_Init failed\n");
		return 1;
	}


	if (argc > 1)
	{
		bool 			batch_test = false;
		const char *	filename   = NULL;

		for (int i = 1; i < argc; ++i)
		{
			const char * arg = argv[i];
			if (*arg == '-')
			{
				++arg;
				if( strcmp( arg, "-batch" ) == 0 )
				{
					batch_test = true;
					break;
				}
				else if (strcmp( arg, "-roms" ) == 0 )
				{
					if (i+1 < argc)
					{
						const char *relative_path = argv[i+1];
						++i;

						char* dir = realpath(relative_path, nullptr);
						CRomDB::Get()->AddRomDirectory(dir);
						free(dir);
					}
				}
			}
			else
			{
				filename = arg;
			}
		}

		if (batch_test)
		{
#ifdef DAEDALUS_BATCH_TEST_ENABLED
				BatchTestMain(argc, argv);
#else
				fprintf(stderr, "BatchTest mode is not present in this build.\n");
#endif
		}
		else if (filename)
		{
			System_Open( filename );

			//
			// Commit the preferences and roms databases before starting to run
			//
			CRomDB::Get()->Commit();
			//CPreferences::Get()->Commit();

			CPU_Run();
			System_Close();
		}
	}
	else
	{
		std::filesystem::path filename;
		std::filesystem::path RomDir = "Roms";
		std::vector<std::filesystem::path> RomList;
		
		// Build up a vector of Roms from RomList
		for (auto const &dir_entry : std::filesystem::directory_iterator{RomDir})
		{
			RomList.push_back(dir_entry);
		}

		// Sort alphabetically. 
		std::sort(RomList.begin(), RomList.end()); 
		
		// Iterate over RomList and add numbers to be able to select 
		// This will eventually change to keyboard input / D-Pad control

		int count = 0;
		
		for (auto n : RomList)
		{
			std::cout << count << "." << n << std::endl;
			count++;
		}
		
		int game = 0;
		
		// Select the game using the outputted numbers
		std::cout << "Select a Game: " << std::endl;
		std::cin >> game;
		// 
		if(game > RomList.size())
		{
			std::cout << "Invalid Entry" << std::endl;
		}
		else
		{
			std::cout << "You have Selected: " << RomList.at(game);
			filename = RomList.at(game);
			std::cout << "Filename is: " << filename << std::endl;
			
			System_Open(filename);
			CRomDB::Get()->Commit();
			CPU_Run();
			System_Close();
		}
	}
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
