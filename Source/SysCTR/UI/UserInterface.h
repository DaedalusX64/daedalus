#pragma once
#include <stdint.h>
#include <imgui.h>
#include <imgui_impl_3ds.h>
#include <imgui_impl_opengl2.h>

namespace UI
{
	void Initialize();
	void RestoreRenderState();
}