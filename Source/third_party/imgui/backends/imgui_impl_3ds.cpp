// dear imgui: Platform Backend for GLUT/FreeGLUT
// This needs to be used along with a Renderer (e.g. OpenGL2)

// !!! GLUT/FreeGLUT IS OBSOLETE SOFTWARE. Using GLUT is not recommended unless you really miss the 90's. !!!
// !!! If someone or something is teaching you GLUT in 2020, you are being abused. Please show some resistance. !!!
// !!! Nowadays, prefer using GLFW or SDL instead!

// Issues:
//  [ ] Platform: GLUT is unable to distinguish e.g. Backspace from CTRL+H or TAB from CTRL+I
//  [ ] Platform: Missing mouse cursor shape/visibility support.
//  [ ] Platform: Missing clipboard support (not supported by Glut).
//  [ ] Platform: Missing gamepad support.

// You can copy and use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2019-04-03: Misc: Renamed imgui_impl_freeglut.cpp/.h to imgui_impl_glut.cpp/.h.
//  2019-03-25: Misc: Made io.DeltaTime always above zero.
//  2018-11-30: Misc: Setting up io.BackendPlatformName so it can be displayed in the About Window.
//  2018-03-22: Added GLUT Platform binding.

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_3ds.h"

#include <3ds.h>

static uint64_t g_Time = 0; // Current time, in milliseconds

bool ImGui_Impl3DS_Init()
{
    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;

    io.BackendPlatformName = "3DS";
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

    g_Time = osGetTime();

    return true;
}

void ImGui_Impl3DS_Shutdown()
{
}

void ImGui_Impl3DS_EnableGamepad(bool enable)
{
    ImGuiIO& io = ImGui::GetIO();

    if(enable)
        io.ConfigFlags |=  ImGuiConfigFlags_NavEnableGamepad;
    else
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
}

static void ImGui_Impl3DS_UpdateTouch()
{
    static bool touch_held_previous = false;

    ImGuiIO& io = ImGui::GetIO();

    touchPosition touch;
    hidTouchRead(&touch);

    if(hidKeysHeld() & KEY_TOUCH)
    {
        io.MousePos = ImVec2((float)touch.px, (float)touch.py);

        if(touch_held_previous == false) io.MouseDown[0] = true;

        touch_held_previous = true;
    }
    else if(touch_held_previous == true)
    {
        touch_held_previous = false;
        io.MouseDown[0] = false;
    }
    else
    {
        io.MousePos = ImVec2(-100.0f, -100.0f);
    }
}

static void ImGui_Impl3DS_UpdateGamepad()
{
    ImGuiIO& io = ImGui::GetIO();

    uint32_t keys = hidKeysHeld();

    io.NavInputs[ImGuiNavInput_Activate]  = (keys & KEY_A)      ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_Cancel]    = (keys & KEY_B)      ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_Input]     = (keys & KEY_X)      ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_Menu]      = (keys & KEY_Y)      ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_DpadLeft]  = (keys & KEY_DLEFT)  ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_DpadRight] = (keys & KEY_DRIGHT) ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_DpadUp]    = (keys & KEY_DUP)    ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_DpadDown]  = (keys & KEY_DDOWN)  ? 1.0f : 0.0f;
}

void ImGui_Impl3DS_NewFrame(gfxScreen_t screen)
{
    ImGuiIO& io = ImGui::GetIO();

    if(screen == GFX_BOTTOM)
        io.DisplaySize = ImVec2((float)320, (float)240);
    else
        io.DisplaySize = ImVec2((float)400, (float)240);

     // Setup time step
    uint64_t current_time  = osGetTime();
    uint64_t delta_time_ms = (current_time - g_Time);

    if (delta_time_ms <= 0)
        delta_time_ms = 1;

    io.DeltaTime = delta_time_ms / 1000.0f;
    g_Time = current_time;
    
    ImGui_Impl3DS_UpdateTouch();

    if(io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad)
        ImGui_Impl3DS_UpdateGamepad();

    // Start the frame
    ImGui::NewFrame();
}

void ImGui::StyleCustomDark(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_Border]                 = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.38f, 0.38f, 0.38f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
    colors[ImGuiCol_Separator]              = ImVec4(0.00f, 0.00f, 0.00f, 0.15f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.70f, 0.65f, 0.60f, 0.30f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.70f, 0.65f, 0.60f, 0.65f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive]              = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabUnfocused]           = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImLerp(colors[ImGuiCol_TabActive],    colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    style->WindowPadding = ImVec2(4, 8);
    style->FramePadding  = ImVec2(6, 4);
    style->ItemSpacing   = ImVec2(6, 4);

    style->ScrollbarSize = 18;

    style->WindowBorderSize = 1;
    style->ChildBorderSize  = 1;
    style->PopupBorderSize  = 1;
    style->FrameBorderSize  = 1; 

    style->WindowRounding    = 3;
    style->ChildRounding     = 3;
    style->PopupRounding     = 3;
    style->FrameRounding     = 3;
    style->ScrollbarRounding = 2;
    style->GrabRounding      = 3;
}

bool ImGui::ColoredButton(const char* label, float hue, const ImVec2& size)
{
    ImGui::PushStyleColor(ImGuiCol_Border, (ImVec4)ImColor::HSV(hue, 0.5f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue, 0.8f, 0.8f));
    bool result = ImGui::Button(label, size);
    ImGui::PopStyleColor(4);

    return result;
}