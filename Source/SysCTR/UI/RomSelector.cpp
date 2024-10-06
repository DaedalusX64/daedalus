#include "UserInterface.h"
#include "InGameMenu.h"
#include "RomSelector.h"

#include <3ds.h>
#include <GL/picaGL.h>
#include <string>
#include <algorithm>
#include <vector>

#include "Interface/ConfigOptions.h"
#include "Interface/Cheats.h"
#include "Core/CPU.h"
#include "Core/PIF.h"
#include "RomFile/RomSettings.h"
#include "Core/Save.h"
#include "Interface/RomDB.h"
#include "System/SystemInit.h"
#include "Utility/Paths.h"

#include "Interface/Preferences.h"
#include "RomFile/RomFile.h"
#include "Graphics/NativeTexture.h"

#define DAEDALUS_CTR_PATH(p)	"sdmc:/3ds/DaedalusX64/" p

struct SRomInfo
{
	std::filesystem::path	mFilename;

	RomID			mRomID;
	u32				mRomSize;
	ECicType		mCicType;

	RomSettings		mSettings;
};

bool operator<(const SRomInfo & lhs, const SRomInfo & rhs)
{
    return std::string(lhs.mSettings.GameName) < std::string(rhs.mSettings.GameName);
}

static std::vector<SRomInfo> PopulateRomList()
{
	std::vector<SRomInfo> roms = {};

	std::filesystem::path roms_path = setBasePath("Roms");

	  if (std::filesystem::exists(roms_path) && std::filesystem::is_directory(roms_path))
    {
        for (const auto& entry : std::filesystem::directory_iterator(roms_path))
        {
            if (entry.is_regular_file())
            {
                const std::filesystem::path& rom_filename = entry.path();
                
                if (std::find(valid_extensions.begin(), valid_extensions.end(), rom_filename.extension()) != valid_extensions.end())
                {
                    SRomInfo info;
                    std::string full_path = rom_filename.string();

                    info.mFilename = rom_filename.filename().string();

                    if (ROM_GetRomDetailsByFilename(full_path.c_str(), &info.mRomID, &info.mRomSize, &info.mCicType))
                    {
                        if (!CRomSettingsDB::Get()->GetSettings(info.mRomID, &info.mSettings))
                        {
                            std::string game_name;

                            info.mSettings.Reset();
                            info.mSettings.Comment = "Unknown";

                            if (!ROM_GetRomName(full_path.c_str(), game_name)) game_name = full_path;

                            game_name = game_name.substr(0, 63);
                            info.mSettings.GameName = game_name.c_str();
                            CRomSettingsDB::Get()->SetSettings(info.mRomID, info.mSettings);
                        }
                    }
                    else
                    {
                        info.mSettings.GameName = "Unknown";
                    }

                    roms.push_back(info);
                }
            }
        }
    }

	std::stable_sort(roms.begin(), roms.end());

	return roms;
}

static bool GetRomNameForList(void* data, int idx, const char** out_text)
{
	std::vector<SRomInfo> *roms = (std::vector<SRomInfo>*) data;

	if(hidKeysHeld() & KEY_L)
		*out_text = roms->at(idx).mFilename.c_str();
	else
		*out_text = roms->at(idx).mSettings.GameName.c_str();

	return true;
}

std::string UI::DrawRomSelector()
{
	static int currentItem = 0;

	bool configure = false;
	bool selected = false;

	std::vector<SRomInfo> roms = PopulateRomList();

	UI::RestoreRenderState();
	
	if(roms.empty())
	{
		pglExit();
		gfxExit();

		gfxInitDefault();
		consoleInit(GFX_BOTTOM, NULL);

		printf("No ROMs found!\n\n");
		printf("Add ROMs to sdmc:/3ds/DaedalusX64/Roms\n\n\n");
		printf("Press START to exit\n");

		while(aptMainLoop())
		{
			hidScanInput();

			if(hidKeysDown() == KEY_START)
				exit(1);
		}
	}

	bool selection_changed = true;

	while(aptMainLoop() && !selected)
	{

		gspWaitForVBlank();
		hidScanInput();

		pglSelectScreen(GFX_BOTTOM, GFX_LEFT);

		glDisable(GL_SCISSOR_TEST);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_Impl3DS_EnableGamepad(true);
		ImGui_Impl3DS_NewFrame(GFX_BOTTOM);
		
		ImGui::SetNextWindowPos(  ImVec2(0, 0) );
		ImGui::SetNextWindowSize( ImVec2(320, 240) );
		ImGui::Begin("roms list", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar );

		ImGui::SetNextItemWidth(-1);
		if(ImGui::ListBox("Roms", &currentItem, GetRomNameForList, (void*)&roms, roms.size(), 10)) selection_changed = true;

		int buttonWidth = (ImGui::GetContentRegionAvail().x - 6) / 2;
		if( ImGui::ColoredButton("Configure", 0.55f, ImVec2(buttonWidth, 32)) ) configure = true;
		ImGui::SameLine();
		if( ImGui::ColoredButton("Launch",    0.40f, ImVec2(buttonWidth, 32)) ) selected = true;

		ImGui::End();
		ImGui::Render();
		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

		glFinish();
		pglSwapBuffers();

		//Draw top screen
		if(selection_changed)
		{
			std::shared_ptr<CNativeTexture>	previewTexture = nullptr;


			std::filesystem::path preview_file =roms.at(currentItem).mSettings.Preview.c_str();
			std::filesystem::path preview_path = "Resources/Preview";
			preview_path /= preview_file;
			
			previewTexture = CNativeTexture::CreateFromPng( preview_path, TexFmt_5650 );

			pglSelectScreen(GFX_TOP, GFX_LEFT);
			glDisable(GL_SCISSOR_TEST);
			glClear(GL_COLOR_BUFFER_BIT);

			ImGui_Impl3DS_NewFrame(GFX_TOP);
		
			ImGui::SetNextWindowPos(  ImVec2(0, 0) );
			ImGui::SetNextWindowSize( ImVec2(400, 240) );
			ImGui::Begin("Rom Selection", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse );

			ImGui::Text("Game Name: %s", roms.at(currentItem).mSettings.GameName.c_str());
			ImGui::Text( "Country: %s", ROM_GetCountryNameFromID( roms.at(currentItem).mRomID.CountryID ) );
			
			if(previewTexture)
			{
				ImVec2 imageSize( previewTexture->GetWidth(), previewTexture->GetHeight() );
				ImVec2 uv0( 0, 0 );
				ImVec2 uv1( imageSize.x *previewTexture->GetScaleX(), imageSize.y *previewTexture->GetScaleY() );

				ImGui::Image( (void*)previewTexture->GetTextureId(), imageSize, uv0, uv1 );
			}

			if( !roms.at(currentItem).mSettings.Comment.empty() )
				ImGui::Text("Comment: %s", roms.at(currentItem).mSettings.Comment.c_str());

			ImGui::End();
			ImGui::Render();
			ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

			selection_changed = false;
		}
		
		if(configure)
		{
			UI::LoadRomPreferences( roms.at(currentItem).mRomID );

			while( UI::DrawOptionsPage(roms.at(currentItem).mRomID) )
			{
				aptMainLoop();
				gspWaitForVBlank();
				pglSwapBuffers();
				hidScanInput();
			}

			configure = false;
		}
		
	}

	ImGui_Impl3DS_EnableGamepad(false);

	return roms.at(currentItem).mFilename;
}