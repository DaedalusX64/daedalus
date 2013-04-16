#include "stdafx.h"
#include "UI.h"
#include "Core/CPU.h"
#include "SysGL/GL.h"
#include "Utility/IO.h"

static void HandleSystemKeys(void * arg)
{
	// Debounce keys
	static bool save_was_pressed = false;
	static bool load_was_pressed = false;

	bool save_pressed = glfwGetKey( GLFW_KEY_F9 );
	bool load_pressed = glfwGetKey( GLFW_KEY_F8 );

	if (save_pressed && !save_was_pressed)
	{
		IO::Path::PathBuf filename;
		IO::Path::Combine(filename, gDaedalusExePath, "quick.save");
		CPU_RequestSaveState(filename);
	}
	if (load_pressed && !load_was_pressed)
	{
		IO::Path::PathBuf filename;
		IO::Path::Combine(filename, gDaedalusExePath, "quick.save");
		CPU_RequestLoadState(filename);
	}

	if (glfwGetKey(GLFW_KEY_ESC))
	{
		CPU_Halt("Escape");
	}

	save_was_pressed = save_pressed;
	load_was_pressed = load_pressed;
}

bool UI_Init()
{
	CPU_RegisterVblCallback(&HandleSystemKeys, NULL);
	return true;
}

void UI_Finalise()
{
	CPU_UnregisterVblCallback(&HandleSystemKeys, NULL);	
}
