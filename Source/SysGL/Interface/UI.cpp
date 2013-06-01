#include "stdafx.h"
#include "UI.h"

#include <stdio.h>

#include "Core/CPU.h"
#include "SysGL/GL.h"
#include "System/Paths.h"
#include "Utility/IO.h"

static void GLFWCALL HandleKeys(int key, int state)
{
	if (state)
	{
		if (key >= '0' && key <= '9')
		{
			int idx = key - '0';

			bool ctrl_down = glfwGetKey(GLFW_KEY_LCTRL) || glfwGetKey(GLFW_KEY_RCTRL);

			IO::Path::PathBuf path;
			IO::Path::PathBuf filename;

			IO::Path::Combine(path, gDaedalusExePath, "SaveStates");
			IO::Directory::EnsureExists( path );		// Ensure this dir exists

			char buf[64];
			sprintf(buf, "quick%d.save", idx);		//Why not use the same name format as PSP? SaveSlotXX.ss
			IO::Path::Combine(filename, path, buf);


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

		if (key == GLFW_KEY_ESC)
		{
			CPU_Halt("Escape");
		}
	}
}

static void PollKeyboard(void * arg)
{
	glfwPollEvents();
}

bool UI_Init()
{
	glfwSetKeyCallback(&HandleKeys);
	CPU_RegisterVblCallback(&PollKeyboard, NULL);
	return true;
}

void UI_Finalise()
{
	CPU_UnregisterVblCallback(&PollKeyboard, NULL);
}
