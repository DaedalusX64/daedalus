#include <3ds.h>
#include <GL/picaGL.h>
#include <stdio.h>

#include "UserInterface.h"
#include "InGameMenu.h"

#include "BuildOptions.h"
#include "Config/ConfigOptions.h"
#include "Core/Cheats.h"
#include "Core/CPU.h"
#include "Core/Memory.h"
#include "Core/PIF.h"
#include "Core/RomSettings.h"
#include "Core/Save.h"
#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"
#include "Graphics/GraphicsContext.h"
#include "HLEGraphics/TextureCache.h"
#include "Input/InputManager.h"
#include "Interface/RomDB.h"
#include "System/Paths.h"
#include "System/System.h"
#include "Test/BatchTest.h"
#include "Utility/IO.h"
#include "Utility/Preferences.h"
#include "Utility/Profiler.h"
#include "Utility/Thread.h"
#include "Utility/Translate.h"
#include "Utility/ROMFile.h"
#include "Utility/Timer.h"

extern uint8_t aspectRatio;
extern float gCurrentFramerate;
extern EFrameskipValue gFrameskipValue;
extern RomInfo g_ROM;

static uint8_t currentPage = 0;

static void ExecSaveState(int slot)
{
	IO::Filename full_path;
	sprintf(full_path, "%s%s.ss%d", DAEDALUS_CTR_PATH("SaveStates/"), g_ROM.settings.GameName.c_str(), slot);

	CPU_RequestSaveState(full_path);
}

static void LoadSaveState(int slot)
{
	IO::Filename full_path;
	sprintf(full_path, "%s%s.ss%d", DAEDALUS_CTR_PATH("SaveStates/"), g_ROM.settings.GameName.c_str(), slot);

	CPU_RequestLoadState(full_path);
}

static bool SaveStateExists(int slot)
{
	IO::Filename full_path;
	sprintf(full_path, "%s%s.ss%d", DAEDALUS_CTR_PATH("SaveStates/"), g_ROM.settings.GameName.c_str(), slot);

	return IO::File::Exists(full_path);
}

static void DrawSaveStatePage()
{
	ImGui_Impl3DS_NewFrame();
	ImGui::SetNextWindowPos( ImVec2(0, 0) );
	ImGui::SetNextWindowSize( ImVec2(320, 240) );

	ImGui::Begin("Save state", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

	char buttonString[20];

	for(int i = 0; i < 5; i++)
	{
		sprintf(buttonString, "Save slot: %i", i);

		if(SaveStateExists(i))
		{
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0xff227ee6));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0xff129cf3));
		}
		else
		{	
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0xff60ae27));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0xff71cc2e));
		}

		if(ImGui::Button(buttonString, ImVec2(304, 30)))
		{
			ExecSaveState(i);
		}

		ImGui::PopStyleColor(2);
	}

	if(ImGui::Button("Cancel", ImVec2(304, 30))) currentPage = 0;

	ImGui::End();
	ImGui::Render();

	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
	
}

static void DrawLoadStatePage()
{
	ImGui_Impl3DS_NewFrame();
	ImGui::SetNextWindowPos( ImVec2(0, 0) );
	ImGui::SetNextWindowSize( ImVec2(320, 240) );

	ImGui::Begin("Load state", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

	char buttonString[20];

	for(int i = 0; i < 5; i++)
	{
		sprintf(buttonString, "Load slot: %i", i);

		if(SaveStateExists(i))
		{
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0xff227ee6));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0xff129cf3));
		}
		else
		{	
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0xff36342d));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0xff36342d));
		}

		if(ImGui::Button(buttonString, ImVec2(304, 30)))
		{
			LoadSaveState(i);
		}

		ImGui::PopStyleColor(2);
	}

	if(ImGui::Button("Cancel", ImVec2(304, 30))) currentPage = 0;

	ImGui::End();
	ImGui::Render();

	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
	
}

static void HelpMarker(const char* desc)
{
	ImGui::SameLine();
	ImGui::SetCursorPosX(284);
    ImGui::TextDisabled("(?)");

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

static SRomPreferences romPreferences;

void UI::LoadRomPreferences(RomID mRomID)
{
	CPreferences::Get()->GetRomPreferences( mRomID, &romPreferences );
}

bool UI::DrawOptionsPage(RomID mRomID)
{
	currentPage = 3;

	int currentSelection = 0;

	ImGui_Impl3DS_NewFrame();
	ImGui::SetNextWindowPos( ImVec2(0, 0) );
	ImGui::SetNextWindowSize( ImVec2(320, 240) );

	ImGui::Begin("Options", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

	ImGui::PushItemWidth(-1);

	ImGui::Spacing();

	ImGui::Text("Dynamic Recompilation");
	ImGui::Checkbox("##DynaRec", &romPreferences.DynarecEnabled);
	ImGui::SameLine();
	ImGui::Text(romPreferences.DynarecEnabled ? "Enabled" : "Disabled");

	ImGui::Spacing();

	ImGui::Text("Dynarec Loop Optimizations");
	HelpMarker("Enable loop optimizations for better performance\n Can cause freezes!!!");
	ImGui::Checkbox("##speedsync", (bool*)&romPreferences.DynarecLoopOptimisation);
	ImGui::SameLine();
	ImGui::Text(romPreferences.DynarecLoopOptimisation ? "Enabled" : "Disabled");

	ImGui::Spacing();

	ImGui::Text("Limit Framerate");
	ImGui::Checkbox("##speedsync", (bool*)&romPreferences.SpeedSyncEnabled);
	ImGui::SameLine();
	ImGui::Text(romPreferences.SpeedSyncEnabled ? "Enabled" : "Disabled");

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	const char* audioOptions[] = { "Disabled", "Asynchronous", "Synchronous" };
	ImGui::Text("Audio Plugin");
	currentSelection = (int)romPreferences.AudioEnabled;
	ImGui::Combo("##audio_combo", &currentSelection, audioOptions, 3);
	romPreferences.AudioEnabled = EAudioPluginMode(currentSelection);

	ImGui::Spacing();

	ImGui::Text("Sync Audio Rate");
	HelpMarker("Speeds up audio logic to match framerate.");
	ImGui::Checkbox("##AudioRateMatch", &romPreferences.AudioRateMatch);
	ImGui::SameLine();
	ImGui::Text(romPreferences.AudioRateMatch ? "Enabled" : "Disabled");

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::Text("Sync Video Rate");
	HelpMarker("Speeds up video logic to match framerate.");
	ImGui::Checkbox("##VideoRateMatch", &romPreferences.VideoRateMatch);
	ImGui::SameLine();
	ImGui::Text(romPreferences.VideoRateMatch ? "Enabled" : "Disabled");

	ImGui::Spacing();

	const char* hashOptions[] = { "Disabled", "Every frame", "Every 2 frames", "Every 4 frames", "Every 8 frames", "Every 16 frames", "Every 32 frames" };
	ImGui::Text("Texture Hash Check Frequency");
	HelpMarker( "Frequency in which to check for texture changes.\n"
				"Disabled is the fastest, but can cause graphical glitches.\n");
	currentSelection = (int)romPreferences.CheckTextureHashFrequency;
	ImGui::Combo("##hash_frequency", &currentSelection, hashOptions, NUM_THF);
	romPreferences.CheckTextureHashFrequency = ETextureHashFrequency(currentSelection);

	ImGui::Spacing();

	const char* frameskipOptions[] = { "Disabled", "Auto 1", "Auto 2", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
	ImGui::Text("Frameskip");
	currentSelection = (int)romPreferences.Frameskip;
	ImGui::Combo("##frameskip_combo", &currentSelection, frameskipOptions, NUM_FRAMESKIP_VALUES);
	romPreferences.Frameskip = EFrameskipValue(currentSelection);

	ImGui::Spacing();

	if( ImGui::Button("Cancel", ImVec2(144, 30)) )
	{
		currentPage = 0;
	}

	ImGui::SameLine(0, 4);

	if( ImGui::Button("Save", ImVec2(144, 30)) )
	{
		CPreferences::Get()->SetRomPreferences( mRomID, romPreferences );
		CPreferences::Get()->Commit();
	
		currentPage = 0;
	}

	ImGui::PopItemWidth();
	ImGui::End();
	ImGui::Render();

	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
	
	romPreferences.Apply();

	return currentPage == 3;
}

static void showFPS()
{
	ImGui::SetNextWindowPos( ImVec2(250,0) );
	ImGui::SetNextWindowSize( ImVec2(70, 20) );

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 2)); 
	ImGui::BeginTooltip();
	ImGui::Text("FPS: %.2f", gCurrentFramerate);
	ImGui::End();
	ImGui::PopStyleVar();

}

static void DrawMainPage()
{
	//sprintf(titleString, "FPS: %.2f", gCurrentFramerate);

	ImGui_Impl3DS_NewFrame();
	ImGui::SetNextWindowPos( ImVec2(0, 0) );
	ImGui::SetNextWindowSize( ImVec2(320, 240) );

	ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

	if( ImGui::Button("Save State", ImVec2(304, 60)) ) currentPage = 1;
	if( ImGui::Button("Load State", ImVec2(304, 60)) ) currentPage = 2;

	if( ImGui::Button("Close ROM", ImVec2(150, 60)) )  ImGui::OpenPopup("Are you sure?");

	ImGui::SameLine(0, 4);

	if( ImGui::Button("Options", ImVec2(150, 60)) )
	{
		UI::LoadRomPreferences( g_ROM.mRomID );
		currentPage = 3;
	}

	ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Are you sure?", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::Text("Any unsaved progress will be lost\n\n");

        if (ImGui::Button("OK", ImVec2(120, 0)))
        { 
        	currentPage = 0;
	 		CPU_Halt("Exiting");
	 		ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
        	ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

	ImGui::End();

	showFPS();

	ImGui::Render();

	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

void UI::DrawInGameMenu()
{
	UI::RestoreRenderState();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	switch(currentPage)
	{
		case 0: DrawMainPage(); break;
		case 1: DrawSaveStatePage(); break;
		case 2: DrawLoadStatePage(); break;
		case 3: DrawOptionsPage(g_ROM.mRomID); break;
	}

	pglSwapBuffers();

	pglSelectScreen(GFX_TOP, GFX_LEFT);
}