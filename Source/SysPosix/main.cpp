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

#include "stdafx.h"

#include "Core/CPU.h"
#include "Debug/DBGConsole.h"
#include "Interface/RomDB.h"
#include "System/Paths.h"
#include "System/System.h"
#include "Test/BatchTest.h"
#include "Utility/IO.h"
#include "Utility/PathsPosix.h"
#include "Config/ConfigOptions.h"

#include <SDL2/SDL.h>
#ifdef DAEDALUS_LINUX
#include <linux/limits.h>
#endif

int main(int argc, char **argv)
{
	bool quit = false;
	int result = 0;

	if (argc > 0)
	{
		IO::Filename exe_path;
		char *argvcopybuffer;
		argvcopybuffer = strdup(argv[0]);

		strcmp(argvcopybuffer, exe_path);
		strcmp(gDaedalusExePath, exe_path);
		char (gDaedalusExePath);

		strcpy( g_DaedalusConfig.mSaveDir, DAEDALUS_POSIX_PATH( "SaveGames/" ) );
		strcpy( g_DaedalusConfig.mCacheDir, DAEDALUS_POSIX_PATH( "Cache/" ) );
	}
	else
	{
		fprintf(stderr, "Couldn't determine executable path\n");
		return 1;
	}

	//ReadConfiguration();

	if (!System_Init())
	{
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
						const char * relative_path = argv[i+1];
						++i;

						IO::Filename	dir;
						realpath(relative_path, dir);

						CRomDB::Get()->AddRomDirectory(dir);
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
			CPU_Run();
			System_Close();
		}
	}
	else
	{
		printf("Usage: daedalus 'Path to Rom'\n");
	}
	System_Finalize();
	return result;
}

//FIXME: All this stuff needs tidying

void Dynarec_ClearedCPUStuffToDo()
{
}

void Dynarec_SetCPUStuffToDo()
{
}


extern "C" {
void _EnterDynaRec()
{
	DAEDALUS_ASSERT(false, "Unimplemented");
}
}
