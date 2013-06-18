#include "stdafx.h"
#include "UI.h"

#include <stdio.h>

#include "Core/CPU.h"
#include "Core/ROM.h"

#include "SysGL/GL.h"
#include "System/Paths.h"
#include "Utility/IO.h"
#include "Utility/Thread.h"

//static bool toggle_fullscreen = false;
static void HandleKeys(GLFWwindow * window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if (key >= '0' && key <= '9')
		{
			int idx = key - '0';

			bool ctrl_down = (mods & GLFW_MOD_CONTROL) != 0;

			char filename_ss[64];
			sprintf( filename_ss, "saveslot%u.ss", idx );

			IO::Filename path_sub;
			sprintf( path_sub, "SaveStates\\%s", g_ROM.settings.GameName.c_str());

			IO::Filename path_ss;
			IO::Path::Combine( path_ss, gDaedalusExePath, path_sub );
			IO::Directory::EnsureExists( path_ss );		// Ensure this dir exists

			IO::Filename filename;
			IO::Path::Combine(filename, path_ss, filename_ss);

			if (ctrl_down)
			{
				CPU_RequestSaveState(filename);
			}
			else
			{
				if (IO::File::Exists(filename))
				{
					CPU_RequestLoadState(filename);
				}
			}
		}
// Wait for GLF 3 which adds proper full screen support
#if 0
		if(key == GLFW_KEY_F1)
		{
			// Toggle fullscreen on/off
			toggle_fullscreen ^= 1;

			u32 width = 640; //SCR_WIDTH
			u32 height= 480; //SCR_HEIGHT
			if(toggle_fullscreen)
			{
				// Get destop resolution, this should tell us the max resolution we can use
				GLFWvidmode info;
				glfwGetDesktopMode( &info );
				width = info.Width;
				height= info.Height;
			}

			// Arg need to close and re open window to toggle fullscreen :(
			// Hopefully future releases of GLFW should make this simpler
			glfwCloseWindow();

			if( !glfwOpenWindow( width, height,
						0,0,0,0,
						24,
						0,
						toggle_fullscreen ?  GLFW_FULLSCREEN : GLFW_WINDOW ) )
			{
				// FIX ME: What to do here? Should exit?
				fprintf( stderr, "Failed to re open GLFW window!\n" );
				//glfwTerminate();
				return;
			}
		}
#endif
		if (key == GLFW_KEY_ESCAPE)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}
}

static void PollKeyboard(void * arg)
{
	glfwPollEvents();
	if (glfwWindowShouldClose(gWindow))
		CPU_Halt("Window Closed");
}

bool UI_Init()
{
	DAEDALUS_ASSERT(gWindow != NULL, "The GLFW window should already have been initialised");
	glfwSetKeyCallback(gWindow, &HandleKeys);
	CPU_RegisterVblCallback(&PollKeyboard, NULL);
	return true;
}

void UI_Finalise()
{
	CPU_UnregisterVblCallback(&PollKeyboard, NULL);
}
