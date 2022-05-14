// dear imgui: Platform Backend for 3DS/PicaGL
// This needs to be used along with a Renderer (e.g. OpenGL2)

#pragma once

#include "imgui.h"      // IMGUI_IMPL_API
#include <3ds.h>

IMGUI_IMPL_API bool     ImGui_Impl3DS_Init();
IMGUI_IMPL_API void     ImGui_Impl3DS_Shutdown();
IMGUI_IMPL_API void     ImGui_Impl3DS_NewFrame(gfxScreen_t screen = GFX_BOTTOM);
IMGUI_IMPL_API void     ImGui_Impl3DS_EnableGamepad(bool enable);

namespace ImGui
{
	IMGUI_API void      StyleCustomDark(ImGuiStyle* dst = NULL);    // Custom dark style
	IMGUI_API bool      ColoredButton(const char* label, float hue, const ImVec2& size = ImVec2(0, 0));   // button
}