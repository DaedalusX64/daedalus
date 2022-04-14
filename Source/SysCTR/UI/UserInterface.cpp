#include <3ds.h>
#include <GL/picaGL.h>
#include <stdio.h>

#include "UserInterface.h"

//Loads the font
void UI::Initialize()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();

	io.Fonts->AddFontFromFileTTF("romfs:/Roboto-Medium.ttf", 14);
	// Setup Platform/Renderer bindings
	ImGui_Impl3DS_Init();
	ImGui_ImplOpenGL2_Init();

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	ImGui::GetStyle().WindowRounding = 0.0f;
}

void UI::RestoreRenderState()
{
	pglSelectScreen(GFX_BOTTOM, GFX_LEFT);

	glClearColor(0.1f, 0.1f, 0.1f, 0.0f);

	glViewport(0,0,320,240);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_CULL_FACE);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);

	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glMatrixMode(GL_PROJECTION);
	glOrtho(0, 320, 240, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}