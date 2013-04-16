#include "stdafx.h"
#include "UI.h"
#include "Core/CPU.h"
#include "SysGL/GL.h"
#include "Utility/IO.h"

static void HandleKeys(int key, int state)
{
	if (state)
	{
		if (key >= '0' && key <= '9')
		{
			int idx = key - '0';

			bool ctrl_down = glfwGetKey(GLFW_KEY_LCTRL) || glfwGetKey(GLFW_KEY_RCTRL);

			char buf[64];
			sprintf(buf, "quick%d.save", idx);
			IO::Path::PathBuf filename;
			IO::Path::Combine(filename, gDaedalusExePath, buf);

			if (ctrl_down)
				CPU_RequestSaveState(filename);
			else
				CPU_RequestLoadState(filename);	
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
