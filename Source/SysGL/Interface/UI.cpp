
#include "Base/Types.h"
#include "SysGL/Interface/UI.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <filesystem>
#include <iostream> 
#include <sstream>

#include "Core/CPU.h"
#include "Core/ROM.h"

#include "SysGL/GL.h"
#include "System/IO.h"
#include "System/Thread.h"

// TODO: Implemenent fullscreen toggle and window resize
static bool toggle_fullscreen = false;

static s32 get_saveslot_from_keysym(s32 keysym)
{
    switch (keysym) 
	{
    case SDL_SCANCODE_0:
        return 0;
    case SDL_SCANCODE_1:
        return 1;
    case SDL_SCANCODE_2:
        return 2;
    case SDL_SCANCODE_3:
        return 3;
    case SDL_SCANCODE_4:
        return 4;
    case SDL_SCANCODE_5:
        return 5;
    case SDL_SCANCODE_6:
        return 6;
    case SDL_SCANCODE_7:
        return 7;
    case SDL_SCANCODE_8:
        return 8;
    case SDL_SCANCODE_9:
        return 9;
    default:
        return -1;
    }
}

static void PollKeyboard(void * arg)
{
	SDL_Event event;
	while (SDL_PollEvent( &event) != 0)
	{
		if (event.type == SDL_QUIT)
		{
			CPU_Halt("Window Closed");	// SDL window was closed
            // Optionally, you can also call SDL_Quit() to terminate SDL subsystems
            SDL_Quit();
            // Exit the application
            exit(0);
		}
		else if(event.type == SDL_KEYDOWN)
		{
			if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
			{
				CPU_Halt("Window Closed");	// User pressed escape to exit
			}

			if (event.key.keysym.scancode == SDL_SCANCODE_F11)
			{
				if (toggle_fullscreen == false) {
					SDL_SetWindowFullscreen(gWindow, SDL_TRUE);
					toggle_fullscreen = true;
				}
				else
				{
					SDL_SetWindowFullscreen(gWindow, SDL_FALSE);
					toggle_fullscreen = false;
				}
			}

			s32 idx = -1;
			if ((idx = get_saveslot_from_keysym(event.key.keysym.scancode)) >= 0)
			{
				std::ostringstream filename_ss;
				filename_ss << "saveslot" << idx << ".ss";

				std::filesystem::path path_sub = IO::Path::FindFileName(g_ROM.settings.GameName.c_str());
				std::filesystem::path path_ss = "SaveStates/";
				std::filesystem::path filename = path_ss /= path_sub /= filename_ss.str();

				std::cout << "SaveState Name:" << filename << std::endl;

				bool ctrl_down = event.key.keysym.mod & ( KMOD_LCTRL | KMOD_RCTRL );
				if (ctrl_down)
				{
					CPU_RequestSaveState(filename);
				}
				else
				{
					if (std::filesystem::exists(filename))
					{
						CPU_RequestLoadState(filename);
					}
				}
			}
		}
	}
}

bool UI_Init()
{
	DAEDALUS_ASSERT(gWindow != nullptr, "The SDL window should already have been initialised");

	CPU_RegisterVblCallback(&PollKeyboard, nullptr);
	return true;
}

void UI_Finalise()
{
	CPU_UnregisterVblCallback(&PollKeyboard, nullptr);
}
