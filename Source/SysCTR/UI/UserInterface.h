#pragma once
#include <stdint.h>
#include "third_party/imgui/imgui.h"
#include "third_party/imgui/backends/imgui_impl_3ds.h"
#include "third_party/imgui/backends/imgui_impl_opengl2.h"

namespace UI
{
	void Initialize();
	void RestoreRenderState();
}